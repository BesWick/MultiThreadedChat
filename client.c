/**
* CE156: Final Assignment
*
* File: client.c
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
#include <readline/readline.h>
#include <signal.h>
#include <errno.h>


#define INITIAL 0
#define WAIT 1
#define INFO 2
#define CHAT 3

#define STDIN 0  // file descriptor for standard input
static volatile int Ctrl_C = 0;
void intHandler(int dummy) {
    signal(SIGINT, intHandler);
    Ctrl_C = 1;
}
/**
 *
 *
 * return   the socket fd
 */
int createConnectedServerPort(int port, char ** args){
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
    if (inet_pton(AF_INET, args[1], &serv_addr.sin_addr) <= 0){
        perror("error: failed inet_pton() for ip \n");
        exit(1);
    }


    //CONNECTING SOCKET
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        fprintf(stderr,"error: couldn't connect socket with server, \n");
        exit(1);
    }
    printf("connected socket\n");

    return sockfd;

}

int createChatConnection(int port){
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
    printf("connected SUCCESSFULLY to socket at port%d\n", port);

    return sockfd;

}



int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;

    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
} 


void writeprompt(char * username){
    printf("%s>",username);
    fflush(stdout);
}



int main(int argc, char * argv[]){

    int port;
    if(argc != 4){
        perror("expected input: ./client <server-ip> <server port> <client identifier> \n");
        exit(1);
    }

    char * id = (char *)calloc(strlen(argv[3]), sizeof(char));
    strcpy(id, argv[3]);
    printf("id = %s\n", id);

    if((port = atoi(argv[2])) < 0){
        perror("invalid port \n");
        exit(1);
    }

    signal(SIGINT, intHandler);

    int state = INITIAL; // *state variable*

    int serverfd = createConnectedServerPort(port, argv);

    char * username = malloc(strlen(id) + 2);
    strcpy(username, id);

    char other_username[512];

    write(serverfd, id, strlen(id) );

    char * userinput;
    
    size_t userinputsize = 512;
    size_t characters;
    userinput = (char *)malloc(userinputsize * sizeof(char));



    char requestedid[100];


    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    

    char buf[1024];

    
    //main loop
//---------------

    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    int listener;    //descriptor for listener socket when in WAIT state
   // int serverfd;        // socket descriptor for the server
    int clientfd;        // newly accept()ed socket descriptor for client
    

            
    FD_ZERO(&master);    // clear the master and temp sets
    FD_ZERO(&read_fds);

    // add STDIN to the master set
    FD_SET(STDIN, &master);
    // add the server socket to the master set
    FD_SET(serverfd, &master);


    fdmax = serverfd;
    listener = -1; // no listener before WAIT state

    state = INFO;

    struct timeval tv;
    //set timeout for select() to 0.5 seconds in order to check for Ctrl-C
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

   writeprompt(username);

    for(;;) {
        read_fds = master; // copy it
         if (select(fdmax+1, &read_fds, NULL, NULL, &tv) == -1) {
            if ( EINTR == errno) {
                FD_ZERO(&read_fds);
            }
            else {
                perror("select");
                exit(4);
            }
         }

        //first handle Ctrl-C
        if (Ctrl_C) {
            if(state == CHAT) {
                close(clientfd);
                FD_CLR(clientfd, &master);// remove from master set
                printf("Left conversation with %s\n", other_username);
                //writeprompt(username); //print prompt
                state = INFO;
            }
            else if(state == WAIT) {
                //exit from WAIT
                close(listener);
                FD_CLR(listener, &master);
                printf("Stopped waiting.\n");
                writeprompt(username); //print prompt
                state = INFO;
            }
            //else don't do anything (?)
            Ctrl_C = 0;
        }

    
        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got data from one fd
                if (i == STDIN){
                    // handle commands from console
                    if((characters = getline(&userinput,&userinputsize,stdin)) > 0){
                        userinput[characters-1] = 0;
                        if(strcmp(userinput, "/quit") == 0){
                            puts("Goodbye");
                            exit(0);
                        }
                        if(state == CHAT) {
                            // printf("\nUSER!@#@#INPUT = %s\n", userinput);
                            // send userinput to connected client using clientfd:
                            if (send(clientfd, userinput, sizeof userinput, 0) == -1) { // USE senall()
                                perror("send");
                            }
                            continue;
                         } else{ //handle user commands
                            if(strcmp(userinput, "/list")== 0){
                                //sendall() to serverfd command to get list
                                if(write(serverfd, "getlist", 10) < 0){
                                    puts("sent getlist err");
                                }
                                // writeprompt(username);
                                continue;
                            }
                            else if(strcmp(userinput, "/wait")== 0){
                                if (state == INFO){ // don't do anything
                                    //port = .....
                                     //first write the waiting msg to server
                                    if(write(serverfd, "waiting", 10) < 0){
                                        puts("sending waiting err");
                                    }

            	                    int myPort;
                                    
                                    puts("before listen");
                                    listener = socket(AF_INET, SOCK_STREAM, 0);

                                    listen(listener, 10);

                                     // add the listener to the master set
                                    FD_SET(listener, &master);

                                    if (listener > fdmax) {    // keep track of the max
                                        fdmax = listener;
                                    }

                                    puts("after listen");

                                     // Sometime later we want to know what port and IP our socket is listening on
                                    getsockname(listener, (struct sockaddr *)&addr, &addr_size);
                                    myPort = ntohs(addr.sin_port);
                                    char portinstring[100];
                                    sprintf(portinstring, "%d", myPort);

                                   
                                   printf("WAITING PORT is %s\n", portinstring);
                                    //now sending the listening port to the server
                                    write(serverfd,portinstring, strlen(portinstring));

                                    //send serverfd wait status passing the port
                                    state = WAIT;
                                }
                                writeprompt(username);
                                continue;
                            }
                            else if(strstr(userinput, "/connect") != NULL){
                                if (state == INFO){
                                    char garbage[512];
                                    if(sscanf(userinput, "%s %s", garbage, requestedid) == 2){
                                        printf("request to connect to client: %s\n", requestedid);
                                        //send serverfd connect command passing the id
                                        if(write(serverfd, "chat", 10) < 0){
                                            puts("sending chat err");
                                        }
                                        if(write(serverfd, requestedid, strlen(requestedid)) < 0){
                                            puts("error: couldn't send request id");
                                        }
                                        puts("done write connection data request");
                                    }
                                    else{
                                        puts("expected /connect <id>");
                                    }
                                }
                                else {
                                    puts("Wrong command for this state");
                                }
                            }
                            else if(strcmp(userinput, "") != 0){
                                printf("Command '%s' not recognized\n", userinput);

                            }
                         }
                      }
                     writeprompt(username);
                    continue;
                } else if (i == listener) {
                    puts("before accept ");
                    fflush(stdout);
                    // handle new connections in our WAIT state
                    clientfd = accept(listener, (struct sockaddr *)&addr, &addr_size);

                    if (clientfd == -1) {
                        perror("accept");
                        exit(1);
                    } else {
                        puts("accept worked");
                        fflush(stdout);
                        //close the listener because the state will become CHAT from WAIT
                        // close (listener);
                        FD_CLR(listener, &master);// remove from master set

                        FD_SET(clientfd, &master); // add client to master set
                        if (clientfd > fdmax) {    // keep track of the max
                            fdmax = clientfd;
                        }
                        //read first message that contains "Connection from %s", username);
                        char newbuff[512];
                        read(clientfd, newbuff, sizeof(newbuff));
    
                        sscanf(newbuff, "Connection from %s\n", other_username);
                        printf("OTHERUSERNAME = %s\n",other_username);

                        state = CHAT;
                    }
                    writeprompt(username);
                    continue;
                } else if (i == serverfd){
                    int nbytes;
                     bzero(buf, sizeof(buf));
                    //message from the server
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by server
                        if (nbytes == 0) {
                            // connection closed
                            puts("server hung up");
                            puts("bye");
                        } else {
                            perror("recv");
                        }
                        exit(1);
                    } else {
                        // handle server message
                        if (strstr(buf, "connectreq") != NULL){
                                //get "port" and "other_username" from buf coming from server
                                // clientfd = connect to port "port"
                                puts("recieved  connectreq");
                                int requestedport = 0;

                                if(sscanf(buf, "connectreq %d username %s", &requestedport, other_username) != 2){
                                    perror("error with getting other_username");
                                }

                                printf("other port = %d\n", requestedport);
                                printf("other_usename to %s\n", other_username);
                                state = CHAT;

                                clientfd = createChatConnection(requestedport);



                                FD_SET(clientfd, &master); // add to master set
                                if (clientfd > fdmax) {    // keep track of the max
                                    fdmax = clientfd;
                                }
                                //sendall() to clientfd "Connection from <your username>" to the other client to print on its console
                                char tempbuff[512];
                                sprintf(tempbuff, "Connection from %s\n", username);

                                write(clientfd, tempbuff, sizeof(tempbuff)) ;
                        }
                        else {
                            //print everything from the server to the console
                            printf("List is:\n");
                            printf("%s", buf);
                            //writeprompt(username); //print prompt
                        }
                        writeprompt(username);
                        continue;
                    }
                } else if (i == clientfd){
                     bzero(buf, sizeof(buf));
                    int nbytes;
                    // printf("recieved data from other client\n");
                    //print the message that came from the other client
                    if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes != 0) {
                            // connection closed
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                        printf("Exit conversation with %s\n", other_username);
                         //writeprompt(username); //print prompt
                        state = INFO;
                    } else {
                        // we got some data from a client
                        printf("\n%s:%s\n",other_username,buf);
                    }
                      writeprompt(username); //print prompt
                    continue;
                } // END handle data from client
            } // END got new incoming connection
        } // END if (FD_ISSET(i, &read_fds))
    } // END for(;;)

    close(serverfd);

}