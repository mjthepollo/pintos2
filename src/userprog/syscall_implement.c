#include "userprog/syscall_implement.h"
#include "devices/shutdown.h"
#include "process.h"

void halt () {
  shutdown_power_off();
}

void exit (int status) {
  thread_current()->parent->exit = true;
  thread_current()->exit_code = status;
  thread_exit();
}

pid_t exec (const char *file){
  process_execute(file);
}

int wait (pid_t){
  return 1;
}
bool create (const char *file, unsigned initial_size){
  return filesys_create(file,initial_size);
}
bool remove (const char *file){
   return filesys_remove(file);
}
// int open (const char *file){
//
// }
// int filesize (int fd);
// int read (int fd, void *buffer, unsigned length);
// int write (int fd, const void *buffer, unsigned length);
// void seek (int fd, unsigned position);
// unsigned tell (int fd);
// void close (int fd);
