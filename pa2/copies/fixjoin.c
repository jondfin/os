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
Queue **runQueue, **waitQueue;
Queue **blockList;
ucontext_t Main, schedContext;
sigset_t set;
struct sigaction sig;
struct itimerval timer;

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {
	// Create Thread Control Block
	// Create and initialize the context of this thread
	// Allocate space of stack for this thread to run
	// After everything is all set, push this thread into run queue

	// YOUR CODE HERE
	/*Initializations*/
	if(runQueue == NULL){
		runQueue = (Queue**)malloc(8*sizeof(Queue*));
		waitQueue = (Queue**)malloc(sizeof(Queue*));
		waitQueue[0] = (Queue*)malloc(sizeof(Queue));
		waitQueue[0]->front = NULL;
		waitQueue[0]->end = NULL;
		int i;
		for(i = 0; i < 8; i++){
			runQueue[i] = (Queue*)malloc(sizeof(Queue));
			runQueue[i]->front = NULL;
			runQueue[i]->end = NULL;
		}
		/*Set up sigaction and sigmask*/
		memset(&sig, 0, sizeof(sig));
		sig.sa_sigaction = switchToScheduler;
		sig.sa_flags = SA_SIGINFO;
		sigemptyset(&sig.sa_mask);
		sigaction(SIGALRM, &sig, NULL);

		sigemptyset(&set);
		sigaddset(&set, SIGALRM);

		/*Define main context*/
		getcontext(&Main);
				
		/*Set timer to go off every 50 ms*/
		timer.it_value.tv_usec = 50000;
		timer.it_value.tv_sec = 0;
		timer.it_interval.tv_usec = 50000;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);
	}
	sigprocmask(SIG_BLOCK, &set, NULL);

	/*Create and initialize TCB*/
	ucontext_t context;
	tcb *TCB = (tcb*)malloc(sizeof(tcb));
	if(TCB == NULL) return -1;
	TCB->tid = (*thread);
	TCB->status = READY;
	TCB->priority = 7;
	TCB->context = context;
	
	/*Initialize context and allocate stack space(32KB)*/
	getcontext(&TCB->context);
	TCB->context.uc_stack.ss_sp = malloc(32768);
	TCB->context.uc_stack.ss_size = 32768;
	
	TCB->context.uc_link = NULL;
	makecontext(&TCB->context, (void*)&function, 1, arg);
	enqueue(runQueue[TCB->priority], TCB);
	
	sigprocmask(SIG_UNBLOCK, &set, NULL);	
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	// Change thread state from Running to Ready
	// Save context of this thread to its thread control block
	// Switch from thread context to scheduler context

	// YOUR CODE HERE
	sigprocmask(SIG_BLOCK, &set, NULL);	

	/*Get current running thread and mark it as READY*/
	int level = getHighestLevel();
	ucontext_t curr = runQueue[level]->end->context;
	runQueue[level]->end->status = READY;
	/*Save context then context switch to scheduler*/
	getcontext(&curr);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	swapcontext(&curr, &schedContext);
	
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
	
	// YOUR CODE HERE
	sigprocmask(SIG_BLOCK, &set, NULL);
	
	/*Remove the current running thread from the queue*/
	int level = getHighestLevel();
	tcb *thread = dequeue(runQueue[level]);
	thread->status = STOPPED;
	thread->context.uc_link = NULL;
	/*Free memory*/
	free(thread->context.uc_stack.ss_sp);
	free(thread);
	thread = NULL;

	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return;
};


/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	// Waiting for a specific thread to terminate
	// Once this thread finishes,
	// Deallocated any dynamic memory created when starting this thread
  
	// YOUR CODE HERE
//	sigprocmask(SIG_BLOCK, &set, NULL);

	/*Make current running thread wait*/
	int level = getHighestLevel();
	tcb *current = dequeue(runQueue[level]);
	current->status = WAITING;
	enqueue(waitQueue[0], current);

	/*Loop to find target thread*/
	int i = 7;
	tcb *start = runQueue[i]->end;
	while(start == NULL || runQueue[i]->end->tid != thread){
		if(start != NULL){
			/*Only one thread in current level of queue*/
			enqueue(runQueue[i], dequeue(runQueue[i]));
			if(runQueue[i]->end->tid == start->tid){
				/*Back to where the loop started, check next level*/
				if(i == 0){
//					sigprocmask(SIG_UNBLOCK, &set, NULL);
					return -1;
				}
				i--;
				while(runQueue[i]->end == NULL){
					if(i == 0){
//						sigprocmask(SIG_UNBLOCK, &set, NULL);
						return -1;
					}
					i--;
				}
				/*Loop through at the next level of queue*/
				start = runQueue[i]->end;
			}
		}else{
			/*Check other levels if thread is present*/
			while(runQueue[i]->end == NULL){
				if(i == 0){
//					sigprocmask(SIG_UNBLOCK, &set, NULL);
					return -1;
				}
				i--;
			}
			start = runQueue[i]->end;
		}
	}

/*
	tcb *start = runQueue[level]->end;	
	while(runQueue[level]->end->tid != thread){
		enqueue(runQueue[level], dequeue(runQueue[level]));
		if(runQueue[level]->end->tid == start->tid){
			exit(EXIT_FAILURE);
			break;
		} 
	}
*/
	/*Found thread, context switch to it*/
	tcb *target = runQueue[i]->end;
	target->status = RUNNING;
	target->context.uc_link = &current->context;

//	sigprocmask(SIG_UNBLOCK, &set, NULL);

	swapcontext(&current->context, &target->context);

//	sigprocmask(SIG_BLOCK, &set, NULL);

	/*Thread has finished running*/
	target = dequeue(runQueue[level]); //Remove thread from queue
	target->status = STOPPED;
	target->context.uc_link = NULL;
	free(target->context.uc_stack.ss_sp);
	free(target);
	target = NULL;

	/*Put the waited thread back into the run queue*/
	current = dequeue(waitQueue[0]);
	enqueue(runQueue[level], current);
	current->status = READY;
	getcontext(&current->context);
	current->context.uc_link = (current->next == NULL) ? NULL : &current->next->context;

//	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return 0;
};

/* initialize the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	// Initialize data structures for this mutex

	// YOUR CODE HERE
	/*Allocate memory for mutex and unlock it*/
	//mutex = (my_pthread_mutex_t*)malloc(sizeof(my_pthread_mutex_t));
	//if(mutex == NULL) return -1;
	sigprocmask(SIG_BLOCK, &set, NULL);

	mutex->locked = 0;

	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	// Use the built-in test-and-set atomic function to test the mutex
	// If mutex is acquired successfuly, enter critical section
	// If acquiring mutex fails, push current thread into block list 
	// and context switch to scheduler 

	// YOUR CODE HERE
	sigprocmask(SIG_BLOCK, &set, NULL);
	
	if(mutex->locked == 0){
		mutex->locked = 1;
		sigprocmask(SIG_UNBLOCK, &set, NULL);
	}else{
		/*Critical section closed, pushing thread into block list*/
		if(blockList == NULL){
			blockList = (Queue**)malloc(sizeof(Queue*));
			blockList[0] = (Queue*)malloc(sizeof(Queue));
			blockList[0]->front = NULL;
			blockList[0]->end = NULL;
		}

		int level = getHighestLevel();
		tcb *thread = dequeue(runQueue[level]);
		thread->status = BLOCKED;
		getcontext(&thread->context);
		enqueue(blockList[0], thread);

		sigprocmask(SIG_UNBLOCK, &set, NULL);

		/*Context switch to scheduler*/
		swapcontext(&thread->context, &schedContext);

	}
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	sigprocmask(SIG_BLOCK, &set, NULL);

	if(mutex->locked == 0){
		/*Attempting to unlock an unlocked mutex*/
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		return -1;
	}
	/*Lock thread and put blocked thread back into run queue*/
	mutex->locked = 0;
	tcb *thread;
	int level = getHighestLevel();
	while(blockList[0] != NULL){
		thread = dequeue(blockList[0]);
		thread->status = READY;
		enqueue(runQueue[level], thread);
	}

	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return 0;
};


/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in my_pthread_mutex_init
	//free(mutex);
//	ret = sigprocmask(SIG_BLOCK, &set, NULL);

	mutex = NULL;

//	ret = sigprocmask(SIG_UNBLOCK, &set, NULL);
	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library 
	// should be contexted switched from thread context to this 
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (STCF or MLFQ)

	//if (sched == STCF)
	//	sched_stcf();
	//else if (sched == MLFQ)
	//	sched_mlfq();

	// YOUR CODE HERE
	/*Check to see if queue is completely empty*/
	int i;
	for(i = 7; i >= 0; i++){
		if(runQueue[i] != NULL) break;
	}
	/*Empty queue, return to main*/
	if(runQueue[i] == NULL) setcontext(&Main);
// schedule policy
#ifndef MLFQ
	// Choose STCF
	void sched_stcf();
#else 
	// Choose MLFQ
	void sched_mlfq();
#endif
}

/* Preemptive SJF (STCF) scheduling algorithm */
void sched_stcf(){
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	int level = getHighestLevel();
	tcb *old = dequeue(runQueue[level]);
	/*Only one thead in queue*/
	if(runQueue[level]->end == NULL){
		old->status = RUNNING;
		++old->priority;
		old->context.uc_link = NULL;
		setcontext(&old->context);
		return;
	}
	/*Link threads to return to their predecessor*/
	runQueue[level]->end->context.uc_link = &old->context;
	old->context.uc_link = (old->next == NULL) ? NULL : &old->next->context;
	
	/*Run the new thread with the lowest and increment priority*/
	runQueue[level]->end->status = WAITING;
	++runQueue[level]->end->priority;
	old->status = READY;
	enqueue(runQueue[level], old);
	swapcontext(&old->context, &runQueue[level]->end->context);
}

/* Preemptive MLFQ scheduling algorithm */
void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	int level = getHighestLevel();
	tcb *old = dequeue(runQueue[level]);
	/*Only one thread in queue*/
	if(runQueue[level]->end == NULL){
		old->status = RUNNING;
		if(old->priority >= 1) --old->priority;
		old->context.uc_link = NULL;
		setcontext(&old->context);
		return;
	}
	/*Link threads to their predecessor*/
	runQueue[level]->end->context.uc_link = &old->context;
	old->context.uc_link = (old->next == NULL) ? NULL : &old->next->context;

	/*Run the highest priority thread and decrement it priority*/
	runQueue[level]->end->status = RUNNING;
	if(runQueue[level]->end->priority >= 1) --runQueue[level]->end->priority;
	old->status = READY;
	enqueue(runQueue[level], old);
	swapcontext(&old->context, &runQueue[level]->end->context);
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
	tcb *ptr = q->front;
	/*Check the first tcb*/
	if(ptr->priority < t->priority){
		tcb *temp = ptr;
		q->front = t;
		q->front->next = temp;
		return;
	}
	/*Loop to find entry point*/
	while(ptr->next != NULL && ptr->next->priority > t->priority){
		ptr = ptr->next;
	}
	t->next = ptr->next;
	ptr->next = t;
	/*Newest thread becomes end of list*/
//	q[level]->end->next = t;
//	q[level]->end = t;
//	t->next = NULL;
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

static void switchToScheduler(int signum, siginfo_t *info, void *old){
//	sigprocmask(SIG_BLOCK, &set, NULL);

	/*Create scheduler context*/
	getcontext(&schedContext);
	schedContext.uc_stack.ss_sp = malloc(32768);
	schedContext.uc_stack.ss_size = 32768;
	makecontext(&schedContext, (void*)&schedule, 0);
	swapcontext((ucontext_t*)old, &schedContext);

//	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return;
}

int getHighestLevel(){
	int i = 7;
	while(runQueue[i] == NULL){
		i--;
	}
	return i;
}

