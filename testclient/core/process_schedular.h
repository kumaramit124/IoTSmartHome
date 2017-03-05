void process_schedular_init();
int add_in_process_queue(int (*funcptr)(void *), void *arg);
int remove_from_process_queue(void *funcptr, void *arg);
void process_start_all(void);
void process_stop_all(void);


