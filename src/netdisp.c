/*--------------------------------*-C-*---------------------------------*
 * File:	netdisp.c
 *----------------------------------------------------------------------*
 * Copyright (C) 1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
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
 *----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 * Originally written:
 *    1996      Chuck Blake <cblake@BBN.COM>
 * Modifications:
 *    1997      mj olesen <olesen@me.queensu.ca>
 *----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 * support for resolving the actual IP number of the host for remote
 * DISPLAYs.  When the display is local (i.e. :0), we add support for
 * sending the first non-loopback interface IP number as the DISPLAY
 * instead of just sending the incorrect ":0".  This way telnet/rlogin
 * shells can actually get the correct information into DISPLAY for
 * xclients.
 *----------------------------------------------------------------------*/

#ifndef lint
static const char rcsid[] = "$Id: netdisp.c,v 1.1.1.1 2004/11/10 17:21:46 sasha Exp $";
#endif

#include "rxvt.h"		/* NECESSARY */

/* compile an empty file if this option is disabled. */
#ifndef DISPLAY_IS_IP
/* char * network_display (const char * display) { return display; } */
#endif
#ifdef DISPLAY_IS_IP
/* On Solaris link with -lsocket and -lnsl */
#include <sys/types.h>
#include <sys/socket.h>

/* these next two are probably only on Sun (not Solaris) */
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_BYTEORDER_H
#include <sys/byteorder.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
/*}}} */
/* extern functions referenced */
/* extern variables referenced */
/* extern variables declared here */
/* local variables */
/* local functions referenced */

/*----------------------------------------------------------------------*/
/* return a pointer to a static buffer */
/* PROTO */
char           *
network_display(const char *display)
{
    static char     ipaddress[32] = "";
    char            buffer[1024], *rval = NULL;
    struct ifconf   ifc;
    struct ifreq   *ifr;
    int             i, skfd;

    if (display[0] != ':' && strncmp(display, "unix:", 5))
	return display;		/* nothing to do */

    ifc.ifc_len = sizeof(buffer);	/* Get names of all ifaces */
    ifc.ifc_buf = buffer;

    if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
	perror("socket");
	return NULL;
    }
    if (ioctl(skfd, SIOCGIFCONF, &ifc) < 0) {
	perror("SIOCGIFCONF");
	close(skfd);
	return NULL;
    }
    for (i = 0, ifr = ifc.ifc_req;
	 i < (ifc.ifc_len / sizeof(struct ifreq));

	 i++, ifr++) {
	struct ifreq    ifr2;

	STRCPY(ifr2.ifr_name, ifr->ifr_name);
	if (ioctl(skfd, SIOCGIFADDR, &ifr2) >= 0) {
	    unsigned long   addr;
	    struct sockaddr_in *p_addr;

	    p_addr = (struct sockaddr_in *)&(ifr2.ifr_addr);
	    addr = htonl((unsigned long)p_addr->sin_addr.s_addr);

	/*
	 * not "0.0.0.0" or "127.0.0.1" - so format the address
	 */
	    if (addr && addr != 0x7F000001) {
		char           *colon = strchr(display, ':');

		if (colon == NULL)
		    colon = ":0.0";

		sprintf(ipaddress, "%d.%d.%d.%d%s",
			(int)((addr >> 030) & 0xFF),
			(int)((addr >> 020) & 0xFF),
			(int)((addr >> 010) & 0xFF),
			(int)(addr & 0xFF), colon);

		rval = ipaddress;
		break;
	    }
	}
    }

    close(skfd);
    return rval;
}
#endif				/* DISPLAY_IS_IP */
/*----------------------- end-of-file (C source) -----------------------*/
