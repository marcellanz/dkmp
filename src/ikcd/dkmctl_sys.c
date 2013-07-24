/*  csd.c is part of dkm
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

#include <linux/unistd.h>
#include <linux/types.h>
#include <linux/dkm.h>

/*
 * library function to call the new systemcall dkmctl
 */
_syscall4(int, dkmctl, int, cmd, pid_t, pid, int, req, struct req_argv*, rq_argv)
