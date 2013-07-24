/*  csm.c is part of dkm
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
 * csm, capability and system information modifier
 * File: $Source: /home/lanzm/data/projects/dkm/src/csm/RCS/csm.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: csm.c,v 0.2 1999/10/27 20:05:28 lanzm Exp $";

#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "../include/cs.h"

void csd_cli(FILE* fp, int sockfd, const struct sockaddr *pservaddr, socklen_t servlen)
{
 int n, cs_type, cs_req_type;
 char resp_type, resp_err;
 char* resp_data;
 char* c;
 char fromuser[MAXLINE], recvline[MAXLINE];
 char key[MAXLINE], value[MAXLINE], type[MAXLINE];
 char packet[MAXLINE];

 fputs("\ncsm> ", stdout);
 while(fgets(fromuser, MAXLINE, fp) != NULL) { 
 	c = &fromuser;
	while(*c && *c != '\n')
		c++; /* find last char in sendline */
	*c = '\0';
	sscanf(fromuser, "%s %s %[^\n]s", &type, &key, &value);
 	cs_type = CS_NOP;
	if(strcmp(type, "set") == 0) cs_type = CS_SET;
	else if(strcmp(type, "get") == 0) cs_type = CS_GET;
	else if(strcmp(type, "del") == 0) cs_type = CS_DEL;
	else if(strcmp(type, "ls") == 0) cs_type = CS_LS;
	else if(strcmp(type, "cap") == 0) cs_type = CS_CAPEXEC;
	else if(strcmp(type, "quit") == 0) return 0;


	if(cs_type != CS_NOP) {
		n = csp_pack_dbreq_packet(&packet, &key, &value, cs_type);
 		sendto(sockfd, packet, n, 0, pservaddr, servlen);
		n = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL);
		csp_unpack_resp_packet(recvline, &resp_type, &resp_err, &resp_data);
		if(resp_err == CS_RESP_SUCC) 
			fputs(resp_data, stdout);
		else
			fputs("error from csd\n", stdout);
		fputs("\ncsm> ", stdout);
	} /* if */
	else	
		printf("invalid action specified\n");
	memset(recvline, 0, MAXLINE);
	memset(packet, 0, MAXLINE);
  } /* while */ 

}


int main(int argc, char** argv)
{
 int sockfd;
 int csd_port;
 char* serv_ip;          /* IP or name as string */
 struct hostent* hptr;
 struct sockaddr_in servaddr;

 if( argc != 2) {
  perror("usage: csm <IP-Addr>\n");
  return 1;
 }
 if( (hptr = gethostbyname(argv[1])) == NULL ) {
	printf("gethostbyname error for %s\n", argv[1]);
 }
 serv_ip = hptr->h_addr_list[0];

 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_port = htons(7678);
 memcpy(&servaddr.sin_addr, serv_ip, sizeof(struct sockaddr_in));
 
 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 csd_cli(stdin, sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));

 return 0;
}
