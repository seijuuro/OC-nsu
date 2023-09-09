#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <locale.h>
#include <signal.h>
#include <pthread.h>

#define PORT 12345
#define CACHE_SIZE 30
#define BUFFER_SIZE 1024
#define ADDRESS_SIZE 256
#define MAX_CLIENTS 200
#define TIMEOUT 1
#define EMPTY -1

//http://lib.pushkinskijdom.ru/Default.aspx?tabid=2018
//http://fit.ippolitov.me/CN_2/2022/list.html


typedef struct cache
{
    int page_size;
    char* title;
    char* page;
    int run;
}cache_t;

pthread_t* clientThreads;
cache_t * cache;
int clientCache[MAX_CLIENTS];
int cacheSize = 0;
int connectedClients = 0;
int end = 0;

pthread_mutex_t cacheIndexMutex;
pthread_mutex_t readCacheMutex;
pthread_cond_t cond;

typedef struct threadArg
{
    int clientFD;
    int clientIndex;
}threadArg_t;

typedef struct url
{
    char* host;
    char* path;
    int port;
}url_t;

int findFreeIndex(pthread_t* client_tids)
{
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
                if(client_tids[i] == EMPTY)
                        return i;
        }

        return -1;
}

int tryAcceptClient(int socket, int* client, int connectedClients) 
{
        if(connectedClients < MAX_CLIENTS)
        {
		if((*client = accept(socket, (struct sockaddr*)NULL, NULL)) == -1)
			return -1;
			
		connectedClients++;
        }
        return connectedClients;
}

int tryFindAtCache(int* clientCache, int index, char* url) 
{
        for(int i = 0; i < cacheSize; i++)
        {
                if(strcmp(cache[i].title, url) == 0)
                {
                        clientCache[index] = i;
                        printf("\nPage found at cache");
                        return 1;
                }
        }

        return 0;
}

void freeURL(url_t* url) 
{
        free(url->host);
        free(url->path);
        free(url);
}

void clientDisconnect(int* clientHttp, int client, int index) 
{
        printf("client %d disconnecting...\n", index);
         
        clientThreads[index] = EMPTY;
        
        close(*clientHttp);
        clientCache[index] = EMPTY;
}

url_t* parseURL(char* urlBuff) 
{
	char* GET = strstr(urlBuff, "GET");
	if(GET == NULL)
		return NULL;
		
        url_t* url = (url_t*)malloc(sizeof(url_t));
        url->path = NULL;
        url->host = NULL;
        url->port = 80;

        int size = strlen(urlBuff);
	int startPortIndex = 0;
	char* buff = strstr(urlBuff, "http://");
	if(buff == NULL)
	{
		free(url);
		return NULL;
	}
	buff+=strlen("http://");
	
        for(int i = 0; i < size; i++)
        {
                if(buff[i] == ':')
                {

                        char port[6] ;
                        startPortIndex = i;
                        int portIndex = i + 1;
                        int k = 0;
                        while(portIndex < size && isdigit(buff[portIndex]))
                        {
                                port[k++] = buff[portIndex];
                                portIndex++;
                        }
                        url->port = atoi(port);
                }

                if(buff[i] == '/')
                {
			url->host = (char *) malloc((startPortIndex ? startPortIndex + 1 : i + 1) * sizeof(char));
			strncpy(url->host, buff, startPortIndex ? startPortIndex : i);
			int k = i;
			while(buff[k] != ' ' && k < size)
				k++;
			url->path = (char *) malloc((k - i) * sizeof(char));
			strncpy(url->path, &(buff[i+1]), k - i - 1);
			
			url->host[startPortIndex ? startPortIndex : i] = '\0';
			url->path[k - i - 1] = '\0';
			
			printf("host = %s\n", url->host);
			printf("path = %s\n", url->path);
			break;
                }
        }
        return url;
}

int urlConnect(char* host, in_port_t port) 
{
	printf("getting host...\n"); 
	char port_[10];
	sprintf(port_, "%d", port);
	
	struct addrinfo hints, *res, *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_protocol = 0;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags |= AI_CANONNAME;

	printf("\n\nportik %s\n", port_);
	printf("host %s\n\n", host);
	int err = getaddrinfo(host, port_,  &hints, &result);
	if (err != 0) 
	{
		printf("getaddrinfo error -_-\n ");	
		return -1;
	}
	res = result;
	
	int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	while (res != NULL) 
	{
		if (connect(sock, result->ai_addr, result->ai_addrlen) == 0) 
			return sock;
		
		res = res->ai_next;
	}
	
	printf("couldnt connect to any host\n");
	return -1;
}

char *createRequest(const url_t *url) 
{
    char* request = (char*)malloc(sizeof(char) * ADDRESS_SIZE);
    strcpy(request, "GET http://");
    strcat(request, url->host);
    strcat(request, "/");
    strcat(request, url->path);
    strcat(request, " HTTP/1.0\r\n");
    strcat(request, "Host: ");
    strcat(request, url->host);
    strcat(request, "\r\n\r\n");
    return request;
}

void sendRequest(int clientHttp, url_t* url) 
{
        if(url->path == NULL)
                write(clientHttp, "GET /\r\n\r\n", strlen("GET /\r\n\r\n"));
        else
        {
                char* request = createRequest(url);
                printf("[DEBUG]: REQUEST: %s", request);
                
                write(clientHttp, request, strlen(request));
                free(request);
        }
}

int dataRead(cache_t* cache, int clientHttp, int client)  
{
    int offset = 0;
    int read_bytes = 0;
    if(cacheSize >= CACHE_SIZE)
    {
	printf("CACHE SPACE IS OVER\n");
	char reserveBuf[BUFFER_SIZE];
	while((read_bytes = read(clientHttp, reserveBuf, BUFFER_SIZE)) > 0) 
	{
		offset += read_bytes;
                printf("[DEBUG]: read_bytes = %d. offset = %d\n", read_bytes, offset);
                if(read_bytes == EMPTY)
                {
                        printf("EMPTY SOCKET\n");
                        return -1;
                }
		write(client, reserveBuf, read_bytes);
	}
    }
    else
    {
 	cache[cacheSize-1].run = 1;
    	while((read_bytes = read(clientHttp, &((cache[cacheSize-1].page)[offset]), BUFFER_SIZE)) != 0) 
	{
		//pthread_mutex_lock(&readCacheMutex);
		
        	offset += read_bytes;
       	printf("[DEBUG]: cache[%d].page size = %d\n", cacheSize-1 , cache[cacheSize-1].page_size);
	        printf("[DEBUG]: read_bytes = %d. offset = %d\n", read_bytes, offset);
		if(read_bytes == EMPTY)
		{
			printf("EMPTY SOCKET\n");
			return -1;
		}
        	cache[cacheSize-1].page_size = offset;
        	cache[cacheSize-1].page = (char*)realloc(cache[cacheSize-1].page, offset + BUFFER_SIZE + 1);
        	usleep(15000);
        	
        	pthread_cond_broadcast(&cond);
        	
        	//pthread_mutex_unlock(&readCacheMutex);
    	}

	cache[cacheSize-1].run = 2;
    	write(client, cache[cacheSize-1].page, offset);
    }
    return 1;
}

int getFromCache(int* connectedClients, int client, int* clientCache, int index)
{	
	pthread_mutex_lock(&readCacheMutex);
	int written_bytes = 0;
	if(clientCache[index] != EMPTY) 
	{
		int offset = 0;
		while(1)
		{
			int diff = cache[clientCache[index]].page_size - offset;
			if((written_bytes = write(client, cache[clientCache[index]].page + offset, BUFFER_SIZE < diff ? BUFFER_SIZE : diff)) > 0 )
			{
				offset += written_bytes;
				printf("\n---------GET FROM CACHE, CACHE INDEX %d, SIZE %d, PAGE SIZE %d------------: \n\n", clientCache[index], offset, cache[clientCache[index]].page_size);
				if(cache[clientCache[index]].run == 1)
				{
					pthread_cond_wait(&cond, &readCacheMutex);
				}
			}
			else 
			{
				if(written_bytes == EMPTY)
				{
					printf("[DEBUG]: Client disconnected!\n");
					clientCache[index] = EMPTY;
					client = EMPTY;
					connectedClients--;	
					break;	
				}
				else 
				{
					if(written_bytes == 0)
					{
						if(cache[clientCache[index]].run == 2)
						{
							printf("END\n");
							break;
						}
					}
				}
			}
			
			
		}
	}
	pthread_mutex_unlock(&readCacheMutex);
        return *connectedClients;
}

void* clientHandler(void* arg)
{
	threadArg_t* threadArg = (threadArg_t*) arg;
	int client = threadArg->clientFD;
	int index = threadArg->clientIndex;
	free(arg);
	printf("[DEBUG]: new client thread created for %d\n", client);
	
	char buff[ADDRESS_SIZE];
	int readBytes;
	int clientHttp = EMPTY;
	
	
	while(1)
	{
		readBytes = read(client, &buff, ADDRESS_SIZE);
		if(readBytes < 1)
		{
			printf("-------readBytes < 1\n");
			clientDisconnect(&clientHttp, client, index);
			break;
		}
		else
		if(readBytes)
		{
			buff[readBytes] = '\0';
			//printf("\n\n%s\n\n", buff);
			url_t* url = parseURL(buff);
			if(url == NULL)
			{
				printf("[DEBUG]: URL parsing fail\n");
				clientDisconnect(&clientHttp, client, index);
				break;
			}
			printf("PORT=%d\n", url->port);
			printf("clientIndex = %d\n", index);
						
			char* address = (char*) malloc(sizeof(char) * (strlen(url->path) + strlen(url->host) + 1));
			strcpy(address, url->host);
			strcat(address, "/");
			strcat(address, url->path);
			
			if(cacheSize < CACHE_SIZE)
			{
				//pthread_mutex_lock(&cacheIndexMutex);
				//cache[cacheSize].title = (char*) malloc(sizeof(char) * strlen(address));
				strcpy(cache[cacheSize].title, address);
			}
			
			printf("CACHE SIZE: %d", cacheSize);
			int find = tryFindAtCache(clientCache, index, address);
			if(!find)
			{
				pthread_mutex_lock(&cacheIndexMutex);
				cacheSize++;
				clientHttp = urlConnect(url->host, url->port);
				printf("CLIENTHTTP %d", clientHttp);
				if(clientHttp == EMPTY)
				{
					freeURL(url);
					cacheSize--;
					free(cache[cacheSize-1].title);
					pthread_mutex_unlock(&cacheIndexMutex);
					break;
				}

				printf("[DEBUG]: Socket connected.\n");
				sendRequest(clientHttp, url);
				
				
				if(cacheSize-1 < CACHE_SIZE)
				{	
					printf("Some INFO\n");

					
					
					
					
					free(address);

					cache[cacheSize-1].page_size = BUFFER_SIZE;
					cache[cacheSize-1].page = (char*) malloc((BUFFER_SIZE + 1) * sizeof(char));

					clientCache[index] = cacheSize-1;
				}
				if(dataRead(cache, clientHttp, client) == EMPTY)
				{
					if(cache[cacheSize-1].title != NULL)
						free(cache[cacheSize-1].title);
					if(cache[cacheSize-1].page != NULL)
						free(cache[cacheSize-1].page);
					printf("READING ERROR");
					clientDisconnect(&clientHttp, client, index);
					pthread_mutex_unlock(&cacheIndexMutex);
					break;
				}
				
				close(clientHttp);
                                                      
                               if(cacheSize-1 < CACHE_SIZE)
                               {
					cache[cacheSize-1].page[cache[cacheSize-1].page_size + 1] = '\0'; 
					printf("[DEBUG]: data write at cache!\n");
					//pthread_mutex_unlock(&cacheIndexMutex);
				}

				pthread_mutex_unlock(&cacheIndexMutex);
			}
			else
			{
				printf("SOMETHING NEW\n");
				connectedClients = getFromCache(&connectedClients, client, clientCache, index);
			}			
			freeURL(url);				
		}
		
	}
	
	printf("pthread end\n");
	pthread_exit((void*)1);
}


int main()
{
	pthread_mutex_init(&cacheIndexMutex, NULL);
	pthread_mutex_init(&readCacheMutex, NULL);
	pthread_cond_init(&cond, NULL);
	clientThreads = (pthread_t*)calloc(MAX_CLIENTS, sizeof(pthread_t));

	setlocale(LC_ALL,"Russian");
	int socket_d = 0;
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	cache = (cache_t*)malloc(sizeof(cache_t) * CACHE_SIZE);

	socket_d = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(socket_d, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		clientCache[i] = EMPTY;
		clientThreads[i] = EMPTY;
	}
	for(int i = 0; i < CACHE_SIZE; i++)
	{
		cache[i].title = (char*) malloc(sizeof(char) * 300);
		cache[i].run = 0;
	}
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	bind(socket_d, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(socket_d, MAX_CLIENTS);

	int client = EMPTY;
	int k, freeIndex;
	while(1)
	{
			if((k = tryAcceptClient(socket_d, &client, connectedClients)) == -1)
			{
				printf("Accept error\n");
			}
			else
			{
				printf("new accept\n");	
				if((freeIndex = findFreeIndex(clientThreads)) == -1)
				{
					printf("Too much clients\n");
					sleep(5);
				}
				
				connectedClients = k;
				
				threadArg_t* threadArg = malloc(sizeof(threadArg_t));
				threadArg->clientFD = client;
				threadArg->clientIndex = freeIndex;

				int err = pthread_create(&(clientThreads[freeIndex]), NULL, clientHandler, threadArg);
				if(err)
				{
					printf("Cant create new thread\n");
					continue;
				}
				printf("Thread created %d\n", freeIndex);
			}
	}


	return 0;
}
