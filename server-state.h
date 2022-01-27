#ifndef INODES_H
#define INODES_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../tecnicofs-api-constants.h"

/* FS root inode number */
#define FS_ROOT 0

#define FREE_INODE -1
#define INODE_TABLE_SIZE 50
#define MAX_DIR_ENTRIES 20

#define SUCCESS 0
#define FAIL -1

#define DELAY 50000


/*
 * Contains the name of the entry and respective i-number
 */
typedef struct dirEntry {
	char name[MAX_FILE_NAME];
	int inumber;
} DirEntry;

/*
 * Data is either text (file) or entries (DirEntry)
 */
union Data {
	char *fileContents; /* for files */
	DirEntry *dirEntries; /* for directories */
};

/*
 * I-node definition
 */
typedef struct inode_t {    
	type nodeType;
	union Data data;
	/* more i-node attributes will be added in future exercises */
	pthread_rwlock_t lock;
} inode_t;

void rd_lock_node(int i_number);
void wr_lock_node(int i_number);
void unlock_nodes(int locked_nodes[], int n);
void unlock_node(int inumber);
int rd_trylock_node(int inumber);
int wr_trylock_node(int inumber);

void insert_delay(int cycles);
void inode_table_init();
void inode_table_destroy();
int inode_create(type nType);

int mv_inode_create(type nType, int desired_inumber);
int inode_delete(int inumber);
int inode_get(int inumber, type *nType, union Data *data);
int inode_set_file(int inumber, char *fileContents, int len);
int dir_reset_entry(int inumber, int sub_inumber);
int dir_add_entry(int inumber, int sub_inumber, char *sub_name);
void inode_print_tree(FILE *fp, int inumber, char *name);

void setData(int inumber, union Data data);

#endif /* INODES_H */
