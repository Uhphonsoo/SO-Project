#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "fs/operations.h"

#define MAX_COMMANDS 10
#define MAX_INPUT_SIZE 100

#define SOCK_MAX_PATH_LEN 100
#define INDIM 30

/* ex2_r1 */
/* ex2_r2 */
/* ex2_r3 */
/* ex3_final */

int numberThreads = 0;
int sockfd;

int reachedEOF = 0;
int modifying_fs = 0, printing_fs = 0;
pthread_cond_t canPrintFS = PTHREAD_COND_INITIALIZER;
pthread_cond_t canModifyFS = PTHREAD_COND_INITIALIZER;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void errorParse(){
    fprintf(stderr, "Error: command invalid\n");
    exit(EXIT_FAILURE);
}


int applyCommand(char *command) {

    int ret = -1;
    char commandCopy[MAX_INPUT_SIZE];
    strcpy(commandCopy, command);

    if (command == NULL){
        return -1;
    }

    char token, type;
    char name[MAX_INPUT_SIZE];

    char name2[MAX_INPUT_SIZE];
    int searchResult, searchResult1, searchResult2;

    int numTokens = sscanf(command, "%c %s %c", &token, name, &type);

    if (numTokens < 2) {
        fprintf(stderr, "Error: invalid command in Queue\n");
        exit(EXIT_FAILURE);
    }

    /* variables needed for case 'm' */
    int k = 0, foundInodeFromPath1 = 0;
    char *aux, *string_array[3], *saveptr, delim[] = " \n";

    switch (token) {
        case 'c':
            switch (type) {
                case 'f':
                    printf("Create file: %s\n", name);
                    
                    if (pthread_mutex_lock(&m) != 0) {
                        fprintf(stderr, "Error: pthread_mutex_lock: Failed to lock mutex.\n");
                        exit(EXIT_FAILURE);
                    }

                    while (printing_fs) {
                        if (pthread_cond_wait(&canModifyFS, &m) != 0) {
                            fprintf(stderr, "Error: pthread_cond_wait: Failed to block thread.\n");
                            exit(EXIT_FAILURE);
                        } 
                    }

                    modifying_fs = 1;
                    ret = create(name, T_FILE);
                    modifying_fs = 0;
                    
                    if (pthread_cond_broadcast(&canPrintFS) != 0) {
                        fprintf(stderr, "Error: pthread_cond_signal: Failed to signal change to mutex.\n");
                        exit(EXIT_FAILURE);
                    }
                    if (pthread_mutex_unlock(&m) != 0) {
                        fprintf(stderr, "Error: pthread_mutex_unlock: Failed to unlock mutex.\n");
                        exit(EXIT_FAILURE);
                    }

                    return ret;

                    break;
                case 'd':
                    printf("Create directory: %s\n", name);

                    if (pthread_mutex_lock(&m) != 0) {
                        fprintf(stderr, "Error: pthread_mutex_lock: Failed to lock mutex.\n");
                        exit(EXIT_FAILURE);
                    }

                    while (printing_fs) {
                        if (pthread_cond_wait(&canModifyFS, &m) != 0) {
                            fprintf(stderr, "Error: pthread_cond_wait: Failed to block thread.\n");
                            exit(EXIT_FAILURE);
                        } 
                    }

                    modifying_fs = 1;
                    ret = create(name, T_DIRECTORY);
                    modifying_fs = 0;

                    if (pthread_cond_broadcast(&canPrintFS) != 0) {
                        fprintf(stderr, "Error: pthread_cond_signal: Failed to signal change to mutex.\n");
                        exit(EXIT_FAILURE);
                    }
                    if (pthread_mutex_unlock(&m) != 0) {
                        fprintf(stderr, "Error: pthread_mutex_unlock: Failed to unlock mutex.\n");
                        exit(EXIT_FAILURE);
                    }

                    return ret;

                    break;
                default:
                    fprintf(stderr, "Error: invalid node type\n");
                    exit(EXIT_FAILURE);
            }
            break;
        case 'l':
            searchResult = lookup(name);

            if (searchResult >= 0)
                printf("Search: %s found\n", name);
            else
                printf("Search: %s not found\n", name);
            return searchResult;

            break;

        case 'd':
            printf("Delete: %s\n", name);

            if (pthread_mutex_lock(&m) != 0) {
                fprintf(stderr, "Error: pthread_mutex_lock: Failed to lock mutex.\n");
                exit(EXIT_FAILURE);
            }
            while (printing_fs) {
                if (pthread_cond_wait(&canModifyFS, &m) != 0) {
                    fprintf(stderr, "Error: pthread_cond_wait: Failed to block thread.\n");
                    exit(EXIT_FAILURE);
                } 
            }
            
            modifying_fs = 1;
            ret = delete(name);
            modifying_fs = 0;

            if (pthread_cond_broadcast(&canPrintFS) != 0) {
                fprintf(stderr, "Error: pthread_cond_signal: Failed to signal change to mutex.\n");
                exit(EXIT_FAILURE);
            }
            if (pthread_mutex_unlock(&m) != 0) {
                fprintf(stderr, "Error: pthread_mutex_unlock: Failed to unlock mutex.\n");
                exit(EXIT_FAILURE);
            }

            return ret;

            break;

        case 'm':
            k = 0;
            aux = strtok_r(commandCopy, delim, &saveptr);

            while (aux != NULL) {
                string_array[k++] = aux;
                aux = strtok_r(NULL, delim, &saveptr);
            }   

            strcpy(name2, string_array[2]);

            /* DEBUG */
            /* printf("~~~~~~~~~~~~~~~~~~~~~~ move ~~~~~~~~~~~~~~~~~~~~~~ %s %s\n", name, name2); */

            searchResult1 = lookup(name);
            searchResult2 = test_lookup(name2, searchResult1, &foundInodeFromPath1);
            
            if (searchResult1 >= 0 && searchResult2 < 0 && !foundInodeFromPath1) {
                /* DEBUG */
                /* printf("Move: conditions were met for %s and %s.\n", name, name2); */

                if (pthread_mutex_lock(&m) != 0) {
                    fprintf(stderr, "Error: pthread_mutex_lock: Failed to lock mutex.\n");
                    exit(EXIT_FAILURE);
                }
                while (printing_fs) {
                    if (pthread_cond_wait(&canModifyFS, &m) != 0) {
                        fprintf(stderr, "Error: pthread_cond_wait: Failed to block thread.\n");
                        exit(EXIT_FAILURE);
                    } 
                }

                modifying_fs = 1;
                move(name, name2);
                modifying_fs = 0;

                if (pthread_cond_broadcast(&canPrintFS) != 0) {
                    fprintf(stderr, "Error: pthread_cond_signal: Failed to signal change to mutex.\n");
                    exit(EXIT_FAILURE);
                }
                if (pthread_mutex_unlock(&m) != 0) {
                    fprintf(stderr, "Error: pthread_mutex_unlock: Failed to unlock mutex.\n");
                    exit(EXIT_FAILURE);
                }

                return SUCCESS;
            }
            return FAIL;

            break;
        case 'p':
            printf("Print to file: %s\n", name);

            if (pthread_mutex_lock(&m) != 0) {
                fprintf(stderr, "Error: pthread_mutex_lock: Failed to lock mutex.\n");
                exit(EXIT_FAILURE);
            }
            while (modifying_fs) {
                if (pthread_cond_wait(&canPrintFS, &m) != 0) {
                    fprintf(stderr, "Error: pthread_cond_wait: Failed to block thread.\n");
                    exit(EXIT_FAILURE);
                } 
            }

            printing_fs = 1;
            ret = print(name);
            printing_fs = 0;

            if (pthread_cond_broadcast(&canModifyFS) != 0) {
                fprintf(stderr, "Error: pthread_cond_signal: Failed to signal change to mutex.\n");
                exit(EXIT_FAILURE);
            }
            if (pthread_mutex_unlock(&m) != 0) {
                fprintf(stderr, "Error: pthread_mutex_unlock: Failed to unlock mutex.\n");
                exit(EXIT_FAILURE);
            }

            return ret;

            break;

        default: { /* error */
            fprintf(stderr, "Error: command to apply\n");
            exit(EXIT_FAILURE);
        }
    }
}

void* fnThread(void* arg) {
    while (1) {
        struct sockaddr_un client_addr;
        char in_command[MAX_INPUT_SIZE];
        int c, operation_staus;
        socklen_t addrlen;

        addrlen = sizeof(struct sockaddr_un);

        /* receive command from client */    
        c = recvfrom(sockfd, in_command, sizeof(in_command) + 1, 0, (struct sockaddr *)&client_addr, &addrlen);

        if (c <= 0) {
            continue;
        }
        in_command[c] = '\0';

        /* DEBUG */
        printf("--%ld--%s--\n", (long)pthread_self(), client_addr.sun_path);

        operation_staus = applyCommand(in_command);

        /* DEBUG */
        /* printf("DEBUG-2 %s\n", client_addr.sun_path); */

        /* send message to client with operation's return value */
        if (sendto(sockfd, &operation_staus, sizeof(int), 0, (struct sockaddr *)&client_addr, addrlen) < 0) {
            perror("server: sendto error");
            exit(EXIT_FAILURE);
        }
    }

    return NULL;
}


int setSockAddrUn(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}


int main(int argc, char* argv[]) {
    int i = 0;

    /* server socket variables */
    struct sockaddr_un server_addr;
    socklen_t serverlen;
    char path[SOCK_MAX_PATH_LEN];

    /* test input validity */
    if (argc != 3) {
        fprintf(stderr, "Error: Invalid input.\nUsage: ./tecnicofs <numthreads> <server_socket_path>\n");
        exit(EXIT_FAILURE);
    }

    /* declare socket */
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: can't open socket");
        exit(EXIT_FAILURE);
    }
    strcpy(path, argv[2]);
    unlink(path);

    /* initialize socket address */
    serverlen = setSockAddrUn(path, &server_addr);

    /* bind socket to socket address */
    if (bind(sockfd, (struct sockaddr *)&server_addr, serverlen) < 0) {
        perror("server: bind error");
        exit(EXIT_FAILURE);
    }

    /* initiate filesystem */
    init_fs();

    /* get number of threads */
    if (atoi(argv[1]) <= 0) {
        fprintf(stderr, "Error: Invalid number of threads.\n");
        exit(EXIT_FAILURE);
    }
    else {
        numberThreads = atoi(argv[1]);
    }

    /* create pool of threads */
    pthread_t tid[numberThreads];
    for (i = 0; i < numberThreads; i++) {

        if (pthread_create(&tid[i], NULL, fnThread, NULL) != 0) {
            fprintf(stderr, "Error: failed to create thread.\n");
            exit(EXIT_FAILURE);
        }
        /* DEBUG */
        /* printf("(T thread %d criada)\n", i); */
    }

    /* start measuring time */
    struct timespec begin, end; 
    clock_gettime(CLOCK_REALTIME, &begin);

    /* wait for threads */
    for (i = 0; i < numberThreads; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            fprintf(stderr, "Error: failed to wait for threads.\n");
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

    /* release allocated memory */
    destroy_fs();

    exit(EXIT_SUCCESS);
}
