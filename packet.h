#ifndef _PACKET_H_
#define _PACKET_H_

#include <arpa/inet.h>
#include <errno.h>
#include "lib.h"

#define MAGIC_NUMBER 15441
#define VERSION_NUMBER 1

#define TYPE_WHOHAS 0
#define TYPE_IHAVE 1
#define TYPE_GET 2
#define TYPE_DATA 3
#define TYPE_ACK 4
#define TYPE_DENIED 5

/* assume the maximum packet size for UDP is 1500 bytes */
#define MAX_PACKET_LENGTH 1500
#define HEADER_SIZE 16
#define MAX_PAYLOAD_LENGTH (MAX_PACKET_LENGTH - HEADER_SIZE)
#define MAX_HASH_LIST_NUM 74

#define ALIGNMENT_BYTE_NUM 4
#define HASH_CHUNK_SIZE 20

#define DATA_PACKET_PAYLAOD_SIZE 1024
#define CHUNK_DATA_NUM 512
#define CHUNK_DATA_SIZE (512*1024)

struct header_s{
    short magic_number; 	// 2 bytes
    char version_number; 	// 1 byte
    char packet_type; 	// 1 byte
    short header_length; 	// 2 bytes
    short packet_length;  	// 2 bytes
    int seq_number; 	// 4 bytes
    int ack_number; 	// 4 bytes
};

/* physical packet send through netwrok */
struct network_packet_s{
	struct header_s header;
	/* the blank field after the end of hash list will not be send*/
	char payload[MAX_PAYLOAD_LENGTH];
};

void print_ihave_packet(struct network_packet_s* ihave_packet);
void print_whohas_packet(struct network_packet_s* whohas_packet);
void print_get_packet(struct network_packet_s* get_packet);
void print_data_packet(struct network_packet_s* data_packet);
void print_ack_packet(struct network_packet_s* ack_packet);
void print_packet_header(struct header_s* header);

/*
 * send out whohas packet array
 */
void send_all_whohas_packet(struct network_packet_s* whohas_packets, 
		int packet_num, int socket, struct sockaddr* to);

/*
 * send out a packet
 */
void send_packet(struct network_packet_s* packet, int socket, struct sockaddr* to);

/*
 * @return the type of this packet
 */
int get_packet_type(struct header_s* header);

/*
 * @return 1 if this packet is valid, else return -1
 */
int validate_packet(struct header_s* header);


void host_to_net_header(struct header_s* header);
void net_to_host_header(struct header_s* header);



#endif /* _PACKET_H_ */