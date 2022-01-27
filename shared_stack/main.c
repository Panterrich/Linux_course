#include "stack.h"

extern FILE* log_file;
int unit_test_1();

int main()
{
    key_t key = ftok("/home/panterrich/Документы/3 semestr/hw_task_4", 1000-7);

    int semid = semget(key, 4, 0666);
    if (semid == -1) {
        perror("ERROR: semget(clear)");
    }

    int res = semctl(semid, 0, IPC_RMID);
    if (res == -1) {
        perror("ERROR: semctl(clear)");
    } 

    
    size_t stack_size = 30;

    pid_t pid = 0;

    for (int i = 0; i < 3; i++)
    {
        pid = fork();
        if (pid == -1)
        {
            perror("fork() error");
            exit(EXIT_FAILURE);
        }
    }
    
    stack_t* stack = attach_stack(key, stack_size);
    if (stack == NULL)
    {
        exit(EXIT_FAILURE);
    }

    // printf("ZZZZZ\n");
    // sleep(1);
    size_t value = 21;
    push(stack, (void *) value);
    //pop(stack, (void **) &value);

    mark_destruct(stack);
    // printf("TUUT3\n");
    // sleep(2);
    detach_stack(stack);
    printf("TUUT4\n");
     // sleep(1);

    return 0;
    // return unit_test_1();
    // log_file = fopen("log.txt", "w");

    // key_t key = ftok("/home/panterrich/Документы/3 semestr/hw_task_4", 1000-7);
    
    // int semid = semget(key, 4, 0666);
    // if (semid == -1) {
    //     perror("ERROR: semget(clear)");
    // }

    // int res = semctl(semid, 0, IPC_RMID);
    // if (res == -1) {
    //     perror("ERROR: semctl(clear)");
    // }

    // stack_t* stack = attach_stack(key, 10);

    // push(stack, (void*)10);
    // push(stack, (void*)20);
    // push(stack, (void*)30);
    
    // stack_dump(stack);
    // void* result = NULL;

    // pop(stack, &result);
    // printf("%p\n", result);
    // stack_dump(stack);

    // pop(stack, &result);
    // printf("%p\n", result);
    // stack_dump(stack);

    // pop(stack, &result);
    // printf("%p\n", result);
    // stack_dump(stack);

    // pop(stack, &result);
    // printf("%p\n", result);
    // stack_dump(stack);

    // mark_destruct(stack);
    // detach_stack(stack);

    // fclose(log_file);
    return 0;
}

int unit_test_1()
{      
    log_file = fopen("log.txt", "w");
    key_t key = ftok("/home/panterrich/Документы/3 semestr/hw_task_4", 1000-7);

    stack_t* stack = attach_stack (key, 1024);
    mark_destruct (stack);
    int original = 0;

    for (int i = 0; i < 10; i++)
        original += !fork ();
    
    push (stack, (void*)getpid());
    stack_dump(stack);
    void* val = 0;
    pop (stack, &val);
    stack_dump(stack);

    if (!original)
    {
        sleep (5);
        printf ("%d\n", get_count(stack));
    }

    detach_stack (stack);
    return 0;
}