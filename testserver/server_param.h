#ifndef __LAVIRESOURCE__
#define __LAVIRESOURCE__
class LaviResource_server
{
  private:
    OCEntityHandlerResult entityHandler(std::shared_ptr<OCResourceRequest> request);

  public:
    /// Access this property from a TB client
    std::string m_name;
    int m_state;
    int m_power;
    int resource_type; //1-state & power,2-only state,3-only power,4-temprature,5-smoke,6-sunlight,7-humidity
    int power_rating;
    std::string m_Uri;
    std::string m_hostname;
    int auto_status; // 0 means inactive and 1 means active- the device will take action automatically

    std::string resourceTypeName;//core.xxxx
    OCResourceHandle m_resourceHandle;
    OCRepresentation m_ResourceRep;
    ObservationIds m_interestedObservers;
    //void createResource(std::string uri,std::string resourceType,std::string room_name,int);
    void createResource(void);
    OCResourceHandle getHandle();
    void put(OCRepresentation& rep);
    OCRepresentation post(OCRepresentation& rep);
    OCRepresentation get();
    void addType(const std::string& type) const;
    void addInterface(const std::string& interface) const;    
    pthread_t threadId;
    int gObservation ;
    int startedThread;
    bool is_auto_thread_running;
    pthread_t autothread_id;

    int fd_sensor;
    int fd_switch;      
    int fd_intensity;
    long int delay_ns;
    struct reosource_switch {
    int pin_number;
    int pin_value;
    };
    struct reosource_intensity {
    int pin_number;
    int pin_value;
    };
    struct reosource_sensor {
    int pin_number;
    int pin_value;
    };

    pthread_t intensityId;
   

    int open_hal_switch_device(void);
    int open_hal_sensor_device(void);
    void close_hal_switch_device(void);
    void close_hal_sensor_device(void);
    int start_intensity(class LaviResource_server *param);
    int stop_intensity(void);
    int set_intensity(void);
    int get_sensor_value(void);
    int set_switch_state(void);
    int get_switch_state(void);
    struct reosource_switch  res_switch;
    struct reosource_intensity res_intens;
    struct reosource_sensor res_sens;

    /// Constructor
    LaviResource_server()
      :m_name(""), m_state(0), m_power(0),resource_type(-1),power_rating(0),m_Uri(""),m_hostname(""),
      m_resourceHandle(nullptr) {
      res_switch.pin_number = -1;
      res_switch.pin_value = -1;
      res_intens.pin_number = -1;
      res_intens.pin_value = -1;
      res_sens.pin_number = -1;
      res_sens.pin_value = -1;
      intensityId = -1;
      fd_sensor = -1;
      fd_switch = -1;
      fd_intensity = -1;
      delay_ns = 0;
      resourceTypeName="";
      gObservation = 0;
      startedThread = 0;
      auto_status = 0;
      is_auto_thread_running = false;
      }

      ~LaviResource_server()
      {
      if(resource_type ==1)
         stop_intensity();
      if(fd_sensor>0)
        close(fd_sensor);  
      if(fd_switch>0)
        close(fd_switch);  
      if(fd_intensity>0)
        close(fd_intensity);  
      }
};

struct server_resource_list
{
	LaviResource_server *Resource;
        struct server_resource_list *next;
};

int server_config_read(void);
struct server_resource_list *server_get_resource_head(void);
void server_free_list();
#endif
