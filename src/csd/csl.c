/*  csl.c is part of dkm
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

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#include "csl.h"

pthread_mutex_t cs_is_logging = PTHREAD_MUTEX_INITIALIZER;


/*
 * some logging and error message functions
 */
void cs_log_msg(int on_syslog, const char* format_s, va_list argv)
{
 char msg[CS_MAX_LOG_MSG];

 pthread_mutex_lock(&cs_is_logging);
 vsnprintf(msg, CS_MAX_LOG_MSG, format_s, argv);
 
 if(on_syslog) {
        syslog( LOG_INFO | LOG_DAEMON, msg);
 } /* if */
 else {
        fflush(stderr);
        fputs(msg, stderr);
        fflush(stderr);
 } /* else */
 pthread_mutex_unlock(&cs_is_logging);
}

void cs_log_exit(int on_syslog, const char* format_s, ...)
{
 va_list argv;
 va_start(argv, format_s);
 cs_log_msg(on_syslog, format_s, argv);
 va_end(argv);
 exit(-1);
}

void cs_log_err(int on_syslog, const char* format_s, ...)
{
 va_list argv;
 va_start(argv, format_s);
 cs_log_msg(on_syslog, format_s, argv);
 va_end(argv);
}

