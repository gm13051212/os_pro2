cd ./ksocket
make
# make CFLAGS=-DMEOW
insmod ksocket.ko

cd ../master_device
make
rmmod master_device
insmod master_device.ko

cd ../slave_device
make
rmmod slave_device
insmod slave_device.ko

cd ../user_program
make


