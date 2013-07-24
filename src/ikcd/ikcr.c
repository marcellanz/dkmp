/*  ikcr.c is part of dkm
    Copyright (C) 2000  Marcel Lanz <marcel.lanz@ds9.ch>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 * ikcr, inter kernel communication request functions
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcr.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: ikcr.c,v 1.1 1999/12/16 22:15:28 lanzm Exp lanzm $";

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <errno.h>

#include "../csd/csda.h"
#include "../include/cs.h"
#include "../include/ikc.h"
#include "ikcl.h"
#include "ikcq.h"
#include "ikcp.h"

extern struct ikc_queue_head ikc_qh_pending_in;
extern pthread_mutex_t ikc_in_cond_mutex;
extern pthread_cond_t ikc_in_cond;

/*
 * pack the request number
 */
unsigned int ikcr_pack_request_number(struct sockaddr_in* cliaddr, unsigned short int request_id)
{
 unsigned int request_number;

 request_number = 0;
 request_number = (ntohl(cliaddr->sin_addr.s_addr)<<16)|request_id;

#ifdef DEBUG
 ikc_log_err(IKC_LOG, "ikcr_pack_request_number: request_number is: %x\n", request_number);
#endif

 return request_number;
}


/*
 * pack the request id
 */
unsigned short int ikcr_pack_request_id(unsigned char thread_id, unsigned short int seq_number)
{
 unsigned short int request_id;

 request_id = 0;
 request_id = (thread_id<<10)|(0x03ff & seq_number);

#ifdef DEBUG
 ikc_log_err(IKC_LOG, "ikcr_pack_request_id: request_id is: %u\n", request_id); 
#endif

 return request_id;
}


/*
 * unpack request id from request number
 */
unsigned short int ikcr_unpack_req_id(unsigned int request_nbr)
{
 unsigned short int request_id;

 request_id = 0;
 request_id = (0x0000ffff & request_nbr);

 return request_id;
}


/*
 * unpack host ID
 */
unsigned short int ikcr_unpack_host_id(unsigned int request_nbr)
{
 unsigned short int host_id;
 
 host_id = 0;
 host_id = request_nbr>>16;
 
 return host_id;
}


/*
 * unpack thread ID
 */
unsigned char ikcr_unpack_thread_id(unsigned int request_nbr)
{
 unsigned char thread_id;

 thread_id = 0;
 thread_id = (request_nbr>>10) & 0x0000003f;

 return thread_id;
}


/*
 * unpack sequence number
 */
unsigned short int ikcr_unpack_seq_number(unsigned int request_nbr)
{
 unsigned short int seq_number;

 seq_number = 0;
 seq_number = (request_nbr & 0x000003ff);

 return seq_number;
}


/*
 * proceed an outgoing request
 */
int ikcr_proceed_outgoing_request(int sockfd, struct ikc_request* request, unsigned char thread_id)
{
 char* serv_ip;
 struct hostent* hptr;
 struct sockaddr_in servaddr;
 socklen_t servlen;

 char* packet;
 int packet_len;
 int n;

 if( (hptr = gethostbyname(request->r_node)) == NULL ) {
        ikc_log_exit(IKC_LOG, "ikcr_proceed_outgoing_request: gethostbyname error for %s\n", request->r_node);
 } /* if */
 serv_ip = hptr->h_addr_list[0];

 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_port = htons(IKC_PORT);
 memcpy(&servaddr.sin_addr, serv_ip, sizeof(struct sockaddr_in));

 servlen = sizeof(servaddr);

 switch(request->req) {
  case IKC_PROC_CREAT_REQ:
		request->rq_argv.arg1 = "";
	 packet_len = ikcp_pack_proc_creat_req(&packet, 
                                               ikcr_pack_request_id(thread_id, request->seq_nbr),
                                               request->o_pid, 
                                               request->o_node, 
                                               request->rq_argv.arg0, 
                                               request->rq_argv.arg1);
	 n = sendto(sockfd, packet, packet_len, 0, (struct sockaddr*) &servaddr, servlen);
	 free(packet);
	 if(ikc_sock_read_timeout(sockfd, 5) == 0) {
	        ikc_log_err(IKC_LOG, "ikcr_proceed_outgoing_request: no response for process creation request\n");
	        return -1;
	 } /* if */
	 else {
	        ioctl(sockfd, FIONREAD, &n);
	        packet = (char*) malloc(n);
	        n = recvfrom(sockfd, packet, n, 0, (struct sockaddr*) &servaddr, &servlen);
	 } /* else */

	 if(n < 0) {
	        free(packet);
	        ikc_log_err(IKC_LOG, "ikcr_proceed_outgoing_request: receive of process creation response failed\n");
	        return -1;
	 } /* if */
	 ikcp_unpack_proc_creat_resp(packet, &request->seq_nbr, &request->r_pid);
#ifdef DEBUG
	 ikc_log_err(IKC_LOG, "ikcr_proceed_outgoing_request: r_pid is: %d\n", request->r_pid);
#endif
	break;
 default:
	break;
 } /* switch */

 return 0;
}

/*
 * create a request struct and queue the request on the pending_in queue
 */
int ikcr_queue_process_creation_request(struct sockaddr_in* cliaddr, socklen_t cli_len, char* packet)
{
 struct ikc_request* request;
 unsigned short int request_id;

 request = (struct ikc_request*) malloc(sizeof(struct ikc_request));
 if(request == NULL) {
	ikc_log_exit(IKC_LOG, "ikcr_queue_process_creation_request: unable to malloc\n");
 } /* if */
 /*ikcp_unpack_proc_creat_req(packet, &seq, &o_pid, &o_nodename, &o_filename, &o_argv);*/

 /*
  * args: request->rq_argv.arg<n>
  *				arg0 is o_filename;
  *				arg1 is o_argv;
  */
 ikcp_unpack_proc_creat_req(packet, &request_id, &request->o_pid, request->o_node, (char**) &request->rq_argv.arg0, (char**) &request->rq_argv.arg1);
 /*
  * fill up the request structure
  */
 request->cliaddr = (struct sockaddr_in*) malloc(cli_len);
 if(request->cliaddr == NULL) {
	ikc_log_exit(IKC_LOG, "ikcr_queue_process_creation_request: unable to malloc\n");
 } /* if */
 memcpy(request->cliaddr, cliaddr, cli_len);
 request->cli_len = cli_len;
 request->req = IKC_PROC_CREAT_REQ;
 request->request_nbr = ikcr_pack_request_number(request->cliaddr, request_id);
 request->host_id = ikcr_unpack_host_id(request->request_nbr);
 request->thread_id = ikcr_unpack_thread_id(request->request_nbr);
 request->seq_nbr = ikcr_unpack_seq_number(request->request_nbr);
#ifdef DEBUG
 ikc_log_err(IKC_LOG, "ikcr_queue_process_creation_request: request_number is: %x\n", request->request_nbr);
 ikc_log_err(IKC_LOG, "ikcr_queue_process_creation_request: host_id is: %x\n", request->host_id);
 ikc_log_err(IKC_LOG, "ikcr_queue_process_creation_request: request_id is: %u\n", ikcr_unpack_req_id(request->request_nbr));
 ikc_log_err(IKC_LOG, "ikcr_queue_process_creation_request: thread_id is: %x\n", request->thread_id);
 ikc_log_err(IKC_LOG, "ikcr_queue_process_creation_request: seq_number is: %u\n", request->seq_nbr);
#endif

 ikcq_queue_request(&ikc_qh_pending_in, request);
 pthread_mutex_lock(&ikc_in_cond_mutex);
 /*
  * inform an another working thread to process a queued pending request
  */
 pthread_cond_signal(&ikc_in_cond);
 pthread_mutex_unlock(&ikc_in_cond_mutex);

 return 0;
}


/*
 * send the response back to the client
 */
int ikcr_respond_to_client(struct ikc_request* request)
{
 int sockfd;
 int packet_len;
 char* packet;

 switch(request->req) {
  case IKC_PROC_CREAT_REQ:	packet_len = ikcp_pack_proc_creat_resp(&packet, ikcr_unpack_req_id(request->request_nbr), request->r_pid);
				break;
  default:			ikc_log_exit(IKC_LOG, "ikcr_response_to_client: unknown request type\n");
 } /* switch */
 if(packet_len > 0) {
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	sendto(sockfd, packet, packet_len, 0, (struct sockaddr*)request->cliaddr, request->cli_len);
	close(sockfd);
 } /* if */

 return 0;
}


/*
 * handle an incoming systemcall request, not implemented yet
 */
int ikcr_handle_remote_syscall_request(int sockfd, struct msghdr* msg_prep)
{
 return 0;
}


/*
 * proceed an incoming request
 * there ist only a demo-implementation of IKC_PROC_CREAT_REQ
 */
int ikcr_proceed_incoming_request(struct ikc_request* request)
{
 switch(request->req) {
  case IKC_PROC_CREAT_REQ: 	request->r_pid = 76; 
#ifdef DEBUG
 ikc_log_err(IKC_LOG, "ikcr_proceed_incoming_request: begin\no_node is: %s\no_pid is: %d\nreq is: %d\nikcr_proceed_incoming_request: end\n", request->o_node, request->o_pid, request->req);
#endif DEBUG
				break;
  default:			break;
 } /* switch */

 return 0;
}


/*
 * helper function for the message-queue communication
 */
void ikc_msq_alarm(int sig)
{
 return;
}

/*
 * csd <-> ikcd communication
 * this communication is for the MAP_REQ: csd has the best node for an application name
 * arg: the application name, best_node buffer
 * ret: 0 is ok, -1 is error
 */
int ikcr_best_node_for_app(char* appname, char** best_node)
{
 int error;
 int client_id, server_id;	/* server and client msq id's */
 char* best_node_buf;
 char* this_node;
 struct bn_request req;		/* best node request */
 struct bn_response resp;	/* best node response */

 error = 0;
 server_id = msgget(7768, 0);
 if(server_id < 0) {
	ikc_log_err(IKC_LOG, "ikcr_best_node_for_app: unable to get the server message queue\n");
	error = -1;
	goto out;
 } /* if */
 client_id = msgget(6877, S_IRWXU | S_IWGRP | S_IWOTH | IPC_CREAT);
 if(client_id < 0) {
	ikc_log_err(IKC_LOG, "ikcr_best_node_for_app: unable to create a message queue\n");
	error = -1;
	goto out;
 } /* if */

 req.mtype = 1;
 sprintf(req.msg, "%d %s", client_id, appname);
 if(msgsnd(server_id, &req, MSQ_MAX_NAME, 0)) {
	ikc_log_err(IKC_LOG, "ikcr_best_node_for_app: unable to send a message\n");
	error = -1;
	goto out;
 } /* if */
 signal(SIGALRM, ikc_msq_alarm);
 alarm(IKC_MSQ_ALARM);
 if(msgrcv(client_id, &resp, MSQ_MAX_NODE, 0, 0) < 0) {
	ikc_log_err(IKC_LOG, "ikcr_best_node_for_app: unable to receive a message\n");
	error = -1;
	goto out;
 } /* if */
 alarm(0);
#ifdef DEBUG
 ikc_log_err(IKC_LOG, "best node for app: /bin/sh is: %s\n", resp.msg);
#endif

out:
 msgctl(client_id, IPC_RMID, NULL);
 if(error == -1) {
	this_node = getenv("HOSTNAME");
	best_node_buf = (char*) malloc(strlen(this_node)+1);
	strcpy(best_node_buf, this_node);
 } /* if */
 else {
	best_node_buf = (char*) malloc(strlen(resp.msg)+1);
	strcpy(best_node_buf, resp.msg);
 } /* else */
 *best_node = best_node_buf; 

 return error;
}
