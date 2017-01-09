#ifndef __CHRDEV_IOCTL_H__
#define __CHRDEV_IOCTL_H__

#include <asm-generic/ioctl.h>

#define A_START   _IO('Y',0)
#define B_START   _IO('Y',1)
#define B_STOP    _IO('Y',2)
#define A_CLOSE_B _IO('Y',3)
#define CLOSE_AB  _IO('Y',4)

#endif
