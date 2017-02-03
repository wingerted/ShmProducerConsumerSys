#ifndef MASTER_RPC_SERVER_H
#define MASTER_RPC_SERVER_H

#include <pthread.h>
#include <thrift/TToString.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include <boost/make_shared.hpp>

#include "../shared_buffer/worker_stat.h"
#include "../shared_buffer/master_client_meta.h"
#include "./master_rpc_server_cpp/MasterRPCService.h"
#include "./master_rpc_server_cpp/client_master_types.h"


class MasterRPCServiceHandler : virtual public MasterRPCServiceIf {
public:
  MasterRPCServiceHandler(int worker_num, int buffer_size,
                          MasterClientMeta *master_client_meta_buffer);
  void RequestWorker(WorkerMeta &_return, const int32_t client_pid);
  void GetCurrentClientStates(std::vector<MasterClientMetaState> &_return);

public:
  std::map<int, std::map<int, int>> GetWorkerId();

private:
  int worker_num;
  int buffer_size;
  MasterClientMeta *master_client_meta_buffer;
  pthread_mutex_t meta_buffer_mutex;
};

class MasterRPCServer {
public:
  MasterRPCServer(int worker_num, int buffer_size);
  int StartMasterRPCServer();
  void InitMasterRPCServer(int server_port);
  void InitMasterClientMetaBuffer(const std::vector<WorkerStat>& worker_stat_vec);

public:
  MasterClientMeta *master_client_meta_buffer;

private:
  int worker_num;
  int buffer_size;
  boost::shared_ptr<MasterRPCServiceHandler> service_handler;
  boost::shared_ptr<apache::thrift::server::TThreadedServer> rpc_server;
};

#endif
