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
Queue **blockList, **doneList;
ucontext_t schedContext, Main;
sigset_t set;
struct sigaction sig;
struct itimerval timer;
static int counter = 0;
static volatile int first = 1;
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
		blockList = (Queue**)malloc(sizeof(Queue*));
		blockList[0] = (Queue*)malloc(sizeof(Queue));
		blockList[0]->front = NULL;
		blockList[0]->end = NULL;
		doneList = (Queue**)malloc(sizeof(Queue*));
		doneList[0] = (Queue*)malloc(sizeof(Queue));
		doneList[0]->front = NULL;
		doneList[0]->end = NULL;
		int i;
		for(i = 0; i < 8; i++){
			runQueue[i] = (Queue*)malloc(sizeof(Queue));
			runQueue[i]->front = NULL;
			runQueue[i]->end = NULL;
		}
		getcontext(&schedContext);
		schedContext.uc_stack.ss_sp = malloc(SIGSTKSZ);
		schedContext.uc_stack.ss_size = SIGSTKSZ;
		schedContext.uc_link = &Main;
		makecontext(&schedContext, (void*)&switchToScheduler, 0);

		getcontext(&Main);
		Main.uc_link = 0;

		/*Set up sigaction and sigmask*/
		memset(&sig, 0, sizeof(sig));
		sig.sa_sigaction = &switchToScheduler;
		sig.sa_flags = SA_SIGINFO;
		sigemptyset(&sig.sa_mask);
		sigaction(SIGALRM, &sig, NULL);

		sigemptyset(&set); 
		sigaddset(&set, SIGALRM);

		/*Set timer to go off every 50 ms*/
		timer.it_value.tv_usec = 5000;
		timer.it_value.tv_sec = 0;
		timer.it_interval.tv_usec = 5000;
		timer.it_interval.tv_usec = 0;
		setitimer(ITIMER_REAL, &timer, NULL);
	}
	sigprocmask(SIG_BLOCK, &set, NULL);

	/*Create and initialize TCB*/
	tcb *TCB = (tcb*)malloc(sizeof(tcb));
	if(TCB == NULL) return -1;
	TCB->tid = ++counter;
	(*thread) = counter;
	TCB->status = READY;
	TCB->priority = 7;
	TCB->context = (ucontext_t*)malloc(sizeof(ucontext_t));
	
	/*Initialize context and allocate stack space(32KB)*/
	if(getcontext(TCB->context) < 0){
		printf("Cannot allocate context\n");
		free(TCB->context);
		free(TCB);
		int i;
		for(i = 0; i < 8; i++){
			free(runQueue[i]);
		}
		free(runQueue);
		exit(EXIT_FAILURE);
	}
	TCB->context->uc_stack.ss_sp = malloc(SIGSTKSZ);
	TCB->context->uc_stack.ss_size = SIGSTKSZ;
	TCB->context->uc_flags = 0;	
	TCB->context->uc_link = &schedContext;
	makecontext(TCB->context, (void*)&function, 1, arg);
	priorityEnqueue(runQueue[TCB->priority], TCB);

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
	tcb *yielder;
	/*Give it the lowest priority and push it back into run queue*/
#ifndef MLFQ
	yielder = dequeue(runQueue[7]);
	if(yielder == NULL) return -1;
	if(runQueue[7] == NULL) enqueue(runQueue[7], yielder);
	else {
		yielder->priority = runQueue[7]->front->priority;
		enqueue(runQueue[7], yielder);
	}
	
#else
	int level = getHighestLevel();
	if(level < 0) return -1;
	yielder = dequeue(runQueue[level]);
	if(yielder == NULL) return -1;
	yielder->priority = 0;
	enqueue(runQueue[0], yielder);
#endif
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	/*Switch to scheduler*/
	yielder = NULL;
	raise(SIGALRM);
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
	
	// YOUR CODE HERE
	sigprocmask(SIG_BLOCK, &set, NULL);

	int level = getHighestLevel();
	tcb *target = dequeue(runQueue[level]);
	free(target);
	target = NULL;	

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
	printf("Joining %d\n", thread);
	int level = getHighestLevel();
	tcb *current = runQueue[level]->end;
	if(current == NULL) return -1;
	/*Check if thread has already finished*/
	if(searchQueue(doneList[0], thread) == 0){
		while(doneList[0]->end->tid != thread){
			enqueue(doneList[0], dequeue(doneList[0]));
		}
		current = dequeue(doneList[0]);
		free(current->context.uc_stack.ss_sp);
		free(current->context);
		free(current);
		current = NULL;
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		return 0;
	}
	current = dequeue(runQueue[level]);
	sigprocmask(SIG_UNBLOCK. &set, NULL);
	/*Check if thread is currently blocked*/
	while(searchQueue(blockList[0], thread) == 0){
		/*Currently blocked*/
	}
	sigprocmask(SIG_BLOCK, &set, NULL);
	while(runQueue[level] != NULL){ //thread became unblocked, run scheduler until target is found
		//current = runQueue[level]->end;	
		if(searchQueue(doneList[0], thread) == 0){
			printf("gothere\n");
			free(current->context.uc_stack.ss_sp);
			free(current->context);
			free(current);
			return;
		}	
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		swapcontext(current->context, &schedContext);
		sigprocmask(SIG_BLOCK, &set, NULL);
		if(runQueue[level] == NULL) level = getHighestLevel();
		if(level == -1) {
			sigprocmask(SIG_UNBLOCK, &set, NULL);
			return -1;
		}
	}
	/*Check runQueue*/
	if(searchQueue(blockList[0], thread) < 0){		
		while(runQueue[level] != NULL){
			current = runQueue[level]->end;
			/*Wait for the thread to be ran*/
			if(searchQueue(doneList[0], thread) == 0){
				printf("Current: %d Thread: %d\n", current->tid, thread);
				break;
			}
			sigprocmask(SIG_UNBLOCK, &set, NULL);
			swapcontext(current->context, &schedContext);
			sigprocmask(SIG_BLOCK, &set, NULL);
			if(runQueue[level] == NULL) level = getHighestLevel();
			if(level == -1){
				sigprocmask(SIG_UNBLOCK, &set, NULL);
				return -1;
			}
		}
	}
	
	while(doneList[0]->end->tid != thread){
		enqueue(doneList[0], dequeue(doneList[0]));
	}
	tcb *finished = dequeue(doneList[0]);
	free(finished);
	finished = NULL;
	printf("Successfully joined %d\n", thread);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return 0;
};

/* initialize the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	// Initialize data structures for this mutex

	// YOUR CODE HERE
	sigprocmask(SIG_BLOCK, &set, NULL);

	printf("Initializing mutex\n");
	mutex->locked = 0;
	printf("Mutex %p has value %d\n", mutex, mutex->locked);

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
	if(mutex->locked == 0){
		sigprocmask(SIG_BLOCK, &set, NULL);
		printf("Locking mutex %p\n", mutex);
		mutex->locked = 1;
		return 0;
	}else{
		printf("Mutex is locked %p\n", mutex);
		/*Critical section closed, pushing thread into block list*/
		int level = getHighestLevel();
		tcb *thread = dequeue(runQueue[level]);
		thread->status = BLOCKED;
		enqueue(blockList[0], thread);
		sigprocmask(SIG_UNBLOCK, &set, NULL);
		raise(SIGALRM);
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
		priorityEnqueue(runQueue[level], thread);
	}
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	return 0;
};


/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in my_pthread_mutex_init
	printf("DESTROYING MUTEX %p\n", mutex);

	mutex->locked = 0;	
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

	//if (sched == STCF)
	//	sched_stcf();
	//else if (sched == MLFQ)
	//	sched_mlfq();

	// YOUR CODE HERE
// schedule policy
#ifndef MLFQ
	// Choose STCF
//	printf("Executing stcf\n");
	void sched_stcf();
	sched_stcf();
#else 
	// Choose MLFQ
//	printf("Executing mlfq\n");
	void sched_mlfq();
	sched_mlfq();
#endif
}

/* Preemptive SJF (STCF) scheduling algorithm */
void sched_stcf(){
	// Your own implementation of STCF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	printf("In STCF\n");
	sigprocmask(SIG_BLOCK, &set, NULL);
	tcb *oldctx, *newctx;
	while(runQueue[7] != NULL){
		oldctx = runQueue[7]->end;
		if(first){
			++oldctx->priority;
			oldctx->status = RUNNING;
			first = 0;
			swapcontext(&schedContext, oldctx->context);
			getcontext(oldctx->context);
			oldctx->status = STOPPED;
			return;
		}
		if(runQueue[7]->front == oldctx){ //only one thread in list
			swapcontext(&schedContext, oldctx->context);
			sigprocmask(SIG_UNBLOCK, &set, NULL);
			oldctx->status = STOPPED;
			return;
		}
		oldctx = dequeue(runQueue[7]);
		if(oldctx->status == STOPPED){
			enqueue(doneList[0], oldctx);
		}
		else if(oldctx->status == RUNNING){
			oldctx->status = READY;
		}
		newctx = runQueue[7]->end;
		if(newctx->status == STOPPED){
			newctx = dequeue(runQueue[7]);
			enqueue(doneList[0], newctx);
		}
		else if(newctx->status == RUNNING){
			newctx->status = READY;
		}
		else if(newctx->status == READY){
			priorityEnqueue(runQueue[7], oldctx);
			break;
		}
	}
	++newctx->priority;
	newctx->status = RUNNING;
	printf("Swapping to %d from %d\n", newctx->tid, oldctx->tid);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	swapcontext(oldctx->context, newctx->context);
	getcontext(newctx->context);
	newctx->status = STOPPED;
	/*
	tcb *newctx;
	while(runQueue[7] != NULL){
		newctx = runQueue[7]->end;
		if(newctx->status == STOPPED){ //Move finished threads out
			newctx = dequeue(runQueue[7]);
			enqueue(doneList[0], newctx);
		}
		else if(newctx->status == RUNNING){
			newctx->status = READY;
			newctx = dequeue(runQueue[7]);
			priorityEnqueue(runQueue[7], newctx);
		}
		else if(newctx->status == READY) break;
	}
	++newctx->priority;	
	newctx->status = RUNNING;
	printf("Swapping to %d\n", newctx->tid);
	
	sigprocmask(SIG_UNBLOCK, &set, NULL);	
	swapcontext(&schedContext, &newctx->context);
	printf("Finished running %d\n", newctx->tid);
	newctx->status = STOPPED;
	*/
}


/* Preemptive MLFQ scheduling algorithm */
void sched_mlfq() {
	// Your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	 
}

// Feel free to add any other functions you need

// YOUR CODE HERE
void priorityEnqueue(Queue *q, tcb *t){
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
	t->next = ptr->next;
	ptr->next = t;
	return;
}
void enqueue(Queue *q, tcb *t){
	/*Empty queue*/
	if(q->front == NULL){
		q->front = t;
		q->end = t;
		t->next = NULL;
		return;
	}
	q->end->next = t;
	q->end = t;
	t->next = NULL;
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
	printf("Switching to scheduler\n");

	/*Finished all threads*/
	int level = getHighestLevel();
	if(level == -1 && blockList[0] == NULL){
		schedContext.uc_link = &Main;
		setcontext(&schedContext);	
	}
	schedule();

	//if(newctx == NULL) setcontext(&schedContext);
	//else swapcontext(&newctx->context, &schedContext);
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
	/*Look for thread*/
	tcb *ptr = q->front;
	while(ptr->next != NULL){
		if(ptr->tid == tid) return 0;
		ptr = ptr->next;
	}
	return 0;
}
