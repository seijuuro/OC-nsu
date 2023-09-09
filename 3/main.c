#include <pthread.h>
#include <string.h>
#include <stdio.h>

#define THREADS_COUNT 4

void* print_strings(void* arg)
{
        char** strings = (char**)arg;

        for(int i = 0; i < 2; i++)
                printf("%s ", strings[i]);

        printf("\n");

        pthread_exit((void*)1)
}

int main()
{
        int err;
        pthread_t threads[THREADS_COUNT];
        char* strings[THREADS_COUNT][2] = {
                {"1", "2"},
                {"3", "4"},
                {"5", "6"},
                {"7", "8"}
        };

        for(int i = 0; i < THREADS_COUNT; i++)
        {
                err = pthread_create(&threads[i], NULL, &print_strings, strings[i]);
                if(err != 0)
                        return err;
        }

        for(int i = 0; i < THREADS_COUNT; i++)
        {
                err = pthread_join(threads[i], NULL);
                if(err != 0)
                        return err;
        }

        return 0;
}