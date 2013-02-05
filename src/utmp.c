/*--------------------------------*-C-*---------------------------------*
 * File:	utmp.c
 *----------------------------------------------------------------------*
 * Copyright 1997,1998 Geoff Wing <gcw@pobox.com>
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
 *    1992      John Bovey <jdb@ukc.ac.uk>
 * Modifications:
 *    1993      lipka
 *    1993      Brian Stempien <stempien@cs.wmich.edu>
 *    1995      Raul Garcia Garcia <rgg@tid.es>
 *    1995      Piet W. Plomp <piet@idefix.icce.rug.nl>
 *    1997      Raul Garcia Garcia <rgg@tid.es>
 *    1997,1998 Geoff Wing <mason@primenet.com.au>
 *----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 * Public:
 *	extern void cleanutent (void);
 *	extern void makeutent (const char * pty, const char * hostname);
 *
 * Private:
 *	write_utmp ();
 *	update_wtmp ();
 *----------------------------------------------------------------------*/

#ifndef lint
static const char rcsid[] = "$Id: utmp.c,v 1.1.1.1 2004/11/10 17:21:52 sasha Exp $";
#endif

#include "rxvt.h"		/* NECESSARY */

/*
 * HAVE_SETUTENT corresponds to SYSV-style utmp support.
 * Without it corresponds to using BSD utmp support.
 * SYSV-style utmp support is further divided in normal utmp support
 * and utmpx support (Solaris 2.x) by HAVE_UTMPX_H
 */

/*
 * update wtmp entries - only for SYSV style UTMP systems
 */
#ifdef UTMP_SUPPORT
static char     ut_id[5];	/* remember if entry to utmp made */
# ifndef USE_SYSV_UTMP
static int      utmp_pos;	/* BSD position of utmp-stamp */
# endif
#endif

/* ------------------------------------------------------------------------- */
#ifndef HAVE_UTMPX_H		/* supposedly we have updwtmpx ? */
#ifdef WTMP_SUPPORT
/* PROTO */
void
rxvt_update_wtmp(char *fname, struct utmp *putmp)
{
    int             fd, retry = 10;	/* 10 attempts at locking */
    struct flock    lck;	/* fcntl locking scheme */

    if ((fd = open(fname, O_WRONLY | O_APPEND, 0)) < 0)
	return;

    lck.l_whence = SEEK_END;	/* start lock at current eof */
    lck.l_len = 0;		/* end at ``largest possible eof'' */
    lck.l_start = 0;
    lck.l_type = F_WRLCK;	/* we want a write lock */

    while (retry--)
    /* attempt lock with F_SETLK - F_SETLKW would cause a deadlock! */
	if ((fcntl(fd, F_SETLK, &lck) < 0) && errno != EACCESS) {
	    close(fd);
	    return;		/* failed for unknown reason: give up */
	}
    write(fd, putmp, sizeof(struct utmp));

/* unlocking the file */
    lck.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lck);

    close(fd);
}
#endif				/* WTMP_SUPPORT */
#endif				/* !HAVE_UTMPX_H */
/* ------------------------------------------------------------------------- */
#ifdef UTMP_SUPPORT
/*
 * make a utmp entry
 */
/* PROTO */
void
makeutent(const char *pty, const char *hostname)
{

    struct passwd  *pwent = getpwuid(getuid());
    UTMP	    utmp;

#ifndef USE_SYSV_UTMP
/*
 * BSD style utmp entry
 *      ut_line, ut_name, ut_host, ut_time
 */
    int             i;
    FILE           *fd0, *fd1;
    char            buf[256], name[256];

#else
/*
 * SYSV style utmp entry
 *      ut_user, ut_id, ut_line, ut_pid, ut_type, ut_exit, ut_time
 */
#ifndef linux
    char           *colon;
#endif

#endif				/* !USE_SYSV_UTMP */

/* BSD naming is of the form /dev/tty?? or /dev/pty?? */

    MEMSET(&utmp, 0, sizeof(UTMP));
    if (!strncmp(pty, "/dev/", 5))
	pty += 5;		/* skip /dev/ prefix */
    if (!strncmp(pty, "pty", 3) || !strncmp(pty, "tty", 3))
	STRNCPY(ut_id, (pty + 3), sizeof(ut_id));
    else
#ifndef USE_SYSV_UTMP
    {
	print_error("can't parse tty name \"%s\"", pty);
	ut_id[0] = '\0';	/* entry not made */
	return;
    }

    STRNCPY(utmp.ut_line, pty, sizeof(utmp.ut_line));
    STRNCPY(utmp.ut_name, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	    sizeof(utmp.ut_name));
    STRNCPY(utmp.ut_host, hostname, sizeof(utmp.ut_host));
    utmp.ut_time = time(NULL);

    if ((fd0 = fopen(UTMP_FILENAME, "r+")) == NULL)
	ut_id[0] = '\0';	/* entry not made */
    else {
	utmp_pos = -1;
	if ((fd1 = fopen(TTYTAB_FILENAME, "r")) != NULL) {
	    for (i = 1; (fgets(buf, sizeof(buf), fd1) != NULL); i++) {
		if (*buf == '#' || sscanf(buf, "%s", name) != 1)
		    continue;
		if (!strcmp(utmp.ut_line, name)) {
		    fclose(fd1);
		    utmp_pos = i * sizeof(struct utmp);

		    break;
		}
	    }
	    fclose(fd1);
	}
	if (utmp_pos < 0)
	    ut_id[0] = '\0';	/* entry not made */
	else {
	    fseek(fd0, utmp_pos, 0);
	    fwrite(&utmp, sizeof(UTMP), 1, fd0);
	}
	fclose(fd0);
    }

#else				/* USE_SYSV_UTMP */
    {
	int             n;

	if (sscanf(pty, "pts/%d", &n) == 1)
	    sprintf(ut_id, "vt%02x", (n % 256));	/* sysv naming */
	else {
	    print_error("can't parse tty name \"%s\"", pty);
	    ut_id[0] = '\0';	/* entry not made */
	    return;
	}
    }

    utmpname(UTMP_FILENAME);

    setutent();			/* XXX: should be unnecessaray */

    STRNCPY(utmp.ut_id, ut_id, sizeof(utmp.ut_id));
    utmp.ut_type = DEAD_PROCESS;
    (void)getutid(&utmp);	/* position to entry in utmp file */

/* set up the new entry */
    utmp.ut_type = USER_PROCESS;
#ifndef linux
    utmp.ut_exit.e_exit = 2;
#endif
    STRNCPY(utmp.ut_user, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	    sizeof(utmp.ut_user));
    STRNCPY(utmp.ut_id, ut_id, sizeof(utmp.ut_id));
    STRNCPY(utmp.ut_line, pty, sizeof(utmp.ut_line));

#ifdef HAVE_UTMP_HOST
    STRNCPY(utmp.ut_host, hostname, sizeof(utmp.ut_host));
#ifndef linux
    if ((colon = strrchr(utmp.ut_host, ':')) != NULL)
	*colon = '\0';
#endif
#endif				/* HAVE_UTMP_HOST */

/* ut_name is normally the same as ut_user, but .... */
    STRNCPY(utmp.ut_name, (pwent && pwent->pw_name) ? pwent->pw_name : "?",
	    sizeof(utmp.ut_name));

    utmp.ut_pid = getpid();

#ifdef HAVE_UTMPX_H
    utmp.ut_session = getsid(0);
    utmp.ut_tv.tv_sec = time(NULL);
    utmp.ut_tv.tv_usec = 0;
#else
    utmp.ut_time = time(NULL);
#endif				/* HAVE_UTMPX_H */

    pututline(&utmp);

#ifdef WTMP_SUPPORT
    update_wtmp(WTMP_FILENAME, &utmp);
#endif

    endutent();			/* close the file */
#endif				/* !USE_SYSV_UTMP */
}
#endif				/* UTMP_SUPPORT */

/* ------------------------------------------------------------------------- */
#ifdef UTMP_SUPPORT
/*
 * remove a utmp entry
 */
/* PROTO */
void
cleanutent(void)
{
    UTMP            utmp;

#ifndef USE_SYSV_UTMP
    FILE           *fd;

    if (ut_id[0] && ((fd = fopen(UTMP_FILENAME, "r+")) != NULL)) {
	MEMSET(&utmp, 0, sizeof(struct utmp));

	fseek(fd, utmp_pos, 0);
	fwrite(&utmp, sizeof(struct utmp), 1, fd);

	fclose(fd);
    }
#else				/* USE_SYSV_UTMP */
    UTMP	   *putmp;

    if (!ut_id[0])
	return;			/* entry not made */

    utmpname(UTMP_FILENAME);
    MEMSET(&utmp, 0, sizeof(UTMP));
    STRNCPY(utmp.ut_id, ut_id, sizeof(utmp.ut_id));
    utmp.ut_type = USER_PROCESS;

    setutent();			/* XXX: should be unnecessaray */

    putmp = getutid(&utmp);
    if (!putmp || putmp->ut_pid != getpid())
	return;

    putmp->ut_type = DEAD_PROCESS;

#ifdef HAVE_UTMPX_H
    putmp->ut_session = getsid(0);
    putmp->ut_tv.tv_sec = time(NULL);
    putmp->ut_tv.tv_usec = 0;
#else				/* HAVE_UTMPX_H */
    putmp->ut_time = time(NULL);
#endif				/* HAVE_UTMPX_H */
    pututline(putmp);

#ifdef WTMP_SUPPORT
    update_wtmp(WTMP_FILENAME, putmp);
#endif

    endutent();
#endif				/* !USE_SYSV_UTMP */
}
#endif
