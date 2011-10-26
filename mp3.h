///////////////////////////////////////////////////////////////////////////////
//
// MP2:		Virtual Memory Page Fault Measurement
// Name:        mp3.h
// Date: 	10/1/2011
// Group:	20: Intisar Malhi, Alexandra Mirtcheva, and Roberto Moreno
// Description: This is the header file for the Virtual Memory Page Fault Profiler
//		kernel module in mp3.c
//		Compiled for Fedora Core 15 64-bits, Linux Kernel 2.60.40. 
//
///////////////////////////////////////////////////////////////////////////////
#ifndef __MP3_INCLUDE__
#define __MP3_INCLUDE__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include "mp3_given.h"

//#define JIFF_TO_MS(t) ((t*1000)/ HZ)
//#define MS_TO_JIFF(j) ((j * HZ) / 1000)
//#define UPDATE_TIME 5000

//#define PROCESSING_TIME_RATIO(t, p) ((t*1000) / p)
 
//#define TASK_STATE_READY     0
//#define TASK_STATE_RUNNING   1
//#define TASK_STATE_SLEEPING  2

// PROCESS CONTROL BLOCK 
struct mp3_task_struct
{
  long pid;
  struct task_struct* linux_task;	// the real PCB
  struct timer_list wakeup_timer;
  struct list_head task_node;
  long unsigned period;			// period
  long unsigned ptime;			// processing time
  long previous_time;  
  int first_yield_call;
  int  task_state;
};

//PROC FILESYSTEM ENTRIES
static struct proc_dir_entry *mp3_proc_dir;
static struct proc_dir_entry *register_task_file;

struct mp3_task_struct *mp3_current_task;
struct task_struct* dispatch_kthread;
int stop_dispatch_thread=0;

// PROFILE BUFFER
int *p_addr; 		// pointer to memory area 
unsigned long mem_size; // memory area size

LIST_HEAD(mp3_task_list);
static DEFINE_MUTEX(mp3_mutex);
#endif
