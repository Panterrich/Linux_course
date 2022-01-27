#include "stack.h"

FILE* log_file;

stack_t* attach_stack(key_t key, int size)
{   
    if (size <= 0) return NULL;

    int shmid = 0;
    void* buffer = stack_get_shm(key, size, &shmid);

    struct sembuf sem_buf = {.sem_num = 4, .sem_op = 1, .sem_flg = 0};

    int semid = stack_get_sem(key, size);
    if (semid == -1) return NULL;

    
    if (buffer == NULL) 
    {   
        if (semop(semid, &sem_buf, 1) == -1) perror("ERROR: semop");
        return NULL;
    }
    
    stack_t* stack = calloc(1, sizeof(stack_t));

    stack->semid      = semid;
    stack->shmid      = shmid;
    stack->shm_buffer = buffer;

    stack->wait_val        = -1;
    stack->timeout.tv_sec  = 0;
    stack->timeout.tv_nsec = 0;

    stack->key = key;

    if (semop(semid, &sem_buf, 1) == -1) perror("ERROR: semop");

    return stack;
}

void* stack_get_shm(key_t key, size_t size, int* shmid_ptr)
{   
    assert(shmid_ptr);

    void* buffer      = NULL;
    size_t size_stack = size * sizeof(void*);

    int shmid = shmget(key, size_stack, IPC_CREAT | IPC_EXCL | 0666);

    if (shmid == -1 && errno == EEXIST)
    {
        shmid = shmget(key, size_stack, 0666);
        if (shmid == -1)
        {
            perror("ERROR: shmget");
            return NULL;
        }

        buffer = shmat(shmid, NULL, 0);
        if (buffer == (void*)-1)
        {
            perror("ERROR: shmat");
            return NULL;
        }
    }

    else if (shmid == -1 && errno != EEXIST)
    {
        perror("ERROR: shmget");
        return NULL;
    }

    else
    {
        buffer = shmat(shmid, NULL, 0);
        if (buffer == (void*)-1)
        {
            perror("ERROR: shmat");
            return NULL;
        }

        for (int i = 0; i < size; ++i)
        {
            *((void**)((char*)buffer + sizeof(void*) * i)) = POISON;
        }
    }

    *shmid_ptr = shmid;
    return buffer;
}

int detach_stack(stack_t* stack)
{
    assert(stack);
    sleep(1);
    struct sembuf sem_buf = {.sem_num = 4, .sem_op = -1, .sem_flg = 0};
    SEMOP();
    printf("SEM %d\n", semctl(stack->semid, 4, GETVAL));
    sleep(1);
    int result = 0;
    
    struct shmid_ds info = {};
    if (shmctl(stack->shmid, IPC_STAT, &info) == -1)
    {
        perror("ERROR: shmctl(info)");
    }

    if (info.shm_nattch == 1)
    {
        result = stack_rm_sem(stack);
    }
    else
    {
        sem_buf.sem_op = 1;
        SEMOP();
    }


    result = shmdt(stack->shm_buffer);
    if (result == -1)
    {
        perror("ERROR: shmdt");
    }

    stack->semid = 0;
    stack->shmid = 0;
    stack->shm_buffer = NULL;

    free(stack);
    return result;
}

int mark_destruct(stack_t* stack)
{
    assert(stack);

    int result = shmctl(stack->shmid, IPC_RMID, NULL);

    if (result == -1)
    {
        perror("ERROR: shmctl(IPC_RMID)");
    }
    
    return result;
}

int get_size(stack_t* stack)
{
    assert(stack);

    int size = semctl(stack->semid, 3, GETVAL);
    if (size == -1)
    {
        perror("ERROR: semop(GETVAL SIZE)");
    }

    return size;
}

int get_count(stack_t* stack)
{
    assert(stack);

    int count = semctl(stack->semid, 1, GETVAL);
    if (count == -1)
    {
        perror("ERROR: semop(GETVAL COUNT)");
    }

    return count;
}

int get_empty(stack_t* stack)
{
    assert(stack);

    int empty = semctl(stack->semid, 2, GETVAL);
    if (empty == -1)
    {
        perror("ERROR: semop(GETVAL EMPTY)");
    }

    return empty;
}

int push(stack_t* stack, void* val)
{
    assert(stack);
    
   
    struct sembuf sem_buf = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
    SEMOP();

    if (get_empty(stack) == 0)
    {
        switch (stack->wait_val)
        {
            case -1:
            {
                sem_buf.sem_op = 1;
                SEMOP();
                return -1;
            }
            break;

            case 0:
            {
                sem_buf.sem_op = 1;
                SEMOP();

                sem_buf.sem_num = 2;
                sem_buf.sem_op  = -1;
                SEMOP();

                sem_buf.sem_num = 0;
                sem_buf.sem_op  = -1;
                SEMOP();
                
                *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack))) = val;

                sem_buf.sem_num = 1;
                sem_buf.sem_op  = 1;
                SEMOP();

                sem_buf.sem_num = 0;
                sem_buf.sem_op  = 1;
                SEMOP();
                return 0;
            }
            break;
            
            case 1:
            {
                sem_buf.sem_op = 1;
                SEMOP();

                nanosleep(&stack->timeout, NULL);

                sem_buf.sem_op = -1;
                SEMOP();

                if (get_empty(stack) == 0) 
                {   
                    sem_buf.sem_op = 1;
                    SEMOP();
                    return -1;
                }

                sem_buf.sem_op  = -1;
                SEMOP();

                *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack))) = val;
                
                sem_buf.sem_num = 2;
                sem_buf.sem_op  = -1;
                SEMOP();

                sem_buf.sem_num = 1;
                sem_buf.sem_op  = 1;
                SEMOP();

                sem_buf.sem_num = 0;
                sem_buf.sem_op  = 1;
                SEMOP();

                return 0;
            }

            default:
                perror("ERROR: undefined wait_val");
                break;
        }

        return -1;
    }
    
    *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack))) = val;
     
    sem_buf.sem_num = 1;
    sem_buf.sem_op  = 1;
    SEMOP();

    sem_buf.sem_num = 2;
    sem_buf.sem_op  = -1;
    SEMOP();
    
    sem_buf.sem_num = 0;
    sem_buf.sem_op  = 1;
    SEMOP();

    return 0;
}

int pop(stack_t* stack, void** val)
{
    assert(stack);

    struct sembuf sem_buf = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
    SEMOP();

    if (get_count(stack) == 0) 
    {   
        switch (stack->wait_val)
        {
            case -1:
            {
                sem_buf.sem_op = 1;
                SEMOP();
                return -1;
            }
            break;

            case 0:
            {
                sem_buf.sem_op = 1;
                SEMOP();

                sem_buf.sem_num = 1;
                sem_buf.sem_op  = -1;
                SEMOP();

                sem_buf.sem_num = 0;
                sem_buf.sem_op  = -1;
                SEMOP();
                
                sem_buf.sem_num = 2;
                sem_buf.sem_op  = 1;
                SEMOP();

                *val = *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack)));
                       *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack))) = POISON;

                sem_buf.sem_num = 0;
                sem_buf.sem_op  = 1;
                SEMOP();
                return 0;
            }
            break;
            
            case 1:
            {
                sem_buf.sem_op = 1;
                SEMOP();

                nanosleep(&stack->timeout, NULL);

                sem_buf.sem_op = -1;
                SEMOP();

                if (get_count(stack) == 0) 
                {   
                    sem_buf.sem_op = 1;
                    SEMOP();
                    return -1;
                }

                sem_buf.sem_op  = -1;
                SEMOP();

                sem_buf.sem_num = 1;
                sem_buf.sem_op  = -1;
                SEMOP();

                sem_buf.sem_num = 2;
                sem_buf.sem_op  = 1;
                SEMOP();

                *val = *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack)));
                       *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack))) = POISON;
                
                sem_buf.sem_num = 0;
                sem_buf.sem_op  = 1;
                SEMOP();

                return 0;
            }

            default:
                perror("ERROR: undefined wait_val");
                break;
        }
        return -1;
    }

    sem_buf.sem_num = 1;
    sem_buf.sem_op  = -1;
    SEMOP();

    sem_buf.sem_num = 2;
    sem_buf.sem_op  = 1;
    SEMOP();


    *val = *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack)));
           *((void**)((char*)stack->shm_buffer + sizeof(void*) * get_count(stack))) = POISON;

    sem_buf.sem_num = 0;
    sem_buf.sem_op  = 1;
    SEMOP();

    return 0;
} 

int stack_get_sem(key_t key, size_t size)
{
    int semid = semget(key, 5, IPC_CREAT | IPC_EXCL | 0666);

    if (semid == -1 && errno == EEXIST)
    {
        semid = semget(key, 5, 0);
        struct sembuf sem_buf = {.sem_num = 4, .sem_op = -1, .sem_flg = 0};
        if (semop(semid, &sem_buf, 1) == -1) perror("ERROR: semop");
    }

    else if (semid == -1 && errno != EEXIST)
    {
        perror("ERROR: semget");
        return -1;
    }

    else 
    {
        semid = semget(key, 5, IPC_CREAT | 0666);
        if (semctl(semid, 0, SETVAL, 1)    == -1) perror("ERROR: semctl(0)");
        if (semctl(semid, 1, SETVAL, 0)    == -1) perror("ERROR: semctl(1)");
        if (semctl(semid, 2, SETVAL, size) == -1) perror("ERROR: semctl(2)");
        if (semctl(semid, 3, SETVAL, size) == -1) perror("ERROR: semctl(3)");
        if (semctl(semid, 4, SETVAL, 1)    == -1) perror("ERROR: semctl(4)");

        struct sembuf sem_buf = {.sem_num = 4, .sem_op = -1, .sem_flg = 0};
        if (semop(semid, &sem_buf, 1) == -1) perror("ERROR: semop");
    }

    return semid;
}

int stack_rm_sem(stack_t* stack)
{
    assert(stack);
    
    if (semctl(stack->semid, 0, IPC_RMID) == -1)
    {
        perror("ERROR: semctl(IPC_RMID)");
        return -1;
    }

    return 0;
}

int stack_dump(stack_t* stack)
{
    assert(stack);

    struct sembuf sem_buf = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
    SEMOP();
    
    if (log_file)
    {
        fprintf(log_file, "SHARED STACK\n");
        fprintf(log_file, "SIZE: %d\n", get_size(stack));
        fprintf(log_file, "COUNT: %d\n", get_count(stack));

        for (int i = 0; i < get_count(stack); ++i)
        {       
            if (*((void**)((char*)stack->shm_buffer + sizeof(void*) * i)) == POISON)
            {
                fprintf(log_file, "*[%d] = %p (POISON!!!)\n", i, *((void**)((char*)stack->shm_buffer+ sizeof(void*) * i)));
            }
            else
            {
                fprintf(log_file, "*[%d] = %p\n", i, *((void**)((char*)stack->shm_buffer + sizeof(void*) * i)));
            }
        }

        for (int i = get_count(stack); i < get_size(stack); ++i)
        {
            if (*((void**)((char*)stack->shm_buffer + sizeof(void*) * i)) == POISON)
            {
                fprintf(log_file, "[%d] = %p (POISON!!!)\n", i, *((void**)((char*)stack->shm_buffer + sizeof(void*) * i)));
            }
            else
            {
                fprintf(log_file, "[%d] = %p\n", i, *((void**)((char*)stack->shm_buffer + sizeof(void*) * i)));
            }
        }

        fprintf(log_file, "\n\n");
    }

    sem_buf.sem_op = 1;
    SEMOP();

    return 0;
}

int set_wait(stack_t* stack, int val, timespec* timeout)
{
    assert(stack);
    
    struct sembuf sem_buf = {.sem_num = 0, .sem_op = -1, .sem_flg = 0};
    SEMOP();

    if (val > 1 || val < -1) return -1;

    stack->wait_val = val;
    if (val == 1)
    {
        if (!timeout) return -1;
        stack->timeout = *timeout;
    }

    sem_buf.sem_op = 1;
    SEMOP();

    return 0;
}