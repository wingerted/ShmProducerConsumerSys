#ifndef MASTER_CLIENT_META_H
#define MASTER_CLIENT_META_H

#include<semaphore.h>

struct MasterClientMeta {
  int last_client_pid;
  int current_client_pid;
  int worker_pid;
  int64_t client_start_timestamp;
  //sem_t control_sem;
};

#endif
