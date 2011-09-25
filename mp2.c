#include "mp2.h"

//Inserts task in list 
//called by REGISTER
void _insert_task(struct mp2_task_struct* t)
{
  BUG_ON(t==NULL);
  list_add_tail(&t->task_node, &mp2_task_list);
}

//validates the PID
//returns node if PID exist in list
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

//admission control

bool shouldAdmit(long period, long processingTime)
{
  long admissionThreshold = 0.693;
  struct list_head *pos;
  struct mp2_task_struct *p;
  long summation = 0;

  summation = summation + (processingTime/period);

  list_for_each(pos, &mp2_task_list)
  {
    p = list_entry(pos, struct mp2_task_struct, task_node);
    summation = summation + (p->ptime / p->period);    
  }

  if(summation <= admissionThreshold)
    return true;
  else
    return false;
}


// register this PID from the task list.
// Return 0 if the task is registered, -1 if it was not
int register_task(long pid, long period, long comp)
{
  struct mp2_task_struct *p;
  long processingTime = 0;
  
  //only add if PID doesn't already exist
  if(_lookup_task(pid) != NULL) return -1;
  
  
  //admission control
  if(!shouldAdmit(period, processingTime)) return -1; 

  p = kmalloc(sizeof(struct mp2_task_struct), GFP_KERNEL);

  // get the task by given PID
  p->linux_task = find_task_by_pid(pid);

  //update task struct
  p->pid = pid;
  p->period = period;

  
  if(p->linux_task == NULL){
    // no task was found associated with given PID
    printk(KERN_INFO "No task associated with PID %ld\n", pid);
    // free the memory
    kfree(p);
    // return error
    return -1;
  }
  mutex_lock(&mp2_mutex);
  _insert_task(p);
  mutex_unlock(&mp2_mutex);
  printk(KERN_INFO "Task added to list\n");

  return 0;
}

// Un-register this PID from the task list.
// Return 0 if the task is removed from the list, -1 if it was not found
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

int proc_registration_read(char *page, char **start, off_t off, int count, int* eof, void* data)
{
  printk(KERN_INFO "Reading from proc file\n");
  return 1;
}

int proc_registration_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
  printk(KERN_INFO "Writing to proc file\n");

  char *proc_buffer;
  char *action;
  long pid, comp;
  int status;
  long period=0;

  proc_buffer=kmalloc(count, GFP_KERNEL);
  action=kmalloc(2, GFP_KERNEL);
  status=copy_from_user(proc_buffer, buffer, count);
  sscanf(proc_buffer, "%s %ld %ld", action, &pid, &comp);
  printk(KERN_INFO "From /proc/mp2/status: %s, %ld, %ld\n", action, pid, comp); 

  if(strcmp(action, "R")==0){
    printk(KERN_INFO "Going to register PID %ld\n", pid);
    // perform registration
    register_task(pid, period, comp);
  }
  if(strcmp(action, "D")==0){
    printk(KERN_INFO "Going to un-register PID %ld\n", pid);
    // perform de-registration
    unregister_task(pid);
  }
  // free the memory
  kfree(proc_buffer);
  kfree(action);

  return count;
}

// Frees the memory from the list
void _destroy_task_list(void)
{
  struct list_head *pos, *tmp;
  struct mp2_task_struct *p;

  list_for_each_safe(pos, tmp, &mp2_task_list)
    {
      p = list_entry(pos, struct mp2_task_struct, task_node);
      list_del(pos);
      printk(KERN_INFO "Destroying task associated with PID %ld\n", p->pid);
      kfree(p);
    }
}

//THIS FUNCTION GETS EXECUTED WHEN THE MODULE GETS LOADED
//NOTE THE __INIT ANNOTATION AND THE FUNCTION PROTOTYPE
int __init my_module_init(void)
{
   mp2_proc_dir=proc_mkdir("mp2",NULL);
   register_task_file=create_proc_entry("status", 0666, mp2_proc_dir);
   register_task_file->read_proc= proc_registration_read;
   register_task_file->write_proc=proc_registration_write;

   //THE EQUIVALENT TO PRINTF IN KERNEL SPACE
   printk(KERN_INFO "MP2 Module LOADED\n");
   return 0;   
}

//THIS FUNCTION GETS EXECUTED WHEN THE MODULE GETS UNLOADED
//NOTE THE __EXIT ANNOTATION AND THE FUNCTION PROTOTYPE
void __exit my_module_exit(void)
{
   remove_proc_entry("status", mp2_proc_dir);
   remove_proc_entry("mp2", NULL);
   _destroy_task_list();
   printk(KERN_INFO "MP2 Module UNLOADED\n");
}

//WE REGISTER OUR INIT AND EXIT FUNCTIONS HERE SO INSMOD CAN RUN THEM
//MODULE_INIT AND MODULE_EXIT ARE MACROS DEFINED IN MODULE.H
module_init(my_module_init);
module_exit(my_module_exit);

//THIS IS REQUIRED BY THE KERNEL
MODULE_LICENSE("GPL");
