///////////////////////////////////////////////////////////////////////////////
//
// MP2:		User Application
// Name:        userapp.c
// Date: 	10/1/2011
// Group:	20: Intisar Malhi, Alexandra Mirtcheva, and Roberto Moreno
// Description: This source tests the Rate-Monotonic CPU scheduler defined by
//	        mp2.c by computing the factorial of a number. 
//
///////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  factorial
//
// PROCESSING:
//
//    This function calculates the factorial of a number 
//
// INPUTS:
//
//    number - the number passed to the function that the factorial is 
//	       calculated for. 
//
// RETURN:
//
//   long long - the factorial of the given number. 
//
// IMPLEMENTATION NOTES
//
//   None.
//
///////////////////////////////////////////////////////////////////////////////
long long factorial(int number)
{
  long long retval=1;
  long long i;
  
  for (i=1; i <= number; i++)
  {
    retval = retval * i; 
  }

  return retval;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  is_registered
//
// PROCESSING:
//
//    This function verifies that the given process ID is registered 
//
// INPUTS:
//
//    pid - the PID of the processes
//
// RETURN:
//
//    bool - FALSE, if the PID is not registered (doesn't appear in the proc entry)
//    	     TRUE, if the PID is registered (appears in the proc entry)
//
// IMPLEMENTATION NOTES
//
//   The is_registered user-space function uses a file handle to the 
//   /proc/mp2/status file in order to read from the file. It uses fscanf
//   to look for the given PID and returns the result. 
//
///////////////////////////////////////////////////////////////////////////////
bool is_registered(pid_t pid){
  
  FILE *mp2_proc;	// for /proc/mp2/status file handler

  mp2_proc = fopen("/proc/mp2/status", "r");
  if(mp2_proc == NULL){
    // an error occurred when trying to read the file
    printf("Unable to open /proc/mp2/status for reading\n");
    return false;
  }

  long fpid;  // this will hold the current PID from file
  bool result=false;
  printf("Going to read /proc/mp2/status\n");
  while(fscanf(mp2_proc, "%ld", &fpid) != EOF){
    printf("current PID=%d\n", fpid);
    if(fpid == pid){
      // match let's set our return value to true and break the loop
      result=true;
      break;
    }
  }
  // close the file
  fclose(mp2_proc);
  printf("Finished reading /proc/mp2/status\n");

  // return true if PID is found (registred); false if otherwise
  return result;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  try_register
//
// PROCESSING:
//
//    This function attempts to register the task with the mp2 kernel module 
//
// INPUTS:
//
//    pid 		- the PID of the process
//    period 		- the period of the process
//    processTime 	- the process time of the process 
//
// RETURN:
//
//    bool - FALSE, if the PID is not registered (doesn't appear in the proc entry)
//    	     TRUE, if the PID is registered (appears in the proc entry)
//
// IMPLEMENTATION NOTES
//
//   The try_register function uses a file handle to the /proc/mp2/status 
//   and then writes a formatted register message to the file. After the call 
//   to the kernel, the function verifies that the PID appears in the proc file 
//   by calling is_registered. 
//
///////////////////////////////////////////////////////////////////////////////
bool try_register(pid_t pid, long period, long processTime){
  FILE *mp2_proc;	// for /proc/mp2/status file handler

  mp2_proc = fopen("/proc/mp2/status", "w");
  if(mp2_proc == NULL){
    // an error occurred when trying to read the file
    printf("Unable to open /proc/mp2/status for writing\n");
    return false;
  }

  // create the string to write to the proc file
  char action[128];
  sprintf(action, "R %d %ld %ld", pid, period, processTime);
  fputs(action, mp2_proc);
  
  // close the file
  fclose(mp2_proc);

  // let's check if we are a registered process
  return is_registered(pid);
}

// Sets yield status in proc file for this PID.
///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  try_yielding
//
// PROCESSING:
//
//    This function attempts to set the yield status in the proc entry for 
//    a given PID. 
//
// INPUTS:
//
//    pid - the PID of the process
//
// RETURN:
//
//    bool - FALSE, the /proc/mp2/status could not be opened for writing 
//    	     TRUE, as default
//
// IMPLEMENTATION NOTES
//
//   The try_yielding function uses a file handle to the /proc/mp2/status 
//   and then writes a formatted yield message to the file. It closes the 
//   file after it has finished writing to it. 
//
///////////////////////////////////////////////////////////////////////////////
bool try_yielding(pid_t pid){
  FILE *mp2_proc;	// for /proc/mp2/status file handler

  mp2_proc = fopen("/proc/mp2/status", "w");
  if(mp2_proc == NULL){
    // an error occurred when trying to read the file
    printf("Unable to open /proc/mp2/status for writing\n");
    return false;
  }

  // create the string to write to the proc file
  char action[128];
  sprintf(action, "Y %d", pid);
  fputs(action, mp2_proc);
  
  // close the file
  fclose(mp2_proc);

  return true;
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  try_unregister
//
// PROCESSING:
//
//    This function attempts to unregister the given PID from the kernel module
//    task list. 
//
// INPUTS:
//
//    pid - the PID of the process
//
// RETURN:
//
//    bool - FALSE, the /proc/mp2/status could not be opened for writing 
//    	     TRUE, if the task is not registered 
//
// IMPLEMENTATION NOTES
//
//   The try_unregister function uses a file handle to the /proc/mp2/status 
//   and then writes a formatted yield message to the file. It closes the file
//   after it has finished writing to it. 
//
///////////////////////////////////////////////////////////////////////////////
bool try_unregister(pid_t pid){

  FILE *mp2_proc;	// for /proc/mp2/status file handler

  mp2_proc = fopen("/proc/mp2/status", "w");
  if(mp2_proc == NULL){
    // an error occurred when trying to read the file
    printf("Unable to open /proc/mp2/status for writing\n");
    return false;
  }

  // create the string to write to the proc file
  char action[128];
  sprintf(action, "D %u", pid);
  fputs(action, mp2_proc);
  
  // close the file
  fclose(mp2_proc);

  // let's check if we are still registered, return false if we are, true if we aren't
  return !is_registered(pid);
}

///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION NAME:  main
//
// PROCESSING:
//
//    This function tests the register, unregister, and yield functions from 
//    user space. 
//
// INPUTS:
//
//    argc - the number of arguments passed to the program
//    argv - the command line vector that contains the arguments passed to the program
//
// RETURN:
//
//    int - (0) default with no errors
//          (-1) unable to register the PID 
//
// IMPLEMENTATION NOTES
//
//   The main function tests all the functions that were implemented in kernel 
//   space that the user-space application will use in order to register and 
//   unregister with the kernel module, and to change it status using yield.  
//
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
  pid_t mypid;
  int j;

  long period = 200;		// in milliseconds
  long processTime = 10;	// in milliseconds
  
  // get our PID so that we can register
  mypid= syscall(__NR_gettid);

  // register with the module
  if(!try_register(mypid, period, processTime)){
    printf("Unable to register this PID %ld\n", mypid);
    return -1;
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  printf("Current time %d\n", tv.tv_sec);
  // writes yield to proc/mp2/status file for this pid
  try_yielding(mypid);
  gettimeofday(&tv, NULL);
  printf("Current time %d\n", tv.tv_sec);

  for(j=1; j<1000; j++)
  {
	printf("Factorial %u: %llu\n",j, factorial(10));
        try_yielding(mypid);
  	gettimeofday(&tv, NULL);
        printf("Current time %d\n", tv.tv_sec);
  }
  // unregister from the module
  if(try_unregister(mypid)){
    printf("We successfully unregistered from the module!\n");
  }else{
    printf("We are still registered...\n");
  }

}
