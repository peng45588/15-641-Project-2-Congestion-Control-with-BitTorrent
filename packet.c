#include "packet.h"

void print_whohas_packet(struct network_packet_s* whohas_packet){
	printf("*******WHOHAS PACKET BEGIN*******\n");

	print_packet_header(&(whohas_packet -> header));

	char hash_num_field = *(whohas_packet -> payload);
	int hash_num = (int)(hash_num_field);
	printf("total hash number: %d\n", hash_num);
	int i = 0;
	for(i = 0; i < hash_num; i++){
		char hash_buffer[SHA1_HASH_SIZE * 2 + 1];
		memset(hash_buffer, 0, SHA1_HASH_SIZE * 2 + 1);
		binary2hex((uint8_t *)whohas_packet->payload + ALIGNMENT_BYTE_NUM + i * HASH_CHUNK_SIZE, 
				SHA1_HASH_SIZE, hash_buffer);
		printf("chunk hex hash: %s\n", hash_buffer);
	}

	printf("********WHOHAS PACKET END********\n");
}

void print_ihave_packet(struct network_packet_s* ihave_packet){
	printf("*******IHAVE PACKET BEGIN*******\n");

	print_packet_header(&(ihave_packet -> header));

	char hash_num_field = *(ihave_packet -> payload);
	int hash_num = (int)(hash_num_field);
	printf("total hash number: %d\n", hash_num);
	int i = 0;
	for(i = 0; i < hash_num; i++){
		char hash_buffer[SHA1_HASH_SIZE * 2 + 1];
		memset(hash_buffer, 0, SHA1_HASH_SIZE * 2 + 1);
		binary2hex((uint8_t *)ihave_packet->payload + ALIGNMENT_BYTE_NUM + i * HASH_CHUNK_SIZE, 
				SHA1_HASH_SIZE, hash_buffer);
		printf("chunk hex hash: %s\n", hash_buffer);
	}

	printf("********IHAVE PACKET END********\n");
}

void print_get_packet(struct network_packet_s* get_packet){
	printf("*******GET PACKET BEGIN*******\n");

	print_packet_header(&(get_packet -> header));
	char hash_buffer[SHA1_HASH_SIZE * 2 + 1];
	memset(hash_buffer, 0, SHA1_HASH_SIZE * 2 + 1);
	binary2hex((uint8_t *)get_packet->payload, SHA1_HASH_SIZE, hash_buffer);
	printf("chunk hex hash: %s\n", hash_buffer);

	printf("********GET PACKET END********\n");
}

void print_data_packet(struct network_packet_s* data_packet){
    printf("*******DATA PACKET BEGIN*******\n");
    print_packet_header(&(data_packet -> header));
    printf("********DATA PACKET END********\n");
}

void print_ack_packet(struct network_packet_s* ack_packet){
    printf("*******ACK PACKET BEGIN*******\n");
    print_packet_header(&(ack_packet -> header));
    printf("********ACK PACKET END********\n");	
}


void print_packet_header(struct header_s* header){
	printf("magic number: %d; ", header -> magic_number);
	printf("version number: %d; ", header -> version_number);
	printf("packet type: %d\n", header -> packet_type);

	printf("header length: %d; ", header -> header_length);
	printf("packet length: %d\n", header -> packet_length);

	printf("sequence number: %d\n", header -> seq_number);

	printf("acknowledgment number: %d\n", header -> ack_number);
}


/*
 * send whohas packet to everyone
 */
void send_all_whohas_packet(struct network_packet_s* whohas_packets, 
		int packet_num, int socket, struct sockaddr* to){
	int i = 0;
  	for(i = 0; i < packet_num; i++){
  		struct network_packet_s temp_packet = whohas_packets[i];
  		
  		int packet_length = temp_packet.header.packet_length;
  		/* change to network order */
  		host_to_net_header(&(temp_packet.header));
  		spiffy_sendto(socket, &temp_packet, packet_length, 0, to, sizeof(*to));
  		//printf("send result %d \n", result);
  	}
}


/*
 * send out a packet
 */
void send_packet(struct network_packet_s* packet, int socket, struct sockaddr* to){
	struct network_packet_s temp_packet = *packet;
	int packet_length = temp_packet.header.packet_length;
  	/* change to network order */
	host_to_net_header(&(temp_packet.header));
	spiffy_sendto(socket, &temp_packet, packet_length, 0, to, sizeof(*to));

	printf("send packet type: %d, seq: %d, ack: %d\n ", (int)(packet->header.packet_type),
			packet->header.seq_number,
			packet->header.ack_number);
	//printf("send result %d \n", result);
}


/*
 * @return the type of this packet
 */
int get_packet_type(struct header_s* header){
	return header -> packet_type;
}

/*
 * @return 1 if this packet is valid, else return -1
 */
int validate_packet(struct header_s* header){
	if(header->magic_number != MAGIC_NUMBER){
		return -1;
	}
	if(header->version_number != VERSION_NUMBER){
		return -1;
	}
	if(header->packet_type < 0 || header->packet_type > 5){
		return -1;
	}
	return 1;
}


void host_to_net_header(struct header_s* header) {
    header->magic_number = htons(header->magic_number);
    header->header_length = htons(header->header_length);
    header->packet_length = htons(header->packet_length);
    header->seq_number = htonl(header->seq_number);
    header->ack_number = htonl(header->ack_number);
}

void net_to_host_header(struct header_s* header) {
	header->magic_number = ntohs(header->magic_number);
    header->header_length = ntohs(header->header_length);
    header->packet_length = ntohs(header->packet_length);
    header->seq_number = ntohl(header->seq_number);
    header->ack_number = ntohl(header->ack_number);
}

