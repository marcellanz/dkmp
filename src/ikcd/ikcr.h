/*
 * ikcr, inter kernel communication request header file
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcr.h,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * -----------------
 * $Id: ikcr.h,v 1.1 1999/12/16 22:21:31 lanzm Exp lanzm $ */


/*
 * function declaration
 */

int ikcr_proceed_outgoing_request(int sockfd, struct ikc_request* request, unsigned char thread_id);
int ikcr_respond_to_kernel(struct ikc_request* request);
int ikcr_queue_process_creation_request(struct sockaddr_in* cliaddr, socklen_t cli_len, char* packet);
int ikcr_respond_to_client(struct ikc_request* request);
int ikcr_handle_remote_syscall_request(int sockfd, struct msghdr* msg_prep);
int ikcr_proceed_incoming_request(struct ikc_request* request);
