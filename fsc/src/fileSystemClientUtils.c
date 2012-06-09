#include "fileSystemClientUtils.h"
#include "src/sockets/sockets.h"
#include <sys/types.h>
#include <stdio.h>

struct nipc_packet *requestRemoteFileSystem(struct nipc_packet *request) {
    void *rawRequest = NULL;
    size_t requestSize = nipc_serialize(request, &rawRequest);
    int socketDescriptor = socket_connect(remoteFileSystemIP, remotePort);
    void *rawResponse = NULL;
    size_t responseSize = socket_query(socketDescriptor, rawRequest,
            requestSize, &rawResponse);
    return nipc_deserialize(rawResponse, responseSize);
}
