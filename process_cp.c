#include <errno.h>
#include <stdio.h>
#include <string.h>     /* String Handling */
#include <sys/types.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>	
#include <pthread.h>    /* Process sharing */

#include "shared_mem.h"

int main(int argc, char *argv[])
{
    int max_process = 3;           /* Total number of processes  */
    pid_t pids[max_process];       /* PIDs of child processes    */
  
    struct shared_mem *shmem_ptr;  /* pointer to shared segment */
    int shmem_id;       	   /* shared memory identifier  */

    key_t key;                     /* A key to access shared memory segments */
    int size;                      /* Memory size needed, in bytes           */
    int flag;                      /* Memory segment Permissions             */

    key  = 5000;         
    size = 2048;    
    flag = 1023;        

    shmem_ptr = malloc(sizeof(struct shared_mem));

    /* Create a shared memory segment */
    shmem_id = shmget (key, sizeof(struct shared_mem), flag);
    if (shmem_id == -1)
    {
        perror ("shmget spaghetti failed");
        exit (1);
    }

    /* Arguments for producer threads */
    char id_green[5];     
    char id_black[5];

    strcpy(id_green, "GREEN");
    strcpy(id_black, "BLACK");

    /* Initialize shared count and index variables */
    shmem_ptr->count1 = shmem_ptr->count2 = 0;
    shmem_ptr->in1    = shmem_ptr->in2    = 0; 
    shmem_ptr->out1   = shmem_ptr->out2   = 0;
  
    /* Initialize shared mutex */
    pthread_mutexattr_init(&shmem_ptr->attrmutex);
    pthread_mutexattr_setpshared(&shmem_ptr->attrmutex, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&shmem_ptr->mutex, NULL);


    /* Attach the new segment into address space */
    shmem_ptr = shmat (shmem_id, (void *) NULL, flag);
    if (shmem_ptr == (void *) -1)
    {
        perror ("shmat failed");
        exit (2);
    }
    
    /* Fork the processes*/
    int x;
    for(x = 0; x < 3; x++){
      /* this is the parent process */
      if (( pids[x] = fork()) < 0) {
	perror("fork");
      }
      /* this is the child process */
      else if(pids[x] == 0)
      {
        char keystr[10];

        /* Execute the child program in this process, passing it the key
           to shared memory segment as a command-line parameter.
	*/
        sprintf (keystr, "%d", key);
	printf("Executing child program.....NOW!\n");
	if(x == 0) {
	  execl ("./producer", "prod_green", keystr, id_green, NULL);
	} else if (x == 1){
	  execl ("./producer", "prod_black", keystr, id_black, NULL);
	} else {
	  execl ("./consumer", "con", keystr, NULL);
	}
      }
    } // End for

    /* Wait for child processes */
    int childStatus;
    pid_t returnValue;
    int process_remaining = max_process;
    while( process_remaining > 0) {
      returnValue = wait(&childStatus);

      /* Report child process exit status */
      if (returnValue > 0)
      {
	if (WIFEXITED(childStatus)) {
	  printf("Child with PID %ld exit Code: %d\n\n",(long) returnValue, WEXITSTATUS(childStatus));
	}
        else {
	  printf("Child with PID %ld exit Status: 0x%.4X\n", (long) returnValue, childStatus);
        }
      }
      else if (returnValue == 0) 
      {
	 printf("Child with PID %ld process still running\n", (long) returnValue);
      } 
      else
      {
	if (errno == ECHILD) {
	   printf("Child with PID %ld: Error ECHILD.\n", (long) returnValue);
	}
        else if (errno == EINTR) {
	   printf("Child with PID %ld: Error EINTR.\n", (long) returnValue);
	}
        else {
	  printf("Child with PID %ld: Error EINVAL.\n", (long) returnValue);
        }
      }
      process_remaining--;
      printf("Process remaing: %d\n", process_remaining);
    } // End while

    /* Free memory */
    pthread_mutex_destroy(&shmem_ptr->mutex);
    pthread_mutexattr_destroy(&shmem_ptr->attrmutex);

    /* Detach the shared segment and terminate */
    shmdt ( (void *) shmem_ptr);

    /* Destroy the shared memory segment and return it to the system */
    shmctl (key, IPC_RMID, NULL);

    printf("Main is exiting\n");
    // Terminate program
    exit(0);
}
