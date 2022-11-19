#ifndef FS_H
#define FS_H
#include "lib/bst.h"

typedef struct tecnicofs {
    node **bstRoot;
    int nextINumber;
    int numberBuckets;
} tecnicofs;

int obtainNewInumber(tecnicofs* fs);
tecnicofs* new_tecnicofs();
void free_tecnicofs(tecnicofs* fs);
void create(tecnicofs* fs, char *name, int inumber);
void delete(tecnicofs* fs, char *name);
int lookup(tecnicofs* fs, char *name);
void instruction_r(tecnicofs* fs, char *common, char *new);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);
void lock();
void unlock();
void rdlock();
void initMutex();
void initRwlock();

#endif /* FS_H */
