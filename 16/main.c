#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdlib.h>


int main()
{
    pid_t pid = 0;
    int flags = O_CREAT;
    mode_t mode = S_IRUSR | S_IWUSR;

    sem_t* sem1 = sem_open("/sem1", flags, mode , 0);
    sem_t* sem2 = sem_open("/sem2", flags, mode , 1);

    if(sem1 == SEM_FAILED)
        return -2;

    if(sem2 == SEM_FAILED)
        return -2;

    switch ((pid = fork()))
    {
    case -1:
        printf("Error fork");
        return -1;
    case 0:
        for (int i = 1; i <= 10; ++i)
        {
            sem_wait(sem1);
            printf("Thread - %d\n", i);
            sem_post(sem2);
        }
        break;
    default:
        for (int i = 1; i <= 10; ++i)
        {
           sem_wait(sem2);
           printf("Main - %d\n", i);
           sem_post(sem1);
        }
        sem_close(sem1);
        sem_close(sem2);
        break;
    }
}