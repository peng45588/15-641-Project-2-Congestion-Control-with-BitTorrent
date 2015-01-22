#include "control.h"

void init_window_log(){
    provider_id = 1;
    gettimeofday(&start_time, NULL);
    window_log_file = fopen("problem2-peer.txt", "w");
}

void init_provider_connection(struct provider_connection_s* provider_connection){
	provider_connection->ssthresh = DEFAULT_SLOW_START_THRESHOLD;
	provider_connection->window_size = SLOW_START_INIT_WINDOW_SIZE;
	provider_connection->status = PROVIDER_STATUS_SLOW_START;

	gettimeofday(&provider_connection->CA_last_increase_time, NULL);
	provider_connection->RTT = DEFAULT_RTT_MS;

	provider_connection->RTT_ack_num = RTT_NOT_START;

	gettimeofday(&provider_connection->last_acked_received_time, NULL);
	gettimeofday(&provider_connection->last_data_send_time, NULL);
	provider_connection->num_duplicate_ack = 0;
}

void init_receiver_connection(struct receiver_connection_s* receiver_connection){
	gettimeofday(&receiver_connection->start_time, NULL);
	receiver_connection->status = RECEIVER_STATUS_PENGDING_DATA;
}

void set_RTT(int ack_num,struct provider_connection_s* provider_connection){
	if(provider_connection->RTT_ack_num == RTT_NOT_START){
		return;
	}

	if(provider_connection->RTT_ack_num != ack_num){
		return;
	}
	else{
		int new_RTT = get_time_diff(&provider_connection->RTT_data_sent_time);
		provider_connection->RTT = (provider_connection->RTT + new_RTT)/2;
		printf("new RTT %d, RTT now %d \n", new_RTT, provider_connection->RTT);
		provider_connection->RTT_ack_num = RTT_NOT_START;
	}
}


void provider_control_by_ack(struct provider_connection_s* provider_connection, 
		int ack_number){
	set_RTT(ack_number, provider_connection);
	printf("provider_control_by_ack %d\n", ack_number);
	printf("last_packet_acked %d  last_packet_sent %d\n", provider_connection -> last_packet_acked,
		provider_connection->last_packet_sent);
	printf("connection status: %d\n", provider_connection->status);
	/* this is unusual, we already get the ACK */
	if(ack_number < provider_connection->last_packet_acked){
		return;
	}
	else if(ack_number > provider_connection->last_packet_sent){
		return;
	}
	/* duplicate ACK, fast retransmit */
	if(ack_number == provider_connection->last_packet_acked){
		provider_connection->num_duplicate_ack = 1 + provider_connection->num_duplicate_ack;
		if(provider_connection->num_duplicate_ack >= DUPLICATE_ACK_NUM){
			/* a packet is lost */
			//printf("packet lose observed by duplicate ack\n");
			provider_connection->num_duplicate_ack = 0;
			connection_scale_down(provider_connection);
			reset_sender_buffer(provider_connection);
		}
	}
	/* ACK falls between the window */
	else{
		provider_connection->num_duplicate_ack = 1;
		provider_connection->last_packet_acked = ack_number;
		connection_scale_up(provider_connection);
	}
	gettimeofday(&provider_connection->last_acked_received_time, NULL);
} 

void provider_control_by_timeout(struct provider_connection_s* provider_connection){
	//printf("connection status: %d\n", provider_connection->status);
	if(get_time_diff(&provider_connection->last_acked_received_time)
			> PEER_CRASH_TIMEOUT){
		/* release the connection, the peer has creashed */
		printf("Peer %d has crashed, time out\n", provider_connection->receiver->id);
		finish_provider_connection(provider_connection);
		return;
	}
	if(get_time_diff(&provider_connection->last_acked_received_time)
			> ACK_TIMEOUT){
		if(get_time_diff(&provider_connection->last_data_send_time)
				> ACK_TIMEOUT){
			/* time out happened */
			//printf("ack time out\n");
			connection_scale_down(provider_connection);
			reset_sender_buffer(provider_connection);
		}
	}
}

void receiver_control_by_timeout(struct receiver_connection_s* receiver_connection){
	if(receiver_connection-> status == RECEIVER_STATUS_PENGDING_DATA){
		if(get_time_diff(&receiver_connection->start_time)
			> PEER_CRASH_TIMEOUT){
			/* remove this connection, change the status of the chunk */
			printf("Peer %d has crashed, time out\n", receiver_connection->provider->id);
			cancel_receiver_connection(receiver_connection);
		 	remove_receiver_connection(receiver_connection);
			return;
		}
		else if(get_time_diff(&receiver_connection->last_get_time)
			> GET_TIMEOUT){
			printf("GET time out with peer %d, resend\n", receiver_connection->provider->id);
			/* send another get */
			struct network_packet_s* get_packet = start_connection(receiver_connection);
			free(get_packet);
			return;
		}
	}
	else if(receiver_connection-> status == RECEIVER_STATUS_RECEIVED_DATA){
		if(get_time_diff(&receiver_connection->last_data_time)
			> PEER_CRASH_TIMEOUT){
			printf("Peer %d has crashed, time out\n", receiver_connection->provider->id);
			/* remove this connection, change the status of the chunk */
			cancel_receiver_connection(receiver_connection);
		 	remove_receiver_connection(receiver_connection);
			return;
		}
	}
}


/* reset the sender buffer, due to lose of packet */
void reset_sender_buffer(struct provider_connection_s* provider_connection){
	printf("reset_sender_buffer() \n");
	provider_connection->last_packet_sent = provider_connection->last_packet_acked;
}


void connection_scale_up(struct provider_connection_s* provider_connection){
	printf("connection_scale_up() \n");
	if(provider_connection->status == PROVIDER_STATUS_SLOW_START){
		slow_start_increase(provider_connection);
	}
	else if (provider_connection->status == PROVIDER_STATUS_CONGESTION_AVOIDANCE){
		congestion_control_increase(provider_connection);
	}
}

void connection_scale_down(struct provider_connection_s* provider_connection){
	printf("connection_scale_down() \n");
	if(provider_connection->status == PROVIDER_STATUS_SLOW_START){
		slow_start_decrease(provider_connection);
	}
	else if (provider_connection->status == PROVIDER_STATUS_CONGESTION_AVOIDANCE){
		congestion_control_decrease(provider_connection);
	}
}

void slow_start_increase(struct provider_connection_s* provider_connection){
	printf("slow_start_increase() \n");
	int new_window_size = 1 + provider_connection->window_size;
	change_window_size_to(provider_connection, new_window_size);
	change_connection_status(provider_connection);
}

void slow_start_decrease(struct provider_connection_s* provider_connection){
	printf("slow_start_decrease() \n");
	provider_connection->ssthresh = (provider_connection->window_size / 2) > 2 ?
			(provider_connection->window_size / 2) : 2;
	change_connection_status(provider_connection);
}

void congestion_control_increase(struct provider_connection_s* provider_connection){
	printf("congestion_control_increase() \n");
	if(get_time_diff(&provider_connection->CA_last_increase_time)
			> provider_connection->RTT){
		int new_window_size = 1 + provider_connection->window_size;
		change_window_size_to(provider_connection, new_window_size);
		gettimeofday(&provider_connection->CA_last_increase_time, NULL);
	}
}

void congestion_control_decrease(struct provider_connection_s* provider_connection){
	printf("congestion_control_decrease() \n");
	provider_connection->ssthresh = (provider_connection->window_size / 2) > 2 ?
			(provider_connection->window_size / 2) : 2;
	change_window_size_to(provider_connection, SLOW_START_INIT_WINDOW_SIZE);
	change_connection_status(provider_connection);
}


void change_connection_status(struct provider_connection_s* provider_connection){
	/* the change of window size may change the status of the connection */
	if(provider_connection->window_size >= provider_connection->ssthresh){
		provider_connection->status = PROVIDER_STATUS_CONGESTION_AVOIDANCE;
	}
	else{
		provider_connection->status = PROVIDER_STATUS_SLOW_START;
	}
}

void change_window_size_to(struct provider_connection_s* provider_connection, int new_size){
	provider_connection->window_size = new_size;
	//printf("ssthresh %d, new window size %d\n", provider_connection->ssthresh ,new_size);
	// fwrite("f", sizeof(char), 1, window_log_file);
	// fwrite(&provider_connection->id, sizeof(int), 1, window_log_file);
	// fwrite("\t", sizeof(char), 1, window_log_file);

	// int time_elapsed = get_time_diff(&start_time);
	// fwrite(&time_elapsed, sizeof(int), 1, window_log_file);
	// fwrite("\t", sizeof(char), 1, window_log_file);

	// fwrite(&provider_connection->window_size, sizeof(int), 1, window_log_file);
	fprintf(window_log_file, "f%d\t%d\t%d\n", 
		provider_connection->id,
		get_time_diff(&start_time),
		provider_connection->window_size
		);

	fflush(window_log_file);
}