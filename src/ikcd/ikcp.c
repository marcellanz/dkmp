/*  ikcp.c is part of dkm
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
 * ikcp, inter kernel communication protocol
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcp.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: ikcp.c,v 1.1 1999/12/16 22:11:40 lanzm Exp lanzm $";

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "../include/ikc.h"

/*
 * determine the type of received packet
 */
int ikcp_det_packet_type(char* packet, char* version, char* type)
{
 if(packet == NULL) {
	return -1;
 } /* if */

 *version = packet[0];
 *type = packet[1];

 return 0;
}

/*
 * pack a process creation req
 */
int ikcp_pack_proc_creat_req(char** packet, unsigned short int seq, pid_t o_pid, char* o_hostname, char* o_filename, char* o_argv)
{
 char* send_packet;
 char* packet_data;
 char ikc_version, ikc_type;
 unsigned short int packet_len, ikc_version_len, ikc_type_len, seq_len, o_pid_len, o_hostname_len, o_filename_len, o_argv_len, string_len_len;

 ikc_version_len = sizeof(char);
 ikc_type_len = sizeof(char);
 seq_len = sizeof(unsigned short int);
 o_pid_len = sizeof(pid_t);
 o_hostname_len = strlen(o_hostname);
 o_filename_len = strlen(o_filename);
 o_argv_len = strlen(o_argv);
 string_len_len = sizeof(unsigned short int);
 
 packet_len = ikc_version_len+ikc_type_len+seq_len+o_pid_len+o_hostname_len+o_filename_len+o_argv_len+3*string_len_len;
 send_packet = (char*) malloc(packet_len); 

 send_packet[0] = IKC_VERSION;
 send_packet[1] = IKC_PROC_CREAT_REQ; 
 
 packet_data = send_packet+ikc_version_len+ikc_type_len;
 memcpy(packet_data, &seq, seq_len);
 packet_data += seq_len;

 memcpy(packet_data, &o_pid, o_pid_len);
 packet_data+= o_pid_len;

 memcpy(packet_data, &o_hostname_len, string_len_len);
 packet_data += string_len_len;
 
 memcpy(packet_data, o_hostname, o_hostname_len);
 packet_data += o_hostname_len;

 memcpy(packet_data, &o_filename_len, string_len_len);
 packet_data += string_len_len;

 memcpy(packet_data, o_filename, o_filename_len);
 packet_data += o_filename_len;

 memcpy(packet_data, &o_argv_len, string_len_len);
 packet_data += string_len_len;

 memcpy(packet_data, o_argv, o_argv_len);
 packet_data += o_argv_len-1;

 *packet = send_packet;

 return packet_len; 
}


/*
 * unpack a process creation request
 */
int ikcp_unpack_proc_creat_req(char* packet, unsigned short int* seq, pid_t* o_pid, char* o_hostname, char** o_filename, char** o_argv)
{
 char* packet_data;
 char* filename;
 char* argv;
 unsigned short int string_len;

 packet_data = packet+IKC_PACKET_HDR_LEN; /* skip version and type */

 memcpy(seq, packet_data, sizeof(unsigned short int));
 packet_data += sizeof(unsigned short int);

 memcpy(o_pid, packet_data, sizeof(pid_t));
 packet_data += sizeof(pid_t);

 memcpy(&string_len, packet_data, sizeof(unsigned short int));
 packet_data += sizeof(unsigned short int);
 memcpy(o_hostname, packet_data, string_len);
 o_hostname[string_len] = '\0';
 packet_data += string_len;
 
 memcpy(&string_len, packet_data, sizeof(unsigned short int));
 filename = (char*) malloc(string_len+1);
 packet_data += sizeof(unsigned short int);
 memcpy(filename, packet_data, string_len);
 *(filename+string_len) = '\0';
 packet_data += string_len;

 memcpy(&string_len, packet_data, sizeof(unsigned short int));
 argv = (char*) malloc(string_len+1);
 packet_data += sizeof(unsigned short int);
 memcpy(argv, packet_data, string_len);
 *(argv+string_len) = '\0';

 *o_filename = filename;
 *o_argv = argv;

 return 0;
}

/*
 * pack a process creation response packet
 */
int ikcp_pack_proc_creat_resp(char** packet, unsigned short int seq, pid_t r_pid)
{
 char* send_packet;
 char* packet_data;
 char ikc_version, ikc_type;
 unsigned short int packet_len;

 packet_len = IKC_PACKET_HDR_LEN+sizeof(unsigned short int)+sizeof(pid_t);
 send_packet = (char*) malloc(packet_len);

 send_packet[0] = IKC_VERSION;
 send_packet[1] = IKC_PROC_CREAT_RESP;

 packet_data = send_packet+IKC_PACKET_HDR_LEN;
 memcpy(packet_data, &seq, sizeof(unsigned short int));
 packet_data += sizeof(unsigned short int);

 memcpy(packet_data, &r_pid, sizeof(pid_t));
 packet_data+= sizeof(pid_t);

 *packet = send_packet;

 return packet_len;
}


/*
 * unpack a process creation response packet
 */
int ikcp_unpack_proc_creat_resp(char* packet, unsigned short int* seq, pid_t* r_pid)
{
 char* packet_data;

 packet_data = packet+IKC_PACKET_HDR_LEN; /* skip version and type */

 memcpy(seq, packet_data, sizeof(unsigned short int));
 packet_data += sizeof(unsigned short int);

 memcpy(r_pid, packet_data, sizeof(pid_t));
 packet_data += sizeof(pid_t);

 return 0;
}


/*
 * timeout helper function for recv_from calls
 */
int ikc_sock_read_timeout(int fd, int sec)
{
 fd_set rset;
 struct timeval tv;

 FD_ZERO(&rset);
 FD_SET(fd, &rset);

 tv.tv_sec = sec;
 tv.tv_usec = 0;

 return (select(fd+1, &rset, NULL, NULL, &tv));
}
