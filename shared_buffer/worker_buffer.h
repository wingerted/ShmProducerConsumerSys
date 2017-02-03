#ifndef WORKER_BUFFER_H
#define WORKER_BUFFER_H

#define MATRIX_MAX_N 3500

struct WorkerBuffer {
  double matrix_A[MATRIX_MAX_N*MATRIX_MAX_N];
  int A_row_num;
  int A_col_num;
  double matrix_X[MATRIX_MAX_N*MATRIX_MAX_N];
  int X_row_num;
  int X_col_num;
  double matrix_B[MATRIX_MAX_N*MATRIX_MAX_N];
  int B_row_num;
  int B_col_num;
  int input_client_pid;
  int output_client_pid;
};

#endif
