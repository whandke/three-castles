#!/bin/bash
#Run this in terminal
clear
printf "\n\n[INFO] Compiling server...\n\n"
gcc server.c -Wall -lncurses -o server.exec
printf "\n\n[INFO] Compiling client...\n\n"
gcc client.c -Wall -lncurses -o client.exec
printf "\n\n"