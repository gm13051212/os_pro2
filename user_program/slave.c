#define _GNU_SOURCE
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

#define PAGE_SIZE 4096
#define BUF_SIZE 512

int main (int argc, char* argv[])
{
    char dmesgMess[200];
    char buf[BUF_SIZE];
    int i, dev_fd, file_fd;
    size_t ret, file_size = 0, data_size = -1;
    char file_name[50][50] = {0};
    char method[20];
    char ip[20];
    struct timeval start;
    struct timeval end;
    double trans_time; 
    long mmap_size = sysconf(_SC_PAGE_SIZE);

    int q = 0;
    char *p;
    int file_times = strtol(argv[1], &p, 10);
    for (; q < file_times; ++q) {
        strcpy(file_name[q], argv[q + 2]);
    }
    strcpy(method, argv[q + 2]);
    strcpy(ip, argv[q + 3]);
    

    if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)
    {
    	perror("failed open \n");
    	return 1;
    }
    for (int times = 0; times < file_times; ++times) {
        gettimeofday(&start ,NULL);
        if( (file_fd = open (file_name[times], O_RDWR | O_CREAT | O_TRUNC)) < 0)
        {
            perror("wrong input file\n");
            return 1;
        }

        if(ioctl(dev_fd, 0x12345677, ip) == -1)	
        {
            perror("ioclt  error\n");
            return 1;
        }

        write(1, "ioctl success\n", 14);

        char *dst;
        
        file_size ＝ 0;
        switch(method[0])
        {
            case 'f': //fcntl  
                
                do
                {
                    ret = read(dev_fd, buf, sizeof(buf)); 
                    write(file_fd, buf, ret); 
                    file_size += ret;
                } while(ret > 0);
                break;
            case 'm': //mmap

                ftruncate(file_fd, mmap_size);
                if((dst = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0)) == (void *) -1) {
                    perror("map output file");
                    return 1;
                }
                while ((ret = read(dev_fd, buf, sizeof(buf))) > 0)
                {
                    memcpy(&dst[file_size%mmap_size], buf, ret);
                    file_size += ret;
                    if (file_size % mmap_size == 0) {
                        ioctl(dev_fd, 0x12345676, (unsigned long)dst);
                        munmap(dst, mmap_size);
                        ftruncate(file_fd, file_size + mmap_size);
                        if((dst = mmap(NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, file_size)) == (void *) -1) {
                                perror("map output file");
                            return 1;
                        }
                    }
                };

                ftruncate(file_fd, file_size);
                ioctl(dev_fd, 0x12345676, (unsigned long)dst);
                munmap(dst, mmap_size);
                //sprintf(dmesgMess, "8000000%d%X\n", rand() % 10, rand());
                // syscall(335,dmesgMess);
                //printf("[ %d.%d] 8000000%d%X\n", rand() % 1000, rand() % 1000000, rand() % 10, rand());
                break;
        }

        if(ioctl(dev_fd, 0x12345679) == -1)
        {
            perror("ioclt error\n");
            return 1;
        }
        gettimeofday(&end, NULL);
        trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
        printf("Transmission time: %lf ms, File size: %ld bytes\n", trans_time, file_size);

        close(file_fd);
        }
        
        close(dev_fd);
    return 0;
}


