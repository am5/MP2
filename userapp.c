#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

long long factorial(int number)
{
  long long retval=1;
  long long i;
  
  for (i=1; i <= number; i++)
  {
    retval=retval *i; 
  }

  return retval;
}
// Read the proc file /proc/mp2/status.
// Return: true, if the PID is registered (appears in the file); false if otherwise
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
// Attempts to register with the mp2 kernel module.
// Return: true, if we were admitted, false if otherwise
bool try_register(pid_t pid, long period, long processTime){
  char cmd[120];
  sprintf(cmd, "echo 'R %u %u %u'>//proc/mp2/status", pid, period, processTime);
  system(cmd);

  // let's check if we are a registered process
  return is_registered(pid);
}

// Sets yield status in proc file for this PID.
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

// Performs the unregistration from kernel module task list.
// Return true if we did unregister (no longer in proc_read output); false if otherwise
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

int main(int argc, char* argv[])
{
  pid_t mypid;
//  int j,k;

  long period = 3000;		// in milliseconds
  long processTime = 1000;	// in milliseconds
  
  // get our PID so that we can register
  mypid= syscall(__NR_gettid);

  // register with the module
  if(!try_register(mypid, period, processTime)){
    printf("Unable to register this PID %ld\n", mypid);
  }
  // writes yield to proc/mp2/status file for this pid
  try_yielding(mypid);
  sleep(10);	// just for testing
/*
  for (k=0;k<10000; k++)
  for(j=0; j<30; j++)
  {
	printf("Factorial %u: %llu\n",j, factorial(j));
        try_yielding(mypid);
  }
*/
  // unregister from the module
  if(try_unregister(mypid)){
    printf("We successfully unregistered from the module!\n");
  }else{
    printf("We are still registered...\n");
  }
}
