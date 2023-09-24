#include <pthread.h>
#include <errno.h>
#include <string.h>


#define SERVER_IMPLEMENTATION
#include "server.h"

#include <raylib.h>
#include "raygui.h"
#include <math.h>

#define SHIP 0
#define BULLET 1


typedef struct GameObject
{
    bool alive;
    bool local;
    bool respaw;
    int type;
    int x;
    int y;
    int angle;
    int score;
    int id;
    int energia;
    int last_id_hit;
    bool hit;
    float life; 
    float radius;
    float times[5];
    Texture2D image;
    char name[50];
    
    
    struct GameObject* next;
}GameObject;



typedef struct ObjectManager
{
  int count;
  GameObject *head;
  GameObject *tail;
}ObjectManager;


GameObject* newGameObject(const char* name, int id)
{
    GameObject * obj =(GameObject*) malloc(sizeof(GameObject));
    obj->next =NULL;
    obj->alive=true;
    obj->angle=0;
    obj->id=id;
    obj->local=true;
    obj->type = SHIP;
    obj->respaw=false;
    obj->energia=100;
    obj->x=0;
    obj->y=0;
    obj->last_id_hit=-1;
    obj->times[0]=0;
    obj->times[1]=0;
    obj->times[2]=0;
    obj->times[3]=0;
    obj->times[4]=0;
    obj->times[5]=0;
    obj->hit=true;
    obj->radius=18;
    obj->score=0;
    strcpy(obj->name, name);
    return obj;
}


void init_manager(ObjectManager *manager)
{
    manager->count=0;
    manager->head=NULL;
    manager->tail=NULL;
}

bool manager_contains(ObjectManager *manager,int id)
{
     GameObject *current = manager->head;
        while(current)
        {
            if (current->id==id)            
                return true;
            current=current->next;
        }
        return false;
}

GameObject *manager_get(ObjectManager *manager,int id)
{
       GameObject *current = manager->head;
        while(current)
        {
            if (current->id==id)            
                return current;
            current=current->next;
        }
        return NULL;
}

void free_manager(ObjectManager *manager)
{
    printf("Remove %d objects.\n",manager->count);
    if (manager->head!=NULL)
    {
        GameObject *current = manager->head;
        while(current)
        {
            GameObject *tmp = current;
            current=current->next;
            if(tmp!=NULL)
            {
                printf("Remove %d ID: %d  Name: %s object.\n",manager->count,tmp->id,tmp->name);
                manager->count--;
                free(tmp);
                tmp=NULL;
            }
        }
    }
    manager->count=0;
    manager->head=NULL;
    manager->tail=NULL;
}



void manager_add(ObjectManager* manager, GameObject *obj)
{
    if (!obj)
        return;
    // if (manager_contains(manager,obj->id))
    //     return;

    if (!manager->head)
    {
        manager->head=obj;
        manager->tail=obj;
    } else
    {
        manager->tail->next=obj;
        manager->tail=obj;
    }
    manager->count++;       
}

void manager_clean(ObjectManager* manager)
{
    if (manager->head!=NULL)
    {
        GameObject *current = manager->head;

        if (!current->alive)
        {
            GameObject *tmp = manager->head;
            GameObject *next = manager->head->next;

        //    printf("Remove  ID: %d  Name: %s object.\n",tmp->id,tmp->name);

            free(tmp);
            manager->count--;

            manager->head= next;
            return;
        }

        GameObject *prev;
        while( (current!=NULL) && current->alive)
        {
            prev=current;            
            current=current->next;
        }
        if(current!=NULL)
        {
            manager->count--;

            if (current==manager->tail)
            {
                manager->tail = prev;
                manager->tail->next=NULL;
            } else
            {
               prev->next = current->next;     
             
            }
        printf("Remove  ID: %d  Name: %s object.\n",current->id,current->name);
         free(current);
        }
    }
}
void manager_remove(ObjectManager* manager, int id)
{
    if (manager->head!=NULL)
    {
        GameObject *current = manager->head;

        if (current->id==id)
        {
            GameObject *tmp = manager->head;
            GameObject *next = manager->head->next;

            printf("Remove  ID: %d  Name: %s object.\n",tmp->id,tmp->name);

            free(tmp);
            manager->count--;

            manager->head= next;
            return;
        }

        GameObject *prev;
        while( (current!=NULL) && current->id != id)
        {
            prev=current;            
            current=current->next;
        }
        if(current!=NULL)
        {
            manager->count--;

            if (current==manager->tail)
            {
                manager->tail = prev;
                manager->tail->next=NULL;
            } else
            {
               prev->next = current->next;     
             
            }
         printf("Remove  ID: %d  Name: %s object.\n",current->id,current->name);
         free(current);

        }
    }
}
int LerpInt(int oldValue, int newValue, float t) 
{
    t = fmax(0.0, fmin(1.0, t));
    int interpolatedValue = (int)((1.0 - t) * oldValue + t * newValue);
    return interpolatedValue;
}

int const PORT = 1478;

#define MESSAGE_INTERVAL 0.2
#define CHAT_INTERVAL 2
#define BULLET_INTERVAL 0.1





const int screenWidth = 640;
const int screenHeight = 400;


int client=-1;
pthread_t threadId;
char errors[256];

ObjectManager manager;
ObjectManager manager_objects;
GameObject *player=NULL;

bool newMessage=false;
float messageTime=0;
int total_message=0;
char **messages;
int chat_size;
int visibleMessages = 20;
int messageSpacing = 12; 
bool isOffLIne=false;
bool playsIsKill=false;
float bulletTime=0;
float bulletMaxTime=0.2;
int player_id=0;

void chat_init()
{
    const int START=50;
    chat_size=START;
    total_message=0;
    messages = (char **)malloc(START * sizeof(char *));
}


void add_chat_msg(char *msg) 
{
    if (total_message >= chat_size) 
    {
        chat_size = chat_size * 2;
        char **new_messages = (char **)realloc(messages, chat_size * sizeof(char *));
        if (new_messages == NULL) 
        {
            return;
        }
        messages = new_messages;
    }
    messages[total_message] = strdup(msg);
    if (messages[total_message] == NULL) 
    {
        return;
    }
    total_message++;

}


void chat_clean() 
{
    if (messages != NULL) 
    {
        for (int i = 0; i < total_message; i++) 
        {
            free(messages[i]);
        }
        total_message=0;
    }
}

void chat_free() 
{
    if (messages != NULL) 
    {
        for (int i = 0; i < total_message; i++) 
        {
            free(messages[i]);
        }
        free(messages);
    }
}



void draw_chat() 
{
    int startY = screenHeight - (visibleMessages * messageSpacing);

    for (int i = 0; i < visibleMessages && i < total_message; i++) 
    {
        int messageIndex = total_message - 1 - i; // Comece da mensagem mais recente
        int x = 10;
        int y = startY - (i * messageSpacing);
        GuiLabel((Rectangle){x, y, screenWidth - 20, messageSpacing}, messages[messageIndex]);
    }
}


void update_network_object(GameObject *obj)
{
        if (!obj || client<0)
            return;
        char data[512];
        Buffer buffer;
        buffer_set(&buffer, data, 512);   
        write_int(&buffer,MSG_PLAYER_DATA);//type
        write_int(&buffer,obj->score);//score
        write_string(&buffer,obj->name);
        write_int(&buffer,obj->x);
        write_int(&buffer,obj->y);
        write_int(&buffer,obj->angle);
        client_sendbuffer(client, &buffer);
}

void respaw_object(const char* name)
{
        char data[512];
        Buffer buffer;

        player = newGameObject(name,player_id);
        player->score = 0;
        player->local=true;
        
        
        player->x=GetRandomValue(40,screenWidth-40);
        player->y=GetRandomValue(40,screenHeight-40);
        player->angle =GetRandomValue(0,360);
    


        buffer_set(&buffer, data, 512);   
        write_int(&buffer,MSG_PLAYER_RESPAW);//type
        write_int(&buffer,player_id);
        write_string(&buffer,name);
        write_int(&buffer,player->x);
        write_int(&buffer,player->y);
        write_int(&buffer,player->angle);
        client_sendbuffer(client, &buffer);

     manager_add(&manager,player);
}



void player_shoot(int x,int y, int angle)
{
        if (client<0)
            return;
        char data[250];
        Buffer buffer;
        buffer_set(&buffer, data, 250);   
        write_int(&buffer,MSG_PLAYER_SHOOT);//type
        write_int(&buffer,x);
        write_int(&buffer,y);
        write_int(&buffer,angle);//type
       client_sendbuffer(client, &buffer);
}




    
static void *ServerThread(void *arg)
{
    

    TraceLog(LOG_INFO, "Start  theread");
    
    

    while(1)
    {
        Buffer buffer;
        int status = process_client(client,&buffer);
        if (status<=1)
        {
            break;
        }
        if (status==2)
        {
            int type = read_int(&buffer);
        //    printf(" read type (%d)\n",type);
            switch (type)
            {

                case MSG_CLIENT_JOIN:
                {
                    int id = read_int(&buffer);
                    printf(" PLAYER (%d) JOIN GAME\n",id);
                    GameObject *jointer;
                    jointer = newGameObject("Remote",id);
                    jointer->local=false;
                    manager_add(&manager,jointer);
                    send_request_list(client,id);
                    break;
                }
                case MSG_CLIENT_PART:
                {
                    int id = read_int(&buffer);
                    printf("PLAYER (%d) EXIT GAME\n",id);
                    manager_remove(&manager,id);
                    break;
                }
                case MSG_CLIENT_LIST:
                {
                    int count = read_int(&buffer);
                   // printf("**** PLAYERS(%d) IN GAME\n",count);
                    for (int i=0;i<count;i++)
                    {
                           int id = read_int(&buffer);
                          

                           if (!manager_contains(&manager,id) && id != player->id)
                           {
                            printf(" PLAYER [%d]  id: (%d) \n",i,id); 
                            GameObject *jointer;
                            jointer = newGameObject("Remote",id);
                            jointer->local=false;
                            manager_add(&manager,jointer); 
                            send_request_info(client, id);
                           }
                    }
                    
                    break;
                }
                case MSG_PLAYER_CHAT:
                {
                     int id = read_int(&buffer);
                     char text[512]={'\0'};
                     read_string(&buffer,text);
                     messageTime = GetTime();
                     add_chat_msg(TextFormat("%d :%s",id,text));
                     newMessage=true;

                    break;
                }
                case MSG_PLAYER_ID:
                {
                    player_id = read_int(&buffer);
                    printf("CREATE LOCAL PLAYER (%d) ID \n",player_id);

                   
                    player = newGameObject("Local",player_id);
                    player->score = 0;
                    player->local=true;
                    player->alive=true;
                    player->x=GetRandomValue(40,screenWidth-40);
                    player->y=GetRandomValue(40,screenHeight-40);
                    player->angle =GetRandomValue(0,360);
                    manager_add(&manager,player);


                    send_request_list(client,player_id);

                    break;

                }
                case MSG_PLAYER_DATA:
                {
                    int id = read_int(&buffer);
                    int score = read_int(&buffer);
                    int x = read_int(&buffer);
                    int y = read_int(&buffer);
                    int angle = read_int(&buffer);
                    char name[55];
                    read_string(&buffer,name);

                    GameObject *process = manager_get(&manager,id);
                    if(process!=NULL)
                    {
                        float drop =0.7f;
                        process->x = LerpInt(process->x,x,drop); 
                        process->y = LerpInt(process->y,y,drop);
                        process->angle = LerpInt(process->angle,angle,drop);
                        strcpy(process->name,name);
                    }

                  //  printf(" [%d] (%d %d)   %s \n",id,x,y,name);

                    break;
                 }
                 case MSG_PLAYER_INFO:
                 {
                    int sendId = read_int(&buffer);
                    int id = read_int(&buffer);
                    GameObject *process = manager_get(&manager,id);
                    if(process!=NULL)
                    {
                     // printf(" request info de [%d] from %d  (%d %d)   %s \n",id,sendId,process->x,process->y,process->name);  
                      update_network_object(process); 
                    }

                  //  printf(" [%d] (%d %d)   %s \n",id,x,y,name);
                
                    break;
                 }
                case MSG_PLAYER_SHOOT:
                 {
                    int id = read_int(&buffer);

                   // printf(" SHOOT %d \n",id);
                    int x = read_int(&buffer);
                    int y = read_int(&buffer);
                    int angle = read_int(&buffer);

                    GameObject *bullet = newGameObject("bullet",id);
                    bullet->x=x;
                    bullet->y=y;
                    bullet->angle =angle;
                    bullet->type=BULLET;
                    bullet->life=1.0f;
                    manager_add(&manager_objects,bullet);
                
                    break;
                 }
                 case MSG_PLAYER_RESPAW:
                 {
                    int id = read_int(&buffer);
                    int x = read_int(&buffer);
                    int y = read_int(&buffer);
                    int angle = read_int(&buffer);
                    char name[55];
                    read_string(&buffer,name);

                    printf ("RESPAW %d %d %d %d %s \n",id,x,y,angle,name);
                    GameObject *jointer;
                    jointer = newGameObject(name,id);
                    jointer->local=false;
                    jointer->x=x;
                    jointer->y=y;
                    jointer->angle=angle=angle;
                    manager_add(&manager,jointer);
                    send_request_list(client,id);
                  

                    break;
                 }
                 case MSG_PLAYER_KILL:
                 {
                     int id = read_int(&buffer);
                     int id_killer = read_int(&buffer);


                    GameObject *remove = manager_get(&manager,id);
                    GameObject *points = manager_get(&manager,id_killer);
                    if (remove)
                        remove->alive=false;
                    if (points)
                        points->score+=10;


                    printf(" player %d kill  %d \n",id_killer,id);
        
                    break;
                 }
 
            }
            
        }
    }
    
    free_manager(&manager);
    free_manager(&manager_objects);
    TraceLog(LOG_INFO, "End  theread");
    return NULL;
}




bool Collide(GameObject *a, GameObject *b)
{
     if (a==NULL && b==NULL)
         return false;

    if (a->id == b->id)
        return false;

    if (a->respaw)
        return false;

    if (b->respaw)
        return false;

    if (!a->alive)
        return false;

    if (!b->alive)
        return false;
    if (a->energia<=0)
        return false;

    if (b->energia<=0)
        return false;

    Vector2 pos1=(Vector2){a->x,a->y};
    Vector2 pos2=(Vector2){b->x,b->y};

    return CheckCollisionCircles(pos1,a->radius,pos2,b->radius);


}

typedef void (*CollideCallback)(GameObject*,GameObject*);

bool checkCollisions(ObjectManager* manager1, ObjectManager* manager2, CollideCallback event) 
{
    GameObject *list1=manager1->head;
    while (list1 != NULL) 
    {
        GameObject *list2=manager2->head;
        while (list2 != NULL ) 
        {
            if (Collide(list1, list2)) 
            {
                event(list1,list2);
                return true; 
            }
            list2 = list2->next;
        }
        list1 = list1->next;
    }
    return false; 
}

void OnCollide(GameObject *a, GameObject *b)
{
        if (a->type==BULLET)
        {
            a->alive=false;
        }
        
        if (b->type==BULLET)
        {
            b->alive=false;
        }

        if (a->type==SHIP)
        {
            
            a->hit=true;
            a->times[0]=1;
            a->energia-=10;
            a->last_id_hit=b->id;
        }
        
        if (b->type==SHIP)
        {
            b->energia-=10;
            b->hit=true;
            b->times[0]=1;
            b->last_id_hit=a->id;
        }
            
     //   printf("collide %d  %d \n",a->id,b->id);

}


int main()
{

 
   

#ifndef GRAPH

    InitWindow(screenWidth, screenHeight, "main");



    int textChatScrollIndex = 0;
    int textChatActive = 0;
    int textChatFocus = 0;

    int bullets=1000;


bool setName=false;
const char newName[25]={'\0'};

    char senMessage[128] = {'\0'};

    bool enable_chat=false;

    init_manager(&manager); 
    init_manager(&manager_objects);


    Texture2D image = LoadTexture("assets/nave1.png");

    chat_init();

   int countdown = 5; 
   double countdownStartTime = GetTime();


   
    SetTargetFPS(60);

    client = connect_client(1479,"192.168.1.77");

     double lastMessageTime = GetTime(); 
     bool isMoving=true;


   
    if (client>0)
    {
        isOffLIne=false;

        int error = pthread_create(&threadId, NULL, &ServerThread, NULL);
        if (error != 0)
        {
                strcpy(errors, TextFormat("THEREAD: %s",strerror(errno)));
                TraceLog(LOG_ERROR, errors);
        }else
            pthread_detach(threadId);
    } else
    {
        isOffLIne=true;

           
        player = newGameObject("offline",5);
        player->score = 0;
        player->local=true;
        player->alive=true;
        player->x=GetRandomValue(40,screenWidth-40);
        player->y=GetRandomValue(40,screenHeight-40);
        player->angle =GetRandomValue(0,360);
        manager_add(&manager,player);

        {

        GameObject *obj = newGameObject("enemy",10);
        obj->score = 0;
        obj->local=true;
        obj->alive=true;
        obj->x=GetRandomValue(40,screenWidth-40);
        obj->y=GetRandomValue(40,screenHeight-40);
        obj->angle =GetRandomValue(0,360);
        manager_add(&manager,obj);


        }


        {

        GameObject *obj = newGameObject("enemy",11);
        obj->score = 0;
        obj->local=true;
        obj->alive=true;
        obj->x=GetRandomValue(40,screenWidth-40);
        obj->y=GetRandomValue(40,screenHeight-40);
        obj->angle =GetRandomValue(0,360);
        manager_add(&manager,obj);


        }
    }

    while (!WindowShouldClose())
    {



            double currentTime = GetTime();

            BeginDrawing();
            ClearBackground((Color){0, 0, 45, 255});




            if (isOffLIne)
            {
                      
                      //  DrawCircle((int)rotatedX, (int)rotatedY, 4, RED);

                    float translatedX = 2.0f  + player->x;
                    float translatedY = -10.0f + player->y;

                    float angleRad = player->angle * DEG2RAD ;

                    float cosAngle = cosf(angleRad);
                    float sinAngle = sinf(angleRad);

                    float rotatedX = cosAngle * (translatedX - player->x) - sinAngle * (translatedY - player->y) + player->x;
                    float rotatedY = sinAngle * (translatedX - player->x) + cosAngle * (translatedY - player->y) + player->y;


                    if (IsKeyDown(KEY_SPACE))
                    {
                        if (currentTime - bulletTime >= BULLET_INTERVAL) 
                        {
                            GameObject *bullet = newGameObject("bullet",player->id);
                            bullet->x=floor(rotatedX);
                            bullet->y=floor(rotatedY);
                            bullet->angle =player->angle;
                            bullet->type=BULLET;
                            bullet->life=1.0f;
                            manager_add(&manager_objects,bullet);
                            bulletTime = currentTime; 
                        }
                    }


                    if (IsKeyDown(KEY_A))
                    {
                            player->angle-=4;   
                    }else
                    if (IsKeyDown(KEY_D))
                    {
                            player->angle+=4;  
                    }
                    float naveSpeed = 5;

                
                    if (IsKeyDown(KEY_W))
                    {
                                player->x += cosf((-90 * DEG2RAD ) +  DEG2RAD * player->angle ) * naveSpeed;
                                player->y += sinf((-90 * DEG2RAD ) +  DEG2RAD * player->angle ) * naveSpeed;
                
                    }

   
                    GameObject *current = manager.head;
                    while(current)
                    {

                        switch (current->type)
                        {
                            case SHIP:
                            {

                                Rectangle src=(Rectangle){0,0,image.width,image.height};
                                Rectangle dst=(Rectangle){current->x,current->y,src.width+5,src.height+5};
                                DrawCircleLines(current->x,current->y,current->radius,RED);
                                DrawTexturePro(image,src,dst,(Vector2){src.width/2,src.height/2},current->angle,(Color){255,255,255,255} );
                                if (current->hit)
                                {

                                     current->times[0]-=currentTime * 0.01f;
                                    
                                   
                                    DrawRectangleLines(current->x-src.width/2 +2,current->y+(src.height/2),src.width-2,10,WHITE);
                                    int energia = current->energia;

                                    float larguraRemain = (float)energia / 100.0f * (src.width - 2);

                                    if (energia>0 && energia <25)
                                        DrawRectangle(current->x - src.width / 2 + 2, current->y + (src.height / 2), larguraRemain, 10, RED);
                                    else
                                    if (energia>=25 && energia <50)
                                        DrawRectangle(current->x - src.width / 2 + 2, current->y + (src.height / 2), larguraRemain, 10, ORANGE);
                                    else
                                    if (energia>=50 && energia <75)
                                        DrawRectangle(current->x - src.width / 2 + 2, current->y + (src.height / 2), larguraRemain, 10, YELLOW);
                                    else
                                    if (energia>=75)
                                        DrawRectangle(current->x - src.width / 2 + 2, current->y + (src.height / 2), larguraRemain, 10, GREEN);
                                    
                                    if (current->times[0]<=0)
                                    {
                                        
                                        current->hit=false;
                                    }
                                }
                                if (current->energia<=0)
                                {
                                    current->respaw=true;
                                }
                                
                                break;
                            }
                        }
                        current=current->next;
                    }


                    GameObject *current_objs = manager_objects.head;
                    while(current_objs)
                    {
                        switch (current_objs->type)
                        {
                            case BULLET:
                            {
                                current_objs->x += cosf((-90 * DEG2RAD ) +  DEG2RAD * current_objs->angle ) * 8;
                                current_objs->y += sinf((-90 * DEG2RAD ) +  DEG2RAD * current_objs->angle ) * 8;
                                current_objs->life-=0.01f;
                                if (current_objs->life<=0)
                                    current_objs->alive=false;
                                if (current_objs->alive)
                                    DrawCircle(current_objs->x,current_objs->y,4,RED);
                                break;
                            }
                        }
                        current_objs=current_objs->next;
                    }

                    manager_clean(&manager);
                    manager_clean(&manager_objects);

                    checkCollisions(&manager,&manager_objects,OnCollide);

            } 
            else
            //*************************************************************************************************************************************************
            {

            if (playsIsKill)
            {

                if (currentTime - countdownStartTime >= 1.0) 
                {
                    countdown--;
                    countdownStartTime = currentTime;
                }
            int w = MeasureText(TextFormat("Respawn in: %d", countdown),30);
            DrawText(TextFormat("Respawn in: %d", countdown), screenWidth/2-(w/2), screenHeight/2, 30, DARKGRAY);

            if (countdown <= 0) 
            {
                countdown = 5; 

                player=NULL;
                respaw_object("respaw");
                playsIsKill=false;
            }


              

            } else
            {
                
            
                    if (IsKeyPressed(KEY_F2))
                    {
                        setName = !setName;   
                        if (setName)
                        {
                            bzero(&newName, 25);
                        }
                    }


                if (IsKeyPressed(KEY_F1))
                {
                    enable_chat = !enable_chat;   
                    if (enable_chat)
                    {
                        chat_clean();
                        bzero(&senMessage, 128);
                        messageTime = currentTime; 
                    }
                }
                if (newMessage)
                {
                    if (currentTime - messageTime >= CHAT_INTERVAL) 
                    {
                        
                        newMessage = false;
                        messageTime = currentTime; 
                    }
                }

            if (newMessage)
            {
                draw_chat();
            }

               
            if (setName)
            {
                enable_chat=false;
                if (GuiTextBox((Rectangle){20, screenHeight - 24, 100, 20}, newName, 25, true))
                {

                    if(player)
                    {
                        strcpy(player->name,newName);
                    }
                    bzero(&newName, 25);
                    setName=false;
                }
            }
            if (enable_chat)
            {
                setName=false;
             if (GuiTextBox((Rectangle){20, screenHeight - 24, screenWidth - 100, 20}, senMessage, 128, enable_chat))
            {

                int len = strlen(senMessage);

                if (len>=3)
                {
                char data[256];
                Buffer buffer;
                buffer_set(&buffer, data, 256);   
                write_int(&buffer,MSG_PLAYER_CHAT);
                write_string(&buffer,senMessage);
                client_sendbuffer(client,&buffer);
                add_chat_msg(TextFormat("%d :%s",player->id,senMessage));
                }

                messageTime = GetTime();
                newMessage=true;

                bzero(&senMessage, 128);
            }


          
            }

          

            if (player!=NULL)

            {


                        if (player->energia<=0)
                        {
                                
                                player->alive=false;
                                

                                char data[128];
                                Buffer buffer;
                                buffer_set(&buffer, data, 128);   
                                write_int(&buffer,MSG_PLAYER_KILL);
                                write_int(&buffer,player->id);
                                write_int(&buffer,player->last_id_hit);
                                client_sendbuffer(client,&buffer);
                                printf (" send kill %d %d \n",player->id,player->last_id_hit);
                                playsIsKill=true;
                        }

                        float translatedX = 2.0f  + player->x;
                        float translatedY = -10.0f + player->y;
                        float angleRad = player->angle * DEG2RAD ;
                        float cosAngle = cosf(angleRad);
                        float sinAngle = sinf(angleRad);
                        float rotatedX = cosAngle * (translatedX - player->x) - sinAngle * (translatedY - player->y) + player->x;
                        float rotatedY = sinAngle * (translatedX - player->x) + cosAngle * (translatedY - player->y) + player->y;

                if (!setName && !enable_chat)
                {

                    if (IsKeyDown(KEY_SPACE))
                    {
                        if (currentTime - bulletTime >= BULLET_INTERVAL) 
                        {
                            GameObject *bullet = newGameObject("bullet",player->id);
                            bullet->x=floor(rotatedX);
                            bullet->y=floor(rotatedY);
                            bullet->angle =player->angle;
                            bullet->type=BULLET;
                            bullet->life=1.0f;
                            manager_add(&manager_objects,bullet);
                            bulletTime = currentTime; 
                            player_shoot(rotatedX,rotatedY,player->angle);
                        }
                    }

                    if (IsKeyDown(KEY_A))
                    {
                            player->angle-=4;   
                            isMoving=true;          
                    }else
                    if (IsKeyDown(KEY_D))
                    {
                            player->angle+=4;  
                            isMoving=true;           
                    }
                    float naveSpeed = 5;

                
                    if (IsKeyDown(KEY_W))
                    {
                                player->x += cosf((-90 * DEG2RAD ) +  DEG2RAD * player->angle ) * naveSpeed;
                                player->y += sinf((-90 * DEG2RAD ) +  DEG2RAD * player->angle ) * naveSpeed;
                                isMoving=true;

                    }
                }

                        
                
                            if (currentTime - lastMessageTime >= MESSAGE_INTERVAL) 
                            {
                                if (isMoving)
                                {
                                    if (player!=NULL)
                                    {
                                    update_network_object(player);
                                    }

                                    isMoving=false;
                                }

                                lastMessageTime = currentTime; 
                            }
            }
                    DrawText(TextFormat("ID:%d %s POINTS:%d",player->id,player->name,player->score),10,10,12,GREEN);
                    
            }
        
                    GameObject *current = manager.head;
                    while(current)
                    {

                        switch (current->type)
                        {
                            case SHIP:
                            {

                              
                                Rectangle src=(Rectangle){0,0,image.width,image.height};
                                Rectangle dst=(Rectangle){current->x,current->y,src.width+5,src.height+5};
                                DrawCircleLines(current->x,current->y,current->radius,RED);
                                DrawTexturePro(image,src,dst,(Vector2){src.width/2,src.height/2},current->angle,(Color){255,255,255,255} );


                                if (current->local)
                                    DrawText(TextFormat("ID:%d %s",current->id,current->name),current->x-src.width/2,current->y-src.height,12,RED);
                                else
                                    DrawText(TextFormat("ID:%d %s",current->id,current->name),current->x-src.width/2,current->y-src.height,12,GREEN);
                    

                                if (current->hit)
                                {

                                     current->times[0]-=currentTime * 0.01f;
                                    
                                   
                                    DrawRectangleLines(current->x-src.width/2 +2,current->y+(src.height/2),src.width-2,10,WHITE);
                                    int energia = current->energia;

                                    float larguraRemain = (float)energia / 100.0f * (src.width - 2);

                                    if (energia>0 && energia <25)
                                        DrawRectangle(current->x - src.width / 2 + 2, current->y + (src.height / 2), larguraRemain, 10, RED);
                                    else
                                    if (energia>=25 && energia <50)
                                        DrawRectangle(current->x - src.width / 2 + 2, current->y + (src.height / 2), larguraRemain, 10, ORANGE);
                                    else
                                    if (energia>=50 && energia <75)
                                        DrawRectangle(current->x - src.width / 2 + 2, current->y + (src.height / 2), larguraRemain, 10, YELLOW);
                                    else
                                    if (energia>=75)
                                        DrawRectangle(current->x - src.width / 2 + 2, current->y + (src.height / 2), larguraRemain, 10, GREEN);
                                    
                    
                                    if (current->times[0]<=0)
                                    {
                                        current->hit=false;
                                    }
                                }
                                
                               
                                break;
                            }
                        }
                        current=current->next;
                    }


                    GameObject *current_objs = manager_objects.head;
                    while(current_objs)
                    {
                        switch (current_objs->type)
                        {
                            case BULLET:
                            {
                                current_objs->x += cosf((-90 * DEG2RAD ) +  DEG2RAD * current_objs->angle ) * 8;
                                current_objs->y += sinf((-90 * DEG2RAD ) +  DEG2RAD * current_objs->angle ) * 8;
                                current_objs->life-=0.01f;
                                if (current_objs->life<=0)
                                    current_objs->alive=false;
                                if (current_objs->alive)
                                    DrawCircle(current_objs->x,current_objs->y,4,RED);
                                break;
                            }
                        }
                        current_objs=current_objs->next;
                    }

                    manager_clean(&manager_objects);
                    manager_clean(&manager);
                    checkCollisions(&manager,&manager_objects,OnCollide);    

        }
        EndDrawing();
    }
    chat_free();
    UnloadTexture(image);
    free_manager(&manager);
    free_manager(&manager_objects);
    CloseWindow();
#endif

    
    close_client(client);
    printf("EXIT\n");

 
    return 0;
}
