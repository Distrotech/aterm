/*
 * $Id: rxvt.h,v 1.18 2006/02/17 21:35:37 sasha Exp $
 */

#ifndef _RXVT_H			/* include once only */
#define _RXVT_H

#define NEXT_SCROLLBAR

#include "version.h"
#include "../config.h"
#include "feature.h"

/*
 *****************************************************************************
 * SYSTEM HACKS
 *****************************************************************************
 */
/* Consistent defines - please report on the necessity
 * @ Unixware: defines (__svr4__)
 */
#if defined (SVR4) && !defined (__svr4__)
# define __svr4__
#endif
#if defined (sun) && !defined (__sun__)
# define __sun__
#endif

#if defined (__sun__)
#include <sys/int_types.h>
#endif

/*
 * sun <sys/ioctl.h> isn't properly protected?
 * anyway, it causes problems when <termios.h> is also included
 */
#if defined (__sun__)
# undef HAVE_SYS_IOCTL_H
#endif

/*
 * Solaris defines SRIOCSREDIR in sys/strredir.h .
 * Needed for displaying console messages under solaris
 */
#if defined(__sun) && defined(__SVR4)
#include <sys/strredir.h>
#endif

#ifdef _SCO_DS     /* SCO Osr5 - Hops */
    /* Scos pty naming is /dev/[pt]typ0 - /dev/[pt]ty256 */
# define ALL_NUMERIC_PTYS
#endif

/*
 *****************************************************************************
 * INCLUDES
 *****************************************************************************
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_UTIL_H
# include <util.h>
#endif

#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

#if defined (HAVE_SYS_IOCTL_H) && !defined (__sun__)
/* seems to cause problems when <termios.h> is also included on some suns */
# include <sys/ioctl.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif

#ifdef HAVE_TERMIOS_H
# include <termios.h>
#else
# include <sgtty.h>
#endif

#ifdef UTMP_SUPPORT
# ifdef HAVE_UTMPX_H
#  include <utmpx.h>
#  define USE_SYSV_UTMP
# else
#  include <utmp.h>
#  ifdef HAVE_SETUTENT
#   define USE_SYSV_UTMP
#  endif
# endif
# ifdef HAVE_LASTLOG_H
#  include <lastlog.h>
# endif
# include <pwd.h>
#endif

#include <sys/wait.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>	/* Xlib, Xutil, Xresource, Xfuncproto */
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xmd.h>


#ifdef HAVE_AFTERIMAGE
#include <libAfterImage/afterimage.h>
extern ASVisual *asv;
#define PREFER_24BIT
#endif

/* this is needed at all times so that .pro could work */
struct MyStyle;

#ifdef HAVE_AFTERSTEP
#include <libAfterStep/afterstep.h>
#include <libAfterStep/asapp.h>
#include <libAfterStep/screen.h>
#include <libAfterStep/mystyle.h>
#include <libAfterStep/mystyle_property.h>
#include <libAfterStep/clientprops.h>
#include <libAfterStep/wmprops.h>
#else 
extern Atom  			_XROOTPMAP_ID;
extern Atom 			_XA_MwmAtom;
extern Atom 			_XA_NET_WM_PID;
extern Atom  	_XA_NET_SUPPORTING_WM_CHECK ;
extern Atom  	_XA_NET_SUPPORTED           ;
extern Atom  	_XA_NET_CURRENT_DESKTOP     ;
extern Atom 	_XA_NET_WM_DESKTOP;
extern Atom 	_XA_NET_WM_STATE;
extern Atom 	_XA_NET_WM_STATE_STICKY;
extern Atom 	_XA_NET_WM_STATE_SHADED;
extern Atom 	_XA_NET_WM_STATE_HIDDEN;
#ifndef get_flags
#define get_flags(var, val) 	(((var) & (val)))  /* making it sign safe */
#define set_flags(var, val) 	((var) |= (val))
#define clear_flags(var, val) 	((var) &= ~(val))
#endif

#endif

#ifdef DEBUG_X
#define XGetGeometry(dpy,win,r,x,y,w,h,b,d) \
	trace_XGetGeometry(__FILE__,__LINE__,dpy,win,r,x,y,w,h,b,d)
#endif

#ifdef RXVT_GRAPHICS
# include "rxvtgrx.h"
#endif

#ifdef GREEK_SUPPORT
# include "grkelot.h"
#endif

#if defined(KANJI) || defined(ZH)
# define MULTICHAR_SET		/* a glyph is defined by 1 or 2 chars */
#else
# undef  MULTICHAR_SET		/* a glyph is only ever defined by 1 char */
#endif

/*
 * Play some size tricks:  <X11/Xmd.h> might be better
 * Now if only we could presume every OS has int16_t, u_int16_t, etc.
 */
typedef RINT16T		R_int16_t;
typedef RUINT16T	R_u_int16_t;
typedef RINT32T		R_int32_t;
typedef RUINT32T	R_u_int32_t;
typedef RINTPT		R_int_p_t;
typedef RUINTPT		R_u_int_p_t;


/*
 *****************************************************************************
 * STRUCTURES AND TYPEDEFS
 *****************************************************************************
 */
/* Sanitize menubar info */
#ifndef MENUBAR
# undef MENUBAR_MAX
#endif
#ifndef MENUBAR_MAX
# define MENUBAR_MAX	0
#endif

/* If we're using either the fancy scrollbar or menu bars, keep the
 * scrollColor resource.
 */
#if !defined(XTERM_SCROLLBAR) || defined(MENUBAR)
# define KEEP_SCROLLCOLOR 1
#else
# undef KEEP_SCROLLCOLOR
#endif

#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT) || defined(_MYSTYLE_)

#ifndef HAVE_AFTERIMAGE

typedef struct ShadingInfo{
    XColor      tintColor ;
    int shading ;
}ShadingInfo ;
#define NO_NEED_TO_SHADE(o) (o.shading==100 && o.tintColor.red==0xFFFF && o.tintColor.green==0xFFFF && o.tintColor.blue == 0xFFFF)

#endif

#define INIT_SHADING(o)     {o.shading=100; o.tintColor.red=0xFFFF; o.tintColor.green=0xFFFF; o.tintColor.blue = 0xFFFF;}
struct MyStyle;

typedef struct _BackroundInfo {
    Pixmap	srcPixmap; /* after it was clipped out according to stuff below*/
    /* x,y,width and height of the subimage to be clipped out of
       the specifyed image and used as background */
    struct MyStyle *mystyle ;
    unsigned int user_flags ;
    int 	srcWidth, srcHeight, srcX, srcY ;
    int 	Width, Height ; /* width and height of the loaded pixmap */
    int		bMySource;/* indicates if src pixmap was loaded by us
			     and needs to be freed */
#define BGT_TILE   	"tile"
#define BGT_CENTER 	"center"
#define BGT_SCALE  	"scale"
#define BGT_SCALEH 	"scaleh"
#define BGT_SCALEV  	"scalev"
#define BGT_NO_TILE 	"notile"
#define BGT_NO_TILE_H 	"notileh"
#define BGT_NO_TILE_V 	"notilev"
#define BGT_CUT     	"cut"
#define BGT_ALL      	"tile|center|scale|scaleh|scalev|notile|notileh|notilev|cut"
    enum { BGT_Tile = 1,
	   BGT_Center,
	   BGT_Scale,
	   BGT_ScaleH,
	   BGT_ScaleV,
	   BGT_NoTile,
	   BGT_NoTileH,
	   BGT_NoTileV,
	   BGT_Cut,
#ifdef _MYSTYLE_
	   BGT_MyStyle,
#endif
	   BGT_None } trgType;



    Pixmap      trgPixmap;
    int 	trgPixmapSet;
#ifdef SCALING_GEOM_ENABLED
    /* requested x,y,width and height of the target pixmap */
    int 	trgWidth, trgHeight, trgX, trgY ;
#endif
    /* final width and height of constructed pixmap */
    int 	finWidth, finHeight ;
    int 	cutX, cutY ;
    ShadingInfo Shading ;

}BackgroundInfo ;
#endif

#ifndef NO_DEBUG_OUTPUT
#define DEBUG_BACKGROUND_PMAP
#endif


typedef struct {
    R_u_int16_t     width,	/* window width [pixels]                    */
		    height,	/* window height [pixels]                   */
		    fwidth,	/* font width [pixels]                      */
		    fheight,	/* font height [pixels]                     */
		    fprop,	/* font is proportional                     */
		    ncol, nrow,	/* window size [characters]                 */
		    bcol,	/* current number of columns in the buffer  */
		    min_bcol,	/* minimum horizontal columns in the buffer */
#ifdef USE_LINESPACE
                    lineSpace,  /* linespace             */
#endif
		    focus,	/* window has focus                         */
		    saveLines,	/* number of lines that fit in scrollback   */
		    borderWidth,/* number of pixels in window border        */
		    nscrolled,	/* number of line actually scrolled         */
		    view_start;	/* scrollback view starts here              */
    Window          parent, vt;	/* parent (main) and vt100 window           */
    GC              gc;		/* GC for drawing text                      */
    XFontStruct    *font;	/* main font structure                      */
#ifndef NO_BOLDFONT
    XFontStruct    *boldFont;	/* bold font                                */
#endif
#ifdef MULTICHAR_SET
    XFontStruct    *mfont;	/* Multichar font structure                 */
#endif
#ifdef USE_XIM
    XFontSet fontset;
#endif
#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT) || defined(_MYSTYLE_)
    BackgroundInfo background;
#endif
	/* geometry of the parent window at the time of the last ConfigureNotify */
	int root_x, root_y, root_width, root_height ; 

#define GC_TYPE_AND 	"and"
#define GC_TYPE_AND_REV	"andReverse"
#define GC_TYPE_AND_INV	"andInverted"
#define GC_TYPE_XOR	"xor"
#define GC_TYPE_OR	"or"
#define GC_TYPE_NOR	"nor"
#define GC_TYPE_INVERT	"invert"
#define GC_TYPE_EQUIV	"equiv"
#define GC_TYPE_INVERT	"invert"
#define GC_TYPE_OR_REV	"orReverse"
#define GC_TYPE_OR_INV	"orInverted"
#define GC_TYPE_NAND	"nand"
#define GC_TYPE_ALL 	"and|andReverse|andInverted|xor|or|nor|invert|equiv|invert|orReverse|orInverted|nand"
#define TINT_TYPE_TRUE	"true"
#define TINT_TYPE_ALL 	"true|and|andReverse|andInverted|xor|or|nor|invert|equiv|invert|orReverse|orInverted|nand"

#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT) || defined(_MYSTYLE_)
    GC              tintGC;
    Pixmap	    	LastPixmapUsed ;
	int 			LastPixmap_width, LastPixmap_height, LastPixmap_root_x, LastPixmap_root_y ; 
#endif

    int		    bMapped;
} TermWin_t;

typedef struct {
    short           beg, end;	/* beg/end of slider sub-window */
    short           top, bot;	/* top/bot of slider */
    short           state;	/* scrollbar state */
    Window          win;
} scrollBar_t;

typedef struct {
    short           state;
    Window          win;
} menuBar_t;

typedef struct {
    short           type;	/* must not be changed; first element */
    short           len;	/* strlen (str) */
    unsigned char  *str;	/* action to take */
} action_t;

typedef struct {
    short           type;	/* must not be changed; first element */
    struct menu_t  *menu;	/* sub-menu */
} submenu_t;

typedef struct menuitem_t {
    struct menuitem_t *prev;	/* prev menu-item */
    struct menuitem_t *next;	/* next menu-item */
    char           *name;	/* character string displayed */
    char           *name2;	/* character string displayed (right) */
    short           len;	/* strlen (name) */
    short           len2;	/* strlen (name) */
    union {
	short           type;	/* must not be changed; first element */
	action_t        action;
	submenu_t       submenu;
    } entry;
} menuitem_t;

enum menuitem_t_action {
    MenuLabel,
    MenuAction,
    MenuTerminalAction,
    MenuSubMenu
};

typedef struct menu_t {
    struct menu_t  *parent;	/* parent menu */
    struct menu_t  *prev;	/* prev menu */
    struct menu_t  *next;	/* next menu */
    menuitem_t     *head;	/* double-linked list */
    menuitem_t     *tail;	/* double-linked list */
    menuitem_t     *item;	/* current item */
    char           *name;	/* menu name */
    short           len;	/* strlen (name) */
    short           width;	/* maximum menu width [chars] */
    Window          win;	/* window of the menu */
    short           x;		/* x location [pixels] (chars if parent == NULL) */
    short           y;		/* y location [pixels] */
    short           w, h;	/* window width, height [pixels] */
} menu_t;

typedef struct bar_t {
    menu_t         *head, *tail;	/* double-linked list of menus */
    char           *title;	/* title to put in the empty menuBar */
#if (MENUBAR_MAX > 1)
# define MAXNAME 16
    char            name[MAXNAME];	/* name to use to refer to menubar */
    struct bar_t   *next, *prev;	/* circular linked-list */
#endif				/* (MENUBAR_MAX > 1) */
#define NARROWS	4
    action_t        arrows[NARROWS];
} bar_t;

typedef struct grcmd_t {
    char            cmd;
    short           color;
    short           ncoords;
    int            *coords;
    unsigned char  *text;
    struct grcmd_t *next;
} grcmd_t;

typedef struct grwin_t {
    Window          win;
    int             x, y;
    unsigned int    w, h;
    short           screen;
    grcmd_t        *graphics;
    struct grwin_t *prev, *next;
} grwin_t;

typedef struct {
    R_int16_t       row, col;
} row_col_t;


/* ways to deal with getting/setting termios structure */
#ifdef HAVE_TERMIOS_H
typedef struct termios ttymode_t;

# ifdef TCSANOW			/* POSIX */
#  define GET_TERMIOS(fd,tios)	tcgetattr (fd, tios)
#  define SET_TERMIOS(fd,tios)		\
	cfsetospeed (tios, BAUDRATE),	\
	cfsetispeed (tios, BAUDRATE),	\
	tcsetattr (fd, TCSANOW, tios)
# else
#  ifdef TIOCSETA
#   define GET_TERMIOS(fd,tios)	ioctl (fd, TIOCGETA, tios)
#   define SET_TERMIOS(fd,tios)		\
	tios->c_cflag |= BAUDRATE,	\
	ioctl (fd, TIOCSETA, tios)
#  else
#   define GET_TERMIOS(fd,tios)	ioctl (fd, TCGETS, tios)
#   define SET_TERMIOS(fd,tios)		\
	tios->c_cflag |= BAUDRATE,	\
	ioctl (fd, TCSETS, tios)
#  endif
# endif
# define SET_TTYMODE(fd,tios)		SET_TERMIOS (fd, tios)
#else
/* sgtty interface */
typedef struct {
    struct sgttyb   sg;
    struct tchars   tc;
    struct ltchars  lc;
    int             line;
    int             local;
} ttymode_t;

# define SET_TTYMODE(fd,tt)				\
	tt->sg.sg_ispeed = tt->sg.sg_ospeed = BAUDRATE,	\
	ioctl (fd, TIOCSETP, &(tt->sg)),		\
	ioctl (fd, TIOCSETC, &(tt->tc)),		\
	ioctl (fd, TIOCSLTC, &(tt->lc)),		\
	ioctl (fd, TIOCSETD, &(tt->line)),		\
	ioctl (fd, TIOCLSET, &(tt->local))
#endif				/* HAVE_TERMIOS_H */

#ifndef min
# define min(a,b)	(((a) < (b)) ? (a) : (b))
# define max(a,b)	(((a) > (b)) ? (a) : (b))
#endif

#define MAX_IT(current, other)	if ((other) > (current)) (current) = (other)
#define MIN_IT(current, other)	if ((other) < (current)) (current) = (other)
#define SWAP_IT(one, two, tmp)				\
    do {						\
	(tmp) = (one); (one) = (two); (two) = (tmp);	\
    } while (0)

#ifndef CLIENTPROPS_H_HEADER_INCLUDED

/* Motif window hints */
#define MWM_HINTS_FUNCTIONS     (1L << 0)
#define MWM_HINTS_DECORATIONS   (1L << 1)
#define MWM_HINTS_INPUT_MODE    (1L << 2)
#define MWM_HINTS_STATUS        (1L << 3)
/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL            (1L << 0)
#define MWM_FUNC_RESIZE         (1L << 1)
#define MWM_FUNC_MOVE           (1L << 2)
#define MWM_FUNC_MINIMIZE       (1L << 3)
#define MWM_FUNC_MAXIMIZE       (1L << 4)
#define MWM_FUNC_CLOSE          (1L << 5)
/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)
/* bit definitions for MwmHints.inputMode */
#define MWM_INPUT_MODELESS                  0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL              2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3
#define PROP_MWM_HINTS_ELEMENTS             5


typedef struct MwmHints {
  CARD32 flags;
  CARD32 functions;
  CARD32 decorations;
  INT32  input_mode;
  CARD32 status;
} MwmHints;

#endif
/*
 *****************************************************************************
 * NORMAL DEFINES
 *****************************************************************************
 */

#ifdef OLD_SELECTION
# define OLD_WORD_SELECTION
#endif

#define DEFAULT_SHELL "/bin/bash"

/* sort out conflicts in feature.h */
#ifdef KANJI
# undef ZH			/* remove Chinese big5 support        */
# undef GREEK_SUPPORT		/* Kanji/Greek together is too weird  */
# undef DEFINE_XTERM_COLOR	/* since kterm-color doesn't exist?   */
#endif

#ifdef ZH
# undef KANJI			/* can't put Chinese/Kanji together   */
# undef GREEK_SUPPORT
# undef DEFINE_XTERM_COLOR
#endif

#define APL_CLASS	"XTerm"	/* class name */
#define APL_SUBCLASS	"Aterm"	/* also check resources under this name */
#define APL_NAME	"aterm"	/* normal name */

/* COLORTERM, TERM environment variables */
#define COLORTERMENV	"rxvt"
#ifdef BACKGROUND_IMAGE
# define COLORTERMENVFULL COLORTERMENV "-xpm"
#else
# define COLORTERMENVFULL COLORTERMENV
#endif
#ifndef TERMENV
# ifdef KANJI
#  define TERMENV	"kterm"
# else
#  define TERMENV	"rxvt"
# endif
#endif

#if defined (NO_MOUSE_REPORT) && !defined (NO_MOUSE_REPORT_SCROLLBAR)
# define NO_MOUSE_REPORT_SCROLLBAR
#endif

#ifdef NO_RESOURCES
# undef USE_XGETDEFAULT
#endif

/* now look for other badly set stuff */

#ifdef UTMP_SUPPORT
# ifndef UTMP_FILENAME
#  ifdef UTMP_FILE
#   define UTMP_FILENAME		UTMP_FILE
#  else
#   ifdef _PATH_UTMP
#    define UTMP_FILENAME		_PATH_UTMP
#   else
#    ifdef RXVT_UTMP_FILE
#     define UTMP_FILENAME		RXVT_UTMP_FILE
#    else
#     error define UTMP_FILENAME in config.h
#    endif
#   endif
#  endif
# endif
# ifdef USE_SYSV_UTMP
#  ifndef USER_PROCESS
#   define USER_PROCESS		7
#  endif
#  ifndef DEAD_PROCESS
#   define DEAD_PROCESS		8
#  endif
# endif
# ifdef HAVE_UTMPX_H
#  define UTMP			struct utmpx
#  define setutent		setutxent
#  define getutent		getutxent
#  define getutid		getutxid
#  define endutent		endutxent
#  define pututline		pututxline
# else
#  define UTMP			struct utmp
# endif
#endif

#ifdef WTMP_SUPPORT
# ifdef HAVE_UTMPX_H		/* means we're on Solaris (or Irix?) */
#  undef WTMP_FILENAME
#  if  defined(WTMPX_FILE) || defined(_WTMPX_FILE)
#    if defined(WTMPX_FILE)
#    	define WTMP_FILENAME	WTMPX_FILE
#    else
#    	define WTMP_FILENAME	_WTMPX_FILE
#    endif
#    define update_wtmp		updwtmpx
#  else
#       define WTMP_FILENAME	RXVT_WTMP_FILE
#       define update_wtmp	rxvt_update_wtmp
#  endif
# else
#  define update_wtmp		rxvt_update_wtmp
#  ifndef WTMP_FILENAME
#   ifdef WTMP_FILE
#    define WTMP_FILENAME		WTMP_FILE
#   else
#    ifdef _PATH_WTMP
#     define WTMP_FILENAME		_PATH_WTMP
#    else
#     ifdef RXVT_WTMP_FILE
#      define WTMP_FILENAME		RXVT_WTMP_FILE
#     else
#      error define WTMP_FILENAME in config.h
#     endif
#    endif
#   endif
#  endif
# endif
#endif

#if !defined (EACCESS) && defined(EAGAIN)
# define EACCESS EAGAIN
#endif

#ifndef EXIT_SUCCESS			/* missing from <stdlib.h> */
# define EXIT_SUCCESS		0	/* exit function success */
# define EXIT_FAILURE		1	/* exit function failure */
#endif

#define menuBar_esc		10
#define scrollBar_esc		30
#define menuBar_margin		2	/* margin below text */

/* width of scrollBar, menuBar shadow, must be 1 or 2 */
#ifdef HALFSHADOW
# define SHADOW 1
#else
# define SHADOW 2
#endif


#define NO_REFRESH		0	/* Window not visible at all!        */
#define FAST_REFRESH		(1<<1)  /* Fully exposed window              */
#define SLOW_REFRESH		(1<<2)	/* Partially exposed window          */
#define SMOOTH_REFRESH		(1<<3)	/* Do sync'ing to make it smooth     */

#ifdef NO_SECONDARY_SCREEN
# define NSCREENS		0
#else
# define NSCREENS		1
#endif

#define IGNORE			0
#define SAVE			's'
#define RESTORE			'r'

/* special (internal) prefix for font commands */
#define FONT_CMD		'#'
#define FONT_DN			"#-"
#define FONT_UP			"#+"

/* flags for scr_gotorc() */
#define C_RELATIVE		1	/* col movement is relative */
#define R_RELATIVE		2	/* row movement is relative */
#define RELATIVE		(R_RELATIVE|C_RELATIVE)

/* modes for scr_insdel_chars(), scr_insdel_lines() */
#define INSERT			-1	/* don't change these values */
#define DELETE			+1
#define ERASE			+2

/* modes for scr_page() - scroll page. used by scrollbar window */
enum {
    UP,
    DN,
    NO_DIR
};

/* arguments for scr_change_screen() */
enum {
    PRIMARY,
    SECONDARY
};

/* all basic bit-flags in first/lower 16 bits */

#define RS_None			0		/* Normal */
#define RS_fgMask		0x0000001Fu	/* 32 colors */
#define RS_bgMask		0x000003E0u	/* 32 colors */
#define RS_Bold			0x00000400u	/* bold */
#define RS_Blink		0x00000800u	/* blink */
#define RS_RVid			0x00001000u	/* reverse video */
#define RS_Uline		0x00002000u	/* underline */
#define RS_acsFont		0x00004000u	/* ACS graphics char set */
#define RS_ukFont		0x00008000u	/* UK character set */
#define RS_fontMask		(RS_acsFont|RS_ukFont)
#define RS_baseattrMask		(RS_Bold|RS_Blink|RS_RVid|RS_Uline)

/* all other bit-flags in upper 16 bits */

#ifdef MULTICHAR_SET
# define RS_multi0		0x10000000u	/* only multibyte characters */
# define RS_multi1		0x20000000u	/* multibyte 1st byte */
# define RS_multi2		(RS_multi0|RS_multi1)	/* multibyte 2nd byte */
# define RS_multiMask		(RS_multi0|RS_multi1)	/* multibyte mask */
#else
# define RS_multiMask		0
#endif

#define RS_attrMask		(RS_baseattrMask|RS_fontMask|RS_multiMask)

#define	Opt_console		(1LU<<0)
#define Opt_loginShell		(1LU<<1)
#define Opt_iconic		(1LU<<2)
#define Opt_visualBell		(1LU<<3)
#define Opt_mapAlert		(1LU<<4)
#define Opt_reverseVideo	(1LU<<5)
#define Opt_utmpInhibit		(1LU<<6)
#define Opt_scrollBar		(1LU<<7)
#define Opt_scrollBar_right	(1LU<<8)
#define Opt_scrollBar_floating	(1LU<<9)
#define Opt_meta8		(1LU<<10)
#define Opt_scrollTtyOutput	(1LU<<11)
#define Opt_scrollKeypress	(1LU<<12)
#define Opt_transparent		(1LU<<13)
#define Opt_transparent_sb	(1LU<<14)
#define Opt_borderLess		(1LU<<16)

/* place holder used for parsing command-line options */
#define Opt_Reverse		(1LU<<30)
#define Opt_Boolean		(1LU<<31)

/*
 * XTerm escape sequences: ESC ] Ps;Pt BEL
 */
#define XTerm_name		0
#define XTerm_iconName		1
#define XTerm_title		2
#define XTerm_Color		4	/* change colors */
#define XTerm_Color_cursor	12	/* change actual 'Cursor' color */
#define XTerm_Color_pointer	13	/* change actual 'Pointer' color */
#define XTerm_Color_BD		18	/* change actual 'Bold' color */
#define XTerm_Color_UL		19	/* change actual 'Underline' color */
#define XTerm_logfile		46	/* not implemented */
#define XTerm_font		50

/*
 * rxvt extensions of XTerm escape sequences: ESC ] Ps;Pt BEL
 */
#define XTerm_Menu		10	/* set menu item */
#define XTerm_Pixmap		20	/* new bg pixmap */
#define XTerm_restoreFG		39	/* change default fg color */
#define XTerm_restoreBG		49	/* change default bg color */

#define restoreFG		39	/* restore default fg color */
#define restoreBG		49	/* restore default bg color */

/* Words starting with `Color_' are colours.  Others are counts */

enum colour_list {
    Color_fg,
    Color_bg,
    minCOLOR,				/* 2 */
    Color_Black = minCOLOR,
    Color_Red3,
    Color_Green3,
    Color_Yellow3,
    Color_Blue3,
    Color_Magenta3,
    Color_Cyan3,
    maxCOLOR,				/* minCOLOR + 7 */
#ifndef NO_BRIGHTCOLOR
    Color_AntiqueWhite = maxCOLOR,
    minBrightCOLOR,			/* maxCOLOR + 1 */
    Color_Grey25 = minBrightCOLOR,
    Color_Red,
    Color_Green,
    Color_Yellow,
    Color_Blue,
    Color_Magenta,
    Color_Cyan,
    maxBrightCOLOR,			/* minBrightCOLOR + 7 */
    Color_White = maxBrightCOLOR,
#else
    Color_White = maxCOLOR,
#endif
#ifndef NO_CURSORCOLOR
    Color_cursor,
    Color_cursor2,
#endif
    Color_pointer,
    Color_border,
#if defined(TRANSPARENT) || defined(BACKGROUND_IMAGE)
    Color_tint,
#endif
#ifndef NO_BOLDUNDERLINE
    Color_BD,
    Color_UL,
#endif
#ifdef KEEP_SCROLLCOLOR
    Color_scroll,
    Color_trough,
#endif
    NRS_COLORS,				/* */
#ifdef KEEP_SCROLLCOLOR
    Color_topShadow = NRS_COLORS,
    Color_bottomShadow,
    TOTAL_COLORS 			/* upto 28 */
#else
    TOTAL_COLORS = NRS_COLORS		/* */
#endif
};

#define DEFAULT_RSTYLE		(RS_None | (Color_fg) | (Color_bg<<5))

/*
 * number of graphics points
 * divisible by 2 (num lines)
 * divisible by 4 (num rect)
 */
#define	NGRX_PTS	1000


/*
 *****************************************************************************
 * MACRO DEFINES
 *****************************************************************************
 */
#define STRCPY(a, b)		strcpy ((char *)(a), (char *) (b))
#define STRNCPY(a, b, c)	strncpy ((char *)(a), (char *) (b), (c))
#ifdef NO_RMEMSET
# define MEMSET(a, b, c)	memset ((a), (b), (c))
#else
# define MEMSET(a, b, c)	rmemset ((a), (b), (R_int_p_t) (c))
#endif

#define MALLOC(sz)		malloc (sz)
#define CALLOC(type, n)		calloc ((n), sizeof(type))
#define REALLOC(mem, sz)	((mem) ? realloc ((mem), (sz)) : malloc(sz))
#define FREE(ptr)		free (ptr)

/* convert pixel dimensions to row/column values */
#define Pixel2Col(x)		Pixel2Width((x) - TermWin_internalBorder)
#define Pixel2Row(y)		Pixel2Height((y) - TermWin_internalBorder)
#define Pixel2Width(x)		((x) / TermWin.fwidth)
#define Pixel2Height(y)		((y) / TermWin.fheight)
#define Col2Pixel(col)		(Width2Pixel(col) + TermWin_internalBorder)
#define Row2Pixel(row)		(Height2Pixel(row) + TermWin_internalBorder)
#define Width2Pixel(n)		((n) * TermWin.fwidth)
#define Height2Pixel(n)		((n) * TermWin.fheight)

#ifdef THAI
#define CharWidth(wf, ch)   ((wf)->per_char[(unsigned char)(ch) - (wf)->min_char_or_byte2].width)
#endif

#define TermWin_TotalWidth()	(TermWin.width  + 2 * TermWin_internalBorder)
#define TermWin_TotalHeight()	(TermWin.height + 2 * TermWin_internalBorder)

#define Xscreen			DefaultScreen(Xdisplay)
#define Xroot			DefaultRootWindow(Xdisplay)

/* how to build & extract colors and attributes */
#define GET_FGCOLOR(r)		(((r) & RS_fgMask))
#define GET_BGCOLOR(r)		(((r) & RS_bgMask)>>5)
#define GET_ATTR(r)		(((r) & RS_attrMask))
#define GET_BGATTR(r)		(((r) & (RS_attrMask | RS_bgMask)))
#define SET_FGCOLOR(r,fg)	(((r) & ~RS_fgMask)  | (fg))
#define SET_BGCOLOR(r,bg)	(((r) & ~RS_bgMask)  | ((bg)<<5))
#define SET_ATTR(r,a)		(((r) & ~RS_attrMask)| (a))

#define scrollbar_visible()	(scrollBar.state)
#define scrollbar_isMotion()	(scrollBar.state == 'm')
#define scrollbar_isUp()	(scrollBar.state == 'U')
#define scrollbar_isDn()	(scrollBar.state == 'D')
#define scrollbar_isUpDn()	isupper (scrollBar.state)
#define isScrollbarWindow(w)	(scrollbar_visible() && (w) == scrollBar.win)

#define scrollbar_setNone()	scrollBar.state = 1
#define scrollbar_setMotion()	scrollBar.state = 'm'
#define scrollbar_setUp()	scrollBar.state = 'U'
#define scrollbar_setDn()	scrollBar.state = 'D'

#ifdef NEXT_SCROLLBAR
# define scrollbar_dnval()	(scrollBar.end + (SB_WIDTH + 1))
# define scrollbar_upButton(y)	((y) > scrollBar.end \
				 && (y) <= scrollbar_dnval())
# define scrollbar_dnButton(y)	((y) > scrollbar_dnval())
#else
# define scrollbar_upButton(y)	((y) < scrollBar.beg)
# define scrollbar_dnButton(y)	((y) > scrollBar.end)
#endif

#define scrollbar_above_slider(y)	((y) < scrollBar.top)
#define scrollbar_below_slider(y)	((y) > scrollBar.bot)
#define scrollbar_position(y)		((y) - scrollBar.beg)
#define scrollbar_size()		(scrollBar.end - scrollBar.beg)

#if (MENUBAR_MAX > 1)
/* rendition style flags */
# define menubar_visible()	(menuBar.state)
# define menuBar_height()	(TermWin.fheight + SHADOW)
# define menuBar_TotalHeight()	(menuBar_height() + SHADOW + menuBar_margin)
# define isMenuBarWindow(w)	((w) == menuBar.win)
#else
# define isMenuBarWindow(w)	(0)
# define menuBar_height()	(0)
# define menuBar_TotalHeight()	(0)
# define menubar_visible()	(0)
#endif

#if defined(BACKGROUND_IMAGE) || defined(_MYSTYLE_)
# define XPMClearArea(a, b, c, d, e, f, g)	XClearArea((a), (b), (c), (d), (e), (f), (g))
#else
# define XPMClearArea(a, b, c, d, e, f, g)
#endif

#define Gr_ButtonPress(x,y)	Gr_ButtonReport ('P',(x),(y))
#define Gr_ButtonRelease(x,y)	Gr_ButtonReport ('R',(x),(y))
/*
 *****************************************************************************
 * VARIABLES
 *****************************************************************************
 */
#ifdef INTERN
# define EXTERN
#else
# define EXTERN extern
#endif

EXTERN Colormap     Xcmap;
EXTERN int          Xdepth;
EXTERN Visual      *Xvisual;

EXTERN TermWin_t	TermWin;
#define PARENTS_NUM 3
#define PARENTS_INIT {0,0,0}
extern Window		ParentWin[PARENTS_NUM];
extern int		ParentWinNum;

/* gap between text and window edges (could be configurable) */
EXTERN int TermWin_internalBorder;
/* must be two times of value above */
EXTERN int TermWin_internalBorders;


EXTERN scrollBar_t	scrollBar;
EXTERN menuBar_t	menuBar;
EXTERN Display	       *Xdisplay;
EXTERN int	       		XdisplayWidth, XdisplayHeight;

typedef struct ExtWMSupportData
{
#define WM_SupportsDesktops 		(0x01<<0)	
#define WM_AtermStateSticky			(0x01<<1)	  
#define WM_AtermStateShaded 		(0x01<<2)	  
#define WM_AtermStateHidden 		(0x01<<3)	  
#define WM_ClaimSupportsDesktops 	(0x01<<4)	  

	unsigned long flags ; 	
	Window supporting_wm_check ; 
	CARD32 current_desktop;
	CARD32 aterm_desktop;
}ExtWMSupportData;

EXTERN ExtWMSupportData ExtWM; 

EXTERN Bool	wm_supports_desktops ;
EXTERN int	wm_current_desktop, wm_aterm_desktop ;

EXTERN unsigned long	Options;
EXTERN int		sb_shadow;
EXTERN int		delay_menu_drawing;
EXTERN const char      *display_name;
EXTERN const char      *rs_name;	/* client instance (resource name) */
EXTERN const char      *rs_term_name;
EXTERN const char      *rs_color[NRS_COLORS];
EXTERN Pixel		PixColorsFocused[TOTAL_COLORS];
#ifdef OFF_FOCUS_FADING
EXTERN Pixel		PixColorsUnFocused[TOTAL_COLORS];
#endif
#ifdef _MYSTYLE_
EXTERN const char      *rs_mystyle;
#endif
EXTERN Pixel	       *PixColors;
EXTERN const char      *rs_title;	/* title name for window */
EXTERN const char      *rs_iconName;	/* icon name for window */
EXTERN const char      *rs_geometry;	/* window geometry */
EXTERN const char      *rs_minBufferWidth; /* minimum buffer width - so we can scroll horizontally */
EXTERN const char      *rs_saveLines;	/* scrollback buffer [lines] */
#ifdef USE_LINESPACE
EXTERN const char      *rs_lineSpace; /* line space [pixels] */
#endif
EXTERN const char      *rs_borderWidth; /* border width [pixels] */
EXTERN const char      *rs_internal_border; /* internal border width [pixels] */
EXTERN const char      *rs_cutchars;	/* chars for selection boundaries */
#ifdef META8_OPTION
EXTERN const char      *rs_modifier;
#endif
#ifndef NO_BACKSPACE_KEY
EXTERN const char      *rs_backspace_key;
#endif
#ifndef NO_DELETE_KEY
EXTERN const char      *rs_delete_key;
#endif
EXTERN const char      *rs_font[NFONTS];
#ifdef MULTICHAR_SET
EXTERN const char      *rs_mfont[NFONTS];
#endif
#ifndef NO_BOLDFONT
EXTERN const char      *rs_boldFont;
#endif
#ifdef PRINTPIPE
EXTERN const char      *rs_print_pipe;
#endif
#ifdef USE_XIM
EXTERN const char      *rs_preeditType;
EXTERN const char      *rs_inputMethod;
#endif
#ifndef NO_BRIGHTCOLOR
EXTERN unsigned int	colorfgbg;
#endif
#ifdef KEYSYM_RESOURCE
EXTERN const unsigned char *KeySym_map[256];
#endif
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
EXTERN KeySym		ks_bigfont;
EXTERN KeySym		ks_smallfont;
#endif
EXTERN const char      *rs_menu;
EXTERN const char      *rs_path;

#ifdef BACKGROUND_IMAGE
EXTERN const char      *rs_backgroundPixmap;
#endif

#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT)
EXTERN const char      *rs_backgroundType;
EXTERN const char      *rs_tintType;
EXTERN const char      *rs_shade;
#endif

#ifdef OFF_FOCUS_FADING
EXTERN const char      *rs_fade;
#endif

EXTERN const char      *rs_textType;

EXTERN Atom     		aterm_XA_TARGETS;
EXTERN Atom     		aterm_XA_TEXT;
EXTERN Atom     		_XA_COMPAUND_TEXT;
EXTERN Atom     		aterm_XA_UTF8_STRING;
EXTERN Atom     		aterm_XA_CLIPBOARD;
EXTERN Atom     		aterm_XA_VT_SELECTION;
EXTERN Atom     		aterm_XA_INCR;		   



/*
 *****************************************************************************
 * PROTOTYPES
 *****************************************************************************
 */
#include "screen.h"

#ifdef PROTOTYPES
# define __PROTO(p)	p
#else
# define __PROTO(p)	()
#endif
#include "protos.h"

#ifdef DEBUG_MALLOC
# include "dmalloc.h"		/* This comes last */
#endif

#endif				/* _RXVT_H */
