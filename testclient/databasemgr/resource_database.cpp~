#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <string.h>
#include<fcntl.h>
#include <time.h>
#include <stdlib.h>
#include<pthread.h>

#define FILE_DATABASE_PATH "resource_database.csv"

static pthread_mutex_t database_lock;

static char *getdate_time(void)
{
  time_t current_time;
  char* c_time_string;

  /* Obtain current time. */
  current_time = time(NULL);

  if (current_time == ((time_t)-1))
  {
    return NULL;
  }
  /* Convert to local time format. */
  c_time_string = ctime(&current_time);
  if (c_time_string == NULL)
  {
    return NULL;
  }
  c_time_string[strlen(c_time_string)-1]='\0';
  return c_time_string;
}

int create_entry_in_database_file(const char *hostname, const char *roomname, const char *uri, int state, int power,int power_rat, int resource_type,int auto_status)
{
  int ret  = -1;
  char* c_time_string;
  FILE *fp;
  pthread_mutex_lock(&database_lock);
  fp = fopen(FILE_DATABASE_PATH, "a");
  if(fp == NULL)
  {       

    fp = fopen(FILE_DATABASE_PATH,"w");
    if(fp != NULL) 
    {
      fprintf(fp,"\"Host-Name\" \"Room-Name\" \"URI\" \"State\" \"Power\" \"start Date-Time\" \"Start-Time\" \"Power-Rating(Watt)\" \"Resource Type\" \"Auto Mode\"\n"); 
      fclose(fp);
    }
    else
    {
      pthread_mutex_unlock(&database_lock);
      return -1;
    }
  } 
  if(uri && hostname)
  {  
    c_time_string = getdate_time();
    fprintf(fp,"\"%s\" \"%s\" \"%s\" \"%d\" \"%d\" \"%s\" \"%ld\" \"%d\" \"%d\" \"%d\"\n",hostname,roomname,uri,state,power,c_time_string==NULL?"FAIL":c_time_string,time(NULL),power_rat,resource_type,auto_status);
  }
  fclose(fp);
  pthread_mutex_unlock(&database_lock);
  return ret;
}

static void last_update_entry_in_database_file(const char *hostname,time_t prev_time)
{
  FILE *fp;
  char* c_time_string;
  pthread_mutex_lock(&database_lock);
  fp = fopen(FILE_DATABASE_PATH, "a");
  if(fp)
  {
 
  c_time_string = ctime(&prev_time);
  if (c_time_string == NULL)
  {
    c_time_string = "Fail";
  }
  else
  c_time_string[strlen(c_time_string)-1]='\0';
   
    fprintf(fp,"\"%s\" \"%s\" \"%s\" \"%d\" \"%d\" \"%s\" \"%ld\" \"%d\" \"%d\" \"%d\"\n",hostname,"down","down",0,0,c_time_string,prev_time,0,0,0);
    fclose(fp);   
  }
  pthread_mutex_unlock(&database_lock);
}

void populate_prev_resource_status()
{
  char ip_addr[256];
  time_t prev_time;
  FILE *fp;
  char buffer[256];
  char *tmp;
  
  fp = fopen("resource_status.txt","r");
  if(fp == NULL)
  {
     return;
  }
  
  while(1)
  {
    memset(buffer,0,sizeof(buffer));
    memset(ip_addr,0,sizeof(ip_addr));
    prev_time=0;
    if (fscanf(fp,"%s",buffer) == EOF )
    {
       break;
    }
    else
    {    
  
       //sscanf(buffer,"%s::%ld",ip_addr,&prev_time);
       tmp = strstr(buffer,"::");
       if(tmp)
          prev_time=atoi(tmp+2);
       strncpy(ip_addr,buffer,strlen(buffer)-strlen(tmp));
       printf("%s  ip-%s  time-%ld\n",buffer,ip_addr,prev_time);
       last_update_entry_in_database_file(ip_addr,prev_time);
    }      
  }
  fclose(fp); 
  system("rm -f resource_status.txt");
}

void database_file_init()
{
  FILE *fp;
  int res = pthread_mutex_init(&database_lock, NULL);
  if (res != 0) {
    perror("database Mutex initialization failed");
    return ;
  }
  fp = fopen(FILE_DATABASE_PATH,"r");
  if(fp == NULL)
  {
    fp = fopen(FILE_DATABASE_PATH,"w");
    if(fp != NULL) 
    {
      fprintf(fp,"\"Host-Name\" \"Room-Name\" \"URI\" \"State\" \"Power\" \"start Date-Time\" \"Start-Time\" \"Power-Rating(Watt)\" \"Resource Type\" \"Auto Mode\"\n");  
      fclose(fp);
    }
  }
}
