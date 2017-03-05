#ifdef __cplusplus
extern "C"{
#endif
int cloud_init (int *cloud_device_mac_addr,int cloud_vendorid,char *cloud_url);
int cloud_deinit(void);
void registerIotCallback(int (*functptr)(int, int ,struct iot_resource *));
int send_file_to_cloud(char const *buffer , char file_mode);
#ifdef __cplusplus
}
#endif
