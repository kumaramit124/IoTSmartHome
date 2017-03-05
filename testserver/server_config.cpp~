#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include<fcntl.h>

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

typedef struct server_resource_list server_node;
static server_node *server_head = NULL;

static bool server_insert(LaviResource_server *Resource)
{
  server_node *pointer = NULL;
  if(server_head == NULL )
  {
    server_head = (server_node *)malloc(sizeof(server_node)); 
    if(server_head)
    {	
      server_head->Resource = Resource;
      server_head->next = NULL;
    }
    else
      return 1;
  }
  else
  { 
    pointer = server_head;
    /* Iterate through the list till we encounter the last node.*/
    while(pointer->next!=NULL)
    {
      pointer = pointer->next;
    }

    /* Allocate memory for the new node and put data in it.*/
    pointer->next = (server_node *)malloc(sizeof(server_node));
    if(pointer->next)
    {
      pointer = pointer->next;
      pointer->Resource = Resource;
      pointer->next = NULL;
    }
    else
      return 1;

  }
  return 0;
}

void server_free_list()
{
  server_node *pointer = server_head;
  server_node *tmp;
  while(pointer != NULL)
  {
    tmp = pointer;                       
    pointer = pointer->next;
    delete tmp->Resource;
    free(tmp);
  }
}


struct server_resource_list *server_get_resource_head(void)
{
  return server_head;
}

int server_config_read(void)
{
  LaviResource_server *myResource = NULL;
  char buffer[1024];
  char tbuffer[1024];
  FILE *fp;
  fp = fopen("server_config.conf","r");
  if(!fp)
    return -1;
  while(1)
    {
      memset(buffer,0,sizeof(buffer));
      memset(tbuffer,0,sizeof(tbuffer));
      if (fscanf(fp,"%[^\n]\n",buffer) == EOF )
      {
        break;
      }
      else
      {
          
         if(buffer[0] == '[')
         {
         strncpy(tbuffer,buffer+1,strlen(buffer)-2);
         // create the class object
         myResource = new LaviResource_server;
         if(myResource)
         {
         // insert in to list
         server_insert(myResource); 
         myResource->resourceTypeName = tbuffer;
         std::cout<<"["<<myResource->resourceTypeName<<"]"<<std::endl;
         }
         }
         else
         {
          // initialize the object parameter
          if(myResource) {
            if(strncmp(buffer,"resource_name=",14)==0) {
            myResource->m_Uri = buffer+14;
            std::cout<<"resource_name="<<myResource->m_Uri<<std::endl;
            }
            else if(strncmp(buffer,"room_name=",10)==0) {
            myResource->m_name = buffer +10;
            std::cout<<"room_name="<<myResource->m_name<<std::endl;
            }
	    else if(strncmp(buffer,"gpio_switch_pin=",16)==0) {
            sscanf(buffer,"gpio_switch_pin=%d",&myResource->res_switch.pin_number);
            std::cout<<"gpio_switch_pin="<<myResource->res_switch.pin_number<<std::endl;
            }   
            else if(strncmp(buffer,"gpio_power_pin=",15)==0) {
            sscanf(buffer,"gpio_power_pin=%d",&myResource->res_intens.pin_number);
            std::cout<<"gpio_power_pin="<<myResource->res_intens.pin_number<<std::endl;
            }   
            else if(strncmp(buffer,"gpio_status_pin=",16)==0) {
            sscanf(buffer,"gpio_status_pin=%d",&myResource->res_sens.pin_number);
            std::cout<<"gpio_status_pin="<<myResource->res_sens.pin_number<<std::endl;
            }   
            else if(strncmp(buffer,"resource_type=",14)==0) {
            sscanf(buffer,"resource_type=%d",&myResource->resource_type);
            std::cout<<"resource_type="<<myResource->resource_type<<std::endl;
            }   
            else if(strncmp(buffer,"gpio_sensor_pin=",16)==0) {
            sscanf(buffer,"gpio_sensor_pin=%d",&myResource->res_sens.pin_number);// for temp
            std::cout<<"gpio_sensor_pin="<<myResource->res_sens.pin_number<<std::endl;
            }
            else if(strncmp(buffer,"power_rating=",13)==0) {
            sscanf(buffer,"power_rating=%d",&myResource->power_rating);// for temp
            std::cout<<"power_rating="<<myResource->power_rating<<std::endl;
            }             
          }
         }
   }
    }

  fclose(fp);
  
  return 0;
}

