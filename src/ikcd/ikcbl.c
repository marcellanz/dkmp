/*  ikcbl.c is part of dkm
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
 * ikcbl, inter kernel communication backlog functions
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcbl.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: ikcbl.c,v 1.1 1999/12/16 21:58:23 lanzm Exp lanzm $";

#include <stdlib.h>
#include "../include/ikc.h"
#include "ikcl.h"
#include "ikcbl.h"

/*
 * initialize a backlog list
 */
int ikcbl_init_bl(struct ikc_backlog_head* list_head, pthread_mutex_t* mutex)
{
 list_head->list = NULL;
 list_head->mutex = mutex;

 return 0;
}


/*
 * add a request to the backlog list
 */
int ikcbl_add_request(struct ikc_backlog_head* list_head, struct ikc_request* request)
{
 struct ikc_backlog_list* new_list_elem;

 new_list_elem = malloc(sizeof(struct ikc_backlog_list));
 if(!new_list_elem) {
	ikc_log_exit(IKC_LOG, "ikcbl_add: unable to allocate memory for new_list_elem\n");
 } /* if */
 pthread_mutex_lock(list_head->mutex);
 new_list_elem->request = request;
 if(list_head->list == NULL) {
	list_head->list = new_list_elem;
	list_head->list->next = NULL;
	list_head->list->prev = NULL;
 } /* if */
 else {
	new_list_elem->next = list_head->list;
	new_list_elem->next->prev = new_list_elem;
	new_list_elem->prev = NULL;
	list_head->list = new_list_elem;
 } /* else */
 pthread_mutex_unlock(list_head->mutex);

 return 0;
}

/*
 * remove a request from the backlog list
 * mutex is held by the caller ikcbl_remove_threads_last_request
 */
int ikcbl_remove_request(struct ikc_backlog_head* list_head, unsigned int request_nbr)
{
 struct ikc_backlog_list* to_find;

 to_find = list_head->list; /* FIXME: use faster search method */
 while(to_find != NULL && to_find->request->request_nbr != request_nbr) {
	to_find = to_find->next;
 } /* while */

 if(to_find == NULL) {
	return -1;
 } /* if */ 

 if(to_find->next != NULL) {
	to_find->next->prev = to_find->prev;
 } /* if */
 if(to_find->prev != NULL) {
	to_find->prev->next = to_find->next;
 } /* if */

 __ikc_free_request(to_find->request);
 free(to_find->request);
 free(to_find);

 return 0;
}


/*
 * try to respond a request from backlog
 */
int ikcbl_try_respond_from_backlog(struct ikc_backlog_head* list_head, struct ikc_request* request)
{
 struct ikc_backlog_list* to_find;

 pthread_mutex_lock(list_head->mutex);
 to_find = list_head->list;
 while(to_find != NULL && to_find->request->request_nbr != request->request_nbr) {
	to_find = to_find->next;
 } /* while */
 if(to_find == NULL) {
 	pthread_mutex_unlock(list_head->mutex);
	return -1;
 } /* if */
 else {
	memcpy(to_find->request->cliaddr, request->cliaddr, request->cli_len);
	ikcr_respond_to_client(to_find->request);
 } /* else */

 pthread_mutex_unlock(list_head->mutex);

 return 0;
}


/*
 * delete the last request of a thread from backlog
 */
int ikcbl_remove_threads_last_request(struct ikc_backlog_head* list_head, struct ikc_request* request)
{
 struct ikc_backlog_list* to_find;

 pthread_mutex_lock(list_head->mutex);
 to_find = list_head->list;
 while(to_find != NULL && 
       to_find->request->host_id != request->host_id &&
       to_find->request->thread_id != request->thread_id) {
		to_find = to_find->next;
 } /* while */
 if(to_find == NULL) {
	pthread_mutex_unlock(list_head->mutex);
	return -1;
 } /* if */
 else {
	ikcbl_remove_request(list_head, request->request_nbr);
 } /* else */

 pthread_mutex_unlock(list_head->mutex);
 return 0;
}
