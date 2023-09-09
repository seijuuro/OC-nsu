#include <pthread.h>
#include <stdio.h>

pthread_mutex_t mutex1;
pthread_mutex_t mutex2;
pthread_mutex_t mutex3;

int init(pthread_mutexattr_t attr)
{
        int err;

        err = pthread_mutexattr_init(&attr);
        if(err != 0)
                return err;
        err = pthread_mutex_init(&mutex1, &attr);
        if(err != 0)
                return err;
        err = pthread_mutex_init(&mutex2, &attr);
        if(err != 0)
                return err;
        err = pthread_mutex_init(&mutex3, &attr);
        if(err != 0)
                return err;

        return 0;
}

int destroy()
{
        int err;

        err = pthread_mutex_destroy(&mutex1);
        if(err != 0)
                return err;
        err = pthread_mutex_destroy(&mutex3);
        if(err != 0)
                return err;
        err = pthread_mutex_destroy(&mutex2);
        if(err != 0)
                return err;

        return 0;
}

void* print_strings()
{
        size_t tid;
        tid = pthread_self();

        pthread_mutex_lock(&mutex2);

        for(int i = 0; i < 10; i++)
        {
                pthread_mutex_lock(&mutex1);
                printf("Thread id: %d\n", tid);

                pthread_mutex_unlock(&mutex2);
                pthread_mutex_lock(&mutex3);

                pthread_mutex_unlock(&mutex1);
                pthread_mutex_lock(&mutex2);
                pthread_mutex_unlock(&mutex3);
        }

        pthread_mutex_unlock(&mutex2);

        pthread_exit( (void*)1);
}

int main()
{
        pthread_t thread;
        int err;
        pthread_mutexattr_t attr;
        size_t tid = pthread_self();

        err = init(attr);
        if(err != 0)
                return err;

        err = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        if(err != 0)
                return err;

        pthread_mutex_lock(&mutex1);
        //pthread_mutex_lock(&mutex2);

        err = pthread_create(&thread, NULL, print_strings, NULL);
        if(err != 0)
                return err;

        for(int i = 0; i < 10; i++)
        {
                printf("Thread id: %d\n", tid);

                pthread_mutex_lock(&mutex3);
                pthread_mutex_unlock(&mutex1);
                pthread_mutex_lock(&mutex2);

                pthread_mutex_unlock(&mutex3);
                pthread_mutex_lock(&mutex1);
                pthread_mutex_unlock(&mutex2);
        }

        pthread_mutex_unlock(&mutex1);
        //pthread_mutex_unlock(&mutex2);

        err = pthread_join(thread, NULL);
        if(err != 0)
                return err;

        err = destroy();
        if(err != 0)
                return err;

        return 0;
}