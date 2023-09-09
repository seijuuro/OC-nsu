#include <pthread.h>
#include <stdio.h>
#include <unistd.h>


void* task()
{
        while(1)
        {
                printf("task is running\n");
                pthread_testcancel();
        }

        pthread_exit((void*)1)
}

int main()
{
        pthread_t thread;
        int err;
        void* status;

        err = pthread_create(&thread, NULL, &task, NULL);
        if(err != 0)
                return err;

        sleep(2);

        err = pthread_cancel(thread);
        if(err != 0)
                return err;

        err = pthread_join(thread, &status);
        if(err != 0)
                return err;

        if(status == PTHREAD_CANCELED)
                printf("\nprogramm ended correctly\n\n");

        return 0;
}