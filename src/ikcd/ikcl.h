/*
 * ikcl, inter kernel communication logging header file
 * File: $Source$
 * Author: Marcel Lanz
 *
 * Revision control:
 * -----------------
 * $Id$ */


#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

/*
 * function declaration
 */
void ikc_log_msg(int on_syslog, const char* format_s, va_list argv);
void ikc_log_exit(int on_syslog, const char* format_s, ...);
void ikc_log_err(int on_syslog, const char* format_s, ...);
