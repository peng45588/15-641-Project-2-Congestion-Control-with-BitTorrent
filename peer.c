/*
 * peer.c
 *
 * Authors: Pengcheng Xu
 *
 */
#include "peer.h"

int main(int argc, char **argv) {

    bt_init(&config, argc, argv);

    DPRINTF(DEBUG_INIT, "peer.c main beginning\n");

#ifdef TESTING
    config.identity = 1; // your group number here
    strcpy(config.chunk_file, "chunkfile");
    strcpy(config.has_chunk_file, "haschunks");
#endif

    bt_parse_command_line(&config);

#ifdef DEBUG
    if (debug & DEBUG_INIT) {
        bt_dump_config(&config);
    }
#endif
    peer_run(&config);
    return 0;
}


void process_inbound_udp(int sock) {
    struct sockaddr_in from;
    socklen_t fromlen;
    char buf[MAX_PACKET_LENGTH];

    fromlen = sizeof(from);
    int read_result = spiffy_recvfrom(sock, buf, MAX_PACKET_LENGTH, 0, (struct sockaddr *) &from, &fromlen);
    printf("read %d bytes\n", read_result);
    //printf("incoming message from %s:%d\n", inet_ntoa(from.sin_addr), ntohs(from.sin_port));

    /* we just assume we got one packet everytime
     * if there are multiple packets in the buffer (whichi is rare), we just ignore them,
     * and everything will just work fine.
     */
    struct network_packet_s* network_packet = (struct network_packet_s*)buf;
    net_to_host_header(& network_packet->header);
    print_packet_header(&network_packet->header);
    if(validate_packet(& network_packet->header) < 0){
        printf("packet is invalid, skip\n");
        return;
    }
    int packet_type = get_packet_type(& network_packet->header);
    /* get the peer who send this packet*/
    bt_peer_t* peer = search_bt_peer(&config, (struct sockaddr *)&from);
    switch(packet_type) {
        case TYPE_WHOHAS: {
            struct network_packet_s* ihave_packet = malloc_ihave_packet(network_packet);
            /* this node has no such chunk */
            if(ihave_packet == NULL){
                return;
            }
            else{
                //print_ihave_packet(ihave_packet);
                send_packet(ihave_packet, global_socket, (struct sockaddr *) &from);
                free(ihave_packet);
            }
            break;
        }
        case TYPE_IHAVE: {
            if(current_job == NULL){
                return;
            }
            /* receiving a new IHAVE packet */
            /* check whether there is alreay a connection with this peer */
            struct receiver_connection_s* existing_connection = 
                    search_receiver_connection(receiver_connection_head, peer);
            if(existing_connection != NULL){
            /* there is alreay a connection */
                add_ihave_to_receiver_connection(existing_connection, network_packet);
            }
            /* there is no such connection */
            else{
                /* check if the number of connection reached the maximum */
                if(get_receiver_connection_number(receiver_connection_head) >=
                        max_receiver_connection){
                    return;
                }
                /* add add the hash whose chunk is in a CHUNK_STATUS_NOT_FOUND status 
                 * to a new connection
                 */
                else{
                    struct receiver_connection_s*  new_connection = 
                        malloc_receiver_connection(network_packet, peer);
                    /* if every chunk has a provider, the new_connection is NULL */
                    if(new_connection != NULL){
                        add_receiver_connection(new_connection);
                        /* start the new connection */
                        struct network_packet_s* get_packet = start_connection(new_connection);
                        //print_get_packet(get_packet);
                        free(get_packet);
                    }   
                }
            }
            //print_receiver_connection_list(receiver_connection_head);
            break;
        }
        case TYPE_GET: {
            //print_get_packet(network_packet);
            /* check whether there is alreay a connection with this peer */
            if(search_provider_connection(provider_connection_head, peer) != NULL){
                printf("Provider connection already exists with peer %d , ignore GET\n", peer->id);
                return;
            }
            /* do nothing is the number of connection reached the maximum*/
            if(get_provider_connection_number(provider_connection_head) >=
                    max_provider_connection){
                printf("Provider connection reached maximum with peer %d \n", peer->id);
                return;
            }
            struct provider_connection_s* new_connection = 
                    malloc_provider_connection(network_packet, peer);
            if(new_connection != NULL){
                printf("Add new provider connection with peer %d \n", peer->id);
                add_provider_connection(new_connection);
                //print_provider_connection(new_connection);
            }
            //print_provider_connection_list(provider_connection_head);
            break;
        }
        case TYPE_DATA: {
            print_data_packet(network_packet);
            printf("received data packet, seq: %d, ack: %d\n ",
                network_packet->header.seq_number,
                network_packet->header.ack_number);

            /* check whether there is a connection with this peer */
            struct receiver_connection_s* receiver_connection = 
                    search_receiver_connection(receiver_connection_head, peer);
            /* connection does not exist, ignore the data packet */
            if(receiver_connection == NULL){
                return;
            }
            int existing_seq_num = receiver_connection->chunk_list_head->chunk->received_seq_num;
            printf("expected data packet, seq: %d \n", 1 + existing_seq_num);
            int packet_seq_num = network_packet->header.seq_number;
            /* sequence number is illegal */
            if(packet_seq_num < 0){
                return;
            }
            /* old packet arrived, do nothing */
            else if(packet_seq_num  < (existing_seq_num + 1)){
                return;
            } 
            /* latter packet arrived first, send duplicate ACK */
            else if(packet_seq_num  > (existing_seq_num + 1)){
                struct network_packet_s* ack_packet = malloc_ack_packet(existing_seq_num);
                send_packet(ack_packet, global_socket, (struct sockaddr *) &from);
                free(ack_packet);
                return;
            }
            /* the packet expected */
            else{
                gettimeofday(&receiver_connection->last_data_time, NULL);
                receiver_connection->status = RECEIVER_STATUS_RECEIVED_DATA;

                struct network_packet_s* ack_packet = malloc_ack_packet(1 + existing_seq_num);
                send_packet(ack_packet, global_socket, (struct sockaddr *) &from);
                free(ack_packet);
                
                /* save_data_packet */
                save_data_packet(network_packet, receiver_connection);
                /* save the downloading chunk of the connnection, if it finihsed */
                save_chunk(receiver_connection);
                /* continue to download next chunk, if finished first one */
                reset_receiver_connection(receiver_connection);
            } 
            break;
        }
        case TYPE_ACK: {
            //print_ack_packet(network_packet);
            struct provider_connection_s* provider_connection = 
                    search_provider_connection(provider_connection_head, peer);
            if(provider_connection == NULL){
                return;
            }
            provider_control_by_ack(provider_connection, 
                    network_packet->header.ack_number);
            /* check if the data has been all send to a receiver */
            if(provider_connection->last_packet_acked == CHUNK_DATA_NUM){
                /* download finished */
                finish_provider_connection(provider_connection);
            }
            break;
        }
        case TYPE_DENIED: {
            break;
        }
        default:{
            break;
        }
    }
}

void process_get(char *chunkfile, char *outputfile) {
    /* initilize the job */
    struct job_s* new_job = malloc_new_job(chunkfile, outputfile);
    if(new_job == NULL){
      /* failed to create a new job*/
      return;
    }
    current_job = new_job;
    print_job(current_job);

    flood_whohas();
}

void handle_user_input(char *line, void *cbdata) {
    char chunkf[128], outf[128];

    bzero(chunkf, sizeof(chunkf));
    bzero(outf, sizeof(outf));

    if (sscanf(line, "GET %120s %120s", chunkf, outf)) {
        if (strlen(outf) > 0) {
            process_get(chunkf, outf);
        }
    }
}

void init_hasChunks(char* has_chunk_file_name) {
    FILE* has_chunk_file = fopen(has_chunk_file_name,"r");

    int chunk_num = 0;
    char read_buffer[CHUNK_FILE_LINE_BUFFER_SIZE];
    while (fgets(read_buffer, CHUNK_FILE_LINE_BUFFER_SIZE, has_chunk_file)) {
        chunk_num++;
    }
    
    /* set the global variable*/
    has_chunks_number = chunk_num;
    has_chunks = malloc(sizeof(struct chunk_s) * chunk_num);

    memset(read_buffer, 0, CHUNK_FILE_LINE_BUFFER_SIZE);
    fseek(has_chunk_file, 0, SEEK_SET);

    int count = 0;
    char hash_buffer[SHA1_HASH_SIZE*2];
    while (fgets(read_buffer, CHUNK_FILE_LINE_BUFFER_SIZE, has_chunk_file)) {
        sscanf(read_buffer,"%d %s", &(has_chunks[count].id), hash_buffer);
        hex2binary(hash_buffer, SHA1_HASH_SIZE * 2, has_chunks[count].hash_binary);
        has_chunks[count].data = NULL;
        count++;
    } 
    fclose(has_chunk_file);

    printf("********CHUNKS I HAVE**********\n");
    int i = 0;
    for(i = 0; i < chunk_num; i++){
        print_chunk(&has_chunks[i]);
    }
    printf("*******END CHUNKS I HAVE********\n");
}

/* for every connection, send out packet */
void all_provider_connection_send_data(
        struct provider_connection_s* connection_head){
    struct provider_connection_s* temp = connection_head;
    while(temp != NULL){
        send_data_packet(temp);
        temp = temp -> next;
    }
}

void finish_job(){
    if(current_job == NULL){
        return;
    }
    if(get_receiver_connection_number(receiver_connection_head) != 0){
        return;
    }
    if(get_not_found_chunk_number(current_job) != 0){
        return;
    }
    printf("GOT %s\n", current_job->output_file_name);

    free_job(current_job);
    current_job = NULL;
}

/* flood WHOHAS packet for chunk in CHUNK_STATUS_NOT_FOUND state 
 * if there are spare receiver connection
 */
void flood_whohas(){
    if(current_job == NULL){
        return;
    }
    if(get_receiver_connection_number(receiver_connection_head) 
            >= max_receiver_connection){
        return;
    }

     /* create all the WHOHAS packet*/
    int whohas_packet_num = 0;
    struct network_packet_s* whohas_packets
        = malloc_all_whohas_packet(&whohas_packet_num, current_job);

    if(whohas_packet_num == 0){
        return;
    }

    /* send out all the WHOHAS packet to everybody */
    char str[20];
    struct bt_peer_s* peer = config.peers;

    while(peer != NULL) {
        inet_ntop(AF_INET, &(peer->addr.sin_addr), str, INET_ADDRSTRLEN);

        /* send to everyone, except itself */
        if (peer->id != config.identity) {
            printf("send all whohas packect CHUNK_STATUS_NOT_FOUND to peer <ID:%d> <ip:%s> <port:%d>\n",
                peer->id, str, ntohs(peer->addr.sin_port));
            send_all_whohas_packet(whohas_packets, whohas_packet_num, 
                global_socket, (struct sockaddr *) &peer->addr);
        }
        peer = peer->next;
    }

    int i = 0;
    for(i = 0; i < whohas_packet_num; i++){
      print_whohas_packet(&whohas_packets[i]);
    }
    printf("fload whohas packet\n");
    free(whohas_packets);

}

void providers_timeout(){
    struct provider_connection_s* temp = provider_connection_head;
    while(temp != NULL){
        provider_control_by_timeout(temp);
        temp = temp -> next;
    }
}

void receivers_timeout(){
    struct receiver_connection_s* temp = receiver_connection_head;
    while(temp != NULL){
        receiver_control_by_timeout(temp);
        temp = temp -> next;
    }
}


void peer_run(bt_config_t *config) {
    int sock;
    struct sockaddr_in myaddr;
    fd_set readfds;
    struct user_iobuf *userbuf;
    
    if ((userbuf = create_userbuf()) == NULL) {
        perror("peer_run could not allocate userbuf");
        exit(-1);
    }
    
    if ((sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
        perror("peer_run could not create socket");
        exit(-1);
    }
    
    bzero(&myaddr, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    //myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    inet_aton("127.0.0.1", (struct in_addr*)&myaddr.sin_addr.s_addr);
    myaddr.sin_port = htons(config->myport);
    
    if (bind(sock, (struct sockaddr *) &myaddr, sizeof(myaddr)) == -1) {
        perror("peer_run could not bind socket");
        exit(-1);
    }
    global_socket = sock;
    spiffy_init(config->identity, (struct sockaddr *)&myaddr, sizeof(myaddr));
    init_window_log();
    init_hasChunks(config->has_chunk_file);
    init_connections();

    struct timeval last_flood_whohas_time;
    gettimeofday(&last_flood_whohas_time, NULL);
    while (1) {
        int nfds;
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sock, &readfds);

        struct timeval select_timeout;
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 300000;

        nfds = select(sock+1, &readfds, NULL, NULL, &select_timeout);
        
        if (nfds > 0) {
            if (FD_ISSET(sock, &readfds)) {
    	       process_inbound_udp(sock);
            }
          
            if(FD_ISSET(STDIN_FILENO, &readfds)) {
    	        process_user_input(STDIN_FILENO, userbuf, handle_user_input,
    		           "Currently unused");
            }
        }
        providers_timeout();
        receivers_timeout();
        /* this is a regular send task, driven purely by time */
        all_provider_connection_send_data(provider_connection_head);

        /* every few seconds, if there are receiver connection available, flooding whohas*/
        if(get_time_diff(&last_flood_whohas_time) > WHOHAS_FLOOD_INTERVAL_MS){
            flood_whohas();
            gettimeofday(&last_flood_whohas_time, NULL);
        }
        /* if there is no more receiver connection and the job is not NULL
         * check to see if the job has finished
         */
        finish_job();
    }
}

