#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"



/* This is the wait queue structure */
struct wait_queue {
    struct QueueNode* head;/* ... Fill this in Lab 3 ... */
};

enum{
    RUNNING = 0,
    READY = 1,
    EXITCODE = 2,
    EXITED = 3,
    
};

/* This is the thread control block */
struct thread {
    Tid id;
    ucontext_t context;
    
    //an integer status code that describes which mode thread is in:
    //0)Running
    //1)Ready
    //2)Blocked
    //3)Exited
    int status;
    
    char* startAStack;
    char* endAStack;
    //a signifier that describes whether a thread is in the first or second
    //stage of get_context 0 = first stage, 1 = second stage
    int context_set;
    int killed;
    
    Tid waiting_for;
    struct wait_queue* myQueue;
    
    int firstInQueue;
    int exitCode;
    
    int holdingLocks[THREAD_MAX_THREADS];
    struct thread* nextThread;
};

//A table of all the threads that currently exist.
//Note that this is the only place where thread structures are stored
struct thread* threadTable[THREAD_MAX_THREADS];
struct lock* lockTable[THREAD_MAX_THREADS];

struct QueueNode{
    Tid id;
    struct QueueNode* nextNode;
};
struct Queue{
    struct QueueNode* head;
};

struct QueueNode* queues[4]; 

void addToQueue(Tid threadId, int queueType){
    struct QueueNode* newNode = (struct QueueNode*)malloc(sizeof(struct QueueNode));
    newNode->id = threadId;
    newNode->nextNode = NULL;
    if(queues[queueType]==NULL){
        queues[queueType] = newNode;
        newNode = NULL;
    }else{
        struct QueueNode* ptr = queues[queueType];
        while(ptr->nextNode != NULL){
            ptr = ptr->nextNode;
        }
        ptr->nextNode = newNode;
        newNode = NULL;
    }
   return;
}

int deleteFromQueue(Tid threadId, int queueType){
    //Queue is empty so what you told me to delete doesn't exist
    if(queues[queueType] == NULL){
        return 0;
    //Queue has one item in it and it's not what you're looking for    
    }else if (queues[queueType]->nextNode == NULL && queues[queueType]->id != threadId){
        return 0;
    //First item in the queue is what you're looking for so I'll delete that.    
    }else if(queues[queueType]->id == threadId){
        struct QueueNode* tmpptr = queues[queueType];
        queues[queueType] = queues[queueType]->nextNode;
        tmpptr->nextNode = NULL;
        free(tmpptr);
        
        return 1;
    //What you're looking for is somewhere deep in the queue, let me find it    
    }else{
        struct QueueNode* prevptr = queues[queueType];
        while(prevptr->nextNode != NULL){
            //found the item, gonna delete it
            if(prevptr->nextNode->id == threadId){
                struct QueueNode* tmpptr = prevptr->nextNode;
                prevptr->nextNode = prevptr->nextNode->nextNode;
                tmpptr->nextNode = NULL;
                prevptr = NULL;
                free(tmpptr);
                return 1;
            }
            prevptr = prevptr->nextNode;
        }
        //What you where looking for isn't anywhere in this queue, returning
        prevptr = NULL;
        return 0;
    }
}

void printQueue(struct QueueNode* head){
    struct QueueNode* tmpptr = head;
    while(tmpptr != NULL){
        printf("%d ->", tmpptr->id);
        tmpptr = tmpptr->nextNode;
    }
    printf("\n");
    return;
}

void
thread_init(void)
{
    //initializing threads database
       for(int i = 0; i<THREAD_MAX_THREADS; i++){
            threadTable[i] = NULL;
            lockTable[i] = NULL;
        }
       
    
    //initializing RUNNING, READY, and 
       for(int i = 0; i<4; i++){
           queues[i] = NULL;
       }
       
    //Create and put the main thread into the running queue   
       struct QueueNode* mainNode = (struct QueueNode*)malloc(sizeof(struct QueueNode));
       mainNode->id = 0;
       mainNode->nextNode = NULL;
       queues[0] = mainNode;
       
       struct thread* mainThread = (struct thread*)malloc(sizeof(struct thread));
       
    //Initialize main thread structure and place it at threadsTable[0]   
       mainThread->nextThread = NULL;
       mainThread->id = 0;
       mainThread->context_set = 0;
       mainThread->killed = 0;
       mainThread->status = RUNNING;
       mainThread->myQueue = NULL;
       mainThread->waiting_for = -1;
       mainThread->firstInQueue = 0;
       mainThread->exitCode = -1;
       for(int i = 0; i < THREAD_MAX_THREADS; i++){
           mainThread->holdingLocks[i] = -1;
       }
       void* stackptr = malloc(THREAD_MIN_STACK);
       mainThread->context.uc_stack.ss_sp = stackptr;
       mainThread->context.uc_stack.ss_size = THREAD_MIN_STACK - 8;
       mainThread->context.uc_mcontext.gregs[REG_RSP] = (long long int)stackptr+THREAD_MIN_STACK - 8;
       mainThread->startAStack = stackptr;
       
       threadTable[0] = mainThread;
       mainThread = NULL;
	/* your optional code here */
      
}

void removeFromWaitQueue(Tid threadId){
    Tid queueId = threadTable[threadId]->waiting_for; 
    //First item in the queue is what you're looking for so I'll delete that.    
    if(threadTable[queueId]->myQueue->head->id == threadId){
        struct QueueNode* tmpptr = threadTable[queueId]->myQueue->head;
        threadTable[queueId]->myQueue->head = threadTable[queueId]->myQueue->head->nextNode;
        tmpptr->nextNode = NULL;
        free(tmpptr);
        threadTable[threadId]->waiting_for = -1;
        addToQueue(threadId, READY);
        return;
    //What you're looking for is somewhere deep in the queue, let me find it    
    }else{
        struct QueueNode* prevptr = threadTable[queueId]->myQueue->head;
        while(prevptr->nextNode != NULL){
            //found the item, gonna delete it
            if(prevptr->nextNode->id == threadId){
                struct QueueNode* tmpptr = prevptr->nextNode;
                prevptr->nextNode = prevptr->nextNode->nextNode;
                tmpptr->nextNode = NULL;
                prevptr = NULL;
                free(tmpptr);
                threadTable[threadId]->waiting_for = -1;
                addToQueue(threadId, READY);
                return;
            }
            prevptr = prevptr->nextNode;
        }
        //What you where looking for isn't anywhere in this queue, returning
        prevptr = NULL;
        threadTable[threadId]->waiting_for = -1;
        addToQueue(threadId, READY);
        return;
    }
}

//Check the head of the RUNNING queue and return the head, which should always been the only element in that code
//needs DEFENSIVE?
Tid
thread_id()
{
    return queues[RUNNING]->id;
}

//Switch the context of current to next. The second return of get context is tracked by context_set
//This function does not edit the queues that the threads are placed in
void switchContext(Tid current, Tid next){
    getcontext(&threadTable[current]->context);
    
    if(threadTable[current]->context_set == 0){
        threadTable[current]->context_set = 1;
        setcontext(&threadTable[next]->context);
    }else{
        threadTable[current]->context_set = 0;
        return;
    }
}

/* New thread starts by calling thread_stub. The arguments to thread_stub are
 * the thread_main() function, and one argument to the thread_main() function. 
 */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
    //since a thread was created with interrupts off, we need to turn them back on when thread runs for the first time
    interrupts_set(1);    
    thread_main(arg); // call thread_main() function with arg
        thread_exit(0);
}


Tid
thread_create(void (*fn) (void *), void *parg)
{   
        interrupts_off();
        struct QueueNode* toDeleteHead = queues[EXITED];
        
        //delete anything still sitting in the exit queue to free up some space
        while(toDeleteHead != NULL){
            if(toDeleteHead->id == thread_id()){
                exit(0);
            }
            free(threadTable[toDeleteHead->id]->startAStack);
            free(threadTable[toDeleteHead->id]);
            threadTable[toDeleteHead->id] = NULL;
            Tid deletedId = toDeleteHead->id;
            toDeleteHead = NULL;
            deleteFromQueue(deletedId, EXITED);
            toDeleteHead = queues[EXITED];
        }
        int i = 0;
        //look for an available thread id
        while(threadTable[i] != NULL){
            i++;
             //if you reach the end of the threadTable, you don't have any available thread ids to provide, return error code
            if(i == THREAD_MAX_THREADS){
                interrupts_set(1);
                return THREAD_NOMORE;
            }
        }
        

        Tid id = i;
    
       struct thread* newThread = (struct thread*)malloc(sizeof(struct thread));
       
       getcontext(&newThread->context);
       newThread->nextThread = NULL;
       newThread->id = id;
       newThread->context_set = 0;
       newThread->killed = 0;
       newThread->status = READY;
       newThread->myQueue = NULL;
       newThread->waiting_for = -1;
       newThread->firstInQueue = 0;
       newThread->exitCode = -1;
       for(int i = 0; i < THREAD_MAX_THREADS; i++){
           newThread->holdingLocks[i] = -1;
       }
       void* stackptr = malloc(THREAD_MIN_STACK);
       if(stackptr == NULL){
           free(newThread);
           interrupts_set(1);
           return THREAD_NOMEMORY;
       }
       newThread->context.uc_stack.ss_sp = stackptr;
       newThread->context.uc_stack.ss_size = THREAD_MIN_STACK - 8;
       newThread->context.uc_mcontext.gregs[REG_RIP] = (long long int)&thread_stub;
       newThread->context.uc_mcontext.gregs[REG_RDI] = (long long int)fn;
       newThread->context.uc_mcontext.gregs[REG_RSI] = (long long int)parg;
       newThread->context.uc_mcontext.gregs[REG_RSP] = (long long int)stackptr+THREAD_MIN_STACK - 8;
       newThread->startAStack = stackptr;
       threadTable[id] = newThread;
       newThread = NULL;
       
       addToQueue(id, READY);
      
       interrupts_on();
        return id;
}


Tid
thread_yield(Tid want_tid)
{
    interrupts_off();
    //if the wanted thread is yourself, then there no need to change the queue, just switch context 
    if(want_tid == THREAD_SELF){
        Tid current = thread_id();
        Tid next = thread_id();
        switchContext(current, next);
        if(threadTable[current]->killed == 1){
            thread_exit(9);
        }
        interrupts_set(1);
        return(next);
    }else if(want_tid == THREAD_ANY){
        Tid current = thread_id();
        if(threadTable[current]->status == EXITED){
             if(queues[READY]==NULL){
                 interrupts_set(1);
                return THREAD_NONE;
            }
        
            Tid next = queues[READY]->id;

            deleteFromQueue(current, RUNNING);
            deleteFromQueue(next, READY);
            addToQueue(current, EXITED);
            addToQueue(next, RUNNING);
            switchContext(current, next);
            if(threadTable[current]->killed == 1){
                thread_exit(9);
            }
            interrupts_set(1);
            return next;   
       }else if(threadTable[current]->status == EXITCODE){
            if(queues[READY]==NULL){
                 interrupts_set(1);
                return THREAD_NONE;
            }
        
            Tid next = queues[READY]->id;

            deleteFromQueue(current, RUNNING);
            deleteFromQueue(next, READY);
            addToQueue(current, EXITCODE);
            addToQueue(next, RUNNING);
            switchContext(current, next);
            if(threadTable[current]->killed == 1){
                thread_exit(9);
            }
            interrupts_set(1);
            return next;  
       }else{
            if(queues[READY]==NULL){
                interrupts_set(1);
                return THREAD_NONE;
            }
        
            Tid next = queues[READY]->id;

            deleteFromQueue(current, RUNNING);
            deleteFromQueue(next, READY);
            addToQueue(current, READY);
            addToQueue(next, RUNNING);
            switchContext(current, next);
            
            //delete anything still sitting in the exit queue to free up some space
            struct QueueNode* toDeleteHead = queues[EXITED];
            while(toDeleteHead != NULL){
                if(toDeleteHead->id == thread_id()){
                    exit(0);
                }
                free(threadTable[toDeleteHead->id]->startAStack);
                free(threadTable[toDeleteHead->id]);
                threadTable[toDeleteHead->id] = NULL;
                Tid deletedId = toDeleteHead->id;
                toDeleteHead = NULL;
                deleteFromQueue(deletedId, EXITED);
                toDeleteHead = queues[EXITED];
            }
            
            if(threadTable[current]->killed == 1){
                    thread_exit(9);
            }
            interrupts_set(1);
            return next;   
       }      
        
    }else{
        Tid current = thread_id();
        Tid next = want_tid;
        
        if(next == current){
            switchContext(current, next);
            interrupts_set(1);
            return(next);
        }else{
            int ret = deleteFromQueue(next, READY);
            if(ret == 0){
                interrupts_set(1);
                return THREAD_INVALID;
            }else{
                deleteFromQueue(current, RUNNING);
                addToQueue(current, READY);
                addToQueue(next, RUNNING);
                switchContext(current, next);
                
                 //delete anything still sitting in the exit queue to free up some space
                struct QueueNode* toDeleteHead = queues[EXITED];
                while(toDeleteHead != NULL){
                    if(toDeleteHead->id == thread_id()){
                        exit(0);
                    }
                    free(threadTable[toDeleteHead->id]->startAStack);
                    free(threadTable[toDeleteHead->id]);
                    threadTable[toDeleteHead->id] = NULL;
                    Tid deletedId = toDeleteHead->id;
                    toDeleteHead = NULL;
                    deleteFromQueue(deletedId, EXITED);
                    toDeleteHead = queues[EXITED];
                }
                
                if(threadTable[current]->killed == 1){
                    thread_exit(9);
                }
                interrupts_set(1);
                return next;
            }
        }  
    }

    interrupts_set(1);	
    return THREAD_FAILED;
}

void
thread_exit(int exit_code)
{
    interrupts_off();
    Tid currentThread = thread_id();
    
    if(threadTable[currentThread]->myQueue != NULL){
        threadTable[currentThread]->status = EXITED;
        threadTable[threadTable[currentThread]->myQueue->head->id]->exitCode = exit_code;
        thread_wakeup(threadTable[currentThread]->myQueue, 1);
        wait_queue_destroy(threadTable[currentThread]->myQueue);
    }else{
        threadTable[currentThread]->status = EXITCODE;
        threadTable[currentThread]->exitCode = exit_code;
    }
    int ret;
    ret = thread_yield(THREAD_ANY);
    if(ret == THREAD_NONE){
        interrupts_set(1);
        exit(0);
    }
    interrupts_set(1);
}

Tid
thread_kill(Tid tid)
{   
    interrupts_off();
    int same = 0;
    if( tid == thread_id()){
       same = 1;
    }
    
    if(same == 1){
        return THREAD_INVALID;
    }
    
    if(threadTable[tid]->waiting_for != -1){
        //Tid waiting_on = threadTable[tid]->waiting_for;
        removeFromWaitQueue(tid);
        addToQueue(tid, READY);
        //printQueue(threadTable[waiting_on]->myQueue->head);
    }
    
    threadTable[tid]->killed = 1;
    
    /*int ret = deleteFromQueue(tid, READY);
    if(ret == 0 || same == 1){
        interrupts_set(1);
        return THREAD_INVALID;
    }else{
        addToQueue(tid, EXITED);
    }*/
    
    interrupts_set(1);
    return tid;
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/
void addToWaitQueue(struct wait_queue* queueptr, Tid tid){
  struct QueueNode* node = (struct QueueNode* )malloc(sizeof(struct QueueNode));
  node->id = tid;
  node->nextNode = NULL;
  
  struct QueueNode* tmpptr;
  if(queueptr->head == NULL){
      queueptr->head = node;
      node = NULL;
      return;
  }else{
      tmpptr = queueptr->head;
      while(tmpptr->nextNode != NULL){
          tmpptr = tmpptr->nextNode;
      }
      tmpptr->nextNode = node;
      tmpptr = NULL;
      node = NULL;
      return;
  }
}

Tid deleteFromWaitQueue(struct wait_queue* queueptr){
    Tid tid;
    struct QueueNode* tmpptr;
    
    if(queueptr->head != NULL){
        tmpptr = queueptr->head;
        tid = queueptr->head->id;
        queueptr->head = queueptr->head->nextNode;
        free(tmpptr);
    }else{
        return -1;
    }
    
    return tid;
}
/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	wq->head = NULL;

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
    
    if(wq->head == NULL){
        free(wq->head);
        free(wq);
        return;
     }else{
        printf("Tried to delete unempty wait queue\n");
        return;
     }
}

Tid
thread_sleep(struct wait_queue *queue)
{
    interrupts_off();
    //int prevSigState = interrupts_set(0);
    
    if(queue == NULL){
        interrupts_on();
        return THREAD_INVALID;
    }
    if(queues[READY] == NULL){
        interrupts_on();
        return THREAD_NONE;
    }
    
    Tid current = thread_id();
    Tid next = queues[READY]->id;
    deleteFromQueue(current, RUNNING);
    deleteFromQueue(next, READY);
    addToWaitQueue(queue, current);
    addToQueue(next, RUNNING);
    //printf("Sleeping: ");
    //printQueue(queue->head);
    //printf("Ready: ");
    //printQueue(queues[READY]);
    
    switchContext(current, next);
    
    interrupts_on();
 
    return next;
}


/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
    int prevSigState = interrupts_set(0);
    int woken = 0;
    
    if(queue == NULL){
        interrupts_set(prevSigState);
        return woken;
    }
    if(queue->head == NULL){
        interrupts_set(prevSigState);
        return woken;
    }else{
        if(all == 0){
            Tid wakeup = queue->head->id;
            deleteFromWaitQueue(queue);
            addToQueue(wakeup, READY);
            threadTable[wakeup]->waiting_for = -1; 
            woken = 1;
            interrupts_set(prevSigState);
            return woken;
        }else{
            Tid wakeup = deleteFromWaitQueue(queue);
            while(wakeup != -1){
                addToQueue(wakeup, READY);
                threadTable[wakeup]->waiting_for = -1;
                woken++;
                wakeup = deleteFromWaitQueue(queue);
            }
            interrupts_set(prevSigState);
            return woken;
        }
    }
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid, int *exit_code)
{
    interrupts_off();
    if(tid == thread_id() || threadTable[tid] == NULL || queues[READY]==NULL){	
        interrupts_on();
        return THREAD_INVALID;
    }else if(threadTable[tid]->status == EXITCODE){
        *exit_code = threadTable[tid]->exitCode;
        deleteFromQueue(tid, EXITCODE);
        addToQueue(tid, EXITED);
        interrupts_on();
        return tid;
    }else if(threadTable[tid]->myQueue == NULL){
        threadTable[tid]->myQueue = wait_queue_create();
        Tid current = thread_id();
        threadTable[current]->firstInQueue = 1;
        threadTable[current]->waiting_for = tid;
        thread_sleep(threadTable[tid]->myQueue);
        *exit_code = threadTable[current]->exitCode;
        threadTable[current]->exitCode = -1;
        interrupts_on();
        return tid;
    }else{
        Tid current = thread_id();
        threadTable[current]->firstInQueue = 0;
        threadTable[current]->waiting_for = tid;
        thread_sleep(threadTable[tid]->myQueue);
        if(threadTable[current]->exitCode != -1){
            *exit_code = threadTable[current]->exitCode;
            interrupts_on();
            return tid;
        }else{
            interrupts_on();
            return THREAD_INVALID;
        }
    } 
}

struct lock {
    int lock_number;
    struct wait_queue* lock_queue;
    int acquirable;
};

struct lock *
lock_create()
{
    struct lock *lock;

    lock = malloc(sizeof(struct lock));
    assert(lock);

    int lockId = 0;
    while(lockTable[lockId] != NULL){
        lockId++;
    }

    lock->lock_number = lockId;
    lock->acquirable = 1;
    lock->lock_queue = wait_queue_create();
    lockTable[lockId] = lock;

    return lock;
}

void
lock_destroy(struct lock *lock)
{
    interrupts_off();
    assert(lock != NULL);
    
    if(lock->acquirable == 1 && lock->lock_queue->head == NULL){
        int destroyId = lock->lock_number;
        lockTable[destroyId] = NULL;
        wait_queue_destroy(lock->lock_queue);
        free(lock);
    }
    interrupts_on();
    return;
}

void
lock_acquire(struct lock *lock)
{
    interrupts_off();
    assert(lock != NULL);
    
    Tid current = thread_id(); 
    if(threadTable[current]->holdingLocks[lock->lock_number] == 1){
        interrupts_on();
        return;
    }
    while(lock->acquirable == 0){
        thread_sleep(lock->lock_queue);
        interrupts_off();
    }
    
    lock->acquirable = 0;
    
    threadTable[current]->holdingLocks[lock->lock_number] = 1;
    interrupts_on();
    return;
}

void
lock_release(struct lock *lock)
{
    interrupts_off();
    assert(lock != NULL);
    
    if(lock->acquirable == 0){
        Tid current = thread_id();
        if(threadTable[current]->holdingLocks[lock->lock_number] == 1){
            lock->acquirable = 1;
            threadTable[current]->holdingLocks[lock->lock_number] = -1;
            thread_wakeup(lock->lock_queue, 1);
        }
    }

    interrupts_on();
    return;
}

struct cv {
    struct wait_queue* condition_queue;
};

struct cv *
cv_create()
{
    struct cv *cv;

    cv = malloc(sizeof(struct cv));
    assert(cv);

             cv->condition_queue = wait_queue_create();

    return cv;
}

void
cv_destroy(struct cv *cv)
{
    assert(cv != NULL);

        if(cv->condition_queue->head == NULL){
            wait_queue_destroy(cv->condition_queue);
            free(cv);
        }
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
    interrupts_off();
    assert(cv != NULL);
    assert(lock != NULL);

    Tid current = thread_id();
            if(threadTable[current]->holdingLocks[lock->lock_number] == 1){
              lock_release(lock);
              thread_sleep(cv->condition_queue);
              lock_acquire(lock);
            }
            interrupts_on();
            return;
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
    interrupts_off();
    assert(cv != NULL);
    assert(lock != NULL);

    Tid current = thread_id();
    if(threadTable[current]->holdingLocks[lock->lock_number] == 1){
        thread_wakeup(cv->condition_queue, 0);
    }
        interrupts_on();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
    interrupts_off();
    assert(cv != NULL);
    assert(lock != NULL);

    Tid current = thread_id();
    if(threadTable[current]->holdingLocks[lock->lock_number] == 1){
        thread_wakeup(cv->condition_queue, 1);
    }

        interrupts_on();
}
