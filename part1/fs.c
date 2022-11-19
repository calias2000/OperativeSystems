#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>


pthread_mutex_t mutex1;
pthread_rwlock_t rwlock1;

/* Funcoes auxiliares para as MACROS que delimitam o decorrer do codigo consoante o input de
   tecnicofs-nosync, tecnicofs-mutex ou tecnicofs-rwlock. */

void lock(){
	#ifdef MUTEX
        pthread_mutex_lock(&mutex1);
    #elif RWLOCK
        pthread_rwlock_wrlock(&rwlock1);
    #endif 
}

void unlock(){
	#ifdef MUTEX
        pthread_mutex_unlock(&mutex1);
    #elif RWLOCK
        pthread_rwlock_unlock(&rwlock1);
    #endif
}

void rdlock(){
	#ifdef MUTEX
        pthread_mutex_lock(&mutex1);
    #elif RWLOCK
        pthread_rwlock_rdlock(&rwlock1);
    #endif 
}

int obtainNewInumber(tecnicofs* fs) {
	int newInumber = ++(fs->nextINumber);
	return newInumber;
}

tecnicofs* new_tecnicofs(){
	tecnicofs*fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->bstRoot = NULL;
	return fs;
}

void free_tecnicofs(tecnicofs* fs){
	free_tree(fs->bstRoot);
	free(fs);
}

void create(tecnicofs* fs, char *name, int inumber){
	lock();

	fs->bstRoot = insert(fs->bstRoot, name, inumber);

	unlock();
}

void delete(tecnicofs* fs, char *name){
	lock();

	fs->bstRoot = remove_item(fs->bstRoot, name);

	unlock();
}

int lookup(tecnicofs* fs, char *name){
	rdlock();
	node* searchNode = search(fs->bstRoot, name);

	if ( searchNode ){
		unlock();

		return searchNode->inumber;
	}

	unlock();

	return 0;
}

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	print_tree(fp, fs->bstRoot);
}
