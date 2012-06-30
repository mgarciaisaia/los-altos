#ifndef SOCKETS_H_
#define SOCKETS_H_
#include <stdint.h>
#include "../commons/log.h"

struct nipc_packet *nipc_query(struct nipc_packet *request, char *remoteIP, uint16_t port);
struct nipc_packet *nipc_receive(int socketDescriptor);
int nipc_send(int socketDescriptor, struct nipc_packet *request);

int socket_connect(char *remoteIP, uint16_t port);
int socket_binded(uint16_t port);

void socket_set_logger(t_log *logger);
void socket_destroy_logger();

#endif /* SOCKETS_H_ */
