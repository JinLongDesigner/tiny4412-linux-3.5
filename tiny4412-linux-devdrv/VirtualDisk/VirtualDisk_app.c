#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#define MEM_CLEAR 0x1				//全局内存清零
#define PORT1_SET 0x2				//将port1端口清零
#define PORT2_SET 0x3				//将port2端口清零

int main(int argc, char **argv)
{
	int fd = 0;
	int ret = 0;
	int len = 0;
	char *p = "This is a c test code";
	char buf[1024] = {0};
	int n = 0;
	fd = open("/dev/VirtualDisk", O_RDWR);
	if (fd < 0)
	{
		printf("Opening /dev/VirtualDisk is failed.\n");
		return 0;
	}
	
	ret = ioctl(fd, MEM_CLEAR);
	if (ret)
	{
		printf("VirtualDisk setting to zero is failed.\n");
		return ret;
	}

	   
	lseek(fd, 0,SEEK_SET);
	write(fd, p, strlen(p));
	
	lseek(fd, 0,SEEK_SET);
	read(fd, buf, strlen(p));
	printf("buf : %s \n", buf);
	
	return 0;
}