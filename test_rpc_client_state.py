import time

from rpc.master_rpc_server_py.client_master.MasterRPCService import Client
from rpc.master_rpc_server_py.client_master.ttypes import *
from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

import os
current_pid = os.getpid()
socket = TSocket.TSocket('127.0.0.1', '22222')
transport = TTransport.TBufferedTransport(socket)
protocol = TBinaryProtocol.TBinaryProtocol(transport)
client = Client(protocol)
transport.open()
print("Open!")
#gpu_worker_meta = client.RequestGpuWorker(current_pid)

import datetime
for state in client.GetCurrentClientStates():
    print(state)
    print(datetime.datetime.fromtimestamp(state.client_start_timestamp/1000.0/1000.0).strftime('%Y-%m-%d %H:%M:%S.%f'))
