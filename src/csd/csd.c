/*  csd.c is part of dkm
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
 * csd, capability and system information daemon
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csd.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: csd.c,v 0.3 1999/11/05 17:23:39 lanzm Exp lanzm $";

#include "csd.h"

pthread_mutex_t cs_thread_exit = PTHREAD_MUTEX_INITIALIZER;
int cs_threads_should_exit;
pthread_t tid_packets, tid_hello, tid_update, tid_best_node;


/*
 * free a structure of a pointer to pointers of char
 */
void cs_free_ppc(char** ppc)
{
 char** keys;
 char** keys_begin;
 char* tof;        /* to be freeed */

 keys = ppc;

 if(keys != NULL) { /* free allocated mem by csdb_parse_key */
	keys_begin = keys;
	tof = *keys;
        while(tof != NULL) {
                keys++;
                free(tof);
                tof=*keys;
        } /* while */
        free(keys_begin);
 } /* if */
}


/*
 * timeout helper function for recv_from calls
 */
int cs_sock_read_timeout(int fd, int sec)
{
 fd_set rset;
 struct timeval tv;

 FD_ZERO(&rset);
 FD_SET(fd, &rset);

 tv.tv_sec = sec;
 tv.tv_usec = 0;

 return (select(fd+1, &rset, NULL, NULL, &tv));
}


/*
 * do a capability request to a given node for given libraries
 * return true or false
 * FIXME: don't allocate a new socket for each new cap request.
 *        use a pre allocated socket
 */
int cs_do_cap_request_for_node(char* node_name, char* libs)
{
 int sockfd;
 int n;
 char* node_ip;
 struct hostent* hptr;
 struct sockaddr_in nodeaddr;

 char* req_packet, *resp_data;
 char  resp_packet[MAXLINE];
 char cs_version, cs_type, cs_resp_type, cs_resp_err;
 int req_packet_len, resp_len;

 if((hptr=gethostbyname(node_name)) == NULL)
	return -1;
 if(node_name == NULL || libs == NULL)
	return -1;

 node_ip = hptr->h_addr_list[0];
 memset(&nodeaddr, 0, sizeof(nodeaddr));
 nodeaddr.sin_family = AF_INET;
 nodeaddr.sin_port = htons(CSD_PORT);
 memcpy(&nodeaddr.sin_addr, node_ip, sizeof(struct sockaddr_in));
 sockfd = socket(AF_INET, SOCK_DGRAM, 0);

 req_packet = (char*) malloc(strlen(libs)+1+CSP_CAPREQ_HDR_SIZE);

 req_packet_len = csp_pack_capreq_packet(req_packet, CS_CAPEXEC, libs);
 n = sendto(sockfd, req_packet, req_packet_len, 0, (struct sockaddr*) &nodeaddr, sizeof(nodeaddr));

 if(n < 0) {
	free(req_packet);
	close(sockfd);
	cs_log_err(CS_LOG, "unable to send capability request to %s, I skip the request\n", node_name);
	return -1;
 } /* if */


 if(cs_sock_read_timeout(sockfd, 5) == 0) {
	cs_log_err(CS_LOG, "socket timeout: node: %s didn't respond to cap_request,  I remove it from alive nodes\n", node_name);
	free(req_packet);
	csc_node_not_alive(node_name);
	return -1;
 } /* if */
 else { /* FIXME: resp_len could only be 256, no len check here ! */
 	resp_len = recvfrom(sockfd, resp_packet, MAXLINE, 0, NULL, NULL);
 } /* else */
 close(sockfd);

 if(resp_len < 0) {
	cs_log_err(CS_LOG, "recvfrom failed for node: %s , I remove it from alive nodes\n", node_name);
	free(req_packet);
	csc_node_not_alive(node_name);
	return -1;
	/*return 0;*/
 } /* if */

 csp_unpack_resp_packet(resp_packet, &cs_resp_type, &cs_resp_err, &resp_data);

 if(resp_data == NULL) {
	free(req_packet);
	return -1;
 } /* if */

#ifdef DEBUG
cs_log_err(CS_LOG, "from node: %s for libs: %si => %c\n", node_name, libs, resp_data[0]);
#endif

 if(resp_data[0] == '0') {
 	free(req_packet);
	return 0;
 } /* if */
 if(resp_data[0] == '1') {
 	free(req_packet);
	return 1;
 } /* if */
}


/*
 * Do a database request to a node for a given key and type of request.
 * if the request was successful store the value in our database
 */
int cs_do_db_request_for_node(char* node_name, char* key)
{
 int sockfd;
 char* node_ip;
 struct hostent* hptr;
 struct sockaddr_in nodeaddr;

 char* req_packet, *resp_data;
 char  resp_packet[MAXLINE];
 char cs_version, cs_type, cs_resp_type, cs_resp_err;
 int req_packet_len, resp_len;
 int error;
 int n;

 if((hptr=gethostbyname(node_name)) == NULL)
        return -1;
 if(node_name == NULL || key == NULL)
        return -1;

 node_ip = hptr->h_addr_list[0];
 memset(&nodeaddr, 0, sizeof(nodeaddr));
 nodeaddr.sin_family = AF_INET;
 nodeaddr.sin_port = htons(7678);
 memcpy(&nodeaddr.sin_addr, node_ip, sizeof(struct sockaddr_in));

 sockfd = socket(AF_INET, SOCK_DGRAM, 0);

 req_packet = (char*) malloc(strlen(key)+1+CSP_DBREQ_HDR_SIZE);
 req_packet_len = csp_pack_dbreq_packet(req_packet, key, NULL, CS_GET);

 n = sendto(sockfd, req_packet, req_packet_len, 0, (struct sockaddr*) &nodeaddr, sizeof(nodeaddr));

 if(n <0) {
	close(sockfd);
	free(req_packet);
	cs_log_err(CS_LOG, "unable to send database request to %s, I skip the request\n", node_name);
 	return -1;
 } /* if */

 if(cs_sock_read_timeout(sockfd, 5) == 0) {
        cs_log_err(CS_LOG, "socket timeout: node: %s didn't respond to db_request, I remove it from alive nodes list\n", node_name);
        free(req_packet);
        csc_node_not_alive(node_name);
        return -1;
 } /* if */
 else {
        resp_len = recvfrom(sockfd, resp_packet, MAXLINE, 0, NULL, NULL);
 } /* else */
 close(sockfd);

 if(resp_len < 0) {
        cs_log_err(CS_LOG, "recvfrom failed for node: %s , I remove it from alive nodes\n", node_name);
        free(req_packet);
        csc_node_not_alive(node_name);
        return -1;
        /*return 0;*/
 } /* if */

 csp_unpack_resp_packet(resp_packet, &cs_resp_type, &cs_resp_err, &resp_data);

 if(cs_resp_err == CS_RESP_SUCC) {
#ifdef DEBUG
	cs_log_err(CS_LOG, "value for key: %s is: %s size is:%d\n", key, resp_data, strlen(resp_data));
#endif
	csdb_set_value_for_key(key, resp_data);
	error = 0;
 } /* if */
 else {
	cs_log_err(CS_LOG, "err from node: %s\n", node_name);
	error = -1;
 } /* else */

 close(sockfd); 
 free(req_packet);

 return error;
}


/*
 * update system information, capabilities and best_node information
 */
void cs_update_db(void)
{
 int should_exit;
 int n;
 char* best_node;
 n=0;
 best_node = (char*) malloc(256);

 /* 
  * this is a thread 
  */
 cs_block_signals_for_thread();

 for(;;) {
	csc_set_dynamic_values();
	csc_do_def_req_for_alive_nodes();
	csc_update_cap_nodes_for_apps(); 

#ifdef DEMO
	/*csdb_dump_from_key(".", 1);*/
	csda_best_node_for_app("/bin/sh", &best_node);
	csda_best_node_for_app("/usr/local/bin/gears", &best_node);
#endif
	usleep(CSD_UPDATERATE);

	pthread_mutex_lock(&cs_thread_exit);
	should_exit = cs_threads_should_exit;	
	pthread_mutex_unlock(&cs_thread_exit);
	if(should_exit)
		pthread_exit(0);
 } /* for */ 
 free(best_node);
}


/*
 * send periodicaly hello packets to default csd multicast-address 224.0.1.178
 */
void cs_send_hello_loop(char** argv)
{
 int should_exit;
 int sockfd;
 const int on=1;
 struct in_addr serv_ip;
 char* value;
 struct hostent* hptr;
 struct sockaddr_in servaddr;
 char* hello_packet;
 char* hello_packet_data;
 char* hostname;
 char data_size;
 char key[KSIZE];
 char version;
 char type;
 int packet_len;
 int err_from_db;

 char *loadavg1, *loadavg5, *loadavg15;

 inet_aton("224.0.1.178", &serv_ip);
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_port = htons(7678);
 servaddr.sin_addr = serv_ip;

 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
 hello_packet = (char*) malloc(1024*sizeof(char));
 hostname = getenv("HOSTNAME");
 version = 1;
 type = 1; /* cs_hello */

 /* packet generation */
 hello_packet[0] = version;
 hello_packet[1] = type;

 /* 
  * this is a thread
  */
 cs_block_signals_for_thread();

 for(;;) {
	sprintf(key, ".node.%s.load.avg1", hostname);
	err_from_db = csdb_get_value_for_key(key, &loadavg1);

	sprintf(key, ".node.%s.load.avg5", hostname);
	err_from_db = csdb_get_value_for_key(key, &loadavg5);

	sprintf(key, ".node.%s.load.avg15", hostname);
	err_from_db = csdb_get_value_for_key(key, &loadavg15);

	packet_len = csp_pack_hello_packet(hello_packet, hostname, loadavg1, loadavg5, loadavg15);
#ifdef DEBUG
	cs_log_err(CS_LOG, "packet_len is: %d\n", packet_len);
	cs_log_err(CS_LOG, "from hello hostname: %s, loadavg1: %s, loadavg5: %s, loadavg15: %s\n", hostname, loadavg1, loadavg5, loadavg15);
#endif
	if(loadavg1 && loadavg5 && loadavg15)
        	sendto(sockfd, hello_packet, packet_len, 0, &servaddr, sizeof(servaddr));
	if(loadavg1 != NULL)
		free(loadavg1);
	if(loadavg5 != NULL)
		free(loadavg5);
	if(loadavg15 != NULL);
		free(loadavg15);
#ifdef DEBUG
	cs_log_err(CS_LOG, "hello_packet is: %s\n", hello_packet);
#endif
	usleep(CSD_HELLORATE);
        pthread_mutex_lock(&cs_thread_exit);
        should_exit = cs_threads_should_exit;
        pthread_mutex_unlock(&cs_thread_exit);

        if(should_exit)
                pthread_exit(0);
  } /* for */
}


/*
 * proceed a request to the local csdb for a given request
 */
int cs_proceed_db_request(int sockfd, struct sockaddr* cliaddr, socklen_t len, char* packet)
{
 char* key, *value, *response, *request_packet;
 char req_type, response_packet[CSP_MAXPACKET_SIZE];
 int ret, err, resp_len;

 req_type = packet[0];
 request_packet = packet+1;
 
 csp_unpack_dbreq_request(request_packet, &key, &value, req_type);

 response = NULL;
#ifdef DEBUG_S
 cs_log_err(CS_LOG, "parsed from req: %s => req_type: %d, key: %s, value: %s\n", packet, req_type, key, value);
#endif

#ifdef DEBUG
 if(key) cs_log_err(CS_LOG, "key: %s\n", key);
 if(value) cs_log_err(CS_LOG, "value: %s\n", value);
#endif

 if(key != NULL && csdb_is_valid_key(key)) {
 	switch(req_type) {
		case CS_SET:	if(*value) {
					csdb_set_value_for_key(key, value); /* FIXME, check if set was succeeded */
					resp_len = csp_pack_resp_packet(response_packet, CS_RESP_NDATA, CS_RESP_SUCC, NULL);
				} else
					resp_len = csp_pack_resp_packet(response_packet, CS_RESP_NDATA, CS_RESP_ERR, NULL);
				break;
		case CS_GET:	err = csdb_get_value_for_key(key, &response);
				if(err >= 0)
					resp_len = csp_pack_resp_packet(response_packet, CS_RESP_WDATA, CS_RESP_SUCC, response);
				else
					resp_len = csp_pack_resp_packet(response_packet, CS_RESP_NDATA, CS_RESP_ERR, NULL);
				break;
		case CS_DEL:	err = csdb_remove_from_key(key); 
				if(err < 0)
					resp_len = csp_pack_resp_packet(response_packet, CS_RESP_NDATA, CS_RESP_ERR, NULL);
				else
					resp_len = csp_pack_resp_packet(response_packet, CS_RESP_NDATA, CS_RESP_SUCC, NULL);
				break;
		case CS_LS:	ret = csdb_get_child_names_for_key(key, NULL, &response);
				if(ret <= 0)
					resp_len = csp_pack_resp_packet(response_packet, CS_RESP_NDATA, CS_RESP_ERR, NULL);
				else
					resp_len = csp_pack_resp_packet(response_packet, CS_RESP_WDATA, CS_RESP_SUCC, response);
				break;
	} /* switch */
 } /* if */
 sendto(sockfd, response_packet, resp_len, 0, cliaddr, len);
 
 if(response != NULL)
	free(response); 

 return 0;
}


/*
 * proceed a capability request for the local node and responde with resp_packet (CSP_RESP_WDATA)
 */
int cs_proceed_cap_request(int sockfd, struct sockaddr* cliaddr, socklen_t len, char* packet)
{
 char req_type, resp_packet[CSP_MAXPACKET_SIZE];
 char* req_packet, *req_data;
 unsigned short int data_len;
 int resp_len, ret;

 req_type = packet[0];
 req_packet = packet+1;

 memcpy(&data_len, req_packet, sizeof(unsigned short int));
 req_data = (char*) malloc((data_len+1)*sizeof(char));
 memcpy(req_data, req_packet+sizeof(unsigned short int), data_len);
 req_data[data_len] = '\0';

 switch(req_type) {
	case CS_CAPEXEC:	ret = csc_cap_to_exec(req_data);
				if(ret < 0) {
					resp_len = csp_pack_resp_packet(resp_packet, CS_RESP_NDATA, CS_RESP_ERR, NULL);
				} 
				else if(ret == 1) {
					resp_len = csp_pack_resp_packet(resp_packet, CS_RESP_WDATA, CS_RESP_SUCC, "1");
				}
				else if(ret == 0) {
					resp_len = csp_pack_resp_packet(resp_packet, CS_RESP_WDATA, CS_RESP_SUCC, "0");
				}
				break;
 } /* switch */
 sendto(sockfd, resp_packet, resp_len, 0, cliaddr, len);
 
 free(req_data);

 return 0;
}

/*
 * proceed a hello packet => save information about the sending node
 * to csdb
 */
int cs_proceed_hello(char* hello_packet)
{
 char* len; 			/* points to the lenght of the next data entry
				 * lenth of a field is limited to 127 bytes */
 char* data_begin;		/* begin and end pointer to data */
 char  data[127];		/* a data field */
 char  nodename[127];
 char  key[KSIZE];
 int field;			/* in wich datafield we are */
				/*
				 * 0: nodename
				 * 1: loadavg1
				 * 2: loadavg5
				 * 3: loadavg15 */
 len = hello_packet;
 field = 0;
 while(len != 0) {
	data_begin = len+1;
	switch(field) {
		case 0:	memcpy(nodename, data_begin, *len);
			nodename[*len] = 0;
			if(csc_node_is_alive(nodename) == 0)
				 cs_log_err(CS_LOG, "csd: detected node: %s\n", nodename);
			field = 1;
			len = data_begin + (*len);
			break;
		case 1: memcpy(data, data_begin, *len);
			data[*len] = 0;
			sprintf(key, ".alive.%s.loadavg1", nodename);
			csdb_set_value_for_key(key, data);
			field = 2;
			len = data_begin + (*len);
			break;
		case 2: memcpy(data, data_begin, *len);
			data[*len] = 0;
			sprintf(key, ".alive.%s.loadavg5", nodename);
			csdb_set_value_for_key(key, data);
			field = 3;
			len = data_begin + (*len);
			break;
		case 3: memcpy(data, data_begin, *len);
			data[*len] = 0;
			sprintf(key, ".alive.%s.loadavg15", nodename);
			csdb_set_value_for_key(key, data);
			field = -1;
			len = 0;
			break;
		default:
			len = 0;
			break;
	} /* switch */
 } /* while */

 return 0;
}


/*
 * this is the main loop to receive packets from CSD_PORT (usualy port 7678)
 * for the local interface and the default multicast csd address 224.0.1.178
 */ 
void cs_receive_packets(void)
{
 int should_exit;
 int n;
 socklen_t len;
 char cs_version;
 char cs_type;
 char* packet;

 /* unicast vars */
 socklen_t clilen;
 int sockfd;
 struct sockaddr_in servaddr, cliaddr;

 /* mcast vars */
 struct in_addr mcast_group;
 struct ip_mreq mreq;

 /* unicast */
 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 servaddr.sin_port = htons(CSD_PORT);
 bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));

 /* join mcast group */
 inet_aton("224.0.1.178", &mcast_group);
 memcpy(&mreq.imr_multiaddr, &mcast_group, sizeof(struct in_addr));
 mreq.imr_interface.s_addr =  htonl(INADDR_ANY);
 setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
 
 clilen = sizeof(cliaddr);
 packet = (char*) malloc(MAXLINE);

 /* 
  * this is a thread 
  */

 for(;;) {
 	len = clilen;
	n = recvfrom(sockfd, packet, MAXLINE, 0, &cliaddr, &len);
	packet[n] = '\0';
#ifdef DEBUG_S
	cs_log_err(CS_LOG, "%s recvd\n", packet);
#endif
	csp_det_packet_type(packet, &cs_version, &cs_type);

	if(cs_version != CS_VER) {
		cs_log_err(CS_LOG, "cs_receive_packets: invalid version\n");
		/* FIXME : send resp_error packet here */
	} /* if */
	else {
		switch(cs_type) {
		case CS_HELLO:	cs_proceed_hello(packet+CS_PACKET_HDR_SIZE);
				break;
		case CS_REQ:	cs_proceed_db_request(sockfd, (struct sockaddr*) &cliaddr, len, packet+CS_PACKET_HDR_SIZE);
				break;
		case CS_CAP:	cs_proceed_cap_request(sockfd, (struct sockaddr*) &cliaddr, len, packet+CS_PACKET_HDR_SIZE);
		default:
				break;
		} /* switch */
	} /* else */

        pthread_mutex_lock(&cs_thread_exit);
        should_exit = cs_threads_should_exit;
        pthread_mutex_unlock(&cs_thread_exit);
        if(should_exit)
                pthread_exit(0);
 } /* for */
}

/*
 * daemonize this process
 */
int cs_daemonize(void)
{
 pid_t pid;

 if( (pid=fork()) < 0)
	return -1; 
 else if(pid != 0)
	exit(0); /* parent */

 /* this is childs work */
 setsid(); 	/* become a sessionleader */
 /*chdir("/"); 	/* switch to root dir */
 umask(0); 	/* clear file creating mask */

 return 0;
}


/*
 * join all threads after they terminate
 */
void cs_join_threads(void)
{
 pthread_join(tid_update, NULL);
 cs_log_err(CS_LOG, "tid_update thread: \t%d joined\n", tid_update);
 pthread_join(tid_packets, NULL);
 cs_log_err(CS_LOG, "tid_packets thread: \t%d joined\n", tid_packets);
 pthread_join(tid_hello, NULL);
 cs_log_err(CS_LOG, "tid_hello thread: \t%d joined\n", tid_hello);
 pthread_join(tid_best_node, NULL);
 cs_log_err(CS_LOG, "tid_best_node thread: \t%d joined\n", tid_best_node);
}

/*
 * signal handler
 */

/*
 * signal handler for SIGTERM
 */
void cs_on_sigterm(int signal)
{
 csc_dump_db_to_file(CS_DB_DUMP_PATH, ".");
 cs_log_err(CS_LOG, "csd: SIGTERM received I'll terminate\n");

 pthread_mutex_lock(&cs_thread_exit);
 cs_threads_should_exit = 1;
 pthread_mutex_unlock(&cs_thread_exit);

 if(pthread_self() != tid_best_node && pthread_self() != tid_packets)
 	cs_join_threads();
 else
	pthread_exit(0);

 exit(0);
}


/*
 * signal handler for SIGHUP
 */
void cs_on_sighup(int signal)
{
 csc_dump_db_to_file(CS_DB_DUMP_PATH, ".");
 cs_log_err(CS_LOG, "csd: SIGHUP received, I dumped the database to: /tmp/db_dump\n");
}


/*
 * signal handler for SIGINT
 */
void cs_on_sigint(int signal)
{
 csc_dump_db_to_file(CS_DB_DUMP_PATH, ".");
 cs_log_err(CS_LOG, "csd: SIGINT received, I'll terminate\n");

 pthread_mutex_lock(&cs_thread_exit);
 cs_threads_should_exit = 1;
 pthread_mutex_unlock(&cs_thread_exit);

 if(pthread_self() != tid_best_node && pthread_self() != tid_packets)
 	cs_join_threads();
 else
	pthread_exit(0);

 exit(0);
}


/*
 * signal handler installation
 */
void cs_install_handler(void)
{
 struct sigaction s_handler;

 /* sigterm */
 s_handler.sa_handler = cs_on_sigterm;
 sigemptyset(&s_handler.sa_mask);
 s_handler.sa_flags = 0;
 sigaction(SIGTERM, &s_handler, NULL);

 /* sighup */
 s_handler.sa_handler = cs_on_sighup;
 sigemptyset(&s_handler.sa_mask);
 s_handler.sa_flags = 0;
 sigaction(SIGHUP, &s_handler, NULL);

 /* sigint */
 s_handler.sa_handler = cs_on_sigint;
 sigemptyset(&s_handler.sa_mask);
 s_handler.sa_flags = 0;
 sigaction(SIGINT, &s_handler, NULL);
}


/*
 * block some termination related signals 
 * the threads will handle termination with
 * a synchronized global variable
 */
void cs_block_signals_for_thread(void)
{
 sigset_t block_mask;

 sigemptyset(&block_mask);
 sigaddset(&block_mask, SIGQUIT);
 sigaddset(&block_mask, SIGINT);
 sigaddset(&block_mask, SIGTERM);
 
 sigprocmask(SIG_BLOCK, &block_mask, NULL);
}


/* 
 * the main line, initialize and invoke threads
 */
int main(int argc, char** argv)
{

/*
 * daemonize
 */
 if(argc == 2)
 	cs_daemonize();

 pthread_mutex_lock(&cs_thread_exit);
 cs_threads_should_exit = 0;
 pthread_mutex_unlock(&cs_thread_exit);

 csdb_init();
 cs_install_handler();

// csdb_test2();
// return 0;

 csc_scan_libs();
 csc_set_dynamic_values();


 csc_read_default_db_requests_from_file(CS_DEF_DB_REQ_PATH);
 csc_read_registered_apps_from_file(CS_REGISTERED_APPS_PATH);
#ifdef DEMO_2
 csdb_dump_from_key(".", 1);
#endif

 /* use threads */
 /*
  * should we be ANSI-C compilant? gcc says: csd.c:369: warning: ANSI forbids passing arg 3 of `pthread_create' between function pointer and `void *'
  */

 pthread_create(&tid_packets, NULL, (void*) &cs_receive_packets, NULL);
 pthread_create(&tid_hello, NULL, (void*) &cs_send_hello_loop, argv);
 pthread_create(&tid_update, NULL, (void*) &cs_update_db, NULL);
 pthread_create(&tid_best_node, NULL, (void*) &csda_best_node_server_thread, NULL);

 /* 
  * wait for signals 
  */
 for(;;)
	pause();

 return 0;
}
