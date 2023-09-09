#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>

#define num_steps 200000000

int threads_count;

struct threadData
{
        int number;
        double partial_sum;
};

void* pi_calculation(void* thread_data)
{
        for(int i = ((struct threadData*)thread_data)->number; i < num_steps; i+= threads_count)
        {
                ((struct threadData*)thread_data)->partial_sum += 1.0/(i*4.0 + 1.0);
                ((struct threadData*)thread_data)->partial_sum -= 1.0/(i*4.0 + 3.0);
        }

        pthread_exit((void*)1);
}

int main(int argc, char** argv)
{

        double pi = 0;
        int err;
        pthread_t* threads;
        struct threadData* thread_data;

        int flag = isdigit(*argv[1]);
        if(!flag)
        {
                printf("error: arg isnot a digit\n");
                return -1;
        }
        threads_count = atoi(argv[1]);


        thread_data = malloc(sizeof(struct threadData) * threads_count);
        threads = malloc(sizeof(pthread_t) * threads_count);


        for(int i = 0; i < threads_count; i++)
        {
                thread_data[i].number = i;
                thread_data[i].partial_sum = 0;
                err = pthread_create(&threads[i], NULL, &pi_calculation, (void*)(&thread_data[i]));
        }

        for(int i = 0; i < threads_count; i++)
        {
                pthread_join(threads[i], NULL);
                pi += thread_data[i].partial_sum;
        }

        pi = pi * 4.0;
        printf("pi done - %.15g \n", pi);

        free(thread_data);
        free(threads);

        return 0;
}