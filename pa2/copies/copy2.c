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
Queue **runQueue;
Queue **blockList;
ucontext_t Main, schedContext;
sigset_t set;
struct sigaction sig;
struct itimerval timer;
static int counter = 0;
char schedStack[32768];

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
		schedContext.uc_stack.ss_sp = schedStack;
		schedContext.uc_stack.ss_size = sizeof(schedStack);
				
		/*Set timer to go off every 50 ms*/
		timer.it_value.tv_usec = 5000;
		timer.it_value.tv_sec = 0;
		timer.it_interval.tv_usec = 5000;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);
	}
	sigprocmask(SIG_BLOCK, &set, NULL);

	/*Create and initialize TCB*/
	ucontext_t context;
	tcb *TCB = (tcb*)malloc(sizeof(tcb));
	if(TCB == NULL) return -1;
	TCB->tid = ++counter;
	(*thread) = counter;
	TCB->status = READY;
	TCB->priority = 7;
//	TCB->context = context;
	
	/*Initialize context and allocate stack space(32KB)*/
	getcontext(&TCB->context);
	memset(&TCB->context.uc_sigmask, 0, sizeof(TCB->context.uc_sigmask)); 
	TCB->context.uc_stack.ss_sp = malloc(32768);
	TCB->context.uc_stack.ss_size = 32768;
	
	TCB->context.uc_link = &schedContext;
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
	//getcontext(&curr);
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
//	thread->context.uc_link = NULL;
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
	sigprocmask(SIG_BLOCK, &set, NULL);
	ucontext_t current;
	int level = getHighestLevel();
	printf("Joining thread %d\n", thread);
	/*Empty queue*/
	if(level == -1) {
		printf("BAD 1\n");
		return -1;
	}
/*
	tcb *current = runQueue[level]->end;
	current->status = WAITING;
//	Called join on current thread
	if(current->tid == thread){
		current->status = STOPPED;
		if(runQueue[current->priority]->front == NULL){
			int k = level;
			while(runQueue[level] == 0){
				if(k == 0){
					current->context.uc_link = NULL;
				}
				k--;
			}
			current->context.uc_link = &runQueue[k]->end->context;
		}else{
			current->context.uc_link = &runQueue[current->priority]->front->context;
		}
		dequeue(runQueue[current->priority]);
//
//		if(getcontext(&current->context) > 0) printf("error\n");
//		swapcontext(NULL, &current->context);
		
//		sigprocmask(SIG_UNBLOCK, &set, NULL);	

		printf("EXITING THREAD %d\n", thread);
		free(current->context.uc_stack.ss_sp);
		free(current);
		current = NULL;
		return 0;
	}
*/
	int i, ret;
	/*Search all levels of queue for thread*/
	for(i = level; i >= 0; i--){
		ret = searchQueue(runQueue[i], thread);
		if(ret == 0) break; //Found thread
	}
	/*Thread not found in run queue, might be blocked*/
	if(ret == -1){
		if(searchQueue(blockList[0], thread) == 0){
			/*Thread is currently blocked, allow other threads to run*/
			sigprocmask(SIG_UNBLOCK, &set, NULL);
			while(searchQueue(blockList[0], thread) == 0){
				printf("Blocked\n");
			} //Wait for thread to be unblocked
			sigprocmask(SIG_BLOCK, &set, NULL);
		}else{
			/*Thread not found*/
			sigprocmask(SIG_UNBLOCK, &set, NULL);
			printf("BAD2\n");
			return;
		}
	}
	/*Thread has been found swap to that context*/
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	tcb *target = runQueue[i]->end;
	while(runQueue[i]->end == target){
		printf("Waiting\n");
	}
	sigprocmask(SIG_BLOCK, &set, NULL);
	printf("Outside of scheduler\n");	
//	target->status = RUNNING;
//	getcontext(&target->context);
//	getcontext(&current);
//	current.uc_stack.ss_sp = malloc(32768);
//	current.uc_stack.ss_size = 32768;
//	current.uc_link = &schedContext;
//	setcontext(&target->context);
//	sigprocmask(SIG_UNBLOCK, &set, NULL);

//	swapcontext(&current, &target->context);
	
	/*Target has finished running, dequeue and cleanup*/
//	sigprocmask(SIG_BLOCK, &set, NULL);

//	searchQueue(runQueue[target->priority], target->tid);
	target->status = STOPPED;
//	target->context.uc_link = NULL;
	free(target->context.uc_stack.ss_sp);
	free(target);
	free(current.uc_stack.ss_sp);
	target = NULL;
/*
	current->status = READY;
	if(current->next == NULL){
		if(runQueue[current->priority]->front == NULL) current->context.uc_link = NULL;
		else current->context.uc_link =	&runQueue[current->priority]->front->context;
	}
*/
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return 0;
};

/* initialize the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	// Initialize data structures for this mutex

	// YOUR CODE HERE
	sigprocmask(SIG_BLOCK, &set, NULL);

	/*Allocate memory for mutex and unlock it*/
	//mutex = malloc(sizeof(my_pthread_mutex_t));
	//if(mutex == NULL) return -1;
	printf("INITIALIZIN MUTEX %p\n", mutex);
	mutex->locked = 0;
	printf("MUTEX %p HAS LOCK VALUE %d\n", mutex, mutex->locked);

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
	fflush(stdout);
	printf("LOKCKING\n");	
	fflush(stdout);
	if(mutex->locked == 0){
		printf("Locking mutex %p\n", mutex);
		mutex->locked = 1;
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		return 0;
	}else{
		printf("Mutex is locked %p\n", mutex);
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
		//getcontext(&thread->context);
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
	printf("DESTROYING MUTEX %p\t\t%p\t\t%p\n", (*mutex), mutex, &mutex);
//	free(mutex);
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
	printf("SCHEDULER IS RUNNING\n");
	sigprocmask(SIG_BLOCK, &set, NULL);
	//getcontext(&schedContext);
	int i;
	for(i = 7; i >= 0; i++){
		if(runQueue[i] != NULL) break;
	}
	/*Empty queue, return to main*/
	if(runQueue[i] == NULL){
		printf("Switchin back to main\n");
		setcontext(&Main);
	}
// schedule policy
#ifndef MLFQ
	// Choose STCF
	printf("Executing stcf\n");
	void sched_stcf(ucontext_t old);
	sched_stcf(schedContext);
#else 
	// Choose MLFQ
	printf("Executing mlfq\n");
	void sched_mlfq();
	sched_mlfq();
#endif
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}

/* Preemptive SJF (STCF) scheduling algorithm */
void sched_stcf(ucontext_t old){
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	printf("In STCF\n");
	int level = getHighestLevel();
	printf("end: %d\n", runQueue[level]->end->tid);
	//getcontext(&old);
//	tcb *new = dequeue(runQueue[level]);
//	printf("old tid: %d\n", old->tid);
//	printf("new tid: %d\n", runQueue[level]->end->tid);
	/*Only one thead in queue*/
	if(runQueue[level]->end == NULL){
		printf("BAD3\n");
		runQueue[level]->end->status = RUNNING;
		++runQueue[level]->end->priority;
		runQueue[level]->end->context.uc_link = &Main;
		swapcontext(&old, &runQueue[level]->end->context);
		return;
	}
	/*Link threads to return to their predecessor*/
//	runQueue[level]->end->context.uc_link = &old->context;
//	old->context.uc_link = (old->next == NULL) ? NULL : &old->next->context;
	
	/*Run the new thread with the lowest and increment priority*/
//	runQueue[level]->end->status = WAITING;
	++runQueue[level]->end->priority;
//	old->status = READY;
	runQueue[level]->end->status = RUNNING;
//	enqueue(runQueue[level], old);
//	printf("new end: %d\n", runQueue[level]->end->tid);
//	printf("new front: %d\n", runQueue[level]->front->tid);
//	getcontext(&schedContext);
//	schedContext.uc_link = &Main;
	//swapcontext(&schedContext, &runQueue[level]->end->context);
//	setcontext(&runQueue[level]->end->context);
	swapcontext(&old, &runQueue[level]->end->context);
	printf("Finished stcf call\n");
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
//		old->context.uc_link = NULL;
		setcontext(&old->context);
		return;
	}
	/*Link threads to their predecessor*/
//	runQueue[level]->end->context.uc_link = &old->context;
//	old->context.uc_link = (old->next == NULL) ? NULL : &old->next->context;

	/*Run the highest priority thread and decrement it priority*/
	runQueue[level]->end->status = RUNNING;
	if(runQueue[level]->end->priority >= 1) --runQueue[level]->end->priority;
	old->status = READY;
	enqueue(runQueue[level], old);
	//getcontext(&old->context);
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
	if(ptr->priority <= t->priority){
		tcb *temp = ptr;
		q->front = t;
		q->front->next = temp;
		return;
	}
	/*Loop to find entry point*/
	while(ptr->next != NULL && ptr->next->priority > t->priority){
		ptr = ptr->next;
	}
	//printf("%d\n", ptr->next->tid);
	t->next = ptr->next;
	ptr->next = t;
	/*Newest thread becomes end of list*/
//	q[level]->end->next = t;
//	q[level]->end = t;
//	t->next = NULL;
	return;
}

tcb *dequeue(Queue *q){
	tcb *ret = q->front;
	/*Empty queue*/
	if(q->front == NULL){
		return NULL;
	}
	/*One item in queue*/
	if(q->front->next == NULL){
		q->front = NULL;
		q->end = NULL;
		return ret;
	}
	/*Oldest thread is removed from end of list*/
	tcb *ptr = q->front;
	while(ptr->next->next != NULL){
		ptr = ptr->next;
	}
	ret = ptr->next;
	q->end = ptr;
	q->end->next = NULL;
//	q->front = q->front->next;
	return ret;
}

static void switchToScheduler(int signum, siginfo_t *info, void *old){
	sigprocmask(SIG_BLOCK, &set, NULL);

	printf("Switching to scheduler\n");
	schedule();
	/*Create scheduler context*/
	//getcontext(&schedContext);
	//schedContext.uc_stack.ss_sp = malloc(32768);
	//schedContext.uc_stack.ss_size = 32768;
	//schedContext.uc_link = &Main;
	//makecontext(&schedContext, (void*)&schedule, 0);
	//swapcontext((ucontext_t*)old, &schedContext);
		

	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return;
}

int getHighestLevel(){
	int i = 7;
	while(runQueue[i] == NULL){
		if(i == 0) return -1;
		i--;
	}
	return i;
}

int searchQueue(Queue *q, my_pthread_t tid){
	/*Empty queue*/
	if(q->front == NULL) return -1;
	/*Reorder the queue so the target is at q->end*/
	tcb *start = q->end; //mark the starting point
	while(q->end->tid != tid){
		enqueue(q, dequeue(q));
		/*Reached the start point without finding the thread*/
		if(q->end->tid == start->tid) return -1;
	}
	/*Loop has exited after finding the thread*/
	return 0;
}
