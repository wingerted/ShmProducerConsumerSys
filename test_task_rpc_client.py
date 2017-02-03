import numpy
import ctypes
import time

from multiprocessing import Pool
from rpc.master_rpc_server_py.client_master.MasterRPCService import Client
from rpc.master_rpc_server_py.client_master.ttypes import *
from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

task_client_lib = ctypes.cdll.LoadLibrary('./client/task_client.so')
task_resolve = task_client_lib._Z15run_from_pythoniiiiPdiiS_iiS_ii

src_matrix = numpy.random.rand(3000, 3000)
src_vector = numpy.random.rand(3000, 1)
dst_vector = numpy.zeros((3000,1))


def run_func():
    import os
    current_pid = os.getpid()
    socket = TSocket.TSocket('127.0.0.1', '22222')
    transport = TTransport.TBufferedTransport(socket)
    protocol = TBinaryProtocol.TBinaryProtocol(transport)
    client = Client(protocol)
    transport.open()
    print("Open!")
    task_worker_meta = client.RequestWorker(current_pid)
    print(task_worker_meta)
    log_file = open('./Worker_id_'+str(task_worker_meta.worker_id)+'_Buffer_id_'+str(task_worker_meta.buffer_id), 'w+')
    for _ in xrange(100):
        print("in range")
        b = time.time()
        task_resolve(
            task_worker_meta.worker_id,
            task_worker_meta.buffer_id,
            task_worker_meta.worker_num,
            task_worker_meta.buffer_size,
            ctypes.c_void_p(src_matrix.ctypes.data), src_matrix.shape[0], src_matrix.shape[1],
            ctypes.c_void_p(src_vector.ctypes.data), src_vector.shape[0], 1,
            ctypes.c_void_p(dst_vector.ctypes.data), dst_vector.shape[0], 1
        )
        e = time.time()

        print("over")
        print 'solve time', e - b

        log_file.write('solve time: '+str(e-b))
        log_file.write("\n")
        log_file.flush()

run_func()
