#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <locale.h>
#include <signal.h>
#include <unistd.h>

#define STRING_SIZE 80

typedef struct listNode
{
    char* str;
    struct listNode* next;
} node_t;

node_t* head;
pthread_mutex_t listMutex;

node_t* addStringNode(node_t* head, char* string)
{
    node_t* cur = head;
    size_t strSize = strlen(string);

    node_t* newHead = (node_t *)malloc(sizeof(node_t));
    newHead->str = (char *)calloc(STRING_SIZE + 1, sizeof(char));
    strncpy(newHead->str, string, strSize );
    if(newHead->str[strlen(newHead->str) - 1] != '\n')
        strcat(newHead->str, "\n");
    newHead->next = cur;

    return newHead;
}

void printList(node_t * head)
{
    node_t* cur = head;
    while (cur != NULL)
    {
        printf("%s", cur->str);
        cur = cur->next;
    }
}


void sort(node_t * head)
{
    if (head == NULL)
    {
        return;
    }
    node_t* edge = head;
    node_t* right = head->next;
    //node_t* tmp = (node_t *)malloc(sizeof(node_t));
    //tmp->str = (char *)calloc(STRING_SIZE + 1, sizeof(char));
    while(edge->next != NULL)
    {
            node_t* left = edge;
            while(right != NULL)
            {
                    if (strncmp(left->str, right->str, STRING_SIZE) > 0)
                    {
                        char* temp = right->str;
                        right->str = left->str;
                        left->str = temp;
                    }
                    right = right->next;
                    left = left->next;
            }
            edge = edge->next;
            right = edge->next;
    }
    //free(tmp->str);
    //free(tmp);
}


void* sortThread(void * param)
{
    while (1)
    {
        sleep(5);
        pthread_mutex_lock(&listMutex);
        printf("Sorting...\n");
        sort(head);
        pthread_mutex_unlock(&listMutex);
    }
}

int main()
{
    setlocale(LC_ALL, "Russian");
    pthread_t thread;
    pthread_mutex_init(&listMutex, NULL);
    int err = pthread_create(&thread, NULL, sortThread, NULL);
    if (err)
        return -1;

    char string[STRING_SIZE + 1];
    while(1)
    {
        fgets(string, STRING_SIZE + 1, stdin);
        pthread_mutex_lock(&listMutex);
        if (!strcmp("\n", string))
        {
            printf("\nLIST:\n--------------------------\n");
            printList(head);
            printf("--------------------------\n\n");
        }
        else
            head = addStringNode(head, string);

        pthread_mutex_unlock(&listMutex);
    }

    return 0;
}