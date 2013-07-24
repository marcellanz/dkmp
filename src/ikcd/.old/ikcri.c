/*
 * ikcri, inter kernel communication incoming request handling
 * File: $Source$
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id$";


#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/uio.h>

#include "ikc.h"
#include "ikcl.h"
#include "ikcq.h"

extern struct ikc_queue_head ikc_qh_pending_in;
extern pthread_mutex_t ikc_in_cond_mutex;
extern pthread_cond_t ikc_in_cond;

/*
 * create a request struct and queue the request on the pending_in queue
 */
int ikcri_queue_process_creation_request(struct sockaddr* cliaddr, socklen_t cli_len, char* packet)
{
 struct ikc_request* request;

 request = (struct ikc_request*) malloc(sizeof(struct ikc_request));
 if(request == NULL) {
	ikc_log_exit(IKC_LOG, "ikcri_queue_process_creation_request: unable to malloc\n");
 } /* if */
 /*ikcp_unpack_proc_creat_req(packet, &seq, &o_pid, &o_gid, &o_nodename, &o_filename, &o_argv);*/

 /*
  * args: request->arg<n>
  *				arg0 is o_filename;
  *				arg1 is o_argv;
  */
 ikcp_unpack_proc_creat_req(packet, &request->seq, &request->o_pid, &request->o_gid, &request->o_node, &request->arg0, &request->arg1);
 /*
  * fill up the request structure
  */
 request->cliaddr = (struct sockaddr*) malloc(cli_len);
 if(request->cliaddr == NULL) {
	ikc_log_exit(IKC_LOG, "ikcri_queue_process_creation_request: unable to malloc\n");
 } /* if */
 memcpy(request->cliaddr, cliaddr, cli_len);
 request->cli_len = cli_len;
 request->req = IKC_PROC_CREAT_REQ;

 ikcq_queue_request(&ikc_qh_pending_in, request);
 pthread_mutex_lock(&ikc_in_cond_mutex);
 /*
  * inform an another working thread to process a queued pending request
  */
 pthread_cond_signal(&ikc_in_cond);
 pthread_mutex_unlock(&ikc_in_cond_mutex);

 return 0;
}

int ikcri_respond_to_client(struct ikc_request* request)
{
 int sockfd;
 int packet_len;
 char* packet;

 switch(request->req) {
  case IKC_PROC_CREAT_REQ:	packet_len = ikcp_pack_proc_creat_resp(&packet, request->seq, request->r_pid, request->r_gid);
				break;
  default:			ikc_log_exit(IKC_LOG, "ikcri_response_to_client: unknown request type\n");
 } /* switch */
 if(packet_len > 0) {
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	sendto(sockfd, packet, packet_len, 0, request->cliaddr, request->cli_len);
	close(sockfd);
 } /* if */

 return 0;
}

/*
 * handle an incoming systemcall request
 */
int ikcri_handle_remote_syscall_request(int sockfd, struct msghdr* msg_prep)
{
 return 0;
}

int ikcri_proceed_incoming_request(struct ikc_request* request)
{
 request->r_pid = 76; 
 request->r_gid = 78; 
#ifdef DEBUG
 ikc_log_err(IKC_LOG, "ikcri_proceed_incoming_request: begin\no_node is: %s\no_pid is: %d\nreq is: %d\nikcri_proceed_incoming_request: end\n", request->o_node, request->o_pid, request->req);
#endif DEBUG

 return 0;
}
