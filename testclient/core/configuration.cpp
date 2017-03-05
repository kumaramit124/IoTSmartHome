#include <stdio.h>
#include <unistd.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <string.h>
#include<fcntl.h>

int is_iot_configured;
int is_cloud_configured;
char cloud_userid[32];
char cloud_password[32];
int cloud_device_mac_addr[6];
char cloud_url[256];
char cloud_target[32];
int cloud_vendorid;
int uiapp_port;
int broadcast_port;

int config_read(void)
{
  FILE *fp;
  fp = fopen("device_config.txt","r");
  if(!fp)
    return -1;
  fscanf(fp,"IOT_CONFIG=%d\n",&is_iot_configured);
  fscanf(fp,"CLOUD_CONFIG=%d\n",&is_cloud_configured);
  fscanf(fp,"CLOUD_USERID=%s\n",cloud_userid);
  fscanf(fp,"CLOUD_PASSWORD=%s\n",cloud_password);
  fscanf(fp,"CLOUD_DEVICEID=%x %x %x %x %x %x\n",&cloud_device_mac_addr[0],&cloud_device_mac_addr[1],&cloud_device_mac_addr[2],&cloud_device_mac_addr[3],&cloud_device_mac_addr[4],&cloud_device_mac_addr[5]);
  fscanf(fp,"CLOUD_URL=%s\n",cloud_url);
  fscanf(fp,"CLOUD_TARGET=%s\n",cloud_target);
  fscanf(fp,"CLOUD_VENDORID=%x\n",&cloud_vendorid); 
  fscanf(fp,"UIAPP_PORT=%d\n",&uiapp_port); 
  fscanf(fp,"BROADCAST_PORT=%d\n",&broadcast_port);  
  fclose(fp);
  printf("IOT_CONFIG=%d\n",is_iot_configured);
  printf("CLOUD_CONFIG=%d\n",is_cloud_configured);
  printf("CLOUD_USERID=%s\n",cloud_userid);
  printf("CLOUD_PASSWORD=%s\n",cloud_password);
  printf("CLOUD_DEVICEID=0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",cloud_device_mac_addr[0],cloud_device_mac_addr[1],cloud_device_mac_addr[2],cloud_device_mac_addr[3],cloud_device_mac_addr[4],cloud_device_mac_addr[5]);
  printf("CLOUD_URL=%s\n",cloud_url);
  printf("CLOUD_TARGET=%s\n",cloud_target);
  printf("CLOUD_VENDORID=0x%x\n",cloud_vendorid); 
  printf("UIAPP_PORT=%d\n",uiapp_port); 
  printf("BROADCAST_PORT=%d\n",broadcast_port);  
  return 0;
}
