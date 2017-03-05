#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include "../cloud_iot_common.h"

int (*iotfunctptr)(int bywhom, int cmd, struct iot_resource *);

void registerIotCallback(int (*functptr)(int, int ,struct iot_resource *))
{
  iotfunctptr = functptr;
}

static int SendToIot(int command,struct iot_resource *iotres)
{
  int ret = -1;
  if(iotres)
  {
    if(iotfunctptr) 
      ret =  (iotfunctptr)(CALLED_BY_CLOUD, command,iotres); // the iotres will have data for set and get  
  }
  return ret;
}

int process_cloud_request(char *buf, char *resbuf)
{
  int ret = -1;
  int cmd;
  struct iot_resource *iotres = (struct iot_resource *)malloc(sizeof(struct iot_resource));
  if(iotres)
  {
    memset(iotres,0,sizeof(struct iot_resource));        
    if(buf)
    {
      sscanf(buf,"%d",&cmd);         
      if(cmd == GET_RESOURSE_FILE) // GET_FILE
      {
        ret = SendToIot(cmd,iotres);
      } 
      else if(cmd == GET_RESOURSE_PARAM) // GET
      {      
        sscanf(buf,"%d %s %s",&cmd,iotres->Uri,iotres->host);
        ret = SendToIot(cmd,iotres);          
      }
      else if(cmd == SET_RESOURSE_PARAM) //SET
      {  
        sscanf(buf,"%d %s %s %s %d %d",&cmd,iotres->Uri,iotres->host,iotres->name,&iotres->state,&iotres->power);
        ret = SendToIot(cmd,iotres);
      }
      else if(cmd == POST_RESOURSE_PARAM) //PUT
      {  
        sscanf(buf,"%d %s %s %s %d %d",&cmd,iotres->Uri,iotres->host,iotres->name,&iotres->state,&iotres->power);
        ret = SendToIot(cmd,iotres);
      }
      else if(cmd == SET_RESOURCE_MODE)
      {  
        sscanf(buf,"%d %s %s %s %d %d %d",&cmd,iotres->Uri,iotres->host,iotres->name,&iotres->state,&iotres->power,&iotres->auto_status);
        ret = SendToIot(cmd,iotres);
      }
      else if(cmd == SET_SCHEDULE_RESOURCE)
      {  
        sscanf(buf,"%d %s %s %s %d %d %d %d %d %d %ld %ld %d",&cmd,iotres->Uri,iotres->host,iotres->name,&iotres->state,&iotres->power,&iotres->auto_status,&iotres->schedule_staus,&iotres->schedule_state,&iotres->schedule_power,&iotres->schedule_start_time,&iotres->schedule_end_time,&iotres->is_repeat_next_day);
        ret = SendToIot(cmd,iotres);
      }                     
      sprintf(resbuf,"%d %s %s %s %d %d %d %d %d %d %ld %ld %d %d %f",ret,iotres->Uri,iotres->host,iotres->name,iotres->state,iotres->power,iotres->auto_status,iotres->schedule_staus,iotres->schedule_state,iotres->schedule_power,iotres->schedule_start_time,iotres->schedule_end_time,iotres->is_repeat_next_day,iotres->observer_status,iotres->consume_unit);
      free(iotres);
    }
  }
  else
  {
    sprintf(resbuf,"%d",ret);
  }        
  return ret;
}



