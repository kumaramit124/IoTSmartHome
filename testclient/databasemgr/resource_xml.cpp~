#include <stdio.h>
#include <unistd.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <string.h>
#include<fcntl.h>
#include <pthread.h>
#include "cloud_iot_common.h"
#include "cloud_iot_api.h"
#include "UIManager.h"

#define FILE_RES_PATH "/tmp/resource.xml"
static int whom;
static pthread_mutex_t resource_lock;

void *send_resource_file_external(void *arg)
{
  int ret  = 0;
  char buffer[256];
  FILE *fp;
  pthread_mutex_lock(&resource_lock);
  printf("calling resourcce file send and called by %d\n",whom);
  fp = fopen(FILE_RES_PATH, "r");
  if(fp == NULL)
  {
    printf("could not get file error\n");
    pthread_mutex_unlock(&resource_lock);
    pthread_exit(NULL);        
    return NULL;
  } 
  if(whom == CALLED_BY_CLOUD)
    ret = send_file_to_cloud("#resource list file\n",'w');
  else if ( whom == CALLED_BY_UI)
    ret = send_file_to_ui((char *)"#resource list file\n");
  else
  {
    printf("towhom send error\n");
    pthread_mutex_unlock(&resource_lock);
    pthread_exit(NULL);        
    return NULL; 
  }
  if(!ret )
  {
    while(1)
    {
      memset(buffer,0,sizeof(buffer));
      if (fscanf(fp,"%255[^\z]s",buffer) == EOF )
      {
        break;
      }
      else {
        if(whom == CALLED_BY_CLOUD)
          ret = send_file_to_cloud(buffer,'a');
        else if ( whom == CALLED_BY_UI)
          ret = send_file_to_ui(buffer);
        else
          break;

        if (ret)
        {
          break;
        }
      }
    }
  }
  else
    printf("send file error\n");
  fclose(fp);
  pthread_mutex_unlock(&resource_lock);
  pthread_exit(NULL);
  return NULL;
}


int send_resource_file(int towhom)
{
  pthread_t threadid;
  whom = towhom;
  //passing whom as a thread argument is giving the garbage value so static int is taken. need to check why it is happening.
  return pthread_create(&threadid, NULL, &send_resource_file_external, NULL);
}

int create_entry_in_file(const char *uri, const char *hostaddr)
{
  int ret  = -1;
  FILE *fp;
  pthread_mutex_lock(&resource_lock);
  fp = fopen(FILE_RES_PATH, "a");
  if(fp == NULL)
  {
    pthread_mutex_unlock(&resource_lock);
    return -1;
  } 
  if(uri && hostaddr)
  {  
    fprintf(fp,"uri=%s host=%s\n",uri,hostaddr);
  }
  fclose(fp);
  pthread_mutex_unlock(&resource_lock);
  return ret;
}

void init_file()
{
  FILE *fp;
  int res = pthread_mutex_init(&resource_lock, NULL);
  if (res != 0) {
    perror("Resouce Mutex initialization failed");
    return ;
  }
  fp = fopen(FILE_RES_PATH,"w");
  if(fp != NULL)
    fclose(fp);
}
