#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#define PAGE_SIZE 4096
#define BUF_SIZE 512
size_t getFilesize(const char* filename);


typedef struct {
    uint64_t pfn : 55;
    unsigned int soft_dirty : 1;
    unsigned int file_page : 1;
    unsigned int swapped : 1;
    unsigned int present : 1;
} PagemapEntry;

/* Parse the pagemap entry for the given virtual address.
 *
 * @param[out] entry      the parsed entry
 * @param[in]  pagemap_fd file descriptor to an open /proc/pid/pagemap file
 * @param[in]  vaddr      virtual address to get entry for
 * @return 0 for success, 1 for failure
 */
int pagemap_get_entry(PagemapEntry *entry, int pagemap_fd, uintptr_t vaddr)
{
    size_t nread;
    ssize_t ret;
    uint64_t data;
    uintptr_t vpn;

    vpn = vaddr / sysconf(_SC_PAGE_SIZE);
    nread = 0;
    while (nread < sizeof(data)) {
        ret = pread(pagemap_fd, &data, sizeof(data) - nread,
                vpn * sizeof(data) + nread);
        nread += ret;
        if (ret <= 0) {
            return 1;
        }
    }
    entry->pfn = data & (((uint64_t)1 << 55) - 1);
    entry->soft_dirty = (data >> 55) & 1;
    entry->file_page = (data >> 61) & 1;
    entry->swapped = (data >> 62) & 1;
    entry->present = (data >> 63) & 1;
    return 0;
}

/* Convert the given virtual address to physical using /proc/PID/pagemap.
 *
 * @param[out] paddr physical address
 * @param[in]  pid   process to convert for
 * @param[in] vaddr virtual address to get entry for
 * @return 0 for success, 1 for failure
 */
int virt_to_phys_user(uintptr_t *paddr, /*pid_t pid,*/ uintptr_t vaddr)
{
    char pagemap_file[BUFSIZ];
    int pagemap_fd;

    //snprintf(pagemap_file, sizeof(pagemap_file), "/proc/%ju/pagemap", (uintmax_t)pid);
    snprintf(pagemap_file, sizeof(pagemap_file), "/proc/self/pagemap");
    pagemap_fd = open(pagemap_file, O_RDONLY);
    if (pagemap_fd < 0) {
        return 1;
    }
    PagemapEntry entry;
    if (pagemap_get_entry(&entry, pagemap_fd, vaddr)) {
        return 1;
    }
    close(pagemap_fd);
    *paddr = (entry.pfn * sysconf(_SC_PAGE_SIZE)) + (vaddr % sysconf(_SC_PAGE_SIZE));
    return 0;
}


int main (int argc, char* argv[])
{
    char buf[BUF_SIZE];
    char dmesgMess[200];
    size_t ret, file_size, offset = 0, tmp;
    int i, dev_fd, file_fd;
    char file_name[50][50] = {0};
    char method[20];
    struct timeval start;
    struct timeval end;
    double trans_time; 

    int q = 0;
    char *p;
    int file_times = strtol(argv[1], &p, 10);
    for (; q < file_times; ++q) {
        strcpy(file_name[q], argv[q + 2]);
    }
   
    strcpy(method, argv[q + 2]);
       


   


    for (int times = 0; times < file_times; ++times) {
        offset = 0;

        if( (dev_fd = open("/dev/master_device", O_RDWR)) < 0)
            {
                perror("fail device\n");
                return 1;
            }
        //gettimeofday(&start ,NULL);
        printf("file_name %s\n", file_name[times]);
        if( (file_fd = open (file_name[times], O_RDWR)) < 0 )
        {
            perror("failed to open input file\n");
            return 1;
        }

        if( (file_size = getFilesize(file_name[times])) < 0)
        {
            perror("wrong file size\n");
            return 1;
        }


        if(ioctl(dev_fd, 0x12345677) == -1) 
        {
            perror("ioclt error\n");
            return 1;
        }

        char *src, *dst;
        
        gettimeofday(&start ,NULL);
        switch(method[0]) 
        {
            case 'f': //fcntl 
                do
                {
                    ret = read(file_fd, buf, sizeof(buf)); 
                    //write device
                    write(dev_fd, buf, ret);
                }while(ret > 0);
                break;
            case 'm': //mmap
                while (offset < file_size) {
                    //printf("file_fd %d offset %d\n", file_fd, offset);
                    if((src = mmap(NULL, PAGE_SIZE, PROT_READ, MAP_SHARED, file_fd, offset)) == (void *) -1) {
                        perror("map input file");
                        return 1;
                    }
                    uintptr_t vaddr, paddr = 0;
                    //printf("%p\n", src);
                    
                    
                    if((dst = mmap(NULL, PAGE_SIZE, PROT_WRITE, MAP_SHARED, dev_fd, offset)) == (void *) -1) {
                        perror("map output device");
                        return 1;
                    }
                    
                    do {
                        int len = (offset + BUF_SIZE > file_size ? file_size % BUF_SIZE : BUF_SIZE);
                        memcpy(dst, src, len);
                        offset = offset + len;
                        ioctl(dev_fd, 0x12345678, len);
                    } while (offset < file_size && offset % PAGE_SIZE != 0);
                    ioctl(dev_fd, 0x12345676, (unsigned long)src);
                    virt_to_phys_user(&paddr, (uintptr_t)src);
                    //printf("0x%jx\n", (uintmax_t)paddr);
                    //sprintf(dmesgMess, "0x%jx\n", (uintmax_t)paddr);
                    //syscall(335,dmesgMess);
                    munmap(src, PAGE_SIZE);
                }
                
                break;
        }
        //close the connection
        if(ioctl(dev_fd, 0x12345679) == -1) 
        {
            perror("ioclt exits error");
            return 1;
        }
        gettimeofday(&end, NULL);
        trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
        printf("Transmission time: %lf ms, File size: %ld bytes\n", trans_time, file_size / 8);

        

        close(file_fd);
        close(dev_fd); 
        }
       
        return 0;
}

size_t getFilesize(const char* filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}
