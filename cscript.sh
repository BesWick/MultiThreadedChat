#!/bin/bash
#client code script

make clean 
make
./client 127.0.0.1 123$1 $2
