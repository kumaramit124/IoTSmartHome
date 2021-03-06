// OCClient.cpp : Defines the entry point for the console application.
#include <stdio.h>
#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <mutex>
#include <condition_variable>
#include "OCPlatform.h"
#include "OCApi.h"
using namespace OC;
#include "iot_api.h"
#include "cloud_iot_common.h"
#include "cloud_iot_api.h"
#include "resource_xml.h"
#include "UIManager.h"
#include "resource_param.h"
#include "resource_list.h"
#include "schedule_res.h"
#include "power_analysys.h"

static std::mutex CurResourceLock;
static int observer_pipes[2];
static pthread_t observer_thread;

void onObserve(const HeaderOptions headerOptions, const OCRepresentation& rep,
    const int& eCode, const int& sequenceNumber)
{
  struct resource_list *ResList;
  std::string noti_buffer;
  std::string hostname;
  std::string uri;        
  
  try
  {             
    if(eCode == OC_STACK_OK)
    {
      rep.getValue("hostname",  hostname);
      uri=rep.getUri();
      ResList = find_by_ip(hostname,uri);
      if(ResList)
      {  
        std::lock_guard<std::mutex> lock(ResList->resparam->ResourceLock);     			
        rep.getValue("state", ResList->resparam->state);
        rep.getValue("power", ResList->resparam->power);
        rep.getValue("name",  ResList->resparam->name);
        rep.getValue("hostname",  ResList->resparam->m_hostname);
        rep.getValue("typeresource",  ResList->resparam->resource_type);
        rep.getValue("power_rating",  ResList->resparam->power_rating);
        rep.getValue("auto_status",  ResList->resparam->auto_status);

        ResList->resparam->Uri = uri;
        std::cout<<"\t------------------\n\tstate:"<< ResList->resparam->state<<std::endl;
        std::cout<<"\tpower:"<< ResList->resparam->power<<std::endl;
        std::cout<<"\tname:"<<  ResList->resparam->name<<std::endl;
        std::cout<<"\thostname:"<< ResList->resparam->m_hostname<<std::endl;
        std::cout<<"\turi:"<<ResList->resparam->Uri<<std::endl;
        std::cout<<"\tResource_type:"<<ResList->resparam->resource_type<<std::endl;
        std::cout<<"\tpower_rating:"<<ResList->resparam->power_rating<<std::endl;
        std::cout<<"\tauto_status:"<<ResList->resparam->auto_status<<std::endl;       

        create_entry_in_database_file(ResList->resparam->m_hostname.c_str(),ResList->resparam->name.c_str(),rep.getUri().c_str(),ResList->resparam->state,ResList->resparam->power,ResList->resparam->power_rating,ResList->resparam->resource_type,ResList->resparam->auto_status);
        update_list((char *)ResList->resparam->m_hostname.c_str(),(char *)rep.getUri().c_str(),ResList->resparam->state,ResList->resparam->power,time(NULL),ResList->resparam->power_rating);

        ResList->resparam->consume_unit = get_power_consumption((char *)ResList->resparam->m_hostname.c_str(),(char *)rep.getUri().c_str());
        std::cout<<"\tPower Consumption:"<<ResList->resparam->consume_unit<<std::endl; 
        sprintf((char *)noti_buffer.c_str(),"%d %s %s %s %d %d %f",NOTIFY_STATE_CHANGE,ResList->resparam->m_hostname.c_str(),ResList->resparam->name.c_str(),rep.getUri().c_str(),ResList->resparam->state,ResList->resparam->power,ResList->resparam->consume_unit); 
        send_file_to_ui((char *)noti_buffer.c_str());
      }                        
    }
    else
    {
      std::cout << "onObserve Response error: " << eCode << std::endl;			

    }
  }
  catch(std::exception& e)
  {
    std::cout << "Exception: " << e.what() << " in onObserve" << std::endl;
  }

}

void onPost(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode)
{
  int resp = -1;
  struct resource_list *ResList;
  std::string hostname;
  std::string uri;
  try
  {
    if(eCode == OC_STACK_OK || eCode == OC_STACK_RESOURCE_CREATED)
    {
      rep.getValue("hostname",  hostname);
      uri=rep.getUri();
      ResList = find_by_ip(hostname,uri);
      if(ResList)
      {
        rep.getValue("state", ResList->resparam->state);
        rep.getValue("power", ResList->resparam->power);
        rep.getValue("name",  ResList->resparam->name);
        rep.getValue("hostname",  ResList->resparam->m_hostname);
        resp = 0;
        ResList->resparam->ErrhandlePostRespMessage(&resp);
      }                    
    }
    else
    {
      std::cout << "onPost Response error: " << eCode << std::endl;			
    }
  }
  catch(std::exception& e)
  {
    std::cout << "Exception: " << e.what() << " in onPost" << std::endl;
  }
}

// Local function to put a different state for this resource
static int postDevice(struct resource_list *ResList)
{
  int resp = -1;
  if(ResList)
  {
    OCRepresentation rep;
    rep.setValue("state", ResList->resparam->state);
    rep.setValue("power", ResList->resparam->power);
    // Invoke resource's post API with rep, query map and the callback parameter
    ResList->Resource->post(rep, QueryParamsMap(), &onPost);
    ResList->resparam->ErrhandleGetRespMessage(&resp);
  }
  return resp;
}

// callback handler on PUT request
void onSet(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode)
{
  int resp = -1;
  struct resource_list *ResList;
  std::string hostname;
  std::string uri;
  try
  {
    if(eCode == OC_STACK_OK)
    {
      rep.getValue("hostname",  hostname);
      uri=rep.getUri();
      ResList = find_by_ip(hostname,uri);
      if(ResList)
      {
        rep.getValue("state", ResList->resparam->state);
        rep.getValue("power", ResList->resparam->power);
        rep.getValue("name",  ResList->resparam->name);
        rep.getValue("hostname", ResList->resparam->m_hostname);
        resp = 0;
        ResList->resparam->ErrhandlePostRespMessage(&resp);
      }                    

    }
    else
    {
      std::cout << "onPut Response error: " << eCode << std::endl;			
    }
  }
  catch(std::exception& e)
  {
    std::cout << "Exception: " << e.what() << " in onPut" << std::endl;
  }
}

// Local function to put a different state for this resource
static int setDevice(struct resource_list *ResList)
{
  int resp = -1;
  if(ResList)
  {
    OCRepresentation rep;
    rep.setValue("state", ResList->resparam->state);
    rep.setValue("power", ResList->resparam->power);
    rep.setValue("auto_status", ResList->resparam->auto_status);
    // Invoke resource's put API with rep, query map and the callback parameter
    ResList->Resource->put(rep, QueryParamsMap(), &onSet);
    ResList->resparam->ErrhandleGetRespMessage(&resp);

  }
  return resp;
}

// Callback handler on GET request
void onGet(const HeaderOptions& headerOptions, const OCRepresentation& rep, const int eCode)
{
  int resp = -1;
  struct resource_list *ResList;
  std::string hostname;
  std::string uri;
  try
  {
    if(eCode == OC_STACK_OK)
    {
      rep.getValue("hostname",  hostname);
      uri=rep.getUri();
      ResList = find_by_ip(hostname,uri);
      if(ResList)
      {
        rep.getValue("state", ResList->resparam->state);
        rep.getValue("power", ResList->resparam->power);
        rep.getValue("name",  ResList->resparam->name);
        rep.getValue("hostname",  ResList->resparam->m_hostname);
        rep.getValue("auto_status",  ResList->resparam->auto_status);
        resp = 0;
        ResList->resparam->ErrhandlePostRespMessage(&resp);
      }             			
    }
    else
    {
      std::cout << "onGET Response error: " << eCode << std::endl;			
    }
  }
  catch(std::exception& e)
  {
    std::cout << "Exception: " << e.what() << " in onGet" << std::endl;
  }
}

// Local function to get representation of light resource
static int getDeviceValue(struct resource_list *ResList)
{
  int resp = -1;
  if(ResList)
  {
    QueryParamsMap test;
    ResList->Resource->get(test, &onGet);
    ResList->resparam->ErrhandleGetRespMessage(&resp);
  }
  return resp;
}

int set_sensor(struct iot_resource *ior)
{
  struct resource_list *ResList;
  ResList = find_by_ip(ior->host,ior->Uri);
  if (ResList)
  {
    std::lock_guard<std::mutex> lock(ResList->resparam->ResourceLock);
    ResList->resparam->state = ior->state;
    ResList->resparam->power = ior->power;
    return(setDevice(ResList));
  }
  return -1;      
}

int get_sensor(struct iot_resource *ior)
{
  struct resource_list *ResList;
  ResList  = find_by_ip(ior->host,ior->Uri);
  if (ResList )
  {
    std::lock_guard<std::mutex> lock(ResList->resparam->ResourceLock);
    if(!getDeviceValue(ResList))
    {
      ior->state =  ResList->resparam->state;
      ior->power =  ResList->resparam->power;
      ior->auto_status =  ResList->resparam->auto_status;
      ior->observer_status =  ResList->resparam->observer_status;
      ior->consume_unit = ResList->resparam->consume_unit;
      strcpy(ior->name,ResList->resparam->name.c_str());
      return 0;
    }

  }
  return -1;  
}

int post_sensor(struct iot_resource *ior)
{
  struct resource_list *ResList;
  ResList = find_by_ip(ior->host,ior->Uri);
  if (ResList)
  {
    std::lock_guard<std::mutex> lock(ResList->resparam->ResourceLock);
    ResList->resparam->state = ior->state;
    ResList->resparam->power = ior->power;
    return(postDevice(ResList));
  }
  return -1;
}

int SetMode(struct iot_resource *ior)
{
  struct resource_list *ResList;
  ResList = find_by_ip(ior->host,ior->Uri);
  if (ResList)
  {
    if(ResList->resparam)
    {	
      std::lock_guard<std::mutex> lock(ResList->resparam->ResourceLock);
      ResList->resparam->auto_status = ior->auto_status;
      std::cout << "Setting Auto Mode Status to: "<< ResList->resparam->auto_status <<std::endl;
      return(setDevice(ResList));      
    }
  }
  return -1;
}

int SetScheduling(struct iot_resource *ior)
{
  struct resource_list *ResList;
  ResList = find_by_ip(ior->host,ior->Uri);
  if (ResList)
  {
    std::lock_guard<std::mutex> lock(ResList->resparam->ResourceLock);
    if(ResList->resparam)
    {	
      ResList->resparam->schedule_staus = ior->schedule_staus;
      if(ResList->resparam->schedule_staus == 1 )  { // start scheduling
      ResList->resparam->schedule_state = ior->schedule_state;
      ResList->resparam->schedule_power = ior->schedule_power;
      ResList->resparam->schedule_start_time = ior->schedule_start_time;
      ResList->resparam->schedule_end_time = ior->schedule_end_time;
      ResList->resparam->is_repeat_next_day = ior->is_repeat_next_day; 
      std::cout << "Setting scheduling parameter status: "<< ResList->resparam->schedule_staus <<std::endl;
      std::cout << "Setting scheduling parameter state: "<< ResList->resparam->schedule_state <<std::endl;
      std::cout << "Setting scheduling parameter power: "<< ResList->resparam->schedule_power <<std::endl;
      std::cout << "Setting scheduling parameter st time: "<< ResList->resparam->schedule_start_time <<std::endl;
      std::cout << "Setting scheduling parameter end time: "<< ResList->resparam->schedule_end_time <<std::endl;
      std::cout << "Setting scheduling parameter next time: "<< ResList->resparam->is_repeat_next_day <<std::endl;
      start_schedular(ResList->resparam);
      }
      else if(ResList->resparam->schedule_staus == 0)  { // stop schedular
      stop_schedular(ResList->resparam);
      }
      else
          return -1;

      return 0;       
    }
  }
  return -1;
}


void Stop_Observe(struct resource_list *ResList)
{
  OCStackResult result ;
  if (ResList)
  {
    std::lock_guard<std::mutex> lock(ResList->resparam->ResourceLock);
    if(ResList->Resource)
    {	
      if(ResList->resparam->observer_status == 1)
      {
        ResList->resparam->observer_status = 0;
        result = ResList->Resource->cancelObserve();
        std::cout << "Cancel Observe result: "<< result <<std::endl;
      }
    }
  }

}

void Start_Observe(struct resource_list *ResList)
{
  if (ResList)
  {
    std::lock_guard<std::mutex> lock(ResList->resparam->ResourceLock);
    if(ResList->Resource)
    {	
      if(ResList->resparam->observer_status == 0)
      {
        ResList->resparam->observer_status = 1;
        ResList->Resource->observe(ObserveType::ObserveAll, QueryParamsMap(), &onObserve);	
      }
    }
  }
}

void StartResObserver()
{
  struct resource_list *pointer = get_resource_head();
  while(pointer != NULL)
  {
    Start_Observe(pointer);
    pointer = pointer->next;
  }
}

void StopResObserver()
{
  struct resource_list *pointer = get_resource_head();
  while(pointer != NULL)
  {
    Stop_Observe(pointer);
    pointer = pointer->next;
  }
}

void *StartAllObserver(void *arg)
{ 
  int ret = 0;
  fd_set set;
  struct timeval timeout;
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  while (1)
  { 
    FD_ZERO(&set); /* clear the set */
    FD_SET(observer_pipes[0], &set); /* add our file descriptor to the set */
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    ret = select(observer_pipes[0] + 1, &set, NULL, NULL, &timeout);
    if(ret == -1)
    {
      printf("an error occured\n");
      pthread_exit(NULL);
      return NULL;
    }
    else if(ret == 0)
    {
      StartResObserver();
      pthread_exit(NULL);
      return NULL;	
    }
    else
    {
      read(observer_pipes[0], &ret, 4);                
    }
  }
  pthread_exit(NULL);
  return NULL;
}

// Callback to found resources
void foundResource(std::shared_ptr<OCResource> resource)
{
  int res=1;
  ResourceParam *resource_param;


  try
  {

    std::lock_guard<std::mutex> lock(CurResourceLock);     
    if(resource)
    {	                         
      write(observer_pipes[1], &res, 4);
      /* the find should be based on some unique ud that should be its uri. The host address can change at every boot */
      /* to be done*/
      if(find_by_host(resource->host(),resource->uri())!=NULL)
      { 
        // need to find other solution, this function should be fast
        // even printf will cause the crash for load of 20 resources, it is the iotivity stack issue.
        // std::cout<<"--deleting resource host: "<<resource->host() <<" uri: "<<resource->uri()<<std::endl; 
        if(delete_node(resource->host(),resource->uri()))
        {
          return;
        }

      }
      resource_param = new ResourceParam;
      if(resource_param)
      {                           
        //std::cout<<"++inserting resource host: "<<resource->host() <<" uri: "<<resource->uri()<<std::endl; 
        if(insert(resource,resource_param))
          delete resource_param;
      }     
      else
        std::cout << "resource creation is failed\n";


    }
    else
    {
      // Resource is invalid
      std::cout << "Resource is invalid" << std::endl;
    }

  }
  catch(std::exception& e)
  {
    std::cerr << "Exception in foundResource: "<< e.what() << std::endl;
  }

}

void init_resource_file()
{
  print_to_file();
}

static FILE* client_open(const char *path, const char *mode)
{
  return fopen("./oic_svr_db_client.json", mode);
}

static int start_observer_thread()
{
  if(pthread_create(&observer_thread, NULL, StartAllObserver, NULL)!=0)
    return -1;
  else
    return 0;
}

static void stop_observer_thread()
{
  void *res;
  int  ret = pthread_cancel(observer_thread);
  if (ret != 0)
    printf("observer pthread_cancel error\n");
  else
  {
    /* Join with thread to see what its exit status was */
    ret = pthread_join(observer_thread, &res);
    if (ret != 0)
      printf("observer pthread_join error\n");
    else
    {
      if (res == PTHREAD_CANCELED)
      {
        printf("Observer thread cancealed\n");
      }
    }  
  }
}

int send_unicast_request(char *ip_addr)
{
  std::ostringstream requestURI;
  stop_observer_thread();
  /*call unicast based on ip address received*/
  requestURI << OC_RSRVD_WELL_KNOWN_URI;// << "?rt=core.light";
  OCPlatform::findResource(ip_addr, requestURI.str(),
      CT_DEFAULT, &foundResource);
  //OCPlatform::findResource(ip_addr, requestURI.str(),
  //		CT_DEFAULT, &foundResource);
  start_observer_thread();
  return 0;
}

int iot_start(void) {
  int ret = -1;
  std::ostringstream requestURI;

  OCPersistentStorage ps {client_open, fread, fwrite, fclose, unlink };

  if (pipe(observer_pipes) != 0) 
    return -1;
  database_file_init();
  populate_prev_resource_status();
  start_load_analysys();
  if(start_observer_thread())
    return -1;

  // Create PlatformConfig object
  PlatformConfig cfg {
    OC::ServiceType::InProc,
      OC::ModeType::Both,
      "0.0.0.0",
      0,
      OC::QualityOfService::LowQos,
      &ps
  };

  OCPlatform::Configure(cfg);
  try
  {
    // makes it so that all boolean values are printed as 'true/false' in this stream
    std::cout.setf(std::ios::boolalpha);
    // Find all resources
    requestURI << OC_RSRVD_WELL_KNOWN_URI;// << "?rt=core.light";

    OCPlatform::findResource("", requestURI.str(),
        CT_DEFAULT, &foundResource);
    // Find resource is done twice so that we discover the original resources a second time.
    // These resources will have the same uniqueidentifier (yet be different objects), so that
    // we can verify/show the duplicate-checking code in foundResource(above);
    //OCPlatform::findResource("", requestURI.str(),
    //		CT_DEFAULT, &foundResource);
    ret = 0;

  }catch(OCException& e)
  {
    ret =-1;
    oclog() << "Exception in main: "<<e.what();
  }

  return ret;
}

int iot_stop(void)
{
  stop_all_schedular_threads();
  StopResObserver();
  stop_observer_thread();
  stop_load_analysys(); 
  free_list();// free the occupied resource
  // unconfigured iotivity
  return 0;
}
