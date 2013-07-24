/*  ikcq.c is part of dkm
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
 * ikcq, inter kernel communication queue functions
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcq.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: ikcq.c,v 1.1 1999/12/16 22:13:40 lanzm Exp lanzm $";

#include "../include/ikc.h"
#include <pthread.h>

/*
 * queue a request to the request queue
 */
int ikcq_queue_request(struct ikc_queue_head* head, struct ikc_request* req)
{
 struct ikc_request_queue* tail;
 struct ikc_request_queue* new_q = (struct ikc_request_queue*) malloc(sizeof(struct ikc_request_queue));
 struct ikc_request_queue** q = &(head->queue);
 if(new_q == NULL)
        return -E_IKC_NOMEM;
 /*
  * the queues tail is q->prev => head->prev
  */
 pthread_mutex_lock(head->mutex);
 if(*q == NULL) {
        *q = new_q;
        (*q)->prev = new_q;
        (*q)->next = NULL;
        (*q)->req_struct = req;
 } /* if */
 else {
        tail = (struct ikc_request_queue*)(*q)->prev;

        new_q->req_struct = req;
        new_q->next = tail->next;
        new_q->prev = tail;
        tail->next = new_q;
        (*q)->prev = new_q;
 } /* else */
 pthread_mutex_unlock(head->mutex);

 return 0;
}

/*
 * dequeue a request from a queue
 */
int ikcq_dequeue_request(struct ikc_queue_head* head, struct ikc_request** request)
{
 struct ikc_request_queue* q_deq;       /* queue element to dequeue */
 struct ikc_request_queue** q = &(head->queue);

 pthread_mutex_lock(head->mutex);
 if(*q == NULL) {
        pthread_mutex_unlock(head->mutex);
        return -E_IKC_NOQUEUE;
 } /* if */

 q_deq = *q;
 if(q_deq == NULL) {
        pthread_mutex_unlock(head->mutex);
        return -E_IKC_NOREQ;
 } /* if */
 else {
        if(q_deq->next != NULL)
                q_deq->next->prev = q_deq->prev;
        if(q_deq != *q)
                q_deq->prev->next = q_deq->next;
        if(q_deq == (*q)->prev)
                (*q)->prev = q_deq->prev;
        if(q_deq == *q)
                *q = q_deq->next;
        if(request != NULL) {
                *request = q_deq->req_struct;
        } /* if */
        else {
                __ikc_free_request(q_deq->req_struct);
                free(q_deq->req_struct);
        } /* else */
        free(q_deq);
 } /* else */
 pthread_mutex_unlock(head->mutex);

 return 0;
}


/*
 * remove a request from a queue
 */
int ikcq_remove_request(struct ikc_queue_head* head, struct ikc_request* req)
{
 struct ikc_request_queue* req_in_queue;
 struct ikc_request_queue** q = &(head->queue);

 pthread_mutex_lock(head->mutex);
 if(*q == NULL) {
        pthread_mutex_unlock(head->mutex);
        return -E_IKC_NOQUEUE;
 } /* if */

 req_in_queue = *q;
 while(req_in_queue->req_struct != req && req_in_queue != NULL) {
        req_in_queue = req_in_queue->next;
 } /* while */

 if(req_in_queue == NULL) {
        pthread_mutex_unlock(head->mutex);
        return -E_IKC_NOREQ;
 } /* if */
 else {
        if(req_in_queue->next != NULL)
                req_in_queue->next->prev = req_in_queue->prev;
        if(req_in_queue != *q)
                req_in_queue->prev->next = req_in_queue->next;
        if(req_in_queue == (*q)->prev)
                (*q)->prev = req_in_queue->prev;
        if(req_in_queue == *q)
                *q = req_in_queue->next;
        __ikc_free_request(req_in_queue->req_struct);
        free(req_in_queue);
 } /* else */
 pthread_mutex_unlock(head->mutex);

 return 0;
}

