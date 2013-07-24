/*  
*    csdb.c: CS-Database
*    Copyright (C) 1999-2000  
*     Marcel Lanz <marcel.lanz@ds9.ch>, 
*     University of Applied Sciences Solothurn Northwest Switzerland
*    Copyright (C) 2000
*     Marcel Lanz <marcel.lanz@dkmp.org>
*
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


/*
 * csdb.o, capability and system information database
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csdb.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: csdb.c,v 0.5 2000/07/22 11:11:41 lanzm Exp lanzm $";

#include "csdb.h"
#include "csl.h"
#include "csd.h"		/* for cs_free_ppc() */
#include "../include/cs.h"

struct csdbe* csdb_root; 	/* this is the root entry of the database */
pthread_mutex_t csdb_root_mutex = PTHREAD_MUTEX_INITIALIZER;


/*
 * get the modification time for the given key
 */
time_t csdb_get_mtime_for_key(char* key)
{
 struct csdbe* entry;
 time_t mtime;
 mtime = 0;
 pthread_mutex_lock(&csdb_root_mutex);
 entry = csdb_get_entry(key); 
 if(entry != NULL)
 	mtime = entry->mtime;
 pthread_mutex_unlock(&csdb_root_mutex);

 return mtime;
}


/*
 * find and return a child entry for the given parent entry
 * and the given name
 */
struct csdbe* csdb_find_child(struct csdbe* root, char* name)
{
#ifdef CSDB_LINEAR_SEARCH
 struct csdbe* sear_ptr;
 sear_ptr = root->childs;
#endif

#ifdef CSDB_BINARY_SEARCH
 unsigned int l;
 unsigned int u;
 unsigned int i;
 int cmp;
 struct csdbe* to_find;
#endif


 if(strcmp(name, ".") == 0)
	return csdb_root; /* "." is not really an entry but... */

#ifdef CSDB_LINEAR_SEARCH
 while(sear_ptr != NULL) {
	if(strcmp(sear_ptr->name, name) == 0)
		return sear_ptr;
	else
		sear_ptr = sear_ptr->next;
 } /* while */
#endif

#ifdef CSDB_BINARY_SEARCH
 /* if no child is present */
 if(root->childs == NULL) {
	return (struct csdbe*) NULL;
 } /* if */

 l = 1;
 u = root->nbr_entries;

b2:
 i = (int)((l+u)/2); /* the approx. middle */
 to_find = csdb_seek(root->childs, i);

 if(u<l) {
  goto out;
 } /* if */
 cmp = strcmp(name, to_find->name);
 if(cmp < 0)
        goto b4;
 if(cmp > 0)
        goto b5;
 if(cmp == 0)
	return to_find;

b4:
 u = i-1;
 goto b2;
b5:
 l = i+1;
 goto b2;

out:
 return (struct csdbe*) NULL;

#endif


 return (struct csdbe*) 0;
} 


/*
 * check if a given key is syntactic valid
 */
int csdb_is_valid_key(char* key)
{
 int ret;

 if(key[0] == '.' && key[strlen(key)-1] != '.')
	ret = 1;
 else
	ret = 0;
 if(key[0] == '.' && strlen(key) == 1) /* root key */
	ret = 1;

 return ret;
}


/*
 * check a given key if its is valid and count the number
 * of key-parts
 */
int csdb_check_key(char* key, int* nof_delims)
{
 char* c; 		/* this char to parse */
 *nof_delims = 0;

 c = key;
 while(*c != '\0') { /* count nof delimiter */
	if(*c == '.')
		*nof_delims+=1;
	c++;
 } /* while */
 if(*nof_delims == 0 || !csdb_is_valid_key(key))
	return -1;
 else 
	return 0;
}


/*
 * parse a given key and return the key-parts as a pointer to pointers
 * of charaacters. the caller is responsible to free the allocated memory
 */
char** csdb_parse_key(char* key)
{
 char* c; 		/* this char to parse */
 char* begin; 		/* begin of a token */
 int nof_delims; 	/* number of delimiter */
 int state; 		/* 0 is not in token, 1 is in token */
 char** keys; 		/* parsed key */
 char** next_key; 	/* next key to insert */
 char* name_new;	/* name allocated for every key-part */

 state = 0; /* not in token */
 c = key;
 csdb_check_key(key, &nof_delims);
 if(nof_delims <= 0)
	return (char**) NULL;

 keys = (char**) malloc((nof_delims+1) * sizeof(char*));
 memset(keys, 0, (nof_delims+1) * sizeof(char*));
 next_key = keys;

 if(nof_delims == 1 && strlen(key) == 1) {
	*next_key = (char*) malloc(2*sizeof(char));
	strncpy(*next_key, key, 2*sizeof(char));
	return keys;
 } /* if */

 c = key;
 begin = key;
 while(*c != '\0') {	/* a state maschine to parse the key */
	switch(state) {
	case 0:	/* not in token */
		if(*c == '.') {
			c++;
			begin++;
			state=1;
		}
		break;
	case 1: /* in token */
		if(*c != '.') {
			c++;
		}
		if(*c == '.' || *c== '\0') { /* token end detected */
			name_new = (char*) malloc(c-begin+1);
			strncpy(name_new, begin, c-begin);
			name_new[c-begin]='\0';
			*next_key = name_new;
			next_key++;
			begin=c;
			state=0;
#ifdef DEBUG
			cs_log_err(CS_LOG, "name is: %s\n", name_new);
#endif
		} /* if */
		break;
	} /* switch */
 } /* while */
 return keys; /* the caller is responsible to free the memory of keys */
}


/*
 * check if a given key exists in the database
 */
int csdb_key_exist(char* key)
{
 unsigned exist;

 pthread_mutex_lock(&csdb_root_mutex);
 exist = csdb_get_entry(key) != NULL;
 pthread_mutex_unlock(&csdb_root_mutex);

 return exist;
}


/*
 * search and return the entry for a given key
 */
struct csdbe* csdb_get_entry(char* key)
{
 struct csdbe* entry;           /* the entry currently on list */
 char** keys;
 char** name;                   /* current name of our keys */

 keys = csdb_parse_key(key);
 name = keys;
 entry = csdb_root;
  
 while(entry && *name) {
        /* find an entry with name and set the entry to them */
        entry = csdb_find_child(entry, *name); 
        name++;
 } /* while */

 cs_free_ppc(keys);

 return entry;
}


/*
 * remove this and all child entries of the given key
 */
int csdb_remove_from_key(char* key)
{
 int ret;
 struct csdbe* root;

 if( strcmp(key, ".") == 0) /* we can't delete the root entry */
	return -1;

 pthread_mutex_lock(&csdb_root_mutex);
 root = csdb_get_entry(key);
 if(root == NULL) {
	pthread_mutex_unlock(&csdb_root_mutex);
	return -ECS_NOENTRY;
 } /* if */
 ret = __csdb_remove_from_entry(root);

 if(root->prev) /* remove this entry and handle if other childs are present */
 	root->prev->next = root->next;
 else 
	root->parent->childs = root->next;

#ifdef CSDB_BINARY_SEARCH
 root->parent->nbr_entries -= 1;
#endif

 if(root->next)
	root->next->prev = root->prev;
 if(root->name)
	free(root->name);
 if(root->value)
	free(root->value);
 free(root);

 pthread_mutex_unlock(&csdb_root_mutex);

 return ret;
}


/* 
 * helper function for csdb_remove_from_key
 * use this function only thread-safe
 */
int __csdb_remove_from_entry(struct csdbe* root)
{
 struct csdbe* entry; 	/* the entry */

 entry = root->childs;

 while(entry) {
	if(entry->childs)
		__csdb_remove_from_entry(entry); /* recursive */
	if(entry->name != NULL)
		free(entry->name);
	if(entry->value != NULL)
		free(entry->value);
	entry = entry->next;
	if(entry && entry->prev)
		free(entry->prev);
 } /* while */

 return 0;
}


/*
 * dump all entries from the given key with key and value to a given filedescriptor
 */
int csdb_dump_from_key(char* key, int fd)
{
 struct csdbe* entry;
 int ret_val;

 pthread_mutex_lock(&csdb_root_mutex);
 entry = csdb_get_entry(key);
 if(entry != NULL)
	ret_val = csdb_dump_from_entry(entry->childs, fd);
 else
	ret_val = 0;

 pthread_mutex_unlock(&csdb_root_mutex);

 return  ret_val;
}


/*
 * helper function for csdb_dump_from_key
 */
int csdb_list_parents_names(int fd, struct csdbe* from)
{
 if(from->name != csdb_root->name)
	csdb_list_parents_names(fd, from->parent);
 if(from != csdb_root) {
	write(fd, ".", 1);
 	write(fd, from->name, strlen(from->name));
 } /* if */

 return 0;
}


/*
 * helper function for csdb_dump_from_key
 */
int csdb_dump_from_entry(struct csdbe* entry, int fd)
{
 struct csdbe* root;

 root = entry;
 while(root != NULL) {
	if(root->is_leaf) {
		csdb_list_parents_names(fd, root);
		write(fd, "=", 1);
		if(root->value != NULL)
			write(fd, root->value, strlen(root->value));
		else
			write(fd, "(null)", 6);
		write(fd, "\n", 1);
	} /* if */
	if(root->childs != NULL)
		csdb_dump_from_entry(root->childs, fd);
	root = root->next;
 } /* while */

 return 0;
}


/*
 * get the value for the given key, copy entry->value to a buffer allocated by the function.
 * the caller is responsible to free the memory.
 */
int csdb_get_value_for_key(char* key, char** val)
{
 struct csdbe* entry;
 char* value;
 int ret;

 ret = 0;
 if(!csdb_is_valid_key(key)) {
	*val = (char*)0;
	return -ECS_INVALIDKEY;
 } /* if */

 pthread_mutex_lock(&csdb_root_mutex);
 entry = csdb_get_entry(key);

 if(entry == NULL) {
	*val = (char*)0;
	return -ECS_INVALIDKEY;
 } /* if */
 else if(!entry->is_leaf) {
	*val = (char*)0;
	return -ECS_NOLEAF;
 } /* else if */
 else {
	value = (char *) malloc(strlen(entry->value) * sizeof(char)+1);
	strcpy(value, entry->value);
	*val = value;
	ret = 0;
 } /* else */
 pthread_mutex_unlock(&csdb_root_mutex);

 return ret;
}


/*
 * return the names of the childs for a given key as string separated by a space or
 * a list of pointers which points to the names.
 * the caller is responsible to free allocated memory.
 */
int csdb_get_child_names_for_key(char* key, char*** child_names, char** child_names_as_string)
{
 int nrchilds, nchars, name_len;
 struct csdbe* entry, *sear_entry;
 char** names;
 char** new_name;
 char* names_as_string, *next_name;

 entry = csdb_get_entry(key);
 if(entry == NULL || entry->is_leaf)
	return -1;
 entry = entry->childs;
 if(entry == NULL)
	return -1;	/* no childs */

 sear_entry = entry;
 nrchilds = 0;
 nchars = 0;
 while(sear_entry != NULL) {
	nchars+=strlen(sear_entry->name)+1;
	sear_entry = sear_entry->next;
	nrchilds++;
 } /* while */

 if(child_names != NULL) {
 	names = (char**) malloc((nrchilds+1)*sizeof(char**));
 	sear_entry = entry;
 	new_name = names;
 	while(sear_entry != NULL) {
		*new_name = (char*) malloc((strlen(sear_entry->name)+1)*sizeof(char));
		strcpy(*new_name, sear_entry->name);
		new_name++;
		sear_entry = sear_entry->next;
 	} /* while */
	*new_name = NULL;
	*child_names = names;
 } /* if */
 if(child_names_as_string != NULL) {
	names_as_string = (char*) malloc((nchars+1)*sizeof(char));
	next_name = names_as_string;
	sear_entry = entry;
	while(sear_entry != NULL) {
		name_len = strlen(sear_entry->name);
		strncpy(next_name, sear_entry->name, name_len);
		next_name+=name_len;
		*next_name++ = ' ';
		sear_entry = sear_entry->next;
	} /* while */
	*next_name = 0;
 	*child_names_as_string = names_as_string;
#ifdef DEBUG
 cs_log_err(CS_LOG, "names_as_string is: %s\n", names_as_string);
#endif
 } /* if */

 return nrchilds;
}


/*
 * set the name of a given entry
 */
int csdb_set_name(struct csdbe* entry, char* name)
{
 int len;
 if(name != NULL) {
 	len = strlen(name);
	if(entry->name != NULL)
		free(entry->name);
 	entry->name = (char*) malloc(len*sizeof(char)+1);
 	strncpy(entry->name, name, len+1);
	entry->is_leaf = 0;
 }
 return 0;
}


/*
 * set the value for a given entry
 */
int csdb_set_value(struct csdbe* entry, char* value)
{
 int len;
 if(value != NULL) {
 	len = strlen(value);
	if(entry->value != NULL) {
		free(entry->value); 
	} /* if */
 	entry->value = (char*) malloc(len*sizeof(char)+1);
 	strncpy(entry->value, value, len+1);
	entry->is_leaf = 1;
 } /* if */
 else {
	if(entry->value != NULL)
		free(entry->value);
	entry->value = NULL;
 } /* else */
 entry->mtime = time(NULL);

 return 0;
}


/*
 * seek on a entry list and return
 * the target entry
 * if the offset is too wide, return the last entry
 */
struct csdbe* csdb_seek(struct csdbe* entry, unsigned int hops)
{
 struct csdbe* hop_entry;
 unsigned int hopsi;

 hop_entry = entry;
 if(hops < 0)
	return hop_entry;
 
 for(hopsi=1; hopsi < hops && hop_entry != NULL && hop_entry->next != NULL; hopsi++)
	hop_entry = hop_entry->next;
  
 return hop_entry;
}

/*
 * append a entry to the child list
 */
int csdb_append_child(struct csdbe* entry, struct csdbe* child) 
{
#ifdef CSDB_BINARY_SEARCH
 unsigned int l;
 unsigned int u;
 unsigned int i;
 int cmp;
 struct csdbe* to_find;
#endif

#ifdef CSDB_LINEAR_SEARCH
 child->next = entry->childs;
 child->prev = NULL;
 child->parent = entry;
 if(child->next)
	child->next->prev=child;
 entry->childs = child;
#endif

#ifdef CSDB_BINARY_SEARCH
 /* if no child is present */
 if(entry->childs == NULL) {
	entry->childs = child;
	child->prev = NULL;
	child->next = NULL;
	child->parent = entry;
 	entry->nbr_entries += 1;
	goto out;
 } /* if */

 l = 1;
 u = entry->nbr_entries;

b2:
 i = (int)((l+u)/2); /* the approx. middle */
 to_find = csdb_seek(entry->childs, i);

 if(u<l) {
	to_find = csdb_seek(entry->childs, l);
	if(l>entry->nbr_entries) { /* it's the last entry */
		child->next = to_find->next;
		child->prev = to_find;
		to_find->next = child;
	} /* if */
	else {
		child->next = to_find;
		child->prev = to_find->prev;
		if(to_find->prev != NULL)
			to_find->prev->next = child;
		to_find->prev = child;
	if(entry->childs == to_find)
		entry->childs = child;
	} /* else */
	child->parent = entry;

  entry->nbr_entries += 1;
  goto out;
 } /* if */
 cmp = strcmp(child->name, to_find->name);
 if(cmp < 0)
	goto b4;
 if(cmp > 0)
	goto b5;
 if(cmp == 0)
	goto out; /* the entry exists */

b4:
 u = i-1;
 goto b2;
b5:
 l = i+1;
 goto b2;

out:
 return 0;
#endif

#ifdef CSDB_BINARY_SEARCH_XX
b1: 
 l = 1;
 u = entry->parent->nbr_entries;
 to_find = entry->childs;
 if(to_find == NULL) {
	entry->childs = child;
	child->prev = NULL;
	child->next = NULL;
	child->parent = entry;
	goto out;
 } /* if */

b2:
 i = (int)((l+u)/2); /* the approx. middle */
 to_find = csdb_seek(entry->childs, i);
 cmp_i = strcmp(to_find->name, child->name);
 if(to_find->next != NULL)
 	cmp_i_next = strcmp(to_find->next->name, child->name);
 else
	cmp_i_next = -1;
 if(to_find->prev != NULL)
	cmp_i_prev = strcmp(to_find->prev->name, child->name);
 else
	cmp_i_prev = 1;

 if(cmp_i > 0 && cmp_i_next < 0) { /* found */
	child->next = to_find->next;
	child->prev = to_find;
	to_find->next = child;
	child->parent = entry;
	goto out;
 } /* if */
 else if(cmp_i < 0 && cmp_i_prev > 0) { /* found */
	child->next = to_find;
	child->prev = to_find->prev;
	child->parent = entry;
	if(to_find->prev == NULL)
		entry->childs = child;
	goto out;
 } /* else if */

 if(cmp_i < 0)
	u = i-1;
 else if(cmp_i > 0)
	l = i+1;

 goto b2;

out:
 	child->parent->nbr_entries += 1;
#endif

 return 0;
}


/*
 * create a new entry for given name and value
 */
struct csdbe* csdb_create_entry(char* name, char* value)
{
 struct csdbe* csdbe_new;
 csdbe_new = (struct csdbe*) malloc(sizeof(struct csdbe));
 memset(csdbe_new, 0, sizeof(struct csdbe));
 csdb_set_name(csdbe_new, name);
 csdb_set_value(csdbe_new, value);
#ifdef CSDB_BINARY_SEARCH
 csdbe_new->nbr_entries = 0;
#endif
 
 return csdbe_new;
}


/*
 * append a value to the value of an existing key
 */
int csdb_append_value_for_key(char* key, char* value, char* btwn)
{
 struct csdbe* entry;
 int err, val_len, btwn_len;
 char* more_space;

 pthread_mutex_lock(&csdb_root_mutex);
 entry = csdb_get_entry(key);
 if(entry == NULL || value == NULL)
	err = -1;
 else if(!entry->is_leaf)
	err = -1;
 else {
	if(entry->value == NULL) {
		csdb_set_value(entry, value);
		err = 0;
	} /* if */
	else {
		val_len = strlen(value);
		if(btwn != NULL) 
			btwn_len = strlen(btwn);
		else	
			btwn_len=0;
		more_space = (char*) realloc(entry->value, strlen(entry->value)+val_len+btwn_len+1);
		if(more_space == NULL) /* FIXME: return here */
			err = -1;
		else
			entry->value = more_space;
		if(btwn_len > 0)
                        strcat(entry->value, btwn);
		strcat(entry->value, value);
		err = 0;
	} /* else */
 } /* else */
 pthread_mutex_unlock(&csdb_root_mutex);

 return err;
}


/*
 * set the value for a given key
 */
int csdb_set_value_for_key(char* key, char* value)
{
 char** keys;                           /* the key parts */
 char** name;                           /* a key part */
 struct csdbe *entry, *new_entry;       /* the entry on we are and a new entry */

 keys = csdb_parse_key(key);
 if(keys == NULL)
	return -1;
 name = keys;
 pthread_mutex_lock(&csdb_root_mutex);
 entry = csdb_root;

 while(*name != NULL) {
	new_entry = csdb_find_child(entry, *name);
        if( new_entry == 0) { /* the entry don't exist yet */
                new_entry = csdb_create_entry(*name, NULL);
                csdb_append_child(entry, new_entry);
                entry = new_entry;
        } /* if */
	else {
		entry = new_entry;
	}
        name++;
        if(*name == NULL) { /* we are on the last key part, it holds the value*/
        	csdb_set_value(entry, value);
	} /* if */
 } /* while */

 cs_free_ppc(keys);
 pthread_mutex_unlock(&csdb_root_mutex);
 
 return 0;
}


/*
 * initialize the database
 */
int csdb_init(void)
{
 pthread_mutex_lock(&csdb_root_mutex);
 csdb_root = csdb_create_entry(".", NULL);
 csdb_root->parent = csdb_root;
 pthread_mutex_unlock(&csdb_root_mutex);

 return 0;
}


/*
 * database test function
 */
int csdb_test2(void)
{
 csdb_set_value_for_key(".lib", "1");
 csdb_set_value_for_key(".app", "1");
 csdb_set_value_for_key(".node", "1");
 csdb_set_value_for_key(".conf", "1");
 csdb_set_value_for_key(".e", "1");
 csdb_set_value_for_key(".f", "1");
 csdb_set_value_for_key(".f.l", "1");
 csdb_dump_from_key(".", 1);

 return 0;
}
int csdb_test(void)
{
 struct csdbe* entry;

 csdb_set_value_for_key(".node.prometheus.loadavg", "0.00 0.00 0.00 1/25 849");
 csdb_set_value_for_key(".node.defiant.loadavg", "0.02 0.08 0.08 1/88 28122");
 csdb_set_value_for_key(".node.defiant.loadavg", "0.02 0.08 0.08 1/88 28128");
 csdb_set_value_for_key(".node.defiant.uptime", "29321.90 26639.91");
 csdb_dump_from_key(".", 1);
 entry = csdb_get_entry(".node.defiant.loadavg");
 entry = csdb_get_entry(".node.dfiant.loadavg");
 entry = csdb_get_entry(".");
 /*printf("value for key: .node.defiant.loadavg is: %s\n", csdb_get_value_for_key(".node.defiant.loadavg", NULL));
 printf("value for key: .node.defiant.loadavg is: %s\n", csdb_get_value_for_key(".node.defiant.loadvg", NULL));*/
 csdb_remove_from_key(".node.defiant.loadavg");
 csdb_remove_from_key(".node.defiant");

 return 0;
}
