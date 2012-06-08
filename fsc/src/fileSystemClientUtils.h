#ifndef FILESYSTEMCLIENTUTILS_H_
#define FILESYSTEMCLIENTUTILS_H_

#include "src/nipc/nipc.h"

struct nipc_packet *requestRemoteFileSystem(struct nipc_packet *packet);

#endif /* FILESYSTEMCLIENTUTILS_H_ */
