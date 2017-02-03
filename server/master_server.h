#ifndef MASTER_SERVER_H
#define MASTER_SERVER_H

#include <vector>
#include "../shared_buffer/worker_stat.h"

class WorkerMasterServer {
 public:
  WorkerMasterServer(int worker_num, int worker_buffer_size) {
    this->worker_num = worker_num;
    this->worker_buffer_size = worker_buffer_size;
  };
  ~WorkerMasterServer() = default;

  void GenerateAllTaskWorkers();

  void StartRun();

 private:
  int GenerateTaskWorker(int worker_id, int worker_buffer_size);

 public:
  std::vector<WorkerStat> worker_stat_vec;
  int worker_buffer_size;

 private:
  int worker_num;
};

#endif
