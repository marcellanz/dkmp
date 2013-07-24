#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "ikc.h"
#include "ikcp.h"

int ikcm_cli(int sockfd, struct sockaddr* servaddr, socklen_t servlen)
{
 int error;
 int n;
 int packet_len;

 char* packet;
 unsigned short int seq;
 pid_t o_pid;
 gid_t o_gid;
 char o_hostname[65];
 char o_filename[256];
 char o_argv[256];

 pid_t r_pid;
 gid_t r_gid;

 seq = 88;
 o_pid = 294;
 o_gid = 911;
 strcpy(o_hostname, "orinoco");
 strcpy(o_filename, "/bin/bash");
 strcpy(o_argv, "");

 packet_len = ikcp_pack_proc_creat_req(&packet, seq, o_pid, o_gid, o_hostname, o_filename, o_argv);
 n = sendto(sockfd, packet, packet_len, 0, servaddr, servlen);
 free(packet);
 if(ikc_sock_read_timeout(sockfd, 5) == 0) {
	ikc_log_err(IKC_LOG, "ikcm_cli: no response for process creation request\n");
	return -1;
 } /* if */
 else {
	n = recvfrom(sockfd, NULL, 0, MSG_PEEK, servaddr, &servlen);
        ioctl(sockfd, 0x541B, &n);
        packet = (char*) malloc(n);
        n = recvfrom(sockfd, packet, n, 0, servaddr, &servlen);
 } /* else */

 if(n < 0) {
	free(packet);
	ikc_log_err(IKC_LOG, "ikcm_cli: receive of process creation response failed\n");
	return -1;
 } /* if */

 ikcp_unpack_proc_creat_resp(packet, &seq, &r_pid, &r_gid);

#ifdef DEBUG
 ikc_log_err(IKC_LOG, "ikcm_cli: r_pid is: %d, r_gid is: %d\n", r_pid, r_gid);
#endif

 return 0;
}

int main(unsigned int argc, char** argv)
{
 int sockfd;
 int csd_port;
 char* serv_ip;          /* IP or name as string */
 struct hostent* hptr;
 struct sockaddr_in servaddr;

 if( argc != 2) {
  ikc_log_exit("usage: ikcm <IP-Addr>\n");
 }
 if( (hptr = gethostbyname(argv[1])) == NULL ) {
        ikc_log_err("gethostbyname error for %s\n", argv[1]);
 }
 serv_ip = hptr->h_addr_list[0];

 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_port = htons(IKC_PORT);
 memcpy(&servaddr.sin_addr, serv_ip, sizeof(struct sockaddr_in));

 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 ikcm_cli(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));

 return 0;
}
