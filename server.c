/**
* CE156: Final Assignment
* 
* File: myserver.c
* 
* Author: Alexander Dobrota (adobrota)
* Date: 7 March 2019
**/

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <readline/readline.h>



#define MAXCLIENTS 10   //# of clients server can handle at a time

typedef struct{
    char * id;  
    char * ip;
    int port;
} clientinfo;

typedef struct{
    int clientip;
    int arg3;
}arg_struct;

typedef struct{
    int socket;
    int clientport;
    char * clientip;
    int clientnum;
}worker_args;


  
clientinfo clients[MAXCLIENTS];
clientinfo waitinglist[MAXCLIENTS];

static int openspotindex = 0;
static int waitingopenindex = 0;

static int clientssize = 0;
static int waitlistsize = 0;

pthread_mutex_t lock;
pthread_mutex_t lock2;



int readclientbuffer(int socket, clientinfo * myclient);
int handleGetListCmd(char * buffer, int socket);
 int checkWaitingListforID(char * requestedclient);


/**
 * 
 * 
 * return   the socket fd
 */
int createBindedPort(int port){
    //CREATING SOCKET 
    int sockfd; struct sockaddr_in serv_addr;
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        perror("error: failed creating socket\n");
        exit(1);
    }
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if((bind(sockfd,(struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0){
        perror("error: binding socket\n");
        exit(1);
    }
    
    return sockfd;
    
}

int   createConnectedServerPort(int port){
    //CREATING SOCKET 
    int sockfd; struct sockaddr_in serv_addr;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("error: failed creating socket\n");
        exit(1);
    }
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   


    //CONNECTING SOCKET
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        fprintf(stderr,"error: couldn't connect socket with server, \n");
        exit(1);
    }
    printf("connected socket\n");
    
    return sockfd;
    
}


void inputclientdata(clientinfo * myclient){

    pthread_mutex_lock(&lock);
    printf("waitingopenindex = %d\n",waitingopenindex );
    printf("waitlistsize = %d\n",waitlistsize);
    waitinglist[waitingopenindex].id = myclient->id;
    waitinglist[waitingopenindex].port = myclient->port;


    clients[openspotindex].id = myclient->id;
    clients[openspotindex].ip = myclient->ip;
    clients[openspotindex].port = myclient->port;


    // printf("struct  client[%d] IP address is: %s\n", waitlistsize, clients[waitlistsize].ip);
    // printf("struct client[%d] port: %d\n", waitlistsize, clients[waitlistsize].port);
    // printf("struct client[%d] id:%s\n", waitlistsize,clients[waitlistsize].id);
    // printf("\n");

    openspotindex ++; clientssize ++;
    waitingopenindex++; waitlistsize ++;
    pthread_mutex_unlock(&lock);
  
    // free(myclient->ip);
    // free(myclient->id);
    // free(myclient);

}

clientinfo * initClientData(char * ip, char * id, int port, int size){

    clientinfo * myclient = malloc(sizeof(clientinfo));

    myclient->ip = (char*)malloc(sizeof(char) * 100);
    myclient->id = (char*)malloc(sizeof(char) * size);
    myclient->port = port;


    // printf("IP address is: %s\n", ip);
    // printf("port is: %d\n", port);
    // printf("client id:%s\n", id);
    // printf("\n");

    strcpy(myclient->ip, ip);
    strcpy(myclient->id, id);



    // printf("init IP address is: %s\n", myclient->ip);
    // printf("init port: %d\n", myclient->port);
    // printf("init client id:%s\n", myclient->id);
    // printf("\n");
    

    return myclient;


}


void sortwaitinglistinalphabetical(){

    int size = waitlistsize;
    printf("size = %d\n", waitlistsize);
    char temp[101];
    
    for(int i=0;i< size -1;i++)
      for(int j=i+1;j <size;j++){
         if(strcmp(waitinglist[i].id,waitinglist[j].id)>0){
            strcpy(temp,waitinglist[i].id);
            strcpy(waitinglist[i].id,waitinglist[j].id);
            strcpy(waitinglist[j].id,temp);
         }
      }
    
 printf("Order of Sorted Strings:\n");
   for(int i=0;i< size;i++){
      printf("%s\n", waitinglist[i].id);
   }




}



void *  handleConnection(void * args){
    char clientbuff[512];
    bzero(clientbuff, sizeof(clientbuff));
    worker_args * data = (worker_args *)args;

    int clientsock = data->socket;

    read(clientsock, clientbuff, sizeof(clientbuff));


    // int usernamesize = strlen(clientbuff);
    // char username[usernamesize];
    // strcpy(username, clientbuff);
    clientinfo * myclient = initClientData(data->clientip, clientbuff, data->clientport, strlen(clientbuff));

    
    puts("after init");
    // printf("for client#%d\n", data->clientnum);
    // printf("handle IP address is: %s\n", myclient->ip);
    // printf("handle port: %d\n", myclient->port);
    // printf("handle client id:%s\n", myclient->id);
    // printf("\n");

    

    //inputclientdata(myclient);

    readclientbuffer(clientsock, myclient);
    puts("after readbuffer()");



    puts("done handling");

     close(clientsock);

     pthread_exit(NULL);

}


int  readclientbuffer(int socket, clientinfo * myclient){


    puts("trying to read clientbuffer now");
    // printf("port = %d\n", port);
    char buffer[512];
    // printf("socket = %d\n", socket);
    while(1){
        bzero(buffer, sizeof(buffer));
        printf("trying to read command from client %s\n", myclient->id);
        int recvsize = recv(socket, buffer, sizeof(buffer), 0);

        if(recvsize > 0){
        printf("buffer = %s\n", buffer);
            puts("got msg");
            if(strcmp(buffer, "getlist")== 0){
                puts("handling getlistcommand");
                bzero(buffer, sizeof(buffer));
                puts("before handlinglist");
                handleGetListCmd(buffer,socket);
                puts("after handlinglist");
                bzero(buffer, sizeof(buffer));
                
            }
            if(strcmp(buffer, "waiting")== 0){
                puts("got waiting msg");
                 bzero(buffer, sizeof(buffer));
                 recv(socket, buffer, sizeof(buffer), 0);
                 printf("port = %s\n",buffer);
                 int listenport = atoi(buffer);
                //  printf("listeningport = %d\n", listenport);
                 myclient->port = listenport;
                 printf("MYCLIENT-> port = %d\n", myclient->port);
                 inputclientdata(myclient);

            }
            if(strcmp(buffer, "chat")== 0){
                puts("got chat msg");
                 bzero(buffer, sizeof(buffer));
                 recv(socket, buffer, sizeof(buffer), 0);
                 char reqid[strlen(buffer)];
                 strcpy(reqid, buffer);
                 printf("requested id = %s\n",reqid);
                 int foundidwithport = checkWaitingListforID(buffer);
                 printf("port num is %d\n", foundidwithport);
                 
                char datareq[512];
                

                 //SENDING the REQ CLIENT DATA
                  sprintf(datareq, "connectreq %d username %s", foundidwithport, reqid);
                 //sending req client port + 
                 //sending req client id
                 write(socket, datareq, strlen(datareq));

            }
        }
        else{
            break;
        }
        
    
    }
     

    return 0;

}

    /**
     * 
     * Checks to see if the waiting queue has the requested id
     * 
     * Returns the port num of requested client if found
     *         -1, Otherwise
     * */
    int checkWaitingListforID(char * requestedclient){
        int size = waitlistsize;
        int port = -1;
        int foundclientindex = 0;

        for(int i=0;i< size;i++){
          if(strcmp(waitinglist[i].id, requestedclient) == 0){
              puts("found requested client");
              port =  waitinglist[i].port;
              foundclientindex = i;
          }
        }

        pthread_mutex_lock(&lock);

        openspotindex = foundclientindex; clientssize --;
        waitingopenindex= foundclientindex; waitlistsize --;   

        pthread_mutex_unlock(&lock);
        


          return port;
   

    }








int handleGetListCmd(char * buffer, int socket){
            
            
            // char waitsizetemp[waitlistsize];
                sortwaitinglistinalphabetical();
                printf("waitlistsize = %d\n", waitlistsize);
                int size = 0;
                char temp[size];
                printf("PRINT %s\n", waitinglist[0].id);
                
                if(waitlistsize > 0){
                    size = 0;
                    for(int i=0;i< waitlistsize;i++){
                        size += strlen(waitinglist[i].id);
                        
                    }
                    // printf("size = %d\n", size);
                    strcpy(temp, waitinglist[0].id);
                    printf("TEMP HAS: %s\n", temp);
                    sprintf(temp, "%s\n", temp);
                    for( int i = 1; i<waitlistsize; i++){
                        strcat(temp, waitinglist[i].id);
                        sprintf(temp, "%s\n",temp);
                        
                    }
                    //int shit = sprintf(buffer, "%d) %s\n", j, waitinglist[i].id);
                    // printf("size of buff is %d\n", size);
                    printf("temp contains:%s\n", temp);
                }
                else{
                    size = 0;
                    bzero(temp, sizeof(temp));
                    strcpy(temp, " ");
                    
                }

                // write(socket, &size, sizeof(size));
                // puts("wrote size ot scoket");
                
                //sending requested list to client
                write(socket, temp, strlen(temp)); 
                puts("wrote temp");
                return 0;

}

int main(int argc, char * argv[]){

struct sockaddr_in client_addr;
socklen_t client_size = sizeof(client_addr);

pthread_t tid; 

// worker_args * threadargs = calloc(1, sizeof(worker_args));


if(argc != 2){
        perror("expected input: ./myserver <port number> \n");
        exit(1);
    }
    
    int port;
    if((port = atoi(argv[1])) < 0){
        perror("invalid port \n");
        exit(1);
    }

int clientlistensockfd = createBindedPort(port);    //creating socket for client
    
    //LISTENING TO CLIENT SOCKET
    if(listen(clientlistensockfd, MAXCLIENTS) < 0){
        perror("error: listen() failed");
        exit(1);
    }
    int i = 0;
    //Main LOOP
    while(1){
        int clientfd   = accept(clientlistensockfd,(struct sockaddr *) &client_addr, &client_size);
        char * clientip = inet_ntoa(client_addr.sin_addr);
        int clientport = (int) ntohs(client_addr.sin_port);
        i++;

            worker_args * threadargs = calloc(1, sizeof(worker_args));
            threadargs->clientnum = i;
            threadargs->socket = clientfd;
            threadargs->clientport = clientport;
            threadargs->clientip = (char*)malloc(sizeof(char) * 100);
            strcpy(threadargs->clientip, clientip);
        if( pthread_create( &tid , NULL ,  handleConnection , (void*) threadargs) < 0){
            perror("could not create thread");
            return 1;
        }
        
    printf("finished with thread #%d\n", i);


    }
    return 0;

}