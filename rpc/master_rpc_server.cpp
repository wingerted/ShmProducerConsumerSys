#include "master_rpc_server.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <utility>

#include "../tools/helper.h"

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;

MasterRPCServiceHandler::MasterRPCServiceHandler(
    int worker_num, int buffer_size,
    MasterClientMeta *master_client_meta_buffer) {
  this->worker_num = worker_num;
  this->buffer_size = buffer_size;
  this->master_client_meta_buffer = master_client_meta_buffer;
  pthread_mutex_init(&this->meta_buffer_mutex, NULL);
}

void MasterRPCServiceHandler::GetCurrentClientStates(
    std::vector<MasterClientMetaState> &_return) {
  pthread_mutex_lock(&this->meta_buffer_mutex);
  for (int buffer_id = 0; buffer_id < this->buffer_size; ++buffer_id) {
    for (int worker_id = 0; worker_id < this->worker_num; ++worker_id) {
      MasterClientMetaState master_client_meta_state;
      master_client_meta_state.worker_id = worker_id;
      master_client_meta_state.buffer_id = buffer_id;
      master_client_meta_state.current_client_pid =
          this->master_client_meta_buffer[worker_id * this->buffer_size +
                                          buffer_id]
              .current_client_pid;
      master_client_meta_state.last_client_pid =
          this->master_client_meta_buffer[worker_id * this->buffer_size +
                                          buffer_id]
              .last_client_pid;
      master_client_meta_state.client_start_timestamp =
          this->master_client_meta_buffer[worker_id * this->buffer_size +
                                          buffer_id]
              .client_start_timestamp;
      _return.push_back(master_client_meta_state);
    }
  }
  pthread_mutex_unlock(&this->meta_buffer_mutex);
}

std::map<int, std::map<int, int>> MasterRPCServiceHandler::GetWorkerId() {
  std::map<int, std::map<int, int>> result_worker_map;
  for (int worker_id = 0; worker_id < this->worker_num; ++worker_id) {
    int worker_used_buffer_num = 0;
    std::map<int, int> empty_buffer_map;
    for (int buffer_id = 0; buffer_id < this->buffer_size; ++buffer_id) {
      bool if_pid_exists = judge_if_pid_exists(
          this->master_client_meta_buffer[worker_id * this->buffer_size +
                                          buffer_id]
              .current_client_pid);
      if (if_pid_exists) {
        ++worker_used_buffer_num;
      } else {
        empty_buffer_map[worker_id] = buffer_id;
      }
    }

    result_worker_map[worker_used_buffer_num] = empty_buffer_map;
  }

  return result_worker_map;
}

void MasterRPCServiceHandler::RequestWorker(WorkerMeta &_return,
                                            const int32_t client_pid) {
  std::cout << "In Handler" << std::endl;
  while (true) {
    std::cout << "Start Lock" << std::endl;
    pthread_mutex_lock(&this->meta_buffer_mutex);

    std::map<int, std::map<int, int>> worker_buffer_map = this->GetWorkerId();

    if (worker_buffer_map.begin()->second.size() == 0) {
      pthread_mutex_unlock(&this->meta_buffer_mutex);
      std::cout << "No Empty Worker" << std::endl;
      sleep(10);
    } else {
      int worker_id = worker_buffer_map.begin()->second.begin()->first;
      int buffer_id = worker_buffer_map.begin()->second.begin()->second;

      printf("Return Worker Id: %d, Buffer Id: %d\n", worker_id, buffer_id);
      _return.worker_id = worker_id;
      _return.buffer_id = buffer_id;
      _return.worker_num = this->worker_num;
      _return.buffer_size = this->buffer_size;
      this->master_client_meta_buffer[worker_id * this->buffer_size + buffer_id]
          .current_client_pid = client_pid;
      this->master_client_meta_buffer[worker_id * this->buffer_size + buffer_id]
          .client_start_timestamp = second_since_epoch();
      std::cout
          << this->master_client_meta_buffer[worker_id * this->buffer_size +
                                             buffer_id]
                 .client_start_timestamp
          << std::endl;
      pthread_mutex_unlock(&this->meta_buffer_mutex);
      return;
    }
  }
}

MasterRPCServer::MasterRPCServer(int worker_num, int buffer_size) {
  this->worker_num = worker_num;
  this->buffer_size = buffer_size;

  int fd = shm_open("/Master_Client_Shm", O_CREAT | O_RDWR, 0777);
  ftruncate(fd, sizeof(MasterClientMeta) * worker_num * buffer_size);
  fchmod(fd, S_IRWXU | S_IRWXG);

  void *buffer = mmap(NULL, sizeof(MasterClientMeta) * worker_num * buffer_size,
                      PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  this->master_client_meta_buffer = (MasterClientMeta *)buffer;
}

void MasterRPCServer::InitMasterClientMetaBuffer(
    const std::vector<WorkerStat> &worker_stat_vec) {
  for (auto &worker_stat : worker_stat_vec) {
    for (int worker_buffer_id = 0; worker_buffer_id < this->buffer_size;
         ++worker_buffer_id) {
      this->master_client_meta_buffer[worker_stat.worker_id *
                                          this->buffer_size +
                                      worker_buffer_id]
          .worker_pid = worker_stat.pid;
    }
  }
}

void MasterRPCServer::InitMasterRPCServer(int server_port) {
  this->service_handler = boost::shared_ptr<MasterRPCServiceHandler>(
      new MasterRPCServiceHandler(this->worker_num, this->buffer_size,
                                  this->master_client_meta_buffer));
  std::cout << "End Service Handler" << std::endl;

  boost::shared_ptr<TProcessor> processor(
      new MasterRPCServiceProcessor(this->service_handler));
  std::cout << "End Processor" << std::endl;

  boost::shared_ptr<TProtocolFactory> protocolFactory(
      new TBinaryProtocolFactory());
  std::cout << "End Factory" << std::endl;

  boost::shared_ptr<TServerTransport> serverTransport(
      new TServerSocket(server_port));
  std::cout << "End Transport" << std::endl;

  boost::shared_ptr<TTransportFactory> transportFactory(
      new TBufferedTransportFactory());
  std::cout << "End Transport Factory" << std::endl;

  this->rpc_server = boost::shared_ptr<apache::thrift::server::TThreadedServer>(
      new apache::thrift::server::TThreadedServer(
          processor, serverTransport, transportFactory, protocolFactory));
}

int MasterRPCServer::StartMasterRPCServer() {
  int server_pid = fork();

  if (server_pid < 0) {
    perror("Fork RPC Server Failed !");
  } else if (server_pid == 0) {
    // 这里已经进入到子进程，开始执行Server，结束后exit跳过剩余流程
    std::cout << "Starting the server..." << std::endl;
    this->rpc_server->serve();

    std::cout << "Done." << std::endl;
    exit(0);
  }

  return server_pid;
}

