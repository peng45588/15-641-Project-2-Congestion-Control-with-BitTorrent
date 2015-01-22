#include "job.h"

struct job_s* malloc_new_job(char* chunk_file_name, char* output_file_name){
	FILE* chunk_file = fopen(chunk_file_name,"r");
	if(chunk_file == NULL){
        return NULL;
	}

	struct job_s* job = (struct job_s*)malloc(sizeof(struct job_s));

	int chunk_num = 0;
	char read_buffer[CHUNK_FILE_LINE_BUFFER_SIZE];
	while (fgets(read_buffer, CHUNK_FILE_LINE_BUFFER_SIZE, chunk_file)) {
        chunk_num++;
    }
	memset(read_buffer, 0, CHUNK_FILE_LINE_BUFFER_SIZE);

    job -> chunk_num = chunk_num;
    job -> chunks = malloc(sizeof(struct chunk_s) * job -> chunk_num);
    memset(job -> output_file_name, 0, 128);
    memcpy(job -> output_file_name, output_file_name, 128);

    /* remember to initilize other member in the struct job_s to be NULL or 0 */
    /* remember to initilize other member in the struct chunk_s to be NULL or 0 */

	fseek(chunk_file, 0, SEEK_SET);
	char hash_buffer[SHA1_HASH_SIZE*2];
	int count = 0;
	while (fgets(read_buffer, CHUNK_FILE_LINE_BUFFER_SIZE, chunk_file)) {
		sscanf(read_buffer,"%d %s", & ((job -> chunks)[count].id), hash_buffer);
		hex2binary(hash_buffer, SHA1_HASH_SIZE * 2, (job -> chunks)[count].hash_binary);

		(job -> chunks)[count].status = CHUNK_STATUS_NOT_FOUND;
		(job -> chunks)[count].received_seq_num = 0;
		(job -> chunks)[count].received_byte_num = 0;
		(job -> chunks)[count].data = NULL;
		//(job -> chunks)[count].provider = NULL;
		count++;
	}

    return job;
}

struct chunk_node_s* malloc_chunk_node(struct chunk_s* chunk){
	struct chunk_node_s* chunk_node = malloc(sizeof(struct chunk_node_s));
	chunk_node -> chunk = chunk;
	chunk_node -> next = NULL;
	return chunk_node;
}

void free_job(struct job_s* job){
	free(job -> chunks);
	free(job);
}

void free_chunk_node(struct chunk_node_s* chunk_node){
	free(chunk_node);
}

struct chunk_s* load_chunk(uint8_t* hash_binary){
	struct chunk_s* chunk = seach_owned_chunk(hash_binary);
	if(chunk == NULL){
		return NULL;
	}
	/* the chunk might has already been load */
	if(chunk -> data == NULL){
		/* open master chunk file to get the location of the data file */
		char read_buffer[256];
		memset(read_buffer, 0, 256);
		FILE* master_chunk_file = fopen(config.chunk_file, "r");
		if(master_chunk_file == NULL){
			printf("WARNING: fail to load chunk <%d>, can not open master chunk file <%s>\n", 
					chunk->id, read_buffer);
			return NULL;
		}
		fgets(read_buffer, 256, master_chunk_file);
		fclose(master_chunk_file);
		/* parse to get the path */
		char data_file_name[256];
		memset(data_file_name, 0, 256);
		sscanf(read_buffer, "File:%s", data_file_name);

		/* open data file, load data to buffer */
		FILE* data_file = fopen(data_file_name, "r");
		if(data_file == NULL){
			printf("WARNING: fail to load chunk <%d>, can not open data file <%s>\n", 
					chunk->id, data_file_name);
			return NULL;
		}
		char data_buffer[CHUNK_DATA_SIZE];
		int offset = chunk->id * CHUNK_DATA_SIZE;
		fseek(data_file, offset, SEEK_SET);
		int read_size = fread(data_buffer, sizeof(char), CHUNK_DATA_SIZE, data_file);
		if(read_size != CHUNK_DATA_SIZE){
			printf("WARNING: fail to load chunk <%d>, can not read consecutive 512*1024 bytes from <%s>\n", 
					chunk->id, data_file_name);
			return NULL;
		}
		printf("load chunk <%d> from file <%s> successfully\n", chunk->id, data_file_name);
		fclose(data_file);

		/* copy data to this chunk */
		chunk -> data = malloc(sizeof(char) * CHUNK_DATA_SIZE);
		memcpy(chunk -> data, data_buffer, CHUNK_DATA_SIZE);
	}
	return chunk;
}

void print_job(struct job_s* job){
	printf("*******JOB BEGIN*******\n");
	printf("chunk number: %d\n", job -> chunk_num);
	int i = 0;
	for(i = 0; i < job -> chunk_num; i++){
		print_chunk( &((job -> chunks)[i]) );
	}

	printf("********JOB END********\n");
}

void print_chunk(struct chunk_s* chunk){
	printf("chunk id: %d; ", chunk -> id);

	char hash_buffer[SHA1_HASH_SIZE * 2 + 1];
	memset(hash_buffer, 0, SHA1_HASH_SIZE * 2 + 1);
	binary2hex(chunk -> hash_binary, SHA1_HASH_SIZE, hash_buffer);

	printf("chunk hex hash: %s", hash_buffer);
	printf("\n");
}

/* count the chunk which is CHUNK_STATUS_NOT_FOUND in the job */
int get_not_found_chunk_number(struct job_s* job){
	int not_found_chunk_num = 0;
	int i = 0; 
	for(i = 0; i < job->chunk_num; i++){
		if(job->chunks[i].status == CHUNK_STATUS_NOT_FOUND){
			not_found_chunk_num++;
		}
	}
	return not_found_chunk_num;
}

struct network_packet_s* malloc_all_whohas_packet(int* num, struct job_s* job){
	int not_found_chunk_num = get_not_found_chunk_number(job);

	int packet_number = (not_found_chunk_num % MAX_HASH_LIST_NUM) == 0 ? 
		(not_found_chunk_num / MAX_HASH_LIST_NUM) : 
		(not_found_chunk_num / MAX_HASH_LIST_NUM + 1);

	// printf("malloc_all_whohas_packet, chunk number: %d, packet number: %d\n",
	// 	not_found_chunk_num, packet_number);

	if(not_found_chunk_num == 0){
		return NULL;
	}

	struct network_packet_s* whohas_packets = malloc(
			sizeof(struct network_packet_s) * packet_number);
	
	int packet_count = 0;
	int chunk_count = 0;
	while(packet_count < packet_number){
		int hash_num = (packet_number - packet_count) > 1 ? MAX_HASH_LIST_NUM :
				(not_found_chunk_num - packet_count * MAX_HASH_LIST_NUM);

		whohas_packets[packet_count].header.magic_number = MAGIC_NUMBER;
		whohas_packets[packet_count].header.version_number = VERSION_NUMBER;
		whohas_packets[packet_count].header.packet_type = TYPE_WHOHAS;
		whohas_packets[packet_count].header.header_length = HEADER_SIZE;
		whohas_packets[packet_count].header.packet_length = HEADER_SIZE + ALIGNMENT_BYTE_NUM + 
				HASH_CHUNK_SIZE * hash_num;
		whohas_packets[packet_count].header.seq_number = 0; // invalid
		whohas_packets[packet_count].header.ack_number = 0; // invalid

  		char hash_field = (char)hash_num;
		*(whohas_packets[packet_count].payload) = hash_field;

		int j = 0;
		for(j = 0; j < hash_num; j++){
			for(; chunk_count < job->chunk_num; chunk_count++){
				if(job->chunks[chunk_count].status == CHUNK_STATUS_NOT_FOUND){
					memcpy(whohas_packets[packet_count].payload + ALIGNMENT_BYTE_NUM
						+ j * HASH_CHUNK_SIZE,
						(job->chunks)[chunk_count].hash_binary,
						HASH_CHUNK_SIZE
						);
					chunk_count++;
					break;
				}	
			}
		}
		packet_count++;
	}

	*num = packet_number;
	return whohas_packets;
}


/*
 * @return NULL if this node doesn't have any chunks
 */
struct network_packet_s* malloc_ihave_packet(struct network_packet_s* whohas_packet){
	int ihave_hash_number = 0;
	struct network_packet_s* ihave_packet = malloc(sizeof(struct network_packet_s));


	(ihave_packet->header).magic_number = MAGIC_NUMBER;
	(ihave_packet->header).version_number = VERSION_NUMBER;
	(ihave_packet->header).packet_type = TYPE_IHAVE;
	(ihave_packet->header).header_length = HEADER_SIZE;
	
	(ihave_packet->header).seq_number = 0; // invalid
	(ihave_packet->header).ack_number = 0; // invalid


	char request_hash_number_field  = *(whohas_packet->payload);
	int request_hash_number = request_hash_number_field;
	/* the number of hash values should not exceed the maximum */
	request_hash_number = request_hash_number_field > MAX_HASH_LIST_NUM?
			MAX_HASH_LIST_NUM:request_hash_number_field;
	int i = 0;
	for(i = 0; i < request_hash_number; i++){
		uint8_t* hash_binary = (uint8_t *) (whohas_packet->payload + 
				ALIGNMENT_BYTE_NUM + i * SHA1_HASH_SIZE);
		if(seach_owned_chunk(hash_binary) != NULL){
			memcpy(ihave_packet->payload + ALIGNMENT_BYTE_NUM + 
						ihave_hash_number * SHA1_HASH_SIZE,
					hash_binary,
					SHA1_HASH_SIZE
				);
			ihave_hash_number++;
		}
	}

	if(ihave_hash_number == 0){
		free(ihave_packet);
		return NULL;
	}

	(ihave_packet->header).packet_length = HEADER_SIZE + ALIGNMENT_BYTE_NUM + 
			HASH_CHUNK_SIZE * ihave_hash_number;

	char hash_field = (char)ihave_hash_number;
	*(ihave_packet->payload) = hash_field;

	return ihave_packet;
}

struct network_packet_s* malloc_get_packet(struct chunk_s* chunk){
	struct network_packet_s* get_packet = malloc(sizeof(struct network_packet_s));

	(get_packet->header).magic_number = MAGIC_NUMBER;
	(get_packet->header).version_number = VERSION_NUMBER;
	(get_packet->header).packet_type = TYPE_GET;
	(get_packet->header).header_length = HEADER_SIZE;
	(get_packet->header).packet_length = HEADER_SIZE + HASH_CHUNK_SIZE;

	(get_packet->header).seq_number = 0; // invalid
	(get_packet->header).ack_number = 0; // invalid

	memcpy(get_packet->payload, chunk->hash_binary, SHA1_HASH_SIZE);

	return get_packet;
}

struct network_packet_s* malloc_data_packet(char* data, int data_size, int seq_num){
	struct network_packet_s* data_packet = malloc(sizeof(struct network_packet_s));

	(data_packet->header).magic_number = MAGIC_NUMBER;
	(data_packet->header).version_number = VERSION_NUMBER;
	(data_packet->header).packet_type = TYPE_DATA;
	(data_packet->header).header_length = HEADER_SIZE;
	(data_packet->header).packet_length = HEADER_SIZE + data_size;

	(data_packet->header).seq_number = seq_num;
	(data_packet->header).ack_number = 0; // invalid

	memcpy(data_packet->payload, data, data_size);
	return data_packet;
}

struct network_packet_s* malloc_ack_packet(int ack_num){
	struct network_packet_s* ack_packet = malloc(sizeof(struct network_packet_s));

	(ack_packet->header).magic_number = MAGIC_NUMBER;
	(ack_packet->header).version_number = VERSION_NUMBER;
	(ack_packet->header).packet_type = TYPE_ACK;
	(ack_packet->header).header_length = HEADER_SIZE;
	(ack_packet->header).packet_length = HEADER_SIZE;

	(ack_packet->header).seq_number = 0; // invalid
	(ack_packet->header).ack_number = ack_num; 

	return ack_packet;
}


struct chunk_s* get_chunk(uint8_t* hash_binary){
	if(hash_binary == NULL){
		printf("struct chunk_s* get_chunk(uint8_t*), parameter is NULL\n");
		return NULL;
	}
	int i = 0;
	for(i = 0; i < current_job->chunk_num; i++){
		struct chunk_s* temp_chunk = &(current_job->chunks[i]);
		if (memcmp(hash_binary, temp_chunk->hash_binary, SHA1_HASH_SIZE) == 0) {
			return temp_chunk;
		}
	}
	return NULL;
}

/*
 *
 * @return the chunk if this nod have this chunk specified by the hash, or return NULL
 */
struct chunk_s* seach_owned_chunk(uint8_t* hash_binary){
	int i = 0;
	for(i = 0; i < has_chunks_number; i++){
		if (memcmp(hash_binary, has_chunks[i].hash_binary, SHA1_HASH_SIZE) == 0) {
			return &has_chunks[i];
		}
	}
	return NULL;
}

