#################################################################################
# README                                                                        #
#                                                                               #
# Description: This file serves as a README and documentation for 15441 Project2#
#                                                                               #
# Authors: Pengcheng Xu <pengchexu@gmail.com>                                   #
#                                                                               #
#################################################################################

This project implements a P2P file transfer protocol and the application. The 
function implemented including:
	A mechanism to discovery file - the node use WHOHAS and IHAVA packet to
communicate about the data it has.
	A concurrent file download mechanism - file is splitted into 512KB chunks, and 
each chunk can be download from different nodes, and combine together to get the
final file.
	A Reliable transfer meachinsm and Sliding Window - the sender of the file will
check for ACK from the receiver for each DATA packet it send. Packet lost in the 
network will be retransmit.
	A Congestion Control mechanism - including Fast Retransmission, Slow Start,
Congestion Avoidance, the window size along with the time is output to a log file.

The following files makes for the core of this project.
peer.c
	which has a loop for the full function of this program. In this loop, it will
check for incoming data packet, the progress of the job and the timeout condition 
of different connections. It has a list of receiver connection and a list of 
provider connection. It also contains the configuration data of this program.

connection.c
	there are two kind of connection, the provider connection and the receiver
connection. The receiver connection is simpler, it has the status for receiving 
data, especially for timeout status. The provider connection has a status for the 
sender window and timeout status. Each receiver connection has a list of chunk which
is going to be download for that node, each chunk will be requested by resetting the
receiver connection and ask for a new provider connetion.

job.c
	a job represents a GET command for the user, it has all the chunks to be download,
and it will track the status of these trunks.

control.c
	This file contains all of the function to control the increase and decrease of 
the window size and the slow start threshold. It also has function to calculate the
RTT.

packet.c
	packet is the basic unit to be transfer in the network, this file has all the
function to create, free and transfer the packet.
	
lib.c 
	this is a helper file, which has function to manipulate time.
