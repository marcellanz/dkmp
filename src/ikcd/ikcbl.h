/*
 * ikcbl, inter kernel communication backlog header file
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcbl.h,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * -----------------
 * $Id: ikcbl.h,v 1.1 1999/12/16 22:02:05 lanzm Exp lanzm $ */

struct ikc_backlog_list {
	struct ikc_request* request;
	struct ikc_backlog_list* next;
	struct ikc_backlog_list* prev;
};

struct ikc_backlog_head {
	struct ikc_backlog_list* list;
	pthread_mutex_t* mutex;
};

/*
 * ikcbl function declaration
 */

int ikcbl_init_bl(struct ikc_backlog_head* list_head, pthread_mutex_t* mutex);
int ikcbl_add_request(struct ikc_backlog_head* list_head, struct ikc_request* request);
int ikcbl_remove_request(struct ikc_backlog_head* list_head, unsigned int request_id);
int ikcbl_try_respond_from_backlog(struct ikc_backlog_head* list_head, struct ikc_request* request);
int ikcbl_remove_threads_last_request(struct ikc_backlog_head* list_head, struct ikc_request* request);
