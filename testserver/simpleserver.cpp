#include <functional>

#include<stdio.h>
#include<signal.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <mutex>
#include <condition_variable>

#include "OCPlatform.h"
#include "OCApi.h"


using namespace OC;
using namespace std;
namespace PH = std::placeholders;

#include "server_param.h"

static int cur_temprature = -999;
static int cur_sunlight   = -999;
static int cur_smoke      = -999;
static int cur_humidity   = -999;

//int gObservation = 0;
void * NotifyResourceChange (void *param);
void * handleSlowResponse (void *param, std::shared_ptr<OCResourceRequest> pRequest);

// Specifies where to notify all observers or list of observers
// false: notifies all observers
// true: notifies list of observers
bool isListOfObservers = true;

// Specifies secure or non-secure
// false: non-secure resource
// true: secure resource
bool isSecure = false;

/// Specifies whether Entity handler is going to do slow response or not
bool isSlowResponse = false;

bool isfound = false;

// Forward declaring the entityHandler

/// This class represents a single resource named 'LaviResource_server'. This resource has
/// two simple properties named 'state' and 'power'

extern char* getinterfaceip();
extern int StartBcastTrans(int port);
extern int StopBcastTrans(void);

/* Note that this does not need to be a member function: for classes you do not have
   access to, you can accomplish this with a free function: */


/// This function internally calls registerResource API.
void LaviResource_server::createResource(void)
{
  m_hostname = getinterfaceip();
  std::string resourceInterface = DEFAULT_INTERFACE;

  // Initialize representation
  m_ResourceRep.setUri(m_Uri);
  m_ResourceRep.setValue("state", m_state);
  m_ResourceRep.setValue("power", m_power);
  m_ResourceRep.setValue("name", m_name);
  m_ResourceRep.setValue("hostname", m_hostname);
  m_ResourceRep.setValue("typeresource",resource_type);
  m_ResourceRep.setValue("power_rating",power_rating);
  m_ResourceRep.setValue("auto_status",auto_status);
   
  // OCResourceProperty is defined ocstack.h
  uint8_t resourceProperty;
  if(isSecure)
  {
    resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE | OC_SECURE;
  }
  else
  {
    resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;
  }
  EntityHandler cb = std::bind(&LaviResource_server::entityHandler, this,PH::_1);

  // This will internally create and register the resource.
  OCStackResult result = OCPlatform::registerResource(
      m_resourceHandle, m_Uri, resourceTypeName,
      resourceInterface, cb, resourceProperty);

  if (OC_STACK_OK != result)
  {
    cout << "Resource creation was unsuccessful\n";
  }
  else
  {
    addType(std::string(resourceTypeName));    
    addInterface(std::string(LINK_INTERFACE));
  }
}

OCResourceHandle LaviResource_server::getHandle()
{
  return m_resourceHandle;
}


void *StartAutoMode(void *param)
{ 
  LaviResource_server* ResPtr = (LaviResource_server*) param;
  if(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL))
  {
    pthread_exit(NULL);
    return NULL;
  }
  if(!param) 
  {
    pthread_exit(NULL);
    return NULL;
  }
  while (ResPtr->auto_status == 1)
  { 
     /*
       need to build the logic based on these input 
          int cur_temprature = 0;
          int cur_sunlight   = 0;
          int cur_smoke      = 0;
      during auto mode we have always insist on convenyence of user so the logic og auto mode 
      should change dynamically based on user behaviour and the user profile should be saved so at next 
      boot the auto behaviour should be alligned with user behaviour
     */
     if(ResPtr->resource_type == 1) //1- both state & power
     {
        // fan,light comes in this range
     }
     else if(ResPtr->resource_type == 2) //2- only state 
     {
       //ac,fridge, geaser comes in this type
     }
     else if(ResPtr->resource_type == 3) //3- only power
     {
       //all devices can comes in this range if every thing is controlled through dimmer and triac.no need to have switch in this case
     }
     else
         break;        
  }
  ResPtr->is_auto_thread_running = false;
  pthread_exit(NULL);
  return NULL;
}


// Puts representation.
// Gets values from the representation and
// updates the internal state
void LaviResource_server::put(OCRepresentation& rep)
{
  try {
    if (rep.getValue("state", m_state))
    {
      cout << "\t\t\t\t" << "settig state to: " << m_state << endl;
      // send to driver
      res_switch.pin_value = m_state;
      set_switch_state();       
    }
    else
    {
      cout << "\t\t\t\t" << "state not found in the representation" << endl;
    }

    if (rep.getValue("power", m_power))
    {
      cout << "\t\t\t\t" << "seeting power: " << m_power << endl;
      // send to driver
      set_intensity();
    }
    else
    {
      cout << "\t\t\t\t" << "power not found in the representation" << endl;
    }
    
    if (rep.getValue("auto_status", auto_status))
    {
      cout << "\t\t\t\t" << "auto_status: " << auto_status << endl;
      if((resource_type >=1) && (resource_type <=3))
      {
      if (auto_status == 1)
      {
         if(is_auto_thread_running == false)
         {
              // create the thread for auto mode
              cout << "\t\t\t\t" << "Starting auto thread"<< endl;
              if(!pthread_create(&autothread_id, NULL, StartAutoMode, (void *)this))
                  is_auto_thread_running = true;
              else
                  auto_status = 0;
         }
      }
      else if (auto_status == 0)
      {
         if(is_auto_thread_running == true)
         {
              // cancel the thread for auto mode
              cout << "\t\t\t\t" << "Stoping auto thread"<< endl;
              is_auto_thread_running = false;
         }
      }
      }
      else
      {
         auto_status = 0;
      }
      
    }


  }
  catch (exception& e)
  {
    cout << e.what() << endl;
  }

}

// Post representation.
// Post can create new resource or simply act like put.
// Gets values from the representation and
// updates the internal state
OCRepresentation LaviResource_server::post(OCRepresentation& rep)
{
  put(rep);
  return get();
}


// gets the updated representation.
// Updates the representation with latest internal state before
// sending out.
OCRepresentation LaviResource_server::get()
{
 
  get_switch_state();
  m_ResourceRep.setValue("state", m_state);
  m_ResourceRep.setValue("power", m_power);
  m_ResourceRep.setValue("name", m_name);
  m_ResourceRep.setValue("hostname", m_hostname);
  m_ResourceRep.setValue("typeresource",resource_type);
  m_ResourceRep.setValue("power_rating",power_rating);
  m_ResourceRep.setValue("auto_status",auto_status);
  return m_ResourceRep;
}

void LaviResource_server::addType(const std::string& type) const
{
  OCStackResult result = OCPlatform::bindTypeToResource(m_resourceHandle, type);
  if (OC_STACK_OK != result)
  {
    cout << "Binding TypeName to Resource was unsuccessful\n";
  }
}

void LaviResource_server::addInterface(const std::string& interface) const
{
  OCStackResult result = OCPlatform::bindInterfaceToResource(m_resourceHandle, interface);
  if (OC_STACK_OK != result)
  {
    cout << "Binding TypeName to Resource was unsuccessful\n";
  }
}

// This is just a sample implementation of entity handler.
// Entity handler can be implemented in several ways by the manufacturer
OCEntityHandlerResult LaviResource_server::entityHandler(std::shared_ptr<OCResourceRequest> request)
{
  isfound = true;

  cout << "\tIn Server CPP entity handler:\n";
  OCEntityHandlerResult ehResult = OC_EH_ERROR;
  if(request)
  {
    // Get the request type and request flag
    std::string requestType = request->getRequestType();
    int requestFlag = request->getRequestHandlerFlag();

    if(requestFlag & RequestHandlerFlag::RequestFlag)
    {
      cout << "\t\trequestFlag : Request\n";
      auto pResponse = std::make_shared<OC::OCResourceResponse>();
      pResponse->setRequestHandle(request->getRequestHandle());
      pResponse->setResourceHandle(request->getResourceHandle());

      // Check for query params (if any)
      QueryParamsMap queries = request->getQueryParameters();

      if (!queries.empty())
      {
        std::cout << "\nQuery processing upto entityHandler" << std::endl;
      }
      for (auto it : queries)
      {
        std::cout << "Query key: " << it.first << " value : " << it.second
          << std:: endl;
      }

      // If the request type is GET
      if(requestType == "GET")
      {
        cout << "\t\t\trequestType : GET\n";
        if(isSlowResponse) // Slow response case
        {
          static int startedThread = 0;
          if(!startedThread)
          {
            std::thread t(handleSlowResponse, (void *)this, request);
            startedThread = 1;
            t.detach();
          }
          ehResult = OC_EH_SLOW;
        }
        else // normal response case.
        {
          pResponse->setErrorCode(200);
          pResponse->setResponseResult(OC_EH_OK);
          pResponse->setResourceRepresentation(get());
          if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
          {
            ehResult = OC_EH_OK;
          }
        }
      }
      else if(requestType == "PUT")
      {
        cout << "\t\t\trequestType : PUT\n";
        OCRepresentation rep = request->getResourceRepresentation();

        // Do related operations related to PUT request
        // Update the LaviResource_server
        put(rep);
        pResponse->setErrorCode(200);
        pResponse->setResponseResult(OC_EH_OK);
        pResponse->setResourceRepresentation(get());
        if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
        {
          ehResult = OC_EH_OK;
        }
      }
      else if(requestType == "POST")
      {
        cout << "\t\t\trequestType : POST\n";

        OCRepresentation rep = request->getResourceRepresentation();

        // Do related operations related to POST request
        OCRepresentation rep_post = post(rep);
        pResponse->setResourceRepresentation(rep_post);
        pResponse->setErrorCode(200);
        if(rep_post.hasAttribute("createduri"))
        {
          pResponse->setResponseResult(OC_EH_RESOURCE_CREATED);
          pResponse->setNewResourceUri(rep_post.getValue<std::string>("createduri"));
        }
        else
        {
          pResponse->setResponseResult(OC_EH_OK);
        }

        if(OC_STACK_OK == OCPlatform::sendResponse(pResponse))
        {
          ehResult = OC_EH_OK;
        }
      }
      else if(requestType == "DELETE")
      {
        cout << "Delete request received" << endl;
      }
    }

    if(requestFlag & RequestHandlerFlag::ObserverFlag)
    {
      ObservationInfo observationInfo = request->getObservationInfo();
      if(ObserveAction::ObserveRegister == observationInfo.action)
      {
        m_interestedObservers.push_back(observationInfo.obsId);
      }
      else if(ObserveAction::ObserveUnregister == observationInfo.action)
      {
        m_interestedObservers.erase(std::remove(
              m_interestedObservers.begin(),
              m_interestedObservers.end(),
              observationInfo.obsId),
              m_interestedObservers.end());
                           
      }
      // Observation happens on a different thread in NotifyResourceChange function.
      // If we have not created the thread already, we will create one here.
      if(!this->startedThread)
      {
        this->gObservation = 1;
        pthread_create (&this->threadId, NULL, NotifyResourceChange, (void *)this);
        this->startedThread = 1;
      }
      ehResult = OC_EH_OK;
    }
  }
  else
  {
    std::cout << "Request invalid" << std::endl;
  }

  return ehResult;
}


// NotifyResourceChange  is an observation function,
// which notifies any changes to the resource to stack
// via notifyObservers
void * NotifyResourceChange (void *param)
{
  int old_power =0;
  int old_state = 0;
  LaviResource_server* ResPtr = (LaviResource_server*) param;
  int ret = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  if (ret != 0)
  {
    pthread_exit(NULL);
    return NULL;
  }
  
  // This function continuously monitors for the changes
  while (ResPtr)
  {
    
    if (ResPtr->gObservation)
    {
      /*the value inside the while loop can be changed either by the gateway device or by the user manually*/
      /*in the case of manual operation by user , the driver will notify the status of manual operation value*/
      /* the state can be identified through software but we need to see how to get the intnsity value back if configured Manually*/
      if(ResPtr->resource_type == 1)
      {
         while((ResPtr->m_power == old_power)&&(ResPtr->m_state == old_state));
         sleep(1); 
         old_power = ResPtr->m_power;
         old_state = ResPtr->m_state;
      }
      else if(ResPtr->resource_type == 2)
      {
         while((ResPtr->m_state == old_state));
         sleep(1);
         old_state = ResPtr->m_state;
      }
      else if(ResPtr->resource_type == 3)
      {
         while((ResPtr->m_power == old_power));
         sleep(1);
         old_power = ResPtr->m_power;
      }
      else if((ResPtr->resource_type > 3) && (ResPtr->resource_type <= 7))
      {
         while((ResPtr->m_power == old_power));
         sleep(1);
         old_power = ResPtr->m_power;
         if(ResPtr->resource_type == 4)
         {
            cur_temprature = old_power;
         }
         else if(ResPtr->resource_type == 5)
         {
            cur_smoke = old_power;
         }
         else if(ResPtr->resource_type == 6)
         {
              cur_sunlight = old_power;
         }
         else if(ResPtr->resource_type == 7)
         {
              cur_humidity = old_power;
         } 
      }
      else
          continue;

      cout << "Notifying observers with resource handle: " << ResPtr->getHandle() << endl;
           
      OCStackResult result = OC_STACK_OK;

      if(isListOfObservers)
      {
        std::shared_ptr<OCResourceResponse> resourceResponse =
        {std::make_shared<OCResourceResponse>()};

        resourceResponse->setErrorCode(200);
        resourceResponse->setResourceRepresentation(ResPtr->get(), DEFAULT_INTERFACE);
        result = OCPlatform::notifyListOfObservers(  ResPtr->getHandle(),
            ResPtr->m_interestedObservers,
            resourceResponse);
      }
      else
      {
        result = OCPlatform::notifyAllObservers(ResPtr->getHandle());
      }

      if(OC_STACK_NO_OBSERVERS == result)
      {
        cout << "No More observers, stopping notifications" << endl;
        ResPtr->gObservation = 0;
      }
    }
  }

  pthread_exit(NULL);
  return NULL;
}

void * handleSlowResponse (void *param, std::shared_ptr<OCResourceRequest> pRequest)
{
  // This function handles slow response case
  LaviResource_server* ResPtr = (LaviResource_server*) param;
  // Induce a case for slow response by using sleep
  std::cout << "SLOW response" << std::endl;
  sleep (10);

  auto pResponse = std::make_shared<OC::OCResourceResponse>();
  pResponse->setRequestHandle(pRequest->getRequestHandle());
  pResponse->setResourceHandle(pRequest->getResourceHandle());
  pResponse->setResourceRepresentation(ResPtr->get());
  pResponse->setErrorCode(200);
  pResponse->setResponseResult(OC_EH_OK);

  // Set the slow response flag back to false
  isSlowResponse = false;
  OCPlatform::sendResponse(pResponse);
  return NULL;
}


void create_resource_and_device(LaviResource_server *myRes)
{
  myRes->createResource();
  myRes->open_hal_switch_device();
  myRes->start_intensity(myRes);
  myRes->open_hal_sensor_device(); 
}

void PrintUsage()
{
  std::cout << std::endl;
  std::cout << "Usage : simpleserver <value>\n";
  std::cout << "    Default - Non-secure resource and notify all observers\n";
  std::cout << "    1 - Non-secure resource and notify list of observers\n\n";
  std::cout << "    2 - Secure resource and notify all observers\n";
  std::cout << "    3 - Secure resource and notify list of observers\n\n";
  std::cout << "    4 - Non-secure resource, GET slow response, notify all observers\n";
}

static FILE* client_open(const char *path, const char *mode)
{
  return fopen("./oic_svr_db_server.json", mode);
}


void sigsegv_server(int t)
{
  printf("ohh I am Killed::server !!!\n");
  StopBcastTrans();
  server_free_list();
  system("killall -9 simpleserver");
  exit(0);
}

int main(int argc, char* argv[])
{
  isfound = false;
  signal(SIGSEGV, sigsegv_server);
  signal(SIGINT, sigsegv_server);
  PrintUsage();
  OCPersistentStorage ps {client_open, fread, fwrite, fclose, unlink };

  if (argc == 1)
  {
    isListOfObservers = false;
    isSecure = false;
  }
  else if (argc == 2)
  {
    int value = atoi(argv[1]);
    switch (value)
    {
      case 1:
        isListOfObservers = true;
        isSecure = false;
        break;
      case 2:
        isListOfObservers = false;
        isSecure = true;
        break;
      case 3:
        isListOfObservers = true;
        isSecure = true;
        break;
      case 4:
        isSlowResponse = true;
        break;
      default:
        break;
    }
  }
  else
  {
    return -1;
  }

  // Create PlatformConfig object
  PlatformConfig cfg {
    OC::ServiceType::InProc,
      OC::ModeType::Server,
      "0.0.0.0", // By setting to "0.0.0.0", it binds to all available interfaces
      0,         // Uses randomly available port
      OC::QualityOfService::LowQos,
      &ps
  };

  OCPlatform::Configure(cfg);
  try
  {

    // Create the instance of the resource class
    // (in this case instance of class 'LaviResource_server').

    server_config_read();
    struct server_resource_list *pointer = server_get_resource_head();
    while(pointer != NULL)
    {
      create_resource_and_device(pointer->Resource);
      pointer = pointer->next;
    }
    
    StartBcastTrans(12345);
    // A condition variable will free the mutex it is given, then do a non-
    // intensive block until 'notify' is called on it.  In this case, since we
    // don't ever call cv.notify, this should be a non-processor intensive version
    // of while(true);
    std::mutex blocker;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock(blocker);
    std::cout <<"Waiting" << std::endl;
    cv.wait(lock, []{return false;});
  }
  catch(OCException &e)
  {
    std::cout << "OCException in main : " << e.what() << endl;
  }

  // No explicit call to stop the platform.
  // When OCPlatform::destructor is invoked, internally we do platform cleanup

  return 0;
}
