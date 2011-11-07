#ifndef __MP3_GIVEN_INCLUDE__
#define __MP3_GIVEN_INCLUDE__

#include <linux/pid.h>

#define find_task_by_pid(nr) pid_task(find_vpid(nr), PIDTYPE_PID)

//THIS FUNCTION RETURNS 0 IF THE PID IS VALID AND THE CPU TIME AND MAJOR AND MINOR PAGE FAULT COUNTS ARE SUCCESFULLY RETURNED BY THE PARAMETER CPU_USE. OTHERWISE IT RETURNS -1
int get_cpu_use(int pid, unsigned long *min, unsigned long *maj, unsigned long *cpu_use)
{
   struct task_struct* task;
   rcu_read_lock();
   task=find_task_by_pid(pid);

   if (task!=NULL) {  
     printk("get_cpu_use: (direct from task struct) Value of pid=%d min=%ld, maj=%ld, utime=%ld\n", pid, task->min_flt, task->maj_flt, task->utime + task->stime);
     *cpu_use = ((task->stime + task->utime)/jiffies) * 1000;
     *min = task->min_flt;
     *maj = task->maj_flt;
     printk("get_cpu_use: (before reset) Value of pid=%d min=%ld, maj=%ld, utime=%ld\n", pid, task->min_flt, task->maj_flt, *cpu_use);

     // reset the values to 0
     //task->utime=0;
     //task->stime=0;
     task->min_flt=0;
     task->maj_flt=0;

     //printk("get_cpu_use: (after reset) Value of pid=%d min=%ld, maj=%ld, utime=%ld\n", pid, task->min_flt, task->maj_flt, task->utime);
     printk("get_cpu_use: (return data) Value of pid=%d min=%ld, maj=%ld, cpu_use=%ld\n", pid, *min, *maj, *cpu_use);

     rcu_read_unlock();
     return 0;
   }
   else {
     rcu_read_unlock();
     return -1;
   }
}

#endif
