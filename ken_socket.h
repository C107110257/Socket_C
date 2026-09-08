#ifndef KEN_SOCKET_H

#define KEN_SOCKET_H

#include <pthread.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h>

#include <arpa/inet.h>

#define APP_NAME "TCP Client/Server Demo"
#define VERSION "26.08.11"
#define AUTHOR "Ken Lin"
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

#define PORT 8787

typedef struct
{
    int sock;
    char name[32];
} ConnInfo;

void run_client(char *server_ip);

void run_server();

#endif