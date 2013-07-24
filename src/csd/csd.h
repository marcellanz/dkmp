#ifndef _CSD_H
#define _CSD_H
/*
 * csd.h, capability and system information daemon header file
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csd.h,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- 
 * $Id: csd.h,v 0.2 1999/11/05 17:28:33 lanzm Exp lanzm $ */

#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include "csp.h"
#include "csda.h"
#include "csc.h"
#include "csl.h"
#include "../include/cs.h"


/* 
 * function declarations 
 */

void cs_free_ppc(char** ppc);
int cs_do_cap_request_for_node(char* node_name, char* libs);
int cs_do_db_request_for_node(char* node_name, char* key);
void cs_update_db(void);
void cs_send_hello_loop(char** argv);
int cs_proceed_db_request(int sockfd, struct sockaddr* cliaddr, socklen_t len, char* packet);
int cs_proceed_cap_request(int sockfd, struct sockaddr* cliaddr, socklen_t len, char* packet);
int cs_proceed_hello(char* hello_packet);
void cs_recv_packets(void);
void cs_block_signals_for_thread();

#endif
