/*  csda.c is part of dkm
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
 * csda.o, capability and system information distribution arbiter
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csda.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: csda.c,v 0.1 1999/10/27 10:31:28 lanzm Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>

#include "csdb.h"
#include "csda.h"
#include "csl.h"
#include "csc.h"
#include "../include/cs.h"


extern pthread_mutex_t cs_thread_exit;
extern int cs_threads_should_exit;

/*
 * Thread: serve the best_node requests via message-queue
 */
void csda_best_node_server_thread(void)
{
 int should_exit;
 int server_id, client_id;
 struct bn_request req;
 struct bn_response resp;
 char* best_node;
 char* app_name;
 char* hostname;
 int error;

 /*
  * this is a thread
  */

 server_id = msgget(7768, S_IRWXU | S_IWGRP | S_IWOTH | IPC_CREAT);
 if(server_id == -1) {
	cs_log_err(CS_LOG, "csda: unable to create the message queue\n");
	exit(-1);
 } /* if */

 best_node = (char *) malloc(NODE_SIZE * sizeof(char));
 app_name = (char *) malloc(APP_SIZE * sizeof(char));
 hostname = getenv("HOSTNAME");
 
 for(;;) {
	if( msgrcv(server_id, &req, MSQ_MAX_NAME, 0, 0) == -1) {
		cs_log_err(CS_LOG, "csda_best_node_server_thread: unable to receive a message\n");
		continue;
	} /* if */
	error = sscanf(req.msg, "%d %s", &client_id, app_name);

	error = csda_best_node_for_app(app_name, &best_node);
	if(error == -1)
		strcpy(best_node, hostname);
	cs_log_err(CS_LOG, "best_node is: %s error is: %d\n", best_node, error);
	strcpy(resp.msg, best_node);
	resp.mtype = 1;
	msgsnd(client_id, &resp, MSQ_MAX_NODE, 0);

	pthread_mutex_lock(&cs_thread_exit);
	should_exit = cs_threads_should_exit;
	pthread_mutex_unlock(&cs_thread_exit);

	if(should_exit) {
 		msgctl(server_id, IPC_RMID, NULL);
		pthread_exit(0);
	} /* if */
 } /* for */

 msgctl(server_id, IPC_RMID, NULL);

 free(best_node);
 free(app_name);
}


/*
 * register an application on the csdb
 * by scanning its shared libraries and
 * determine capable nodes
 */
int csda_register_app(char* app)
{
 csc_scan_libs_for_app(app);
 csc_update_cap_nodes_for_app(app);

 return 0;
}

/*
 * calculate the goodness of a node
 */
double csda_calc_bench(char* node)
{
 char key[KSIZE];
 double load1, load5, load15;
 int error;

 char* load1_s, *load5_s, *load15_s;
 double bench;
 bench = 0;
 error = 0;

 sprintf(key, ".node.%s.load.avg1", node);
 error = csdb_get_value_for_key(key, &load1_s);
 if(error < 0) return 0;
 else {
	load1 = atof(load1_s);
	free(load1_s);
 } /* else */

 sprintf(key, ".node.%s.load.avg5", node);
 error = csdb_get_value_for_key(key, &load5_s);
 if(error < 0) return 0;
 else {
	load5 = atof(load5_s);
	free(load5_s);
 } /* else */

 sprintf(key, ".node.%s.load.avg15", node);
 error = csdb_get_value_for_key(key, &load15_s);
 if(error < 0) return 0;
 else {
	load15 = atof(load15_s);
	free(load15_s);
 } /* else */

 bench = (1/(1+load1)) * 0.7 + (1/(1+load5)) * 0.2 + (1/(1+load15)) * 0.1;
#ifdef DEMO
 cs_log_err(CS_LOG, "in calc_bench: bench for node: %s is: %f\n", node, bench);
#endif

 return bench;
}


int csda_best_node_for_app(char* app_name, char** node_name)
{
 char* node;
 char* cap_nodes_db;
 char* cap_nodes;
 char key[KSIZE];
 char delims[] = " ";
 double max_bench, bench;
 int error;

 sprintf(key, ".app.%s.capnodes", app_name);
 if(csdb_key_exist(key))
 	error = csdb_get_value_for_key(key, &cap_nodes_db);
 else
	return -1;
 if(error < 0)
	return -1;
 cap_nodes = (char *) malloc((strlen(cap_nodes_db) + 1)*sizeof(char));
 strcpy(cap_nodes, cap_nodes_db);
 node = strtok(cap_nodes, delims);
 max_bench = -1;

 while(node != NULL) {
	if(csc_node_is_alive(node) == 0) {
		node = strtok(NULL, delims);
		continue;
	} /* if */
	bench = csda_calc_bench(node);
	if(bench > max_bench) {
		max_bench = bench;
		strcpy(*node_name, node);
	} /* if */
	node = strtok(NULL, delims);
 } /* while */

#ifdef DEMO
 cs_log_err(CS_LOG, "best node for app: %s is: %s\n", app_name, *node_name);
#endif

 free(cap_nodes_db);
 free(cap_nodes);
 
 return max_bench;
}
