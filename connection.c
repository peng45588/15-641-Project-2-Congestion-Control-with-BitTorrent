#include "connection.h"

void init_connections(){
 	max_provider_connection = config.max_conn;
	provider_connection_head = NULL;

	max_receiver_connection = config.max_conn;
	receiver_connection_head = NULL;
}


struct receiver_connection_s* search_receiver_connection(
		struct receiver_connection_s* connection_head, bt_peer_t* provider){
	struct receiver_connection_s* temp = connection_head;
	while(temp != NULL){
		if(temp -> provider == provider){
			return temp;
		}
		temp = temp -> next;
	}
	return NULL;
}

struct provider_connection_s* search_provider_connection(
		struct provider_connection_s* connection_head, bt_peer_t* receiver){
	struct provider_connection_s* temp = connection_head;
	while(temp != NULL){
		if(temp -> receiver == receiver){
			return temp;
		}
		temp = temp -> next;
	}
	return NULL;
}

/*
 * add the hash whose chunk is in a CHUNK_STATUS_NOT_FOUND status
 * to the list of this existing connection
 */
void add_ihave_to_receiver_connection(struct receiver_connection_s* receiver_connection,
		struct network_packet_s* ihave_packet){
	char hash_num_field = *(ihave_packet->payload);
	int hash_num = (int) hash_num_field;
	/* the number of hash values should not exceed the maximum */
	hash_num = hash_num > MAX_HASH_LIST_NUM? MAX_HASH_LIST_NUM:hash_num;
	int i = 0;
	for(i = 0; i < hash_num; i++){
		struct chunk_s* temp_chunk = get_chunk((uint8_t*) 
			ihave_packet->payload + ALIGNMENT_BYTE_NUM + i * SHA1_HASH_SIZE);
		if(temp_chunk != NULL){
			if(temp_chunk -> status == CHUNK_STATUS_NOT_FOUND){
				add_chunk_to_receiver_connection(temp_chunk,receiver_connection);
			}
		}
		else{
			printf("WARNING: received IHAVE hash which is not need by me!\n");
		}

	}
}

/*
 * add the chunk to the end of downloading list of a receiver connection
 * change status into CHUNK_STATUS_PENDING
 */
void add_chunk_to_receiver_connection(struct chunk_s* chunk,
		struct receiver_connection_s* receiver_connection){
	struct chunk_node_s* chunk_node = malloc_chunk_node(chunk);

	/* change the status of this chunk */
	chunk -> status = CHUNK_STATUS_PENDING;
	//chunk -> provider = receiver_connection -> provider;

	if(receiver_connection -> chunk_list_head == NULL){
		receiver_connection -> chunk_list_head = chunk_node;
		return;
	}
	struct chunk_node_s* temp = receiver_connection -> chunk_list_head;
	while(temp -> next != NULL){
		temp = temp -> next;
	}
	temp -> next = chunk_node;
	chunk -> status = CHUNK_STATUS_PENDING;
	return;
}

void free_receiver_connection(struct receiver_connection_s* connection){
	struct chunk_node_s* temp = connection -> chunk_list_head;
	while(temp != NULL){
		struct chunk_node_s* next = temp -> next;
		free_chunk_node(temp);
		temp = next;
	}
	free(connection);
}

void cancel_receiver_connection(struct receiver_connection_s* connection){
	struct chunk_node_s* temp = connection -> chunk_list_head;
	while(temp != NULL){
		if(temp->chunk->status != CHUNK_STATUS_FINISHED){
			temp->chunk->status = CHUNK_STATUS_NOT_FOUND;
			temp->chunk->received_seq_num = 0;
			temp->chunk->received_byte_num = 0;
		}
		temp = temp -> next;
	}
	free_receiver_connection(connection);
}

/* add add the hash whose chunk is in a CHUNK_STATUS_NOT_FOUND status 
 * to a new connection, if no hash whose chunk is in such status
 * no connection should be created
 */
struct receiver_connection_s* malloc_receiver_connection(
		struct network_packet_s* ihave_packet, bt_peer_t* from){
	struct receiver_connection_s* receiver_connection = malloc(
		sizeof(struct receiver_connection_s));
	receiver_connection -> provider = from;
	receiver_connection -> chunk_list_head = NULL;
	receiver_connection -> next = NULL;
	init_receiver_connection(receiver_connection);
	add_ihave_to_receiver_connection(receiver_connection, ihave_packet);
	/* all hash has already have a provider */
	if(receiver_connection -> chunk_list_head == NULL){
		free_receiver_connection(receiver_connection);
		printf("malloc_receiver_connection(), every hash has a provider, return NULL new connection\n");
		return NULL;
	}
	else{
		return receiver_connection;
	}
}

void add_receiver_connection(struct receiver_connection_s* new_receiver_connection){
	if(new_receiver_connection == NULL){
		return;
	}
	new_receiver_connection -> next = receiver_connection_head;
	receiver_connection_head = new_receiver_connection;
}

void remove_receiver_connection(struct receiver_connection_s* target){
	struct receiver_connection_s* temp = receiver_connection_head;
	if(temp == NULL){
		return;
	}
	if(temp == target){
		receiver_connection_head = receiver_connection_head->next;
		return;
	}
	while(temp -> next != NULL){
		if(temp -> next == target){
			temp -> next = target -> next;
			return;
		}
		temp = temp -> next;
	}
}


struct network_packet_s* start_connection( 
		struct receiver_connection_s* new_connection){
	struct network_packet_s* get_packet = malloc_get_packet(
			new_connection->chunk_list_head->chunk);
	/* set the state of this connection */
	new_connection->chunk_list_head->chunk->status = CHUNK_STATUS_DOWNDLOADING;
	gettimeofday(&new_connection->last_get_time, NULL);

	send_packet(get_packet, global_socket, 
			(struct sockaddr *) &(new_connection->provider->addr));
	return get_packet;
}

struct provider_connection_s* malloc_provider_connection(
		struct network_packet_s* get_packet, bt_peer_t* from){
	struct chunk_s* chunk_requested = load_chunk((uint8_t*) get_packet->payload);
	if(chunk_requested == NULL){
		return NULL;
	}

	struct provider_connection_s* provider_connection = malloc(
		sizeof(struct provider_connection_s));
	provider_connection->id = provider_id;
	provider_id ++;
	provider_connection -> receiver = from;	

	provider_connection -> chunk = chunk_requested;

	init_provider_connection(provider_connection);

	provider_connection -> last_packet_acked = 0;
	provider_connection -> last_packet_sent = 0;

	provider_connection -> next = NULL;
	
	return provider_connection;
}

void add_provider_connection(struct provider_connection_s* new_provider_connection){
	if(new_provider_connection == NULL){
		return;
	}
	new_provider_connection -> next = provider_connection_head;
	provider_connection_head = new_provider_connection;
}

void free_provider_connection(struct provider_connection_s* target){
	free(target);
}

void remove_provider_connection(struct provider_connection_s* target){
	struct provider_connection_s* temp = provider_connection_head;
	if(temp == NULL){
		return;
	}
	if(temp == target){
		provider_connection_head = provider_connection_head->next;
		return;
	}
	while(temp -> next != NULL){
		if(temp -> next == target){
			temp -> next = target -> next;
			return;
		}
		temp = temp -> next;
	}
}

void send_data_packet(struct provider_connection_s* provider_connection){
	int packet_pending_ack_num = provider_connection->last_packet_sent - 
			provider_connection->last_packet_acked;
	/*  sender window is full, do not send any packet */
	if(packet_pending_ack_num == provider_connection->window_size){
		return;
	}
	/* this should not happen */
	if(packet_pending_ack_num > provider_connection->window_size){
		printf("ERROR: window size is smaller than the number of packets pending acked\n");
		return;
	}
	/* all the data packet has been sent out */
	if(provider_connection->last_packet_sent == CHUNK_DATA_NUM){
		return;
	}

	/* initial value of last_packet_sent is 0 */
	/* seqence number start from 1 */
	provider_connection->last_packet_sent = provider_connection->last_packet_sent + 1;

	struct network_packet_s* data_packet = malloc_data_packet(
			provider_connection->chunk->data + 
					(provider_connection->last_packet_sent -1) * DATA_PACKET_PAYLAOD_SIZE, 
			DATA_PACKET_PAYLAOD_SIZE,
			provider_connection->last_packet_sent);
	send_packet(data_packet, global_socket, 
			(struct sockaddr *) &(provider_connection->receiver->addr));

	if(provider_connection->RTT_ack_num == RTT_NOT_START){
		gettimeofday(&provider_connection->RTT_data_sent_time, NULL);
		provider_connection->RTT_ack_num = data_packet->header.seq_number;
	}
	gettimeofday(&provider_connection->last_data_send_time, NULL);
	// char str[20];
	// inet_ntop(AF_INET, &(provider_connection->receiver->addr.sin_addr), str, INET_ADDRSTRLEN);
	// printf("data packet send to peer <ID:%d> <ip:%s> <port:%d>\n",
 //                provider_connection->receiver->id, str, 
 //                ntohs(provider_connection->receiver->addr.sin_port));
}

/* this will change the state of the receiver connection */
void save_data_packet(struct network_packet_s* data_packet, 
		struct receiver_connection_s* receiver_connection){
	/* save data to this chunk */
	struct chunk_s* chunk = receiver_connection->chunk_list_head->chunk;
	int seq_num = data_packet->header.seq_number;
	//int offset = (seq_num - 1) * DATA_PACKET_PAYLAOD_SIZE;

	if(chunk->data == NULL){
		chunk->data = malloc(sizeof(char) * CHUNK_DATA_SIZE);
	}

	int data_size = data_packet->header.packet_length - 
		data_packet->header.header_length;
	if(data_size < 0){
		data_size = 0;
	} 
	if(data_size > MAX_PAYLOAD_LENGTH){
		data_size = MAX_PAYLOAD_LENGTH;
	}

	memcpy(chunk->data + chunk->received_byte_num, 
			data_packet->payload, 
			data_size);

	/* change the sequence number received of this chunk */
	chunk->received_seq_num = seq_num;
	chunk->received_byte_num = chunk->received_byte_num + data_size;
}

/* check the first chunk of this connection, 
 * if the whole chunk has been received, 
 * save back to file.
 */
void save_chunk(struct receiver_connection_s* receiver_connection){
	struct chunk_s* chunk = receiver_connection->chunk_list_head->chunk;
	if(chunk->received_byte_num == CHUNK_DATA_SIZE){
		chunk -> status = CHUNK_STATUS_FINISHED;
		printf("Finished Downloading Chunk: \n");
		print_chunk(chunk);

		/* verify the hash of the data */
		uint8_t hash[SHA1_HASH_SIZE];
		shahash((uint8_t *)chunk->data, CHUNK_DATA_SIZE, hash);

		char received_hash_buffer[SHA1_HASH_SIZE * 2 + 1];
		memset(received_hash_buffer, 0, SHA1_HASH_SIZE * 2 + 1);
		binary2hex(hash, SHA1_HASH_SIZE, received_hash_buffer);
		printf("received hex hash: %s\n", received_hash_buffer);

		/* the chunk received is broken */
		if (memcmp(hash, chunk->hash_binary, SHA1_HASH_SIZE) != 0){
			printf("Verify Fail \n");
			chunk -> status = CHUNK_STATUS_NOT_FOUND;
		}
		else{
			printf("Verify Success \n");
			chunk -> status = CHUNK_STATUS_FINISHED;

			/* save the chunk to file */
			FILE* data_file = fopen(current_job->output_file_name, "a+");
			if(data_file == NULL){
				printf("Error: fail to save chunk <%d>, can not open output file <%s>\n", 
						chunk->id, current_job->output_file_name);
				chunk -> status = CHUNK_STATUS_NOT_FOUND;
			}
			else{
				int offset = chunk->id * CHUNK_DATA_SIZE;
				fseek(data_file, offset, SEEK_SET);
				int write_size = fwrite(chunk->data, sizeof(char), CHUNK_DATA_SIZE, data_file);
				if(write_size != CHUNK_DATA_SIZE){
					printf("WARNING: fail to save chunk <%d>, can not write consecutive 512*1024 bytes to <%s>\n", 
							chunk->id, current_job->output_file_name);
					chunk -> status = CHUNK_STATUS_NOT_FOUND;
				}
				else{
					printf("save chunk <%d> to file <%s> successfully\n",
							chunk->id, current_job->output_file_name);
				}
				fclose(data_file);
			}
		}
		/* free the data of this chunk to save space */
		free(chunk->data);
		chunk->data = NULL;
		chunk->received_seq_num = 0;
	 	chunk->received_byte_num = 0;
	}
}

/* make the receiver connection to GET next chunk in its list
 * if the first chunk has been download
 */
void reset_receiver_connection(struct receiver_connection_s* receiver_connection){
	struct chunk_s* chunk = receiver_connection->chunk_list_head->chunk;
	if(chunk->status == CHUNK_STATUS_DOWNDLOADING){
		return;
	}
	struct chunk_node_s* finished_node = receiver_connection->chunk_list_head;
	receiver_connection->chunk_list_head = receiver_connection->chunk_list_head->next;
	free_chunk_node(finished_node);
	if(receiver_connection->chunk_list_head != NULL){
		/* create new connection if there are more chunk to downlaod */
		printf("GET next chunk with the same peer %d\n", receiver_connection->provider->id);
		struct network_packet_s* get_packet = start_connection(receiver_connection);
		free(get_packet);
	}
	else{
		/* no more chunk to download, free this receiver connection */
		printf("Receiver connection finished all chunk with peer %d\n",receiver_connection->provider->id);
		remove_receiver_connection(receiver_connection);
		free_receiver_connection(receiver_connection);
	}
}


void finish_provider_connection(struct provider_connection_s* provider_connection){
	printf("Provider connection finished with peer %d\n", 
			provider_connection->receiver->id);
	remove_provider_connection(provider_connection);
	free_provider_connection(provider_connection);
}

int get_provider_connection_number(
		struct provider_connection_s* connection_head){
	int count = 0;
	while(connection_head != NULL){
		count ++;
		connection_head = connection_head->next;
	}
	return count;
}


int get_receiver_connection_number(
		struct receiver_connection_s* connection_head){
	int count = 0;
	while(connection_head != NULL){
		count ++;
		connection_head = connection_head->next;
	}
	return count;
}

void print_receiver_connection(struct receiver_connection_s* receiver_connection){
	printf("*******START RECEIVER CONNECTION********\n");
	bt_peer_t* peer = receiver_connection->provider;
	printf("peer <%d>:%s:%d\n", peer->id, 
			inet_ntoa(peer->addr.sin_addr), ntohs(peer->addr.sin_port));
	struct chunk_node_s* temp = receiver_connection->chunk_list_head;
	while(temp != NULL){
		print_chunk(temp -> chunk);
		temp = temp -> next;
	}
	printf("*******END RECEIVER CONNECTION********\n");
}

void print_receiver_connection_list(struct receiver_connection_s* connection_head){
	printf(">>>>>>>>START RECEIVER CONNECTION LIST\n");
	int number = get_receiver_connection_number(connection_head);
	printf("There are totoal %d connections\n", number);
	struct receiver_connection_s* temp = connection_head;
	while(temp != NULL){
		print_receiver_connection(temp);
		temp = temp -> next;
	}
	printf(">>>>>>>>END RECEIVER CONNECTION LIST\n");
}

void print_provider_connection(struct provider_connection_s* provider_connection){
	printf("*******START PROVIDER CONNECTION********\n");
	bt_peer_t* peer = provider_connection->receiver;
	printf("peer <%d>:%s:%d\n", peer->id, 
			inet_ntoa(peer->addr.sin_addr), ntohs(peer->addr.sin_port));
	print_chunk(provider_connection -> chunk);
	printf("*******END PROVIDER CONNECTION********\n");
}

void print_provider_connection_list(struct provider_connection_s* connection_head){
	printf(">>>>>>>>START PROVIDER CONNECTION LIST\n");
	int number = get_provider_connection_number(connection_head);
	printf("There are totoal %d connections\n", number);
	struct provider_connection_s* temp = connection_head;
	while(temp != NULL){
		print_provider_connection(temp);
		temp = temp -> next;
	}
	printf(">>>>>>>>END PROVIDER CONNECTION LIST\n");
}