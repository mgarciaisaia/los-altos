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
#include <unistd.h>
#include "../nipc/nipc.h"
#include "../commons/log.h"
#include <errno.h>
#include <netinet/tcp.h>

t_log *logger_socket;

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
    int flag = 1;
    setsockopt(socketDescriptor, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));
    struct sockaddr_in *address = socket_address(inet_addr(remoteIP),
            port);
    connect(socketDescriptor, (struct sockaddr*) address,
            sizeof(struct sockaddr_in));
    free(address);
    return socketDescriptor;
}

int nipc_send(int socketDescriptor, struct nipc_packet *request) {
    void *rawRequest = NULL;

    log_debug(logger_socket, "Envio un paquete de tipo %d por %d", request->type, socketDescriptor);
    size_t requestSize = nipc_serialize(request, &rawRequest);

    log_debug(logger_socket, "Envio %d bytes por %d", requestSize, socketDescriptor);

    int sentBytes = 0;
    while (sentBytes < requestSize) {
        int sentChunkSize = send(socketDescriptor, rawRequest, requestSize, 0);
        if (sentChunkSize < 0) {
            log_error(logger_socket, "Error enviando un chunk - %s", strerror(errno));
            perror("Error enviando un chunk");
            free(rawRequest);
            return -1;
        }
        sentBytes += sentChunkSize;
        log_trace(logger_socket, "Envie %d / %d bytes (total: %d)", sentChunkSize, sentBytes, requestSize);
    }

    log_trace(logger_socket, "Envie %d bytes (total: %d)", sentBytes, requestSize);
    free(rawRequest);
    return sentBytes;
}

struct nipc_packet *nipc_receive(int socketDescriptor) {
    struct nipc_packet* packet = NULL;
    size_t headerSize = sizeof(packet->type) + sizeof(packet->client_id) + sizeof(packet->data_length);

    log_debug(logger_socket, "Recibo un header de %d bytes en %d", headerSize, socketDescriptor);

    void *header = malloc(headerSize);
    int receivedHeaderLenght = 0;
    while (receivedHeaderLenght < headerSize) {
        int received = recv(socketDescriptor, header, headerSize, MSG_PEEK);
        if (received < 0) {
            log_error(logger_socket, "Error recibiendo cabecera en %d - %s", socketDescriptor, strerror(errno));
            free(header);
            return new_nipc_error(strdup("Error recibiendo cabecera"));
        } else if(received == 0) {
            log_error(logger_socket, "Error recibiendo cabecera en %d - Desconectado del servidor", socketDescriptor);
            free(header);
            return new_nipc_disconnected();
        }
        receivedHeaderLenght += received;
        log_trace(logger_socket, "Recibi %d / %d bytes del header (total: %d) en %d", received, receivedHeaderLenght, headerSize, socketDescriptor);
    }

    log_debug(logger_socket, "Recibi un header de %d bytes en %d", headerSize, socketDescriptor);


    u_int16_t messageSize = headerSize
            + *(u_int16_t *) (header + sizeof(packet->type) + sizeof(packet->client_id));
    free(header);
    void *message = malloc(messageSize);

    log_debug(logger_socket, "Recibiendo un mensaje de %d bytes en %d", messageSize, socketDescriptor);

    int receivedBytes = 0;
    while (receivedBytes < messageSize) {
        int received = recv(socketDescriptor, message + receivedBytes,
                messageSize - receivedBytes, 0);
        if (received < 0) {
            free(message);
            log_error(logger_socket, "Error recibiendo paquete en %d - %s", socketDescriptor, strerror(errno));
            return new_nipc_error(strdup("Error recibiendo paquete"));
        } else if(received == 0) {
            free(message);
            log_error(logger_socket, "Error recibiendo paquete en %d - Desconectado del servidor", socketDescriptor);
            return new_nipc_disconnected();
        }
        receivedBytes += received;
        log_trace(logger_socket, "Recibi %d / %d bytes (total: %d) en %d", received, receivedBytes, messageSize, socketDescriptor);
    }

    log_debug(logger_socket, "Recibi un mensaje de %d bytes en %d", receivedBytes, socketDescriptor);

    packet = nipc_deserialize(message, messageSize);

    log_debug(logger_socket, "Recibi un paquete de tipo %d en %d", packet->type, socketDescriptor);

    return packet;
}

struct nipc_packet *nipc_query(struct nipc_packet *request, char *remoteIP, uint16_t port) {
    log_debug(logger_socket, "Conectando a %s : %d", remoteIP, port);
    int socketDescriptor = socket_connect(remoteIP, port);
    log_debug(logger_socket, "Conectado a %s : %d en %d", remoteIP, port, socketDescriptor);
    if(nipc_send(socketDescriptor, request) < 0) {
        log_error(logger_socket, "Error enviando paquete");
        return new_nipc_error(strdup("Error enviando paquete"));
    }
    struct nipc_packet *response = nipc_receive(socketDescriptor);
    log_debug(logger_socket, "Cierro el socket %d", socketDescriptor);
    close(socketDescriptor);
    return response;
}

int socket_binded(uint16_t port) {
    int socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if(socketDescriptor < 0) {
        perror("socket");
        return -1;
    }
    struct sockaddr_in *address = socket_address(INADDR_ANY, port);
    if(bind(socketDescriptor, (struct sockaddr*) address,
            sizeof(struct sockaddr_in))) {
        perror("bind");
        close(socketDescriptor);
        return -1;
    }
    free(address);
    return socketDescriptor;
}

void socket_set_logger(t_log *logger) {
    logger_socket = logger;
}

void socket_destroy_logger() {
    log_destroy(logger_socket);
}
