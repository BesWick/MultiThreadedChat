##Makefile for the Final
## Author: Alexander Dobrota
## Date: 01/24/19
CC = gcc
CFLAGS  = -g -Wall -O0 -std=gnu99 -D_DEFAULT_SOURCE
HELPER = ./src/helperfunc.c

all: client server	

client: client.c
	${CC} ${CFLAGS} -o  client client.c -lreadline -lpthread
	
server: server.c
	${CC} ${CFLAGS} -o server server.c -lreadline -lpthread

clean:
	${RM} client server 