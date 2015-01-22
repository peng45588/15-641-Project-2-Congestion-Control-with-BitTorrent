#ifndef _JOB_H_
#define _JOB_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include "lib.h"

#define CHUNK_FILE_LINE_BUFFER_SIZE 64

#define CHUNK_STATUS_NOT_FOUND -1
#define CHUNK_STATUS_DOWNDLOADING 0
#define CHUNK_STATUS_PENDING 1
#define CHUNK_STATUS_FINISHED 2

struct chunk_s {
	int id;
	uint8_t hash_binary[SHA1_HASH_SIZE];

	/* status of each chunk, used for receiver
	 * CHUNK_STATUS_NOT_FOUND -1;
	 * CHUNK_STATUS_DOWNDING 0;
	 * CHUNK_STATUS_PENDING 1;
	 * CHUNK_STATUS_FINISHED 2;
	 * there is only one job for one provider is downding
	 * initial value is CHUNK_STATUS_NOT_FOUND
	 */
	int status;
	/* initial value is 0 */
	int received_seq_num;
	int received_byte_num;

	/* used by the data provider, the size of data is fixed as CHUNK_DATA_SIZE, 512*1024 Bytes*/
	/* also used by the data receiver, which will be filled in gradually */
	char *data;

	//bt_peer_t *provider;
};

struct job_s {
    char output_file_name[128];
    int chunk_num;
    struct chunk_s* chunks;
};

struct chunk_node_s{
	struct chunk_s* chunk;
	struct chunk_node_s* next;
};
 
struct job_s* malloc_new_job(char* chunkFile, char* output_file);
struct chunk_node_s* malloc_chunk_node(struct chunk_s* chunk);
void free_job(struct job_s* job);
void free_chunk_node(struct chunk_node_s* chunk_node);

/* load the data of the chunk specified by the hash, IN THE CHUNKS THIS NODE ALREADY HAVE 
 * return NULL, if this node do not have the chunk.
 */
struct chunk_s* load_chunk(uint8_t* hash_binary);

void print_job(struct job_s* job);
void print_chunk(struct chunk_s* chunk);

int get_not_found_chunk_number(struct job_s* job);
/* only add the chunk which is CHUNK_STATUS_NOT_FOUND */
struct network_packet_s* malloc_all_whohas_packet(int* num, struct job_s* job);
struct network_packet_s* malloc_ihave_packet(struct network_packet_s* whohas_packet);
struct network_packet_s* malloc_get_packet(struct chunk_s* chunk);
struct network_packet_s* malloc_data_packet(char* data, int data_size, int seq_num);
struct network_packet_s* malloc_ack_packet(int ack_num);

struct chunk_s* get_chunk(uint8_t* hash_binary);

struct chunk_s* seach_owned_chunk(uint8_t* hash_binary);

#endif /* _JOB_H_ */