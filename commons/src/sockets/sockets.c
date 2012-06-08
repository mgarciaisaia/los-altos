#include <string.h>
#include <stdlib.h>
#include "sockets.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

struct sockaddr_in *socket_address(char *remoteIP, uint16_t port) {
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    memset(address, '\0', sizeof(struct sockaddr_in));
    address->sin_addr.s_addr = inet_addr(remoteIP);
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    return address;
}

int socket_connect(char *remoteIP, uint16_t port) {
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    const struct sockaddr_in *address = socket_address(remoteIP, port);
    connect(socketDescriptor, (struct sockaddr*) address,
            sizeof(struct sockaddr_in));
    return socketDescriptor;
}

int socket_send(int socketDescriptor, void* message, size_t messageLenght) {
    int sentBytes = 0;
    while (sentBytes < messageLenght) {
        int sentChunkSize = send(socketDescriptor, message, messageLenght, 0);
        if (sentChunkSize < 0) {
            perror("Error enviando un chunk");
            return -1;
        }
        sentBytes += sentChunkSize;
    }
    return sentBytes;
}

size_t socket_receive(int socketDescriptor, void *receivedMessage) {
    char header[3];
    int receivedHeaderLenght = 0;
    while (receivedHeaderLenght < 3) {
        int received = recv(socketDescriptor, &header, 3, MSG_PEEK);
        if (received < 0) {
            perror("Error recibiendo cabecera");
            return -1;
        }
        receivedHeaderLenght += received;
    }

    uint16_t messageSize = (uint16_t) header[1] + 3;
    receivedMessage = malloc(messageSize);
    int receivedBytes = 0;
    while (receivedBytes < messageSize) {
        int received = recv(socketDescriptor, receivedMessage + receivedBytes,
                messageSize - receivedBytes, 0);
        if (received < 0) {
            perror("Error recibiendo el mensaje");
            free(receivedMessage);
            return -1;
        }
        receivedBytes += received;
    }

    return receivedBytes;
}

size_t socket_query(int socketDescriptor, void *rawRequest, size_t requestSize,
        void *rawResponse) {
    socket_send(socketDescriptor, rawRequest, requestSize);
    return socket_receive(socketDescriptor, rawResponse);
}
