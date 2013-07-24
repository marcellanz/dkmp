/*
 * ikcp, inter kernel communication protocol header file
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcp.h,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * -----------------
 * $Id: ikcp.h,v 1.1 1999/12/16 22:13:02 lanzm Exp lanzm $ */

/*
 * function declaration
 */
int ikcp_det_packet_type(char* packet, char** version, char** type);
int ikcp_pack_proc_creat_req(char** packet, unsigned short int seq, pid_t o_pid, char* o_hostname, char* o_filename, char* o_argv);
int ikcp_unpack_proc_creat_req(char* packet, unsigned short int* seq, pid_t* o_pid, char* o_hostname, char** o_filename, char** o_argv);
