/*  ikcd.c is part of dkm
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

 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcd.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: ikcd.c,v 1.1 1999/12/16 22:02:26 lanzm Exp lanzm $";

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/uio.h>

#include <linux/dkm.h>		/* for dkm  */
#include "../include/ikc.h"	/* default  */
#include "ikcp.h"		/* protocol */
#include "ikcl.h"		/* logging  */
#include "ikcr.h" 		/* request functions  */
#include "ikcbl.h"		/* backlist functions */

int sig_usr2_flag;	/* indicates if requests are pending */

pthread_mutex_t ikc_out_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ikc_out_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t ikc_in_cond_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t ikc_in_cond = PTHREAD_COND_INITIALIZER;

struct ikc_queue_head ikc_qh_pending_out;
pthread_mutex_t ikc_po_mutex = PTHREAD_MUTEX_INITIALIZER; /* mutex for pending out queue */

struct ikc_queue_head ikc_qh_pending_in;
pthread_mutex_t ikc_pi_mutex = PTHREAD_MUTEX_INITIALIZER; /* mutex for pending in queue */

struct ikc_backlog_head ikc_backlog_lst;
pthread_mutex_t ikc_backlog_mutex = PTHREAD_MUTEX_INITIALIZER; /* mutex for the backlog list */

pthread_t tid_packets;
pthread_t tid_out_worker_1, tid_out_worker_2, tid_out_worker_3;
pthread_t tid_in_worker_1, tid_in_worker_2, tid_in_worker_3;

/*
 * initialize the queues
 */
int ikc_init_queues(void)
{
 ikc_qh_pending_out.queue = NULL;
 ikc_qh_pending_out.mutex = &ikc_po_mutex;

 ikc_qh_pending_in.queue = NULL;
 ikc_qh_pending_in.mutex = &ikc_pi_mutex;

 return 0;
}


/*
 * create a process for a remote host
 */
int ikc_create_process_for_remote_host(char* node, char* o_filename, char* o_argv, pid_t o_pid, pid_t* r_pid)
{
 /* 
  * dummy function, not implemented yet
  */
 *r_pid = o_pid+1;

 return 0;
}


/*
 * Thread: incoming request collector
 */
void ikc_incoming_request_collector(void)
{
 int sockfd;
 struct sockaddr_in servaddr, cliaddr;
 socklen_t cli_len;
 
 char ikc_version;
 char ikc_type;
 char* packet;
 int n;
 int error;

 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 if(sockfd < 0) {
	ikc_log_exit(IKC_LOG, "ikc_incoming_request_collector: unable to get a free socket\n");
 } /* if */
 memset(&servaddr, 0, sizeof(servaddr));
 servaddr.sin_family = AF_INET;
 servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
 servaddr.sin_port = htons(IKC_PORT);
 error = bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr));
 if(error < 0) {
	ikc_log_exit(IKC_LOG, "ikc_incoming_request_collector: unable to bind a socket\n");
 } /* if */
 cli_len = sizeof(cliaddr);
 
 for(;;) {
	n = recvfrom(sockfd, NULL, 0, MSG_PEEK, &cliaddr, &cli_len);
	ioctl(sockfd, FIONREAD, &n);
	packet = (char*) malloc(n);
	n = recvfrom(sockfd, packet, n, 0, &cliaddr, &cli_len);
#ifdef DEBUG
	ikc_log_err(IKC_LOG, "ikc_incoming_request_collector: received a packet\n");
#endif
	if(n < IKC_MIN_PACKET_SIZE) {
		free(packet);
		ikc_log_err(IKC_LOG, "ikc_incoming_request_collector: received to short packet\n");
		continue;
	} /* if */
	ikc_version = packet[0];
	ikc_type = packet[1];
	if(ikc_version != IKC_VERSION) {
		ikc_log_err(IKC_LOG, "ikc_incoming_request_collector: received packet with invalid version\n");
	} /* if */
	else {
		switch(ikc_type) {
		case IKC_PROC_CREAT_REQ: /* start a process for a remote node */
					ikcr_queue_process_creation_request(&cliaddr, cli_len, packet);
					free(packet);
					break;
		case IKC_SYSCALL_REQ:	/* do things to proceed a remote system-call request */
					break;

		default:		ikc_log_err(IKC_LOG, "ikc_incoming_request_collector: received unknown packet type\n");
					break;
		} /* switch */
	} /* else */
 } /* for */

}


/*
 * Thread: handle a request from the outgoing request queue
 */
void ikc_out_request_worker(unsigned char* ikc_thread_id)
{
 struct ikc_request* request; 
 int sockfd;
 unsigned short int seq_nbr;
 unsigned int attempts;
 int error;

 sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 if(sockfd < 0) {
        ikc_log_exit(IKC_LOG, "ikcr_proceed_outgoing_request: unable to get a free socket\n");
 } /* if */
 seq_nbr = -1;

 for(;;) {
	pthread_mutex_lock(&ikc_out_cond_mutex);
	/*
	 * wait until a new request is queued on the pending_out queue
	 */
	pthread_cond_wait(&ikc_out_cond, &ikc_out_cond_mutex);
	pthread_mutex_unlock(&ikc_out_cond_mutex);
	while(ikcq_dequeue_request(&ikc_qh_pending_out, &request) >= 0) {
		attempts = 0;
		if(seq_nbr == 1024)
			seq_nbr = 0;
		else {
			seq_nbr++;
		} /* else */
		request->seq_nbr = seq_nbr;
		error = -1;
		while(attempts < IKC_MAX_ATTEMPTS && error < 0) {
			error = ikcr_proceed_outgoing_request(sockfd, request, *ikc_thread_id);
			attempts++;
		} /* while */
		ikc_respond_to_kernel(request);
	} /* while */
 } /* for */

 close(sockfd);
}

/*
 * Thread: handle a request from the incoming request queue
 */
void ikc_in_request_worker(void)
{
 struct ikc_request* request;

 for(;;) {
        pthread_mutex_lock(&ikc_in_cond_mutex);
        /*
         * wait until a new request is queued on the pending_in queue
         */
        pthread_cond_wait(&ikc_in_cond, &ikc_in_cond_mutex);
        pthread_mutex_unlock(&ikc_in_cond_mutex);
        while(ikcq_dequeue_request(&ikc_qh_pending_in, &request) >= 0) {
		if(ikcbl_try_respond_from_backlog(&ikc_backlog_lst, request) < 0) {
			ikcbl_remove_threads_last_request(&ikc_backlog_lst, request);
                	ikcr_proceed_incoming_request(request);
                	ikcr_respond_to_client(request);
			ikcbl_add_request(&ikc_backlog_lst, request);
		} /* if */
        } /* while */
 } /* for */
}


/*
 * free allocated memory of a request
 */
int __ikc_free_request(struct ikc_request* req)
{
 if(req->cliaddr != NULL)
	free(req->cliaddr);

 return 0;
}


/*
 * put response to kernel 
 */
int ikc_respond_to_kernel(struct ikc_request* request)
{
 int error;

 switch(request->req) {
  case IKC_PROC_CREAT_REQ:	request->rq_argv.arg0 = &request->r_pid;
				error = dkmctl(DKM_PUT_RESP, request->o_pid, request->req, &request->rq_argv);
				break;
  default:			break;
 } /* switch */

 return error;
}


/*
 * get req and arguments
 */
int ikc_get_request_from_kernel(void)
{
 pid_t o_pid;
 int req;
 int error;
 struct ikc_request* request;
 char* best_node;
 char* app_name;
 struct req_argv rq_argv;

 app_name = (char*) malloc(IKC_MAX_FILE_LEN);

#ifdef DEBUG
 ikc_log_err(IKC_LOG, "ikc_get_request_from_kernel: get requests\n");
#endif

again:
 rq_argv.arg0 = &o_pid;
 rq_argv.arg1 = &req;
 error = dkmctl(DKM_GET_REQ, 0, 0, &rq_argv);
 if(error == 1) /* no pending req found */
	return 0;
#ifdef DEBUG
 ikc_log_err(IKC_LOG, "ikc_get_request_from_kernel: DKM_GET_REQ: pid is: %d, req is: %d\n", o_pid, req); 
#endif
 switch(req) {
  case MAP_REQ: rq_argv.arg0 = app_name;
		error = dkmctl(DKM_GET_ARGS, o_pid, req, &rq_argv);
		if(error == 1) {
			ikc_log_err(IKC_LOG, "ikc_get_request_from_kernel: panic: unable to get args after getting req\n");
			ikc_log_exit(IKC_LOG, "ikc_get_request_from_kernel: panic: process is in undefined state\n");
		} /* if */
		ikcr_best_node_for_app(app_name, &best_node);
		rq_argv.arg0 = best_node;
		error = dkmctl(DKM_PUT_RESP, o_pid, req, &rq_argv);
		free(best_node);
		break;

  case IKC_PROC_CREAT_REQ: 
		request = (struct ikc_request*) malloc(sizeof(struct ikc_request));
		request->rq_argv.arg0 = request->r_node;
		request->rq_argv.arg1 = (char*) malloc(IKC_MAX_FILE_LEN);
		request->o_pid = o_pid;
		request->req = req;
		error = dkmctl(DKM_GET_ARGS, request->o_pid, request->req, &request->rq_argv);
		if(error == 1) {
			ikc_log_err(IKC_LOG, "ikc_get_request_from_kernel: panic: unable to get args after getting req\n");
			ikc_log_exit(IKC_LOG, "ikc_get_request_from_kernel: panic: process is in undefined state\n");
		} /* if */
		strcpy(request->o_node, getenv("HOSTNAME"));
	
		ikcq_queue_request(&ikc_qh_pending_out, request);
		pthread_mutex_lock(&ikc_out_cond_mutex);
		/*
		 * inform an another working thread to process a queued pending request
		 */
		pthread_cond_signal(&ikc_out_cond);
		pthread_mutex_unlock(&ikc_out_cond_mutex);
                break;

  case READ_REQ: /* not implemented yet:

		pthread_mutex_lock(&ikc_out_cond_mutex);
		/* 	
		 * inform an another working thread to process a queued pending request 
		 */
		pthread_cond_signal(&ikc_out_cond);	
		pthread_mutex_unlock(&ikc_out_cond_mutex);

		break;
  default:      return -1;  /* TODO: check this better */
 } /* switch */
 goto again;

}


/*
 * signal handler for sigusr2
 */ 
void on_sigusr2(int signal, siginfo_t* si, void* x) 
{
 sig_usr2_flag = 1;
}


/*
 * signal handler for signals sigterm and sigint
 */
void on_sigterm_int(int signal, siginfo_t* si, void* x)
{
 dkmctl(DKM_IKCD_UNREG, 0, 0, NULL);
} 


/*
 * install signal handler
 */
void install_handler(void)
{
 struct sigaction s_handler;

 /* 
  * sigusr2 
  */
 /*s_handler.sa_handler = on_sigusr2;*/
 s_handler.sa_handler = NULL;
 s_handler.sa_sigaction = on_sigusr2;
 sigemptyset(&s_handler.sa_mask);
 s_handler.sa_flags = 0;
 s_handler.sa_flags |= SA_SIGINFO;
 sigaction(SIGUSR2, &s_handler, NULL);

 /* 
  * sigterm / sigint 
  */
 s_handler.sa_handler = NULL;
 s_handler.sa_sigaction = on_sigterm_int;
 sigemptyset(&s_handler.sa_mask);
 s_handler.sa_flags = 0;
 s_handler.sa_flags |= SA_SIGINFO;
 sigaction(SIGTERM|SIGINT, &s_handler, NULL);
}

/*
 * daemonize this process
 */
int ikc_daemonize(void)
{
 pid_t pid;

 if( (pid=fork()) < 0)
        return -1;
 else if(pid != 0)
        exit(0); /* parent */

 /* this is childs work */
 setsid();      /* become a sessionleader */
 /*chdir("/");  /* switch to root dir */
 umask(0);      /* clear file creating mask */

 return 0;
}



/*
 * the main line: initialization and thread creation
 */
int main(unsigned int argc, char** argv)
{
 int error;
 int  csd_pid;
 sigset_t new_mask, old_mask, null_mask;

 unsigned char ikc_out_thread_id_1, ikc_out_thread_id_2, ikc_out_thread_id_3;

/*
 * daemonize
 */
 /*ikc_daemonize();*/

 ikc_init_queues();
 ikcbl_init_bl(&ikc_backlog_lst, &ikc_backlog_mutex);
 ikc_out_thread_id_1=1;
 ikc_out_thread_id_2=2;
 ikc_out_thread_id_3=3;

/*
 * create threads
 */
 pthread_create(&tid_packets, NULL, (void*) &ikc_incoming_request_collector, NULL);
 pthread_create(&tid_in_worker_1, NULL, (void*) &ikc_in_request_worker, NULL);
 pthread_create(&tid_in_worker_2, NULL, (void*) &ikc_in_request_worker, NULL);
 pthread_create(&tid_in_worker_3, NULL, (void*) &ikc_in_request_worker, NULL);
 pthread_create(&tid_out_worker_1, NULL, (void*) &ikc_out_request_worker, &ikc_out_thread_id_1);
 pthread_create(&tid_out_worker_2, NULL, (void*) &ikc_out_request_worker, &ikc_out_thread_id_2);
 pthread_create(&tid_out_worker_3, NULL, (void*) &ikc_out_request_worker, &ikc_out_thread_id_3);

 csd_pid = getpid();
 sig_usr2_flag = 0;

 sigemptyset(&null_mask);
 sigemptyset(&new_mask);
 sigaddset(&new_mask, SIGUSR2);

 install_handler();
 dkmctl(DKM_IKCD_REG, csd_pid, 0, NULL);

 sigprocmask(SIG_SETMASK, &null_mask, NULL);
 error = 0;
 while(78==78) {
	sigsuspend(&null_mask);
	if(sig_usr2_flag == 1) {
		sig_usr2_flag = 0;
		sigprocmask(SIG_BLOCK, &new_mask, &old_mask);
		error = ikc_get_request_from_kernel();
		if(error == -1) {
			ikc_log_exit(IKC_LOG, "ikc main: unexpected error in ikc_get_request_from_kernel()\n");
		} /* if */
	} /* if */
 } /* while */

 dkmctl(DKM_IKCD_UNREG, 0, 0, NULL); 
 return 0;
} 
