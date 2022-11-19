#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "fs.h"
#include <pthread.h>

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100


int numberThreads = 0;
tecnicofs* fs;

/* Declaracao do mutex e do rwlock */

pthread_mutex_t mutex; 
pthread_rwlock_t rwlock;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;

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

int insertCommand(char* data) {
    if(numberCommands != MAX_COMMANDS) {
        strcpy(inputCommands[numberCommands++], data);
        return 1;
    }
    return 0;
}

char* removeCommand() {

    /* Criacao de um char pointer de modo a ser possivel incrementar HeadQueue antes de 
       efetuar mutex/rwlock e so depois fazer return */

    char * a;

    /* Nao colocamos em MACROS porque sao utilizados apenas uma vez na main.c.
       Definem a utilizacao de mutex, rwlock ou nosync consoante o input do utilizador */

    #ifdef MUTEX
        pthread_mutex_lock(&mutex);
    #elif RWLOCK
        pthread_rwlock_wrlock(&rwlock);
    #endif 

    if((numberCommands > 0)){

        numberCommands--;
        a = inputCommands[headQueue++];
        #ifdef MUTEX
            pthread_mutex_unlock(&mutex);
        #elif RWLOCK
            pthread_rwlock_unlock(&rwlock);
        #endif

        return a;
    }

    #ifdef MUTEX
        pthread_mutex_unlock(&mutex);
    #elif RWLOCK
        pthread_rwlock_unlock(&rwlock);
    #endif

    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    //exit(EXIT_FAILURE);
}

void processInput(FILE *filein){
    char line[MAX_INPUT_SIZE];
    while (fgets(line, MAX_INPUT_SIZE, filein)) {
        char token;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s", &token, name);
        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
            case 'l':
            case 'd':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }
}

void* applyCommands(void * file){
        while(numberCommands > 0){
        const char* command = removeCommand();
        if (command == NULL){
            continue;
        }
        char token;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s", &token, name);
        if (numTokens != 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }
        int searchResult;
        int iNumber;
        switch (token) {
            case 'c':
                iNumber = obtainNewInumber(fs);
                create(fs, name, iNumber);
                break;
            case 'l':
                searchResult = lookup(fs, name);
                if(!searchResult)
                    printf("%s not found\n", name);
                else
                    printf("%s found with inumber %d\n", name, searchResult);
                break;
            case 'd':
                delete(fs, name);               
                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    return 0;
}

int main(int argc, char* argv[]){
    int i, f = 0;
    
    pthread_t thread[MAX_COMMANDS];
    
    parseArgs(argc, argv);

    /* Verificacoes de erros em MACROS */

    #ifdef MUTEX
        if (atoi(argv[3]) < 1){
            fprintf(stderr, "Utilizando o tecnicofs-mutex e obrigatorio o uso de pelo menos uma thread. \n");
            exit(EXIT_FAILURE);
        }
    #elif RWLOCK
        if (atoi(argv[3]) < 1){
            fprintf(stderr, "Utilizando o tecnicofs-rwlock e obrigatorio o uso de pelo menos uma thread. \n");
            exit(EXIT_FAILURE);
        }
    #else
        if (atoi(argv[3])!=1){
            fprintf(stderr, "Utilizando o tecnicofs-nosync e obrigatorio o uso de uma thread. \n");
            exit(EXIT_FAILURE);
        }
    #endif

    /* Verificacao de erro na inicializacao do rwlock */

    if (pthread_rwlock_init(&rwlock, NULL) != 0){
        fprintf(stderr, "Falhou a inicializar o lock\n");
    }

    /* Inicializacao da contagem do tempo */

    struct timeval start_t, end_t;

    FILE *filein, *fileout;

    gettimeofday(&start_t, NULL);
    
    /* Abertura dos ficheiros */

    filein = fopen(argv[1], "r");
    fileout = fopen(argv[2], "w");

    fs = new_tecnicofs();
    processInput(filein);

    /* Criacao das threads consoante o valor do input */
    
    for (i = 0 ; i < atoi(argv[3]); i++){
        
        if(pthread_create(&thread[i], NULL, applyCommands, (void *)fileout) != 0){
            perror("Erro na criacao de thread");
        }
    }

    while (f < atoi(argv[3])){
        pthread_join(thread[f], NULL);
        f++;
    }

    applyCommands((void *)fileout);

    print_tecnicofs_tree(fileout, fs);

    free_tecnicofs(fs);
    gettimeofday(&end_t, NULL);
    printf("TecnicoFS completed in %0.4f seconds. \n", ((end_t.tv_sec + end_t.tv_usec/1000000.0)
		  - (start_t.tv_sec+ start_t.tv_usec/1000000.0)));
    fclose(filein);
    fclose(fileout);
    
    exit(EXIT_SUCCESS);
}


