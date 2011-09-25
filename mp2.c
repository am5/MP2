#include "mp2.h"

int proc_registration_read(char *page, char **start, off_t off, int count, int* eof, void* data)
{
  printk(KERN_INFO "Reading from proc file\n");
  return 1;
}

int proc_registration_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
  printk(KERN_INFO "Writing to proc file\n");
  return 1;
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
      // yes, we need to remove this entry
      mutex_lock(&mp2_mutex);
      list_del(pos);
      kfree(p);
      mutex_unlock(&mp2_mutex);
      printk(KERN_INFO "Removing PID %ld", pid);
      found=0;
      break;
    } // no, keep searching
  }
  // return the result status
  return found;
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
