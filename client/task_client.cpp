#include "task_client.h"

#include <fcntl.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <unistd.h>
#include <atomic>
#include <iostream>
#include <string>

#include "../tools/helper.h"

double second(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

TaskClient::TaskClient(int worker_id, int buffer_id, int worker_num,
                       int buffer_size) {
  this->worker_id = worker_id;
  this->buffer_id = buffer_id;
  this->worker_num = worker_num;
  this->buffer_size = buffer_size;
  std::cout << "Worker ID: " << worker_id
            << "Buffer ID: " << buffer_id
            << "Worker Num: " << worker_num
            << "Buffer Size: " << buffer_size << std::endl;


  std::string work_shm_name = "/Task_Worker_" + std::to_string(worker_id) +
                              "_Work_Shm_" + std::to_string(buffer_id);
  int fd = shm_open(work_shm_name.c_str(), O_RDWR, 0770);
  ftruncate(fd, sizeof(WorkerBuffer));
  this->worker_buffer = (WorkerBuffer *)mmap(
      NULL, sizeof(WorkerBuffer), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  fd = shm_open("/Master_Client_Shm", O_CREAT | O_RDWR, 0770);
  ftruncate(fd, sizeof(MasterClientMeta) * worker_num * buffer_size);

  void *buffer = mmap(NULL, sizeof(MasterClientMeta) * worker_num * buffer_size,
                      PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  this->master_client_meta_buffer = (MasterClientMeta *)buffer;

  std::string control_sem_name =
      "/Task_Worker_" + std::to_string(worker_id) + "_Control_Sem";
  std::cout << control_sem_name << std::endl;
  this->control_sem = sem_open(control_sem_name.c_str(), O_CREAT | O_RDWR, 0770, 0);
  std::cout << this->control_sem << std::endl;

  std::string input_sem_name = "/Task_Worker_" + std::to_string(worker_id) + "_Input_Sem_" + std::to_string(buffer_id);
  std::cout << input_sem_name.c_str() << std::endl;
  this->input_sem = sem_open(input_sem_name.c_str(), O_CREAT | O_RDWR, 0770, 0);
  std::cout << this->input_sem << std::endl;

  std::string output_sem_name = "/Task_Worker_" + std::to_string(worker_id) +
                                "_Output_Sem_" + std::to_string(buffer_id);
  this->output_sem = sem_open(output_sem_name.c_str(), O_CREAT | O_RDWR, 0770, 0);
  std::cout << this->output_sem << std::endl;
}

void TaskClient::run() {
  std::atomic<bool> need_reset = {false};
  int last_worker_pid =
      this->master_client_meta_buffer[this->worker_id * this->buffer_size +
                                      this->buffer_id]
          .worker_pid;

  std::cout << "Start Run: " << last_worker_pid << std::endl;

  std::cout << this->input_sem << std::endl;

  while (sem_trywait(this->input_sem) == 0);

  std::cout << "Clean Input Over" << std::endl;

  sem_post(this->input_sem);
  std::cout << "Add Input Over" << std::endl;

  sem_post(this->control_sem);
  std::cout << "Add Control Over" << std::endl;



  while (true) {
    std::cout << "In While" << std::endl;
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 10;

    if (sem_timedwait(this->output_sem, &ts) == -1) {
      int sem_result = 0;
      int result = sem_getvalue(this->input_sem, &sem_result);
      if (result == -1) {
        exit(-1);
      }
      if (sem_result == 0) {
        int judge_worker_pid =
            this->master_client_meta_buffer[this->worker_id * this->buffer_size +
                                            this->buffer_id]
                .worker_pid;
        std::cout << "Worker Pid: " << judge_worker_pid << std::endl;
        if (last_worker_pid != judge_worker_pid ||
            !judge_if_pid_exists(judge_worker_pid)) {
          sem_post(this->input_sem);
        }
      }

      sem_post(this->control_sem);
      continue;

    } else {
      std::cout << "Output" << std::endl;
      std::cout << this->worker_buffer->output_client_pid << std::endl;
      std::cout << "Input" << std::endl;
      std::cout << this->worker_buffer->input_client_pid << std::endl;

      if (this->worker_buffer->output_client_pid !=
          this->worker_buffer->input_client_pid) {
        continue;
      } else {
        break;
      }
    }
  }
}

void TaskClient::CopyDataToWorkBuffer(double *matrix_A, int A_row_num,
                                      int A_col_num, double *matrix_B,
                                      int B_row_num, int B_col_num) {
  memcpy(this->worker_buffer->matrix_A, matrix_A,
         sizeof(double) * A_row_num * A_col_num);
  //  double end = second();
  //  double time_solve = end - start;
  //  fprintf (stdout, "timing: memcopy = %10.6f sec\n", time_solve);

  this->worker_buffer->A_col_num = A_col_num;
  this->worker_buffer->A_row_num = A_row_num;

  //  start = second();
  this->worker_buffer->B_col_num = B_col_num;
  this->worker_buffer->B_row_num = B_row_num;
  memcpy(this->worker_buffer->matrix_B, matrix_B,
         sizeof(double) * B_row_num * B_col_num);
  //  printf("\nB_Matrix\n");

  //  end = second();
  //  time_solve = end - start;
  //  fprintf (stdout, "timing: memcopy2 = %10.6f sec\n", time_solve);

  //  printf("MemCopy Over\n");

  this->worker_buffer->input_client_pid =
      this->master_client_meta_buffer[this->worker_id * this->buffer_size + this->buffer_id]
          .current_client_pid;
}

void TaskClient::CopyDataBack(double *matrix_X, int X_row_num, int X_col_num) {
  //  printf("Copy Data Back\n");
  //  printf("\nX_Matrix\n");
  this->master_client_meta_buffer[this->worker_id * this->buffer_size + this->buffer_id]
      .last_client_pid =
      this->master_client_meta_buffer[this->worker_id * this->buffer_size + this->buffer_id]
          .current_client_pid;

  memcpy(matrix_X, this->worker_buffer->matrix_X,
         sizeof(double) * X_row_num * X_col_num);
}

int run_from_python(int worker_id, int buffer_id, int worker_num,
                    int buffer_size, double *matrix_A, int A_row_num,
                    int A_col_num, double *matrix_B, int B_row_num,
                    int B_col_num, double *matrix_X, int X_row_num,
                    int X_col_num) {
  double start = second();
  TaskClient worker = TaskClient(worker_id, buffer_id, worker_num, buffer_size);
  double end = second();
  double time_solve = end - start;
  fprintf(stdout, "timing: Init = %10.6f sec\n", time_solve);

  start = second();
  worker.CopyDataToWorkBuffer(matrix_A, A_row_num, A_col_num, matrix_B,
                              B_row_num, B_col_num);
  end = second();
  time_solve = end - start;
  fprintf(stdout, "timing: Copy To Task = %10.6f sec\n", time_solve);

  start = second();
  worker.run();
  end = second();
  time_solve = end - start;
  fprintf(stdout, "timing: Run = %10.6f sec\n", time_solve);

  start = second();
  worker.CopyDataBack(matrix_X, X_row_num, X_col_num);
  end = second();
  time_solve = end - start;
  fprintf(stdout, "timing: Copy Back = %10.6f sec\n", time_solve);
}
