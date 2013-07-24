#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ikc.h"
#include "ikcp.h"

int ikcro_proceed_outgoing_request(struct ikc_request* request)
{
 int sockfd;
 char* serv_ip;
 struct hostent* hptr;
 struct sockaddr_in servaddr;
 socklen_t servlen;

 char* packet;
 int packet_len;
 int n;
 unsigned short int seq;

 if( (hptr = gethostbyname(request->r_node)) == NULL ) {
        ikc_log_exit("ikcro_proceed_outgoing_request: gethostbyname error for %s\n", request->r_node);
 } /* if */
 serv_ip = hptr->h_addr_list[0];

 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_port = htons(IKC_PORT);
 memcpy(&servaddr.sin_addr, serv_ip, sizeof(struct sockaddr_in));

 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 if(sockfd < 0) {
	ikc_log_exit("ikcro_proceed_outgoing_request: unable to get a free socket\n");
 } /* if */
 servlen = sizeof(servaddr);

 switch(request->req) {
  case IKC_PROC_CREAT_REQ:

	 packet_len = ikcp_pack_proc_creat_req(&packet, seq, request->o_pid, request->o_gid, request->o_node, request->arg0, request->arg1);
	 n = sendto(sockfd, packet, packet_len, 0, (struct sockaddr*) &servaddr, servlen);
	 free(packet);
	 if(ikc_sock_read_timeout(sockfd, 5) == 0) {
	        ikc_log_err(IKC_LOG, "ikcro_proceed_outgoing_request: no response for process creation request\n");
	        return -1;
	 } /* if */
	 else {
	        n = recvfrom(sockfd, NULL, 0, MSG_PEEK, (struct sockaddr*) &servaddr, servlen);
	        ioctl(sockfd, 0x541B, &n);
	        packet = (char*) malloc(n);
	        n = recvfrom(sockfd, packet, n, 0, (struct sockaddr*) &servaddr, servlen);
	 } /* else */

	 if(n < 0) {
	        free(packet);
	        ikc_log_err(IKC_LOG, "ikcro_proceed_outgoing_request: receive of process creation response failed\n");
	        return -1;
	 } /* if */
	 ikcp_unpack_proc_creat_resp(packet, &seq, &request->r_pid, &request->r_gid);
#ifdef DEBUG
	ikc_log_err(IKC_LOG, "ikcro_proceed_outgoing_request: r_pid is: %d, r_gid is: %d\n", request->r_pid, request->r_gid);
#endif
	break;
 default:
	break;
 } /* switch */

 return 0;
}

int ikcro_respond_to_kernel(struct ikc_request* request)
{

 return 0;
}
