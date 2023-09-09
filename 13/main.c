#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t mutex;
pthread_cond_t cond;

int flag = 1;

void* printStrings(void* arg)
{
        for(int i = 1; i <= 10; ++i)
        {
                pthread_mutex_lock(&mutex);

                if(flag)
                        pthread_cond_wait(&cond, &mutex);

                printf("Child - %d\n", i);
                if(!flag)
                {
                        flag = 1;
                        pthread_mutex_unlock(&mutex);
                }
                pthread_cond_signal(&cond);

        }

        pthread_exit((void *) 0);
}

int main()
{
        pthread_t thread;
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);

        int err = pthread_create(&thread, NULL, printStrings, NULL);
        if(err)
                return err;

        for(int i = 1; i <= 10; i++)
        {
                pthread_mutex_lock(&mutex);

                if(!flag)
                        pthread_cond_wait(&cond, &mutex);

                printf("Main - %d\n", i);
                if(flag)
                {
                        flag = 0;
                        pthread_mutex_unlock(&mutex);
                }
                pthread_cond_signal(&cond);
        }

        err = pthread_join(thread, NULL);
        if(err != 0)
                return err;

        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);

        return 0;
}