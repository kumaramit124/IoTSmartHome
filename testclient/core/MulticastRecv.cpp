#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>



#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include<pthread.h>
#include "OCPlatform.h"
#include "OCApi.h"
#include "iot_api.h"
using namespace OC;

#define ALIVE 1
#define FINDME 2
#define DISCONNECTED 3

#define KEEPALIVE_GROUP "225.0.0.37"
#define MSGBUFSIZE 1
static int MYPORT = 12345;	// the port users will be connecting to
static pthread_t Bcastid;

static int update_resource_live_status(char * ip_addr)
{
  FILE *fp;
  char buffer[256];
  
  fp = fopen("resource_status.txt","r+");
  if(fp == NULL)
  {
     fp = fopen("resource_status.txt","w");
     fprintf(fp,"%s::%ld\n",ip_addr,time(NULL));
     fclose(fp);   
     return 0;
  }
  
  while(1)
  {
  memset(buffer,0,sizeof(buffer));
  if (fscanf(fp,"%s",buffer) == EOF )
  {
       fprintf(fp,"%s::%ld\n",ip_addr,time(NULL));
       break;
  }
  else
  {       
       if(strncmp(buffer,ip_addr,strlen(ip_addr))==0)
       {
         fseek(fp,ftell(fp)-strlen(buffer),0);
         fprintf(fp,"%s::%ld\n",ip_addr,time(NULL));
         break; 
       }
  }
  } 
  fclose(fp);
  return 0;
}

void *bcastReceiver(void *arg)
{
  struct sockaddr_in addr;
  int fd, nbytes;
  socklen_t addrlen;
  struct ip_mreq mreq;
  char msgbuf[MSGBUFSIZE];

  unsigned int yes=1;            /*** MODIFICATION TO ORIGINAL */

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


  /**** MODIFICATION TO ORIGINAL */
  /* allow multiple sockets to use the same PORT number */
  if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0) {
    perror("Reusing ADDR failed");
    pthread_exit(NULL);
    return NULL;
  }
  /*** END OF MODIFICATION TO ORIGINAL */

  /* set up destination address */
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */
  addr.sin_port=htons(MYPORT);

  /* bind to receive address */
  if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
    perror("bind");
    pthread_exit(NULL);
    return NULL;
  }

  /* use setsockopt() to request that the kernel join a multicast group */
  mreq.imr_multiaddr.s_addr=inet_addr(KEEPALIVE_GROUP);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);
  if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
    perror("setsockopt");
    pthread_exit(NULL);
    return NULL;
  }

  while(1)
  {
    addrlen=sizeof(addr);
    if ((nbytes=recvfrom(fd,msgbuf,MSGBUFSIZE,0,
            (struct sockaddr *) &addr,&addrlen)) < 0) {
      perror("recvfrom");
      close(fd);
      pthread_exit(NULL);
      return NULL;
    }
    if(msgbuf[0] ==ALIVE ){
           //update the file
           update_resource_live_status(inet_ntoa(addr.sin_addr));
    }
    else if(msgbuf[0] == FINDME){
           std::cout << "got packet from :" << inet_ntoa(addr.sin_addr) << " and meeasge is FINDME"<< std::endl;
           send_unicast_request(inet_ntoa(addr.sin_addr));
    }
    else if(msgbuf[0] == DISCONNECTED){
          std::cout<<"host: "<< inet_ntoa(addr.sin_addr) << " is disconnected"<<std::endl;
    }
  }
  close(fd);
  pthread_exit(NULL);
  return NULL;
}

int StartBcastReceiver(int port)
{
  int ret ;
  MYPORT = port;
  ret = pthread_create(&Bcastid, NULL, &bcastReceiver, NULL);
  if(ret)
    std::cout << "broadcast receiver thread is not created" << std::endl;
  return ret; 
}

int StopBcastReceiver(void)
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
