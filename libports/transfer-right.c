/* Transfer the receive right from one port structure to another
   Copyright (C) 1996 Free Software Foundation, Inc.
   Written by Michael I. Bushnell, p/BSG.

   This file is part of the GNU Hurd.

   The GNU Hurd is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   The GNU Hurd is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA. */


#include "ports.h"
#include <hurd/ihash.h>

error_t
ports_transfer_right (void *tostruct, 
		      void *fromstruct)
{
  struct port_info *topi = tostruct;
  struct port_info *frompi = fromstruct;
  mach_port_t port;
  int dereffrompi = 0;
  int dereftopi = 0;
  int hassendrights = 0;
  error_t err;

  mutex_lock (&_ports_lock);

  /* Fetch the port in FROMPI and clear its use */
  port = frompi->port_right;
  if (port != MACH_PORT_NULL)
    {
      ihash_locp_remove (frompi->bucket->htable, frompi->hentry);
      frompi->port_right = MACH_PORT_NULL;
      if (frompi->flags & PORT_HAS_SENDRIGHTS)
	{
	  frompi->flags &= ~PORT_HAS_SENDRIGHTS;
	  hassendrights = 1;
	  dereffrompi = 1;
	}
    }
  
  /* Destroy the existing right in TOPI. */
  if (topi->port_right != MACH_PORT_NULL)
    {
      ihash_locp_remove (topi->bucket->htable, topi->hentry);
      err = mach_port_mod_refs (mach_task_self (), topi->port_right,
				MACH_PORT_RIGHT_RECEIVE, -1);
      if ((topi->flags & PORT_HAS_SENDRIGHTS) && !hassendrights)
	{
	  dereftopi = 1;
	  topi->flags &= ~PORT_HAS_SENDRIGHTS;
	}
      else if (((topi->flags & PORT_HAS_SENDRIGHTS) == 0) && hassendrights)
	{
	  topi->flags |= PORT_HAS_SENDRIGHTS;
	  topi->refcnt++;
	}
    }
  
  /* Install the new right in TOPI. */
  topi->port_right = port;
  topi->cancel_threshold = frompi->cancel_threshold;
  topi->mscount = frompi->mscount;
  
  if (port)
    {
      ihash_add (topi->bucket->htable, port, topi, &topi->hentry);
      if (topi->bucket != frompi->bucket)
	mach_port_move_member (mach_task_self (), port, topi->bucket->portset);
    }
  
  mutex_unlock (&_ports_lock);
  
  /* Take care of any lowered reference counts. */
  if (dereffrompi)
    ports_port_deref (frompi);
  if (dereftopi)
    ports_port_deref (topi);
  return 0;
}


  
