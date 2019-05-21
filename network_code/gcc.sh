#!/bin/sh


gcc tcpclientsend.c -o tcpclientsend
gcc tcpserver.c -o tcpserver

./tcpserver 192.168.1.103 8000
./tcpclientsend 192.168.1.103 8000

