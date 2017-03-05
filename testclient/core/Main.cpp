#include<stdio.h>
#include<signal.h>
#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include<fcntl.h>
#include <condition_variable>
#include "OCPlatform.h"
#include "OCApi.h"
#include "configmgr.h"
#include "process_schedular.h"
#include "iot_api.h"
#include "cloud_iot_common.h"
#include "cloud_iot_api.h"
#include "MulticastRecv.h"
#include "UIManager.h"
#include "resource_xml.h"

static pthread_mutex_t api_lock;
int is_process_minitor_running = 0;
static int Cloud_Status = -1;
static int Iot_Status = -1;

int iot_request_response(int whocalled, int cmd , struct iot_resource * iores)
{
  int ret = -1;
  pthread_mutex_lock(&api_lock); 
  if(cmd == GET_RESOURSE_PARAM) // get
  {
    ret = get_sensor(iores);  //thread safe call
  }
  else if (cmd == SET_RESOURSE_PARAM) // set
  {
    ret = set_sensor(iores);
  }
  else if (cmd == POST_RESOURSE_PARAM ) // post
  {
    ret = post_sensor(iores);
  }
  else if (cmd == GET_RESOURSE_FILE) // get resource list file
  {
    init_resource_file();
    ret = send_resource_file(whocalled); // 0 for cloud, 1 for ui
  }
  else if (cmd == DEVICE_CONFIG_FILE_CHANGED)
  {
    //take action       
  }
  else if (cmd == SET_RESOURCE_MODE)
  {
    ret = SetMode(iores);       
  }
  else if (cmd == SET_SCHEDULE_RESOURCE)
  {
    ret = SetScheduling(iores);       
  }


  pthread_mutex_unlock(&api_lock); 
  return ret;
}

static int IotStart(void)
{
  int ret=-1;
  if( Iot_Status == -1)
  {
    if(is_iot_configured == 1)
    {
      ret = iot_start();		
    }
    Iot_Status = ret;
  }
  return ret;
}

static int IotStop(void)
{
  int ret = -1;
  if(Iot_Status == 0)
  {
    if(is_iot_configured == 1)
    {
      ret = iot_stop();
    }
    if(ret == 0)
      Iot_Status = -1;
    else
      Iot_Status = ret;
  }
  return ret;
}

static int CloudStart(void)
{
  int ret = -1;
  if(Cloud_Status == -1)
  {
    if(is_cloud_configured == 1)
    {
      registerIotCallback(iot_request_response);
      ret = cloud_init (cloud_device_mac_addr,cloud_vendorid,cloud_url);
    }
    Cloud_Status = ret;
  }
  return ret;
}

static int CloudStop(void)
{
  int ret = -1;
  if(Cloud_Status == 0)
  {
    if(is_cloud_configured == 1)
    {
      registerIotCallback(NULL);
      ret = cloud_deinit();
    }
    if(ret == 0)
      Cloud_Status = -1;
    else
      Cloud_Status = ret;
  }
  return ret;
}

int isCloudStared()
{
  return(Cloud_Status);
}

int isIotStared()
{
  return(Iot_Status);
}

int SystemStop(void)
{
  int ret;
  ret = IotStop();
  if(ret)
  {
    std::cout << "iot stop failed"<<std::endl;
  }
  else
  {
    ret = StopBcastReceiver();
    if(ret)
    {
      std::cout << "Broadcastreceiver stop failed"<<std::endl;
    }
  }
  ret = CloudStop();
  if(ret)
  {
    std::cout << "Cloud stop failed"<<std::endl;
  }

  return ret;
}


void sigsegv(int t)
{
  printf("ohh I am Killed !!!\n");
  SystemStop();
  StopUi();
  system("killall -9 ihome");
  exit(0);
}

int SystemStart(void)
{
  int ret;
  signal(SIGSEGV, sigsegv);
  signal(SIGINT, sigsegv);
  ret = pthread_mutex_init(&api_lock, NULL);
  if (ret != 0) {
    perror("Resouce app lock Mutex initialization failed");
    return ret;
  }
  ret = IotStart();
  if(ret)
  {
    std::cout << "iot start failed"<<std::endl; 
  }
  else
  {                
    ret = StartBcastReceiver(broadcast_port);
    if(ret)
    {
      std::cout << "Broadcastreceiver start failed"<<std::endl;
    }
    else
    {
      ret = CloudStart();
      if(ret)
      {
        std::cout << "cloud start failed"<<std::endl;
      } 
    }
  }

  return ret;
}

int main(void)
{
  config_read(); // need to checck , all condition for config
  if(!SystemStart())
  {
    StartUI(uiapp_port);
  }
  else {
    SystemStop();
    StopUi();
  }

  while(1);
  return 0; 
}



