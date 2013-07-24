#ifndef _CSP_H
#define _CSP_H
/*
 * csp.h, capability and system information protocoll header file
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csp.h,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- 
 * $Id: csp.h,v 0.2 1999/11/05 17:30:40 lanzm Exp lanzm $ */

/*
 * function declaration
 */

int csp_pack_hello_packet(char* hello_packet, char* hostname, char* loadavg1, char* loadavg5, char* loadavg15);
int csp_pack_resp_packet(char* resp_packet, char type, char err, char* resp_data);
int csp_pack_capreq_packet(char* req_packet, char type, char* data);
int csp_unpack_resp_packet(char* resp_packet, char* type, char* err, char** resp_data);
int csp_det_packet_type(char* packet, char* cs_version, char* cs_type);
int csp_unpack_dbreq_request(char* req_packet, char** key, char** value, int req_type);
int csp_pack_dbreq_packet(char* packet, char* key, char* value, char type);

#endif
