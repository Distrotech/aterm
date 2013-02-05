/*---------------------------------------------------------------------------*
 * File:	rmemset.c
 *---------------------------------------------------------------------------*
 * Copyright (C) 1997,1998 Geoff Wing <gcw@pobox.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*
 * Originally written:
 *    1997,1998 Geoff Wing <mason@primenet.com.au>
 *--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*
 * Fast memset()
 * presumptions:
 *   1) R_int_p_t write the best
 *   2) SIZEOF_INT_P= power of 2
 *--------------------------------------------------------------------------*/

#ifndef lint
static const char rcsid[] = "$Id: rmemset.c,v 1.1.1.1 2004/11/10 17:21:47 sasha Exp $";
#endif

#include "rxvt.h"		/* NECESSARY */

#ifdef PROTOTYPES
#define __PROTO(p)      p
#else
#define __PROTO(p)      ()
#endif
#include "rmemset.pro"

/* PROTO */
void
rmemset(void *p, unsigned char c, R_int_p_t len)
{
    R_u_int_p_t     i, val, *rp;
    unsigned char  *lp;

    if (len < 16)		/* probably not worth all the calculations */
	lp = p;
    else {
/* write out preceding characters so we align on an integer boundary */
	if ((i = ((-(R_u_int_p_t)p) & (SIZEOF_INT_P - 1))) == 0)
	    rp = p;
	else {
	    len -= i;
	    for (lp = p; i--;)
		*lp++ = c;
	    rp = (R_int_p_t *)lp;
	}

/* do the fast writing */
	val = (c << 8) + c;
#if SIZEOF_INT_P >= 4
	val |= (val << 16);
#endif
#if SIZEOF_INT_P >= 8
	val |= (val << 32);
#endif
#if SIZEOF_INT_P == 16
	val |= (val << 64);
#endif
	for (i = len / SIZEOF_INT_P; i--;)
	    *rp++ = val;
	len &= (SIZEOF_INT_P - 1);
	lp = (unsigned char *)rp;
    }
/* write trailing characters */
    for (; len--;)
	*lp++ = c;
}
