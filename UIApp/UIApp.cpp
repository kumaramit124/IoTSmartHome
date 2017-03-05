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
#include<pthread.h>

#define PORT 3490
#define MAXSIZE 256 
static int socket_fd;

void *ReceiveData(void *arg)
{
   char buff[1024];
   int num;   
   while(1)
   {
   if(socket_fd >0)
   { 
    memset(buff, 0 , sizeof(buff));
    if ((num = recv(socket_fd, buff, 1024,0))== -1) {
            perror("recv");
            pthread_exit(NULL);
            return NULL;
    }
    else
    {
      printf("%s\n", buff);
    }   
   }
   }
 pthread_exit(NULL);
 return NULL;
}



int main(int argc, char *argv[])
{
    struct sockaddr_in server_info;
    struct hostent *he;
    int num;
    char buffer[1024];
    
    int ret;
    pthread_t threadid;

    if (argc != 2) {
        fprintf(stderr, "Usage: client hostname\n");
        exit(1);
    }
 
    if ((he = gethostbyname(argv[1]))==NULL) {
        fprintf(stderr, "Cannot get host name\n");
        exit(1);
    }

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
        fprintf(stderr, "Socket Failure!!\n");
        exit(1);
    }

    memset(&server_info, 0, sizeof(server_info));
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(PORT);
    server_info.sin_addr = *((struct in_addr *)he->h_addr);
    if (connect(socket_fd, (struct sockaddr *)&server_info, sizeof(struct sockaddr))<0) {
        //fprintf(stderr, "Connection Failure\n");
        perror("connect");
        exit(1);
    }
    ret = pthread_create(&threadid, NULL, &ReceiveData, NULL);
          if(ret) {
		printf("UIApp receiver thread is not created\n");
                exit(0);
                }
    while(1)
    {
    printf("Enter:: cmd uri hostaddress roomname state intensity\n");    
    memset(buffer, 0 , sizeof(buffer));
        fgets(buffer,MAXSIZE-1,stdin);
        if ((send(socket_fd,buffer, strlen(buffer),0))== -1) {
            fprintf(stderr, "Failure Sending Message\n");
            close(socket_fd);
            exit(1);
        }
        else {
            printf("Message being sent: %s\n",buffer);
        }
     }
    
    
    close(socket_fd);   

}
