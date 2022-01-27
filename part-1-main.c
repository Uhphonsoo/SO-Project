#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include "fs/operations.h"

#define MAX_COMMANDS 150000
#define MAX_INPUT_SIZE 100

int numberThreads = 0;
pthread_mutex_t trinco_mutex;
pthread_rwlock_t trinco_rwl;
int syncStrategy = 1;

char inputCommands[MAX_COMMANDS][MAX_INPUT_SIZE];
int numberCommands = 0;
int headQueue = 0;


void initiateLocks() {
    /* nosync */
    if (syncStrategy == 1) {
        /* do nothing */;
    }
    /* mutex */
    else if (syncStrategy == 2) {
        pthread_mutex_init(&trinco_mutex, NULL);
    }
    /* wrlock */
    else if (syncStrategy == 3) {
        pthread_mutex_init(&trinco_mutex, NULL);
        pthread_rwlock_init(&trinco_rwl, NULL);
    }
}


void destroyLocks() {
    /* nosync */
    if (syncStrategy == 1) {
        /* do nothing */;
    }
    /* mutex */
    else if (syncStrategy == 2) {
        pthread_mutex_destroy(&trinco_mutex);
    }
    /* rwlock */
    else if (syncStrategy == 3) {
        pthread_mutex_destroy(&trinco_mutex);
        pthread_rwlock_destroy(&trinco_rwl);
    }
}


void wrLock() {
    if (syncStrategy == 1) {
        /* do nothing */;
    }
    else if (syncStrategy == 2) {
        pthread_mutex_lock(&trinco_mutex);
    }
    else if (syncStrategy == 3) {
        pthread_rwlock_wrlock(&trinco_rwl);
    }
    else {
        fprintf(stderr, "An error occured during locking.\n");
    }
}

void rdLock() {
    if (syncStrategy == 1) {
        /* do nothing */;
    }
    else if (syncStrategy == 2) {
        pthread_mutex_lock(&trinco_mutex);
    }
    else if (syncStrategy == 3) {
        pthread_rwlock_rdlock(&trinco_rwl);
    }
    else {
        fprintf(stderr, "An error occured during locking.\n");
    }
}

void unlock() {
    if (syncStrategy == 1) {
        /* do nothing */;
    }
    else if (syncStrategy == 2) {
        pthread_mutex_unlock(&trinco_mutex);
    }
    else if (syncStrategy == 3) {
        pthread_rwlock_unlock(&trinco_rwl);
    }
    else {
        fprintf(stderr, "An error occured during unlocking.\n");
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
    if(numberCommands > 0){
        numberCommands--;
        return inputCommands[headQueue++];  
    }
    return NULL;
}

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}

void processInput(FILE* fpIn){
    char line[MAX_INPUT_SIZE];

    /* break loop with ^Z or ^D */
    while (fgets(line, sizeof(line)/sizeof(char), fpIn)) {
        char token, type;
        char name[MAX_INPUT_SIZE];

        int numTokens = sscanf(line, "%c %s %c", &token, name, &type);

        /* perform minimal validation */
        if (numTokens < 1) {
            continue;
        }
        switch (token) {
            case 'c':
                if(numTokens != 3)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
            case 'l':
                if(numTokens != 2)
                    errorParse();
                if(insertCommand(line))
                    break;
                return;
            
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

void applyCommands(){
    while (numberCommands > 0){

        /* w ~~~~~~~~~~~~~~~~~~~~~ */
        pthread_mutex_lock(&trinco_mutex);
        const char* command = removeCommand();
        pthread_mutex_unlock(&trinco_mutex);
        /* ~~~~~~~~~~~~~~~~~~~~~ */

        if (command == NULL){
            continue;
        }

        char token, type;
        char name[MAX_INPUT_SIZE];
        int numTokens = sscanf(command, "%c %s %c", &token, name, &type);
        if (numTokens < 2) {
            fprintf(stderr, "Error: invalid command in Queue\n");
            exit(EXIT_FAILURE);
        }

        int searchResult;
        switch (token) {
            case 'c':
                switch (type) {
                    case 'f':
                        printf("Create file: %s\n", name);

                        /* w --------------------- */
                        wrLock();
                        create(name, T_FILE);
                        unlock();
                        /* --------------------- */

                        break;
                    case 'd':
                        printf("Create directory: %s\n", name);

                        /* w --------------------- */
                        wrLock();
                        create(name, T_DIRECTORY);
                        unlock();
                        /* --------------------- */

                        break;
                    default:
                        fprintf(stderr, "Error: invalid node type\n");
                        exit(EXIT_FAILURE);
                }
                break;
            case 'l': 

                /* r --------------------- */
                rdLock();
                searchResult = lookup(name);
                unlock();
                /* --------------------- */

                if (searchResult >= 0)
                    printf("Search: %s found\n", name);
                else
                    printf("Search: %s not found\n", name);
                break;
            case 'd':
                printf("Delete: %s\n", name);

                /* w --------------------- */
                wrLock();
                delete(name);
                unlock();
                /* --------------------- */

                break;
            default: { /* error */
                fprintf(stderr, "Error: command to apply\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void* fnThread(void* arg) {
    applyCommands();
    return NULL;
}

int main(int argc, char* argv[]) {
    int i = 0;

    /* init filesystem */
    init_fs();

    /* open files */
    FILE* fpIn = fopen(argv[1], "r");
    if (fpIn == NULL) {
        fprintf(stderr, "Error: Invalid input file.\n");
        exit(EXIT_FAILURE);
    }
    FILE* fpOut = fopen(argv[2], "w");
    if (fpOut == NULL) {
        fprintf(stderr, "Error: Invalid output file.\n");
        exit(EXIT_FAILURE);
    }

    /* get number of threads */
    if (atoi(argv[3]) <= 0) {
        fprintf(stderr, "Error: Invalid number of threads.\n");
        exit(EXIT_FAILURE);
    }
    else {
        numberThreads = atoi(argv[3]);
    }

    /* assign sync strategy */
    if (strcmp("nosync", argv[4]) == 0 && numberThreads == 1) {
        syncStrategy = 1;
    }
    else if (strcmp("mutex", argv[4]) == 0 && numberThreads > 1) {
        syncStrategy = 2;
    }
    else if (strcmp("rwlock", argv[4]) == 0 && numberThreads > 1) {
        syncStrategy = 3;
    }
    else {
        fprintf(stderr, "Error: Incorrect input.\n");
        exit(EXIT_FAILURE);
    }

    /* process input */
    processInput(fpIn);

    /* initiate locks */
    initiateLocks();

    /* create pool of threads */
    pthread_t tid[numberThreads];
    for (i = 0; i < numberThreads; i++) {

        if (pthread_create(&tid[i], NULL, fnThread, NULL) != 0) {
            fprintf(stderr, "ERROR during creation of thread.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* start measuring time */
    struct timespec begin, end; 
    clock_gettime(CLOCK_REALTIME, &begin);

    /* wait for threads */
    for (i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "ERROR during wait of threads.\n");
            exit(EXIT_FAILURE);
        }
    }

    /* stop measuring time and calculate the elapsed time */
    clock_gettime(CLOCK_REALTIME, &end);
    long seconds = end.tv_sec - begin.tv_sec;
    long nanoseconds = end.tv_nsec - begin.tv_nsec;
    double elapsed = seconds + nanoseconds*1e-9;
    /* print total execution time */
    fprintf(stdout, "TecnicoFS completed in %0.4f seconds.\n", elapsed);

    /* destroy locks */
    destroyLocks();

    /* print tree */
    print_tecnicofs_tree(fpOut);

    if (fclose(fpIn) != 0) {
        fprintf(stderr, "Error: Failed to close file.\n");
    }
    if (fclose(fpOut) != 0) {
        fprintf(stderr, "Error: Failed to close file.\n");
    }

    /* release allocated memory */
    destroy_fs();

    exit(EXIT_SUCCESS);
}
