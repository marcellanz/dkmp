#ifndef _CSL_H
#define _CSL_H

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>
#include <pthread.h>

#define CS_MAX_LOG_MSG  1024

/*
 * global variables
 */


/*
 * function declaration
 */

void cs_log_msg(int on_syslog, const char* format_s, va_list argv);
void cs_log_exit(int on_syslog, const char* format_s, ...);
void cs_log_err(int on_syslog, const char* format_s, ...);

#endif
