/*  csc.c is part of dkm
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
 * csc.o, capability and system information collector
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csc.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: csc.c,v 0.4 2000/07/11 21:42:10 lanzm Exp lanzm $";

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/cs.h"
#include "csc.h"
#include "csd.h"
#include "csl.h"


/*
 * register that a node is alive
 */
int csc_node_is_alive(char* node_name)
{
 char key[KSIZE];
 sprintf(key, ".alive.%s", node_name);

 return csdb_key_exist(key);
}

/*
 * remove an alive node from db
 */
int csc_node_not_alive(char* node_name)
{
 char key[KSIZE];

 if(node_name == NULL)
	return -1;
 sprintf(key, ".alive.%s", node_name);
 csdb_remove_from_key(key);

 return 0;
}

/*
 * do all default requests given by the def_req.rc file for all alive nodes
 */
int csc_do_def_req_for_alive_nodes(void)
{
 char** alive_nodes;
 char** node;
 int nr_nodes;
 char* req, *requests, *req_str;
 char key[256];
 char delims[]=" ";
 char* this_hostname;
 int error;

 nr_nodes = csdb_get_child_names_for_key(".alive", &alive_nodes, NULL);
 if(nr_nodes < 1)
	return -1;
 node = alive_nodes; 

 error = csdb_get_value_for_key(".config.def_db_req", &req_str);
 if(error < 0) {
 	cs_free_ppc(alive_nodes);
	return -1;
 } /* if */

 requests = (char*) malloc((strlen(req_str)+1)*sizeof(char));
 strcpy(requests, req_str);
 this_hostname = getenv("HOSTNAME");

 while(*node != NULL) {
	strcpy(requests, req_str);
	req = strtok(requests, delims);
	while(req != (char*) NULL && strcmp(this_hostname, *node) != 0) {
		sprintf(key, ".node.%s%s", *node, req);
		error = cs_do_db_request_for_node(*node, key);
#ifdef DEBUG
		cs_log_err(CS_LOG, " did cs_do_db_request_for_node(*node, key); for node: %s, key %s, error is: %d; this_hostname is: %s, req is: %s\n", *node, key, error, this_hostname, req);
#endif
		if(error < 0) {
			req = (char*)NULL;
			cs_log_err(CS_LOG, "error in csc_do_def_req_for_alive_nodes\n");
		} /* if */
		else
			req = strtok(NULL, delims);
	} /* while */
	node++;
 } /* while */

 free(requests);
 free(req_str);
 cs_free_ppc(alive_nodes);

 return 0;
}


/*
 * read default requests for alive nodes an store it in local db under .config.def_db_req
 */
int csc_read_default_db_requests_from_file(char* path)
{
 int fd;
 struct stat attr;
 char* 	map;
 char* line;
 char delims[] = "\n";
 char key[]=".config.def_db_req";

 fd = open(path, O_RDONLY);
 if(fd < 0)
	cs_log_exit(CS_LOG, "csd: could not open: %s\n", path);
 fstat(fd, &attr);
 map = (char*) mmap(NULL, attr.st_size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
 close(fd);

 line = strtok(map, delims); 
 while(line != NULL) {
	if(csdb_key_exist(key)) 
		csdb_append_value_for_key(key, line, " ");
	else
		csdb_set_value_for_key(key, line);
	line = strtok(NULL, delims);
 } /* while */
 munmap(map, attr.st_size);
	
 return 0;
}


/*
 * read the registered applications from a given file store it in 
 * the local database under .app.<path_of_application> .
 * storage does also determination of requiered libraries,
 * which stored under .app.<path_of_application>.lib
 */
int csc_read_registered_apps_from_file(char* path)
{
 int fd;
 struct stat attr;
 char* map;
 char* app_path;
 char delims[]="\n";
 char key[256];

 fd = open(path, O_RDONLY);
 if(fd < 0) {
	cs_log_err(CS_LOG, "csd: could not open: %s\n", path);
	return -1;
 } /* if */

 fstat(fd, &attr);
 map = (char*) mmap(NULL, attr.st_size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
 close(fd);

 app_path = strtok(map, delims);
 while(app_path != NULL) {
	sprintf(key, ".app.%s", app_path);
	if(!csdb_key_exist(key)) {
		csc_scan_libs_for_app(app_path);
	} /* if */
	app_path = strtok(NULL, delims);
 } /* while */
 munmap(map, attr.st_size);

 return 0;
}

/*
 * dump the database from a given key to a file
 */
int csc_dump_db_to_file(char* path, char* key)
{
 int fd;

 fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, S_IROTH|S_IRGRP|S_IRUSR|S_IWUSR);
 if(fd < 0)
 	return -1;
 csdb_dump_from_key(key, fd);
 close(fd);

 return 0;
}


/*
 * read dumped database from file
 */
int csc_read_db_from_file(char* path)
{
 int fd;
 struct stat attr;
 char* map;
 char* line;
 char delims[]= "\n";
 char* key;
 char* value;

 fd = open(path, O_RDONLY);
 if(fd < 0)
	return -1;
 
 fstat(fd, &attr);
 map = (char*) mmap(NULL, attr.st_size, PROT_WRITE | PROT_READ, MAP_PRIVATE, fd, 0);
 close(fd); 

 line = strtok(map, delims);
 while(line != NULL) {
	key = line;
	value = strchr(line, '=');
	if(value == NULL) {
		line = strtok(NULL, delims);
		continue;
	} /* if */
	*value = '\0';
	value = value+1;
	if(key && value)
		csdb_set_value_for_key(key, value);	
	line = strtok(NULL, delims);
 } /* while */

 munmap(map, attr.st_size);

 return 0;
}


/* 
 * check if this node is capable to execute a application with 
 * the given libraries
 */
int csc_cap_to_exec(char* libs)
{
 char* lib_names, *tok, *c, *key;
 char delims[] = " \n";
 int ok;

 /* replace points with _ */
 lib_names = (char*) malloc(strlen(libs)+1);
 key = (char*) malloc(strlen(libs)+1);
 strcpy(lib_names, libs);
 c = lib_names;
 while(*c != (char)NULL) {
	if(*c == '.') *c='_';
	c++;
 } /* while */

 ok = 1;
 tok = strtok(lib_names, delims);
 while(tok != NULL && ok) {
	sprintf(key, ".lib.%s", tok);
	ok = csdb_key_exist(key);
	tok = strtok(NULL, delims);
 } /* while */

 free(lib_names);
 free(key);

 return ok;
}


/*
 * get the library names of a application
 */
int csc_get_libs_for_app(char* appname, char** libs)
{
 char key[KSIZE];

 sprintf(key, ".app.%s.lib", appname);

 return csdb_get_value_for_key(key, libs);
}

/*
 * register a given application to the local database. 
 * if library scan is forced or the last update of the required
 * libraries was a long time ago, determine the required libraries
 * for this application and store it under .app.name.lib
 */
int csc_scan_libs_for_app(char* appname)
{
 /* format from ld-linux.so.2 --list:
        libpthread.so.0 => /lib/libpthread.so.0 (0x4000b000)
        libc.so.6 => /lib/libc.so.6 (0x4001d000)
        /lib/ld-linux.so.2 => /lib/ld-linux.so.2 (0x80000000)
  format in csdb: .app.name.lib => "libpthread.so.0 libc.so.6 /lib/ld-linux.so.2" 
 */

 FILE* file;
 char cmd[VSIZE], key[KSIZE];
 char name[VSIZE], path[VSIZE];
 char modname[VSIZE];
 char* c;

 sprintf(key, ".app.%s.lib", appname);

 sprintf(cmd, "/lib/ld-linux.so.2 --list %s", appname);
 file = popen(cmd, "r");

 if(csdb_key_exist(key))
 	csdb_remove_from_key(key);

 while(fscanf(file, "%s %*s %s", &name, &path) == 2) {
 	/* handle lib names like: /lib/ld-linux.so.2 should be: ld-linux.so.2 */
 	c = strrchr(name, '/'); /* find last / */
 	if(c != NULL) {
 		c++; /* skip last / */
 		strcpy(modname, c);
 		strcpy(name, modname);
 	} /* if */
 	fscanf(file, "%*[^\n\t]");
#ifdef DEBUG
  	cs_log_err(CS_LOG, "name is: %s, path is: %s\n", name, path);
#endif
 	if(csdb_key_exist(key)) {
 		csdb_append_value_for_key(key, name, " ");
 	} /* if */
 	else {
 		csdb_set_value_for_key(key, name);
 	} /* else */
 } /* while */
 pclose(file);

 return 0;
}


/*
 * scan the local node for all known libraries by the dynamic linker
 */
int csc_scan_libs(void)
{
 /* format from ldconfig :
    746 libs found in cache `/etc/ld.so.cache' (version 1.7.0)
        libzvt.so.2 (libc6) => /usr/local/lib/libzvt.so.2
        libzvt.so.2 (libc6) => /opt/gnome/lib/libzvt.so.2
        libzvt.so (libc6) => /usr/local/lib/libzvt.so

    format in csdb: .lib.libzvt_so_2 => libc6 
  */

 int fd;
 struct stat attr;
 char* 	map, *start, *end, *c, *type_value;
 char line[256], name[256], type[256], path[256], key[256];

 system("/sbin/ldconfig -p > /tmp/csc.lib.tmp"); 
 fd = open("/tmp/csc.lib.tmp", O_RDONLY);
 fstat(fd, &attr);
 map = (char*) mmap(NULL, attr.st_size, PROT_READ, MAP_SHARED, fd, 0);
 close(fd);

 end = map;
 while(*end != '\t') end++;
 start = end+1;
 while(*start != (char)NULL) {
	end = start;
	while(*end != '\n') end++;
	strncpy(line, start, end-start);
	line[end-start] = '\0';
	sscanf(start, "%s %s %*s %s", name, type, path);
#ifdef DEBUG
	cs_log_err(CS_LOG, "name: %s, type: %s, path: %s\n", name, type, path);
#endif
	c=name;
	while(*c != '\0') { /* conv: libEterm.so.0 => libEterm_so_0 */
		if(*c == '.') *c='_';
		c++;
	} /* while */
	sprintf(key, ".lib.%s", name);
	if(csdb_key_exist(key) && 
	  ((csdb_get_value_for_key(key, &type_value)) >= 0)) {
	  	if((strstr(type_value, type) == NULL)) {
			csdb_append_value_for_key(key, type, " ");
			free(type_value);
		} /* if */
	} /* if */
	else
		csdb_set_value_for_key(key, type);	
	start = end+1;
 } /* while */
 munmap(map, attr.st_size);

 return 0;
}


/*
 * search all capabale nodes to execute all registered applications
 * under .app
 */
int csc_update_cap_nodes_for_apps(void)
{
 char** apps;
 char** app;
 int nr_apps;

 nr_apps = csdb_get_child_names_for_key(".app", &apps, NULL);

 if(nr_apps <= 0)
	return -1;
 app = apps;
 while(*app != NULL) {
#ifdef DEBUG
	cs_log_err(CS_LOG, "app is: %s\n", *app);
#endif
	csc_update_cap_nodes_for_app(*app);
	app++;
 } /* while */

 cs_free_ppc(apps);
 
 return 0;
}


/*
 * search all capable nodes which can execute a given application
 */
int csc_update_cap_nodes_for_app(char* app)
{
 char* alive_nodes;
 char* node;
 char* libs_for_app;
 char key[KSIZE];
 int nr_nodes;
 int error;
 char delims[] = " ";

 libs_for_app = NULL;
 /*libs_for_app = csc_libs_for_app(app, 1);*/
 
 csc_get_libs_for_app(app, &libs_for_app);
 if(libs_for_app == NULL)
	return -1;

 nr_nodes = csdb_get_child_names_for_key(".alive", NULL, &alive_nodes);
#ifdef DEBUG
 cs_log_err(CS_LOG, "nr: %d\n", nr_nodes);
#endif
 sprintf(key, ".app.%s.capnodes", app);

 csdb_remove_from_key(key);

 error = 0;
 if(nr_nodes > 0 && libs_for_app != NULL) {
        node = strtok(alive_nodes, delims);
        while(node != NULL) {
		error = cs_do_cap_request_for_node(node, libs_for_app);
                if(error > 0) { 
                        if(csdb_key_exist(key))
                                csdb_append_value_for_key(key, node, " ");
                        else 
                                csdb_set_value_for_key(key, node);
                } /* if */
		if(error == -1) {
			node = NULL;
		}
		else {
                	node = strtok(NULL, delims);
		} /* else */
        } /* while */
 } /* if */
 
 free(alive_nodes);
 free(libs_for_app);
 
 return 0;
}


/*
 * read a file from /proc, alloc memory and modify the value pointer.
 * the caller is responsible to free the space.
 */
int csc_get_procval(char* filename, char** value)
{
 int fd, len;
 char* buffer;
 char* l_value;

 fd = open(filename, O_RDONLY);
 if(fd == -1)
	return -1;

 buffer = (char*) malloc(4096);
 len = read(fd, buffer, 4096);
 close(fd); 
 
 l_value = (char*) malloc(len+1);
 strncpy(l_value, buffer, len+1);
 l_value[len-1] = 0;
 free(buffer);
 
 *value = l_value;

#ifdef DEBUG
 cs_log_err(CS_LOG, "value is: %s\n", l_value);
#endif

 return 0;
}


/*
 * set load of local node to the database
 */
int csc_set_load(void)
{
 /* values to set here:
   	.node.name.load.avg1	=> load in last minute
    	.node.name.load.avg5	=> load in last 5 minutes
	.node.name.load.avg15	=> load in last 15 minutes
	.node.name.load.nop	=> number of all processes
	.node.name.load.active	=> number of running processes
	.node.name.load.lastpid	=> the last pid 
 */

 char* loadavg; /* loadavg from /proc/loadavg: 3.81 2.96 1.65 2/91 28094 */
 char* key;
 char savg1[20], savg5[20], savg15[20], srun[20], snop[20], spid[20];
 char* hostname;

 if(csc_get_procval("/proc/loadavg", &loadavg) != 0)
	return -1;
#ifdef DEBUG
 cs_log_err(CS_LOG, "loadavg: %s\n", loadavg);
#endif
 key = (char*) malloc(256*sizeof(char)); 
 hostname = getenv("HOSTNAME");

 sscanf(loadavg, "%s %s %s %[^/]%*1[^ ]%s %s", savg1, savg5, savg15, srun, snop, spid);
#ifdef DEBUG
 cs_log_err(CS_LOG, "csc_set_load: %s %s %s %s %s %s \n", savg1, savg5, savg15, srun, snop, spid);
#endif
 sprintf(key,".node.%s.load.avg1", hostname);
 csdb_set_value_for_key(key, savg1);
 
 sprintf(key,".node.%s.load.avg5", hostname);
 csdb_set_value_for_key(key, savg5);

 sprintf(key,".node.%s.load.avg15", hostname);
 csdb_set_value_for_key(key, savg15);

 sprintf(key,".node.%s.load.nop", hostname);
 csdb_set_value_for_key(key, snop);

 sprintf(key,".node.%s.load.active", hostname);
 csdb_set_value_for_key(key, srun);

 sprintf(key,".node.%s.load.lastpid", hostname);
 csdb_set_value_for_key(key, spid);

 free(loadavg);
 free(key);

 return 0;
}


/*
 * set uptime values of local node to the database
 */
int csc_set_uptime(void)
{
 /* values to set here:
	.node.name.uptime.sreboot	=> seconds since reboot
	.node.name.uptime.idle		=> seconds in idle since reboot
 */

 char* uptime; /* uptime format from /proc/uptime: 12115.97 10743.37 */
 char* key;
 char sreboot[20], idle[20];
 char* hostname;

 if(csc_get_procval("/proc/uptime", &uptime) != 0)
	return -1;
#ifdef DEBUG
 cs_log_err(CS_LOG, "uptime: %s\n", uptime);
#endif
 key = (char*) malloc(KSIZE*sizeof(char));
 hostname = getenv("HOSTNAME");

 sscanf(uptime, "%s %s", sreboot, idle);
#ifdef DEBUG
 cs_log_err(CS_LOG, "csc_set_uptime: %s %s\n", sreboot, idle);
#endif

 sprintf(key,".node.%s.uptime.sreboot", hostname);
 csdb_set_value_for_key(key, sreboot);

 sprintf(key,".node.%s.uptime.idle", hostname);
 csdb_set_value_for_key(key, idle);

 free(uptime);
 free(key); 

 return 0;
}


/*
 * set memory values of the local node to the database
 */
int csc_set_mem(void)
{
 /* values to set here:
	.node.name.mem.total
	.node.name.mem.used
	.node.name.mem.free
	.node.name.mem.shared
	.node.name.mem.buffers
	.node.name.mem.cached
	.node.name.mem.swaptotal
	.node.name.mem.swapused
	.node.name.mem.swapfree
 */

 char* mem; /* meminfo format from /proc/meminfo:
 	         total:    used:    free:  shared: buffers:  cached:
 	Mem:  260452352 59326464 201125888 10629120  5427200 13602816
 	Swap: 131600384  4325376 127275008
 	MemTotal:    254348 kB
 	MemFree:     196412 kB
 	MemShared:    10380 kB
 	Buffers:       5300 kB
 	Cached:       13284 kB
 	SwapTotal:   128516 kB
 	SwapFree:    124292 kB
 	*/
 char* key;
 char total[20], used[20], free_mem[20], shared[20], buffers[20], 
      cached[20], swaptotal[20], swapused[20], swapfree[20];
 char* hostname;

 if(csc_get_procval("/proc/meminfo", &mem) != 0)
	return -1;
#ifdef DEBUG
 cs_log_err(CS_LOG, "mem: %s\n", mem);
#endif
 key = (char*) malloc(KSIZE*sizeof(char));
 hostname = getenv("HOSTNAME");
 
 sscanf(mem, "%*[^\n] Mem: %s %s %s %s %s %s%*[^S] Swap: %s  %s %s %*[^\n]",
             total, used, free_mem, shared, buffers, cached,
             swaptotal, swapused, swapfree);
#ifdef DEBUG
 cs_log_err(CS_LOG, "csc_set_mem: %s %s %s %s %s %s %s %s %s\n",
        total, used, free_mem, shared, buffers, cached,
         swaptotal, swapused, swapfree);
#endif

 sprintf(key,".node.%s.mem.total", hostname);
 csdb_set_value_for_key(key, total);

 sprintf(key,".node.%s.mem.used", hostname);
 csdb_set_value_for_key(key, used);

 sprintf(key,".node.%s.mem.free", hostname);
 csdb_set_value_for_key(key, free_mem);

 sprintf(key,".node.%s.mem.shared", hostname);
 csdb_set_value_for_key(key, shared);

 sprintf(key,".node.%s.mem.buffers", hostname);
 csdb_set_value_for_key(key, buffers);

 sprintf(key,".node.%s.mem.cached", hostname);
 csdb_set_value_for_key(key, cached);

 sprintf(key,".node.%s.mem.swaptotal", hostname);
 csdb_set_value_for_key(key, swaptotal);

 sprintf(key,".node.%s.mem.swapused", hostname);
 csdb_set_value_for_key(key, swapused);

 sprintf(key,".node.%s.mem.swapfree", hostname);
 csdb_set_value_for_key(key, swapfree);

 free(mem);
 free(key);

 return 0;
}


/*
 * set cpu relevant values to the local database
 */
int csc_set_cpu(void)
{
 /* values to set here:
	.node.name.cpu.nrcpu	=> number of cpu's
	.node.name.cpu.all.user	=> jiffies in user-mode for all cpu's
	.node.name.cpu.all.nice	=> jiffies in nice-mode for all cpu's
	.node.name.cpu.all.sys	=> jiffies in sys-mode  for all cpu's
	.node.name.cpu.all.idle	=> jiffies in idle-mode for all cpu's
	.node.name.cpu.0	=> same for the first cpu
	.node.name.cpu.1	=> same for the second cpu if we have SMP
	.node.name.cpu.n	=> same for the n'th cpu if we have SMP
 */

 char* cpu; /* stat from from /proc/stat:
		cpu  26923 0 42114 1047365
		cpu0 13564 0 22142 522495
		cpu1 13359 0 19972 524870
		... (no relevant trailing info)
	     */
 char* key;
 char cpuname[20], user[20], nice[20], sys[20], idle[20];
 char* hostname;
 char* cpu_ptr;
 char* cpu_ptr_end;
 char cpu_string[5];
 unsigned nr_cpu;
 char nr_cpu_s[4];

 if(csc_get_procval("/proc/stat", &cpu) != 0)
	return -1;
#ifdef DEBUG
 cs_log_err(CS_LOG, "cpu: %s\n", cpu);
#endif

 key = (char*) malloc(KSIZE*sizeof(char));  
 hostname = getenv("HOSTNAME");
 cpu_ptr = cpu;
 nr_cpu = 0;

 /* determine how many cpu's we have */
 while( cpu_ptr = strstr(cpu_ptr, "cpu"), cpu_ptr != NULL) {
 	cpu_ptr_end = strchr(cpu_ptr, '\n');
 	*cpu_ptr_end = 0;
 	sscanf(cpu_ptr, "%s %s %s %s %s", cpuname, user, nice, sys, idle);
#ifdef DEBUG
 cs_log_err(CS_LOG, "cpu_ptr: %s %s %s %s %s\n", cpuname, user, nice, sys, idle);
#endif
 	cpu_ptr = cpu_ptr_end+1;
	if(strlen(cpuname) <= 3) /* we have all cpu stats */
		strcpy(cpu_string, "all");
	else {
		sprintf(cpu_string, "%d", nr_cpu);
		nr_cpu++;
	} /* else */
	sprintf(key, ".node.%s.cpu.%s.user", hostname, cpu_string);
	csdb_set_value_for_key(key, user);

	sprintf(key, ".node.%s.cpu.%s.nice", hostname, cpu_string);
	csdb_set_value_for_key(key, nice);

	sprintf(key, ".node.%s.cpu.%s.sys", hostname, cpu_string);
	csdb_set_value_for_key(key, sys);

	sprintf(key, ".node.%s.cpu.%s.idle", hostname, cpu_string);
	csdb_set_value_for_key(key, idle);
 } /* while */

 /* no SMP box is detected, CPU counting will fail */
 if(nr_cpu == 0) {
	nr_cpu = 1;
 } /* if */

 sprintf(key, ".node.%s.cpu.nrcpu", hostname);
 sprintf(nr_cpu_s, "%d", nr_cpu);
 csdb_set_value_for_key(key, nr_cpu_s);

 free(key);
 free(cpu);

 return 0;
}


/*
 * set all dynamic values
 */
void csc_set_dynamic_values(void)
{
 	csc_set_load();
	csc_set_uptime();
	csc_set_mem();
	csc_set_cpu(); 
}
