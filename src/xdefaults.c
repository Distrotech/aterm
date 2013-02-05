/*--------------------------------*-C-*---------------------------------*
 * File:	xdefaults.c
 *----------------------------------------------------------------------*
 * Copyright 1997,1998 mj olesen <olesen@me.queensu.ca>
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
 *    1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 * Modifications:
 *    1997,1998 mj olesen <olesen@me.queensu.ca>
 *----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*
 * get resources from ~/.Xdefaults or ~/.Xresources with the memory-saving
 * default or with XGetDefault() (#define USE_XGETDEFAULT)
 *
 * Coding style:
 *	resource strings are indicated by an `rs_' prefix followed by
 *	the resource name.
 *	eg, `rs_saveLines' is the resource string corresponding to
 *	    the `saveLines' resource
 *----------------------------------------------------------------------*/

#ifndef lint
static const char rcsid[] = "$Id: xdefaults.c,v 1.5 2005/12/06 17:08:44 sasha Exp $";
#endif

#include "rxvt.h"		/* NECESSARY */

/* #define DEBUG_RESOURCES */

/* local functions referenced */
/*{{{ local variables */
static const char *rs_borderLess = NULL;
static const char *rs_loginShell = NULL;
static const char *rs_utmpInhibit = NULL;
static const char *rs_scrollBar = NULL;
static const char *rs_scrollBar_right = NULL;
static const char *rs_scrollBar_floating = NULL;
static const char *rs_scrollTtyOutput = NULL;
static const char *rs_scrollKeypress = NULL;

#ifdef TRANSPARENT
static const char *rs_transparent = NULL;
static const char *rs_transparent_sb = NULL;
#endif

#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
static const char *rs_bigfont_key = NULL;
static const char *rs_smallfont_key = NULL;
#endif

#ifndef NO_MAPALERT
# ifdef MAPALERT_OPTION
static const char *rs_mapAlert = NULL;
# endif
#endif
static const char *rs_visualBell = NULL;
static const char *rs_reverseVideo = NULL;

#ifdef META8_OPTION
static const char *rs_meta8 = NULL;
#endif
#ifdef MULTICHAR_SET
static const char *rs_multichar_encoding = NULL;
#endif
#ifdef GREEK_SUPPORT
static const char *rs_greek_keyboard = NULL;
#endif
/*}}} */

/*{{{ monolithic option/resource structure: */
/*
 * `string' options MUST have a usage argument
 * `switch' and `boolean' options have no argument
 *
 * if there's no desc(ription), it won't appear in usage()
 */
static const struct {
    unsigned long   flag;
    const char    **dp;		/* data pointer */
    const char     *const kw;	/* keyword */
    const char     *const opt;	/* option */
    const char     *const arg;	/* argument */
    const char     *const desc;	/* description */
} optList[] = {

/*
 * INFO() - descriptive information only
 * STRG() - command-line option, with/without resource
 * RSTRG() - resource/long-option
 * BOOL() - regular boolean `-/+' flag
 * SWCH() - `-' flag
 */
#define INFO(opt, arg, desc)			\
    {0, NULL, NULL, opt, arg, desc}
#define STRG(p, kw, opt, arg, desc)		\
    {0, &p, kw, opt, arg, desc}
#define RSTRG(p, kw, arg)			\
    {0, &p, kw, NULL, arg, NULL}
#define BOOL(p, kw, opt, flag, desc)		\
    {(Opt_Boolean|flag), &p, kw, opt, NULL, desc}
#define SWCH(opt, flag, desc)			\
    {(flag), NULL, NULL, opt, NULL, desc}

/* convenient macros */
#define optList_strlen(i)		\
    (optList[i].flag ? 0 : (optList[i].arg ? strlen (optList[i].arg) : 1))
#define optList_isBool(i)		\
    (optList[i].flag & Opt_Boolean)
#define optList_isReverse(i)		\
    (optList[i].flag & Opt_Reverse)
#define optList_size()			\
    (sizeof(optList) / sizeof(optList[0]))

    STRG(display_name, NULL, "display", "string",
	 "X server to contact"),
    STRG(rs_term_name, "termName", "tn", "string",
         "value of the TERM environment variable"),
    STRG(rs_geometry, "geometry", "geometry", "geometry",
         "size (in characters) and position"),
    STRG(display_name, NULL, "d", NULL, NULL),	/* short form */
    STRG(rs_geometry, NULL, "g", NULL, NULL),	/* short form */
    BOOL(rs_reverseVideo, "reverseVideo", "rv", Opt_reverseVideo,
         "reverse video"),
#ifdef _MYSTYLE_
    STRG(rs_mystyle, "MyStyle", "mst", "name",
         "MyStyle"),
#endif
    STRG(rs_color[Color_bg], "background", "bg", "color",
         "background color"),
    STRG(rs_color[Color_fg], "foreground", "fg", "color",
         "foreground color"),
/* colors: command-line long-option = resource name */
    RSTRG(rs_color[minCOLOR + 0], "color0", "color"),
    RSTRG(rs_color[minCOLOR + 1], "color1", "color"),
    RSTRG(rs_color[minCOLOR + 2], "color2", "color"),
    RSTRG(rs_color[minCOLOR + 3], "color3", "color"),
    RSTRG(rs_color[minCOLOR + 4], "color4", "color"),
    RSTRG(rs_color[minCOLOR + 5], "color5", "color"),
    RSTRG(rs_color[minCOLOR + 6], "color6", "color"),
    RSTRG(rs_color[minCOLOR + 7], "color7", "color"),
#ifndef NO_BRIGHTCOLOR
    RSTRG(rs_color[minBrightCOLOR + 0], "color8", "color"),
    RSTRG(rs_color[minBrightCOLOR + 1], "color9", "color"),
    RSTRG(rs_color[minBrightCOLOR + 2], "color10", "color"),
    RSTRG(rs_color[minBrightCOLOR + 3], "color11", "color"),
    RSTRG(rs_color[minBrightCOLOR + 4], "color12", "color"),
    RSTRG(rs_color[minBrightCOLOR + 5], "color13", "color"),
    RSTRG(rs_color[minBrightCOLOR + 6], "color14", "color"),
    RSTRG(rs_color[minBrightCOLOR + 7], "color15", "color"),
#endif				/* NO_BRIGHTCOLOR */
#ifndef NO_BOLDUNDERLINE
    RSTRG(rs_color[Color_BD], "colorBD", "color"),
    RSTRG(rs_color[Color_UL], "colorUL", "color"),
#endif				/* NO_BOLDUNDERLINE */
#ifdef KEEP_SCROLLCOLOR
    RSTRG(rs_color[Color_scroll], "scrollColor", "color"),
    RSTRG(rs_color[Color_trough], "troughColor", "color"),
#endif				/* KEEP_SCROLLCOLOR */
#if defined (BACKGROUND_IMAGE) || (MENUBAR_MAX)
    RSTRG(rs_path, "path", "search path"),
#endif				/* defined (BACKGROUND_IMAGE) || (MENUBAR_MAX) */
#ifdef BACKGROUND_IMAGE
    STRG(rs_backgroundPixmap,
         "backgroundPixmap",
         "pixmap", "file[;geom]", "background pixmap"),
#endif				/* BACKGROUND_IMAGE */
#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT)
    STRG(rs_backgroundType,
         "backgroundType",
         "bgtype", BGT_ALL, "type of the background pixmap transformation"),
    STRG(rs_tintType,
         "tintingType",
         "tinttype", TINT_TYPE_ALL,
	 "defines function to be used for background tinting"),
    STRG(rs_shade,
         "shading",
         "sh", "%", "make transparent background x% darker"),
    STRG(rs_color[Color_tint],
         "tinting",
	 "tint", "color",
         "tinted transparency color"),
#endif
    STRG(rs_textType,
         "textType",
         "txttype", GC_TYPE_ALL,
	 "defines function to be used for text drawing"),
#ifdef OFF_FOCUS_FADING
    STRG(rs_fade,
         "fading",
         "fade", "%", "make colors x% darker when aterm is loosing focus."),
#endif

#ifdef BOOL
#ifdef TRANSPARENT
    BOOL(rs_transparent, "transparent", "tr", Opt_transparent,
         "transparent background"),
    BOOL(rs_transparent_sb, "transpscrollbar", "trsb", Opt_transparent_sb,
         "transparent scrollbar"),
#endif
#endif
#if (MENUBAR_MAX)
    RSTRG(rs_menu, "menu", "name[;tag]"),
#endif
#ifndef NO_BOLDFONT
    STRG(rs_boldFont, "boldFont", "fb", "fontname", "bold text font"),
#endif
    STRG(rs_font[0], "font", "fn", "fontname", "normal text font"),
/* fonts: command-line option = resource name */
#if NFONTS > 1
    RSTRG(rs_font[1], "font1", "fontname"),
#endif
#if NFONTS > 2
    RSTRG(rs_font[2], "font2", "fontname"),
#endif
#if NFONTS > 3
    RSTRG(rs_font[3], "font3", "fontname"),
#endif
#if NFONTS > 4
    RSTRG(rs_font[4], "font4", "fontname"),
#endif
#if NFONTS > 5
    RSTRG(rs_font[5], "font5", "fontname"),
#endif
#if NFONTS > 6
    RSTRG(rs_font[6], "font6", "fontname"),
#endif
#if NFONTS > 7
    RSTRG(rs_font[7], "font7", "fontname"),
#endif
#ifdef MULTICHAR_SET
    STRG(rs_mfont[0], "mfont", "fm", "fontname", "multichar font"),

/* fonts: command-line option = resource name */
# if NFONTS > 1
    RSTRG(rs_mfont[1], "mfont1", "fontname"),
# endif
# if NFONTS > 2
    RSTRG(rs_mfont[2], "mfont2", "fontname"),
# endif
# if NFONTS > 3
    RSTRG(rs_mfont[3], "mfont3", "fontname"),
# endif
# if NFONTS > 4
    RSTRG(rs_mfont[4], "mfont4", "fontname"),
# endif
# if NFONTS > 5
    RSTRG(rs_mfont[5], "mfont5", "fontname"),
# endif
# if NFONTS > 6
    RSTRG(rs_mfont[6], "mfont6", "fontname"),
# endif
# if NFONTS > 7
    RSTRG(rs_mfont[7], "mfont7", "fontname"),
# endif
#endif				/* MULTICHAR_SET */

#ifdef MULTICHAR_SET
    STRG(rs_multichar_encoding, "multichar_encoding", "km", "mode",
         "multiple-character font encoding; mode = eucj | sjis | big5"),
#endif				/* MULTICHAR_SET */
#ifdef USE_XIM
    STRG(rs_preeditType, "preeditType", "pt", "style",
        "input style of input method; style = OverTheSpot | OffTheSpot | Root"),
    STRG(rs_inputMethod, "inputMethod", "im", "name", "name of input method"),
#endif                         /* USE_XIM */
#ifdef GREEK_SUPPORT
    STRG(rs_greek_keyboard, "greek_keyboard", "grk", "mode",
         "greek keyboard mapping; mode = iso | ibm"),
#endif
    SWCH("iconic", Opt_iconic, "start iconic"),
    SWCH("ic", Opt_iconic, NULL),	/* short form */
    STRG(rs_name, NULL, "name", "string",
         "client instance, icon, and title strings"),
    STRG(rs_title, "title", "title", "string", "title name for window"),
    STRG(rs_title, NULL, "T", NULL, NULL),	/* short form */
    STRG(rs_iconName, "iconName", "n", "string", "icon name for window"),
#ifndef NO_CURSORCOLOR
    STRG(rs_color[Color_cursor], "cursorColor", "cr", "color",
         "cursor color"),
/* command-line option = resource name */
    RSTRG(rs_color[Color_cursor2], "cursorColor2", "color"),
#endif				/* NO_CURSORCOLOR */
    STRG(rs_color[Color_pointer], "pointerColor", "pr", "color",
         "pointer color"),
    STRG(rs_borderWidth, "borderWidth", "bw", "number",
         "width of border"),
    STRG(rs_internal_border, "internalBorder", "ib", "number",
         "width of the internal border"),
    STRG(rs_color[Color_border], "borderColor", "bd", "color",
         "border color"),
    BOOL(rs_borderLess, "borderLess", "bl", Opt_borderLess, "no decoration"),
    BOOL(rs_loginShell, "loginShell", "ls", Opt_loginShell, "login shell"),
    BOOL(rs_scrollBar, "scrollBar", "sb", Opt_scrollBar, "scrollbar"),
    BOOL(rs_scrollBar_right, "scrollBar_right", "sr", Opt_scrollBar_right,
         "scrollbar right"),
    BOOL(rs_scrollBar_floating, "scrollBar_floating", "st",
         Opt_scrollBar_floating, "scrollbar without a trough"),
    BOOL(rs_scrollTtyOutput, "scrollTtyOutput", NULL,
         Opt_scrollTtyOutput, NULL),
    BOOL(rs_scrollTtyOutput, NULL, "si", Opt_Reverse | Opt_scrollTtyOutput,
	 "scroll-on-tty-output inhibit"),
    BOOL(rs_scrollKeypress, "scrollKey", "sk",
         Opt_scrollKeypress, "scroll-on-keypress"),
    STRG(rs_minBufferWidth, "minBufferWidth", "mbw", "number",
         "minimum number of columns stored in buffer"),
    STRG(rs_saveLines, "saveLines", "sl", "number",
         "number of scrolled lines to save"),
#ifdef USE_LINESPACE
    STRG(rs_lineSpace, "lineSpace", "lsp", "number",
         "line space"),
#endif
    BOOL(rs_utmpInhibit, "utmpInhibit", "ut", Opt_utmpInhibit,
         "utmp inhibit"),
    BOOL(rs_visualBell, "visualBell", "vb", Opt_visualBell, "visual bell"),

#ifndef NO_MAPALERT
# ifdef MAPALERT_OPTION
    BOOL(rs_mapAlert, "mapAlert", NULL, Opt_mapAlert, NULL),
# endif
#endif
#ifdef META8_OPTION
    BOOL(rs_meta8, "meta8", NULL, Opt_meta8, NULL),
    RSTRG(rs_modifier, "modifier", "string"),
#endif
#ifndef NO_BACKSPACE_KEY
    RSTRG(rs_backspace_key, "backspacekey", "string"),
#endif
#ifndef NO_DELETE_KEY
    RSTRG(rs_delete_key, "deletekey", "string"),
#endif
#ifdef PRINTPIPE
    RSTRG(rs_print_pipe, "print-pipe", "string"),
#endif
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    RSTRG(rs_bigfont_key, "bigfont_key", "keysym"),
    RSTRG(rs_smallfont_key, "smallfont_key", "keysym"),
#endif
#ifdef CUTCHAR_RESOURCE
    RSTRG(rs_cutchars, "cutchars", "string"),
#endif				/* CUTCHAR_RESOURCE */
    SWCH("C", Opt_console, "intercept console messages"),
    INFO("e", "command arg ...", "command to execute")
};

#undef INFO
#undef STRG
#undef RSTRG
#undef SWCH
#undef BOOL
/*}}} */

/*{{{ list_options: */
/*----------------------------------------------------------------------*/
/* PROTO */
void
list_options()
{
#define INDENT 30
    fprintf(stderr, "(");
#ifdef BACKGROUND_IMAGE
    fprintf(stderr, "background image,");
#endif
#ifdef XPM
    fprintf(stderr, "XPM,");
#endif
#ifdef USE_LIBASIMAGE
#ifdef JPEG
    fprintf(stderr, "JPEG,");
#endif
#ifdef PNG
    fprintf(stderr, "PNG,");
#endif
    fprintf(stderr, "AfterStep integration,");
#ifdef _MYSTYLE_
    fprintf(stderr, "AfterStep MyStyles,");
#else
    fprintf(stderr, "no AfterStep MyStyles,");
#endif
#endif
#ifdef UTMP_SUPPORT
    fprintf(stderr, "utmp,");
#endif
#ifdef MENUBAR
    fprintf(stderr, "menubar,");
#endif
#ifdef KANJI
    fprintf(stderr, "Kanji,");
#endif
#ifdef ZH
    fprintf(stderr, "Chinese,");
#endif
#ifdef THAI
    fprintf(stderr, "Thai,");
#endif
#ifdef XTERM_SCROLLBAR
    fprintf(stderr, "XTerm-scrollbar,");
#endif
#ifdef GREEK_SUPPORT
    fprintf(stderr, "Greek,");
#endif
#ifdef NO_BACKSPACE_KEY
    fprintf(stderr, "no backspace,");
#endif
#ifdef NO_DELETE_KEY
    fprintf(stderr, "no delete,");
#endif
#ifdef TRANSPARENT
    fprintf(stderr, "transparency,");
#else
    fprintf(stderr, "no transparency,");
# endif
#ifdef OFF_FOCUS_FADING
    fprintf(stderr, "fading,");
#else
    fprintf(stderr, "no fading,");
# endif
#ifdef NEXT_SCROLLBAR
    fprintf(stderr, "NeXT scrollbar,");
# endif
#ifdef NO_RESOURCES
    fprintf(stderr, "NoResources");
#else
# ifdef USE_XGETDEFAULT
    fprintf(stderr, "XGetDefaults");
# else
    fprintf(stderr, ".Xdefaults");
# endif
#endif

    fprintf(stderr, ")" );

}
/*}}} */

/*{{{ usage: */
/*----------------------------------------------------------------------*/
/* PROTO */
void
version(int type)
{
    switch (type) {
        case 0:			/* brief listing */
	    fprintf(stderr, "%s version %s\n", APL_NAME,VERSION);
	    break ;
        case 1:			/* full command-line listing */
	    fprintf(stderr, "%s version %s from %s\n", APL_NAME,VERSION, DATE);
	    list_options();
	    fprintf(stderr, "\n");
	    break ;
	case 2:			/* full resource listing */
	    fprintf(stderr, "%s\n", VERSION);
	    break ;
    }
    exit(EXIT_FAILURE);
}
/*}}} */

/*{{{ usage: */
/*----------------------------------------------------------------------*/
/* PROTO */
void
usage(int type)
{
    int             i, col;

    fprintf(stderr, "\nUsage v%s :", VERSION);
    list_options();
    fprintf(stderr, "\n%s", APL_NAME);

    switch (type) {
    case 0:			/* brief listing */
	fprintf(stderr, " [-help][-V]\n");
	col = 3;
	for (i = 0; i < optList_size(); i++) {
	    if (optList[i].desc != NULL) {
		int             len = 2;

		if (!optList_isBool(i)) {
		    len = optList_strlen(i);
		    if (len > 0)
			len++;	/* account for space */
		}
		len += 4 + strlen(optList[i].opt);

		col += len;
		if (col > 79) {	/* assume regular width */
		    fprintf(stderr, "\n");
		    col = 3 + len;
		}
		fprintf(stderr, " [-");
		if (optList_isBool(i))
		    fprintf(stderr, "/+");
		fprintf(stderr, "%s", optList[i].opt);
		if (optList_strlen(i))
		    fprintf(stderr, " %s]", optList[i].arg);
		else
		    fprintf(stderr, "]");
	    }
	}
	fprintf(stderr, "\n\n");
	break;

    case 1:			/* full command-line listing */
	fprintf(stderr,
		" [options] [-e command args]\n\n"
		"where options include:\n");

	for (i = 0; i < optList_size(); i++)
	    if (optList[i].desc != NULL)
		fprintf(stderr, "    %s%s %-*s%s%s\n",
			(optList_isBool(i) ? "-/+" : "-"),
			optList[i].opt,
			(INDENT - strlen(optList[i].opt)
			 + (optList_isBool(i) ? 0 : 2)),
			(optList[i].arg ? optList[i].arg : ""),
			(optList_isBool(i) ? "turn on/off " : ""),
			optList[i].desc);
	fprintf(stderr, "\n    --help to list long-options\n    --version for the version information\n\n");
	break;

    case 2:			/* full resource listing */
	fprintf(stderr,
		" [options] [-e command args]\n\n"
		"where resources (long-options) include:\n");

	for (i = 0; i < optList_size(); i++)
	    if (optList[i].kw != NULL)
		fprintf(stderr, "    %s: %*s\n",
			optList[i].kw,
			(INDENT - strlen(optList[i].kw)),
			(optList_isBool(i) ? "boolean" : optList[i].arg));

#ifdef KEYSYM_RESOURCE
	fprintf(stderr, "    " "keysym.sym" ": %*s\n",
		(INDENT - strlen("keysym.sym")), "keysym");
#endif
	fprintf(stderr, "\n    -help to list options\n    -version for the version information with options list\n\n");
	break;
    }
    exit(EXIT_FAILURE);
}
/*}}} */

/*{{{ get command-line options before getting resources */
/* PROTO */
void
get_options(int argc, char *argv[])
{
    int             i, bad_option = 0;
    static const char *const On = "ON";
    static const char *const Off = "OFF";

    for (i = 1; i < argc; i++) {
	int             entry, longopt = 0;
	const char     *flag;
	char           *opt = argv[i];

#ifdef DEBUG_RESOURCES
	fprintf(stderr, "argv[%d] = %s: ", i, argv[i]);
#endif
	if (*opt == '-') {
	    flag = On;
	    if (*++opt == '-')
		longopt = *opt++;	/* long option */
	} else if (*opt == '+') {
	    flag = Off;
	    if (*++opt == '+')
		longopt = *opt++;	/* long option */
	} else {
	    bad_option = 1;
	    print_error("bad option \"%s\"", opt);
	    continue;
	}

	if (!strcmp(opt, "help"))
	    usage(longopt ? 2 : 1);
	if (!strcmp(opt, "h"))
	    usage(0);
	if (!strcmp(opt, "version"))
	    version(longopt ? 2 : 1);
	if (!strcmp(opt, "V"))
	    version(0);

    /* feature: always try to match long-options */
	for (entry = 0; entry < optList_size(); entry++)
	    if ((optList[entry].kw && !strcmp(opt, optList[entry].kw)) ||
		(!longopt &&
		 optList[entry].opt && !strcmp(opt, optList[entry].opt)))
		break;

	if (entry < optList_size()) {
	    if (optList_isReverse(entry))
		flag = flag == On ? Off : On;
	    if (optList_strlen(entry)) {	/* string value */
		char           *str = argv[++i];

#ifdef DEBUG_RESOURCES
		fprintf(stderr, "string (%s,%s) = ",
			optList[entry].opt ? optList[entry].opt : "nil",
			optList[entry].kw ? optList[entry].kw : "nil");
#endif
		if (flag == On && str && optList[entry].dp) {
#ifdef DEBUG_RESOURCES
		    fprintf(stderr, "\"%s\"\n", str);
#endif
		    *(optList[entry].dp) = str;

		/* special cases are handled in main.c:main() to allow
		 * X resources to set these values before we settle for
		 * default values
		 */
		}
#ifdef DEBUG_RESOURCES
		else
		    fprintf(stderr, "???\n");
#endif
	    } else {		/* boolean value */
#ifdef DEBUG_RESOURCES
		fprintf(stderr, "boolean (%s,%s) = %s\n",
			optList[entry].opt, optList[entry].kw, flag);
#endif
		if (flag == On)
		    Options |= (optList[entry].flag);
		else
		    Options &= ~(optList[entry].flag);

		if (optList[entry].dp)
		    *(optList[entry].dp) = flag;
	    }
	} else
#ifdef KEYSYM_RESOURCE
	/* if (!strncmp (opt, "keysym.", strlen ("keysym."))) */
	if (Str_match(opt, "keysym.")) {
	    char           *str = argv[++i];

	/*
	 * '7' is strlen("keysym.")
	 */
	    if (str != NULL)
		parse_keysym(opt + 7, str);
	} else
#endif
	{
	/* various old-style options, just ignore
	 * Obsolete since about Jan 96,
	 * so they can probably eventually be removed
	 */
	    const char     *msg = "bad";

	    if (longopt) {
		opt--;
		bad_option = 1;
	    } else if (!strcmp(opt, "7") || !strcmp(opt, "8")
#ifdef GREEK_SUPPORT
	    /* obsolete 12 May 1996 (v2.17) */
		       || !Str_match(opt, "grk")
#endif
		)
		msg = "obsolete";
	    else
		bad_option = 1;

	    print_error("%s option \"%s\"", msg, --opt);
	}
    }

    if (bad_option)
	usage(0);
}
/*}}} */

#ifndef NO_RESOURCES
/*----------------------------------------------------------------------*/
/*{{{ string functions */
/*
 * a replacement for strcasecmp() to avoid linking an entire library
 */
/* PROTO */
int
my_strcasecmp(const char *s1, const char *s2)
{
    for ( /*nil */ ; (*s1 && *s2); s1++, s2++) {
	register int    c1 = toupper(*s1);
	register int    c2 = toupper(*s2);

	if (c1 != c2)
	    return (c1 - c2);
    }
    return (int)(*s1 - *s2);
}

/*}}} */

# ifdef KEYSYM_RESOURCE
/*
 * Define key from XrmEnumerateDatabase.
 *   quarks will be something like
 *      "rxvt" "keysym" "0xFF01"
 *   value will be a string
 */
/* ARGSUSED */
/* PROTO */
Bool
define_key(XrmDatabase * database, XrmBindingList bindings, XrmQuarkList quarks, XrmRepresentation * type, XrmValue * value, XPointer closure)
{
    int             last;

    for (last = 0; quarks[last] != NULLQUARK; last++)	/* look for last quark in list */
	;
    last--;
    parse_keysym(XrmQuarkToString(quarks[last]), (char *)value->addr);
    return False;
}

/*
 * look for something like this (XK_Delete)
 * rxvt*keysym.0xFFFF: "\177"
 *
 * arg will be
 *      NULL for ~/.Xdefaults and
 *      non-NULL for command-line options (need to allocate)
 */
/* PROTO */
int
parse_keysym(char *str, char *arg)
{
    char           *key_string;
    int             n, sym;

    if (arg == NULL) {
	if ((n = Str_match(str, "keysym.")) == 0)
	    return 0;
	str += n;		/* skip `keysym.' */
    }
/* some scanf() have trouble with a 0x prefix */
    if (isdigit(str[0])) {
	if (str[0] == '0' && toupper(str[1]) == 'X')
	    str += 2;
	if (arg) {
	    if (sscanf(str, (strchr(str, ':') ? "%x:" : "%x"), &sym) != 1)
		return -1;
	} else {
	    if (sscanf(str, "%x:", &sym) != 1)
		return -1;

	/* cue to ':', it's there since sscanf() worked */
	    str = strchr(str, ':');
	    str++;
	    arg = Str_trim(str);
	    if (arg == NULL)
		return -1;
	}
    } else {
    /*
     * convert keysym name to keysym number
     */
	if (arg == NULL) {
	    arg = str;

	    arg = strchr(str, ':');
	    if (arg == NULL)
		return -1;

	    *arg++ = '\0';
	    arg = Str_trim(arg);
	    if (arg == NULL)
		return -1;
	}
	sym = XStringToKeysym(str);

	if (sym == None)
	    return -1;
    }

    if (sym < 0xFF00 || sym > 0xFFFF)	/* we only do extended keys */
	return -1;
    sym -= 0xFF00;

    if (KeySym_map[sym] != NULL)	/* already set ? */
	return -1;

    if ((n = strlen(arg)) == 0)
	return -1;

    key_string = MALLOC((n + 2) * sizeof(char));

    STRCPY(key_string + 1, arg);

    n = Str_escaped(key_string + 1);
    if (n) {
	key_string[0] = min(n, 255);
	KeySym_map[sym] = (unsigned char *) key_string;
    } else {
	FREE(key_string);
	return -1;
    }

    return 1;
}
# endif				/* KEYSYM_RESOURCE */

# ifndef USE_XGETDEFAULT
/*{{{ get_xdefaults() */
/*
 * the matching algorithm used for memory-save fake resources
 */
/* PROTO */
void
get_xdefaults(FILE * stream, const char *name)
{
    unsigned int    len;
    char           *str, buffer[256];

    if (stream == NULL)
	return;
    len = strlen(name);
    while ((str = fgets(buffer, sizeof(buffer), stream)) != NULL) {
	unsigned int    entry, n;

	while (*str && isspace(*str))
	    str++;		/* leading whitespace */

	if ((str[len] != '*' && str[len] != '.') ||
	    (len && strncmp(str, name, len)))
	    continue;
	str += (len + 1);	/* skip `name*' or `name.' */

# ifdef KEYSYM_RESOURCE
	if (!parse_keysym(str, NULL))
# endif				/* KEYSYM_RESOURCE */
	    for (entry = 0; entry < optList_size(); entry++) {
		const char     *const kw = optList[entry].kw;

		if (kw == NULL)
		    continue;
		n = strlen(kw);
		if (str[n] == ':' && Str_match(str, kw)) {
		/* skip `keyword:' */
		    str += (n + 1);
		    str = Str_skip_space(str);
		    str = Str_trim(str);
		    n = (str ? strlen(str) : 0);
		    if (n && *(optList[entry].dp) == NULL) {
		    /* not already set */
			int		s;
			char           *p = MALLOC((n + 1) * sizeof(char));

			STRCPY(p, str);
			*(optList[entry].dp) = p;
			if (optList_isBool(entry)) {
			    s = my_strcasecmp(p, "TRUE") == 0
				|| my_strcasecmp(p, "YES") == 0
				|| my_strcasecmp(p, "ON") == 0
				|| my_strcasecmp(p, "1") == 0;
			    if (optList_isReverse(entry))
				s = !s;
			    if (s)
				Options |= (optList[entry].flag);
			    else
				Options &= ~(optList[entry].flag);
			}
		    }
		    break;
		}
	    }
    }
    rewind(stream);
}
/*}}} */
# endif				/* ! USE_XGETDEFAULT */
#endif				/* NO_RESOURCES */

/*{{{ read the resources files */
/*
 * using XGetDefault() or the hand-rolled replacement
 */
/* ARGSUSED */
/* PROTO */
void
extract_resources(Display * display, const char *name)
{
#ifndef NO_RESOURCES
# ifdef USE_XGETDEFAULT
/*
 * get resources using the X library function
 */
    int             entry;

#  ifdef XrmEnumOneLevel
    XrmName         name_prefix[3];
    XrmClass        class_prefix[3];
    char           *displayResource;
    XrmDatabase     database;
    char           *screenResource;
    XrmDatabase     screenDatabase;

/*
 * Get screen-specific resources (X11R5) and merge into common resources.
 */
    database = NULL;
    screenDatabase = NULL;
    displayResource = XResourceManagerString(display);
    if (displayResource != NULL)
	database = XrmGetStringDatabase(displayResource);
    screenResource = XScreenResourceString(DefaultScreenOfDisplay(display));
    if (screenResource != NULL)
	screenDatabase = XrmGetStringDatabase(screenResource);
    XrmMergeDatabases(screenDatabase, &database);
    XrmSetDatabase(display, database);
#  endif

    for (entry = 0; entry < optList_size(); entry++) {
	int		s;
	char           *p, *p0;
	const char     *kw = optList[entry].kw;

	if (kw == NULL || *(optList[entry].dp) != NULL)
	    continue;		/* previously set */

#define STRCMP(x, y)		strcmp((const char *)(x), (const char *)(y))
	p = XGetDefault(display, name, kw);
	p0 = XGetDefault(display, "!INVALIDPROGRAMMENAMEDONTMATCH!", kw);
	if (p == NULL || (p0 && STRCMP(p, p0) == 0)) {
	    p = XGetDefault(display, APL_SUBCLASS, kw);
	    if (p == NULL || (p0 && STRCMP(p, p0) == 0))
		p = XGetDefault(display, APL_CLASS, kw);
	}
	if (p == NULL && p0)
	    p = p0;
	if (p) {
	    *optList[entry].dp = p;

	    if (optList_isBool(entry)) {
		s = my_strcasecmp(p, "TRUE") == 0
		    || my_strcasecmp(p, "YES") == 0
		    || my_strcasecmp(p, "ON") == 0
		    || my_strcasecmp(p, "1") == 0;
		if (optList_isReverse(entry))
		    s = !s;
		if (s)
		    Options |= (optList[entry].flag);
		else
		    Options &= ~(optList[entry].flag);
	    }
	}
    }

/*
 * [R5 or later]: enumerate the resource database
 */
#  ifdef XrmEnumOneLevel
#   ifdef KEYSYM_RESOURCE
    name_prefix[0] = XrmStringToName(name);
    name_prefix[1] = XrmStringToName("keysym");
    name_prefix[2] = NULLQUARK;
    class_prefix[0] = XrmStringToName(APL_SUBCLASS);
    class_prefix[1] = XrmStringToName("Keysym");
    class_prefix[2] = NULLQUARK;
    XrmEnumerateDatabase(XrmGetDatabase(display),
			 name_prefix,
			 class_prefix,
			 XrmEnumOneLevel,
			 define_key,
			 NULL);
    name_prefix[0] = XrmStringToName(APL_CLASS);
    name_prefix[1] = XrmStringToName("keysym");
    class_prefix[0] = XrmStringToName(APL_CLASS);
    class_prefix[1] = XrmStringToName("Keysym");
    XrmEnumerateDatabase(XrmGetDatabase(display),
			 name_prefix,
			 class_prefix,
			 XrmEnumOneLevel,
			 define_key,
			 NULL);
#   endif
#  endif

# else				/* USE_XGETDEFAULT */
/* get resources the hard way, but save lots of memory */
    const char     *fname[] = { ".Xdefaults", ".Xresources" };
    FILE           *fd = NULL;
    char           *home;

    if ((home = getenv("HOME")) != NULL) {
	int             i, len = strlen(home) + 2;
	char           *f = NULL;

	for (i = 0; i < (sizeof(fname) / sizeof(fname[0])); i++) {
	    f = REALLOC(f, (len + strlen(fname[i])) * sizeof(char));

	    sprintf(f, "%s/%s", home, fname[i]);

	    if ((fd = fopen(f, "r")) != NULL)
		break;
	}
	FREE(f);
    }
/*
 * The normal order to match resources is the following:
 * @ global resources (partial match, ~/.Xdefaults)
 * @ application file resources (XAPPLOADDIR/Rxvt)
 * @ class resources (~/.Xdefaults)
 * @ private resources (~/.Xdefaults)
 *
 * However, for the hand-rolled resources, the matching algorithm
 * checks if a resource string value has already been allocated
 * and won't overwrite it with (in this case) a less specific
 * resource value.
 *
 * This avoids multiple allocation.  Also, when we've called this
 * routine command-line string options have already been applied so we
 * needn't to allocate for those resources.
 *
 * So, search in resources from most to least specific.
 *
 * Also, use a special sub-class so that we can use either or both of
 * "XTerm" and "Rxvt" as class names.
 */

    get_xdefaults(fd, name);
    get_xdefaults(fd, APL_SUBCLASS);

#  ifdef XAPPLOADDIR
    {
	FILE           *ad = fopen(XAPPLOADDIR "/" APL_SUBCLASS, "r");

	if (ad != NULL) {
	    get_xdefaults(ad, "");
	    fclose(ad);
	}
    }
#  endif				/* XAPPLOADDIR */

    get_xdefaults(fd, APL_CLASS);
    get_xdefaults(fd, "");	/* partial match */
    if (fd != NULL)
	fclose(fd);
# endif				/* USE_XGETDEFAULT */
#endif				/* NO_RESOURCES */

/*
 * even without resources, at least do this setup for command-line
 * options and command-line long options
 */
#ifdef MULTICHAR_SET
    set_multichar_encoding(rs_multichar_encoding);
#endif
#ifdef GREEK_SUPPORT
/* this could be a function in grkelot.c */
/* void set_greek_keyboard (const char * str); */
    if (rs_greek_keyboard) {
	if (!strcmp(rs_greek_keyboard, "iso"))
	    greek_setmode(GREEK_ELOT928);	/* former -grk9 */
	else if (!strcmp(rs_greek_keyboard, "ibm"))
	    greek_setmode(GREEK_IBM437);	/* former -grk4 */
    }
#endif				/* GREEK_SUPPORT */

#define to_keysym(pks,str) do { KeySym sym;\
if (str && ((sym = XStringToKeysym(str)) != 0)) *pks = sym; } while (0)

#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
    to_keysym(&ks_bigfont, rs_bigfont_key);
    to_keysym(&ks_smallfont, rs_smallfont_key);
#endif
#undef to_keysym
}
/*}}} */
/*----------------------- end-of-file (C source) -----------------------*/
