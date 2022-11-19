#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "tecnicofs-client-api.h"
#include "tecnicofs-api-constants.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#define MAX_COMMAND 512


int sockfd;
int mount = 0;


int tfsMount(char * address){
    if(mount!=0){
        return TECNICOFS_ERROR_OPEN_SESSION;
    }
    mount++;
    struct sockaddr_un serv_addr;
    int servlen;
    if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0) ) < 0)
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, address);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

    if(connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0){
        return TECNICOFS_ERROR_CONNECTION_ERROR;
    }
    return 0;
}


int tfsCreate(char *filename, permission ownerPermissions, permission othersPermissions){
    if (mount != 1){
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    enum permission owner = ownerPermissions;
    enum permission others = othersPermissions;
    char permissions[3];
    char command[MAX_COMMAND];
    sprintf(permissions, "%d", owner*10+others);
    sprintf(command, "c %s %s", filename, permissions);
    write(sockfd, command, MAX_COMMAND);
    read(sockfd, command, MAX_COMMAND);
    int j = atoi(command);
    return j;
}

int tfsDelete(char *filename){
    if (mount != 1){
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }

    char command[MAX_COMMAND];
    sprintf(command, "d %s", filename);
    write(sockfd, command, MAX_COMMAND);
    read(sockfd, command, MAX_COMMAND);
    int j = atoi(command);
    return j;
}

int tfsRename(char *filenameOld, char *filenameNew){
    if (mount != 1){
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    char command[MAX_COMMAND];
    sprintf(command, "r %s %s", filenameOld, filenameNew);
    write(sockfd, command, MAX_COMMAND);
    read(sockfd, command, MAX_COMMAND);
    int j = atoi(command);
    return j;
}

int tfsOpen(char *filename, permission mode){
    if (mount != 1){
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    enum permission modo = mode;
    char permission[2];
    char command[MAX_COMMAND];
    sprintf(permission, "%d", modo);
    sprintf(command, "o %s %s", filename, permission);
    write(sockfd, command, MAX_COMMAND);
    read(sockfd, command, MAX_COMMAND);
    int j = atoi(command);
    return j;
}

int tfsClose(int fd){
    if (mount != 1){
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    char command[MAX_COMMAND];
    sprintf(command, "x %d", fd);
    write(sockfd, command, MAX_COMMAND);
    read(sockfd, command, MAX_COMMAND);
    int j = atoi(command);
    return j;
}

int tfsRead(int fd, char *buffer, int len){
    int j=0;
    if (mount != 1){
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    char command[MAX_COMMAND];
    sprintf(command, "l %d %s %d", fd, buffer, len);
    write(sockfd, command, MAX_COMMAND);
    read(sockfd, command, MAX_COMMAND);
    sscanf(command, "%d %s", &j, command);
    strcpy(buffer, command);
    if (j==0){
        return strlen(command);
    }
    return j;
}

int tfsWrite(int fd, char *buffer, int len){
    if (mount != 1){
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    char command[MAX_COMMAND];
    sprintf(command, "w %d %s %d", fd, buffer, len);
    write(sockfd, command, MAX_COMMAND);
    read(sockfd, command, MAX_COMMAND);
    int j = atoi(command);
    return j;
}

int tfsUnmount(){
    if (mount != 1){
        return TECNICOFS_ERROR_NO_OPEN_SESSION;
    }
    close(sockfd);
    return 0;
}
