#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/vaddr.h"
#include "process.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}


static void
syscall_handler (struct intr_frame *f UNUSED)
{

    // printf("system CALL!\n");
    int * p = f->esp;
    int system_call = * p;
    int i;

  	switch (system_call){
      case SYS_HALT:
          printf ("SYS HALT!\n");
		      shutdown_power_off();
		      break;

      case SYS_EXIT:
        // printf ("SYS EXIT!, EXIT_CODE: %d\n", *(p+1));
    		thread_current()->parent->exit = true;
    		thread_current()->exit_code = *(p+1);
    		thread_exit();
    		break;

    	case SYS_EXEC:
        printf ("SYS EXEC! FILE_NAME: %s\n", *(p+1));
    		hex_dump(*(p+1),*(p+1),64,true);
    		check_addr(*(p+1));
    		f->eax = process_execute(*(p+1));
    		break;

    	case SYS_CREATE:
        printf ("SYS CREATE! NAME: %s, SIZE : %d\n", *(p+1), *(p+2));
    		check_addr(*(p+1));
    		f->eax = filesys_create(*(p+1),*(p+2));
    		break;

    	case SYS_REMOVE:
        printf ("SYS REMOVE! NAME: %s\n", *(p+1));
    		check_addr(*(p+1));
    		if(filesys_remove(*(p+1))==NULL)
    			f->eax = false;
    		else
    			f->eax = true;
    		break;

    	case SYS_OPEN:
        printf ("SYS OPEN! NAME: %s\n", *(p+1));
    		check_addr(*(p+1));
    		struct file* fptr = filesys_open (*(p+1));
    		if(fptr==NULL)
    			f->eax = -1;
    		else
    		{
    			struct proc_file *pfile = malloc(sizeof(*pfile));
    			pfile->ptr = fptr;
    			pfile->fd = thread_current()->fd_count;
    			thread_current()->fd_count++;
    			list_push_back (&thread_current()->files, &pfile->elem);
    			f->eax = pfile->fd;
    		}
    		break;

    	case SYS_FILESIZE:
        printf ("SYS FILESIZE!\n");
    		f->eax = file_length (list_search(&thread_current()->files, *(p+1))->ptr);
    		break;

    	case SYS_READ:
        printf ("SYS READ! FD: %d\n, Buffer:%s, SIZE: %d\n", *(p+1), *(p+2), *(p+3));
    		check_addr(*(p+2));
    		if(*(p+1)==0)
    		{
    			int i;
    			uint8_t* buffer = *(p+2);
    			for(i=0;i<*(p+3);i++)
    				buffer[i] = input_getc();
    			f->eax = *(p+3);
    		}
    		else
    		{
    			struct proc_file* fptr = list_search(&thread_current()->files, *(p+1));
    			if(fptr==NULL)
    				f->eax=-1;
    			else
    				f->eax = file_read_at (fptr->ptr, *(p+2), *(p+3),0);
    		}

    	case SYS_WRITE:
        // printf ("SYS WRITE! FD: %d, Buffer:%s, SIZE: %d\n", *(p+1), *(p+2), *(p+3));
        check_addr(*(p+2));
    		if(*(p+1)==1)
    		{
    			putbuf(*(p+2),*(p+3));
    			f->eax = *(p+3);
    		}
    		else
    		{
    			struct proc_file* fptr = list_search(&thread_current()->files, *(p+1));
    			if(fptr==NULL)
    				f->eax=-1;
    			else
    				f->eax = file_write_at (fptr->ptr, *(p+2), *(p+3),0);
    		}
        // printf("SYS_WRITE_FINISHED\n");
    		break;

    	case SYS_CLOSE:
        printf("SYS CLOSE! FD: %d\n", *(p+1));
    		close_file(&thread_current()->files,*(p+1));
    		break;


    	default:
    		printf("No match\n");
    		printf("%d\n",*p);
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
