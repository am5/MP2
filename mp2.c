///////////////////////////////////////////////////////////////////////////////
//
// MP2:		Rate Monotonic CPU Scheduler
// Name:        mp2.c
// Date: 	10/1/2011
// Group:	20: Intisar Malhi, Alexandra Mirtcheva, and Roberto Moreno
// Description: This source implements a CPU scheduler for the Liu and Layland
//		Periodic Task Model, based the on the Rate-Monotonic Scheduler.
//              Implemented using a Linux Kernel Module and the Proc Filesystem.
//		Compiled for Fedora Core 15 64-bits, Linux Kernel 2.60.40. 
//
///////////////////////////////////////////////////////////////////////////////

#include "mp2.h"

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  timer_init
//
// PROCESSING:
//
//    This is a helper function to initialize a timer with a single call. 
//
// INPUTS:
//
//    timer 	- the timer list
//    function  - function pointer 
//
// RETURN:
//
//   None
//
// IMPLEMENTATION NOTES
//
//   None 
//
///////////////////////////////////////////////////////////////////////////////
inline void timer_init(struct timer_list  *timer, void (*function)(unsigned long), struct mp2_task_struct * data)
{
  BUG_ON(timer==NULL || function==NULL);
  init_timer(timer);
  timer->function=function;
//  timer->data=(struct mp2_task_struct*) data;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  set_timer
//
// PROCESSING:
//
//    This is a helper function to set a timer with a single call, and 
//    specifies the relative time in milliseconds. 
//    (Kernel timers are absolute and expressed in jiffies)
//
// INPUTS:
//
//    tlist 	    - the timer list
//    release_time  - the time when the current task has to be released. 
//
// RETURN:
//
//   None
//
// IMPLEMENTATION NOTES
//
//   None 
//
///////////////////////////////////////////////////////////////////////////////
inline void set_timer(struct timer_list* tlist, long release_time)
{
  BUG_ON(tlist==NULL);
  tlist->expires=jiffies+MS_TO_JIFF(release_time);
  mod_timer(tlist, tlist->expires);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  up_handler
//
// PROCESSING:
//
//    This function implements the timer handler; it signals the dispatcher 
//    thread that an update much occur. 
//    (This must be very fast so we have to use a two halves approach)
//
// INPUTS:
//
//    ptr - points to the PID of the thread to be ran  
//
// RETURN:
//
//   None
//
// IMPLEMENTATION NOTES
//
//   None 
//
///////////////////////////////////////////////////////////////////////////////
void up_handler(unsigned long ptr)
{
  // change the state of the current task to ready since our timer expired
  struct mp2_task_struct *mytask;
  mytask=(struct mp2_task_struct *) ptr;
  if(mytask != NULL){
	printk(KERN_INFO "Setting mytask to state ready\n");
  	mytask->task_state = TASK_STATE_READY;
        set_task_state(mytask->linux_task, TASK_INTERRUPTIBLE);
	printk(KERN_INFO "Task state is %d\n", mytask->task_state);
  }

  printk(KERN_INFO "Calling our dispatch threadPID %ld\n", mytask->pid);
  //SCHEDULE THE THREAD TO RUN (WAKE UP THE THREAD)
  wake_up_process(dispatch_kthread);
}

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
void _insert_task(struct mp2_task_struct* t)
{
  BUG_ON(t==NULL);
  list_add_tail(&t->task_node, &mp2_task_list);
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
struct mp2_task_struct* _lookup_task(long pid)
{
  struct list_head *pos;
  struct mp2_task_struct *p;
  
  list_for_each(pos, &mp2_task_list)
  {
    p = list_entry(pos, struct mp2_task_struct, task_node);
    if(p->pid == pid)
      return p;
  }
  
  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  should_admit
//
// PROCESSING:
//
//    This function implements the admission control 
//
// INPUTS:
//
//    period -		the time from when a job begins executing until the time the
//			next job starts running 
//    processing time - the total time it takes for a single job to run 
//
// RETURN:
//
//   bool - TRUE if the function should be admitted (the current task can be 
//          scheduled without missing any deadline according to the utilization 
//          bound-based method). 
//          FALSE if the function should not be admitted (the current task cannot
//          be scheduled without missing any deadline according to the utilization 
//          bound-based method)
//
// IMPLEMENTATION NOTES
//
//   The admission criteria is based on the utilization bound-based method: 
//   the method establishes that a task is schedulable if the sum of the processing 
//   time of all tasks in system divided by the period of all tasks in the system
//   is less than 0.693. 
//
///////////////////////////////////////////////////////////////////////////////
bool should_admit(long period, long processingTime)
{
  long admissionThreshold = 693; //normalized by multiplying by 1000
  struct list_head *pos;
  struct mp2_task_struct *p;
  long summation = 0;

  summation = summation + PROCESSING_TIME_RATIO(processingTime, period);

  list_for_each(pos, &mp2_task_list)
  {
    p = list_entry(pos, struct mp2_task_struct, task_node);
    summation = summation + PROCESSING_TIME_RATIO(p->ptime, p->period);    
  }

  if(summation <= admissionThreshold)
    return true;
  else
    return false;
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
  struct mp2_task_struct *p;
  
  //only add if PID doesn't already exist
  if(_lookup_task(pid) != NULL) return -1;
  
  //admission control
  if(!should_admit(period, processingTime)) return -1; 

  p = kmalloc(sizeof(struct mp2_task_struct), GFP_KERNEL);

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
  p->period = period;
  p->ptime = processingTime;
  p->task_state = TASK_STATE_SLEEPING;
  p->first_yield_call = 0;
  //timer_init(&(p->wakeup_timer), up_handler, p);
  init_timer(&(p->wakeup_timer));
  (p->wakeup_timer).function=up_handler;
  (p->wakeup_timer).data=(unsigned long) p;
  

  // Insert the task into the task list 
  mutex_lock(&mp2_mutex);
  _insert_task(p);
  mutex_unlock(&mp2_mutex);
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
  struct list_head *pos, *tmp;
  struct mp2_task_struct *p;
  int found = -1;  // init to not found

  // loop through the list until we find our PID, then remove it
  list_for_each_safe(pos, tmp, &mp2_task_list)
  {
    p = list_entry(pos, struct mp2_task_struct, task_node);
    // is this is our task?
    if(p->pid == pid){
      printk(KERN_INFO "Found node with PID %ld\n", p->pid);
      // yes, we need to remove this entry
      mutex_lock(&mp2_mutex);
      // remove the timer
      del_timer_sync(&(p->wakeup_timer));
      list_del(pos);
      kfree(p);
      mutex_unlock(&mp2_mutex);
      printk(KERN_INFO "Removing PID %ld\n", pid);
      found=0;
    } // no, keep searching
  }
  // return the result status
  return found;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  yield_task
//
// PROCESSING:
//
//    This function yields the CPU to the next task in the READY queue
//    with the highest priority 
//
// INPUTS:
//
//    pid - the process ID of the calling task
//
// RETURN:
//
//   int - (-1) if there is no task associated with the given PID
//	        (0) if the task is registered successfully. 
//
// IMPLEMENTATION NOTES
//
//   The unregister_task function uses the Linux kernel linked-list
//   API to search for a given task. If the task is found in the list, the 
//   memory is freed and the node is removed from the task list. 
//
///////////////////////////////////////////////////////////////////////////////
int yield_task(long pid)
{
  struct list_head *pos, *tmp;
  struct mp2_task_struct *p = NULL;

  // loop through the list until we find our PID
  list_for_each_safe(pos, tmp, &mp2_task_list)
  {
    p = list_entry(pos, struct mp2_task_struct, task_node);
    // is this is our task?
    if(p->pid == pid)
    {
      printk(KERN_INFO "yield_task: Found node with PID %ld\n", p->pid);
      break;
    } 
  }
 
  if(p != NULL && p->pid == pid)
  {
    // indicate that it's the first time we're calling yield
    if(p->first_yield_call == 0)
    {
      printk(KERN_INFO "This is the first time we're yielding, pid=%ld\n", p->pid);
      p->first_yield_call = 1;
      p->previous_time = jiffies;
    }

    //if next period has not started yet
    //  set state to sleeping and wake up timer
    if(jiffies < MS_TO_JIFF(p->period) + p->previous_time)
    {
      printk(KERN_INFO "Our period has not started yet, pid=%ld\n", p->pid);
      //adjust new previous
      p->previous_time = p->previous_time + MS_TO_JIFF(p->period);

      // change task state to sleeping
      p->task_state = TASK_STATE_SLEEPING;
      set_task_state(p->linux_task, TASK_UNINTERRUPTIBLE);
    
      printk(KERN_INFO "Setting the wakeup timer to %ld\n", p->period);
      // setup the wakeup_timer
      set_timer(&(p->wakeup_timer), p->period);
    }
  }

  // pre-empt the CPU to the next READY application 
  // with the highest priority
  wake_up_process(dispatch_kthread);
 
  return 0;
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
  struct list_head *pos;
  struct mp2_task_struct *p;

  printk(KERN_INFO "Reading from proc file\n");
  // should return the number of bytes printed

  mutex_lock(&mp2_mutex);
  // loop through the task list and print the PID, period and processing time (space delimited)
  list_for_each(pos, &mp2_task_list)
  {
    p = list_entry(pos, struct mp2_task_struct, task_node);
    i += sprintf(page+off+i, "%ld %ld %ld\n", p->pid, p->period, p->ptime);
  }
  mutex_unlock(&mp2_mutex);
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
  printk(KERN_INFO "From /proc/mp2/status: %s, %ld, %ld, %ld\n", action, pid, period, processingTime); 

  if(strcmp(action, "R")==0){
    printk(KERN_INFO "Going to register PID %ld\n", pid);
    // perform registration
    register_task(pid, period, processingTime);
  }
  if(strcmp(action, "D")==0){
    printk(KERN_INFO "Going to un-register PID %ld\n", pid);
    // perform de-registration
    unregister_task(pid);
  }
  if(strcmp(action, "Y")==0){
    printk(KERN_INFO "Going to yield PID %ld\n", pid);
    // perform yield
    yield_task(pid);
  }
  // free the memory
  kfree(proc_buffer);
  kfree(action);

  return count;
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
  struct mp2_task_struct *p;

  list_for_each_safe(pos, tmp, &mp2_task_list)
    {
      p = list_entry(pos, struct mp2_task_struct, task_node);
      //destroy timer
      del_timer_sync(&(p->wakeup_timer));
      //remove from list
      list_del(pos);
      printk(KERN_INFO "Destroying task associated with PID %ld\n", p->pid);
      kfree(p);
    }
}
int perform_scheduling(void *data){
  
  struct mp2_task_struct *highest_priority = NULL;
  struct sched_param highest_prio_sparam;
  struct list_head *pos;
  struct mp2_task_struct *p;

  while(1){

    mutex_lock(&mp2_mutex);
    if(stop_dispatch_thread==1)
    {
      mutex_unlock(&mp2_mutex);
      break;
    }
    highest_priority=NULL;

    //find highest priority
    list_for_each(pos, &mp2_task_list)
    {
      p = list_entry(pos, struct mp2_task_struct, task_node);
      if(p->task_state == TASK_STATE_READY)
      {
         // initialize the first task as high priority
         if(highest_priority == NULL)
           highest_priority = p;
         printk("Current PID=%ld, period=%ld\n", p->pid, p->period);
         if(p->period < highest_priority->period){
           printk("PID %ld period (%ld) is less than period of highest priority (PID=%ld, period=%ld)\n", p->pid, p->period, highest_priority->pid, highest_priority->period);
           highest_priority = p;
         }
      }
    }
    mutex_unlock(&mp2_mutex);

    if(highest_priority != NULL){
      if(current_task != NULL){
        if(current_task->pid != highest_priority->pid){
          printk(KERN_INFO "New high priority process PID=%ld, context switch\n", highest_priority->pid);
          // set higher priority process
          highest_priority->task_state = TASK_STATE_RUNNING;
          wake_up_process(highest_priority->linux_task);
          highest_prio_sparam.sched_priority = MAX_USER_RT_PRIO-1;
          sched_setscheduler(highest_priority->linux_task, SCHED_FIFO, &highest_prio_sparam);

          // set lower priority process
          //set to READY only if it was running
          if(current_task->task_state == TASK_STATE_RUNNING)
            current_task->task_state = TASK_STATE_READY;

          struct sched_param sparam;
          sparam.sched_priority = 0;
          sched_setscheduler(current_task->linux_task, SCHED_NORMAL, &sparam);
        }else{
          printk(KERN_INFO "current_task (PID=%ld, state=%d) is EQUAL to highest_priority (PID=%ld, state=%d)\n", current_task->pid, current_task->task_state, highest_priority->pid, highest_priority->task_state);
        }
      }else{
        printk(KERN_INFO "Current task is NULL, context switch\n");
        current_task = highest_priority;
        current_task->task_state = TASK_STATE_RUNNING;
        wake_up_process(current_task->linux_task);
        highest_prio_sparam.sched_priority = MAX_USER_RT_PRIO-1;
        sched_setscheduler(current_task->linux_task, SCHED_FIFO, &highest_prio_sparam);
      }
    }else{
      printk(KERN_INFO "Highest Priority is NULL\n");
    }
    //put scheduler to sleep until woken up again
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
  }
  return 0;

}
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME: perform_scheduling
//
// PROCESSING:
//
//    This function starts the dispatcher thread and performs any scheduling
//    updates 
//
// INPUTS:
//
//    data - pointer to private data 
//
// RETURN:
//
//   int -  
//
// IMPLEMENTATION NOTES
//
//   
//
///////////////////////////////////////////////////////////////////////////////
int perform_scheduling2(void *data)
{
  struct mp2_task_struct *highest_priority = NULL;
  while(1)
  {
    printk("Running scheduler function\n");
    mutex_lock(&mp2_mutex);
    if(stop_dispatch_thread==1)
    {
      mutex_unlock(&mp2_mutex);
      break;
    }
    
    highest_priority = NULL;
    struct list_head *pos;
    struct mp2_task_struct *p;
    struct sched_param highest_prio_sparam;

    //find highest priority
    list_for_each(pos, &mp2_task_list)
    {
      p = list_entry(pos, struct mp2_task_struct, task_node);
      if(p->task_state == TASK_STATE_READY)
      {
         // initialize the first task as high priority
         if(highest_priority == NULL)
	   highest_priority = p;
         printk("Current PID=%ld, period=%ld\n", p->pid, p->period);
         if(p->period < highest_priority->period){
           printk("PID %ld period (%ld) is less than period of highest priority (PID=%ld, period=%ld)\n", p->pid, p->period, highest_priority->pid, highest_priority->period);
           highest_priority = p;
         }
      }
    }
    mutex_unlock(&mp2_mutex);

    //context switch
    if(highest_priority != NULL)
    {
      printk(KERN_INFO "Context switch!\n");
      highest_priority->task_state = TASK_STATE_RUNNING;
      
      wake_up_process(highest_priority->linux_task);
      highest_prio_sparam.sched_priority = MAX_USER_RT_PRIO-2;
      sched_setscheduler(highest_priority->linux_task, SCHED_FIFO, &highest_prio_sparam);
    }
  
    if(current_task != NULL && highest_priority != NULL && current_task->pid != highest_priority->pid)
    {  
      struct sched_param sparam;
      sparam.sched_priority = 0;
      sched_setscheduler(current_task->linux_task, SCHED_NORMAL, &sparam);
         
      //set to READY only if it was running
      if(current_task->task_state == TASK_STATE_RUNNING)
        current_task->task_state = TASK_STATE_READY;
    }

    //set new running task(if any) to current now
    current_task = highest_priority;
    printk("Putting scheduler function to sleep...\n");
    //put scheduler to sleep until woken up again  
    set_current_state(TASK_INTERRUPTIBLE);
    schedule();
    //set_current_state(TASK_STATE_RUNNING);
  }

  return 0;
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
//   The my_module_init function calls the timer_init function with the 
//   timer_list structure and a function pointer to the up_handler function.
//   It initializes the proc_file entry variables and creates the dispatcher
//   thread.
//   
///////////////////////////////////////////////////////////////////////////////
int __init my_module_init(void)
{
  struct sched_param sparam;

  mp2_proc_dir=proc_mkdir("mp2",NULL);
  register_task_file=create_proc_entry("status", 0666, mp2_proc_dir);
  register_task_file->read_proc= proc_registration_read;
  register_task_file->write_proc=proc_registration_write;

  dispatch_kthread = kthread_create(perform_scheduling, NULL, "kmp2");  

  //set scheduling thread to higher priority than task so that this cannot be preempted.
  sparam.sched_priority = MAX_RT_PRIO;
  sched_setscheduler(dispatch_kthread, SCHED_FIFO, &sparam);

  //THE EQUIVALENT TO PRINTF IN KERNEL SPACE
  printk(KERN_INFO "MP2 Module LOADED\n");
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
  remove_proc_entry("status", mp2_proc_dir);
  remove_proc_entry("mp2", NULL);
  
  stop_dispatch_thread=1;
  wake_up_process(dispatch_kthread);
  
  _destroy_task_list();
  printk(KERN_INFO "MP2 Module UNLOADED\n");
}

// WE REGISTER OUR INIT AND EXIT FUNCTIONS HERE SO INSMOD CAN RUN THEM
// MODULE_INIT AND MODULE_EXIT ARE MACROS DEFINED IN MODULE.H
module_init(my_module_init);
module_exit(my_module_exit);

// THIS IS REQUIRED BY THE KERNEL
MODULE_LICENSE("GPL");
