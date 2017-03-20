#include <unistd.h>     /* Symbolic Constants */
#include <sys/types.h>  /* Primitive System Data Types */ 
#include <errno.h>      /* Errors */
#include <stdio.h>      /* Input/Output */
#include <stdlib.h>     /* General Utilities */
#include <pthread.h>    /* POSIX Threads */
#include <string.h>     /* String handling */
#include <semaphore.h>  /* Semaphore */
#include <stdbool.h>    /* Booleans */
#include <sys/time.h>   /* Local time */
#include <fcntl.h>

#define MAX1 4       	/* Max size of buffers*/
#define MAX2 4
#define MAX_STRING 50

char **buffer1;
char **buffer2;

pthread_mutex_t mutex, mutex2;         
pthread_cond_t spaceAvailable, itemAvailable;

int count1, count2;

int in1, in2, out1, out2;
int fp_green, fp_black, fp_output;

void produce( void *ptr );
void consume( void *ptr );

void main()
{
  buffer1 = malloc(4 * sizeof(char*));
  buffer2 = malloc(4 * sizeof(char*));

  // Allocate memory for buffer elements
  int i;
  for (i = 0; i < MAX1; i++) {
    buffer1[i] = malloc(MAX_STRING * sizeof(char)); 
  }
  
  int j;
  for (j = 0; j < MAX2; j++) {
    buffer2[j] = malloc(MAX_STRING * sizeof(char)); 
  }

  // open files
  int perms = 0740;
  if ((fp_green = open("prod_green.txt", (O_WRONLY | O_CREAT), perms)) == -1) {
     perror("Failed to open file:");
  }
  if ((fp_black = open("prod_black.txt", (O_WRONLY | O_CREAT), perms)) == -1) {
     perror("Failed to open file:");
  }
  if ((fp_output = open("output.txt", (O_WRONLY | O_CREAT), perms)) == -1) {
     perror("Failed to open file:");
  }

  // Arguments for producer threads
  char* id_green = (char *) malloc(100);      
  char* id_black = (char *) malloc(100);

  strcpy(id_green, "GREEN");
  strcpy(id_black, "BLACK");

  // Initialize shared variables
  count1 = count2 = in1 = in2 = out1 = out2 = 0;
  
  // Initialize mutex
  pthread_mutex_init(&mutex, NULL);
  pthread_mutex_init(&mutex2, NULL);

  // Declare threads
  pthread_t producer_green;
  pthread_t producer_black;
  pthread_t consumer;

  // Create threads
  pthread_create(&producer_green, NULL, (void *) &produce, (void *) id_green);
  pthread_create(&producer_black, NULL, (void *) &produce, (void *) id_black);
  pthread_create(&consumer, NULL, (void *) &consume, NULL);  

  // Join threads
  pthread_join(producer_green, NULL);
  pthread_join(producer_black, NULL);
  pthread_join(consumer, NULL);

  // Free memory
  pthread_mutex_destroy(&mutex);
  free(id_green);
  free(id_black);

  for (i = 0; i < MAX1; i++) {
    free( buffer1[i] );
  }
  
  for (j = 0; i < MAX2; j++) {
    free( buffer2[j] );
  }

  free( buffer1[1] );
  free( buffer2[2] );

  // Terminate program
  exit(0);
}

void produce( void *ptr ){

  char *item = (char *) ptr;
  char *time = malloc(MAX_STRING * sizeof(char));
  char *message =  malloc(MAX_STRING * sizeof(char));
  struct timeval tv;
  int fwrite;
  
  // Sleep
  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 100000000L;
	
  int i;
  for(i = 1; i <= 50; i++) {

    pthread_mutex_lock(&mutex);
    while (count1 + count2 == 8){
      while ( pthread_cond_wait(&spaceAvailable, &mutex) != 0);
    }
    // Prepare the item 
    gettimeofday(&tv, NULL);
    sprintf(time, " %ld", tv.tv_usec);
    strcat(message, item);
    strcat(message, time);
    strcat(message, "\n");

    // Put the item in appropriate buffer
    if(count1 < MAX1) {
       strcpy(buffer1[in1], message);
       fwrite = write(fp_green, message, strlen(message));
       in1 = (in1 + 1) % MAX1;
       count1++;
    }else {
       strcpy(buffer2[in2], message);
       fwrite = write(fp_black, message, strlen(message));
       in2 = (in2 + 1) % MAX2;
       count2++;
    }
    printf("%s items produced: %d\n",item, i);
    // Release lock
    pthread_mutex_unlock(&mutex);

    // Signal consumer
    pthread_cond_signal(&itemAvailable);

    // Reset the time and message buffers
    memset(time,0, MAX_STRING);
    memset(message,0, MAX_STRING);
    
    // Sleep
    nanosleep(&tim , &tim2);
  }
  printf("%s Producer exiting!\n", item);
  fflush(0);
  free(item);
  exit(0);
}


void consume( void *ptr){

  char *message = (char *) malloc(MAX_STRING * sizeof(char));
  char *reportFile = (char*) malloc(MAX_STRING * sizeof(char));
  int fwrite;
	
  int i = 1;
  while (i <= 200) {

    pthread_mutex_lock( &mutex );
    while (count1 + count2 == 0){
      while( pthread_cond_wait(&itemAvailable, &mutex) != 0 );
    }

    // Put the item in appropriate buffer
    if(count1 > 0) {
       count1--;
       fwrite = write(fp_output, buffer1[out1], strlen(buffer1[out1]));
       out1 = (out1 + 1) % MAX1;
    }else {
       count2--; 
       fwrite = write(fp_output, buffer2[out2], strlen(buffer2[out2]));
       out2 = (out2 + 1) % MAX2; 
    }
    printf("Consumer: items consumed: %d\n", i);
    // Release the lock
    pthread_mutex_unlock(&mutex);

    // Signal producer
    pthread_cond_signal(&spaceAvailable);
    i++;
  }
  // Free memory
  free(message);
  free(reportFile);

   printf("Consumer exiting\n");
   /* Gracefully exit */
   exit(0); 
}
