/*
*    csdb.c: cs database
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
 * cst.o, cs time synchronisation
 * File: $Source: /home/lanzm/data/projects/dkm/src/csd/RCS/csdb.c,v $
 * Author: Marcel Lanz
 *
 * Revision control:
 * ----------------- */
   static char rcsId[]="$Id$";


time_t cst_get_authoritive_time(void)
{
 time_t the_time;

/*
 send cst_req
 start timer
 if t exp:
  if tries > 1
	goto a1
  else
	try again
 else
  receive cst_resp
  set local time
  set time of this action
  goto out

a1:
  try to find alternative time authorative
  by find the most recent time.
  inform other nodes to store this time
  log about this
*/

 

 return the_time;
}
