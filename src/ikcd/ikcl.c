/*  ikcl.c is part of dkm
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
 * ikcl, inter kernel communication logging functions
 * File: $Source: /home/lanzm/data/projects/dkm/src/ikcd/RCS/ikcl.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id: ikcl.c,v 1.1 1999/12/16 22:07:49 lanzm Exp lanzm $";

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#include "../include/ikc.h"
#include "ikcp.h"
#include "ikcl.h"


/*
 * some logging and error message functions
 */
void ikc_log_msg(int on_syslog, const char* format_s, va_list argv)
{
 char msg[1024];

 vsprintf(msg, format_s, argv);
 if(on_syslog) {
        syslog( LOG_INFO | LOG_DAEMON, msg);
 } /* if */
 else {
        fflush(stderr);
        fputs(msg, stderr);
        fflush(stderr);
 } /* else */
}

void ikc_log_exit(int on_syslog, const char* format_s, ...)
{
 va_list argv;
 va_start(argv, format_s);
 ikc_log_msg(on_syslog, format_s, argv);
 va_end(argv);
 exit(-1);
}

void ikc_log_err(int on_syslog, const char* format_s, ...)
{
 va_list argv;
 va_start(argv, format_s);
 ikc_log_msg(on_syslog, format_s, argv);
 va_end(argv);
}

