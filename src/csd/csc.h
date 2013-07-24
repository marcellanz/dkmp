#ifndef _CSC_H
#define _CSC_H
/*
 * csc.h, capability and system information collector header file
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csc.h,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * -----------------
 * $Id: csc.h,v 0.2 1999/11/05 17:29:02 lanzm Exp lanzm $ */

#include "csdb.h"


/*
 * function declaration
 */

int csc_do_def_req_for_alive_nodes(void);
int csc_read_default_db_requests_from_file(char* path);
int csc_read_registered_apps_from_file(char* path);
int csc_cap_to_exec(char* libs);
int csc_scan_libs_for_app(char* appname);
int csc_get_libs_for_app(char* appname, char** libs);
int csc_scan_libs(void);
int csc_update_cap_nodes_for_apps(void);
int csc_update_cap_nodes_for_app(char* app);
int csc_get_procval(char* filename, char** value);
int csc_set_load(void);
int csc_set_uptime(void);
int csc_set_mem(void);
int csc_set_cpu(void);
void csc_set_dynamic_values(void);
int csc_node_is_alive(char* node_name);
int csc_node_not_alive(char* node_name);
int csc_dump_db_to_file(char* path, char* key);

#endif
