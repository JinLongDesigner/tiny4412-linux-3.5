#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "io_cmd.h"

#define DEV "/dev/Tiny4412-leds"

struct led_protocal
{
	int led_num;
	int light;
	
	int status;
};


int main(int argc, char **argv)
{
	int fd;
	char buf[20];
	int ret;
	
	struct led_protocal lp;

	fd = open(DEV, O_RDWR);
	if (fd < 0)
	{
		printf(" open leds dev failed");
		return -1;
	}

	write(fd, &lp, sizeof(lp));
	read(fd, &lp, sizeof(lp));
	sleep(3);
	ioctl(fd, IOCTL_LED_ALLON, 0);
	sleep(3);
	ioctl(fd, IOCTL_LED_ALLOFF, 0);
	close(fd);
	
	return 0;
}
