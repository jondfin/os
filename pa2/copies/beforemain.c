// File:	my_pthread.c
// Author:	Yujie REN
// Date:	January 2019

// List all group member's name: Jon Cobi Delfin
// username of iLab: jjd279
// iLab Server: top.cs.rutgers.edu

#include "my_pthread_t.h"

// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE

/*The first thread in the run queue is the one that is currently being ran*/
Queue *runQueue;
Queue *blockList;
ucontext_t scheduler;
char GLOBAL_STACK[32768];
/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {
	// Create Thread Control Block
	// Create and initialize the context of this thread
	// Allocate space of stack for this thread to run
	// After everything is all set, push this thread into run queue

	// YOUR CODE HERE
	if(!runQueue) runQueue = (Queue*)malloc(sizeof(Queue));
	int ret;
	/*Create TCB and initialize TCB*/
	tcb *TCB = (tcb*)malloc(sizeof(tcb));
	if(!TCB) return -1;
	TCB->tid = (*thread); //pthread_create passes the address of the pthread
	TCB->status = READY;
	TCB->counter = 0;
	TCB->context = (ucontext_t*)malloc(sizeof(ucontext_t));
	if(!TCB->context){
		free(TCB);
		return -1;
	}
	

	/*Initialize thread context*/
	ret = getcontext(TCB->context);	
	if(ret < 0) return -1;
	/*Initialize 32K stack*/
	TCB->context->uc_stack.ss_sp = malloc(32768);
	if(!TCB->context->uc_stack.ss_sp) return -1;
	TCB->context->uc_stack.ss_size = 32768;

	/*Initialize link*/
	TCB->context->uc_link = NULL;
	
	/*Assign function and push thread into run queue*/
	makecontext(TCB->context, (void*)&function, 1, arg); 
	enqueue(runQueue, TCB);

	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	// Change thread state from Running to Ready
	// Save context of this thread to its thread control block
	// Switch from thread context to scheduler context

	// YOUR CODE HERE
	// This is called within a function so get the current context and save it
	// After it has been saved we need to swap to the scheduler to determine the next thread to run
	ucontext_t *context = runQueue->end->context;
	runQueue->end->status = READY;
	getcontext(context);
	switchToScheduler(context);
	/*Take the current running thread and save its context*/
//	tcb *thread = dequeue(runQueue);
//	thread->status = READY;
//	getcontext(thread->context);

	/*Enqueue the thread again to give time to other threads*/
//	enqueue(runQueue, thread);

	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
	
	// YOUR CODE HERE
	tcb *thread = dequeue(runQueue);
	thread->context->uc_link = NULL;
	free(thread->context->uc_stack.ss_sp);
	free(thread->context);
	thread->status = STOPPED; //probably pointless since thread has already been killed
	thread = NULL;

	return;
};


/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	// Waiting for a specific thread to terminate
	// Once this thread finishes,
	// Deallocated any dynamic memory created when starting this thread
  
	// YOUR CODE HERE
	/*Mark the current running thread as waiting*/
	tcb *current = dequeue(runQueue);
	current->status = WAITING;

	while(runQueue->end->tid != thread){
		if(runQueue->front->next == NULL){
			/*An incorrect TID was passed in*/
			break;
		}
		//Wait for target to be run
		//The schedule should send SIGALRMs at specific intervals to run the
		//next thread in the queue
		//This should not be an infinite loop
	}
	/*Target thread has been found*/
	tcb *target = runQueue->end;
	target->status = RUNNING;
	target->context->uc_link = current->context;
	swapcontext(current->context, target->context);
	
	/*Threads have finished running, clean up memory and pointers*/
	current->status = STOPPED;
	target->status = STOPPED;
	current->context->uc_link = NULL;
	target->context->uc_link = NULL;
	free(current->context->uc_stack.ss_sp);
	free(current->context);
	free(target->context->uc_stack.ss_sp);
	free(target->context);
	current = NULL;
	target = NULL;

	return 0;
};

/* initialize the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	// Initialize data structures for this mutex

	// YOUR CODE HERE
	/*Initialize a list for blocked threads, if not done so already*/
	if(!blockList) blockList = (Queue*)malloc(sizeof(Queue));
	/*Initialize mutex and unlock it*/
	mutex = (my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t));
	if(!mutex) return -1;
	mutex->locked = 0;	
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	// Use the built-in test-and-set atomic function to test the mutex
	// If mutex is acquired successfuly, enter critical section
	// If acquiring mutex fails, push current thread into block list 
	// and context switch to scheduler 

	// YOUR CODE HERE
	if(test_and_set_bit(mutex->locked, mutex) == 0){
		/*Critical section is available*/
			
	}else{
		/*Critical section closed, pushing thread into block list*/
		tcb *thread = dequeue(runQueue);
		thread->status = BLOCKED;
		getcontext(thread->context);
		enqueue(blockList, thread);
		
		switchToScheduler(thread->context);
	}
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	/*Reset the lock*/
	test_and_clear_bit(mutex->locked, mutex);
	
	/*Put the thread back intro the run queue*/
	tcb *thread;	
	while(blockList != NULL){
		thread = dequeue(blockList);
		thread->status = READY;
		enqueue(runQueue, thread);
	}
	return 0;
};


/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in my_pthread_mutex_init
	free(mutex);
	mutex = NULL;	
	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library 
	// should be contexted switched from thread context to this 
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	// if (sched == STCF)
	//		sched_stcf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE
	//TODO write signal handler to catch SIGALRM when timer goes off to swap in a different thread
	//[thread 3:READY] -> [thread 2:READY] -> [thread 1:RUNNING]
	//[thread 1:READY} -> [thread 3:READY] -> [thread 2:RUNNING]
	//Current implementation: FCFS

	/*Set a timer to go off every 5 milliseconds*/
	struct itimerval timer;
	timer.it_value.tv_usec = 5000;
	timer.it_value.tv_sec = 0;
	timer.it_interval.tv_usec = 5000;
	timer.it_interval.tv_sec = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
	
	
	tcb *old = dequeue(runQueue);
	/*If there is only one thread in the queue*/
	if(runQueue->end == NULL){
		old->status = RUNNING;
		setcontext(old->context);
		old->context->uc_link = NULL;
		return;
	}
	runQueue->end->context->uc_link = old->context;
	old->context->uc_link = (old->next == NULL) ? NULL : old->next->context;

	runQueue->end->status = RUNNING;
	old->status = READY;
	enqueue(runQueue, old);
	swapcontext(old->context, runQueue->end->context);
	
	
	

// schedule policy
#ifndef MLFQ
	// Choose STCF
#else 
	// Choose MLFQ
#endif

}

/* Preemptive SJF (STCF) scheduling algorithm */
static void sched_stcf() {
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

// Feel free to add any other functions you need

// YOUR CODE HERE
void enqueue(Queue *q, tcb *t){
	/*Empty queue*/
	if(q->front == NULL){
		q->front = t;
		q->end = t;
		t->next = NULL;
		return;
	}
	/*Newest thread becomes end of list*/
	q->end->next = t;
	q->end = t;
	t->next = NULL;
	return;
}

tcb *dequeue(Queue *q){
	/*One item in queue*/
	tcb *ret = q->front;
	if(q->front->next == NULL){
		q->front = NULL;
		q->end = NULL;
		free(q->end);
		free(q->front);
		return ret;
	}
	/*Oldest thread is removed from front of list*/
	q->front = q->front->next;
	return ret;
}

void switchToScheduler(ucontext_t old){
	/*Initialize scheduler context if not already done*/
	getcontext(&scheduler);
	scheduler.uc_stack.ss_sp = GLOBAL_STACK;
	scheduler.uc_stack.ss_size = sizeof(GLOBAL_STACK);
	scheduler.uc_link = context;
	makecontext(&scheduler, schedule, 0);
	swapcontext(&old, &scheduler);

	return;
}
void sighandler(){
	

	return;
}


int main(){
	/*Initialize globals and timers*/
	
	return 0;
}
