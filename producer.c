#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>

#include "shared_mem.h"
			
struct shared_mem *shmem_ptr;	/* Pointer to shared memory */

main(int argc, char *argv[])
{
    char *item       = argv[2];	                       /* Item that will produced */
    char *time       = malloc(20       *sizeof(char)); /* Time each item was produced */
    char *message    = malloc(MAX_SIZE *sizeof(char)); /* Message to put in buffer */
    char *reportFile = malloc(MAX_SIZE *sizeof(char)); /* Name of the report to write to */

    struct timeval tv;                                 /* gettimeofday() structure */
    int fwrite;                                        /* Bytes written to file */
    int fp;					       /* File descriptor */

    /* Set up the ouput file */
    int perms = 0740;
    strcpy(reportFile, item);
    strcat(reportFile, ".txt");
    if ((fp = open(reportFile, (O_WRONLY | O_CREAT), perms)) == -1) {
      perror("Failed to open file:");
    }

    struct timespec tim, tim2;    /* nanosleep() structure */
    tim.tv_sec = 0;               /* Sleep min */
    tim.tv_nsec = 100000000L;      /* Sleep max */

    int id;         		  /* Shared memory identifier */  
    int flag = 1023;     	  /* Shared memory permissions */

    /* Get segment id of the segment that the parent process created */
    id = shmget (atoi(argv[1]), 0, 0);
    if (id == -1)
    {
        perror ("child shmget failed");
        exit(1);
    }

    /* Attach this segment into the address space */
    shmem_ptr = shmat (id, (void *) NULL, flag);
    if (shmem_ptr == (void *) -1)
    {
        perror ("child shmat failed");
        exit(2);
    }

    int itemsProduced;
    for(itemsProduced = 1; itemsProduced <= 1000; itemsProduced++) {

      pthread_mutex_lock(&(shmem_ptr->mutex));
      /* Wait if no space available */
      while ((shmem_ptr->count1 + shmem_ptr->count2) == 8){
	while ( pthread_cond_wait(&(shmem_ptr->spaceAvailable), &(shmem_ptr->mutex)) != 0);
      }

      /* Prepare the item */ 
      gettimeofday(&tv, NULL);
      sprintf(time, " %ld", tv.tv_sec);
      strcat(message, item);
      strcat(message, time);
      strcat(message, "\n");

      /* Put item in buffer with available space */
      if(shmem_ptr->count1 < MAX_ELEM) {
	memcpy(shmem_ptr->buffer1[shmem_ptr->in1], message, MAX_SIZE);
	fwrite = write(fp, message, strlen(message));
	shmem_ptr->in1 = (shmem_ptr->in1 + 1) % MAX_ELEM;
	(shmem_ptr->count1)++;
      }else {
	memcpy(shmem_ptr->buffer2[shmem_ptr->in2], message, MAX_SIZE);
	fwrite = write(fp, message, strlen(message));
	shmem_ptr->in2 = (shmem_ptr->in2 + 1) % MAX_ELEM;
	(shmem_ptr->count2)++;
      }
      // Release lock
      printf("%s Producer: produced item %d\n", item, itemsProduced);
      pthread_mutex_unlock(&(shmem_ptr->mutex));

      // Signal consumer
      pthread_cond_signal(&(shmem_ptr->itemAvailable));

      // Reset the time and message buffers
      memset(time   ,0, MAX_SIZE);
      memset(message,0, MAX_SIZE);
   
      // Sleep random time
      nanosleep(&tim , &tim2);
    }

    /* Free local memory 
     * Note: time and message memory cleared on last iteration
     */
    free(reportFile);
    close(fp);

    /* Detach the shared segment and terminate */
    shmdt ( (void *) shmem_ptr);    

    printf("Producer exiting!\n");
    /* Exit Gracefully */
    exit(0);
}


