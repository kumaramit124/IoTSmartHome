#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include "resource_param.h"

static int file_pipes[2];
static pthread_mutex_t read_lock; 
static pthread_mutex_t write_lock;

int ResourceParam::ErrhandleReqRespInit(void)
{
  int res = pthread_mutex_init(&read_lock, NULL);
  if (res != 0) {
    perror("Mutex initialization failed");
    return -1;
  }

  res = pthread_mutex_init(&write_lock, NULL);
  if (res != 0) {
    perror("Mutex initialization failed");
    return -1;
  }
  if (pipe(file_pipes) == 0) 
    return 0;
  return -1;
}

int ResourceParam::ErrhandlePostRespMessage(int *resp)
{
  int ret;
  pthread_mutex_lock(&write_lock);
  ret = write(file_pipes[1], resp, 4);
  pthread_mutex_unlock(&write_lock);
  return (ret == 0 ? -1 : 0);
}

int ResourceParam::ErrhandleGetRespMessage(int *resp)
{ 
  int ret = 0;
  fd_set set;
  struct timeval timeout;
  pthread_mutex_lock(&read_lock);
  FD_ZERO(&set); /* clear the set */
  FD_SET(file_pipes[0], &set); /* add our file descriptor to the set */
  timeout.tv_sec = 30;
  timeout.tv_usec = 0;
  ret = select(file_pipes[0] + 1, &set, NULL, NULL, &timeout);
  if(ret == -1)
  {
    printf("GETResp - an error occured\n");
    *resp = -1; /* an error accured */
  }
  else if(ret == 0)
  {
    printf("time out occur\n");
    *resp = -1; /* a timeout occured */
  }
  else
    ret = read(file_pipes[0], resp, 4); // need to implement timeout option

  pthread_mutex_unlock(&read_lock);
  return (ret <= 0 ? -1 : 0);
}

#if 0
int ResourceParam::ErrhandleGetRespMessage(int *resp)
{ 
  int ret;
  pthread_mutex_lock(&read_lock);
  ret = read(file_pipes[0], resp, 4); // need to implement timeout option
  pthread_mutex_unlock(&read_lock);
  return (ret == 0 ? -1 : 0);
}
#endif
