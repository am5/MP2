#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

#define NPAGES (128)   // The size of profiler buffer (Unit: memory page)
#define BUFD_MAX 48000 // The max number of profiled samples stored in the profiler buffer

static int buf_fd = -1;
static int buf_len;

// This function opens a character device (which is pointed by a file named as fname) and performs the mmap() operation. If the operations are successful, the base address of memory mapped buffer is returned. Otherwise, a NULL pointer is returned.
void *buf_init(char *fname)
{
  unsigned int *kadr;

  if(buf_fd == -1){
    buf_len = NPAGES * getpagesize();
    if ((buf_fd=open(fname, O_RDWR|O_SYNC))<0){
        printf("file open error. %s\n", fname);
        return NULL;
    }
  }
  //unsigned int data[buf_len];
  unsigned int *data;
  int i=0;
/*  while(i < buf_len){
    data[i] = (unsigned int) malloc(sizeof(int));
    i++;
  }
  i=0;
  while(read(buf_fd, &data[i++], 4096));
*/
  data = malloc(buf_len);
  
  errno=0;
/*  kadr = mmap(0, buf_len, PROT_READ|PROT_WRITE, MAP_SHARED, buf_fd, 0);
  if (kadr == MAP_FAILED){
      printf("buf file open error, errno=%d.\n", errno);
      return NULL;
  }else if(kadr < 0){
      printf("Error code: %d\n", kadr);
      return NULL;
  }
  return kadr;
*/
  return data;
}

// This function closes the opened character device file.
void buf_exit()
{
  if(buf_fd != -1){
    close(buf_fd);
    buf_fd = -1;
  }
}

int main(int argc, char* argv[])
{
  long *buf;
  int index = 0;
  int i;

  // Open the char device and mmap()
  buf = buf_init("node");
  if(!buf)
    return -1;
  if(buf == NULL){
    printf("buf is NULL\n");
    return -1;
  }
  // Read and print profiled data
  for(index=0; index<BUFD_MAX; index++)
    if(buf[index] != -1) break;
  i=0;
  while(buf[index] != -1){
    printf("%d ", buf[index]);
    buf[index++] = -1;
    if(index >= BUFD_MAX)
      index = 0;

    printf("%d ", buf[index]);
    buf[index++] = -1;
    if(index >= BUFD_MAX)
      index = 0;

    printf("%d ", buf[index]);
    buf[index++] = -1;
    if(index >= BUFD_MAX)
      index = 0;

    printf("%d\n", buf[index]);
    buf[index++] = -1;
    if(index >= BUFD_MAX)
      index = 0;
    i++;
  }
  printf("read %d profiled data\n", i);
  // Close the char device
  buf_exit();
}

