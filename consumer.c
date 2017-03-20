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

#include "shared_mem.h"
			
struct shared_mem *shmem_ptr;	/* Pointer to shared memory */

main(int argc, char *argv[])
{
    char *message    = malloc(MAX_SIZE * sizeof(char));  /* Message to put in buffer */
    char *reportFile = malloc(MAX_SIZE * sizeof(char));  /* Name of the report file  */
    int   fwrite;                                        /* Bytes written            */
    int   fp;

    int id;         	/* Shared memory identifier  */       
    int flag = 1023;    /* Shared memory permissions */
    int perms = 0740;   /* Output file permissions   */

    /* Open the output file */
    if ((fp = open("output.txt", (O_WRONLY | O_CREAT), perms)) == -1) {
      perror("Failed to open file:");
    }

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

    int itemsConsumed = 1;
    while (itemsConsumed <= 2000) {

      pthread_mutex_lock( &(shmem_ptr->mutex) );
      /* Wait for items to placed into the buffer */
      while ((shmem_ptr->count1 + shmem_ptr->count2) == 0){
	while ( pthread_cond_wait(&(shmem_ptr->itemAvailable), &(shmem_ptr->mutex)) != 0);
      }

      /* Remove an item from the appropriate buffer */
      if(shmem_ptr->count1 > 0) {
	(shmem_ptr->count1)--;
	fwrite = write(fp, shmem_ptr->buffer1[shmem_ptr->out1], strlen(shmem_ptr->buffer1[shmem_ptr->out1]));
	shmem_ptr->out1 = (shmem_ptr->out1 + 1) % MAX_ELEM;
      } else {
	(shmem_ptr->count2)--; 
	fwrite = write(fp, shmem_ptr->buffer2[shmem_ptr->out2], strlen(shmem_ptr->buffer2[shmem_ptr->out2]));
	shmem_ptr->out2 = (shmem_ptr->out2 + 1) % MAX_ELEM; 
      }
      printf("Consumer: items consumed: %d\n", itemsConsumed);
      // Release the lock
      pthread_mutex_unlock(&(shmem_ptr->mutex));

      // Signal producer
      pthread_cond_signal(&(shmem_ptr->spaceAvailable));

      itemsConsumed++;
    }

    // Free memory
    free(message); 
    free(reportFile);
    close(fp);

    /* Detach the shared segment and terminate */
    shmdt ( (void *) shmem_ptr);

    printf("Consumer exiting!\n");
    /* Exit Gracefully */
    exit(0);
}
