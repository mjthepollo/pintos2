#include "threads/thread.h"

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

struct proc_file {
  struct file* ptr;
  int fd;
  struct list_elem elem;
};

void syscall_init (void);

void* check_addr(const void*);

struct proc_file* list_search(struct list* files, int fd);


#endif /* userprog/syscall.h */
