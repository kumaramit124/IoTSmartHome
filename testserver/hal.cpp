#include<fcntl.h>
#include<sys/ioctl.h>
#include<time.h>
#include <functional>

#include <pthread.h>
#include <mutex>
#include <condition_variable>

#include "OCPlatform.h"
#include "OCApi.h"


using namespace OC;
using namespace std;
namespace PH = std::placeholders;

#include "server_param.h"
#include "hal.h"

void *intensity_controller(void *param)
{ 
  struct timespec time_req;
  struct timespec time_rem;
  class LaviResource_server *serverparam = (LaviResource_server*) param;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

  time_req.tv_sec = 0;
  while (1)
  {
    time_req.tv_nsec = serverparam->delay_ns; 
    if (nanosleep(&time_req,&time_rem) == -1)
      break;
    if(serverparam->fd_intensity>0)
    {
      if(!ioctl(serverparam->fd_intensity,IOCTL_SET_FOR_INTENSITY, &serverparam->res_intens))
      {
        if(serverparam->delay_ns > 0)
        serverparam->res_intens.pin_value ^= 1;
        else if(serverparam->delay_ns == 0)
        {
              if(serverparam->m_power == 100)
                 serverparam->res_intens.pin_value = 1;
              else if(serverparam->m_power == 0)
                 serverparam->res_intens.pin_value = 0;
        }
      }
    }

  }
  pthread_exit(NULL);
  return NULL;
}

int LaviResource_server::get_sensor_value()
{
  if(fd_sensor > 0)
  { 
    return ioctl(fd_sensor,IOCTL_GET_FROM_SENSOR, &res_sens)==0?res_sens.pin_value:-1;
  }
  return -1;        
}

int LaviResource_server::set_intensity()
{
  if (m_power == 100)
  {
    res_intens.pin_value = 1;
    delay_ns=0;
  }
  else if (m_power == 0)
  {
    res_intens.pin_value = 0;
    delay_ns=0;
  }
  else if(m_power >=10 && m_power <=100)
  {
    res_intens.pin_value = 0;
    delay_ns = (long int)(100/m_power);
  }
  else
    return -1;
  return 0;     
}

int LaviResource_server::get_switch_state()
{
  if(fd_switch > 0)
  { 
    return ioctl(fd_switch,IOCTL_GET_FROM_SWITCH, &res_switch)==0?res_switch.pin_value:-1;
  }
  return -1;        
}

int LaviResource_server::set_switch_state()
{
  if (fd_switch > 0)
  {
    return ioctl(fd_switch,IOCTL_SET_TO_SWITCH, &res_switch)==0?0:-1;
  }
  return -1;
}

int LaviResource_server::stop_intensity()
{
  void *res;
  int  ret;
  if(fd_intensity>0)
  {
    ret= pthread_cancel(intensityId);
    if (ret != 0)
      printf("intensity pthread_cancel error\n");
    else
    {
      /* Join with thread to see what its exit status was */
      ret = pthread_join(intensityId, &res);
      if (ret != 0)
        printf("intensity pthread_join error\n");
      else
      {
        if (res == PTHREAD_CANCELED)
        {
          printf("intensity thread cancealed\n");
        }
      }  
    }
    close(fd_intensity);
    return 0;
  }
  return -1;
}

int LaviResource_server::start_intensity(class LaviResource_server *param)
{
  char intensity_device[16];
  memset(intensity_device,0,sizeof(intensity_device));
  sprintf(intensity_device,"/dev/intensity%d",res_intens.pin_number);
  fd_intensity = open(intensity_device,O_RDWR);
  if(fd_intensity > 0)
  {
    param->res_intens.pin_value = 0;
    param->delay_ns=0;
    if(pthread_create(&intensityId, NULL, intensity_controller,(void *)param))
    {
      close(fd_intensity);
      return -1;
    }
  }
  else
  {
    std::cout<<intensity_device<<" open error"<<std::endl;
  }
  return fd_intensity;       
}

int LaviResource_server::open_hal_switch_device()
{
  char switch_device[16];
  memset(switch_device,0,sizeof(switch_device));
  sprintf(switch_device,"/dev/switch%d",res_switch.pin_number);
  fd_switch = open(switch_device,O_RDWR);
  if(fd_switch < 0)
     std::cout<<switch_device<<" open error"<<std::endl;
  else
     std::cout<<switch_device<<" file desc "<<fd_switch<<std::endl;
  return fd_switch;
}

int LaviResource_server::open_hal_sensor_device()
{
  char sensor_device[16] = {"/dev/sensor"};
  memset(sensor_device,0,sizeof(sensor_device));
  sprintf(sensor_device,"/dev/sensor%d",res_sens.pin_number);
  fd_sensor = open(sensor_device,O_RDONLY);
  if(fd_sensor < 0)
     std::cout<<sensor_device<<" open error"<<std::endl;
  return fd_sensor;
}

void LaviResource_server::close_hal_switch_device()
{
  if(fd_switch>0)
    close(fd_switch);
}

void LaviResource_server::close_hal_sensor_device()
{
  if(fd_sensor>0)
    close(fd_sensor);
}



