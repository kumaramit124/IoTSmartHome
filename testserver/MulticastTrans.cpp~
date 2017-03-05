#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include<pthread.h>
#include "OCPlatform.h"
#include "OCApi.h"
using namespace OC;

#define ALIVE 1
#define FINDME 2
#define DISCONNECTED 3

#define MAXBUFLEN 1
pthread_t Bcastid;
static int MYPORT=12345;
#define KEEPALIVE_GROUP "225.0.0.37"

extern bool isfound;

void *bcastTrans(void *arg)
{
  struct sockaddr_in addr;
  int fd;
  char msgbuf[MAXBUFLEN]={""};
  int ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (ret != 0)
  {
    pthread_exit(NULL);
    return NULL;
  }
  /* create what looks like an ordinary UDP socket */
  if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
    perror("socket");
    pthread_exit(NULL);
    return NULL;
  }

  /* set up destination address */
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=inet_addr(KEEPALIVE_GROUP);
  addr.sin_port=htons(MYPORT);

  /* now just sendto() our destination! */
  while (1) {
    memset(msgbuf,0,sizeof(msgbuf));
    sleep(10);
    if(isfound == false)
       msgbuf[0] = FINDME;
    else 
       msgbuf[0]= ALIVE;
    
    if (sendto(fd,msgbuf,sizeof(msgbuf),0,(struct sockaddr *) &addr,
          sizeof(addr)) < 0) {
      perror("sendto");
      close(fd);
      pthread_exit(NULL);
      return NULL;
    }
   
  }
  close(fd);
  pthread_exit(NULL);
  return NULL;
}

int StartBcastTrans(int port)
{
  int ret ;
  MYPORT = port;
  ret = pthread_create(&Bcastid, NULL, &bcastTrans, NULL);
  if(ret)
    std::cout << "broadcast receiver thread is not created" << std::endl;
  return ret; 
}

int StopBcastTrans(void)
{ 
  void *res;
  int ret = pthread_cancel(Bcastid);
  if (ret != 0)
    std::cout<<"pthread_cancel error\n";
  else
  {
    /* Join with thread to see what its exit status was */
    ret = pthread_join(Bcastid, &res);
    if (ret != 0)
      std::cout<<"pthread_join error\n";
    else
    {
      if (res == PTHREAD_CANCELED)
      {
        std::cout<<"Broadcast receiver thread cancealed\n";
      }
    }  
  }
  return ret;
}
