#ifndef __MP2_INCLUDE__
#define __MP2_INCLUDE__

#include <linux/module.h>
#include <linux/kernel.h>

#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <asm/uaccess.h>

#include "mp2_given.h"

#define JIFF_TO_MS(t) ((t*1000)/ HZ)
#define MS_TO_JIFF(j) ((j * HZ) / 1000)

#define UPDATE_TIME 5000

// Process Control Block
struct mp2_task_struct
{
  long pid;
  struct task_struct* linux_task;	// the real PCB
  struct timer_list wakeup_timer;
  struct list_head task_node;
  long unsigned period;			// period
  long unsigned ptime;			// processing time
};

//PROC FILESYSTEM ENTRIES
static struct proc_dir_entry *mp2_proc_dir;
static struct proc_dir_entry *register_task_file;

struct timer_list up_timer;
struct task_struct* dispatch_kthread;
int stop_dispatch_thread=0;

LIST_HEAD(mp2_task_list);
static DEFINE_MUTEX(mp2_mutex);
#endif
