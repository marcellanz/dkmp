#ifndef _CSDB_H
#define _CSDB_H
/*
 * csdb.h, capability and system information database header file
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csdb.h,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * -----------------
 * $Id: csdb.h,v 0.1 1999/10/27 20:02:02 lanzm Exp $ */

#include <time.h>
#include <unistd.h>
#include <pthread.h>

struct csdbe {
	char* name;		/* the entrys name */
	char* value;		/* the entrys value */
	struct csdbe* childs;	/* its childs */
	struct csdbe* parent;	/* its parent */
	struct csdbe* next;	/* the next entry on the same level */
	struct csdbe* prev;	/* the previous entry on the same level */

	time_t mtime;		/* time of last modification */	
	unsigned dumpit;	/* dump the entry and its childs on shutdown */
	unsigned is_leaf;	/* its a leaf or not */
#ifdef CSDB_BINARY_SEARCH
	unsigned int nbr_entries /* number of entries a child has */
#endif
};


/* Datastructure of csdb 
 *
 *     _____   
 *    |     | csdb_root
 *    |     |
 *    |_____|
 *    /
 *   / childs
 *  /______            _____
 *  |      |---next-->|     |---next-->
 *  |      |          |     |
 *  |______|<--prev---|_____|<--prev---
 *                    /
 *                   / childs
 *                  /______           _____
 *                 |      |---next-->|     |---next-->
 *                 |      |          |     |
 *                 |______|<--prev---|_____|<--prev---
 */

/*
 * function declarations
 */ 

time_t csdb_get_mtime_for_key(char* key);
struct csdbe* csdb_find_child(struct csdbe* root, char* name);
int csdb_is_valid_key(char* key);
int csdb_check_key(char* key, int* nof_delims);
char** csdb_parse_key(char* key);
int csdb_key_exist(char* key);
struct csdbe* csdb_get_entry(char* key);
int csdb_remove_from_key(char* key);
int __csdb_remove_from_entry(struct csdbe* root);
int csdb_dump_from_key(char* key, int fd);
int csdb_list_parents_names(int fd, struct csdbe* from);
int csdb_dump_from_entry(struct csdbe* entry, int fd);
int csdb_get_value_for_key(char* key, char** val);
int csdb_get_child_names_for_key(char* key, char*** child_names, char** child_names_as_string);
int csdb_set_name(struct csdbe* entry, char* name);
int csdb_set_value(struct csdbe* entry, char* value);
int csdb_append_child(struct csdbe* entry, struct csdbe* child);
struct csdbe* csdb_create_entry(char* name, char* value);
int csdb_append_value_for_key(char* key, char* value, char* btwn);
int csdb_set_value_for_key(char* key, char* value);
int csdb_init(void);
int csdb_test(void);

#endif
