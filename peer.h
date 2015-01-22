#ifndef _PEER_H_
#define _PEER_H_


#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib.h"

struct job_s* current_job;
bt_config_t config;
int global_socket;

/* the chunks this node already have */
struct chunk_s* has_chunks;
int has_chunks_number;

/* the connections to provide data to other peers */
int max_provider_connection;
struct provider_connection_s* provider_connection_head;

/* the connections to receive data from other peers */
int max_receiver_connection;
struct receiver_connection_s* receiver_connection_head;

void peer_run(bt_config_t *config);
void flood_whohas();

#endif /* _PEER_H_ */