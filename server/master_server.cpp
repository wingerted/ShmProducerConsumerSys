#include "master_server.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "../rpc/master_rpc_server.h"
#include "../tools/helper.h"
#include "task_worker.h"

int WorkerMasterServer::GenerateTaskWorker(int worker_id,
                                           int worker_buffer_size) {
  int worker_pid = fork();

  if (worker_pid < 0) {
    perror("Fork Task Worker Process Failed !");
    return -1;
  } else if (worker_pid == 0) {
    // 这里已经进入到子进程，开始执行Worker任务，结束后exit跳过剩余流程
    TaskWorker(worker_id, worker_buffer_size).StartRun();
    exit(0);
  }

  return worker_pid;
}

void WorkerMasterServer::GenerateAllTaskWorkers() {
  for (int worker_index = 0; worker_index < worker_num; ++worker_index) {
    int worker_pid = this->GenerateTaskWorker(worker_index, worker_buffer_size);
    // 这里父进程拿到了子进程ID，进行Worker状态设置
    WorkerStat worker_stat;
    worker_stat.pid = worker_pid;
    worker_stat.worker_id = worker_index;
    this->worker_stat_vec.push_back(worker_stat);
  }
}

void WorkerMasterServer::StartRun() {
  while (true) {
    int child_pid = wait(NULL);
    for (auto &worker_stat : this->worker_stat_vec) {
      if (worker_stat.pid == child_pid) {
        int new_worker_pid =
            this->GenerateTaskWorker(worker_stat.worker_id, worker_buffer_size);

        std::cout << "Old Pid: " << worker_stat.pid << std::endl;
        std::cout << "New Pid: " << new_worker_pid << std::endl;
        worker_stat.pid = new_worker_pid;
      }
    }
  }
}

int main() {
  const int worker_num = 3;
  const int worker_buffer_size = 3;
  const int rpc_server_port = 22222;

  auto worker_master_server =
      WorkerMasterServer(worker_num, worker_buffer_size);

  worker_master_server.GenerateAllTaskWorkers();

  MasterRPCServer rpc_server{worker_num, worker_buffer_size};
  rpc_server.InitMasterClientMetaBuffer(worker_master_server.worker_stat_vec);
  rpc_server.InitMasterRPCServer(rpc_server_port);
  if (rpc_server.StartMasterRPCServer() < 0) {
    return -1;
  } else {
    worker_master_server.StartRun();
  }

  return 0;
}
