#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/vaddr.h"
#include "process.h"



/********************UTILITY FUNCTIONS**********************/
void* check_vaddr(const void *vaddr){
	if (!is_user_vaddr(vaddr)){
    exit(-1);
		return NULL;
  }
	void *page = pagedir_get_page(thread_current()->pagedir, vaddr);
	if (!page){
		exit(-1);
		return NULL;
	}
	return page;
}

struct list_elem* pfile_list_elem_search(struct list* files, int fd){
  struct list_elem *e;
  for (e = list_begin (files); e != list_end (files); e = list_next (e)){
      struct process_file *pfile = list_entry (e, struct process_file, elem);
      if(pfile->fd == fd) return e;
    }
   return NULL;
}

struct process_file* pfile_search(struct list* files, int fd){
	struct list_elem *e = pfile_list_elem_search(files, fd);
  return e ? list_entry(e, struct process_file, elem) : NULL;
}

void close_files(struct list* files, int fd){ // Can close multiple files
	struct list_elem *e;
  for (e = list_begin (files); e != list_end (files);e = list_next (e)){
      struct process_file *pfile = list_entry (e, struct process_file, elem);
      if(pfile->fd == fd){
      	file_close(pfile->f);
      	list_remove(e);
      }
    }
}

void close_all_files(struct list* files){
	struct list_elem *e;
  for (e = list_begin (files); e != list_end (files); e = list_next (e)){
      struct process_file *pfile = list_entry (e, struct process_file, elem);
    	file_close(pfile->f);
    	list_remove(e);
    }
}
/********************UTILITY FUNCTIONS FIN**********************/

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
  check_vaddr(file);
  return process_execute(file);
}

int wait (pid_t pid){
  return process_wait(pid);
}

bool create (const char *file, unsigned initial_size){
  check_vaddr(file);
  LOCK_FILE();
  bool return_value =  filesys_create(file,initial_size);
  RELEASE_FILE();
  return return_value;
}

bool remove (const char *file){
  check_vaddr(file);
  LOCK_FILE();
  bool return_value =  filesys_remove(file);
  RELEASE_FILE();
  return return_value;
}

int open (const char *file){
  check_vaddr(file);
  LOCK_FILE();
  struct file* fptr = filesys_open(file);
  RELEASE_FILE();

  if(fptr==NULL)
    return -1;

  struct process_file *pfile = malloc(sizeof(*pfile));
  pfile->f = fptr;
  pfile->fd = thread_current()->fd_count++; // BASIC fd is 2
  list_push_back (&thread_current()->files, &pfile->elem);
  return pfile->fd;
}

int filesize (int fd){
  LOCK_FILE();
  int return_value =  file_length (pfile_search(&thread_current()->files, fd)->f);
  RELEASE_FILE();
  return return_value;
}

int read (int fd, void *buffer, unsigned length){
  check_vaddr(buffer);
  if(fd==STDIN_FILENO){
    char* casted_buffer = buffer;
    int i;
    for(i=0;i<length;i++)
      casted_buffer[i] = input_getc();
    return length;
  }
  else{
    struct process_file* pfile = pfile_search(&thread_current()->files, fd);
    LOCK_FILE();
    int return_value =  pfile==NULL ? -1 : file_read_at (pfile->f, buffer, length,0);
    RELEASE_FILE();
    return return_value;
  }
}

int write (int fd, const void *buffer, unsigned length){
  check_vaddr(buffer);
  if(fd == STDOUT_FILENO){
    if (length>250)
      exit(-1);
    putbuf(buffer,length);
    return length;
  }else{
    struct process_file* pfile = pfile_search(&thread_current()->files, fd);
    LOCK_FILE();
    int return_value =  pfile==NULL ? -1 : file_write_at (pfile->f, buffer, length,0);
    RELEASE_FILE();
    return return_value;
  }
}

void seek (int fd, unsigned position){
  struct process_file* pfile = pfile_search(&thread_current()->files, fd);
  LOCK_FILE();
  file_seek(pfile->f, position);
  RELEASE_FILE();
}

unsigned tell (int fd){
  struct process_file* pfile = pfile_search(&thread_current()->files, fd);
  LOCK_FILE();
  unsigned return_value =  file_tell(pfile->f);
  RELEASE_FILE();
  return return_value;
}

void close (int fd){
  close_files(&thread_current()->files,fd);
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

    check_vaddr(esp);

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

      case SYS_WAIT:
        *eax = wait(arg1);
        break;

    	case SYS_CREATE:
        // printf ("SYS CREATE! NAME: %s, SIZE : %d\n", *(esp+1), *(esp+2));
        // LOCK_FILE();
    		*eax = create(arg1,arg2);
        // RELEASE_FILE();
    		break;

    	case SYS_REMOVE:
        // printf ("SYS REMOVE! NAME: %s\n", *(esp+1));
        // LOCK_FILE();
    		*eax = remove(arg1);
        // RELEASE_FILE();
    		break;

    	case SYS_OPEN:
        // printf ("SYS OPEN! NAME: %s\n", *(esp+1));
        // LOCK_FILE();
        *eax = open(arg1);
        // RELEASE_FILE();
    		break;

    	case SYS_FILESIZE:
        // printf ("SYS FILESIZE!\n");
        // LOCK_FILE();
    		*eax = filesize(arg1);
        // RELEASE_FILE();
    		break;

    	case SYS_READ:
        // printf ("SYS READ! FD: %d\n, Buffer:%s, SIZE: %d\n", *(esp+1), *(esp+2), *(esp+3));
        // LOCK_FILE();
        *eax = read(arg1,arg2,arg3);
        // RELEASE_FILE();
        break;

    	case SYS_WRITE:
        // printf ("SYS WRITE! FD: %d, Buffer:%s, SIZE: %d\n", *(esp+1), *(esp+2), *(esp+3));
        // check_vaddr(*(esp+2));
        // LOCK_FILE();
    		*eax = write(arg1,arg2,arg3);
        // RELEASE_FILE();
        // printf("SYS_WRITE_FINISHED\n");
    		break;

      case SYS_SEEK:
        // LOCK_FILE();
        seek(arg1, arg2);
        // RELEASE_FILE();
        break;

      case SYS_TELL:
        // LOCK_FILE();
        *eax = tell(arg1);
        // RELEASE_FILE();
        break;

    	case SYS_CLOSE:
        // printf("SYS CLOSE! FD: %d\n", *(esp+1));
        // LOCK_FILE();
    		close(arg1);
        // RELEASE_FILE();
    		break;

    	default:
    		printf("!!!!!NO MATCHING SYSTEM CALL ERROR!!!!!\n");
    		printf("%d\n",*esp);
    }
}
