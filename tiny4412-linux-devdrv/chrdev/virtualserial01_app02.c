#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define DEVNAME "/dev/virtualFIFO"

int main(int argc, char **argv)
{
	char buffer[64];
	int fd;
	int ret;
	size_t len;
	char message[80] = "Testing the virtual FIFO device.";
	char * read_buffer;

	len = sizeof(message);

	read_buffer = malloc(2*len);
	memset(read_buffer, 0, 2*len);

	fd = open(DEVNAME, O_RDWR | O_NONBLOCK);
	if (fd < 0)
	{
		printf("Open device %s failed.\n", DEVNAME);
		return -1;
	}
	
	/*1. 先读数据*/
	ret = read(fd, read_buffer, 2*len);
	if (!ret)
	{
		printf("Cannot read on device %d, ret = %d.\n", fd, ret);
		return -1;
	}

	printf("read %d bytes.\n", ret);
	printf("read buffer = %s.\n", read_buffer);

	/*2. 写数据到设备中*/
	ret = write(fd, message, len);
	if (ret != len)
	{
		printf("Cannot write on device %d, ret = %d.\n", fd, ret);
		return -1;
	}

	/*3. 再次写数据到设备中*/
        ret = write(fd, message, len);
        if (ret != len)
        {
                printf("Cannot write on device %d, ret = %d.\n", fd, ret);
                return -1;
        }

	/*4. 最后读取*/
	ret = read(fd, read_buffer, 2*len);
	if (!ret)
	{
		printf("Cannot read on device %d, ret = %d.\n", fd, ret);
		return -1;
	}

	printf("read %d bytes.\n", ret);
	printf("read buffer = %s.\n", read_buffer);

	close(fd);

	return 0;
}
