#include "operations.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


/* Given a path, fills pointers with strings for the parent path and child
 * file name
 * Input:
 *  - path: the path to split. ATENTION: the function may alter this parameter
 *  - parent: reference to a char*, to store parent path
 *  - child: reference to a char*, to store child file name
 */
void split_parent_child_from_path(char * path, char ** parent, char ** child) {

	int n_slashes = 0, last_slash_location = 0;
	int len = strlen(path);

	// deal with trailing slash ( a/x vs a/x/ )
	if (path[len-1] == '/') {
		path[len-1] = '\0';
	}

	for (int i=0; i < len; ++i) {
		if (path[i] == '/' && path[i+1] != '\0') {
			last_slash_location = i;
			n_slashes++;
		}
	}

	if (n_slashes == 0) { // root directory
		*parent = "";
		*child = path;
		return;
	}

	path[last_slash_location] = '\0';
	*parent = path;
	*child = path + last_slash_location + 1;

}


/*
 * Initializes tecnicofs and creates root node.
 */
void init_fs() {
	inode_table_init();
	
	/* create root inode */
	int root = inode_create(T_DIRECTORY);
    unlock_node(root);
	
	if (root != FS_ROOT) {
		printf("failed to create node for tecnicofs root\n");
		exit(EXIT_FAILURE);
	}
}


/*
 * Destroy tecnicofs and inode table.
 */
void destroy_fs() {
	inode_table_destroy();
}


/*
 * Checks if content of directory is not empty.
 * Input:
 *  - entries: entries of directory
 * Returns: SUCCESS or FAIL
 */

int is_dir_empty(DirEntry *dirEntries) {

	if (dirEntries == NULL) {
		return FAIL;
	}

	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
		if (dirEntries[i].inumber != FREE_INODE) {
			return FAIL;
		}
	}

	return SUCCESS;
}


/*
 * Looks for node in directory entry from name.
 * Input:
 *  - name: path of node
 *  - entries: entries of directory
 * Returns:
 *  - inumber: found node's inumber
 *  - FAIL: if not found
 */
int lookup_sub_node(char *name, DirEntry *entries) {
	if (entries == NULL) {
		return FAIL;
	}
	for (int i = 0; i < MAX_DIR_ENTRIES; i++) {
        if (entries[i].inumber != FREE_INODE && strcmp(entries[i].name, name) == 0) {
            return entries[i].inumber;
        }
    }
	return FAIL;
}


/*
 * Creates a new node given a path.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int create(char *name, type nodeType){

	int locked_nodes[MAX_PATH_LENGTH];
	int number_of_locked_nodes = 0;

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);

	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	/* DEBUG */
	/* printf(" ------------------ create ------------------ name: %s\n", name); */

	parent_inumber = wr_lookup(parent_name, locked_nodes, &number_of_locked_nodes);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n", name, parent_name);
		
		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n", name, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}
	
	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n", child_name, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	child_inumber = inode_create(nodeType);

	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n", child_name, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	/* on creation successful lock child node */
	/* wr_lock_node(child_inumber); */
	locked_nodes[number_of_locked_nodes] = child_inumber;
	number_of_locked_nodes += 1;

	/* DEBUG */
	/* printf("( <> created child %d)\n", child_inumber); */

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n", child_name, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	unlock_nodes(locked_nodes, number_of_locked_nodes);
	return SUCCESS;
}


/*
 * Deletes a node given a path.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int delete(char *name){

	int locked_nodes[MAX_PATH_LENGTH];
	int number_of_locked_nodes = 0;

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);

	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	/* DEBUG */
	/* printf(" ------------------ delete ------------------ name: %s\n", name); */

	parent_inumber = wr_lookup(parent_name, locked_nodes, &number_of_locked_nodes);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n", child_name, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n", child_name, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n", name, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	/* on lookup successful lock child node */
	wr_lock_node(child_inumber);
	locked_nodes[number_of_locked_nodes] = child_inumber;
	number_of_locked_nodes += 1;

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n", name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n", child_name, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n", child_inumber, parent_name);

		unlock_nodes(locked_nodes, number_of_locked_nodes);
		return FAIL;
	}

	/* DEBUG */
	/* printf("( <> deleted child %d)\n", child_inumber); */

	unlock_nodes(locked_nodes, number_of_locked_nodes);
	return SUCCESS;
}


/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int lookup(char *name) {

	int locked_nodes[MAX_PATH_LENGTH];
	int number_of_locked_nodes = 0;
	char* saveptr;

	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	strcpy(full_path, name);

	/* DEBUG */
	/* printf(" ------------------ lookup ------------------ name: %s\n", name); */

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* lock root node for reading */
	rd_lock_node(current_inumber);
	locked_nodes[number_of_locked_nodes] = current_inumber;
	number_of_locked_nodes += 1;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	char *path = strtok_r(full_path, delim, &saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		
		rd_lock_node(current_inumber);
		locked_nodes[number_of_locked_nodes] = current_inumber;
		number_of_locked_nodes += 1;
		
		inode_get(current_inumber, &nType, &data);
	}

	unlock_nodes(locked_nodes, number_of_locked_nodes);

	/* DEBUG */
	/* if (current_inumber != FAIL) {
		printf("( <> found inode %d)\n", current_inumber);
	}
	else {
		printf("( <> could not find inode %d)\n", current_inumber);
	} */

	return current_inumber;
}


/*
 * Moves existing node in the first path to the location given by the second path
 * Input:
 *  - name1: path of node to move from
 *  - name2: path of node to move to
 * Returns:
 *  inumber: identifier of moved inode
 */
void move(char* name1, char* name2) {
	int locked_nodes1[MAX_PATH_LENGTH], locked_nodes2[MAX_PATH_LENGTH];
    int locked_merged_nodes[MAX_PATH_LENGTH];
	int number_of_locked_nodes1 = 0, number_of_locked_nodes2 = 0, number_of_merged_locked_nodes = 0;

	int i = 0;
    int test;

	int moved_inumber;
	int parent_inumber1, child_inumber1;
	/* use for copy */
	type pType, type_of_moved_node;
	union Data data, pdata;

	char *parent_name1, *child_name1, name_copy1[MAX_FILE_NAME];
	char *parent_name2, *child_name2, name_copy2[MAX_FILE_NAME];

	strcpy(name_copy1, name1);
	strcpy(name_copy2, name2);

	split_parent_child_from_path(name_copy1, &parent_name1, &child_name1);
	split_parent_child_from_path(name_copy2, &parent_name2, &child_name2);

	/* DEBUG */
	/* printf(" ------------------ move ------------------ name: %s %s\n", name1, name2); */

	parent_inumber1 = mv_wr_lookup(parent_name1, locked_nodes1, &number_of_locked_nodes1);
	mv_wr_lookup(parent_name2, locked_nodes2, &number_of_locked_nodes2);

    inode_get(parent_inumber1, &pType, &pdata);

    /* get moved_node's inumber */
	child_inumber1 = lookup_sub_node(child_name1, pdata.dirEntries);

	/* first path's child is the node to be moved */
	moved_inumber = child_inumber1;

    /* lock inode to be moved */
	wr_lock_node(moved_inumber);
	locked_nodes1[number_of_locked_nodes1] = moved_inumber;
	number_of_locked_nodes1 += 1;

    /* get type and data of inode to be moved */
	inode_get(moved_inumber, &type_of_moved_node, &data);

    /* lock nodes from shortest path first */
    /* 1) path1 shorter than path2 */
    if (number_of_locked_nodes1 < number_of_locked_nodes2) {
        for (i = 0; i < number_of_locked_nodes1 - 2; i++) {
            rd_lock_node(locked_nodes1[i]);
        }

        if (locked_nodes1[i] < locked_nodes1[i+1]) {
            wr_lock_node(locked_nodes1[i]);
            wr_lock_node(locked_nodes1[i+1]);
        }
        else {
            wr_lock_node(locked_nodes1[i+1]);
            wr_lock_node(locked_nodes1[i]);
        }
        
        for (i = 0; i < number_of_locked_nodes2 - 1; i++) {
            test = rd_trylock_node(locked_nodes2[i]);
            if (test == EBUSY || test == EDEADLK || test == 0) {
                continue;
            }
            else {
                fprintf(stderr, "Error: move: could not try-lock lock for reading.\n");
            }
        }
        test = wr_trylock_node(locked_nodes2[i]);
        if (!(test == EBUSY || test == EDEADLK || test == 0)) {
            fprintf(stderr, "Error: move: could not try-lock lock for writing.\n");
        }
    }

    /* 2) path2 shorter than path1 */
    else {
        for (i = 0; i < number_of_locked_nodes2 - 1; i++) {
            rd_lock_node(locked_nodes2[i]);
        }

        for (i = 0; i < number_of_locked_nodes1 - 2; i++) {
            test = rd_trylock_node(locked_nodes1[i]);
            if (test == EBUSY || test == EDEADLK || test == 0) {
                continue;
            }
            else {
                fprintf(stderr, "Error: move: could not try-lock lock for reading.\n");
            }
        }
        if (locked_nodes1[i] < locked_nodes1[i+1]) {
            test = wr_trylock_node(locked_nodes1[i]);
            if (!(test == EBUSY || test == EDEADLK || test == 0)) {
                fprintf(stderr, "Error: move: could not try-lock lock for writing.\n");
            }
            test = wr_trylock_node(locked_nodes1[i+1]);
            if (!(test == EBUSY || test == EDEADLK || test == 0)) {
                fprintf(stderr, "Error: move: could not try-lock lock for writing.\n");
            }
        }
        else {
            test = wr_trylock_node(locked_nodes1[i+1]);
            if (!(test == EBUSY || test == EDEADLK || test == 0)) {
                fprintf(stderr, "Error: move: could not try-lock lock for writing.");
            }
            test = wr_trylock_node(locked_nodes1[i]);
            if (!(test == EBUSY || test == EDEADLK || test == 0)) {
                fprintf(stderr, "Error: move: could not try-lock lock for writing.");
            }
        }
    }

    /* delete moved inumber */
	mv_delete(name1);
    /* create moved inumber in new position */
	mv_create(name2, type_of_moved_node, moved_inumber);

	/* setter for data */
	copyData(moved_inumber, data);
	
    /* merge both lists to avoid unlocking root twice */
    for (i = 0; i < number_of_locked_nodes1; i++) {
        locked_merged_nodes[i] = locked_nodes1[i];
    }
    int k = 1;
    for (int j = i; j < number_of_locked_nodes1 + number_of_locked_nodes2; j++) {
        locked_merged_nodes[j] = locked_nodes2[k++];
    }

    number_of_merged_locked_nodes = number_of_locked_nodes1 + number_of_locked_nodes2 - 1;
    unlock_nodes(locked_merged_nodes, number_of_merged_locked_nodes);
}


/*
 * Lookup called by operations create() and delete().
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int wr_lookup(char *name, int locked_nodes[], int* number_of_locked_nodes) {

	char full_path[MAX_FILE_NAME];
	char aux_full_path[MAX_FILE_NAME];
	char* saveptr;

	char delim[] = "/";

	strcpy(full_path, name);
	strcpy(aux_full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	char *path = strtok_r(full_path, delim, &saveptr);
	if (path == NULL) {
		wr_lock_node(current_inumber);
	}
	else {
		rd_lock_node(current_inumber);
	}
	locked_nodes[*number_of_locked_nodes] = current_inumber;
	*number_of_locked_nodes += 1;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		
		if (path == NULL) {
			wr_lock_node(current_inumber);
		}
		else {
			rd_lock_node(current_inumber);
		}
		locked_nodes[*number_of_locked_nodes] = current_inumber;
		*number_of_locked_nodes += 1;
		
		inode_get(current_inumber, &nType, &data);
	}

	return current_inumber;
}


/* 
 * Copies data given by argument data into the inode given by inumber.
 * Input:
 *  - inumber: identifier of destination inode
 *  - data: pointer to data to be copied
 */
void copyData(int inumber, union Data data) {
	type inode_type;

	inode_get(inumber, &inode_type, &data);
	setData(inumber, data);
}


/*
 * Prints tecnicofs tree.
 * Input:
 *  - fp: pointer to output file
 */
void print_tecnicofs_tree(FILE *fp){
	inode_print_tree(fp, FS_ROOT, "");
}

/*
 * Creates a new node given a path without locking any inodes.
 * Input:
 *  - name: path of node
 *  - nodeType: type of node
 * Returns: SUCCESS or FAIL
 */
int mv_create(char *name, type nodeType, int desired_inumber){

	int parent_inumber , child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType;
	union Data pdata;

	strcpy(name_copy, name);

	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	/* DEBUG */
	/* printf(" ------------------ mv_create ------------------ name: %s\n", name); */

	parent_inumber = mv_lookup(parent_name);

	if (parent_inumber == FAIL) {
		printf("failed to create %s, invalid parent dir %s\n", name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to create %s, parent %s is not a dir\n", name, parent_name);
		return FAIL;
	}
	
	if (lookup_sub_node(child_name, pdata.dirEntries) != FAIL) {
		printf("failed to create %s, already exists in dir %s\n", child_name, parent_name);
		return FAIL;
	}

	child_inumber = mv_inode_create(nodeType, desired_inumber);

	if (child_inumber == FAIL) {
		printf("failed to create %s in  %s, couldn't allocate inode\n", child_name, parent_name);
		return FAIL;
	}

	/* DEBUG */
	/* printf("( <> created child %d)\n", child_inumber); */

	if (dir_add_entry(parent_inumber, child_inumber, child_name) == FAIL) {
		printf("could not add entry %s in dir %s\n", child_name, parent_name);
		return FAIL;
	}

	return SUCCESS;
}

/*
 * Deletes a node given a path without locking any inodes.
 * Input:
 *  - name: path of node
 * Returns: SUCCESS or FAIL
 */
int mv_delete(char *name){

	int parent_inumber, child_inumber;
	char *parent_name, *child_name, name_copy[MAX_FILE_NAME];
	/* use for copy */
	type pType, cType;
	union Data pdata, cdata;

	strcpy(name_copy, name);

	split_parent_child_from_path(name_copy, &parent_name, &child_name);

	/* DEBUG */
	/* printf(" ------------------ mv_delete ------------------ name: %s\n", name); */

	parent_inumber = mv_lookup(parent_name);

	if (parent_inumber == FAIL) {
		printf("failed to delete %s, invalid parent dir %s\n", child_name, parent_name);
		return FAIL;
	}

	inode_get(parent_inumber, &pType, &pdata);

	if(pType != T_DIRECTORY) {
		printf("failed to delete %s, parent %s is not a dir\n", child_name, parent_name);
		return FAIL;
	}

	child_inumber = lookup_sub_node(child_name, pdata.dirEntries);

	if (child_inumber == FAIL) {
		printf("could not delete %s, does not exist in dir %s\n", name, parent_name);
		return FAIL;
	}

	inode_get(child_inumber, &cType, &cdata);

	if (cType == T_DIRECTORY && is_dir_empty(cdata.dirEntries) == FAIL) {
		printf("could not delete %s: is a directory and not empty\n", name);
		return FAIL;
	}

	/* remove entry from folder that contained deleted node */
	if (dir_reset_entry(parent_inumber, child_inumber) == FAIL) {
		printf("failed to delete %s from dir %s\n", child_name, parent_name);
		return FAIL;
	}

	if (inode_delete(child_inumber) == FAIL) {
		printf("could not delete inode number %d from dir %s\n", child_inumber, parent_name);
		return FAIL;
	}

	/* DEBUG */
	/* printf("( <> deleted child %d)\n", child_inumber); */

	return SUCCESS;
}

/*
 * Lookup for a given path without locking any nodes for writing nor for reading. 
 * Used by mv_create() and mv_delete().
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int mv_lookup(char *name) {

	char* saveptr;
	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	strcpy(full_path, name);

	/* DEBUG */
	/* printf(" ------------------ mv_lookup ------------------ name: %s\n", name); */

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	char *path = strtok_r(full_path, delim, &saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		
		inode_get(current_inumber, &nType, &data);
	}

	return current_inumber;
}

/*
 * Lookup for a given path without locking any nodes for writing nor for reading.
 * Input:
 *  - name: path of node
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: otherwise
 */
int mv_wr_lookup(char *name, int locked_nodes[], int* number_of_locked_nodes) {

	char full_path[MAX_FILE_NAME];
	char aux_full_path[MAX_FILE_NAME];
	char* saveptr;

	char delim[] = "/";

	strcpy(full_path, name);
	strcpy(aux_full_path, name);

	/* start at root node */
	int current_inumber = FS_ROOT;
	locked_nodes[*number_of_locked_nodes] = current_inumber;
	*number_of_locked_nodes += 1;

	/* use for copy */
	type nType;
	union Data data;

	char *path = strtok_r(full_path, delim, &saveptr);

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		
		locked_nodes[*number_of_locked_nodes] = current_inumber;
		*number_of_locked_nodes += 1;
		
		inode_get(current_inumber, &nType, &data);
	}

	return current_inumber;
}

/*
 * Lookup for a given path.
 * Input:
 *  - name: path of node
 *  - test_inumber: inumber whose existence will be tested
 *  - foundInode: flag to be changed in case inumber is found
 * Returns:
 *  inumber: identifier of the i-node, if found
 *     FAIL: if inumber given was found during search
 */
int test_lookup(char *name, int test_inumber, int *foundInode) {

	int locked_nodes[MAX_PATH_LENGTH];
	int number_of_locked_nodes = 0;
	char* saveptr;

	char full_path[MAX_FILE_NAME];
	char delim[] = "/";

	strcpy(full_path, name);

	/* DEBUG */
	/* printf(" ------------------ test-lookup ------------------ name: %s\n", name); */

	/* start at root node */
	int current_inumber = FS_ROOT;

	/* use for copy */
	type nType;
	union Data data;

	/* lock root node for reading */
	rd_lock_node(current_inumber);
	locked_nodes[number_of_locked_nodes] = current_inumber;
	number_of_locked_nodes += 1;

	/* get root inode data */
	inode_get(current_inumber, &nType, &data);

	char *path = strtok_r(full_path, delim, &saveptr);

	/* search for all sub nodes */
	while (path != NULL && (current_inumber = lookup_sub_node(path, data.dirEntries)) != FAIL) {
		path = strtok_r(NULL, delim, &saveptr);
		
		rd_lock_node(current_inumber);
		locked_nodes[number_of_locked_nodes] = current_inumber;
		number_of_locked_nodes += 1;

        if (current_inumber == test_inumber) {
            *foundInode = 1;
            unlock_nodes(locked_nodes, number_of_locked_nodes);
            return -1;
        }

		inode_get(current_inumber, &nType, &data);
	}

	unlock_nodes(locked_nodes, number_of_locked_nodes);

	/* DEBUG */
	/* if (current_inumber != FAIL) {
		printf("( <> found inode %d)\n", current_inumber);
	}
	else {
		printf("( <> could not find inode %d)\n", current_inumber);
	} */

	return current_inumber;
}

/*
 * Print file system tree to given file.
 * Input:
 *  - fileName: path of file
 * Returns:
 *  SUCCESS: if operation is successful
 *     FAIL: otherwise
 */
int print(char* fileName) {
	FILE* fpOut = fopen(fileName, "w");

	if (fpOut == NULL) {
		fprintf(stderr, "Error: Invalid output file.\n");
		return -1;
	}

	print_tecnicofs_tree(fpOut);

	if (fclose(fpOut) != 0) {
        fprintf(stderr, "Error: Failed to close file.\n");
		return -1;
	}
	return 0;
}