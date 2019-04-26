// File: project1.c
// Author: Yujie REN
// Date: January 2019
// 

// Student name: Jon Cobi Delfin and Matthew Enad
// Ilab machine used: top.cs.rutgers.edu

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

pthread_t t1, t2;
pthread_mutex_t mutex;
int x = 0;

void segment_fault_handler(int signum) {

	printf("I am slain!\n");

	// Do your tricks here for Part 1
	int *ptr = &signum + 51;
	(*ptr) += 2;
}

void *inc_shared_counter(void *arg) {
	// Add your code here for Part 2
	int i = 0;
	while(i < 5){
		i++;
		pthread_mutex_lock(&mutex);
		x += 1;	
		printf("x is incremented to %d\n", x);
		pthread_mutex_unlock(&mutex);
	}

	return NULL;
}

int main() {
	/* Part 1: Recall of x86 calling convention */

	int r2 = 0;

	signal(SIGSEGV, segment_fault_handler);

	r2 = *( (int *) 0 );
	
	printf("I live again!\n");

	/* Part 2: Recall of pThread programming */
	// Add your code here for Part 2
	pthread_create(&t1, NULL, inc_shared_counter, NULL);
	pthread_create(&t2, NULL, inc_shared_counter, NULL);
	
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	
	printf("The final value of x is %d\n", x);

	return 0;
}
