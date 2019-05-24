#ifndef _IO_CMD_H_
#define _IO_CMD_H_
#define LED_FLAG    'h'

#define IOCTL_LED_ALLON             _IO(LED_FLAG, 1)
#define IOCTL_LED_ONEON             _IO(LED_FLAG, 2)
#define IOCTL_LED_ONEOFF            _IO(LED_FLAG, 3)
#define IOCTL_LED_ALLOFF            _IO(LED_FLAG, 4)
#endif
