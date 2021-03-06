#include <string>
#include <map>
#include <cstdlib>
#include <pthread.h>
#include <mutex>
#include <condition_variable>
#include "OCPlatform.h"
#include "OCApi.h"
using namespace OC;
class ResourceParam
{
   public:
   std::string Uri;
   std::string m_hostname;
   std::string name;
   int state;
   int power;  // fan,bulb,gyser temprature, ac temprature, fridge temprature
   int resource_type;//1-state & power,2-only state,3-only power,4-temprature,5-smoke,6-sunlight,7-humidity 
    
   int auto_status; // 0 means inactive and 1 means active- the device will take action automatically

   int schedule_staus; // scheduling is enable=1 or disable is 0 , 3 is cancealed
   int schedule_state; // the state at the schedule start time
   int schedule_power; // the power at scheduled start time
   time_t schedule_start_time;// seconds left from now 
   time_t schedule_end_time; // seconds left from now, if end time is zero then it will maintain the state of start time otherwise previous
   int is_repeat_next_day; // 0 means no and 1 mean yes

   int threshold_power; // only for input resource, threshold level for temprature , gas etc, it can be also number of person in house.

   int power_rating; // power rating marked on resource
   float fixed_unit;  // the maximum unit a resource can consume, programmed by user
   float consume_unit; // How much it is consumed so far since statred.
   
   int observer_status;
   std::mutex ResourceLock;
   int  ErrhandleReqRespInit(void);
   int  ErrhandlePostRespMessage(int *resp);
   int  ErrhandleGetRespMessage(int *resp);
   pthread_t schedular_thread;
   bool is_schedule_task_running;
  
   ResourceParam() : Uri(""),m_hostname(""),name(""),state(0), power(0),resource_type(-1),auto_status(0) 
   {
           schedule_staus = 0;
           schedule_state = 0;
           schedule_power = 0;
           schedule_start_time=0;
           schedule_end_time=0;
           is_repeat_next_day = 0;
           threshold_power=0;
           power_rating=0;
           fixed_unit=0;
           consume_unit=0;
           observer_status=0;
           is_schedule_task_running = false;
           ErrhandleReqRespInit();
   }
};

struct resource_list
{
	std::shared_ptr<OCResource> Resource;
        ResourceParam *resparam;
	struct resource_list *next;
};





