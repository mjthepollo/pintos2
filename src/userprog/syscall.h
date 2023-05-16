#include "threads/thread.h"
#include "threads/synch.h"

#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

typedef int pid_t;

void halt (void) NO_RETURN;
void exit (int status) NO_RETURN;
pid_t exec (const char *file);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned length);
int write (int fd, const void *buffer, unsigned length);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);

struct process_file {
  struct file* f;
  int fd;
  struct list_elem elem;
};

void syscall_init (void);

void* check_vaddr(const void*);
struct list* list_serarch(struct list* files, int fd);
struct process_file* pfile_search(struct list* files, int fd);
void close_all_files(struct list* files);
void close_files(struct list* files, int fd);

#endif /* userprog/syscall.h */
