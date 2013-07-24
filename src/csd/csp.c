/*  csp.c is part of dkm
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
 * csp, capability and system information protocoll
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csp.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: csp.c,v 0.2 1999/11/05 17:26:58 lanzm Exp lanzm $";

#include "../include/cs.h"


/*
 * pack a hello-packet
 */
int csp_pack_hello_packet(char* hello_packet, char* hostname, char* loadavg1, char* loadavg5, char* loadavg15)
{
 char* hello_packet_data;
 char val_len;
 int packet_len;

 /* packet generation */
 packet_len = 0;
 hello_packet[0] = 1;
 hello_packet[1] = CS_HELLO;
 packet_len+=2;

 if(loadavg1 && loadavg5 && loadavg15) {
	hello_packet_data = hello_packet+2;

	val_len = (char)strlen(hostname);
	packet_len+=val_len;
	*hello_packet_data = val_len;
	memcpy(hello_packet_data+1, hostname, *hello_packet_data);
	hello_packet_data+=*hello_packet_data+1;

	val_len = (char)strlen(loadavg1);
	packet_len+=val_len;
	*hello_packet_data = val_len;
	memcpy(hello_packet_data+1, loadavg1, *hello_packet_data);
	hello_packet_data+=*hello_packet_data+1;

	val_len = (char)strlen(loadavg5);
	packet_len+=val_len;
	*hello_packet_data = val_len;
	memcpy(hello_packet_data+1, loadavg5, *hello_packet_data);
	hello_packet_data+=*hello_packet_data+1;

	val_len = (char)strlen(loadavg15);
	packet_len+=val_len;
	*hello_packet_data = val_len;
	memcpy(hello_packet_data+1, loadavg15, *hello_packet_data);
	hello_packet_data+=*hello_packet_data+1;

	*hello_packet_data = 0;
	packet_len+=4;
	
	return packet_len;
 } /* if */
 else
	return -1;
}


/*
 * pack a response packet
 */
int csp_pack_resp_packet(char* resp_packet, char type, char err, char* resp_data)
{
 int packet_len;
 unsigned short int data_len;

 packet_len = 0;
 resp_packet[0] = 1;
 resp_packet[1] = type;
 resp_packet[2] = err;
 packet_len+= 3;

 if(type == CS_RESP_WDATA) {
	data_len = (unsigned short int) strlen(resp_data);
	if(data_len >= CSP_MAXPACKET_SIZE-3) 
		data_len = CSP_MAXPACKET_SIZE;
	memcpy(&resp_packet[3], &data_len, sizeof(unsigned short int));
	strncpy(resp_packet+3+sizeof(unsigned short int), resp_data, data_len);
	packet_len+=data_len+sizeof(unsigned short int);
 } /* if */
 
 return packet_len;
}


/*
 * pack capability packet
 */
int csp_pack_capreq_packet(char* req_packet, char type, char* data)
{
 int packet_len;
 unsigned short int data_len;

 packet_len = 0;
 req_packet[0] = 1;
 req_packet[1] = CS_CAP;
 req_packet[2] = type;
 packet_len+=3;

 data_len = (unsigned short int) strlen(data);
 memcpy(&req_packet[3], &data_len, sizeof(unsigned short int));
 strncpy(req_packet+3+sizeof(unsigned short int), data, data_len);
 packet_len+=data_len+sizeof(unsigned short int);

 return packet_len;
}


/*
 * unpack a response packet
 */
int csp_unpack_resp_packet(char* resp_packet, char* type, char* err, char** resp_data)
{
 unsigned short int data_len;

 *type = resp_packet[1];
 *err = resp_packet[2];
 if(resp_packet[1] == CS_RESP_WDATA) {
	memcpy(&data_len, resp_packet+3, sizeof(unsigned short int));
	*(resp_packet+3+sizeof(unsigned short int)+data_len) = 0;
	*resp_data = resp_packet+3+sizeof(unsigned short int);
 } /* if */

 return 0;
}


/*
 * determine a packets version and type
 */
int csp_det_packet_type(char* packet, char* cs_version, char* cs_type)
{
 *cs_version = packet[0];
 *cs_type = packet[1];

#ifdef DEBUG_S
 cs_log_err(CS_LOG, "cs_version: %d\n", *cs_version);
 cs_log_err(CS_LOG, "cs_type: %d\n", *cs_type);
#endif 

 return 0;
}


/*
 * parse a given db-request
 */
int csp_unpack_dbreq_request(char* req_packet, char** key, char** value, int req_type)
{
 char* key_data, *val_data;
 unsigned short int key_len, val_len;

 memcpy(&key_len, req_packet, sizeof(unsigned short int));
 key_data = req_packet+sizeof(unsigned short int);
 if(req_type == CS_SET) {
 	memcpy(&val_len, key_data+key_len, sizeof(unsigned short int));
	*(key_data+key_len)= '\0';
	val_data = key_data+key_len + sizeof(unsigned short int);
	*(val_data+val_len) = '\0';
 } /* if */
 else
	val_data = 0;

#ifdef DEBUG_S
 cs_log_err(CS_LOG, "key_data is: %s\n", key_data);
 cs_log_err(CS_LOG, "val_data is: %s\n", val_data);
#endif

 *key = key_data;
 *value = val_data;
 return 0;
}


/*
 * pack a database request packet
 */
int csp_pack_dbreq_packet(char* packet, char* key, char* value, char type)
{
 char* packet_data; 
 unsigned short int key_len, val_len, pack_len;

 if(key != NULL)
 	key_len = strlen(key);
 else
	return -1;
 if(value != NULL) 
 	val_len = strlen(value);
 else
	val_len = 0;
 pack_len = 3+key_len+val_len + (3*sizeof(unsigned short int));
 
 packet[0] = CS_VER;
 packet[1] = CS_REQ;
 packet[2] = type;
 packet_data = packet + 3;

 if(key_len > 0) {
	memcpy(packet_data, &key_len, sizeof(unsigned short int));
	packet_data+=sizeof(unsigned short int);
	memcpy(packet_data, key, key_len);
	packet_data+=key_len;
	*packet_data = '\0';
 } /* if */
 if(val_len > 0 && type == CS_SET) {
	memcpy(packet_data, &val_len, sizeof(unsigned short int));
	packet_data+=sizeof(unsigned short int);
	memcpy(packet_data, value, val_len);
	packet_data+=val_len;
	*packet_data = '\0';
 } /* if */

return pack_len;
}
