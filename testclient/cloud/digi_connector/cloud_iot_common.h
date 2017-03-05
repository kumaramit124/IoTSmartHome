#define GET_RESOURSE_PARAM 0
#define SET_RESOURSE_PARAM 1
#define POST_RESOURSE_PARAM 2
#define GET_RESOURSE_FILE 3
#define DEVICE_CONFIG_FILE_CHANGED 4
#define NOTIFY_STATE_CHANGE 5
#define SET_RESOURCE_MODE 6
#define SET_SCHEDULE_RESOURCE 7
#define CALLED_BY_CLOUD 0
#define CALLED_BY_UI 1

struct iot_resource
{
   char Uri[64];
   char host[64];
   char name[32];
   int  state;
   int  power;  // fan,bulb,gyser temprature, ac temprature, fridge temprature

   int auto_status;
   
   int schedule_staus; // scheduling is enable=1 or disable is 0 , 3 is cancealed
   int schedule_state; // the state at the schedule start time
   int schedule_power; // the power at scheduled start time
   time_t schedule_start_time;// seconds left from now 
   time_t schedule_end_time; // seconds left from now, if end time is zero then it will maintain the state of start time otherwise previous
   int is_repeat_next_day; // 0 means no and 1 mean yes

   int observer_status; // read only
   float consume_unit;
};

