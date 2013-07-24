/*
 * ikcq, inter kernel communication queue header file
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcq.h,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * -----------------
 * $Id: ikcq.h,v 1.1 1999/12/16 22:15:11 lanzm Exp lanzm $ */


/*
 * function declaration
 */

int ikcq_queue_request(struct ikc_queue_head* head, struct ikc_request* req);
int ikcq_dequeue_request(struct ikc_queue_head* head, struct ikc_request** request);
int ikcq_remove_request(struct ikc_queue_head* head, struct ikc_request* req);
