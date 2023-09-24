#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define MAX(x, y) ((x) > (y) ? (x) : (y))


#define MSG_CLIENT_JOIN  (1 << 0)  // 1
#define MSG_CLIENT_PART  (1 << 1)  // 2
#define MSG_CLIENT_LIST  (1 << 2)  // 4
#define MSG_CLIENT_INFO  (1 << 3)  // 8
#define MSG_PLAYER_ID    (1 << 4)  // 16
#define MSG_PLAYER_CHAT  (1 << 5)  
#define MSG_PLAYER_VOICE (1 << 6)  
#define MSG_PLAYER_DATA  (1 << 7)  
#define MSG_PLAYER_INFO  (1 << 8)  
#define MSG_PLAYER_SHOOT (1 << 9)  
#define MSG_PLAYER_KILL  (1 << 10)  
#define MSG_PLAYER_RESPAW (1 << 11)  
#define MSG_TO_SERVER    (1 << 12) 

#define MSG_USERDATA     (1 << 100)  



typedef signed int      int24;
typedef signed long     int32;
typedef signed short    int16;
typedef signed char     int8 ;
typedef unsigned int   uint24;
typedef unsigned long  uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8 ;


typedef struct Buffer
{
    int32	size;			// Size of buffer in bytes
	int32	pos;			// Current size/pos
	uint8	*data;			// Pointer to data
    uint8   is_set;
} Buffer;


typedef struct Client
{
    int id;
    int socket;
    int ready;
    int done;
    struct Client *next;
} Client;

typedef void (*EventsCallback)(int);
typedef void (*ReadCallback)(int, int, Buffer*);



typedef struct Server
{
    Client *head;
    Client *tail;
    fd_set master;
    fd_set socket_list;
    int loop;
    int socket;
    int max_fd;
    int clients_id;
    int clients_count;
    int reading;
    int ready;
    
    EventsCallback OnCreate;
    EventsCallback OnRemove;
    ReadCallback OnRead;


} Server;


int connect_client(int connect_port, char *address);
int close_client(int socket);
int process_client(int clientSocket, Buffer *bytes);
void client_sendbuffer(int socket,Buffer *buffer);
void client_send(int socket,unsigned char* buffer, int bufferSize);

int create_server(Server *server, int listen_port);
void close_server(Server *server);
int process_server(Server *server);


void send_to(int socket, unsigned char* buffer, int bufferSize);
void sendbuffer_to(int id, Buffer *buffer);

void send_to_ignore(Server *server, uint8* buffer, int bufferSize, int ignore);
void send_all(Server *server,  uint8* buffer, int bufferSize);
void send_to_id(Server *server, int id, unsigned char* buffer, int bufferSize);

void sendbuffer_to_ignore(Server *server, Buffer *buffer, int ignore);
void sendbuffer_to_id(Server *server, int id, Buffer *buffer);
void sendbuffer_all(Server *server, Buffer *buffer);

//void send_buffer_ignore(Server )

void send_clients_list_to(Server *server,int to);
void send_request_list(int socket,int id);
void send_request_info(int socket,int id);


void seek_buffer(Buffer* buffer, int pos);
void load_buffer(Buffer* buffer,uint8 *data,int size);
void init_buffer(Buffer* buffer,int size);
void free_buffer(Buffer *buffer);


bool buffer_load(Buffer *buffer, const char* filename);
bool buffer_save(Buffer *buffer, const char* filename);


void buffer_set(Buffer *buffer, uint8 *data, int32 size);

int write_buffer(Buffer *buffer, const uint8 *data, int size) ;
int read_buffer(Buffer *buffer, uint8 *data, int size) ;
int append_buffer(Buffer *buffer, const uint8 *data, int size) ;


int write_byte(Buffer *buffer, uint8 v);
int write_short(Buffer *buffer, int v);
int write_int(Buffer *buffer, int v);
int write_float(Buffer *buffer, float v);
int write_long(Buffer *buffer, uint32 v);


uint8  read_byte(Buffer *buffer);
int    read_short(Buffer *buffer);
int    read_int(Buffer *buffer);
float  read_float(Buffer *buffer);
uint32 read_long(Buffer *buffer);


int write_string(Buffer *buffer, const char *text) ;
int read_string(Buffer *buffer, char *text) ;


#endif

#if defined(SERVER_IMPLEMENTATION)

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/select.h>
#include <string.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <assert.h>

#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define LIGHT_GREEN "\033[92m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define BLACK "\033[30m"
#define BOLD "\033[1m"
#define UNDERLINE "\033[4m"
#define ORANGE "\033[38;2;255;165;0m"
#define LIGHT_GRAY "\033[37m"
#define DARK_GRAY "\033[90m"
#define LIGHT_RED "\033[91m"


int new_connection(Server *server)
{
    socklen_t addrlen;
    struct sockaddr_in client_addr;
    int socket;
    addrlen = sizeof(client_addr);
    memset(&client_addr, 0, addrlen);
    socket = accept(server->socket, (struct sockaddr *)&client_addr, &addrlen);
    if (socket == -1)
    {
        perror("accept()");
        return -1;
    }
    FD_SET(socket, &server->socket_list);

    if (fcntl(socket, F_SETFL, O_NONBLOCK) == -1)
	{
    }
    return socket;
}


int connect_client(int connect_port, char *address)
{
    struct sockaddr_in addr;
    int cfd;

    cfd = socket(AF_INET, SOCK_STREAM, 0);
    if (cfd == -1)
    {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(connect_port);
    addr.sin_family = AF_INET;

    if (!inet_aton(address, (struct in_addr *)&addr.sin_addr.s_addr))
    {
        fprintf(stderr, "inet_aton(): bad IP address format\n");
        close(cfd);
        return -1;
    }

    if (connect(cfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("connect()");
        shutdown(cfd, SHUT_RDWR);
        close(cfd);
        return -1;
    }

    if (fcntl(cfd, F_SETFL, O_NONBLOCK) == -1)
	{
    }
    return cfd;
}



void on_create(int id)
{
    printf("%sClient id [%d] join the server .%s\n", MAGENTA, id, RESET);
}

void on_close(int id)
{
    printf("%sClient id [%d] exit server .%s\n", YELLOW, id, RESET);    
}

void on_data(int id, int socket, Buffer *buffer)
{
    printf("%sClient id [%d]  socket[%d]  size [%ld]  %s\n", YELLOW, id, socket, buffer->size, RESET);   
}


int create_server(Server *server, int listen_port)
{
    struct sockaddr_in addr;
    int yes;
    server->ready = 0;
    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server->socket == -1)
    {
        perror("socket");
        return -1;
    }

    if (fcntl(server->socket, F_SETFL, O_NONBLOCK) == -1)
	{
    }


    yes = 1;
    if (setsockopt(server->socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1)
    {
        perror("setsockopt");
        close(server->socket);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_port = htons(listen_port);
    addr.sin_family = AF_INET;
    if (bind(server->socket, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(server->socket);
        return -1;
    }

    printf("%saccepting connections on port %d%s\n", GREEN, listen_port, RESET);
    listen(server->socket, 10);

    server->head = NULL;
    server->tail = NULL;
    server->clients_count = 0;
    server->clients_id = 0;
    server->loop = 1;
    server->max_fd = server->socket;
    server->OnCreate = on_create;
    server->OnRemove = on_close;
    server->OnRead   = on_data;


    

    FD_ZERO(&server->socket_list);
    FD_SET(server->socket, &server->socket_list);
    server->ready = 1;

    return server->socket;
}

void close_server(Server *server)
{
    server->ready = 0;
    if (server->head != NULL)
    {
        Client *current = server->head;
        printf("%sClose %d clients%s\n", GREEN, server->clients_count, RESET);
        while (current)
        {
            Client *tmp = current;
            current = current->next;
            if (tmp != NULL)
            {
                if (tmp->socket > 0)
                {
                    close(tmp->socket);
                }

                printf("%s[%d] Close socket: [%d] id: [%d] %s\n", GREEN, server->clients_count, tmp->socket, tmp->id, RESET);
                free(tmp);
                tmp = NULL;
            }
            server->clients_count--;
        }
    }
    server->head = NULL;
    server->tail = NULL;
    if (server->socket > 0)
    {
        printf("%sCloser server%s\n", RED, RESET);
        shutdown(server->socket, SHUT_RDWR);
        close(server->socket);
    }
    
    server->OnCreate = NULL;
    server->OnRemove = NULL;
    server->OnRead   = NULL;

}

int server_add(Server *server, int socket)
{
    if (server->socket <= 0)
        return -1;
    Client *client = (Client *)malloc(sizeof(Client));
    client->next = NULL;
    client->ready = 1;
    client->id = server->clients_id++;
    client->socket = socket;
    if (server->head == NULL)
    {
        server->head = client;
        server->tail = client;
    }
    else
    {
        server->tail->next = client;
        server->tail = client;
    }

    return client->id;
}

int server_remove(Server *server, int socket)
{
    if (server->socket <= 0 || !server->head)
    {
        return -1;
    }
    int id = -1;

    if (server->head->socket == socket)
    {
        id = server->head->id;
        Client *tmp = server->head;
        server->head = server->head->next;
        free(tmp);
        tmp = NULL;
        
        return id;
    }

    Client *current = server->head;
    Client *prev = NULL;

    while ((current != NULL) && current->socket != socket)
    {
        prev = current;
        current = current->next;
    }

    if (!current) // not found
    {
        return id;
    }

    //  printf(" prev %d current %d \n",prev->socket,current->socket);
    if (current == server->tail)
    {
        id = current->id;
        server->tail = prev;
        server->tail->next = NULL;

        free(current);
        current = NULL;

        return id;
    }
    else
    {

        id = current->id;
        prev->next = current->next;
        free(current);
        current = NULL;
        return id;
    }

    return id;
}




Client *server_get_client_by_socket(Server *server, int socket)
{
    Client *client = NULL;
    if (!server->head)
        return client;

    Client *current = server->head;
    while (current != NULL)
    {
         client = current;
         if (client->socket == socket)
         {
            break;
         }
        current = current->next;
    }
    return client;
}

void send_to_ignore(Server *server,unsigned char* buffer, int bufferSize, int ignore)
{
    Client *client = NULL;
    if (!server->head)
        return;

    Client *current = server->head;
    while (current != NULL)
    {
        client = current;
        if (client->socket != ignore && client->socket>0)
        {
            send(client->socket, buffer, bufferSize, MSG_DONTWAIT);
        }
        current = current->next;
    }    
}

void send_to_id(Server *server, int id, unsigned char* buffer, int bufferSize)
{
    Client *client = NULL;
    if (!server->head)
        return;

    Client *current = server->head;
    while (current != NULL)
    {
        client = current;
        if (client->id ==id && client->socket>0)
        {
            send(client->socket, buffer, bufferSize, MSG_DONTWAIT);
            break;
        }
        current = current->next;
    }    
}


void send_to(int socket, unsigned char* buffer, int bufferSize)
{
    if (socket>0)
    {
        send(socket, buffer, bufferSize, MSG_DONTWAIT);
    }
}

void send_all(Server *server,  unsigned char* buffer, int bufferSize)
{
    Client *client = NULL;
    if (!server->head)
        return;

    Client *current = server->head;
    while (current != NULL)
    {
        client = current;
        if (client->socket>0)
        {
            send(client->socket, buffer, bufferSize, MSG_DONTWAIT);
            break;
        }
        current = current->next;
    }    
}




void client_send(int socket,unsigned char* buffer, int bufferSize)
{
    if (socket>0)
        send(socket, buffer, bufferSize, MSG_DONTWAIT);
}

void client_sendbuffer(int socket,Buffer *buffer)
{
    client_send(socket,buffer->data,buffer->pos*sizeof(uint8));
}

void sendbuffer_to_ignore(Server *server, Buffer *buffer, int ignore)
{
    send_to_ignore(server,buffer->data,buffer->pos*sizeof(uint8),ignore);
}

void sendbuffer_to_id(Server *server, int id, Buffer *buffer)
{
  send_to_id(server,id,buffer->data,buffer->pos*sizeof(uint8));
}

void sendbuffer_all(Server *server, Buffer *buffer)
{
   send_all(server,buffer->data,buffer->pos*sizeof(uint8));
}
void sendbuffer_to(int socket, Buffer *buffer)
{
    send_to(socket,buffer->data,buffer->pos*sizeof(uint8));
}

void send_client_part(Server *server, int ignore, int id)
{
    Buffer buffer;
    const int SIZE =20;
    uint8 data[SIZE];
    buffer_set(&buffer,data,SIZE);
    write_int(&buffer,MSG_CLIENT_PART);
    write_int(&buffer,id);
    sendbuffer_to_ignore(server,&buffer,ignore);
}

void send_client_join(Server *server, int ignore, int id)
{
    Buffer buffer;
    const int SIZE =20;
    uint8 data[SIZE];
    buffer_set(&buffer,data,SIZE);
    write_int(&buffer,MSG_CLIENT_JOIN);
    write_int(&buffer,id);
    sendbuffer_to_ignore(server,&buffer,ignore);
}

void send_client_id(int socket, int id)
{
    Buffer buffer;
    const int SIZE =20;
    uint8 data[SIZE];
    buffer_set(&buffer,data,SIZE);
    write_int(&buffer,MSG_PLAYER_ID);
    write_int(&buffer,id);
    client_sendbuffer(socket,&buffer);
}

void send_request_list(int socket,int id)
{
    Buffer buffer;
    const int SIZE =10;
    uint8 data[SIZE];
    buffer_set(&buffer,data,SIZE);
    write_int(&buffer,MSG_CLIENT_LIST);
    write_int(&buffer,id);
    client_sendbuffer(socket,&buffer);
}

void send_request_info(int socket,int id)
{
    Buffer buffer;
    const int SIZE =10;
    uint8 data[SIZE];
    buffer_set(&buffer,data,SIZE);
    write_int(&buffer,MSG_PLAYER_INFO);
    write_int(&buffer,id);
    client_sendbuffer(socket,&buffer);
}

void send_clients_list(Server *server)
{
    Client *client = NULL;
    if (!server->head)
        return;

    Buffer buffer;
    uint8 data[200];
    buffer_set(&buffer,data,200);
    write_int(&buffer,MSG_CLIENT_LIST);
    write_int(&buffer,server->clients_count);

    Client *current = server->head;
    while (current != NULL)
    {
        client = current;
        if (client->socket>0)
        {
            write_int(&buffer,client->id);
        }
        current = current->next;
    }    
    
    sendbuffer_all(server,&buffer);
}


void send_clients_list_ignore(Server *server,int ignore)
{
    Client *client = NULL;
    if (!server->head)
        return;

    Buffer buffer;
    uint8 data[200];
    buffer_set(&buffer,data,200);
    write_int(&buffer,MSG_CLIENT_LIST);
    write_int(&buffer,server->clients_count);

    Client *current = server->head;
    while (current != NULL)
    {
        client = current;
        if (client->socket>0)
        {
            write_int(&buffer,client->id);
        }
        current = current->next;
    }    
    
    sendbuffer_to_ignore(server,&buffer,ignore);
}


void send_clients_list_to(Server *server,int to)
{
    Client *client = NULL;
    if (!server->head)
        return;

    Buffer buffer;
    uint8 data[2000];
    buffer_set(&buffer,data,2000);
    write_int(&buffer,MSG_CLIENT_LIST);
    write_int(&buffer,server->clients_count);

    Client *current = server->head;
    while (current != NULL)
    {
        client = current;
        if (client->socket>0)
        {
            write_int(&buffer,client->id);
        }
        current = current->next;
    }    
    sendbuffer_to(to,&buffer);
}
int close_client(int socket)
{
    if (socket>0)
    {
        return shutdown(socket, SHUT_RDWR)  ||        close(socket);
    }
    return -1;
}
int process_client(int clientSocket, Buffer *bytes)
{

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(clientSocket, &readfds);
    int ready = select(clientSocket + 1, &readfds, NULL, NULL, NULL);
    if (ready <=0) 
    {
        perror("select");
        return -1;
    } else 
    {
       
        if (FD_ISSET(clientSocket, &readfds)) 
        {
            unsigned char buffer[1024];
            
            int bytesRead = recv(clientSocket, buffer, sizeof(buffer)-1, MSG_DONTWAIT);
            if (bytesRead == -1) 
            {
               // printf("Erro ao receber dados do servidor");
                return -1;
            } else if (bytesRead == 0) 
            {
              //  printf("Conexão encerrada pelo servidor.\n");
               return -1;
            } else 
            {
            buffer_set(bytes,buffer,bytesRead*sizeof(uint8));
            return 2;
            }
        }

    }
   return 1;
}


int process_server(Server *server)
{
    if (server->socket <= 0 || !server->ready)
    {
        return -1;
    }

    while (server->loop)
    {
        server->max_fd = server->socket;

        Client *current = server->head;
        while (current)
        {
            if (current->socket > 0)
                server->max_fd = current->socket;
            current = current->next;
        }

        // printf(" max %d  count %d \n",server->max_fd,server->clients_count);

        server->master = server->socket_list;
        server->ready = select(server->max_fd + 1, &server->master, NULL, NULL, NULL);
        if (server->ready == -1 && errno == EINTR)
            break;
        if (server->ready == -1)
        {
            perror("select()");
            break;
        }

        for (int fd = 0; fd <= server->max_fd; fd++)
        {

            if (FD_ISSET(fd, &server->master))
            {
                if (fd == server->socket) // create client
                {

                    int socket = new_connection(server);
                    if (socket > 0)
                    {
                        int id = server_add(server, socket);
                        server->OnCreate(id);
                        send_client_id(socket,id);
                        send_client_join(server,socket,id);
                        send_clients_list_to(server,socket);
                        server->clients_count++;
                    }
                }
                else // process client
                {
                     
                     unsigned char buffer[1024*4]={'\0'};
                     int reading=-1;
                     int totalRead=0;
                     
                    //  while( (reading = recv(fd, buffer, sizeof(buffer)-1, MSG_DONTWAIT)>0))
                    //  {
                    //         append_buffer(&server->buffer, buffer,reading);
                    //         totalRead+=reading;
                    //  }

                    reading = recv(fd, buffer, sizeof(buffer)-1, MSG_DONTWAIT);
                    totalRead+=reading;
                     

                      
                    if (reading == -1) // client abort connection
                    {
                        int id = server_remove(server, fd);
                        FD_CLR(fd, &server->socket_list);
                        close(fd);
                        printf("%sClient id  [%d] quit.%s\n", YELLOW, id, RESET);
                        server->clients_count--;
                        
                        
                    }
                    else if (reading == 0 && totalRead<=0) // client close connection
                    {
                        int id = server_remove(server, fd);
                        FD_CLR(fd, &server->socket_list);
                        close(fd);
                        server->OnRemove(id); 
                        send_client_part(server,fd,id);
                        server->clients_count--;
                        //send_clients_list_ignore(server,fd);
                        //  printf("%sClient id  [%d] close.%s\n", YELLOW, id, RESET);
                    }
                    else // process data
                    {
                     
                       buffer[reading]='\0';
                       Buffer data;
                       buffer_set(&data, buffer, reading);
                    
                        Client *client = server_get_client_by_socket(server, fd);

                        if (client != NULL)
                        {
                          //  printf("Client id [%d] data.\n", client->id);
                            server->OnRead(client->id, fd, &data);
                        }
                    }
                }
            }
        }
    }
    return -1;
}


////******************************************************************************************************************

void seek_buffer(Buffer* buffer, int pos)
{
    if (pos<0)
        pos=0;
    if (pos>buffer->size-1)
        pos=buffer->size;
    buffer->pos=pos;
}

void load_buffer(Buffer* buffer,uint8 *data,int size)
{
    buffer->pos =0;
    buffer->size=size;
    buffer->data=(uint8*)malloc(size*sizeof(uint8));
    memcpy(buffer->data, data, size * sizeof(uint8));
    buffer->is_set=0;
}


void init_buffer(Buffer* buffer,int size)
{
    buffer->pos=0;
    buffer->size=size;
    buffer->data=(uint8*)malloc(size*sizeof(uint8));
    buffer->is_set=0;

}

void free_buffer(Buffer *buffer)
{
    buffer->pos=0;
    buffer->size=0;
    if(!buffer->is_set)
    {
    if (buffer->data!=NULL)
        free(buffer->data);
    buffer->data=NULL;
    }
}

int write_buffer(Buffer *buffer, const uint8 *data, int size) 
{
    int32		i;

	assert(buffer->pos + size < buffer->size);

	if (buffer->pos + size >= buffer->size)
		return -1;

	for (i=0; i< size; i++)
		buffer->data[buffer->pos++] = data[i];

	return buffer->pos;
}

int append_buffer(Buffer *buffer, const uint8 *data, int size) 
{
    if (buffer->pos + size >= buffer->size) 
    {
        buffer->size = buffer->size *2;
        buffer->data = realloc(buffer->data, buffer->size *sizeof(uint8));
    } 
    memcpy(buffer->data + buffer->pos, data, size * sizeof(uint8));
    buffer->pos += size;
    return buffer->pos;
}


int read_buffer(Buffer *buffer, uint8 *data, int size) 
{
   int32		i;

	assert(buffer->pos + size <= buffer->size);

	for (i=0; i< size; i++)
		data[i] = buffer->data[buffer->pos++];

	return buffer->pos;
}



int write_byte(Buffer *buffer,  uint8 Byte)
{
   assert(buffer->pos + (int32)sizeof(uint8) < buffer->size);

	if (buffer->pos + (int32)sizeof(uint8) >= buffer->size)
		return -1;

	buffer->data[buffer->pos] = Byte;

	buffer->pos += sizeof(uint8);

	return buffer->pos;
}

int write_short(Buffer *buffer, int v)
{
     write_byte (buffer, (uint8)((v >> 8) & 0xFF));
     write_byte (buffer, (uint8)((v >> 0) & 0xFF));
   
    return buffer->pos;
}

int write_int(Buffer *buffer, int v)
{
    write_byte(buffer, (uint8)((v >> 24) & 0xFF));
    write_byte(buffer, (uint8)((v >> 16) & 0xFF));
    write_byte(buffer, (uint8)((v >> 8) & 0xFF));
    write_byte(buffer, (uint8)((v >> 0) & 0xFF));   
    return buffer->pos;
}


int write_long(Buffer *buffer, uint32 v)
{
    write_byte(buffer, (uint8)((v >> 56) & 0xFF));
    write_byte(buffer, (uint8)((v >> 48) & 0xFF));
    write_byte(buffer, (uint8)((v >> 40) & 0xFF));
    write_byte(buffer, (uint8)((v >> 32) & 0xFF));
    write_byte(buffer, (uint8)((v >> 24) & 0xFF));
    write_byte(buffer, (uint8)((v >> 16) & 0xFF));
    write_byte(buffer, (uint8)((v >> 8) & 0xFF));
    write_byte(buffer, (uint8)((v >> 0) & 0xFF));   


    return buffer->pos;
}


uint8 read_byte(Buffer *buffer)
{
    assert(buffer->pos + (int32)sizeof(uint8) <= buffer->size);

    uint8	byte = buffer->data[buffer->pos];

	buffer->pos += sizeof(uint8);
    return byte;
}


int read_short(Buffer *buffer)
{
    int b1 = read_byte(buffer) & 0xFF;
    int b2 = read_byte(buffer) & 0xFF;
    return (int)((b1 << 8) | b2); 

}

int read_int(Buffer *buffer)
{
    assert(buffer->pos + (int32)sizeof(int) <= buffer->size);


        int b1 = ((int) read_byte(buffer)) & 0xFF;
        int b2 = ((int) read_byte(buffer)) & 0xFF;
        int b3 = ((int) read_byte(buffer)) & 0xFF;
        int b4 = ((int) read_byte(buffer)) & 0xFF;
        return ((b1 << 24) | (b2 << 16) | (b3 << 8) | b4);


    
}



uint32 read_long(Buffer *buffer)
{
    assert(buffer->pos + (int32)sizeof(uint32) <= buffer->size);

 return ((uint32)read_byte(buffer) << 56) + 
        ((long)(read_byte(buffer) & 255) << 48) + 
        ((long)(read_byte(buffer) & 255) << 40) +
        ((long)(read_byte(buffer) & 255) << 32) +
        ((long)(read_byte(buffer) & 255) << 24) + 
        (long)((read_byte(buffer) & 255) << 16) +
        (long)((read_byte(buffer) & 255) << 8) +
        (long)((read_byte(buffer) & 255) << 0);
    
}
int write_float(Buffer *buffer, float v)
{
 

	assert(buffer->pos + (int32)sizeof(float) < buffer->size);

    uint32_t float_bits;
    memcpy(&float_bits, &v, sizeof(float));


    write_byte(buffer, (uint8_t)((float_bits >> 24) & 0xFF));
    write_byte(buffer, (uint8_t)((float_bits >> 16) & 0xFF));
    write_byte(buffer, (uint8_t)((float_bits >> 8) & 0xFF));
    write_byte(buffer, (uint8_t)((float_bits >> 0) & 0xFF));

    return buffer->pos;
}

float read_float(Buffer *buffer)
{
    assert(buffer->pos + (int32)sizeof(float) <= buffer->size);
   
    uint8_t b1 = read_byte(buffer);
    uint8_t b2 = read_byte(buffer);
    uint8_t b3 = read_byte(buffer);
    uint8_t b4 = read_byte(buffer);


    uint32_t float_bits = ((uint32_t)b1 << 24) |
                          ((uint32_t)b2 << 16) |
                          ((uint32_t)b3 << 8) |
                          (uint32_t)b4;


    float v;
    memcpy(&v, &float_bits, sizeof(float));

    return v;
}



int write_string(Buffer *buffer, const char *text) 
{
    int32       len =(int32)strlen((char*)text)+1 ;
	assert(buffer->pos + len < buffer->size);
	write_short(buffer,len);
    memcpy(buffer->data + buffer->pos, text, len);
    buffer->pos += len;
    return buffer->pos;
}


int read_string(Buffer *buffer, char *text) 
{

    int strLength = read_short(buffer) ;
    if (strLength >= 0 && (buffer->pos + strLength <= buffer->size) ) 
    {
        memcpy(text, buffer->data + buffer->pos, strLength);
     //   text[strLength] = '\0'; 
        buffer->pos += strLength;
    }

    return buffer->pos;
}

void buffer_set(Buffer *buffer, uint8 *data, int32 size)
{
	assert(buffer);

	buffer->data = data;
	buffer->size = size;
	buffer->pos = 0;
    buffer->is_set=1;

	
}




uint8 *load_file(const char *fileName, unsigned int *bytesRead)
{
    unsigned char *data = NULL;
    *bytesRead = 0;

    if (fileName != NULL)
    {
      
        FILE *file = fopen(fileName, "rb");

        if (file != NULL)
        {
            fseek(file, 0, SEEK_END);
            int size = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (size > 0)
            {
                data = (unsigned char *)malloc(size*sizeof(unsigned char));
                unsigned int count = (unsigned int)fread(data, sizeof(unsigned char), size, file);
                *bytesRead = count;
            }
            fclose(file);
        }
    }
    return data;
}



bool save_file(const char *fileName, void *data, unsigned int bytesToWrite)
{
    bool success = false;
    if (fileName != NULL)
    {

        FILE *file = fopen(fileName, "wb");

        if (file != NULL)
        {
            fwrite(data, sizeof(unsigned char), bytesToWrite, file);
            int result = fclose(file);
            if (result == 0) success = true;
        }
    }
    return success;
}

bool buffer_load(Buffer *buffer, const char* filename)
{
    unsigned int bytesRead;
    unsigned char *data = load_file(filename, &bytesRead);
    if (data!=NULL)
    {
        load_buffer(buffer,data,bytesRead * sizeof(uint8));
        free(data);
        seek_buffer(buffer,0);
        return true;
    }
    return false;
}
bool buffer_save(Buffer *buffer, const char* filename)
{
    return save_file(filename,buffer->data,buffer->pos);
    
}

#endif