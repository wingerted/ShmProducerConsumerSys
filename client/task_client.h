#include <semaphore.h>

#include "../shared_buffer/master_client_meta.h"
#include "../shared_buffer/worker_buffer.h"

class TaskClient {
 public:
  TaskClient(int worker_id, int buffer_id, int worker_num, int buffer_size);

  void run();
  void CopyDataToWorkBuffer(double* matrix_A, int A_row_num, int A_col_num,
                            double* matrix_B, int B_row_num, int B_col_num);
  void CopyDataBack(double* matrix_X, int X_row_num, int X_col_num);

 private:
  int worker_id;
  int buffer_id;
  int worker_num;
  int buffer_size;
  sem_t* input_sem;
  sem_t* output_sem;
  sem_t* control_sem;
  WorkerBuffer* worker_buffer;
  MasterClientMeta* master_client_meta_buffer;
};
