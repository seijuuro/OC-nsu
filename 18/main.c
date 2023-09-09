#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define STRING_SIZE 80

typedef struct listNode
{
    char* str;
    pthread_mutext_t mutex;
    struct listNode* next;
} node_t;

node_t* head;
//pthread_mutex_t listMutex;

node_t* addStringNode(node_t* head, char* string)
{
    node_t* cur = head;
    size_t strSize = strlen(string);
    if (strSize > STRING_SIZE)
    {
        cur = addStringNode(cur, string + STRING_SIZE);
    }

    node_t * newHead = (node_t *)malloc(sizeof(node_t));
    pthread_mutex_init(&(newHead->mutex), NULL);
    pthread_mutex_lock(&(newHead->mutex));
    newHead->str = (char *)calloc(STRING_SIZE + 1, sizeof(char));
    strncpy(newHead->str, string, strSize % (STRING_SIZE + 1));
    pthread_mutex_unlock(&(newHead->mutex));
    newHead->next = cur;

    return newHead;
}

void printList(node_t * head)
{
    node_t* cur = head;
    node_t* cur_copy;
    while (cur != NULL)
    {
        pthread_mutex_lock(&(cur->mutex));
        printf("%s", cur->str);
        cur_copy = cur;
        cur = cur->next;
        pthread_mutex_unlock(&(cur_copy->mutex));
    }
}


void sort(node_t * head)
{
    if (head == NULL)
    {
        return;
    }
    pthread_mutex_lock(&(head->mutex));
    node_t* left = head;
    node_t* right = head->next;
    node_t* next_left = left->next;
    node_t* right_copy = right;
    node_t* left_copy = left;
    pthread_mutex_unlock(&(head->mutex));
    node_t* tmp = (node_t *)malloc(sizeof(node_t));
    tmp->str = (char *)calloc(STRING_SIZE + 1, sizeof(char));
    pthread_mutex_init(&(tmp->mutex));
    while(next_left != NULL)
    {
            pthread_mutex_lock(&(left->mutex));
            while(right != NULL)
            {
                    pthread_mutex_lock(&(right->mutex));
                    pthread_mutex_lock(&(tmp->mutex)); //can be deleted?
                    if (strncmp(left->str, right->str, STRING_SIZE) > 0)
                    {
                        strncpy(tmp->str, left->str,  STRING_SIZE);
                        strncpy(left->str, right->str, STRING_SIZE);
                        strncpy(right->str, tmp->str, STRING_SIZE);
                    }
                    pthread_mutex_unlock(&(tmp->mutex));
                    right_copy = right;
                    right = right->next;
                    pthread_mutex_lock(&(right_copy->mutex));
            }
            left_copy = left;
            left = left->next;
            pthread_mutex_lock(&(left->mutex));
            right = left->next;
            next_left = left->next;
            pthread_mutex_unlock(&(left->mutex));
            pthread_mutex_unlock(&(left_copy->mutex));
    }
    pthread_mutex_destroy(&(tmp->mutex));
    free(tmp->str);
    free(tmp);
}


void* sortThread(void * param)
{
    while (1)
    {
        sleep(5);
        //pthread_mutex_lock(&listMutex);
        printf("Sorting...\n");
        sort(head);
        //pthread_mutex_unlock(&listMutex);
    }
}

int main()
{
    pthread_t thread;
    //pthread_mutex_init(&listMutex, NULL);
    int err = pthread_create(&thread, NULL, sortThread, NULL);
    if (err)
        return -1;

    char string[STRING_SIZE + 1];
    while(1)
    {
        fgets(string, STRING_SIZE, stdin);
        //pthread_mutex_lock(&listMutex);
        if (!strcmp("\n", string))
        {
            printf("\nLIST:\n--------------------------\n");
            printList(head);
            printf("--------------------------\n\n");
        }
        else
            head = addStringNode(head, string);

        //pthread_mutex_unlock(&listMutex);
    }

    return 0;
}