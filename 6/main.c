#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define factor 10000// 0.01 c

void* waiter(void* arg)
{
        char* str = (char*)arg;

        usleep(strlen(str) * factor);

        printf("%s", str);

        pthread_exit((void*)1);
}

int main(int argc, char **argv)
{
        int n;
        int err;
        char** str;
        pthread_t* threads;

        scanf("%d", &n);
        n++;

        threads = malloc(sizeof(pthread_t) * n);
        str = malloc(sizeof(char*) * n);
        for(int i = 0; i < n; i++)
                str[i] = malloc(sizeof(char) * 100);

        for(int i = 0; i < n; i++)
        {
                if(fgets(str[i], 100, stdin) == NULL)
                        return -1;
        }

        printf("\n");

        for(int i = 0; i < n; i++)
        {
                err = pthread_create(&threads[i], NULL, &waiter, (void*)str[i]);
                if(err != 0)
                        return -1;
        }

        for(int i = 0; i < n; i++)
        {
                err = pthread_join(threads[i], NULL);
                if(err != 0)
                        return -1;
        }

        free(threads);
        free(str);
        return 0;
}