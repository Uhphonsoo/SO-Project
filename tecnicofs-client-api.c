#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

#define CLIENT_SOCKET_BASE_PATH "/tmp/client_socket_so_g55"
#define SOCK_MAX_PATH_LEN 200

char server_socket_path[SOCK_MAX_PATH_LEN];
char clientSocketPath[SOCK_MAX_PATH_LEN] = "";
int sockfd;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {
    if (addr == NULL)
        return 0;

    bzero((char *)addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_UNIX;
    strcpy(addr->sun_path, path);

    return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
    socklen_t servlen;
    struct sockaddr_un serv_addr;
    char nodeTypeString[3];
    int res = 0;
    char command[MAX_INPUT_SIZE] = "c ";

    nodeTypeString[0] = ' ';
    nodeTypeString[1] = nodeType;
    nodeTypeString[2] = '\0';

    strcat(command, filename);
    strcat(command, nodeTypeString);

    /* initialize server socket address */
    servlen = setSockAddrUn(server_socket_path, &serv_addr);

    /* send message to server */
    if (sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    /* receive message from server */
    if (recvfrom(sockfd, &res, sizeof(int), 0, (struct sockaddr *)&serv_addr, &servlen) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    printf("client: Received %d from server.\n", res);

    return res;
}

int tfsDelete(char *path) {
    socklen_t servlen;
    struct sockaddr_un serv_addr;
    int res = 0;
    char command[MAX_INPUT_SIZE] = "d ";

    strcat(command, path);

    /* initialize server socket address */
    servlen = setSockAddrUn(server_socket_path, &serv_addr);

    /* send message to server */
    if (sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    /* receive message from server */
    if (recvfrom(sockfd, &res, sizeof(int), 0, (struct sockaddr *)&serv_addr, &servlen) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    printf("Received %d from server.\n", res);

    return res;
}

int tfsMove(char *from, char *to) {
    socklen_t servlen;
    struct sockaddr_un serv_addr;
    int res = 0;
    char command[MAX_INPUT_SIZE] = "m ";

    strcat(command, from);
    strcat(command, " ");
    strcat(command, to);

    /* intialize server socket address */
    servlen = setSockAddrUn(server_socket_path, &serv_addr);

    /* send message to server */
    if (sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    /* receive message from server */
    if (recvfrom(sockfd, &res, sizeof(int), 0, (struct sockaddr *)&serv_addr, &servlen) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    printf("Received %d from server.\n", res);

    return res;
}

int tfsLookup(char *path) {
    socklen_t servlen;
    struct sockaddr_un serv_addr;
    int res = 0;
    char command[MAX_INPUT_SIZE] = "l ";

    strcat(command, path);

    /* initialize server socket address */
    servlen = setSockAddrUn(server_socket_path, &serv_addr);

    /* send message to server */
    if (sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    /* receive message from server */
    if (recvfrom(sockfd, &res, sizeof(int), 0, (struct sockaddr *)&serv_addr, &servlen) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    printf("Received %d from server.\n", res);

    return res;
}

int tfsPrint(char *outputFile) {   

    socklen_t servlen;
    struct sockaddr_un serv_addr;
    int res = 0;
    char command[MAX_INPUT_SIZE] = "p ";

    strcat(command, outputFile);

    /* initialize server socket address */
    servlen = setSockAddrUn(server_socket_path, &serv_addr);

    /* send message to server */
    if (sendto(sockfd, command, strlen(command) + 1, 0, (struct sockaddr *)&serv_addr, servlen) < 0) {
        perror("client: sendto error");
        exit(EXIT_FAILURE);
    }

    /* receive message from server */
    if (recvfrom(sockfd, &res, sizeof(int), 0, (struct sockaddr *)&serv_addr, &servlen) < 0) {
        perror("client: recvfrom error");
        exit(EXIT_FAILURE);
    }
    printf("Received %d from server.\n", res);

    return res;
}

int tfsMount(char *sockPath) {
    socklen_t clilen;
    struct sockaddr_un serv_addr, client_addr;

    strcpy(server_socket_path, sockPath);

    /* create/declare socket */
    if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("client: can't open socket");
        exit(EXIT_FAILURE);
    }

    /* create unique path */
    pid_t pid = getpid();
    sprintf(clientSocketPath, "%s%ld", CLIENT_SOCKET_BASE_PATH, (long)pid);

    if (unlink(clientSocketPath) != 0) {
        perror("client: unlink error");
        exit(EXIT_FAILURE);
    }

    /* initialize client socket address */
    clilen = setSockAddrUn(clientSocketPath, &client_addr);

    /* bind socket to address */
    if (bind(sockfd, (struct sockaddr *)&client_addr, clilen) < 0) {
        perror("client: bind error");
        exit(EXIT_FAILURE);
    }

    /* initialize server socket address */
    setSockAddrUn(server_socket_path, &serv_addr);

    return 0;
}

int tfsUnmount() {

    /* destroy client socket */
    if (close(sockfd) != 0) {
        fprintf(stderr, "Error: failed to close descriptor for socket.\n");
        exit(EXIT_FAILURE);
    }
    if (unlink(clientSocketPath) != 0) {
        fprintf(stderr, "Error: failed to deleted socket.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
