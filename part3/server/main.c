#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "fs.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include "tecnicofs-api-constants.h"


#define MAX_INPUT_SIZE 512
#define MAXTHREADS 40

tecnicofs* fs;

pthread_rwlock_t rwlock;
int numberThreads = 0;
int headQueue = 0;
int numberCommands = 0;
int numberBuckets = 1;
int signa = 1;

/* forma de aceder a tabela de inodes */

extern inode_t inode_table[INODE_TABLE_SIZE];

/* char geral que guarda as informacoes lidas do buffer pelo comando "l" */

char geral[256] = "";

/* estrutura de um ficheiro que guarda o estado(aberto/fechado), inumber e modo(permissao) */

typedef struct files {
    int estado;
    int inumber;
    int modo;
} files;


/* funcoes de lock e unlock */

void lockm(){
    if(pthread_rwlock_wrlock(&rwlock) != 0){
        fprintf(stderr, "Impossivel dar rwlock\n");
    }
}

void unlockm(){
    if(pthread_rwlock_unlock(&rwlock) != 0){
        fprintf(stderr, "Impossivel dar rwunlock\n");
    }
}

static void displayUsage (const char* appName){
    fprintf(stderr, "Usage: %s\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 4) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
}


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

/* funcao que verifica as permissoes e retorna 0 caso correspondam e retorna 1 em caso contrario */

int checkPermissions(int Fpermission, int Upermission){
  if (Fpermission == 3 && (Upermission == 1 || Upermission == 2)){
    return 0;
  }
  else if (Upermission == 3 && (Fpermission == 1 || Fpermission == 2)){
    return 0;
  }
  else if(Fpermission == Upermission){
    return 0;
  }
  return 1;
}

/* funcao que inicia o vetor de ficheiros colocando todas as posicoes com o valor -1 */

void initOpenFiles(files file[]){
    for(int x = 0;  x < 5; x++){
        file[x].estado = -1;
        file[x].inumber = -1;
        file[x].modo = -1;
    }
}

/* funcao que retorna o estado do vetor, retornando 2 quando o cliente tem 5 ficheiros abertos e retonando 1 em caso contrario */

int vetorEstado(files file[]){
    int vazios=0;
    for (int k=0; k<5; k++){
        if (file[k].estado == -1)
            vazios++;
    }
    if(vazios==0){
        return 2;
    }
    else{
        return 1;
    }
}

/* funcao que insere um file no vetor de files abertos, sendo estes abertos ou pelo owner ou por qualquer outro user,
        definindo o inumber, estado e permissoes */

void insertVetorOwner(files file[], int new, int permissao){
    for (int k=0; k<5; k++){
        if (file[k].estado == -1){
            file[k].inumber = new;
            file[k].estado = 1;
            file[k].modo = permissao;
            break;
        }
    }
}

void insertVetorUser(files file[], int new, int permissao){
    for (int k=0; k<5; k++){
        if (file[k].estado == -1){
            file[k].inumber = new;
            file[k].estado = 0;
            file[k].modo = permissao;
            break;
        }
    }
}


/* funcao que retorna o indice do ficheiro de certo inumber */

int getIndice(files file[], int inumber){
  for (int k=0; k<5; k++){
      if (file[k].inumber == inumber){
          return k;
      }
  }
  return -5;
}

/* retorna o tipo de user(owner ou user) */

int typeOfUser(files file[], int inumber){
    for (int k=0; k<5; k++){
        if (file[k].inumber == inumber){
            return file[k].estado;
        }
    }
    return -5;
}

/* remove um ficheiro do vetor */

void removeVetor(files file[], int deleted){
    for (int k=0; k<5; k++){
        if (file[k].inumber == deleted){
            file[k].inumber = -1;
            file[k].estado = -1;
            break;
        }
    }
}


int applyCommands(char command[], files file[], uid_t owner){

    int y = 0;
    strcpy(geral, "");
    int permissions = 0, permissao = 0, iNumber, numTokens, searchResult, len = 0;
    char filename[MAX_INPUT_SIZE], errou[MAX_INPUT_SIZE], old_name[MAX_INPUT_SIZE], new_name[MAX_INPUT_SIZE], token;
    uid_t ownerteste;
    permission permissionowner, permissionother;
    char dataInBuffer[MAX_INPUT_SIZE];

    lockm();

    token = command[0];

    switch (token) {

        case 'c':

            numTokens= sscanf(command, "%c %s %d %s", &token, filename, &permissions, errou);
            if (numTokens==4){
                fprintf(stderr, "Input errado no c %s\n", command);
            }
            searchResult = lookup(fs, filename);
            if(searchResult>=0){
                unlockm();
                y = TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                return y;
            }
            int others = permissions%10;
            iNumber = inode_create(owner, permissions/10, others);
            unlockm();
            create(fs, filename, iNumber);
            return y;

        case 'd':

            unlockm();
            numTokens= sscanf(command, "%c %s %s", &token, old_name, errou);
            if (numTokens==3){
                fprintf(stderr, "Input errado no c %s\n", command);
            }
            searchResult = lookup(fs, old_name);
            if (searchResult == -1){
              y = TECNICOFS_ERROR_FILE_NOT_FOUND;
              return y;
            }
            inode_delete(searchResult);
            delete(fs, old_name);
            removeVetor(file, searchResult);
            return y;

        case 'r':

            unlockm();
            int searchResultN = 0;
            numTokens= sscanf(command, "%c %s %s %s", &token, old_name, new_name, errou);
            if (numTokens==4){
                fprintf(stderr, "Input errado no c %s\n", command);
            }
            searchResult = lookup(fs, old_name);
            searchResultN = lookup(fs, new_name);

            /* verifica se o ficheiro que se quer renomear ja existe e se o nome que se quer atribuir
                    nao esta ja atribuido*/

            if(searchResult != -1){
                if(searchResultN == -1){
                    instruction_r(fs, old_name, new_name);
                }
                else{
                    y = TECNICOFS_ERROR_FILE_ALREADY_EXISTS;
                    return y;
                }
            }
            else{
              y = TECNICOFS_ERROR_FILE_NOT_FOUND;
              return y;
            }
            return y;

        case 'w':

            unlockm();
            numTokens= sscanf(command, "%c %d %s %s", &token, &iNumber, dataInBuffer, errou);
            len = strlen(dataInBuffer);
            if(getIndice(file, iNumber)!=-5){
                if ((typeOfUser(file, iNumber) == 1) && (inode_table[iNumber].ownerPermissions == 1 || inode_table[iNumber].ownerPermissions == 3) && (file[getIndice(file, iNumber)].modo == 1 || file[getIndice(file, iNumber)].modo == 3)){
                    if(inode_set(iNumber, dataInBuffer, len)==-1){
                        perror("Errou write owner\n");
                    }
                }
                else if ((typeOfUser(file, iNumber) == 0) && (inode_table[iNumber].othersPermissions == 1 || inode_table[iNumber].othersPermissions == 3) && (file[getIndice(file, iNumber)].modo == 1 || file[getIndice(file, iNumber)].modo == 3)){
                    if(inode_set(iNumber, dataInBuffer, len)==-1){
                        perror("Errou write other\n");
                    }
                }
                else{
                    y = TECNICOFS_ERROR_PERMISSION_DENIED;
                    return y;
                }
            }
            else{
                y = TECNICOFS_ERROR_FILE_NOT_OPEN;
                return y;
            }
            return y;

        case 'l':

            unlockm();
            numTokens= sscanf(command, "%c %d %d %s", &token, &iNumber, &len, errou);
            if(getIndice(file, iNumber)!=-5){
                if ((typeOfUser(file, iNumber) == 1) && (inode_table[iNumber].ownerPermissions == 2 || inode_table[iNumber].ownerPermissions == 3) && (file[getIndice(file, iNumber)].modo == 2 || file[getIndice(file, iNumber)].modo == 3)){
                    inode_get(iNumber, &ownerteste, &permissionowner, &permissionother, geral, len-1);

                }
                else if ((typeOfUser(file, iNumber) == 0) && (inode_table[iNumber].othersPermissions == 2 || inode_table[iNumber].othersPermissions == 3) && (file[getIndice(file, iNumber)].modo == 2 || file[getIndice(file, iNumber)].modo == 3)){
                    inode_get(iNumber, &ownerteste, &permissionowner, &permissionother, geral, len-1);

                }
                else{
                    y = TECNICOFS_ERROR_INVALID_MODE;
                    return y;
                }
            }
            else{
                y = TECNICOFS_ERROR_FILE_NOT_OPEN;
                return y;
            }
            return y;

        case 'x':
            unlockm();
            numTokens = sscanf(command, "%c %d %s", &token, &iNumber, errou);
            if(iNumber != -1){
                if (vetorEstado(file) != 0){
                    removeVetor(file, iNumber);
                }
            }
            return y;

        case 'o':

            unlockm();
            numTokens= sscanf(command, "%c %s %d %s", &token, filename, &permissao, errou);
            searchResult = lookup(fs, filename);
            if(searchResult != -1){
                if (vetorEstado(file) != 2){
                    if(getIndice(file, searchResult)==-5){
                        if(inode_table[searchResult].owner == owner && ((checkPermissions(inode_table[searchResult].ownerPermissions, permissao)) == 0)){
                          insertVetorOwner(file, searchResult, permissao);
                        }
                        else if (!(checkPermissions(inode_table[searchResult].othersPermissions, permissao))){
                          insertVetorUser(file, searchResult, permissao);
                        }
                        else{
                            y = TECNICOFS_ERROR_PERMISSION_DENIED;
                            return y;
                        }
                    }
                    else{
                        y = TECNICOFS_ERROR_FILE_IS_OPEN;
                        return y;
                    }
                }
                else{
                    y = TECNICOFS_ERROR_MAXED_OPEN_FILES;
                    return y;
                }
            }
            else{
                y = TECNICOFS_ERROR_FILE_NOT_FOUND;
                return y;
            }
            return y;

        default: {
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}
void mataServer(int xd){
    signa = 0;
}
/* funcao auxiliar que le comando a comando e envia para o apply commands */

void * auxRead(void * sock){

  uid_t owner = *(int*)sock;
  files file[5];
  sigset_t sigset;
  char command[MAX_INPUT_SIZE] = "";
  sigemptyset(&sigset);
  sigaddset(&sigset, SIGINT);
  pthread_sigmask(SIG_BLOCK, &sigset, NULL);
  initOpenFiles(file);
  while(read(owner, command, MAX_INPUT_SIZE) > 0){
    int j = applyCommands(command, file, owner);
    if(!(strcmp(geral, ""))){
        sprintf(command, "%d", j);
    }
    else{
        sprintf(command, "%d %s", j, geral);
    }
    write(owner, command, MAX_INPUT_SIZE);
  }
  pthread_exit(NULL);
  return 0;
}


int main(int argc, char* argv[]){

    int sockfd, newsockfd;
    int clilen, servlen, thread_id = 0;
    struct sockaddr_un cli_addr, serv_addr;
    struct sigaction action;
    FILE *fileout;
    action.sa_sigaction = (void*)&mataServer;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGINT, &action, NULL);
    

    parseArgs(argc, argv);
    fileout = fopen(argv[2], "w");

    pthread_t thread[MAXTHREADS];
	inode_table_init();
    fs = new_tecnicofs(numberBuckets);

    if (pthread_rwlock_init(&rwlock, NULL) != 0){
            fprintf(stderr, "Impossível iniciar rwlock\n");
        }
    if ((sockfd = socket(AF_UNIX,SOCK_STREAM,0)) < 0){
        perror("server: can't open stream socket");
    }

    unlink(argv[1]);
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, argv[1]);
    servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0){
        perror("server, can't bind local address");
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    while (signa) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*) &clilen);
        if(pthread_create(&thread[thread_id++], NULL, auxRead, &newsockfd)<0){
            fprintf(stderr, "Could not create thread %d\n", thread_id);
        }
    }
    print_tecnicofs_tree(fileout, fs);
    fclose(fileout);
    if(pthread_rwlock_destroy(&rwlock)){
        fprintf(stderr, "Impossivel destruir rwlock \n");
    }
    inode_table_destroy();
    free_tecnicofs(fs);
    return 0;
    
}
