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
#include "mp3_given.h"

// CHAR DEVICE
char memory_buf[12000];  // character device
int open_dev(struct inode *inode, struct file *filep);
int close_dev(struct inode *inode, struct file *filep);
unsigned int mmap(int addr, int buff_len, pgprot_t prot, unsigned short flags, int fd, int offset);

struct file_operations mp3_fops = {
    open  : open_dev,
    mmap  : mmap,
    release : close_dev
};

int open_dev(struct inode *inode, struct file *filep)
{
    return 0;
}

int close_dev(struct inode *inode, struct file *filep)
{
    return 0;
}

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
int *p_addr; 		// pointer to memory area 
unsigned long mem_size; // memory area size

// workqueue
struct delayed_work *wqueue;
int queue_stop=0;	// determines when work should stop

LIST_HEAD(mp3_task_list);
static DEFINE_MUTEX(mp3_mutex);
#endif
