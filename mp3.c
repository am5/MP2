///////////////////////////////////////////////////////////////////////////////
//
// MP3:		Virtual Memory Page Fault Measurement 
// Name:        mp3.c
// Date: 	11/5/2011
// Group:	20: Intisar Malhi, Alexandra Mirtcheva, and Roberto Moreno
// Description: This source implements a profiler tool kernel module for 
//				virtual memory
//				page fault measurement. 
//              Implemented using a Linux Kernel Module and the Proc Filesystem.
//		Compiled for Fedora Core 15 64-bits, Linux Kernel 2.60.40. 
//
///////////////////////////////////////////////////////////////////////////////

#include "mp3.h"

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  _insert_task
//
// PROCESSING:
//
//    This funtion inserts the current task in the task list. 
//
// INPUTS:
//
//    t - the task structure to be inserted into the task list  
//
// RETURN:
//
//   None
//
// IMPLEMENTATION NOTES
//
//   This function is called by the register_task function. 
//
///////////////////////////////////////////////////////////////////////////////
void _insert_task(struct mp3_task_struct* t)
{
  BUG_ON(t==NULL);
  list_add_tail(&t->task_node, &mp3_task_list);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  _lookup_task
//
// PROCESSING:
//
//    This funtion this function looks up the task specified by the given PID
//    and returns the task structure to the calling function 
//
// INPUTS:
//
//    pid - the PID of the task structure that is being searched for in the 
//          list of tasks.  
//
// RETURN:
//
//   mp2_task_struct - the task structure that corresponds to the given PID. 
//
// IMPLEMENTATION NOTES
//
//   None.  
//
///////////////////////////////////////////////////////////////////////////////
struct mp3_task_struct* _lookup_task(long pid)
{
  struct list_head *pos;
  struct mp3_task_struct *p;
  
  list_for_each(pos, &mp3_task_list)
  {
    p = list_entry(pos, struct mp3_task_struct, task_node);
    if(p->pid == pid)
      return p;
  }
  
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  work_handler
//
// PROCESSING:
//
//    This function gets the sample information for each registered task.
//
// INPUTS:
//
//    arg - pointer to data of the work queue 
//
// RETURN:
//
//    Nothing.
//
// IMPLEMENTATION NOTES
//
//   None.  
//
///////////////////////////////////////////////////////////////////////////////
void work_handler (void *arg){
  // run as long as flag is true
  if(queue_stop){
    return;
  }
  unsigned long maj, min, cpu;
  struct list_head *pos, *tmp;
  struct mp3_task_struct *p;
  int p_index=0;	// keeps track of the current task
  // for every item on our list, get the stats
  list_for_each_safe(pos, tmp, &mp3_task_list)
  {
    p = list_entry(pos, struct mp3_task_struct, task_node);
    // read the stats and store them on the buffer
    if(get_cpu_use(p->pid, &min, &maj, &cpu)){
      // an error occur
      printk(KERN_INFO "Unable to get stats for pid=%ld\n", p->pid);
    }else{
      // store the sum of information for each PID
      p->min += min;
      p->maj += maj;
      p->cpu = (p->linux_task->stime + p->linux_task->utime)/jiffies;

      // store the information on the memory buffer
      *(p_addr + (p_index * PAGE_SIZE) + 0) = jiffies;
      *(p_addr + (p_index * PAGE_SIZE) + 1) = p->min;
      *(p_addr + (p_index * PAGE_SIZE) + 2) = p->maj;
      *(p_addr + (p_index * PAGE_SIZE) + 3) = p->cpu;
      p_index++;
    }
  }
  // schedule the work queue again
  schedule_delayed_work(wqueue, HZ/20);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  create_mp3queue
//
// PROCESSING:
//
//    This function initializes the work_queue
//
// INPUTS:
//
//    arg - pointer to data of the work queue 
//
// RETURN:
//
//    Nothing.
//
// IMPLEMENTATION NOTES
//
//   None.  
//
///////////////////////////////////////////////////////////////////////////////
void create_mp3queue (void) {
  // initialize the work queue
  // create the work queue
  wqueue = kmalloc(sizeof(struct delayed_work), GFP_KERNEL);
  if(wqueue){
    INIT_DELAYED_WORK(wqueue, work_handler);
    printk("Starting the work handler\n");
    // schedule for every 50 milliseconds (20 times per second)
    schedule_delayed_work(wqueue, HZ/20);
  }else{
    // unable to allocate memory for work queue
    printk("create_mp3queue: Unable to allocate memory for work queue object");
  }
}
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  register_task
//
// PROCESSING:
//
//    This funtion implements the task registration
//
// INPUTS:
//
//    pid - 		the process ID of the calling task
//    period -		the time from when a job of the calling task begins 
//			executing until the time the next job of the calling task
//			starts running 
//    processing time - the total time it takes for a single job to run of the 
//			calling task to run
//
// RETURN:
//
//   int - (-1) if there is no task associated with the given PID
//	    (0) if the task is registered successfully. 
//
// IMPLEMENTATION NOTES
//
//   The register_task function calls admission control before registering
//   the specified task in order to verify that it's schedulable. If the task
//   passes admission control, then the register_task function allocates enough
//   memory for it, initializes the task structure variables, sets the task 
//   state to TASK_INTERRUPTIBLE (SLEEPING), initializes the timer, and 
//   inserts the task into the task list. 
//
///////////////////////////////////////////////////////////////////////////////
int register_task(long pid, long period, long processingTime)
{
  struct mp3_task_struct *p, *first;
  struct list_head *pos, *tmp;
  
  //only add if PID doesn't already exist
  if(_lookup_task(pid) != NULL) return -1;
  
  p = kmalloc(sizeof(struct mp3_task_struct), GFP_KERNEL);

  // get the task by given PID
  p->linux_task = find_task_by_pid(pid);
  if(p->linux_task == NULL){
    // no task was found associated with given PID
    printk(KERN_INFO "No task associated with PID %ld\n", pid);
    // free the memory
    kfree(p);
    // return error
    return -1;
  }

  // Update the task structure
  p->pid = pid;

  // Insert the task into the task list 
  mutex_lock(&mp3_mutex);
  _insert_task(p);
  mutex_unlock(&mp3_mutex);

  /***NEED MUTEX HERE? *****/
  // create the work queue job if task list is empty
  list_for_each_safe(pos, tmp, &mp3_task_list)
  {
    first = list_entry(pos, struct mp3_task_struct, task_node);
    if(first==NULL)
      create_mp3queue();
    break;
  }

  printk(KERN_INFO "Task added to list\n");
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  unregister_task
//
// PROCESSING:
//
//    This funtion implements the task de-registration
//
// INPUTS:
//
//    pid - the process ID of the calling task
//
// RETURN:
//
//   int - (-1) if there is no task associated with the given PID
//	   (0) if the task is registered successfully. 
//
// IMPLEMENTATION NOTES
//
//   The unregister_task function uses the Linux kernel linked-list
//   API to search for a given task. If the task is found in the list, the 
//   memory is freed and the node is removed from the task list. 
//
///////////////////////////////////////////////////////////////////////////////
int unregister_task(long pid)
{
  struct list_head *pos,*pos2, *tmp, *tmp2;
  struct mp3_task_struct *p, *first;
  int found = -1;  // init to not found


  // loop through the list until we find our PID, then remove it
  list_for_each_safe(pos, tmp, &mp3_task_list)
  {
    p = list_entry(pos, struct mp3_task_struct, task_node);
    // is this is our task?
    if(p->pid == pid){
      printk(KERN_INFO "Found node with PID %ld\n", p->pid);
      // yes, we need to remove this entry
      mutex_lock(&mp3_mutex);
      list_del(pos);
      kfree(p);
      mutex_unlock(&mp3_mutex);
      printk(KERN_INFO "Removing PID %ld\n", pid);
      found=0;
    } // no, keep searching
  }


  /***NEED MUTEX HERE? *****/
  // delete the work queue job if task list is empty
  list_for_each_safe(pos2, tmp2, &mp3_task_list)
  {
    first = list_entry(pos, struct mp3_task_struct, task_node);
    if(first==NULL)
      kfree(wqueue);
    break;
  }

  // return the result status
  return found;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  proc_registration_read
//
// PROCESSING:
//
//    This funtion displays the PID, period, and processing time when the 
//    /proc/mp2/status file is read. 
//
// INPUTS:
//
//    page 	- the location into which the user data is being written 
//    start 	- the argument that specifies when the data begins (when
//		  returning more than a page of data)
//    off 	- the argument that specifies where data end (when returning
//                more than a page of data)
//    count 	- the maximum number of characters that can be written 
//    eof 	- the end-of-file argument that is set when all the data
//		  has been written 
//    data 	- the private data to be written to the file 
//
// RETURN:
//
//   int - greater than (0) if there is data to be read in the status file 
//
// IMPLEMENTATION NOTES
//
//   None.
//
///////////////////////////////////////////////////////////////////////////////
int proc_registration_read(char *page, char **start, off_t off, int count, int* eof, void* data)
{
  off_t i=0;
  struct list_head *pos, *tmp;
  struct mp3_task_struct *p;

  printk(KERN_INFO "Reading from proc file\n");
  // should return the number of bytes printed

  mutex_lock(&mp3_mutex);
  // loop through the task list and print the PID, period and processing time (space delimited)
  list_for_each_safe(pos, tmp, &mp3_task_list)
  {
    p = list_entry(pos, struct mp3_task_struct, task_node);
    i += sprintf(page+off+i, "%ld\n", p->pid);
  }
  mutex_unlock(&mp3_mutex);
  *eof=1;
  return i;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  proc_registration_write
//
// PROCESSING:
//
//    This function writes to the /proc/mp2/status file 
//
// INPUTS:
//
//    file 	- the open file structure
//    buffer 	- the string of data being passed from user to kernel space
//    count 	- the amount of characters that are being written 
//    data 	- pointer to private data 
//
// RETURN:
//
//   int - the number of characters that were written. 
//
// IMPLEMENTATION NOTES
//
//   The proc_registration_write function processes the message type based on 
//   the first character. If:
//   "R", the function calls the register_task function with the given PID,
//        period and processing time. 
//   "Y", the function calls the yield_task function with the given PID
//   "D", the function calls the unregister_task function with the given PID 
//
///////////////////////////////////////////////////////////////////////////////
int proc_registration_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
  char *proc_buffer;
  char *action;
  long pid, processingTime;
  int status;
  long period;

  printk(KERN_INFO "Writing to proc file\n");

  proc_buffer=kmalloc(count, GFP_KERNEL);
  action=kmalloc(2, GFP_KERNEL);
  status=copy_from_user(proc_buffer, buffer, count);
  sscanf(proc_buffer, "%s %ld %ld %ld", action, &pid, &period, &processingTime);
  printk(KERN_INFO "From /proc/mp3/status: %s, %ld, %ld, %ld\n", action, pid, period, processingTime); 

  if(strcmp(action, "R")==0){
    printk(KERN_INFO "Going to register PID %ld\n", pid);
    // perform registration
    register_task(pid, period, processingTime);
  }
  if(strcmp(action, "U")==0){
    printk(KERN_INFO "Going to un-register PID %ld\n", pid);
    // perform de-registration
    unregister_task(pid);
  }
  // free the memory
  kfree(proc_buffer);
  kfree(action);

  return count;
}
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: open_dev
//
// PROCESSING:
//
//	  Callback handler for the device open function. 
//
// INPUTS:
//
//    inode  - The inode of the character device.
//	  filep  - The pointer to the structure of the device file
//
// RETURN:
//
//   0
//
// IMPLEMENTATION NOTES
//
//   Function is only defined and does not do any processing. 
//
///////////////////////////////////////////////////////////////////////////////
int open_dev(struct inode *inode, struct file *filep)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: close_dev
//
// PROCESSING:
//
//	  Callback handler for the device close function. 
//
// INPUTS:
//
//    inode  - The inode of the character device.
//	  filep  - The pointer to the structure of the device file
//    
//
// RETURN:
//
//   0 
//
// IMPLEMENTATION NOTES
//
//   Function is only defined and does not do any processing. 
//
///////////////////////////////////////////////////////////////////////////////
int close_dev(struct inode *inode, struct file *filep)
{
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: mp3_mmap
//
// PROCESSING:
//
//
// INPUTS:
//
//    None.
//
// RETURN:
//
//   None. 
//
// IMPLEMENTATION NOTES
//
//
///////////////////////////////////////////////////////////////////////////////
int mp3_mmap(struct file *filp, struct vm_area_struct *vma)
{
//  struct vm_area_struct *vma;
  unsigned long pfn=0;
  int i;

  for(i=0; i < mem_size; i+= PAGE_SIZE)
  {
    pfn = vmalloc_to_pfn(p_addr+i);
    remap_pfn_range(vma, vma->vm_start, pfn, PAGE_SIZE, PAGE_SHARED);
    vma->vm_start += PAGE_SIZE;
  }
 
  return p_addr;
  
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: _destroy_task_list
//
// PROCESSING:
//
//    This function de-alloactes the memory held by the task list 
//
// INPUTS:
//
//    None.
//
// RETURN:
//
//   None. 
//
// IMPLEMENTATION NOTES
//
//   The _destroy_task_list function uses the Linux kernel linked list API in order
//   to iterate through the list and deallocate memory for each node in the list. 
//
///////////////////////////////////////////////////////////////////////////////
void _destroy_task_list(void)
{
  struct list_head *pos, *tmp;
  struct mp3_task_struct *p;

  list_for_each_safe(pos, tmp, &mp3_task_list)
    {
      p = list_entry(pos, struct mp3_task_struct, task_node);
      //remove from list
      list_del(pos);
      printk(KERN_INFO "Destroying task associated with PID %ld\n", p->pid);
      kfree(p);
    }
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: my_module_init
//
// PROCESSING:
//
//    This function gets executed when the module gets loaded
//
// INPUTS:
//
//    None. 
//
// RETURN:
//
//   int - returns 0 
//
// IMPLEMENTATION NOTES
//
//   It initializes the proc_file entry variables and creates the dispatcher
//   thread.
//	 The profiler memory buffer is also allocated here to store work process
//	 information. 
//   
///////////////////////////////////////////////////////////////////////////////
int __init my_module_init(void)
{
  mp3_proc_dir=proc_mkdir("mp3",NULL);
  register_task_file=create_proc_entry("status", 0666, mp3_proc_dir);
  register_task_file->read_proc= proc_registration_read;
  register_task_file->write_proc=proc_registration_write;

  // Allocate memory buffer
  p_addr = vmalloc(mem_size*PAGE_SIZE);
  if(!p_addr){
    printk("Unable to allocate the memory (size=%ld)\n", mem_size * PAGE_SIZE);
    return -1;
  }
  printk("Allocated memory (size=%ld)\n", mem_size * PAGE_SIZE);
  int i;
  // set the PG_reserved bit
  for(i=0; i < mem_size; i+= PAGE_SIZE)
  {
    SetPageReserved(vmalloc_to_page(p_addr+i));
  }
  memset(p_addr, 0, mem_size * PAGE_SIZE);


  // register the character device 
  if(!register_chrdev(693, "mp3_char_device", &mp3_fops))
	printk(KERN_INFO "mp3_char_device registered \n");
  else
	printk(KERN_INFO "Could not register mp3_char_device \n");
 

  //THE EQUIVALENT TO PRINTF IN KERNEL SPACE
  printk(KERN_INFO "MP3 Module LOADED\n");
  return 0;   
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: my_module_exit
//
// PROCESSING:
//
//    This function gets executed when the module gets unloaded
//
// INPUTS:
//
//    None. 
//
// RETURN:
//
//   None.
//
// IMPLEMENTATION NOTES
//
//   The my_module_exit function removes the proc filesystem entries and 
//   deallocates memory. 
//   
///////////////////////////////////////////////////////////////////////////////
void __exit my_module_exit(void)
{
  remove_proc_entry("status", mp3_proc_dir);
  remove_proc_entry("mp3", NULL);
  
  // need to stop the workqueue and free memory
  queue_stop=1;
  kfree(wqueue);

  // deregister the character device 
  unregister_chrdev(693, "mp3_char_device");
  
  _destroy_task_list();
  
  int i;
  // set the PG_reserved bit
  for(i=0; i < (mem_size); i+= PAGE_SIZE)
  {
    ClearPageReserved(vmalloc_to_page(p_addr+i));
  }
  vfree(p_addr);   // deallocate profile buffer 
  printk(KERN_INFO "MP3 Module UNLOADED\n");
}

// WE REGISTER OUR INIT AND EXIT FUNCTIONS HERE SO INSMOD CAN RUN THEM
// MODULE_INIT AND MODULE_EXIT ARE MACROS DEFINED IN MODULE.H
module_init(my_module_init);
module_exit(my_module_exit);

// THIS IS REQUIRED BY THE KERNEL
MODULE_LICENSE("GPL");
