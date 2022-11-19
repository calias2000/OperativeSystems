#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "lib/hash.h"

pthread_mutex_t* mutex1;
pthread_rwlock_t* rwlock1;

/* inicializacao do array de mutex e de rwlock */

void initMutex(int numberBuckets){
	for(int i=0; i< numberBuckets; i++){
		if (pthread_mutex_init(&mutex1[i], NULL)!=0){
        	fprintf(stderr, "Impossivel inicializar mutex1 %d\n", i);
    	}
	}
}
void initRwlock(int numberBuckets){
	for(int i=0; i< numberBuckets; i++){
		if (pthread_rwlock_init(&rwlock1[i], NULL)!=0){
        	fprintf(stderr, "Impossivel inicializar rwlock1 %d\n", i);
    	}
	}
}

/* funcoes de lock, trylock, rdlock e unlock com respetivas verificacoes de erros */

void lock(int numHash){
    #ifdef MUTEX
    	if(pthread_mutex_lock(&mutex1[numHash])!=0){
        	fprintf(stderr, "Impossivel dar mutexlock\n");
    	}
	#elif RWLOCK
		if(pthread_rwlock_wrlock(&rwlock1[numHash])!=0){
        	fprintf(stderr, "Impossivel dar rwlock\n");
    	}
    #endif 
}

int trylock(int numHash){
    #ifdef MUTEX
    	if(pthread_mutex_trylock(&mutex1[numHash])!=0){
			return 1;
    	}
		return 0;
	#elif RWLOCK
		if(pthread_rwlock_trywrlock(&rwlock1[numHash])!=0){
			return 1;
    	}
		return 0;
    #endif
	return 0; 
}

void rdlock(int numHash){
    #ifdef MUTEX
    	if(pthread_mutex_lock(&mutex1[numHash])!=0){
        	fprintf(stderr, "Impossivel dar mutexlock\n");
    	}
	#elif RWLOCK
		if(pthread_rwlock_rdlock(&rwlock1[numHash])!=0){
        	fprintf(stderr, "Impossivel dar rwlock\n");
    	}
    #endif 
}

void unlock(int numHash){
    #ifdef MUTEX
		if(pthread_mutex_unlock(&mutex1[numHash])!=0){
			fprintf(stderr, "Impossivel dar mutexunlock\n");
		}
	#elif RWLOCK
		if(pthread_rwlock_unlock(&rwlock1[numHash])!=0){
        	fprintf(stderr, "Impossivel dar rwunlock\n");
    	}
    #endif
}

int obtainNewInumber(tecnicofs* fs) {
	int newInumber = ++(fs->nextINumber);
	return newInumber;
}

tecnicofs* new_tecnicofs(int numberBuckets){

	tecnicofs*fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}

	fs->bstRoot = malloc(numberBuckets * sizeof(node*));
	fs->numberBuckets = numberBuckets;

	/* colocar todas as roots a NULL */

	for(int i = 0; i<fs->numberBuckets; i++){
		fs->bstRoot[i] = NULL;
	}

	/* inicializacao do mutex e rwlock */

	#ifdef MUTEX
		mutex1 = malloc(numberBuckets*sizeof(pthread_mutex_t));
		initMutex(fs->numberBuckets);
	#elif RWLOCK
		rwlock1 = malloc(numberBuckets*sizeof(pthread_rwlock_t));
		initRwlock(fs->numberBuckets);
	#endif

	return fs;
}

void free_tecnicofs(tecnicofs* fs){
	int i;
	for (i = 0; i < fs->numberBuckets; i++){
		free_tree(fs->bstRoot[i]);

		/* destruicao do array de mutex e de rwlock e respetivos erros no caso de nao ser possivel efetualo */

		#ifdef MUTEX
			if (pthread_mutex_destroy(&mutex1[i])!=0){
				fprintf(stderr, "Impossivel destruir mutex[%d]\n", i);
			}
		#elif RWLOCK
			if (pthread_rwlock_destroy(&rwlock1[i])!=0){
				fprintf(stderr, "Impossivel destruir rwlock\n");
			}
		#endif
	}

	#ifdef MUTEX
		free(mutex1);
	#elif RWLOCK
		free(rwlock1);
	#endif

	free(fs->bstRoot);
	free(fs);
	
}

void create(tecnicofs* fs, char *name, int inumber){
	int numHash;
	numHash = hash(name, fs->numberBuckets);

	lock(numHash);

	fs->bstRoot[numHash] = insert(fs->bstRoot[numHash], name, inumber);

	unlock(numHash);

}

void delete(tecnicofs* fs, char *name){
	int numHash;

	numHash = hash(name, fs->numberBuckets);
	lock(numHash);

	fs->bstRoot[numHash] = remove_item(fs->bstRoot[numHash], name);

	unlock(numHash);

}

int lookup(tecnicofs* fs, char *name){
	int numHash;
	numHash = hash(name, fs->numberBuckets);
	rdlock(numHash);

	node* searchNode = search(fs->bstRoot[numHash], name);

	if ( searchNode ){
		unlock(numHash);

		return searchNode->inumber;
	}

	unlock(numHash);


	return 0;
}

/* funcao auxiliar de rename */

void instruction_r(tecnicofs* fs, char *common, char *new){
	int numHashC, numHashN, new_iNumber;
	numHashC = hash(common, fs->numberBuckets);
	numHashN = hash(new, fs->numberBuckets);

	/* criacao de um ciclo infinito do qual so se sai quando 
		foi possivel dar lock da/s arvore/s calculada/s atraves da funcao de hash */

	while(1){
		if(numHashC == numHashN){
			if(!trylock(numHashC)){
				break;
			}
		}
		else if(numHashC!=numHashN){
			if (!trylock(numHashC)){
				if(!trylock(numHashN)){
					break;
				}
				else{
					unlock(numHashC);
				}
			}
		}
	}

	node* searchNodeC = search(fs->bstRoot[numHashC], common);
	node* searchNodeN = search(fs->bstRoot[numHashN], new);

	/* verificao dos nodes do input e em caso contrario e feito unlock da/s arvore/s correspondente/s */

	if(!(searchNodeC) || searchNodeN){
		if(numHashC==numHashN){
			unlock(numHashN);
		}
		else{
			unlock(numHashC);
			unlock(numHashN);
		}
		return;
	}

	new_iNumber = searchNodeC->inumber;

	/*remocao do node e create de um novo node com o mesmo inumber */
	
	fs->bstRoot[numHashC] = remove_item(fs->bstRoot[numHashC], common);
	fs->bstRoot[numHashN] = insert(fs->bstRoot[numHashN], new, new_iNumber);

	if(numHashC==numHashN){
		unlock(numHashN);
	}
	else{
		unlock(numHashC);
		unlock(numHashN);
	}

}

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	for(int i=0; i< fs->numberBuckets; i++){
		print_tree(fp, fs->bstRoot[i]);
		
	}
}
