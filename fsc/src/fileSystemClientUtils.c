#include "fileSystemClientUtils.h"
#include "src/sockets/sockets.h"
#include <sys/types.h>
#include <stdio.h>

//FIXME: parametrizar
#define remoteFileSystemIP "127.0.0.1"
#define remotePort 3087

struct nipc_packet *requestRemoteFileSystem(struct nipc_packet *request) {
    void *rawRequest;
    size_t requestSize = nipc_serialize(request, rawRequest);
    int socketDescriptor = socket_connect(remoteFileSystemIP, remotePort);
    void *rawResponse = NULL;
    size_t responseSize = socket_query(socketDescriptor, rawRequest, requestSize, rawResponse);
    return nipc_deserialize(rawResponse, responseSize);
}
