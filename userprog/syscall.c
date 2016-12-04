#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "threads/synch.h"
#include "filesys/directory.h"
#include "filesys/inode.h"

//#define SYSCALL_DEBUG 1

struct child_list_elem* add_child_to_list(struct thread* parent_thread, tid_t pid);
int add_file_to_fd_table(struct thread* current_thread, struct file* fp, bool warning);

static int
get_user (const uint8_t *uaddr);
static void syscall_handler (struct intr_frame *);
struct list_elem* find_fd_element(int fd, struct thread* current_thread);
bool create (const char *file, unsigned initial_size);
int open (const char *file);
unsigned tell (int fd);
bool seek(int fd, unsigned offset);
bool close (int fd);
void exit (int status, struct intr_frame *f);
int write (int fd, const void *buffer, unsigned size, bool * warning);
static struct lock fd_lock;
void
syscall_init (void)
{
  lock_init(&read_write_lock);
  lock_init(&fd_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&open_close_lock);
  lock_init(&find_child);
}

static void
syscall_handler (struct intr_frame *f) //UNUSED)
{




	// check for valid esp pointer (esp < PHYS_BASE)
	if( !is_user_vaddr( f->esp))
		exit(-1, f);

	// Check if stack size is over the limit of 8 mb
	// specifically to pass the test wait-kill
	// see also : http://courses.cs.vt.edu/cs3204/spring2006/gback/pintos/doc/pintos_5.html#SEC101
	//printf("%u", (uint32_t) PHYS_BASE - (uint32_t) f->esp);
	//if(( (uint32_t) PHYS_BASE - (uint32_t) f->esp) > ( (uint32_t ) 3086692358))
	//	exit(-1, f);
	//Check for valid esp read access
	if(get_user((char*) f->esp) == -1)
		exit(-1, f);

	//Assume that the esp pointer goes to the top of the stack (looks at return address)
	uint32_t system_call_number = * (uint32_t**)(f->esp+0); //Create a pointer to the top of the stack (looks at argv[0])
	uint32_t* stack_ptr =  (uint32_t*)(f->esp+0); // Two pointers with same address, but using different names

	// Check for valid arg pointers
  	if (!( is_user_vaddr (stack_ptr + 1) && is_user_vaddr (stack_ptr + 2) && is_user_vaddr (stack_ptr + 3)))
    	exit (-1,f);
	// to avoid confusion in usage
	char* name = NULL;
	uint32_t file_size = 0;
	int fd = -1;
/*
	if (pid_t == 0) { //The process id is 0 if the process is a child
		struct child_list *list = struct child_list; //Point to the existing list of children
		while (list->next != NULL) { //Iterate through the list
			list = list->next;
		}
		struct child_list *newChild = malloc(struct child_list);
		list->next = newChild; //Link the list
		newChild->pid = find_thread(pid_t)->tid; //Get the thread id of the current process
		newChild->status = RUNNING; //Since you know that the process is running
		newChild->next = NULL;
	}*/

	switch(system_call_number) { //This gives us the command that needs to be executed
		case SYS_CREATE: //A pre-defined constant that refers to a "create" call
		{
			name = *(stack_ptr+1); //With this, we can load the name of the file
			if (name == NULL) {
				exit(-1, f); //If the pointer or file name is empty, then return an error code
			}
			file_size = *(stack_ptr+2); //Now get the second arg: the size of the file
			f->eax = create(name, file_size); //Create the file and then save the status to the eax register
			break;
		}
			//(Does this mean that eax is just some storage register. What is it really??)
		case SYS_OPEN: //A pre-defined constant that refers to an "open" call
		{
			name = *(stack_ptr+1); //This looks just to the first and only needed parameter, the file to open
			if (name == NULL) { //Check for a non-existant file of course
				exit(-1, f);
			}
			else {
				if(get_user(name) == -1) // check if pointer to name is actually valid
					exit(-1, f);
			}


			fd = open(name); //Going to refer from eax from now on as the "status" register
			//if(fd == -1)
			//	exit(-1, f);
			f->eax = fd;
			break;
		}
		case SYS_SEEK:
		{
			fd = *(stack_ptr + 1);
			if(fd <= 0)
			{
				exit(-1, f);
			}
			file_size = *(stack_ptr + 2);
			f->eax = seek(fd, file_size);
			break;
		}
		case SYS_TELL:
		{
			fd = *(stack_ptr + 1);
			if(fd <= 0)
			{
				exit(-1, f);
			}
			f->eax = tell(fd);
			break;
		}
		case SYS_CLOSE:
		{
			fd = *(stack_ptr+1); //Just do something almost exactly the same as what was done for SYS_CREATE
			if (fd <= 0) {
				exit(-1, f); //If the pointer or file name is empty, then return an error code
			}
			file_size = *(stack_ptr+2);
			fd = close(fd); //The only line different from SYS_OPEN
			if(fd == false)
				exit(-1, f);
			f->eax = fd;
			break;
		}
		case SYS_READ:
		{

			fd = *(stack_ptr+1);
			void* buffer = *(stack_ptr+2);
			// checks for buffer < PHYS_BASE
			if( !is_user_vaddr(buffer))
				exit(-1, f);
			//Check for valid buffer read access
			if(get_user(buffer) == -1)
				exit(-1, f);


			file_size = *(stack_ptr+3);
			int size_read = read(fd, buffer, file_size);
			if(size_read == -1)
				exit(-1, f);
			else{
				f->eax = size_read;
			}
			break;
		}
		case SYS_WRITE:
		{
			fd = *(stack_ptr+1);
			void* buffer = *(stack_ptr+2);
			// checks for buffer < PHYS_BASE
			if( !is_user_vaddr( buffer) )
				exit(-1, f);
			//Check for valid buffer read access
			if(get_user(buffer) == -1)
					exit(-1, f);
			file_size = *(stack_ptr+3);
			bool warning = false;
			int size_write = write(fd, buffer, file_size, &warning);
			if(size_write == -1 && !warning)
				exit(-1, f);
			else
				f->eax = size_write;

			break;
		}


		case SYS_FILESIZE:
		{
			fd = *(stack_ptr + 1);
			f->eax = filesize_get(fd);
			break;
		}

		case SYS_REMOVE:
		{
			name = *(stack_ptr + 1);
			if (name == NULL) { //Check for a non-existant file of course
				exit(-1, f);
			}
			else {
				if(get_user(name) == -1) // check if pointer to name is actually valid
					exit(-1, f);
			}

			f->eax = filesys_remove(name);

			break;
		}

		case SYS_EXIT:
	      {
			fd = *(stack_ptr+1);
			f->eax = fd;
			exit(fd, f);
			break;
	      }

	    case SYS_EXEC:
	    {
	    	name = *(stack_ptr+1);
	    	f->eax = exec(name);
	    	break;
	    }

	    case SYS_WAIT:
	    {
	    	fd = *(stack_ptr + 1);
	    	f->eax = wait(fd);
	    	break;
	    }

	    case SYS_HALT:
	    {
	    	halt();
	    	break;
	    }

	    /*Creates the directory named dir, which may be relative or absolute. 
	    Returns true if successful, false on failure. Fails if dir already exists 
	    or if any directory name in dir, besides the last, does not already exist. 
	    That is, mkdir("/a/b/c") succeeds only 
	    if /a/b already exists and /a/b/c does not.*/
	    case SYS_MKDIR:
	    {
	    	name = *(stack_ptr + 1);
			if (name == NULL) { //Check for a non-existant file of course
				exit(-1, f);
			}
			else {
				if(get_user(name) == -1) // check if pointer to name is actually valid
					exit(-1, f);
			}

			f->eax = filesys_create(name, 16*sizeof (struct dir_entry), true);
			break;

	    }
	    /*
	    Changes the current working directory of the process to dir, which may be 
	    relative or absolute. Returns true if successful, false on failure.*/
	    case SYS_CHDIR:
	    {
	        name = *(stack_ptr + 1);
			if (name == NULL) { //Check for a non-existant file of course
				exit(-1, f);
			}
			else {
				if(get_user(name) == -1) // check if pointer to name is actually valid
					exit(-1, f);
			}

			f->eax = filesys_open(name, true, NULL) != NULL ? true : false;
			break;

	    }

	    /*Returns true if fd represents a directory, 
	    false if it represents an ordinary file.*/
		case SYS_ISDIR:
		{
			fd = *(stack_ptr + 1);
			struct thread* current_thread = thread_current();
			struct list_elem* e = find_fd_element(fd, current_thread);
			if(e == NULL) // This should never happen
			{ 
				f->eax = false; // Directory doesn't exist
			}
			else
			{
				struct  fd_list_element *fd_element = 
				list_entry (e, struct fd_list_element, elem_fd);
				f->eax = fd_element->warning;
			}
			break;
		}

		/*Returns the inode number of the inode associated with fd, 
		which may represent an ordinary file or a directory.
		An inode number persistently identifies a file or directory. 
		It is unique during the file's existence. In Pintos, 
		the sector number of the inode is suitable for use as an inode number*/
		case SYS_INUMBER:
		{
			fd = *(stack_ptr + 1);
			struct thread* current_thread = thread_current();
			struct list_elem* e = find_fd_element(fd, current_thread);
			if(e == NULL) // This should never happen
			{ 
				f->eax = -1; // Directory doesn't exist
			}
			else
			{
				struct  fd_list_element *fd_element = 
				list_entry (e, struct fd_list_element, elem_fd);
				struct dir* dir = (struct dir*) fd_element->fp;
				f->eax = dir->inode->sector;
			}
			break;
		}
		/*
		Reads a directory entry from file descriptor fd, 
		which must represent a directory. If successful, 
		stores the null-terminated file name in name, which must 
		have room for READDIR_MAX_LEN + 1 bytes, and returns true. 
		If no entries are left in the directory, returns false.
		. and .. should not be returned by readdir.

		If the directory changes while it is open, 
		then it is acceptable for some entries not to be read at all 
		or to be read multiple times. Otherwise, each directory entry 
		should be read once, in any order. 

		READDIR_MAX_LEN is defined in lib/user/syscall.h.
		 If your file system supports longer file names than the 
		 basic file system, you should increase this value from the default of 14.
		 */
		case SYS_READDIR:
		{
			fd = *(stack_ptr + 1);
		    name = *(stack_ptr + 2);
		    if (name == NULL) { //Check for a non-existant file of course
				exit(-1, f);
			}
			else {
				if(get_user(name) == -1) // check if pointer to name is actually valid
					exit(-1, f);
			}
			struct thread* current_thread = thread_current();
			struct list_elem* e = find_fd_element(fd, current_thread);
			if(e == NULL) // This should never happen
			{ 
				f->eax = -1; // Directory doesn't exist
			}
			else
			{
				struct  fd_list_element *fd_element = 
				list_entry (e, struct fd_list_element, elem_fd);
				if(fd_element->warning) 
				{
					struct dir* dir = (struct dir*) fd_element->fp;
					/* Reads the next directory entry in DIR and stores the name in
	   				NAME.  Returns true if successful, false if the directory
	   				contains no more entries. */
	                f->eax = dir_readdir (dir, name);

	                /* For Debugging
	                printf("readdir name %s %p\n", name, dir);
	                printf("current_thread: cd: %p\n", current_thread->cd.cd_dir);

	                char name2[20];
	                struct thread* current_thread2 = thread_current();
					struct list_elem* e2 = find_fd_element(2, current_thread2);
									struct  fd_list_element *fd_element2 = 
				  list_entry (e2, struct fd_list_element, elem_fd);
	              dir_readdir((struct dir*)fd_element2->fp, name2);
	              printf("readdir name2 %p %s\n",fd_element2->fp, name2);*/

	            }
	            else
	            {
	                f->eax = -1; // fd does not represent a directory
	            }
			}
		break;
		}

		default:
		{
			//#ifdef PROJECT2_DEBUG
			printf("DID NOT IMPLEMENT THIS SYSCALL ERROR, number:%d\n", system_call_number);
			//#endif
			break;
		}
		}

}


tid_t exec(const char* name)
{
	tid_t pid = process_execute(name);

	if(pid != TID_ERROR)
		return pid;
	else
		return -1;
}

/* Used by syscall_filesize*/
int filesize_get(int fd)
{
	struct thread* current_thread = thread_current();
	struct list_elem* e = find_fd_element(fd, current_thread);
	if(e == NULL) return -1; // return false if fd not found TODO?
	struct  fd_list_element *fd_element = list_entry (e, struct fd_list_element, elem_fd);
	return file_length (fd_element -> fp) ;
}

/* Terminates Pintos by calling shutdown_power_off() (declared in "threads/init.h"). This should be seldom used, because you lose some information about possible deadlock situations, etc. */

void halt (void) {
	shutdown_power_off(); //Fairly straightforward
}

/* Terminates the current user program, returning status to the kernel. If the process's parent waits for it (see below), this is the status that will be returned. Conventionally, a status of 0 indicates success and nonzero values indicate errors. */

void exit (int status, struct intr_frame *f) {

	if(f!=NULL)
		f->eax = status; //Save the status that was returned by the existing process to the stack

	struct list_elem * e = NULL;
	/*close all open files, but do not free the actual element in the list, we do that in process_exit()*/
	/*for (e = list_begin (&t->fd_table); e != list_end (&t->fd_table);
           e = list_next (e))
        {
          	struct  fd_list_element *fd_element = list_entry (e, struct fd_list_element, elem_fd);


          	file_close(fd_element->fp); // this call frees fp
          	fd_element->fp = NULL;

        }*/

	thread_exit_process(status); //A function in thread.h that terminates and removes from the list of threads the current thread t. t's status also becomes THREAD_DYING
}



 /* Waits for a child process pid and retrieves the child's exit status.

    If pid is still alive, waits until it terminates. Then, returns the status that pid passed to exit. If pid did not call exit(), but was terminated by the kernel (e.g. killed due to an exception), wait(pid) must return -1. It is perfectly legal for a parent process to wait for child processes that have already terminated by the time the parent calls wait, but the kernel must still allow the parent to retrieve its child's exit status, or learn that the child was terminated by the kernel. wait must fail and return -1 immediately if any of the following conditions is true:
        - pid does not refer to a direct child of the calling process. pid is a direct child of the calling process if and only if the calling process received pid as a return value from a successful call to exec.
        - Note that children are not inherited: if A spawns child B and B spawns child process C, then A cannot wait for C, even if B is dead. A call to wait(C) by process A must fail. Similarly, orphaned processes are not assigned to a new parent if their parent process exits before they do.
		- The process that calls wait has already called wait on pid. That is, a process may wait for any given child at most once.
		- Processes may spawn any number of children, wait for them in any order, and may even exit without having waited for some or all of their children. Your design should consider all the ways in which waits can occur. All of a process's resources, including its struct thread, must be freed whether its parent ever waits for it or not, and regardless of whether the child exits before or after its parent.

    You must ensure that Pintos does not terminate until the initial process exits. The supplied Pintos code tries to do this by calling process_wait() (in "userprog/process.c") from main() (in "threads/init.c"). We suggest that you implement process_wait() according to the comment at the top of the function and then implement the wait system call in terms of process_wait().

    Implementing this system call requires considerably more work than any of the rest. */

int wait (tid_t pid) {
	return process_wait(pid);
}

/* Creates a new file called file initially initial_size bytes in size. Returns true if successful, false otherwise. Creating a new file does not open it: opening the new file is a separate operation which would require a open system call. */

bool create (const char *file, unsigned initial_size) {
	bool return_bool = filesys_create(file, initial_size, false); //Already in filesys.c...

	return return_bool;
}

/* Deletes the file called file. Returns true if successful, false otherwise. A file may be removed regardless of whether it is open or closed, and removing an open file does not close it. See Removing an Open File, for details. */
//TODO
//

bool remove (const char *file)
{
	return filesys_remove(file);
}

/* Opens the file called file. Returns a nonnegative integer handle called a "file descriptor" (fd), or -1 if the file could not be opened.

    File descriptors numbered 0 and 1 are reserved for the console: fd 0 (STDIN_FILENO) is standard input, fd 1 (STDOUT_FILENO) is standard output. The open system call will never return either of these file descriptors, which are valid as system call arguments only as explicitly described below.

    Each process has an independent set of file descriptors. File descriptors are not inherited by child processes.

    When a single file is opened more than once, whether by a single process or different processes, each open returns a new file descriptor. Different file descriptors for a single file are closed independently in separate calls to close and they do not share a file position. */

int open (const char *file) {

	struct thread* current_thread = thread_current();
	lock_acquire(&read_write_lock);
	bool warning = false;
	struct file* fp = filesys_open(file, false, &warning); //Again, already in filesys.c
	lock_release(&read_write_lock);
	int return_fd = -1;
	/* Now update the file descriptor table */
	if (fp != NULL) {

		return_fd = add_file_to_fd_table(current_thread, fp, warning);
	}
	return return_fd; // IF The file could not be assigned a new file descriptor, then return_fd == -1
}

/* Returns the size, in bytes, of the file open as fd. */

//int filesize (int fd)

/* Reads size bytes from the file open as fd into buffer. Returns the number of bytes actually read (0 at end of file), or -1 if the file could not be read (due to a condition other than end of file). Fd 0 reads from the keyboard using input_getc(). */

int read (int fd, void *buffer, unsigned size)
{


	int return_size = -1;
	if(fd == 0)
	{
		int i = 0;
		for(i = 0; i < size; i++)
		{
			((char*) buffer) [i] = input_getc();
		}
		return_size = size;

	}

	else if (fd != 1)
	{
		struct thread* t = thread_current();
		struct list_elem* e = find_fd_element(fd, t);
		if(e == NULL) // if no file found with given fd, return error
			{
				goto read_done;
			}
		struct fd_list_element *fd_element = list_entry(e, struct fd_list_element, elem_fd);
			lock_acquire(&read_write_lock);

		return_size = file_read (fd_element->fp, buffer, size) ;
			lock_release(&read_write_lock);

	}

	read_done:



	return return_size;
}

/* Writes size bytes from buffer to the open file fd. Returns the number of bytes actually written, which may be less than size if some bytes could not be written.

    Writing past end-of-file would normally extend the file, but file growth is not implemented by the basic file system. The expected behavior is to write as many bytes as possible up to end-of-file and return the actual number written, or 0 if no bytes could be written at all.

    Fd 1 writes to the console. Your code to write to the console should write all of buffer in one call to putbuf(), at least as long as size is not bigger than a few hundred bytes. (It is reasonable to break up larger buffers.) Otherwise, lines of text output by different processes may end up interleaved on the console, confusing both human readers and our grading scripts. */

int write (int fd, const void *buffer, unsigned size, bool* warning) { //Already done in file.c, but will implement anyway (super confused now)

	struct thread* current_thread = thread_current();
	struct file* fp = NULL;
	int return_size = -1;

	if (fd == 1) //
	{
		putbuf(buffer, size);
		return_size = size;
	}

	else if (fd != 0 && fd !=1) {
		struct list_elem* e = find_fd_element(fd, current_thread);
		if(e == NULL)
			goto write_done; // return error if file not found
		struct  fd_list_element *fd_element = list_entry (e, struct fd_list_element, elem_fd);
		if(fd_element->warning == true)
		{
			*warning = true;
			goto write_done;
		}
		lock_acquire(&read_write_lock);
		return_size = file_write (fd_element->fp, buffer, size) ;
		lock_release(&read_write_lock);

	}
write_done:
	return return_size;
}

    /* Changes the next byte to be read or written in open file fd to position, expressed in bytes from the beginning of the file. (Thus, a position of 0 is the file's start.)

    A seek past the current end of a file is not an error. A later read obtains 0 bytes, indicating end of file. A later write extends the file, filling any unwritten gap with zeros. (However, in Pintos files have a fixed length until project 4 is complete, so writes past end of file will return an error.) These semantics are implemented in the file system and do not require any special effort in system call implementation. */

bool seek (int fd, unsigned position)
{
	struct thread* current_thread = thread_current();
	struct list_elem* e = find_fd_element(fd, current_thread);
	if(e == NULL) return false; //is this needed?
	struct  fd_list_element *fd_element = list_entry (e, struct fd_list_element, elem_fd);
	file_seek(fd_element->fp, position);
	return true;

}

/* Returns the position of the next byte to be read or written in open file fd, expressed in bytes from the beginning of the file. */

unsigned tell (int fd) {
	struct thread* current_thread = thread_current();
	struct list_elem* e = find_fd_element(fd, current_thread);
	//if(e == NULL) return false; is this needed?
	struct  fd_list_element *fd_element = list_entry (e, struct fd_list_element, elem_fd);
	return file_seek(fd_element->fp);

}

/* Closes file descriptor fd. Exiting or terminating a process implicitly closes all its open file descriptors, as if by calling this function for each one. */

bool close (int fd) {
	struct thread* current_thread = thread_current();
	struct list_elem* e = find_fd_element(fd, current_thread);
	if(e == NULL) return false; // return false if fd not found
	struct list_elem*  return_e = list_remove (e);

	struct  fd_list_element *fd_element = list_entry (e, struct fd_list_element, elem_fd);
	lock_acquire(&read_write_lock);
	if(fd_element->warning == false)
	file_close(fd_element->fp);
	else
	{
	dir_close((struct dir*)fd_element->fp);
	}
	lock_release(&read_write_lock);
	free(fd_element); // Free the element we just removed, please also see open()
	return true;
}



// Find the element in the linkedlist coressponding to the given fd
// Uses: Returns the list_element* type
// 			You can thus then use this returned type to remove element or
//          Edit the contents of the actual element
// You can view an example of these two uses in void close function above
struct list_elem* find_fd_element(int fd, struct thread* current_thread)
{
	    struct list_elem *e;
      	// search through the fd_table for the matching fd
		for (e = list_begin (&current_thread->fd_table); e != list_end (&current_thread->fd_table);
           e = list_next (e))
        {
          struct  fd_list_element *fd_element = list_entry (e, struct fd_list_element, elem_fd);
          if(fd_element->fd == fd)
          {
          	return e;
          }
        }

        return NULL;
}

// Find the element in the linkedlist coressponding to the given tid_t pid
// Uses: Returns the list_element* type
// 			You can thus then use this returned type to remove element or
//          Edit the contents of the actual element
struct list_elem* find_child_element(struct thread* current_thread, tid_t pid)
{
	    struct list_elem *e;
      	// search through the fd_table for the matching fd
		for (e = list_begin (&current_thread->child_list); e != list_end (&current_thread->child_list);
           e = list_next (e))
        {
          struct  child_list_elem *child_element = list_entry (e, struct child_list_elem, elem_child);
          if(child_element->pid == pid)
          {
          	return e;
          }
        }

        return NULL;
}



// TODO: Should check if file is already opened/added
int add_file_to_fd_table(struct thread* current_thread, struct file* fp, bool warning)
{
		int return_fd = -1;
		struct fd_list_element* fd_element = malloc(sizeof(struct fd_list_element));
		fd_element->fd = current_thread->fd_table_counter;
		fd_element->fp = fp;
		fd_element->warning =warning;
		return_fd = fd_element->fd;

		list_push_back(&current_thread->fd_table, &fd_element->elem_fd);
		lock_acquire(&fd_lock);
		current_thread->fd_table_counter++; // increment counter, so we have a new fd to use for the next file
		lock_release(&fd_lock);
		return return_fd;
}
// TODO: Should check if child is already added
struct child_list_elem* add_child_to_list(struct thread* parent_thread, tid_t pid)
{
		/* create new child element to push to the parent's child list*/
		struct child_list_elem* child_element = malloc(sizeof(struct child_list_elem));
		child_element->pid = pid;
    child_element->load_status = NULL;
		child_element->parent_pid = parent_thread->tid;
		child_element->status = PROCESS_RUNNING;
		child_element->mom_im_out_of_money = false;
		child_element->sema = malloc(sizeof(struct semaphore)); // set only in process_wait by the parent, used for waiting
		sema_init(child_element->sema, 0);
		// TODO: This may cause concurrency issues by doing it this way, but we will see....
		struct thread* child_thread = find_thread(pid);
		child_thread->child_data = child_element;
		/* also give the child thread struct itself a ptr to the child_element
		   so that the child can update its status/ and exit status and the parent will see */
		child_element->inception = &child_thread->child_data;
		list_push_back(&parent_thread->child_list, &child_element->elem_child);
		return child_element;
}



/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int
get_user (const uint8_t *uaddr)
{
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool
put_user (uint8_t *udst, uint8_t byte)
{
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}
