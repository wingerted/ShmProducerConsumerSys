struct WorkerMeta {
       1: i32 worker_id,
       2: i32 buffer_id,
       3: i32 worker_num,
       4: i32 buffer_size
}

struct MasterClientMetaState {
       1: i32 worker_id,
       2: i32 buffer_id,
       3: i32 current_client_pid,
       4: i32 last_client_pid,
       5: i64 client_start_timestamp,
}

service MasterRPCService {
	WorkerMeta RequestWorker(1: i32 client_pid),
  list<MasterClientMetaState> GetCurrentClientStates()
}
