#include <pthread.h>
#include <stdio.h>

void* print_strings()
{
        size_t tid;
        tid = pthread_self();
        for(int i = 0; i < 10; i++)
                printf("Thread id: %d\n", tid);

        printf("\n");
        pthread_exit( (void*)1);
}

int main()
{
        pthread_t thread;
        int err;


        err = pthread_create(&thread, NULL, print_strings, NULL);
        if(err != 0)
        {
                return err;
        }

        err = pthread_join(thread, NULL);
        if(err != 0)
        {
                return err;
        }

        print_strings();

        return 0;
}