#include<stdlib.h>
#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include "OCPlatform.h"
#include "OCApi.h"
#include "iot_api.h"
#include "resource_param.h"
#include "resource_xml.h"

using namespace OC;

typedef struct resource_list node;
node *head = NULL;

char *getipadr_from_hostaddr(char *hostaddress)
{
  unsigned int i=0;
  static char ipaddress[16];
  char *ptr = strstr((char *)hostaddress,"/");
  ptr = ptr+2;
  memset(ipaddress,0,sizeof(ipaddress));
  if(ptr)
  {
    for (i=0; i<strlen(ptr);i++)
    {
      if(i < 16)
      {
        if(ptr[i] != ':') {
          ipaddress[i]= ptr[i];
        }
        else
          break;
      }
    }
  }
  return ipaddress;
}

bool insert(std::shared_ptr<OCResource> Resource, ResourceParam *resparam)
{
  node *pointer = NULL;
  if(head == NULL )
  {
    head = (node *)malloc(sizeof(node)); 
    if(head)
    {	
      head->Resource = Resource;
      head->resparam = resparam;
      head->next = NULL;
    }
    else
      return 1;
  }
  else
  { 
    pointer = head;
    /* Iterate through the list till we encounter the last node.*/
    while(pointer->next!=NULL)
    {
      pointer = pointer->next;
    }

    /* Allocate memory for the new node and put data in it.*/
    pointer->next = (node *)malloc(sizeof(node));
    if(pointer->next)
    {
      pointer = pointer->next;
      pointer->Resource = Resource;
      pointer->resparam = resparam;
      pointer->next = NULL;
    }
    else
      return 1;

  }
  return 0;
}

bool delete_node(std::string hostAddress, std::string resourceURI)
{
  node *pointer, *prev;
  pointer=head;

  while(pointer!=NULL)
  {
    if((((pointer->Resource->uri().compare(resourceURI)) == 0) && ((pointer->Resource->host().compare(hostAddress)) == 0) ) || 
        (((pointer->Resource->uri().compare(resourceURI)) == 0) && (strcmp(getipadr_from_hostaddr((char *)pointer->Resource->host().c_str()),getipadr_from_hostaddr((char *)hostAddress.c_str())) == 0)))
    {
      if(pointer==head)
      {
        head=pointer->next;
        delete pointer->resparam;
        free(pointer);
        return 0;

      }
      else
      {
        prev->next=pointer->next;
        delete pointer->resparam;
        free(pointer);
        return 0;
      }
    }
    else
    {
      prev=pointer;
      pointer= pointer->next;
    }
  }
  return 1;
}

struct resource_list * find_by_host(std::string hostAddress, std::string resourceURI)
{
  node *pointer = head; //First node is dummy node.
  /* Iterate through the entire linked list and search for the key. */
  while(pointer!=NULL)
  {
    //if(((pointer->Resource->uri().compare(resourceURI)) == 0) && ((pointer->Resource->host()).compare(hostAddress)) == 0) ) 

    if(((pointer->Resource->uri().compare(resourceURI)) == 0))   
    {
      if(((pointer->Resource->host()).compare(hostAddress)) == 0)
      {
        return pointer;
      }
      else if(strcmp(getipadr_from_hostaddr((char *)pointer->Resource->host().c_str()),getipadr_from_hostaddr((char *)hostAddress.c_str())) == 0) 
      {
        return pointer;
      }
    }
    pointer = pointer->next;//Search in the next node.
  }
  /*Key is not found */
  return NULL;
}

struct resource_list *find_by_ip(std::string ipaddress, std::string resourceURI)
{
  node *pointer = head; //First node is dummy node.
  /* Iterate through the entire linked list and search for the key. */
  while(pointer!=NULL)
  {
    if(((pointer->Resource->uri().compare(resourceURI)) == 0) && (strcmp(getipadr_from_hostaddr((char *)pointer->Resource->host().c_str()),ipaddress.c_str()) == 0)) 
    {
      return pointer;
    }
    pointer = pointer->next;//Search in the next node.
  }
  /*Key is not found */
  return NULL;
}

void print_to_file()
{
  node *pointer = head;
  // flush the old /tmp/resource.xml contents
  init_file();        
  while(pointer != NULL)
  {
    create_entry_in_file(pointer->Resource->uri().c_str(),getipadr_from_hostaddr((char *)pointer->Resource->host().c_str())); 
    pointer = pointer->next;
  }
}

void free_list()
{
  node *pointer = head;
  node *tmp;
  while(pointer != NULL)
  {
    tmp = pointer;                       
    pointer = pointer->next;
    delete tmp->resparam;
    free(tmp);
  }
}

struct resource_list *get_resource_head(void)
{
  return head;
}

