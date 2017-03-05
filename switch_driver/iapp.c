#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<fcntl.h>
#include<time.h>
#include<signal.h>
#include<sys/ioctl.h>
int fd;

struct iot_resource 
{
 int pin_number;
 int pin_value;
}res;

main ()
{
fd=open("/dev/switch0",O_RDWR);
if(fd<0)
{
perror("file opening error");
exit(0);
}
res.pin_number =1;
res.pin_value =0;
while(1)
{
ioctl(fd,1,&res);
ioctl(fd,2,&res);
ioctl(fd,3,&res);
ioctl(fd,4,&res);
}
close(fd);
}

