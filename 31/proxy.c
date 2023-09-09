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

#define PORT 1234
#define CACHE_SIZE 10
#define BUFFER_SIZE 1024
#define ADDRESS_SIZE 256
#define MAX_CLIENTS 20
#define TIMEOUT 1
#define EMPTY -1

//http://lib.pushkinskijdom.ru/Default.aspx?tabid=2018
//http://fit.ippolitov.me/CN_2/2022/list.html


typedef struct cache
{
    int page_size;
    char* title;
    char* page;
}cache_t;

typedef struct url
{
    char* host;
    char* path;
    int port;
}url_t;

int findFreeIndex(int* clients)
{
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
                if(clients[i] == EMPTY)
                        return i;
        }

        return 0;
}

int acceptClient(int socket, int* clients) 
{
        int index = findFreeIndex(clients);
        clients[index] = accept(socket, (struct sockaddr*)NULL, NULL);

        printf("\nFREE CLIENT INDEX: %d\n",index);


}

int tryAcceptClient(int socket, int* clients, int connectedClients) 
{
        if(connectedClients < MAX_CLIENTS)
        {
		acceptClient(socket, clients);
		connectedClients++;
        }
        return connectedClients;
}

int tryFindAtCache(cache_t* cache, int cacheSize, int* clientCache, int index, char* url)
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

void clientDisconnect(int* clients, int* clientsHttpSockets, int* clientCache, int index)
{
        printf("client %d disconnecting...\n", index);
        close(clients[index]);
        close(clientsHttpSockets[index]);
        clients[index] = EMPTY;
        clientsHttpSockets[index] = EMPTY;
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

	printf("host = %s\n", host);

	struct hostent *hp = gethostbyname(host);
	if(hp == NULL)
		return -1;

	printf("host got!\n");
	struct sockaddr_in addr;
	memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;

	printf("opening host...\n");

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1)
		return -1;
	
	printf("host openned!\n");

	if(connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
		return -1;
	

	return sock;
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

void sendRequest(int* clientHttpSockets, int index, url_t* url)
{
        if(url->path == NULL)
                write(clientHttpSockets[index], "GET /\r\n\r\n", strlen("GET /\r\n\r\n"));
        else
        {
                char* request = createRequest(url);
                printf("[DEBUG]: REQUEST: %s", request);
                
                write(clientHttpSockets[index], request, strlen(request));
                free(request);
        }
}

int dataRead(cache_t* cache, int cacheSize, int* clientHttpSockets, int* clients, int index)
{
    int offset = 0;
    int read_bytes = 0;
    if(cacheSize >= CACHE_SIZE)
    {
	printf("CACHE SPACE IS OVER\n");
	char reserveBuf[BUFFER_SIZE];
	while((read_bytes = read(clientHttpSockets[index], reserveBuf, BUFFER_SIZE)) > 0) // !=0
	{
		offset += read_bytes;
                printf("[DEBUG]: read_bytes = %d. offset = %d\n", read_bytes, offset);
                if(read_bytes == EMPTY)
                {
                        printf("EMPTY SOCKET\n");
                        return -1;
                }
		write(clients[index], reserveBuf, read_bytes);
	}
    }
    else
    {
    	while((read_bytes = read(clientHttpSockets[index], &((cache[cacheSize].page)[offset]), BUFFER_SIZE)) != 0) 
	{
        	offset += read_bytes;
       	printf("[DEBUG]: cache[%d].page size = %d\n", cacheSize , cache[cacheSize].page_size);
	        printf("[DEBUG]: read_bytes = %d. offset = %d\n", read_bytes, offset);
		if(read_bytes == EMPTY)
		{
			printf("EMPTY SOCKET\n");
			return -1;
		}
        	cache[cacheSize].page_size = offset;
        	cache[cacheSize].page = (char*)realloc(cache[cacheSize].page, offset + BUFFER_SIZE + 1);
    	}

    	write(clients[index], cache[cacheSize].page, offset);
    }
    return 1;
}

int getFromCache(cache_t* cache, int* connectedClients, int* clients, int* clientCache, int index)
{
	int written_bytes = 0;
	if(clientCache[index] != EMPTY) 
	{
		printf("\n---------GET FROM CACHE, CACHE INDEX------------: %d \n\n", clientCache[index]);
		int bytes = write(clients[index], cache[clientCache[index]].page, cache[clientCache[index]].page_size);
		if(bytes == EMPTY)
		{
			printf("[DEBUG]: Client disconnected!\n");
			clientCache[index] = EMPTY;
			clients[index] = EMPTY;
			connectedClients--;
		}
	}
        return *connectedClients;
}


int main()
{
	setlocale(LC_ALL,"Russian");
	int socket_d = 0;
	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	cache_t* cache = (cache_t*)malloc(sizeof(cache_t) * CACHE_SIZE);

	socket_d = socket(AF_INET, SOCK_STREAM, 0);

	int clients[MAX_CLIENTS];
	int clientsHttpSockets[MAX_CLIENTS];
	int clientCache[MAX_CLIENTS];

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		clients[i] = EMPTY;
		clientsHttpSockets[i] = EMPTY;
		clientCache[i] = EMPTY;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	bind(socket_d, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(socket_d, MAX_CLIENTS);

	struct timeval timeout;
	timeout.tv_sec = TIMEOUT;
	timeout.tv_usec = 0;

	fd_set set;

	int connectedClients = 0;
	int cacheSize = 0;
	while(1)
	{
			connectedClients = tryAcceptClient(socket_d, clients, connectedClients);

			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(clients[i] < 2)
					continue;

				FD_ZERO(&set);
				FD_SET(clients[i], &set);
				
				if(clientsHttpSockets[i] == EMPTY && select(clients[i] + 1, &set, NULL, NULL, &timeout))
				{
					char buff[ADDRESS_SIZE];
					int readBytes = read(clients[i], &buff, ADDRESS_SIZE);
					if(readBytes)
					{
						buff[readBytes] = '\0';
						printf("%s\n", buff);
						  
						url_t* url = parseURL(buff);
						if(url == NULL)
						{
							printf("[DEBUG]: URL parsing fail\n");
							clientDisconnect(clients, clientsHttpSockets, clientCache, i);
							continue;
						}
						printf("PORT=%d\n", url->port);
						printf("clientIndex = %d, MAX_CLIENTS = %d\n", i, MAX_CLIENTS);
						
						char* address = (char*) malloc(sizeof(char) * (strlen(url->path) + strlen(url->host) + 1));
						strcpy(address, url->host);
						strcat(address, "/");
						strcat(address, url->path);
						
						int find = tryFindAtCache(cache, cacheSize, clientCache, i, address);
						if(!find)
						{
							clientsHttpSockets[i] = urlConnect(url->host, url->port);
							if(clientsHttpSockets[i] == EMPTY)
							{
								freeURL(url);
								continue;
							}

							printf("[DEBUG]: Socket connected.\n");
							
							sendRequest(clientsHttpSockets, i, url);

							if(cacheSize < CACHE_SIZE)
							{	
								cache[cacheSize].title = (char*) malloc(sizeof(char) * strlen(address));
								strcpy(cache[cacheSize].title, address);
								free(address);

								cache[cacheSize].page_size = BUFFER_SIZE;
								cache[cacheSize].page = (char*) malloc((BUFFER_SIZE + 1) * sizeof(char));

								clientCache[i] = cacheSize;
							}
							printf("[DEBUG]: reading...\n");

							if(dataRead(cache, cacheSize, clientsHttpSockets, clients, i) == EMPTY)
							{
								free(cache[cacheSize].title);
								free(cache[cacheSize].page);
								printf("READING ERROR");
								clientDisconnect(clients, clientsHttpSockets, clientCache, i);
								continue;
							}
							
							close(clientsHttpSockets[i]);
                                                      
                                                      if(cacheSize < CACHE_SIZE)
                                                      {
								cache[cacheSize].page[cache[cacheSize].page_size + 1] = '\0';
								cacheSize++;
								printf("[DEBUG]: data write at cache!\n");
							}
						}
						else 
							connectedClients = getFromCache(cache, &connectedClients, clients, clientCache, i);

						freeURL(url);
					}
					//clientDisconnect(clients, clientsHttpSockets, clientCache, i);
				}
			}
	}


	return 0;
}
