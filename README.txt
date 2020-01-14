Author: Alexander Dobrota (adobrota)

Final HW: Chat Program

What is this? 
    A TCP-based chat program composed of a client for the end users and a server program which will be used to connect clients so that they can exchange messages. The client operates in three modes: asking the server for information (INFO), waiting to be contacted by another client (WAIT), or exchanging messages with another client (CHAT). For more information check out my final report!



Files:
    README.txt
    Makefile
        -the makefile in this project
    client.c
        -client code with the main function
        -expected usage: ./client <server-ip> <server port> <client identifier>
    server.c
        -expected usage: ./myserver <port number>
        -server code with the main function
    cscript.c
        -script for client code
        -will run make and make clean for you too
        - Usage = ./cscript <last num of server's port> <username>  
    sscript.c
        -script for server code
        -will run make and make clean for you too
        - Usage = ./sscript 1234
    design.txt  
        -explains the thoughts/logic behind my program
