void init_file();
int create_entry_in_file(const char *uri, const char *hostaddr);
int send_resource_file(int towhom);
int create_entry_in_database_file(const char *hostname, const char *roomname, const char *uri, int state, int power,int power_rat, int resource_type,int auto_status);
void database_file_init();
void populate_prev_resource_status();
