#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/vaddr.h"
#include "process.h"


static void syscall_handler (struct intr_frame *);

void halt () {
  shutdown_power_off();
}

void exit (int status) {
  thread_current()->parent->exit = true;
  thread_current()->exit_code = status;
  thread_exit();
}

pid_t exec (const char *file){
  return process_execute(file);
}

int wait (pid_t pid){
  return 1;
}

bool create (const char *file, unsigned initial_size){
  return filesys_create(file,initial_size);
}

bool remove (const char *file){
   return filesys_remove(file);
}

int open (const char *file){
  struct file* fptr = filesys_open(file);
  if(fptr==NULL)
    return -1;

  struct proc_file *pfile = malloc(sizeof(*pfile));
  pfile->ptr = fptr;
  pfile->fd = thread_current()->fd_count;
  thread_current()->fd_count++;
  list_push_back (&thread_current()->files, &pfile->elem);
  return pfile->fd;
}

int filesize (int fd){
  return file_length (list_search(&thread_current()->files, fd)->ptr);
}

int read (int fd, void *buffer, unsigned length){
  if(fd==0){
    char* casted_buffer = buffer;
    int i;
    for(i=0;i<length;i++)
      casted_buffer[i] = input_getc();
    return length;
  }
  else{
    struct proc_file* fptr = list_search(&thread_current()->files, fd);
    return fptr==NULL ? -1 : file_read_at (fptr->ptr, buffer, length,0);
  }
}

int write (int fd, const void *buffer, unsigned length){
  if(fd == 1){
    putbuf(buffer,length);
    return length;
  }else{
    struct proc_file* fptr = list_search(&thread_current()->files, fd);
    return fptr==NULL ? -1 : file_write_at (fptr->ptr, buffer, length,0);
  }
}

void seek (int fd, unsigned position){

}

unsigned tell (int fd){

}

void close (int fd){
  close_file(&thread_current()->files,fd);
}


/*-------------NOW SYSCALL HANDLERS--------------*/

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f UNUSED)
{

    // printf("system CALL!\n");
    int* esp = f->esp;
    int* arg1 = *(esp+1);
    int* arg2 = *(esp+2);
    int* arg3 = *(esp+3);
    int system_call = *esp;
    uint32_t* eax = &(f->eax);
    int i;

  	switch (system_call){
      case SYS_HALT:
        // printf ("SYS HALT!\n");
		    halt();
		    break;

      case SYS_EXIT:
        // printf ("SYS EXIT!, EXIT_CODE: %d\n", *(esp+1));
    		exit(arg1);
    		break;

    	case SYS_EXEC:
        // printf ("SYS EXEC! FILE_NAME: %s\n", *(esp+1));
    		*eax = exec(arg1);
    		break;

    	case SYS_CREATE:
        // printf ("SYS CREATE! NAME: %s, SIZE : %d\n", *(esp+1), *(esp+2));
    		*eax = create(arg1,arg2);
    		break;

    	case SYS_REMOVE:
        // printf ("SYS REMOVE! NAME: %s\n", *(esp+1));
    		*eax = remove(arg1);
    		break;

    	case SYS_OPEN:
        // printf ("SYS OPEN! NAME: %s\n", *(esp+1));
        check_addr(arg1);
        *eax = open(arg1);
    		break;

    	case SYS_FILESIZE:
        // printf ("SYS FILESIZE!\n");
    		*eax = filesize(arg1);
    		break;

    	case SYS_READ:
        // printf ("SYS READ! FD: %d\n, Buffer:%s, SIZE: %d\n", *(esp+1), *(esp+2), *(esp+3));
        *eax = read(arg1,arg2,arg3);
        break;

    	case SYS_WRITE:
        // printf ("SYS WRITE! FD: %d, Buffer:%s, SIZE: %d\n", *(esp+1), *(esp+2), *(esp+3));
        // check_addr(*(esp+2));
    		*eax = write(arg1,arg2,arg3);
        // printf("SYS_WRITE_FINISHED\n");
    		break;

    	case SYS_CLOSE:
        // printf("SYS CLOSE! FD: %d\n", *(esp+1));
    		close(arg1);
    		break;

    	default:
    		printf("No match\n");
    		printf("%d\n",*esp);
    }
}



void* check_addr(const void *vaddr)
{
	if (!is_user_vaddr(vaddr))
	{
		thread_current()->parent->exit = true;
		thread_current()->exit_code = -1;
		thread_exit();
		return 0;
	}
	void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!ptr)
	{
		thread_current()->parent->exit = true;
		thread_current()->exit_code = -1;
		thread_exit();
		return 0;
	}
	return ptr;
}

struct proc_file* list_search(struct list* files, int fd)
{

	struct list_elem *e;

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          struct proc_file *f = list_entry (e, struct proc_file, elem);
          if(f->fd == fd)
          	return f;
        }
   return NULL;
}

void close_file(struct list* files, int fd)
{

	struct list_elem *e;

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          struct proc_file *f = list_entry (e, struct proc_file, elem);
          if(f->fd == fd)
          {
          	file_close(f->ptr);
          	list_remove(e);
          }
        }
}

void close_all_files(struct list* files)
{

	struct list_elem *e;

      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          struct proc_file *f = list_entry (e, struct proc_file, elem);

	      	file_close(f->ptr);
	      	list_remove(e);
        }
}
