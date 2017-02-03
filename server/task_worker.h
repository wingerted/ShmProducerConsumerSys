#ifndef TASK_WORKER_H
#define TASK_WORKER_H

#include "../shared_buffer/worker_buffer.h"
#include <semaphore.h>
#include <vector>
#include <string>
#include <iostream>

struct WorkerSems {
  std::vector<sem_t *> input_sems;
  std::vector<sem_t *> output_sems;
  sem_t *control_sem;
};

class TaskWorker {
 public:
  TaskWorker(int id, int buffer_size);
  ~TaskWorker() = default;

  virtual void OnWorkerInit() { std::cout << "On Worker Init" << std::endl; };
  virtual void OnWorkerBufferInit(void *buffer_address, int buffer_size) {
    std::cout << "On Worker Buffer Init" << std::endl;
  };
  virtual void OnWorkerCompute(int buffer_index) {
    std::cout << "On Worker Compute" << std::endl;
  };
  void InitSem(std::string sem_name,
               std::string sem_path,
               std::vector<sem_t *> *sem_vector = nullptr);
  void* InitWorkerBuffer(std::string work_buffer_name);
  void StartRun();

 private:
  int id;
  int buffer_size;
  WorkerSems worker_sems;
  std::vector<WorkerBuffer *> worker_buffer;
};

#endif
