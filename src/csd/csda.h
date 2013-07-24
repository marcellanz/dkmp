#ifndef _CSDA_H
#define _CSDA_H
/*
 * csda.h, capability and system information distribution arbiter header file
 * File: $Source$
 * Author: Marcel Lanz
 *
 * Revision control:
 * -----------------
 * $Id$ */


/*
 * best node request
 */
struct bn_request {
	key_t mtype;
	char msg[257];
};


/*
 * best node response
 */
struct bn_response {
	key_t mtype;
	char msg[65];
};

/*
 * function declaration
 */

void csda_best_node_server_thread(void);
int csda_register_app(char* app);
double csda_calc_bench(char* node);
int csda_best_node_for_app(char* app_name, char** node_name);

#endif
