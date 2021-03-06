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

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/timer.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <asm/uaccess.h>
#include <asm/current.h>
#include <asm/segment.h>
#include <linux/vmalloc.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include "mp3_given.h"

unsigned long mem_size = 512*1024;

// CHAR DEVICE
char memory_buf[12000];  // character device
int open_dev(struct inode *inode, struct file *filep);
int close_dev(struct inode *inode, struct file *filep);
int mp3_mmap(struct file *filp, struct vm_area_struct *vma);
ssize_t mp3_read(struct file *filp, char *buff, size_t len, loff_t *off);

struct file_operations mp3_fops = {
    open  : open_dev,
    mmap  : mp3_mmap,
    read  : mp3_read,
    release : close_dev
};

// PROCESS CONTROL BLOCK 
struct mp3_task_struct
{
  long pid;
  struct task_struct* linux_task;	// the real PCB
  struct list_head task_node;
  unsigned long cpu;
  unsigned long maj;
  unsigned long min;
};

//PROC FILESYSTEM ENTRIES
static struct proc_dir_entry *mp3_proc_dir;
static struct proc_dir_entry *register_task_file;

struct mp3_task_struct *mp3_current_task;

// PROFILE BUFFER
unsigned long *p_addr; 		// pointer to memory area 
//unsigned long mem_size; // memory area size

// workqueue
struct delayed_work *wqueue;
int queue_stop=0;	// determines when work should stop
int list_count=0;       // keep track of the number of elements on list
static unsigned long p_index=0;

LIST_HEAD(mp3_task_list);
static DEFINE_MUTEX(mp3_mutex);
#endif
