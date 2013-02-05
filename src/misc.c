/*--------------------------------*-C-*---------------------------------*
 * File:	misc.c
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

#ifndef lint
static const char rcsid[] = "$Id: misc.c,v 1.1.1.1 2004/11/10 17:21:46 sasha Exp $";
#endif

#include "rxvt.h"		/* NECESSARY */

/*----------------------------------------------------------------------*/
/* PROTO */
const char     *
my_basename(const char *str)
{
    const char     *base = strrchr(str, '/');

    return (base ? base + 1 : str);
}

/*
 * Print an error message
 */
/* PROTO */
void
print_error(const char *fmt,...)
{
    va_list         arg_ptr;

    va_start(arg_ptr, fmt);
    fprintf(stderr, APL_NAME ": ");
    vfprintf(stderr, fmt, arg_ptr);
    fprintf(stderr, "\n");
    va_end(arg_ptr);
}

/*
 * check that the first characters of S1 match S2
 *
 * No Match
 *      return: 0
 * Match
 *      return: strlen (N)
 */
/* PROTO */
int
Str_match(const char *s1, const char *s2)
{
    int             n = 0;

    if (s1 == NULL || s2 == NULL)
	return 0;

    while (*s2) {
	if (*s1++ != *s2++)
	    return 0;
	n++;
    }

    return n;
}

/*
 * remove leading/trailing space and strip-off leading/trailing quotes
 * Do it in-place
 */
/* PROTO */
char           *
Str_skip_space(char *str)
{
    if (str && *str) {
	while (*str && isspace(*str))
	    str++;
    }
    return str;
}

/* PROTO */
char           *
Str_trim(char *str)
{
    if (str && *str) {
	char           *src;
	int             n;

	src = Str_skip_space(str);
	n = strlen(src) - 1;

	while (n > 0 && isspace(src[n]))
	    n--;
	src[n + 1] = '\0';

    /* strip leading/trailing quotes */
	if (src[0] == '"') {
	    src++;
	    n--;
	    if (src[n] == '"')
		src[n--] = '\0';
	}
	if (n < 0)
	    *str = '\0';
	else if (src != str) {
	    char           *dst = str;

	/* copy back in-place */
	    do {
		*dst++ = *src;
	    }
	    while (*src++ != '\0');
	}
    }
    return str;
}

/*
 * in-place interpretation of string:
 *
 *      backslash-escaped:      "\a\b\E\e\n\r\t", "\octal"
 *      Ctrl chars:     ^@ .. ^_, ^?
 *
 *      Emacs-style:    "M-" prefix
 *
 * Also,
 *      "M-x" prefixed strings, append "\r" if needed
 *      "\E]" prefixed strings (XTerm escape sequence) append "\a" if needed
 *
 * returns the converted string length
 */
/* PROTO */
int
Str_escaped(char *str)
{
    register char  *p = str;
    int             i = 0, len, n, append = 0;

/* use 'i' to increment through destination and p through source */

    if (str == NULL || (len = strlen(str)) == 0)
	return 0;

/* Emacs convenience, replace leading `M-..' with `\E..' */
    if ((n = Str_match(p, "M-")) != 0) {
	str[i++] = '\033';	/* destination */
	len--;
	p += n;
	if (toupper(*p) == 'X') {
	/* append carriage-return for `M-xcommand' */
	    append = '\r';
	    str[i++] = 'x';	/* destination */
	    p++;
	    while (isspace(*p)) {
		p++;
		len--;
	    }
	}
    }
    for ( /*nil */ ; i < len; i++) {
	register char   ch = *p++;

	if (ch == '\\') {
	    ch = *p;
	    if (ch >= '0' && ch <= '7') {	/* octal */
		int             j, num = 0;

		for (j = 0; j < 3 && (ch >= '0' && ch <= '7'); j++) {
		    num = num * 010 + (ch - '0');
		    p++;
		    len--;
		    ch = *p;
		}
		ch = (unsigned char)num;
	    } else {
		p++;
		len--;
		switch (ch) {
		case 'a':
		    ch = 007;
		    break;	/* bell */
		case 'b':
		    ch = '\b';
		    break;	/* backspace */
		case 'E':
		case 'e':
		    ch = 033;
		    break;	/* escape */
		case 'n':
		    ch = '\n';
		    break;	/* newline */
		case 'r':
		    ch = '\r';
		    break;	/* carriage-return */
		case 't':
		    ch = '\t';
		    break;	/* tab */
		}
	    }
	} else if (ch == '^') {
	    ch = *p;
	    p++;
	    len--;
	    ch = toupper(ch);
	    ch = (ch == '?' ? 127 : (ch - '@'));
	}
	str[i] = ch;
    }

/* ESC] is an XTerm escape sequence, must be ^G terminated */
    if (str[0] == '\0' && str[1] == '\033' && str[2] == ']')
	append = 007;

/* add trailing character as required */
    if (append && str[len - 1] != append)
	str[len++] = append;

    str[len] = '\0';

    return len;
}

/*----------------------------------------------------------------------*
 * file searching
 */

/* #define DEBUG_SEARCH_PATH */

#if defined (BACKGROUND_IMAGE) || (MENUBAR_MAX)
/*
 * search for FILE in the current working directory, and within the
 * colon-delimited PATHLIST, adding the file extension EXT if required.
 *
 * FILE is either semi-colon or zero terminated
 */
/* PROTO */
const char     *
File_search_path(const char *pathlist, const char *file, const char *ext)
{
    static char     name[256];
    int             maxpath, len;
    char           *p;
    char           *path = (char *)pathlist;	/* remove const */

    if (!access(file, R_OK))
	return file;

/* semi-colon delimited */
    if ((p = strchr(file, ';')) == NULL)
	p = strchr(file, '\0');
    len = (p - file);

#ifdef DEBUG_SEARCH_PATH
    getcwd(name, sizeof(name));
    fprintf(stderr, "pwd: \"%s\"\n", name);
    fprintf(stderr, "find: \"%.*s\"\n", len, file);
#endif

/* leave room for an extra '/' and trailing '\0' */
    maxpath = sizeof(name) - (len + (ext ? strlen(ext) : 0) + 2);
    if (maxpath <= 0)
	return NULL;

/* check if we can find it now */
    strncpy(name, file, len);
    name[len] = '\0';

    if (!access(name, R_OK))
	return name;
    if (ext) {
	strcat(name, ext);
	if (!access(name, R_OK))
	    return name;
    }
    for ( /*nil */ ; path != NULL && *path != '\0'; path = p) {
	int             n;

    /* colon delimited */
	if ((p = strchr(path, ':')) == NULL)
	    p = strchr(path, '\0');

	n = (p - path);
	if (*p != '\0')
	    p++;

	if (n > 0 && n <= maxpath) {
	    strncpy(name, path, n);
	    if (name[n - 1] != '/')
		name[n++] = '/';
	    name[n] = '\0';
	    strncat(name, file, len);

	    if (!access(name, R_OK))
		return name;
	    if (ext) {
		strcat(name, ext);
		if (!access(name, R_OK))
		    return name;
	    }
	}
    }
    return NULL;
}

/* PROTO */
const char     *
File_find(const char *file, const char *ext)
{
    const char     *f;

    if (file == NULL || *file == '\0')
	return NULL;

/* search environment variables here too */
    if ((f = File_search_path(rs_path, file, ext)) == NULL)
#ifdef ATERM_PATH_ENV
	if ((f = File_search_path(getenv(ATERM_PATH_ENV), file, ext)) == NULL)
#endif
#ifdef PATH_ENV
	    if ((f = File_search_path(getenv(PATH_ENV), file, ext)) == NULL)
#endif
		f = File_search_path(getenv("PATH"), file, ext);

#ifdef DEBUG_SEARCH_PATH
    if (f)
	fprintf(stderr, "found: \"%s\"\n", f);
#endif

    return f;
}
#endif				/* defined (BACKGROUND_IMAGE) || (MENUBAR_MAX) */

/*----------------------------------------------------------------------*
 * miscellaneous drawing routines
 */

/*
 * draw bottomShadow/highlight along top/left sides of the window
 */
/* PROTO */
void
Draw_tl(Window win, GC gc, int x, int y, int w, int h)
{
    int             shadow = SHADOW;

    if (w == 0 || h == 0)
	shadow = 1;

    w += (x - 1);
    h += (y - 1);

    for ( /*nil */ ; shadow > 0; shadow--, x++, y++, w--, h--) {
	XDrawLine(Xdisplay, win, gc, x, y, w, y);
	XDrawLine(Xdisplay, win, gc, x, y, x, h);
    }
}

/*
 * draw bottomShadow/highlight along the bottom/right sides of the window
 */
/* PROTO */
void
Draw_br(Window win, GC gc, int x, int y, int w, int h)
{
    int             shadow = SHADOW;

    if (w == 0 || h == 0)
	shadow = 1;

    w += (x - 1);
    h += (y - 1);

    x++;
    y++;
    for ( /*nil */ ; shadow > 0; shadow--, x++, y++, w--, h--) {
	XDrawLine(Xdisplay, win, gc, w, h, w, y);
	XDrawLine(Xdisplay, win, gc, w, h, x, h);
    }
}

/* PROTO */
void
Draw_Shadow(Window win, GC topShadow, GC botShadow, int x, int y, int w, int h)
{
    Draw_tl(win, topShadow, x, y, w, h);
    Draw_br(win, botShadow, x, y, w, h);
}

/* button shapes */
/* PROTO */
void
Draw_Triangle(Window win, GC topShadow, GC botShadow, int x, int y, int w, int type)
{
    switch (type) {
    case 'r':			/* right triangle */
	XDrawLine(Xdisplay, win, topShadow, x, y, x, y + w);
	XDrawLine(Xdisplay, win, topShadow, x, y, x + w, y + w / 2);
	XDrawLine(Xdisplay, win, botShadow, x, y + w, x + w, y + w / 2);
	break;

    case 'l':			/* right triangle */
	XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x + w, y);
	XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x, y + w / 2);
	XDrawLine(Xdisplay, win, topShadow, x, y + w / 2, x + w, y);
	break;

    case 'd':			/* down triangle */
	XDrawLine(Xdisplay, win, topShadow, x, y, x + w / 2, y + w);
	XDrawLine(Xdisplay, win, topShadow, x, y, x + w, y);
	XDrawLine(Xdisplay, win, botShadow, x + w, y, x + w / 2, y + w);
	break;

    case 'u':			/* up triangle */
	XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x + w / 2, y);
	XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x, y + w);
	XDrawLine(Xdisplay, win, topShadow, x, y + w, x + w / 2, y);
	break;
#if 0
    case 's':			/* square */
	XDrawLine(Xdisplay, win, topShadow, x + w, y, x, y);
	XDrawLine(Xdisplay, win, topShadow, x, y, x, y + w);
	XDrawLine(Xdisplay, win, botShadow, x, y + w, x + w, y + w);
	XDrawLine(Xdisplay, win, botShadow, x + w, y + w, x + w, y);
	break;
#endif
    }
}
/*----------------------- end-of-file (C source) -----------------------*/
