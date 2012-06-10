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
#include "../nipc/nipc.h"

struct sockaddr_in *socket_address(in_addr_t ip, uint16_t port) {
    struct sockaddr_in *address = malloc(sizeof(struct sockaddr_in));
    memset(address, '\0', sizeof(struct sockaddr_in));
    address->sin_addr.s_addr = ip;
    address->sin_family = AF_INET;
    address->sin_port = htons(port);
    return address;
}

int socket_connect(char *remoteIP, uint16_t port) {
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    const struct sockaddr_in *address = socket_address(inet_addr(remoteIP),
            port);
    connect(socketDescriptor, (struct sockaddr*) address,
            sizeof(struct sockaddr_in));
    return socketDescriptor;
}

int nipc_send(int socketDescriptor, struct nipc_packet *request) {
    void *rawRequest = NULL;
    size_t requestSize = nipc_serialize(request, &rawRequest);

    int sentBytes = 0;
    while (sentBytes < requestSize) {
        int sentChunkSize = send(socketDescriptor, rawRequest, requestSize, 0);
        if (sentChunkSize < 0) {
            perror("Error enviando un chunk");
            return -1;
        }
        sentBytes += sentChunkSize;
    }
    return sentBytes;
}

struct nipc_packet *nipc_receive(int socketDescriptor) {
    struct nipc_packet* packet = NULL;
    size_t headerSize = sizeof(packet->type) + sizeof(packet->data_length);

    char header[headerSize];
    int receivedHeaderLenght = 0;
    while (receivedHeaderLenght < headerSize) {
        int received = recv(socketDescriptor, &header, headerSize, MSG_PEEK);
        if (received < 0) {
            return new_nipc_error("Error recibiendo cabecera");
        } else if(received == 0) {
            return new_nipc_error("Desconectado del servidor");
        }
        receivedHeaderLenght += received;
    }

    uint16_t messageSize = headerSize
            + ((uint16_t) header[sizeof(packet->type)]);
    void *message = malloc(messageSize);
    int receivedBytes = 0;
    while (receivedBytes < messageSize) {
        int received = recv(socketDescriptor, message + receivedBytes,
                messageSize - receivedBytes, 0);
        if (received < 0) {
            free(message);
            return new_nipc_error("Error recibiendo paquete");
        } else if(received == 0) {
            free(message);
            return new_nipc_error("Desconectado del servidor");
        }
        receivedBytes += received;
    }

    packet = nipc_deserialize(message, messageSize);

    return packet;
}

struct nipc_packet *nipc_query(struct nipc_packet *request, char *remoteIP, uint16_t port) {
    int socketDescriptor = socket_connect(remoteIP, port);
    if(nipc_send(socketDescriptor, request) < 0) {
        return new_nipc_error("Error enviando paquete");
    }
    return nipc_receive(socketDescriptor);
}

int socket_binded(uint16_t port) {
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    const struct sockaddr_in *address = socket_address(INADDR_ANY, port);
    bind(socketDescriptor, (struct sockaddr*) address,
            sizeof(struct sockaddr_in));
    return socketDescriptor;
}
