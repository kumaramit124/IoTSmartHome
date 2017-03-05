#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include<pthread.h>
#include "OCPlatform.h"
#include "OCApi.h"
#include "ui_request.h"
using namespace OC;


#define BACKLOG 10
static int UIPORT = 3490;
static pthread_t UiRecvid;
static int client_fd;

void *ReceiverStart(void *arg)
{
  struct sockaddr_in server;
  struct sockaddr_in dest;
  int socket_fd, num;
  socklen_t size;

  char inbuffer[1024];
  char resbuffer[1024];
  int yes =1;

  int ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (ret != 0)
  {
    pthread_exit(NULL);
    return NULL;
  }

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
    fprintf(stderr, "Socket failure!!\n");
    pthread_exit(NULL);
    return NULL;
  }

  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    pthread_exit(NULL);
    return NULL;
  }
  memset(&server, 0, sizeof(server));
  memset(&dest,0,sizeof(dest));
  server.sin_family = AF_INET;
  server.sin_port = htons(UIPORT);
  server.sin_addr.s_addr = INADDR_ANY; 
  if ((bind(socket_fd, (struct sockaddr *)&server, sizeof(struct sockaddr )))== -1)    { //sizeof(struct sockaddr) 
    pthread_exit(NULL);
    return NULL;
  }

  if ((listen(socket_fd, BACKLOG))== -1){
    pthread_exit(NULL);
    return NULL;
  }

  while(1) {
    size = sizeof(struct sockaddr_in);  

    if ((client_fd = accept(socket_fd, (struct sockaddr *)&dest, &size))==-1) {
      pthread_exit(NULL);
      return NULL;
    }
    printf("Server got connection from client %s\n", inet_ntoa(dest.sin_addr));
    while(1)
    {
      memset(inbuffer,0,sizeof(inbuffer));
      memset(resbuffer,0,sizeof(resbuffer));
      if ((num = recv(client_fd, inbuffer, 1024,0))== -1) {
        printf("Connection problem\n");
        break; 
      }   
      else if (num == 0) {
        printf("Connection closed\n");
        break;        
      }
      else
      {
        inbuffer[num] = '\0';
        printf("Message received: %s\n", inbuffer);
        process_ui_request(inbuffer, resbuffer);
        if ((send(client_fd,resbuffer, strlen(resbuffer),0)) == -1) {
          printf("Send Data error\n");
          break;
        }			
      }
    }
    printf("Closing Client Connection\n");
    close(client_fd);
    client_fd = 0;
  }
  close(socket_fd);   
  pthread_exit(NULL);
  return NULL;
}

int send_file_to_ui(char *resbuffer)
{
  if(client_fd <= 0)
  {
    return -1;
  }
  if ((send(client_fd,resbuffer, strlen(resbuffer),0)) == -1) {
    printf("Send Data error\n");
    return -1;
  } 

  return 0;
}

int StartUI(int port)
{
  int ret; 
  UIPORT = port;
  ret= pthread_create(&UiRecvid, NULL, &ReceiverStart, NULL);
  if(ret)
    std::cout << "UI receiver thread is not created" << std::endl;
  return ret; 
}

int StopUi(void)
{ 
  void *res;
  int ret = pthread_cancel(UiRecvid);
  if (ret != 0)
    std::cout<<"pthread_cancel error\n";
  else
  {
    /* Join with thread to see what its exit status was */
    ret = pthread_join(UiRecvid, &res);
    if (ret != 0)
      std::cout<<"pthread_join error\n";
    else
    {
      if (res == PTHREAD_CANCELED)
      {
        std::cout<<"UI receiver thread cancealed\n";
      }
    }  
  }
  return ret;
}
