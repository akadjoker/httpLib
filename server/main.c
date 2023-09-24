
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define SERVER_IMPLEMENTATION
#include "server.h"





void handlesginal(int sig)
{
    printf("\n %s Aborting  %d %s\n", RED, sig, RESET);
}


void client_create(int id)
{
    printf("%s id [%d] join the server .%s\n", MAGENTA, id, RESET);
}

void client_close(int id)
{
    printf("%sid [%d] exit server .%s\n", YELLOW, id, RESET);    
}


Server server;

void client_data(int id, int socket, Buffer *buffer)
{
  //  printf("%s id [%d]  socket[%d]  size [%d] total[%d] %s\n", YELLOW, id, socket, buffer->size, buffer->count, RESET);   
    
    int type = read_int(buffer);
    


 //   printf("on read %s id [%d]  socket[%d]  size [%ld]  %s\n", YELLOW, id, socket, buffer->size, RESET);   

    

    if (type==MSG_PLAYER_CHAT) //string 
    {   
        char text[512]={'\0'};
        read_string(buffer,text);
        
        Buffer sendBUffer;
        uint8 data[512];
        buffer_set(&sendBUffer,data,512);
        write_int(&sendBUffer,MSG_PLAYER_CHAT);//type
        write_int(&sendBUffer,id);//QUEM
        write_string(&sendBUffer,text);
        sendbuffer_to_ignore(&server, &sendBUffer, socket);//todos menos quem evia

       //  printf("%s id [%d]  MSG: (%s) %s\n", GREEN, id, text, RESET); 
    } else  
    if (type==MSG_CLIENT_LIST)
    {
        read_int(buffer);
        send_clients_list_to(&server,socket);   
    
    } else  
    if (type==MSG_PLAYER_DATA)
    {
        
        int score= read_int(buffer);
        char name[20]={'\0'};
        read_string(buffer,name);
        int x = read_int(buffer);
        int y = read_int(buffer);
        int angle = read_int(buffer);

        Buffer sendBUffer;
        uint8 data[512];
        buffer_set(&sendBUffer,data,512);
        write_int(&sendBUffer,MSG_PLAYER_DATA);//type
        write_int(&sendBUffer,id);//type
        write_int(&sendBUffer,score);//score
        write_int(&sendBUffer,x);
        write_int(&sendBUffer,y);
        write_int(&sendBUffer,angle);
        write_string(&sendBUffer,name);
        sendbuffer_to_ignore(&server, &sendBUffer, socket);

        
     //   printf("%s player  id [%d]  name:(%s)  score:(%d)  (%d,%d,%d) %s\n", GREEN, id, name,score,x,y,angle, RESET); 
        
    } else
    if (type==MSG_PLAYER_INFO)
    {
        
        int rec_id = read_int(buffer);
        Buffer sendBUffer;
        uint8 data[25];
        buffer_set(&sendBUffer,data,25);
        write_int(&sendBUffer,MSG_PLAYER_INFO);//type
        write_int(&sendBUffer,id);//envia para este id , quem pediu  ainfo
        write_int(&sendBUffer,rec_id);//do player
        sendbuffer_to_id(&server, rec_id ,&sendBUffer);

       // printf (" clientd %d request info de %d \n",id,rec_id);

    } else
    if (type ==MSG_PLAYER_SHOOT)
    {
        
        int x = read_int(buffer);
        int y = read_int(buffer);
        int angle = read_int(buffer);
        

        
        Buffer sendBUffer;
        uint8 data[512];
        buffer_set(&sendBUffer,data,512);
        write_int(&sendBUffer,MSG_PLAYER_SHOOT);//type
        write_int(&sendBUffer,id);
        write_int(&sendBUffer,x);
        write_int(&sendBUffer,y);
        write_int(&sendBUffer,angle);
    
        sendbuffer_to_ignore(&server, &sendBUffer, socket);
        
    } else
    if (type == MSG_PLAYER_RESPAW)
    {
        int id_current = read_int(buffer);
        char name[20]={'\0'};
        read_string(buffer,name);
        int x = read_int(buffer);
        int y = read_int(buffer);
        int angle = read_int(buffer);

        Buffer sendBUffer;
        uint8 data[512];
        buffer_set(&sendBUffer,data,512);
        write_int(&sendBUffer,MSG_PLAYER_RESPAW);//type
        write_int(&sendBUffer,id_current);//type
        write_int(&sendBUffer,x);
        write_int(&sendBUffer,y);
        write_int(&sendBUffer,angle);
        write_string(&sendBUffer,name);
        sendbuffer_to_ignore(&server, &sendBUffer, socket);

         printf ("RESPAW  id %d  create id %d \n",id,id_current);



    } else
    if (type==MSG_PLAYER_KILL)
    {
        int id_killed = read_int(buffer);
        int id_killer = read_int(buffer);


      //  printf (" id %d  killed by %d \n",id,id_killer);

        Buffer sendBUffer;
        uint8 data[128];
        buffer_set(&sendBUffer,data,128);
        write_int(&sendBUffer,MSG_PLAYER_KILL);//type
        write_int(&sendBUffer,id_killed);
        write_int(&sendBUffer,id_killer);
        sendbuffer_to_ignore(&server, &sendBUffer, socket); 
    }

    



}


int main()
{ 

    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, handlesginal);
    signal(SIGQUIT, handlesginal);
    signal(SIGTERM, handlesginal);


    



    create_server(&server, 1479);

    server.OnCreate = client_create;
    server.OnRemove = client_close;
    server.OnRead   = client_data;


    process_server(&server);
    close_server(&server);


    return 0;
}
