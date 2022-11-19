#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include "lib/inodes.h"

typedef struct tecnicofs {
    node **bstRoot;
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
void lock(int numHash);
void unlock(int numHash);
void rdlock(int numHash);
void initMutex();
void initRwlock();

#endif /* FS_H */
