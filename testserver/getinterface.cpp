#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
char *getinterfaceip()
{
  struct ifaddrs *ifaddr, *ifa;
  //int family, s;
  int s;
  static char host[NI_MAXHOST];

  if (getifaddrs(&ifaddr) == -1) 
  {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }

  memset(host, 0, sizeof(host));

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) 
  {
    if (ifa->ifa_addr == NULL)
      continue;  

    s=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

    //if((strcmp(ifa->ifa_name,"ra0")==0)&&(ifa->ifa_addr->sa_family==AF_INET))
    if((ifa->ifa_addr->sa_family==AF_INET))
    {
      if (s != 0)
      {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }
      if(strcmp(host,"127.0.0.1")!=0)
      {
        printf("\tInterface : <%s>\n",ifa->ifa_name );
        printf("\t  Address : <%s>\n", host); 
        break;
      }
    }
  }

  freeifaddrs(ifaddr);
  return (host);
}
