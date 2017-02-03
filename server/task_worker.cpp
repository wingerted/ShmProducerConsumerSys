#include "task_worker.h"

#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <unistd.h>
#include <iostream>



TaskWorker::TaskWorker(int id, int buffer_size) {
  this->id = id;
  this->buffer_size = buffer_size;
  this->OnWorkerInit();
  printf("Init\n");

  for (int i = 0; i < buffer_size; ++i) {
    void *buffer = this->InitWorkerBuffer("/Task_Worker_" + std::to_string(id) +
                                          "_Work_Shm_" + std::to_string(i));

    this->OnWorkerBufferInit(buffer, sizeof(WorkerBuffer));

    this->worker_buffer.push_back((WorkerBuffer *)buffer);
  }


  InitSem("/Task_Worker_" + std::to_string(id) + "_Control_Sem",
          "/run/shm/sem.Task_Worker_" + std::to_string(id) + "_Control_Sem");

  for (int i = 0; i < buffer_size; ++i) {
    InitSem("/Task_Worker_" + std::to_string(id) + "_Input_Sem_" + std::to_string(i),
            "/run/shm/sem.Task_Worker_" + std::to_string(id) + "_Input_Sem_" + std::to_string(i),
            &this->worker_sems.input_sems);

    InitSem("/Task_Worker_" + std::to_string(id) + "_Output_Sem_" + std::to_string(i),
            "/run/shm/sem.Task_Worker_" + std::to_string(id) + "_Output_Sem_" + std::to_string(i),
            &this->worker_sems.output_sems);
  }

  printf("Init Over\n");
}

void* TaskWorker::InitWorkerBuffer(std::string work_buffer_name) {
  int fd = shm_open(work_buffer_name.c_str(), O_CREAT | O_RDWR, 0770);
  ftruncate(fd, sizeof(WorkerBuffer));
  fchmod(fd, S_IRWXU | S_IRWXG);

  void *buffer_address = mmap(NULL,
                              sizeof(WorkerBuffer),
                              PROT_READ | PROT_WRITE,
                              MAP_SHARED,
                              fd,
                              0);
  return buffer_address;
}

void TaskWorker::InitSem(std::string sem_name,
                         std::string sem_path,
                         std::vector<sem_t *> *sem_vector) {

  auto sem = sem_open(sem_name.c_str(), O_CREAT | O_RDWR, 0770, 0);
  std::cout << sem << std::endl;

  if (sem_vector != nullptr) {
    sem_vector->push_back(sem);
  } else {
    this->worker_sems.control_sem = sem;
  }
  chmod(sem_path.c_str(), S_IRWXU | S_IRWXG);
}

void TaskWorker::StartRun() {
  printf("Start Run\n");
  int last_working_buffer_id = -1;
  while (true) {
    printf("Start Wait Control\n");
    sem_wait(this->worker_sems.control_sem);
    printf("Wait Control\n");

    for (int i = 0; i < this->buffer_size; ++i) {
      int buffer_index = (i + last_working_buffer_id + 1) % this->buffer_size;

      // 这个input_pid的赋值一定要在trywait前面
      int input_pid = this->worker_buffer[buffer_index]->input_client_pid;

      if (sem_trywait(this->worker_sems.input_sems[buffer_index]) == -1) {
        printf("Try Wait Buffer Index %d Fail\n", buffer_index);
        continue;
      } else {
        printf("Try Wait Buffer Index %d Success\n", buffer_index);

        last_working_buffer_id = buffer_index;
        OnWorkerCompute(buffer_index);

        this->worker_buffer[buffer_index]->output_client_pid = input_pid;
        sem_post(this->worker_sems.output_sems[buffer_index]);

        break;
      }
    }
  }
}
