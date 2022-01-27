#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "state.h"
#include "../tecnicofs-api-constants.h"

inode_t inode_table[INODE_TABLE_SIZE];

/* 
 * Lock a node for reading
 * Input:
 *  - i_number: the i-number of the node to be locked  
 */
void rd_lock_node(int i_number) {
    if (pthread_rwlock_rdlock(&inode_table[i_number].lock) != 0) {
        fprintf(stderr, "Error: rd_lock_node: could not rd-lock node %d\n", i_number);
        exit(EXIT_FAILURE);
    }
}

/* 
 * Lock a node for writing
 * Input:
 *  - i_number: the i-number of the node to be locked  
 */
void wr_lock_node(int i_number) {
    if (pthread_rwlock_wrlock(&inode_table[i_number].lock) != 0) {
        fprintf(stderr, "Error: wr_lock_node: could not wr-lock node %d\n", i_number);
        exit(EXIT_FAILURE);
    }
}

/* 
 * Unlock multiple nodes
 * Input:
 *  - locked_nodes: vector of i-numbers of the nodes to be unlocked 
 *  - n: length of the vector 
 */
void unlock_nodes(int locked_nodes[], int n) {
    int j = 0;
    for (j = n-1; j >= 0; j--) {
        if (pthread_rwlock_unlock(&inode_table[locked_nodes[j]].lock) != 0) {
            fprintf(stderr, "Error: unlock_nodes: could not unlock node %d\n", locked_nodes[j]);
            exit(EXIT_FAILURE);
        }
    }
}

/* 
 * Unlock a sigle node
 * Input:
 *  - inumber: the i-number of the node to be unlocked
 */
void unlock_node(int inumber) {
    if (pthread_rwlock_unlock(&inode_table[inumber].lock) != 0) {
        fprintf(stderr, "Error: unlock_node: could not unlock node %d\n", inumber);
        exit(EXIT_FAILURE);
    }
}

/* 
 * Attempt to lock a node for reading
 * Input:
 *  - inumber: the i-number of the node to be locked
 * Returns:
 *  0: if node's lock is locked successfully
 *  EBUSY or EDEADLK: if node's lock can't be acquired by the thread
 */
int rd_trylock_node(int inumber) {
    /* DEBUG */
    /* printf("(pthread_rwlock_tryrdlock: locked node %d)\n", inumber); */
    return pthread_rwlock_tryrdlock(&inode_table[inumber].lock);
}

/* 
 * Attempt to lock a node for writing.
 * Input:
 *  - inumber: the i-number of the node to be locked
 * Returns:
 *  0: if node's lock is locked successfully
 *  EBUSY or EDEADLK: if node's lock can't be acquired by the thread
 */
int wr_trylock_node(int inumber) {
    /* DEBUG */
    /* printf("(pthread_rwlock_trywrlock: locked node %d)\n", inumber); */
    return pthread_rwlock_trywrlock(&inode_table[inumber].lock);
}

/*
 * Sleeps for synchronization testing.
 */
void insert_delay(int cycles) {
    for (int i = 0; i < cycles; i++) {}
}


/*
 * Initializes the i-nodes table.
 */
void inode_table_init() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        inode_table[i].nodeType = T_NONE;
        inode_table[i].data.dirEntries = NULL;
        inode_table[i].data.fileContents = NULL;
        pthread_rwlock_init(&inode_table[i].lock, NULL);
    }
}


/*
 * Releases the allocated memory for the i-nodes tables.
 */
void inode_table_destroy() {
    for (int i = 0; i < INODE_TABLE_SIZE; i++) {
        if (inode_table[i].nodeType != T_NONE) {
            /* as data is an union, the same pointer is used for both dirEntries and fileContents */
            /* just release one of them */
	        if (inode_table[i].data.dirEntries)
                free(inode_table[i].data.dirEntries);
        }
        pthread_rwlock_destroy(&inode_table[i].lock);
    }
}


/*
 * Creates a new i-node in the table with the given information.
 * Input:
 *  - nType: the type of the node (file or directory)
 * Returns:
 *  inumber: identifier of the new i-node, if successfully created
 *     FAIL: if an error occurs
 */
int inode_create(type nType) {
    /* int MAX_PATH_LENGTH = 20;
    int locked_nodes[MAX_PATH_LENGTH];
    int number_of_locked_nodes = 0, i = 0; */

    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    for (int inumber = 0; inumber < INODE_TABLE_SIZE; inumber++) {
        int test = pthread_rwlock_trywrlock(&inode_table[inumber].lock);
        /* DEBUG */
        /* printf("(pthread_rwlock_trywrlock: wr-locked node %d)\n", inumber); */

        if (test == 0) {
            /* locked_nodes[i++] = inumber;
            number_of_locked_nodes++; */

            if (inode_table[inumber].nodeType == T_NONE) {
                inode_table[inumber].nodeType = nType;

                if (nType == T_DIRECTORY) {
                    /* Initializes entry table */
                    inode_table[inumber].data.dirEntries = malloc(sizeof(DirEntry) * MAX_DIR_ENTRIES);
                    
                    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
                        inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
                    }

                }
                else {
                    inode_table[inumber].data.fileContents = NULL;
                }
                /* unlock_node(inumber); */
                /* unlock_nodes(locked_nodes, number_of_locked_nodes); */
                return inumber;
            }
            unlock_node(inumber);
        }
        else if (test == EBUSY || test == EDEADLK) {
            continue;
        }
        else {
            fprintf(stderr, "Error: pthread_rwlock_trywrlock: Failed to try-lock lock.\n");
            exit(EXIT_FAILURE);
        }
    }
    return FAIL;
}

/*
 * Creates a new i-node in the table on the specified position.
 * Input:
 *  - nType: the type of the node (file or directory)
 *  - desired_inumber: the position of the new inode in the inode table
 * Returns:
 *  inumber: identifier of the new i-node, if successfully created
 *     FAIL: if an error occurs
 */
int mv_inode_create(type nType, int desired_inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

        inode_table[desired_inumber].nodeType = nType;

        if (nType == T_DIRECTORY) {
            /* Initializes entry table */
            inode_table[desired_inumber].data.dirEntries = malloc(sizeof(DirEntry) * MAX_DIR_ENTRIES);
            
            for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
                inode_table[desired_inumber].data.dirEntries[i].inumber = FREE_INODE;
            }
        }
        else {
            inode_table[desired_inumber].data.fileContents = NULL;
        }
        return desired_inumber;

    return FAIL;
}


/*
 * Deletes the i-node.
 * Input:
 *  - inumber: identifier of the i-node
 * Returns: SUCCESS or FAIL
 */
int inode_delete(int inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_delete: invalid inumber\n");
        return FAIL;
    } 

    inode_table[inumber].nodeType = T_NONE;
    /* see inode_table_destroy function */
    if (inode_table[inumber].data.dirEntries)
        free(inode_table[inumber].data.dirEntries);

    return SUCCESS;
}


/*
 * Copies the contents of the i-node into the arguments.
 * Only the fields referenced by non-null arguments are copied.
 * Input:
 *  - inumber: identifier of the i-node
 *  - nType: pointer to type
 *  - data: pointer to data
 * Returns: SUCCESS or FAIL
 */
int inode_get(int inumber, type *nType, union Data *data) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_get: invalid inumber %d\n", inumber);
        return FAIL;
    }


    if (nType)
        *nType = inode_table[inumber].nodeType;

    if (data)
        *data = inode_table[inumber].data;

    return SUCCESS;
}


/*
 * Resets an entry for a directory.
 * Input:
 *  - inumber: identifier of the i-node
 *  - sub_inumber: identifier of the sub i-node entry
 * Returns: SUCCESS or FAIL
 */
int dir_reset_entry(int inumber, int sub_inumber) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid inumber\n");
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_reset_entry: can only reset entry to directories\n");
        return FAIL;
    }

    if ((sub_inumber < FREE_INODE) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_reset_entry: invalid entry inumber\n");
        return FAIL;
    }

    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == sub_inumber) {
            inode_table[inumber].data.dirEntries[i].inumber = FREE_INODE;
            inode_table[inumber].data.dirEntries[i].name[0] = '\0';
            return SUCCESS;
        }
    }
    return FAIL;
}


/*
 * Adds an entry to the i-node directory data.
 * Input:
 *  - inumber: identifier of the i-node
 *  - sub_inumber: identifier of the sub i-node entry
 *  - sub_name: name of the sub i-node entry 
 * Returns: SUCCESS or FAIL
 */
int dir_add_entry(int inumber, int sub_inumber, char *sub_name) {
    /* Used for testing synchronization speedup */
    insert_delay(DELAY);

    if ((inumber < 0) || (inumber > INODE_TABLE_SIZE) || (inode_table[inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid inumber\n");
        return FAIL;
    }

    if (inode_table[inumber].nodeType != T_DIRECTORY) {
        printf("inode_add_entry: can only add entry to directories\n");
        return FAIL;
    }

    if ((sub_inumber < 0) || (sub_inumber > INODE_TABLE_SIZE) || (inode_table[sub_inumber].nodeType == T_NONE)) {
        printf("inode_add_entry: invalid entry inumber\n");
        return FAIL;
    }

    if (strlen(sub_name) == 0 ) {
        printf("inode_add_entry: \
               entry name must be non-empty\n");
        return FAIL;
    }

    for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (inode_table[inumber].data.dirEntries[i].inumber == FREE_INODE) {
            inode_table[inumber].data.dirEntries[i].inumber = sub_inumber;
            strcpy(inode_table[inumber].data.dirEntries[i].name, sub_name);
            return SUCCESS;
        }
    }
    return FAIL;
}


/*
 * Prints the i-nodes table.
 * Input:
 *  - inumber: identifier of the i-node
 *  - name: pointer to the name of current file/dir
 */
void inode_print_tree(FILE *fp, int inumber, char *name) {

    if (inode_table[inumber].nodeType == T_FILE) {
        fprintf(fp, "%s\n", name);
        return;
    }

    if (inode_table[inumber].nodeType == T_DIRECTORY) {
        fprintf(fp, "%s\n", name);
        for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
            if (inode_table[inumber].data.dirEntries[i].inumber != FREE_INODE) {
                char path[MAX_FILE_NAME];
                if (snprintf(path, sizeof(path), "%s/%s", name, inode_table[inumber].data.dirEntries[i].name) > sizeof(path)) {
                    fprintf(stderr, "truncation when building full path\n");
                }
                inode_print_tree(fp, inode_table[inumber].data.dirEntries[i].inumber, path);
            }
        }
    }
}

/*
 * Sets the data of given inode to data given as input.
 * Input:
 *  - inumber: identifier of the i-node
 *  - data: data contents
 */
void setData(int inumber, union Data data) {
	inode_table[inumber].data = data;
}
