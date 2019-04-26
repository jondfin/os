// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	January 2019

// List all group member's name: Jon Cobi Delfin
// username of iLab: jjd279
// iLab Server: top.cs.rutgers.edu

#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* To use real pthread Library in Benchmark, you have to comment the USE_MY_PTHREAD macro */
#define USE_MY_PTHREAD 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/time.h>
#include <string.h> 

typedef uint my_pthread_t;

typedef struct threadControlBlock {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE
	my_pthread_t tid;
	enum {RUNNING, BLOCKED, READY, STOPPED} status;
	ucontext_t *context;
	int priority;
	struct threadControlBlock *next;
} tcb; 

/* mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* add something here */

	// YOUR CODE HERE
	volatile int locked;
} my_pthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE
typedef struct Queue{
	tcb *front, *end;
} Queue;
/* Function Declarations: */

/*Enqueue a thread based on priority*/
void priorityEnqueue(Queue *q, tcb *t);
/*Enqueue a thread normally*/
void enqueue(Queue *q, tcb *t);
/*Dequeue a thread*/
tcb *dequeue(Queue *q);
/*Context switch to scheduler*/
void switchToScheduler(int signum, siginfo_t *info, void *old);
/*Find the next highest priority thread*/
int getHighestPriority();
/*Find tid in queue*/
int searchQueue(Queue *q, my_pthread_t thread);

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif

#endif
