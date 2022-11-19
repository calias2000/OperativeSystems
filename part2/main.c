#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>
#include "fs.h"
#include <pthread.h>
#include <semaphore.h>

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

tecnicofs* fs;

/* Declaracao do mutex, rwlock, semaforo produtor e semaforo consumidor */

pthread_mutex_t mutex;
pthread_rwlock_t rwlock; 
sem_t semaforo_prod;
sem_t semaforo_cons;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberThreads = 0;
int headQueue = 0;
int numberCommands = 0;
int numberBuckets = 0;

/* Funcoes de lock e unlock com respetivas verificacoes de erros */

void lockm(){
    #ifdef MUTEX
        if(pthread_mutex_lock(&mutex) != 0){
            fprintf(stderr, "Impossivel dar mutexlock\n");
        }
    #elif RWLOCK
        if(pthread_rwlock_wrlock(&rwlock) != 0){
            fprintf(stderr, "Impossivel dar rwlock\n");
        }
    #endif 
}

void unlockm(){
    #ifdef MUTEX
        if(pthread_mutex_unlock(&mutex)!=0){
            fprintf(stderr, "Impossivel dar mutexunlock\n");
        }
    #elif RWLOCK
        if(pthread_rwlock_unlock(&rwlock) != 0){
            fprintf(stderr, "Impossivel dar rwunlock\n");
        }
    #endif
}

static void displayUsage (const char* appName){
    fprintf(stderr, "Usage: %s\n", appName);
    exit(EXIT_FAILURE);
}

static void parseArgs (long argc, char* const argv[]){
    if (argc != 5) {
        fprintf(stderr, "Invalid format:\n");
        displayUsage(argv[0]);
    }
}

int insertCommand(char* data) {
    if(strcpy(inputCommands[numberCommands], data)){
        numberCommands = (numberCommands+1) % MAX_COMMANDS;
        return 1;
    }
    return 0;
}

char* removeCommand() {
    char *c;
    c = inputCommands[headQueue];  
    headQueue = (headQueue+1) % MAX_COMMANDS;
    return c;
}


void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void * processInput(void * filein){
    char line[MAX_INPUT_SIZE];
        char end_command[10]= "x fim";

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
            case 'r':
            case 'd':
                if(numTokens != 2)
                    errorParse();

                sem_wait(&semaforo_prod);
                lockm();

                if(!insertCommand(line)){
                    fprintf(stderr, "Nao deu insert command \n");
                    exit(EXIT_FAILURE);
                }

                unlockm();
                sem_post(&semaforo_cons);
                break;
            case '#':
                break;
            default: { /* error */
                errorParse();
            }
        }
    }
    fclose(filein);
    sem_wait(&semaforo_prod);
    lockm();
    insertCommand(end_command);
    unlockm();
    sem_post(&semaforo_cons);
    pthread_exit(NULL);
}

void * applyCommands(void * fileout){
    char end_command[10]= "x fim";
    while(1){

        sem_wait(&semaforo_cons);
        lockm();

        const char* command = removeCommand();
        

        char token;
        char common_name[MAX_INPUT_SIZE-2], new_name[MAX_INPUT_SIZE-2], errou[MAX_INPUT_SIZE-2];
        int numTokens = sscanf(command, "%c %s %s %s", &token, common_name, new_name, errou);
        
        if ((numTokens != 2 && token!='r') || (numTokens != 3 && token=='r')) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        int iNumber;

        switch (token) {
            case 'c':

                iNumber = obtainNewInumber(fs);
                unlockm();
                sem_post(&semaforo_prod);
                create(fs, common_name, iNumber);
                break;

            case 'l':

                unlockm();
                sem_post(&semaforo_prod);
                searchResult = lookup(fs, common_name);
                if(!searchResult)
                    printf("%s not found\n", common_name);
                else
                    printf("%s found with inumber %d\n", common_name, searchResult);
                break;

            case 'd':

                unlockm();
                sem_post(&semaforo_prod);
                delete(fs, common_name);               
                break;

            case 'r':

                unlockm();
                sem_post(&semaforo_prod);
                instruction_r(fs, common_name, new_name);
                break;

            case 'x':

                unlockm();
                insertCommand(end_command);
                sem_post(&semaforo_cons);
                pthread_exit(NULL);
                break;

            default: { /* error */

                unlockm();
                sem_post(&semaforo_prod);
                printf("%s\n", command);
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }    
}



int main(int argc, char* argv[]){
    int i= 0;
    
    parseArgs(argc, argv);

    numberThreads = atoi(argv[3]);
    numberBuckets = atoi(argv[4]);

    pthread_t thread[numberThreads];
    pthread_t produtora;

    /* inicializa o mutex e o rwlock e respetiva verificacao de erros 
        em funcao do executavel que esta a ser corrido */

    #ifdef MUTEX
        if (pthread_mutex_init(&mutex, NULL) != 0){
            fprintf(stderr, "Impossivel inicializar mutex\n");
        }
    #elif RWLOCK
        if (pthread_rwlock_init(&rwlock, NULL) != 0){
            fprintf(stderr, "Impossível iniciar rwlock\n");
        }
    #endif

    /* inicializacao do semaforo produtor e consumidor */

    if (sem_init(&semaforo_prod, 0, MAX_COMMANDS) != 0){
        fprintf(stderr, "Impossivel inicializar semaforo\n");
    }

    if (sem_init(&semaforo_cons, 0, 0) != 0){
        fprintf(stderr, "Impossivel inicializar semaforo\n");
    }

    /* verificacao do numero de buckets */

    if (numberBuckets < 1){
        fprintf(stderr, "Numero de buckets inferior a 1 \n");
        exit(EXIT_FAILURE);
    }

    /* verificacoes do numero de threads para cada um dos executaveis */

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

    struct timeval start_t, end_t;

    FILE *filein, *fileout;
    
    /* Abertura dos ficheiros */

    filein = fopen(argv[1], "r");
    fileout = fopen(argv[2], "w");

    if (filein == NULL){
        printf("File does not exist\n");
        exit(EXIT_FAILURE);
    }
    
    fs = new_tecnicofs(numberBuckets);

    /* criacao de uma unica thread produtora */

    if(pthread_create(&produtora, NULL, processInput, (void *)filein) != 0){
        perror("Erro na criacao de thread produtora");
    }

    /*  inicio da contagem do tempo */

    gettimeofday(&start_t, NULL); 

    /* Criacao das threads consumidoras consoante o valor do input */
    
    for (i = 0 ; i < atoi(argv[3]); i++){

        if(pthread_create(&thread[i], NULL, applyCommands, (void *)fileout) != 0){
            perror("Erro na criacao de thread consumidora");
        }
    }

    /* join de todas as threads criadas */

    if(pthread_join(produtora, NULL)!=0){
        perror("Erro no join de thread produtora");
    }
    for (i = 0 ; i < atoi(argv[3]); i++){

        if(pthread_join(thread[i], NULL)!=0){
            perror("Erro no join de thread consumidora");
        }
    }

    gettimeofday(&end_t, NULL);

    print_tecnicofs_tree(fileout, fs);

    free_tecnicofs(fs);


    printf("TecnicoFS completed in %0.4f seconds. \n", ((end_t.tv_sec + end_t.tv_usec/1000000.0)
		  - (start_t.tv_sec+ start_t.tv_usec/1000000.0)));
          
    fclose(fileout);

    /* destruicao do mutex e do rwlock e respetivas verificacoes de erros */
    
    #ifdef MUTEX    
        if (pthread_mutex_destroy(&mutex)){
            fprintf(stderr, "Impossivel destruir mutex \n");
        }
    #elif RWLOCK
        if(pthread_rwlock_destroy(&rwlock)){
            fprintf(stderr, "Impossivel destruir rwlock \n");
        }
    #endif
    if(sem_destroy(&semaforo_cons) || sem_destroy(&semaforo_prod)){
        fprintf(stderr, "Impossivel destruir semaforo \n");
    }

    exit(EXIT_SUCCESS);
}


