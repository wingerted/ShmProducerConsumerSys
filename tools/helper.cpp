#include "./helper.h"

#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <iostream>
#include <string>

int64_t second_since_epoch(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

bool judge_if_pid_exists(int pid) {
  if (pid == 0) {
    return false;
  }

  std::string pathname = "/proc/" + std::to_string(pid);

  struct stat st;
  int return_value = ::stat(pathname.c_str(), &st);

  if (return_value != -1) {
    return true;
  } else {
    return false;
  }
}
