#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>

sem_t sem1;
sem_t sem2;

int init()
{
        int err;

        err = sem_init(&sem1, 0, 1);
        if(err != 0)
                return err;
        err = sem_init(&sem2, 0, 1);
        if(err != 0)
                return err;

        return 0;
}

int destroy()
{
        int err;

        err = sem_destroy(&sem1);
        if(err != 0)
                return err;
        err = sem_destroy(&sem2);
        if(err != 0)
                return err;

        return 0;
}

void* print_strings()
{
        size_t tid;
        tid = pthread_self();

        for(int i = 0; i < 10; i++)
        {
                sem_wait(&sem1);

                printf("Thread id: %d\n", tid);

                sem_post(&sem2);
        }

        pthread_exit( (void*)1);
}

int main()
{
        pthread_t thread;
        int err;
        size_t tid = pthread_self();

        err = init();
        if(err != 0)
                return err;

        sem_wait(&sem1);
        sem_wait(&sem2);

        err = pthread_create(&thread, NULL, print_strings, NULL);
        if(err != 0)
                return err;

        for(int i = 0; i < 10; i++)
        {
                printf("Thread id: %d\n", tid);

                sem_post(&sem1);

                sem_wait(&sem2);
        }

        err = pthread_join(thread, NULL);
        if(err != 0)
                return err;

        err = destroy();
        if(err != 0)
                return err;

        return 0;
}