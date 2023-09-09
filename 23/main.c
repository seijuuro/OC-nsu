#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define STRING_SIZE 100
#define factor 10000            // 0.01 c
pthread_mutex_t mutex;


typedef struct listNode
{
    char* str;
    struct listNode* next;
} node_t;

node_t* head;
node_t* head_p;

void addNode(char* string)
{
    if(head == NULL)
    {
        head = (node_t*)malloc(sizeof(node_t));
        head->str = (char*)calloc(STRING_SIZE, sizeof(char));
        strcpy(head->str, string);
        head->next = NULL;
        head_p = head;
        return;
    }
    node_t* cur = head;
    node_t* tmp = (node_t *)malloc(sizeof(node_t));
    tmp->str = (char*)calloc(STRING_SIZE, sizeof(char));


    strcpy(tmp->str, string);
    tmp->next = NULL;

    cur->next = tmp;
    head = tmp;
}

void printList(node_t* head)
{
    node_t* cur = head;
    while (cur != NULL)
    {
        printf("%s", cur->str);
        cur = cur->next;
    }
}

void removeList(node_t* head)
{
        while(head != NULL)
        {
                node_t* tmp = head;
                head = head->next;
                free(tmp);
        }
}

void* waiter(void* arg)
{
        char* str = (char*)arg;

        usleep(strlen(str) * factor);

        pthread_mutex_lock(&mutex);

        addNode(str);

        pthread_mutex_unlock(&mutex);

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

        pthread_mutex_init(&mutex, NULL);
        threads = malloc(sizeof(pthread_t) * n);
        str = malloc(sizeof(char*) * n);

        for(int i = 0; i < n; i++)
        {
                str[i] = malloc(sizeof(char) * STRING_SIZE);
        }

        for(int i = 0; i < n; i++)
        {
                if(fgets(str[i], STRING_SIZE, stdin) == NULL)
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

        printList(head_p);

        removeList(head_p);
        for(int i = 0; i < n; i++)
        {
                free(str[i]);
        }
        free(str);
        free(threads);
        pthread_mutex_destroy(&mutex);

        return 0;
}