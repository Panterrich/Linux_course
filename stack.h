#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <time.h>
#include <sys/shm.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#define POISON (void*)(1000 - 7) 

#define SEMOP() do {if (semop(stack->semid, &sem_buf, 1) == -1) perror("ERROR: semop");} while(0)

//=================================================================================================
typedef struct timespec timespec;

typedef struct
{
    void* shm_buffer;
    int   shmid;
    int   semid;

    int wait_val;
    timespec timeout;

    key_t key;

} stack_t;

/* Attach (create if needed) shared memory stack to the process.
Returns stack_t* in case of success. Returns NULL on failure. */
stack_t* attach_stack(key_t key, int size);

/* Detaches existing stack from process. 
Operations on detached stack are not permitted since stack pointer becomes invalid. */
int detach_stack(stack_t* stack);

/* Marks stack to be destroed. Destruction are done after all detaches */ 
int mark_destruct(stack_t* stack);

/* Returns stack maximum size. */
int get_size(stack_t* stack);

/* Returns current stack size. */
int get_count(stack_t* stack);

/* Push val into stack. */
int push(stack_t* stack, void* val);

/* Pop val from stack into memory */
int pop(stack_t* stack, void** val);

//---------------------------------------------
/* Additional tasks */

/* Control timeout on push and pop operations in case stack is full or empty.
val == -1 Operations return immediatly, probably with errors.
val == 0  Operations wait infinitely.
val == 1  Operations wait timeout time.
*/
int set_wait(stack_t* stack, int val, timespec* timeout);

//===========================================================================

void* stack_get_shm(key_t key, size_t size, int* shmid_ptr);

int stack_get_sem(key_t key, size_t size);

int stack_rm_sem(stack_t* stack);

int stack_dump(stack_t* stack);

int get_empty(stack_t* stack);