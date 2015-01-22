#ifndef _CONTROL_H_
#define _CONTROL_H_

#include "lib.h"

//#define DEFAULT_WINDOW_SIZE 8  
#define WHOHAS_FLOOD_INTERVAL_MS 3000 

#define DEFAULT_RTT_MS 2000;

#define PEER_CRASH_TIMEOUT 10000
#define GET_TIMEOUT 4000
#define ACK_TIMEOUT 500

#define SLOW_START_INIT_WINDOW_SIZE 1
#define DEFAULT_SLOW_START_THRESHOLD 64

#define DUPLICATE_ACK_NUM 3

/* log to record window size */
int provider_id;
struct timeval start_time;
//char* window_log_file_name ="problem2-peer.txt";
FILE* window_log_file;
//char* provider_name_prefix = "f";


void init_window_log();

void init_provider_connection(struct provider_connection_s* provider_connection);

void init_receiver_connection(struct receiver_connection_s* receiver_connection);

void set_RTT(int ack_num,struct provider_connection_s* provider_connection);

void provider_control_by_ack(struct provider_connection_s* provider_connection, 
		int ack_number);

void provider_control_by_timeout(struct provider_connection_s* provider_connection);

void receiver_control_by_timeout(struct receiver_connection_s* receiver_connection);

/* reset the sender buffer, due to lose of packet */
void reset_sender_buffer(struct provider_connection_s* provider_connection);


void connection_scale_up(struct provider_connection_s* provider_connection);

void connection_scale_down(struct provider_connection_s* provider_connection);

void slow_start_increase(struct provider_connection_s* provider_connection);

void slow_start_decrease(struct provider_connection_s* provider_connection);

void congestion_control_increase(struct provider_connection_s* provider_connection);

void congestion_control_decrease(struct provider_connection_s* provider_connection);

void change_connection_status(struct provider_connection_s* provider_connection);

void change_window_size_to(struct provider_connection_s* provider_connection, int new_size);

#endif /* _CONTROL_H_ */