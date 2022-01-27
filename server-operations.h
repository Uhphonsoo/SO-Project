#ifndef FS_H
#define FS_H
#include "state.h"

#define MAX_PATH_LENGTH 20

int wr_lookup(char *name, int locked_nodes[], int* number_of_locked_nodes);

void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);

void move(char* name1, char* name2);
int mv_create(char *name, type nodeType, int desired_inumber);
int mv_delete(char *name);
int mv_lookup(char *name);
int mv_wr_lookup(char *name, int locked_nodes[], int* number_of_locked_nodes);
int test_lookup(char *name, int test_inumber, int *foundInode);
void copyData(int inumberOrigin, union Data data); 
int print(char* fileName);

int delete(char *name);
int lookup(char *name);
void print_tecnicofs_tree(FILE *fp);

#endif /* FS_H */
