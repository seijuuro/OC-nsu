#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>

int threads_count;
int exec = 1;
int k = 0;
pthread_barrier_t barrier;
pthread_barrier_t barrier2;

struct threadData
{
        int number;
        double partial_sum;
};

void* pi_calculation(void* thread_data)
{
        long iter = 0;
        while(1)
        {
                for(long long i = iter*1000000 + ((struct threadData*)thread_data)->number; i < (iter+1)*1000000; i+= threads_count)
                {
                        ((struct threadData*)thread_data)->partial_sum += 1.0/(i*4.0 + 1.0);
                        ((struct threadData*)thread_data)->partial_sum -= 1.0/(i*4.0 + 3.0);
                }

                pthread_barrier_wait(&barrier);
                if(exec == 0)
                {
                        printf("Local pi %.15g, iter: %d\n", 4 * ((struct threadData*)thread_data)->partial_sum, iter);
                        pthread_exit((void*)1);
                }
                else
                        iter++;

                pthread_barrier_wait(&barrier2);

        }
}

void stopPI(int signal)
{
        printf("\n");
        exec = 0;
}

int main(int argc, char** argv)
{

        double pi = 0;
        int err;
        pthread_t* threads;
        struct threadData* thread_data;

        signal(SIGINT, stopPI);

        int flag = isdigit(*argv[1]);
        if(!flag)
        {
                printf("error: arg isnot a digit\n");
                return -1;
        }
        threads_count = atoi(argv[1]);
        if(threads_count < 1)
        {
                printf("error: threads count < 1");
                return -1;
        }

        thread_data = malloc(sizeof(struct threadData) * threads_count);
        threads = malloc(sizeof(pthread_t) * threads_count);


        pthread_barrier_init(&barrier, NULL, threads_count);

        for(int i = 0; i < threads_count; i++)
        {
                thread_data[i].number = i;
                thread_data[i].partial_sum = 0;
                err = pthread_create(&threads[i], NULL, &pi_calculation, (void*)(&thread_data[i]));
                if(err)
                {
                        printf("couldnt create new thread");
                        return -1;
                }
        }

        //pthread_barrier_init(&barrier, NULL, threads_count);
        pthread_barrier_init(&barrier2, NULL, threads_count);
        for(int i = 0; i < threads_count; i++)
        {
                pthread_join(threads[i], NULL);
                pi += thread_data[i].partial_sum;
        }

        pi = pi * 4.0;
        printf("pi done - %.15g \n", pi);

        pthread_barrier_destroy(&barrier);
        pthread_barrier_destroy(&barrier2);
        free(thread_data);
        free(threads);

        return 0;
}