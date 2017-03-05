#ifndef LAVI_HAL
#define LAVI_HAL
#include <linux/ioctl.h>

#define MAGIC_NUMBER 251
#define IOCTL_SET_FOR_INTENSITY _IOW(MAGIC_NUMBER, 0, int)
#define IOCTL_GET_FROM_SWITCH _IOR(MAGIC_NUMBER, 1, int)
#define IOCTL_SET_TO_SWITCH _IOWR(MAGIC_NUMBER, 2, int)
#define IOCTL_GET_FROM_SENSOR _IOR(MAGIC_NUMBER, 3, int)

#endif

