#include <stdlib.h>
#include <semaphore.h>  /*Semaphores*/

#define MAX_SIZE 50
#define MAX_ELEM 4

typedef struct shared_mem {
  
  /* Shared buffers */
  char buffer1[MAX_ELEM][MAX_SIZE];
  char buffer2[MAX_ELEM][MAX_SIZE];  
  
  /* Shared mutex */ 
  pthread_mutex_t mutex;
  pthread_mutexattr_t attrmutex;

  /* Shared condition variables */
  pthread_cond_t spaceAvailable;
  pthread_condattr_t spaceattr;
 
  pthread_cond_t itemAvailable;
  pthread_condattr_t itemattr;
  
  /* Number of objects currently in each buffer */
  int count1, count2;

  /* Current index to place/remove item in each buffer */
  int in1, in2, out1, out2;

} shared_mem_t;
