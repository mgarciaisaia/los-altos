#ifndef FILESYSTEMCLIENTUTILS_H_
#define FILESYSTEMCLIENTUTILS_H_

#include "src/nipc/nipc.h"
//FIXME: parametrizar
#define remoteFileSystemIP "127.0.0.1"
#define remotePort 3087

struct nipc_packet *requestRemoteFileSystem(struct nipc_packet *packet);

#endif /* FILESYSTEMCLIENTUTILS_H_ */
