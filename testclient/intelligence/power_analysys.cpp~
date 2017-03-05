#include<stdio.h>
#include<fcntl.h>
#include<time.h>
#include<string.h>
#include<iostream>
#include<stdlib.h>
#include <map>
#include <cstdlib>

using namespace std;

#define FILE_DATABASE_PATH "resource_database.csv"

struct resource_load
{
  char ip_address[256];
  char uri[256];
  int old_state;
  int old_power;
  int power_rat;
  time_t prev_time;
  float unit_consume;
  struct resource_load *next;
};
typedef struct resource_load load_node;
load_node *load_head = NULL;

static bool load_insert(char * ip_address, char *uri, int state, int power, time_t prev_time,int power_rat)
{
  load_node *pointer = NULL;
  if(load_head == NULL )
  {
    load_head = (load_node *)malloc(sizeof(load_node)); 
    if(load_head)
    {	
      strcpy((char *)load_head->ip_address,ip_address);
      strcpy((char *)load_head->uri,uri);
      load_head->old_state= state;
      load_head->old_power= power;
      load_head->prev_time=prev_time;
      load_head->power_rat=power_rat;
      load_head->unit_consume=0;
      load_head->next = NULL;

    }
    else
      return 1;
  }
  else
  { 
    pointer = load_head;
    /* Iterate through the list till we encounter the last load_node.*/
    while(pointer->next!=NULL)
    {
      pointer = pointer->next;
    }

    /* Allocate memory for the new load_node and put data in it.*/
    pointer->next = (load_node *)malloc(sizeof(load_node));
    if(pointer->next)
    {
      pointer = pointer->next;
      strcpy((char *)pointer->ip_address,ip_address);
      strcpy((char *)pointer->uri, uri);
      pointer->old_state= state;
      pointer->old_power= power;
      pointer->prev_time=prev_time;
      pointer->power_rat=power_rat;
      pointer->unit_consume=0;
      pointer->next = NULL;
    }
    else
      return 1;

  }
  return 0;
}

static load_node * load_find_load_node(char * hostAddress, char *resourceURI)
{
  load_node *pointer = load_head; //First load_node is dummy load_node.
  /* Iterate through the entire linked list and search for the key. */
  while(pointer!=NULL)
  {
    //if(((pointer->Resource->uri().compare(resourceURI)) == 0) && ((pointer->Resource->host()).compare(hostAddress)) == 0) ) 

    if(strcmp(pointer->uri,resourceURI) == 0)   
    {
      if(strcmp(pointer->ip_address,hostAddress) == 0)
      {
        return pointer;
      }
    }
    pointer = pointer->next;//Search in the next load_node.
  }
  /*Key is not found */
  return NULL;
}

void update_list(char *ip_addr,char *uri,int state, int power,time_t st_time,int power_rat)
{
  load_node *ptr = NULL;
  ptr=load_find_load_node(ip_addr, uri);
  if(!ptr)
  {         
    load_insert(ip_addr, uri,state,power,st_time,power_rat);             
  }
  else
  {         
    if((ptr->old_state ==0 ) && (state ==0)) // it is zero
    {
      ptr->old_power = power;
      ptr->prev_time= st_time;
      ptr->power_rat= power_rat;       
    }     
    else if((ptr->old_state ==0 ) && (state !=0)) // it is switced from 0 to 1
    {
      ptr->old_state = state;
      ptr->old_power = power;
      ptr->prev_time= st_time; 
      ptr->power_rat= power_rat; 
    }
    else if((ptr->old_state !=0 ) && (state ==0)) // it is switced from 1 to 0
    {
      // increase the unit
      ptr->unit_consume += (float)(power_rat * ptr->old_power*(st_time - ptr->prev_time))/(360000);
      ptr->old_state = state;
      ptr->old_power = power;
      ptr->prev_time= st_time; 
    }
    else if((ptr->old_state !=0 ) && (state !=0)) // it is 1 , there may be chance that intensity has changed
    {
      //check for power
      if(ptr->old_power != power)
      {
        // increase the unit
        ptr->unit_consume += (float)(power_rat * ptr->old_power*(st_time - ptr->prev_time))/(360000); 
        ptr->old_power = power;
        ptr->prev_time= st_time; 
      }
    }
  } 
}

float get_power_consumption(char *ip_addr,char *uri)
{
  load_node *ptr = load_find_load_node(ip_addr, uri);
  if(ptr)
  {         
     return(ptr->unit_consume);
  }
   return 0;
}

void stop_load_analysys()
{
  load_node *pointer = load_head;
  load_node *tmp;
  while(pointer != NULL)
  {
    tmp = pointer;            
    pointer = pointer->next;
    free(tmp);
  }
  std::cout<<"Releasing power analysys resource\n";
}

int start_load_analysys()
{
  load_node *pointer = load_head;
  FILE *fp;
  unsigned int i =0;
  char buffer[256];
  char ip_addr[256];
  char room_name[256];
  char uri[256];
  int state;
  int power;
  char start_date_time[256];
  time_t st_time;
  int power_rat;
  int res_type;
  int auto_mode;

  fp = fopen(FILE_DATABASE_PATH,"r");
  if(fp == NULL)
  {
    return 0;
  }
  if (fscanf(fp,"%[^\n]\n",buffer) == EOF )
  {
    fclose(fp);
    return 0;
  }
  while(1)
  {
    memset(buffer,0,sizeof(buffer));
    memset(ip_addr,0,sizeof(ip_addr));
    memset(room_name,0,sizeof(room_name));
    memset(uri,0,sizeof(uri));
    memset(start_date_time,0,sizeof(start_date_time));
    if (fscanf(fp,"%[^\n]\n",buffer) == EOF )
    {
      break;
    }
    else
    {       
      for(i=0;i<=strlen(buffer);i++)
      {
        if(buffer[i] == '"')
          buffer[i] = ' ';
      }
      sscanf(buffer," %s  %s  %s  %d  %d  %24[^\n]  %ld  %d  %d  %d ",ip_addr,room_name,uri,&state,&power,start_date_time,&st_time,&power_rat,&res_type,&auto_mode);
      if(strncmp(uri,"down",4)==0)
      {
        pointer = load_head;  
        while(pointer != NULL)
        {
          if(strcmp(pointer->ip_address,ip_addr)==0)
          {
            update_list(pointer->ip_address,pointer->uri,0,0,st_time,pointer->power_rat);  
          }     
          pointer = pointer->next;
        }        
      }
      else
      {
        update_list(ip_addr,uri,state,power,st_time,power_rat);      
      }
    }
  }
  fclose(fp);
  return 0;
}
