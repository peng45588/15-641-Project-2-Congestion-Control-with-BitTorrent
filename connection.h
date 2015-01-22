#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <stdio.h>
#include <stdlib.h> 
#include "lib.h"

#define PROVIDER_STATUS_SLOW_START 1
#define PROVIDER_STATUS_CONGESTION_AVOIDANCE 2

/* the receiver has not received a single data packet yet */
#define RECEIVER_STATUS_PENGDING_DATA 1
#define RECEIVER_STATUS_RECEIVED_DATA 2

#define RTT_NOT_START -1

struct provider_connection_s{
	bt_peer_t* receiver;
	int id;

	struct chunk_s* chunk;

	int ssthresh;
	int window_size;
	int status;

	/* congestion avoidance last increase time */
	struct timeval CA_last_increase_time;
	int RTT;

	struct timeval RTT_data_sent_time;
	int RTT_ack_num;

	struct timeval last_acked_received_time;
	struct timeval last_data_send_time;
	int num_duplicate_ack;

	/* start from 1 */
	int last_packet_acked;
	int last_packet_sent;

	struct provider_connection_s* next;
};

struct receiver_connection_s{
	bt_peer_t* provider;
	/* the first one is the chunk currently downloading*/
	struct chunk_node_s* chunk_list_head;

	/* the time the connection starts */
	struct timeval start_time;
	/* the time the last get is send out */
	struct timeval last_get_time;
	/* the time the last data received */
	struct timeval last_data_time;
	/* whether this connection has received the first DATA packet*/
	int status;

	struct receiver_connection_s* next;
};

void init_connections();

struct receiver_connection_s* search_receiver_connection(
		struct receiver_connection_s* connection_head, bt_peer_t* provider);

struct provider_connection_s* search_provider_connection(
		struct provider_connection_s* connection_head, bt_peer_t* receiver);
/*
 * add the hash whose chunk is in a CHUNK_STATUS_NOT_FOUND status
 * to the list of this existing connection
 */
void add_ihave_to_receiver_connection(struct receiver_connection_s* receiver_connection,
		struct network_packet_s* ihave_packet);

/*
 * add the chunk to the downloading list of a receiver connection
 */
void add_chunk_to_receiver_connection(struct chunk_s* chunk, 
		struct receiver_connection_s* receiver_connection);

void free_receiver_connection(struct receiver_connection_s* connection);

void cancel_receiver_connection(struct receiver_connection_s* connection);

/* add add the hash whose chunk is in a CHUNK_STATUS_NOT_FOUND status 
 * to a new connection, if no hash whose chunk is in such status
 * no connection should be created
 */
struct receiver_connection_s* malloc_receiver_connection(
		struct network_packet_s* ihave_packet, bt_peer_t* from);

void add_receiver_connection(struct receiver_connection_s* new_receiver_connection);

void remove_receiver_connection(struct receiver_connection_s* target);

/* the data receiver start a connection by sending out GET packet */
struct network_packet_s* start_connection(
		struct receiver_connection_s* new_connection);

/* the data provider accept a connection and set the status*/
struct provider_connection_s* malloc_provider_connection(
		struct network_packet_s* get_packet, bt_peer_t* from);

void add_provider_connection(struct provider_connection_s* new_provider_connection);

void free_provider_connection(struct provider_connection_s* target);

void remove_provider_connection(struct provider_connection_s* target);

/* send out one data packet */
void send_data_packet(struct provider_connection_s* provider_connection);

void save_data_packet(struct network_packet_s* data_packet, 
		struct receiver_connection_s* receiver_connection);

void save_chunk(struct receiver_connection_s* receiver_connection);

void reset_receiver_connection(struct receiver_connection_s* receiver_connection);

void finish_provider_connection(struct provider_connection_s* provider_connection);

int get_provider_connection_number(
		struct provider_connection_s* connection_head);

int get_receiver_connection_number(
		struct receiver_connection_s* connection_head);

void print_receiver_connection(struct receiver_connection_s* receiver_connection);

void print_receiver_connection_list(struct receiver_connection_s* connection_head);

void print_provider_connection(struct provider_connection_s* provider_connection);

void print_provider_connection_list(struct provider_connection_s* connection_head);

#endif /* _CONNECTION_H_ */