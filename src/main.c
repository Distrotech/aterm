/*--------------------------------*-C-*---------------------------------*
 * File:	main.c
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
 *---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*
 * Originally written:
 *    1992      John Boyey, University of Canterbury
 * Modifications:
 *    1994      Robert Nation <nation@rocket.sanders.lockheed.com>
 *              - extensive modifications
 *    1995      Garrett D'Amore <garrett@netcom.com>
 *    1997      mj olesen <olesen@me.QueensU.CA>
 *              - extensive modifications
 *    1997,1998 Oezguer Kesim <kesim@math.fu-berlin.de>
 *    1998      Geoff Wing <gcw@pobox.com>
 *    1998      Sasha Vasko <sasha at aftercode.net>
 *----------------------------------------------------------------------*/

#ifndef lint
static const char rcsid[] = "$Id: main.c,v 1.33 2007/08/01 14:08:29 vae Exp $";
#endif

#define INTERN			/* assign all global vars to me */
#include "rxvt.h"		/* NECESSARY */
#include "X11/Xatom.h"
#include "X11/Xproto.h"
#include <locale.h>

Window ParentWin[PARENTS_NUM] = PARENTS_INIT;
int    ParentWinNum = 0;

#ifdef HAVE_AFTERIMAGE
ASVisual *asv = NULL ;
#endif

#ifdef HAVE_AFTERSTEP
void DeadPipe( int unused )
{
}	 
#else
Atom 			_XA_MwmAtom = None;
Atom 			_XA_NET_WM_PID = None;
Atom  			_XROOTPMAP_ID = None;
Atom  			_XA_NET_SUPPORTING_WM_CHECK = None;
Atom  			_XA_NET_SUPPORTED           = None;
Atom  			_XA_NET_CURRENT_DESKTOP     = None;
Atom 			_XA_NET_WM_DESKTOP			= None;
Atom 			_XA_NET_WM_STATE			= None;
Atom 			_XA_NET_WM_STATE_STICKY		= None;
Atom 			_XA_NET_WM_STATE_SHADED		= None;
Atom 			_XA_NET_WM_STATE_HIDDEN		= None;

#endif 

/*{{{ extern functions referenced */
#ifdef DISPLAY_IS_IP
extern char    *network_display(const char *display);
#endif
/*}}} */

/*{{{ local variables */
static Cursor   TermWin_cursor;	/* cursor for vt window */

static XSizeHints szHint =
{
    PMinSize | PResizeInc | PBaseSize | PWinGravity,
    0, 0, 80, 24,		/* x, y, width, height */
    1, 1,			/* Min width, height */
    0, 0,			/* Max width, height - unused */
    1, 1,			/* increments: width, height */
    {1, 1},			/* increments: x, y */
    {0, 0},			/* Aspect ratio - unused */
    0, 0,			/* base size: width, height */
    NorthWestGravity		/* gravity */
};

static const char *def_colorName[] =
{
    "White", "Black", 		/* fg/bg */
/* low-intensity colors */
    "Black",			/* 0: black             (#000000) */
#ifndef NO_BRIGHTCOLOR
    "Red3",			/* 1: red               (#CD0000) */
    "Green3",			/* 2: green             (#00CD00) */
    "Yellow3",			/* 3: yellow            (#CDCD00) */
    "Blue3",			/* 4: blue              (#0000CD) */
    "Magenta3",			/* 5: magenta           (#CD00CD) */
    "Cyan3",			/* 6: cyan              (#00CDCD) */
    "AntiqueWhite",		/* 7: white             (#FAEBD7) */
/* high-intensity colors */
    "Grey25",			/* 8: bright black      (#404040) */
#endif				/* NO_BRIGHTCOLOR */
    "Red",			/* 1/9: bright red      (#FF0000) */
    "Green",			/* 2/10: bright green   (#00FF00) */
    "Yellow",			/* 3/11: bright yellow  (#FFFF00) */
    "Blue",			/* 4/12: bright blue    (#0000FF) */
    "Magenta",			/* 5/13: bright magenta (#FF00FF) */
    "Cyan",			/* 6/14: bright cyan    (#00FFFF) */
    "White",			/* 7/15: bright white   (#FFFFFF) */
#ifndef NO_CURSORCOLOR
    NULL, NULL,
#endif				/* NO_CURSORCOLOR */
    NULL,			/* pointerColor                   */
    NULL			/* borderColor                    */
#if defined(TRANSPARENT) || defined(BACKGROUND_IMAGE)
    ,NULL
#endif
#ifndef NO_BOLDUNDERLINE
  , NULL, NULL
#endif				/* NO_BOLDUNDERLINE */
#ifdef KEEP_SCROLLCOLOR
  , "#B2B2B2",			/* scrollColor: `match' Netscape color */
    "#969696"			/* troughColor */
#endif
};

#ifdef MULTICHAR_SET
/* Multicharacter font names, roman fonts sized to match */
static const char *def_mfontName[] =
{
    MFONT_LIST
};
#endif				/* MULTICHAR_SET */

static const char *def_fontName[] =
{
    NFONT_LIST
};

/*}}} */

#if 0
/*----------------------------------------------------------------------*/
/* ARGSUSED */
/* PROTO */
XErrorHandler
xerror_handler(Display * display, XErrorEvent * event)
{
    print_error("XError: Request: %d . %d, Error: %d", event->request_code,
		event->minor_code, event->error_code);
/*
    if( *p )
	return 1 ;
*/
    if(  ( event->request_code != X_GetAtomName ) &&
	     ( event->request_code != X_GetGeometry || event->error_code != BadDrawable ) )
		exit(EXIT_FAILURE);
    return 0;
}

#else

XErrorHandler
xerror_handler (Display * dpy, XErrorEvent * event)
{
	char         *err_text;

    fprintf (stderr, "aterm has encountered the following problem interacting with X Windows :\n");
	if (event && dpy)
	{
        err_text = malloc (128);
		strcpy (err_text, "unknown error");
		XGetErrorText (dpy, event->error_code, err_text, 120);
		fprintf (stderr, "      Request: %d,    Error: %d(%s)\n", event->request_code, event->error_code, err_text);
		free (err_text);
		fprintf (stderr, "      in resource: 0x%lX\n", event->resourceid);
    }
	return 0;
}
#endif



#ifdef DEBUG_X
#define XGetGeometry(dpy,win,r,x,y,w,h,b,d) \
	trace_XGetGeometry(__FILE__,__LINE__,dpy,win,r,x,y,w,h,b,d)
#endif

/*{{{ color aliases, fg/bg bright-bold */
/* PROTO */
void
color_aliases(int idx)
{
    if (rs_color[idx] && isdigit(*rs_color[idx])) {
	int             i = atoi(rs_color[idx]);

	if (i >= 8 && i <= 15) {	/* bright colors */
	    i -= 8;
#ifndef NO_BRIGHTCOLOR
	    rs_color[idx] = rs_color[minBrightCOLOR + i];
	    return;
#endif
	}
	if (i >= 0 && i <= 7)	/* normal colors */
	    rs_color[idx] = rs_color[minCOLOR + i];
    }
}

/*
 * find if fg/bg matches any of the normal (low-intensity) colors
 */
#ifndef NO_BRIGHTCOLOR
/* PROTO */
void
set_colorfgbg(void)
{
    unsigned int    i;
    static char     colorfgbg_env[] = "COLORFGBG=default;default;bg";
    char           *p;
    int             fg = -1, bg = -1;

    for (i = Color_Black; i <= Color_White; i++) 
	{
		if (PixColors[Color_fg] == PixColors[i]) {
	    	fg = (i - Color_Black);
	    	break;
		}
    }
    for (i = Color_Black; i <= Color_White; i++) 
	{
		if (PixColors[Color_bg] == PixColors[i]) {
	    	bg = (i - Color_Black);
	    	break;
		}
    }

    p = strchr(colorfgbg_env, '=');
    p++;
    if (fg >= 0)
	sprintf(p, "%d;", fg);
    else
	STRCPY(p, "default;");
    p = strchr(p, '\0');
    if (bg >= 0)
	sprintf(p,"%d", bg);
    else
	STRCPY(p, "default");

    putenv(colorfgbg_env);

    colorfgbg = DEFAULT_RSTYLE;
    for (i = minCOLOR; i <= maxCOLOR; i++) {
	if (PixColors[Color_fg] == PixColors[i]
# ifndef NO_BOLDUNDERLINE
	    && PixColors[Color_fg] == PixColors[Color_BD]
# endif				/* NO_BOLDUNDERLINE */
    /* if we wanted boldFont to have precedence */
# if 0				/* ifndef NO_BOLDFONT */
	    && TermWin.boldFont == NULL
# endif				/* NO_BOLDFONT */
	    )
	    colorfgbg = SET_FGCOLOR(colorfgbg, i);
	if (PixColors[Color_bg] == PixColors[i])
	    colorfgbg = SET_BGCOLOR(colorfgbg, i);
    }
}
#else				/* NO_BRIGHTCOLOR */
# define set_colorfgbg() ((void)0)
#endif				/* NO_BRIGHTCOLOR */
/*}}} */

/* PROTO */
void
set_terminal_size( unsigned int new_ncol, unsigned int new_nrow )
{
    if( new_nrow <= 0) new_nrow = 24 ;
    if( new_ncol <= 0) new_ncol = 80 ;
    TermWin.ncol = new_ncol;
    TermWin.nrow = new_nrow;
    TermWin.bcol = new_ncol;
    MAX_IT(TermWin.bcol, TermWin.min_bcol);
}


/*{{{ set_cursor_color() - Updates color of the TerWin_cursor */
/* PROTO */
void
set_cursor_color()
{
    XColor          fg, bg;

    fg.pixel = PixColorsFocused[Color_pointer];
    XQueryColor(Xdisplay, Xcmap, &fg);
    bg.pixel = PixColorsFocused[Color_bg];
    XQueryColor(Xdisplay, Xcmap, &bg);
    XRecolorCursor(Xdisplay, TermWin_cursor, &fg, &bg);
}
/*}}} */
#if defined(BACKGROUND_IMAGE)
int SetBackgroundPixmap(char* PixmapSpec)
{
  char *geometry = strchr(PixmapSpec, ';');
  int bChanged = 0 ;

    if (geometry != NULL)
    {
        *geometry = '\0' ; /* so to get clean pixmap filename */
        bChanged = parse_pixmap_geom(geometry+1);
    }
    LoadBGPixmap(PixmapSpec);
#ifdef _MYSTYLE_
    if( TermWin.background.trgType == BGT_MyStyle )
		TermWin.background.trgType = BGT_Tile ;
#endif

    if( geometry ) *geometry = ';' ;
    return bChanged ;
}
#endif

#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT) || defined(_MYSTYLE_)

void SetBackgroundType( const char *type )
{

#ifdef _MYSTYLE_
    if( TermWin.background.trgType != BGT_MyStyle )
#endif
	TermWin.background.trgType = BGT_Tile ;

#ifdef TRANSPARENT
    if( (Options & Opt_transparent) )
    {
		if( TermWin.background.Shading.shading != 100)
	    	TermWin.background.trgType = BGT_Cut ;
		else
	    	TermWin.background.trgType = BGT_None ;
    }
#endif
    if( type == NULL || TermWin.background.trgType != BGT_Tile) 
		return ;

    if( strcmp( type, BGT_CENTER ) == 0 )
		TermWin.background.trgType = BGT_Center ;
    else if( strcmp( type, BGT_SCALE ) == 0 )
		TermWin.background.trgType = BGT_Scale ;
    else if( strcmp( type, BGT_SCALEH ) == 0 )
		TermWin.background.trgType = BGT_ScaleH ;
    else if( strcmp( type, BGT_SCALEV ) == 0 )
		TermWin.background.trgType = BGT_ScaleV ;
    else if( strcmp( type, BGT_NO_TILE ) == 0 )
		TermWin.background.trgType = BGT_NoTile ;
    else if( strcmp( type, BGT_NO_TILE_H ) == 0 )
		TermWin.background.trgType = BGT_NoTileH ;
    else if( strcmp( type, BGT_NO_TILE_V ) == 0 )
		TermWin.background.trgType = BGT_NoTileV ;
    else if( strcmp( type, BGT_CUT ) == 0 )
		TermWin.background.trgType = BGT_Cut ;
}

void InitBackground()
{
    TermWin.background.srcPixmap = None ;
    TermWin.background.mystyle = NULL ;
    TermWin.background.user_flags = 0 ;
    TermWin.background.bMySource = 0 ;
    TermWin.background.trgPixmap = None ;
    TermWin.background.trgPixmapSet = 0 ;
    TermWin.background.Width = 0 ;
    TermWin.background.Height = 0 ;
    TermWin.background.srcWidth = -1 ;
    TermWin.background.srcHeight = -1 ;
    TermWin.background.srcX = -1 ;
    TermWin.background.srcY = -1 ;
#ifdef SCALING_GEOM_ENABLED
    TermWin.background.trgWidth = -1 ;
    TermWin.background.trgHeight = -1 ;
    TermWin.background.trgX = -1 ;
    TermWin.background.trgY = -1 ;
#endif
    TermWin.background.finWidth = 0 ;
    TermWin.background.finHeight = 0 ;
    TermWin.background.cutX = -1 ;
    TermWin.background.cutY = -1 ;

    /* some defaults here */
    INIT_SHADING(TermWin.background.Shading)

}
#endif

int ParseGCType( const char* type, int def_type )
{
    if( !type ) return def_type ;

    if( strcmp( type, GC_TYPE_AND )== 0 ) return GXand;
    else if( strcmp( type, GC_TYPE_AND_REV )== 0 ) return GXandReverse;
    else if( strcmp( type, GC_TYPE_AND_INV )== 0 ) return GXandInverted;
    else if( strcmp( type, GC_TYPE_XOR	)== 0 ) return GXxor;
    else if( strcmp( type, GC_TYPE_OR	)== 0 ) return GXor;
    else if( strcmp( type, GC_TYPE_NOR )== 0 ) return GXnor;
    else if( strcmp( type, GC_TYPE_INVERT )== 0 ) return GXinvert;
    else if( strcmp( type, GC_TYPE_EQUIV )== 0 ) return GXequiv;
    else if( strcmp( type, GC_TYPE_INVERT )== 0 ) return GXinvert ;
    else if( strcmp( type, GC_TYPE_OR_REV )== 0 ) return GXorReverse;
    else if( strcmp( type, GC_TYPE_OR_INV )== 0 ) return GXorInverted ;
    else if( strcmp( type, GC_TYPE_NAND )== 0 ) return GXnand	;
    return def_type ;
}

#ifdef OFF_FOCUS_FADING
unsigned long fade_color(unsigned long pixel)
{
    if( rs_fade )
    {/* make unfocused colors here */
      XColor          faded_xcol;
      int fade = 0;

        fade = atoi( rs_fade );
        faded_xcol.pixel = pixel ;
	XQueryColor( Xdisplay, Xcmap, &faded_xcol );
        faded_xcol.red   = (faded_xcol.red/100)*fade ;
        faded_xcol.green = (faded_xcol.green/100)*fade ;
        faded_xcol.blue  = (faded_xcol.blue/100)*fade ;

        if( XAllocColor(Xdisplay, Xcmap, &faded_xcol) )
    	    return faded_xcol.pixel ;
    }
    return pixel ;
}
#endif

/*{{{ Create_Windows() - Open and map the window */
/* PROTO */
void
Create_Windows(int argc, char *argv[])
{
    Cursor          cursor;
    XClassHint      classHint;
    XWMHints        wmHint;
    MwmHints 		mwmhints;
    int             i, x, y, flags;
    unsigned int    width, height;
    XSetWindowAttributes attributes;
	unsigned long attr_mask ;

#ifdef PREFER_24BIT
    XWindowAttributes gattr;
        Xcmap = DefaultColormap(Xdisplay, Xscreen);
        Xvisual = DefaultVisual(Xdisplay, Xscreen);

    if (Options & Opt_transparent) {
        XGetWindowAttributes(Xdisplay, RootWindow(Xdisplay, Xscreen), &gattr);
        Xdepth = gattr.depth;
    } else {
        Xdepth = DefaultDepth(Xdisplay, Xscreen);
 /*
  * If depth is not 24, look for a 24bit visual.
  */
        if (Xdepth != 24) {
             XVisualInfo        vinfo;

             if (XMatchVisualInfo(Xdisplay, Xscreen, 24, TrueColor, &vinfo)) {
                Xdepth = 24;
                Xvisual = vinfo.visual;
                Xcmap = XCreateColormap(Xdisplay,
                                        RootWindow(Xdisplay, Xscreen), Xvisual,
                                        AllocNone);
             }
	}
    }
#endif

    if (Options & Opt_borderLess) 
	{
    	if (_XA_MwmAtom == None) {
/*     print_warning("Window Manager does not support MWM hints.  Bypassing window manager control for borderless window.\n");*/
       		attributes.override_redirect = TRUE;
       		mwmhints.flags = 0;
    	} else 
		{
    		mwmhints.flags = MWM_HINTS_DECORATIONS;
			mwmhints.decorations = 0;
    	}
    } else 
	{
    	mwmhints.flags = 0;
    }


/*
 * grab colors before netscape does
 */
    PixColors = &(PixColorsFocused[0]);

    for (i = 0;
	 i < (Xdepth <= 2 ? 2 : NRS_COLORS);
	 i++) {
	const char     *const msg = "can't load color \"%s\", colorID = %d, (%d)";
	XColor          xcol;

	if (!rs_color[i])
	    continue;

	if (!XParseColor(Xdisplay, Xcmap, rs_color[i], &xcol) ||
	    !XAllocColor(Xdisplay, Xcmap, &xcol))
	{
	    print_error(msg, rs_color[i], i, TOTAL_COLORS);
	    rs_color[i] = def_colorName[i];
	    if (!rs_color[i])
		continue;
	    if (!XParseColor(Xdisplay, Xcmap, rs_color[i], &xcol) ||
		!XAllocColor(Xdisplay, Xcmap, &xcol))
	    {
		print_error(msg, rs_color[i], i, TOTAL_COLORS);
		switch (i) {
		case Color_fg:
		case Color_bg:
		/* fatal: need bg/fg color */
		    print_error("aborting");
		    exit(EXIT_FAILURE);
		    break;
#ifndef NO_CURSORCOLOR
		case Color_cursor:
		    xcol.pixel = PixColors[Color_bg];
		    break;
		case Color_cursor2:
		    xcol.pixel = PixColors[Color_fg];
		    break;
#endif				/* NO_CURSORCOLOR */
		case Color_pointer:
		    xcol.pixel = PixColors[Color_fg];
		    break;
#if defined(TRANSPARENT) || defined(BACKGROUND_IMAGE)
		case Color_tint:
		    xcol.pixel = PixColors[Color_bg];
		    break;
#endif
		default:
		    xcol.pixel = PixColors[Color_bg];	/* None */
		    break;
		}
		XQueryColor( Xdisplay, Xcmap,&xcol);
	    }
	}
	PixColors[i] = xcol.pixel;
#ifdef OFF_FOCUS_FADING
	PixColorsUnFocused[i] = fade_color(xcol.pixel);
#endif
#if defined(TRANSPARENT) || defined(BACKGROUND_IMAGE)
	if( i == Color_tint )
	{
	    TermWin.background.Shading.tintColor.pixel = xcol.pixel ;
	    TermWin.background.Shading.tintColor.red = xcol.red ;
	    TermWin.background.Shading.tintColor.green = xcol.green ;
	    TermWin.background.Shading.tintColor.blue = xcol.blue ;
	    TermWin.background.Shading.tintColor.flags = xcol.flags ;
	}
#endif

    }

    if (Xdepth <= 2 || !rs_color[Color_pointer])
	PixColors[Color_pointer] = PixColors[Color_fg];
    if (Xdepth <= 2 || !rs_color[Color_border])
	PixColors[Color_border] = PixColors[Color_fg];

/*
 * get scrollBar/menuBar shadow colors
 *
 * The calculations of topShadow/bottomShadow values are adapted
 * from the fvwm window manager.
 */
#ifdef KEEP_SCROLLCOLOR
    if (Xdepth <= 2) {		/* Monochrome */
	PixColors[Color_scroll] = PixColors[Color_fg];
	PixColors[Color_topShadow] = PixColors[Color_bg];
	PixColors[Color_bottomShadow] = PixColors[Color_bg];
    } else {
	XColor          xcol, white;

    /* bottomShadowColor */
	xcol.pixel = PixColors[Color_scroll];
	XQueryColor(Xdisplay, Xcmap, &xcol);

	xcol.red = ((xcol.red) / 2);
	xcol.green = ((xcol.green) / 2);
	xcol.blue = ((xcol.blue) / 2);

	if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
	    print_error("can't allocate %s", "Color_bottomShadow");
	    xcol.pixel = PixColors[minCOLOR];
	}
	PixColors[Color_bottomShadow] = xcol.pixel;

    /* topShadowColor */
# ifdef PREFER_24BIT
	white.red = white.green = white.blue = (unsigned short) ~0;
	XAllocColor(Xdisplay, Xcmap, &white);
/*        XFreeColors(Xdisplay, Xcmap, &white.pixel, 1, ~0); */
# else
	white.pixel = WhitePixel(Xdisplay, Xscreen);
	XQueryColor(Xdisplay, Xcmap, &white);
# endif

	xcol.pixel = PixColors[Color_scroll];
	XQueryColor(Xdisplay, Xcmap, &xcol);

	xcol.red = max((white.red / 5), (int)xcol.red);
	xcol.green = max((white.green / 5), (int)xcol.green);
	xcol.blue = max((white.blue / 5), (int)xcol.blue);

	xcol.red = min((int)white.red, (xcol.red * 7) / 5);
	xcol.green = min((int)white.green, (xcol.green * 7) / 5);
	xcol.blue = min((int)white.blue, (xcol.blue * 7) / 5);

	if (!XAllocColor(Xdisplay, Xcmap, &xcol)) {
	    print_error("can't allocate %s", "Color_topShadow");
	    xcol.pixel = PixColors[Color_White];
	}
	PixColors[Color_topShadow] = xcol.pixel;
    }
#endif				/* KEEP_SCROLLCOLOR */

    szHint.base_width = (2 * TermWin_internalBorder +
			 (Options & Opt_scrollBar ? (SB_WIDTH + 2 * sb_shadow)
			  : 0));
    szHint.base_height = (2 * TermWin_internalBorder);

    flags = (rs_geometry ?
	     XParseGeometry(rs_geometry, &x, &y, &width, &height) : 0);

    if (flags & WidthValue) {
	szHint.width = width;
	szHint.flags |= USSize;
    }
    if (flags & HeightValue) {
	szHint.height = height;
	szHint.flags |= USSize;
    }

    set_terminal_size( szHint.width, szHint.height );

    change_font(1, NULL);

    { /* ONLYIF MENUBAR */
	szHint.base_height += (delay_menu_drawing ? menuBar_TotalHeight() : 0);
    }

    if (flags & XValue) {
	if (flags & XNegative) {
	    x += (DisplayWidth(Xdisplay, Xscreen)
		  - (szHint.width + TermWin_internalBorder));
	    szHint.win_gravity = NorthEastGravity;
	}
	szHint.x = x;
	szHint.flags |= USPosition;
    }
    if (flags & YValue) {
	if (flags & YNegative) {
	    y += (DisplayHeight(Xdisplay, Xscreen)
		  - (szHint.height + TermWin_internalBorder));
	    szHint.win_gravity = (szHint.win_gravity == NorthEastGravity ?
				  SouthEastGravity : SouthWestGravity);
	}
	szHint.y = y;
	szHint.flags |= USPosition;
    }
/* parent window - reverse video so we can see placement errors
 * sub-window placement & size in resize_subwindows()
 */
    attributes.background_pixel = PixColors[Color_bg];
    attributes.border_pixel = PixColors[Color_fg];
	attributes.event_mask = ( KeyPressMask | 
							  FocusChangeMask |
		  					  StructureNotifyMask | 
							  VisibilityChangeMask |
							  PropertyChangeMask);
    attributes.colormap = Xcmap;
	attributes.background_pixmap = ParentRelative;
	attr_mask = CWBorderPixel | CWEventMask ; 
#ifdef HAVE_AFTERSTEP
	if( TermWin.background.trgType == BGT_MyStyle )
		attr_mask |= CWBackPixmap ; 
#endif		
#ifdef TRANSPARENT
	if( get_flags(Options, Opt_transparent) )
		attr_mask |= CWBackPixmap ; 
#endif
	if( (attr_mask & CWBackPixmap ) == 0 )
	  	attr_mask |= CWBackPixel ; 	 

#ifdef HAVE_AFTERIMAGE
    TermWin.parent = create_visual_window( asv, Xroot, 
					   szHint.x, szHint.y,
					   szHint.width, szHint.height,
                       TermWin.borderWidth,
					   InputOutput,
					   attr_mask,
					   &attributes);

#else
	attr_mask |= CWColormap ;
    TermWin.parent = XCreateWindow(Xdisplay, Xroot,
				   szHint.x, szHint.y,
				   szHint.width, szHint.height,
                                   TermWin.borderWidth,
				   Xdepth, InputOutput,
				   Xvisual,
				   attr_mask,
				   &attributes);
#endif
    TermWin.bMapped = 0 ;
    ParentWin[0] = TermWin.parent ;
    ParentWinNum = 1 ;

    xterm_seq(XTerm_title, rs_title);
    xterm_seq(XTerm_iconName, rs_iconName);
/* ignore warning about discarded `const' */
    classHint.res_name = (char *)rs_name;
    classHint.res_class = APL_CLASS;
    wmHint.input = True;
    wmHint.initial_state = (Options & Opt_iconic ? IconicState : NormalState);
    wmHint.window_group = TermWin.parent;
    wmHint.flags = (InputHint | StateHint | WindowGroupHint);

    XSetWMProperties(Xdisplay, TermWin.parent, NULL, NULL, argv, argc,
		     &szHint, &wmHint, &classHint);

	/* publish our PID : */
	{
		long ldata = getpid();
        XChangeProperty (Xdisplay, TermWin.parent, _XA_NET_WM_PID, XA_CARDINAL, 32,
                         PropModeReplace, (unsigned char *)&ldata, 1);
	}


    if (mwmhints.flags && _XA_MwmAtom) {
    	XChangeProperty(Xdisplay, TermWin.parent, _XA_MwmAtom, _XA_MwmAtom, 32, PropModeReplace, (unsigned char *) &mwmhints, PROP_MWM_HINTS_ELEMENTS);
    }
   

/* vt cursor: Black-on-White is standard, but this is more popular */
    TermWin_cursor = XCreateFontCursor(Xdisplay, XC_xterm);
    {
		XColor          fg, bg;

		fg.pixel = PixColors[Color_pointer];
		XQueryColor(Xdisplay, Xcmap, &fg);
		bg.pixel = PixColors[Color_bg];
		XQueryColor(Xdisplay, Xcmap, &bg);
		XRecolorCursor(Xdisplay, TermWin_cursor, &fg, &bg);
    }

/* cursor (menuBar/scrollBar): Black-on-White */
    cursor = XCreateFontCursor(Xdisplay, XC_left_ptr);
	attributes.event_mask = (ExposureMask | 
							 ButtonPressMask | 
							 ButtonReleaseMask |
		  					 Button1MotionMask | 
							 Button3MotionMask);
	attributes.cursor = TermWin_cursor ;
/* the vt window */
#ifdef HAVE_AFTERIMAGE
    TermWin.vt = create_visual_window( asv, TermWin.parent,
				     	0, 0,
				     	szHint.width, szHint.height,
					   	0,
					   	InputOutput,
					   	attr_mask|CWCursor,
					   	&attributes);
#else
    TermWin.vt = XCreateSimpleWindow(Xdisplay, TermWin.parent,
				     0, 0,
				     szHint.width, szHint.height,
				     0,
				     PixColors[Color_fg],
				     PixColors[Color_bg]);
    XSelectInput(Xdisplay, TermWin.vt, attributes.event_mask );
	XDefineCursor(Xdisplay, TermWin.vt, TermWin_cursor);
#endif
    

#ifdef TRANSPARENT
    if (get_flags(Options, Opt_transparent)) {
		if( TermWin.background.trgType != BGT_None )
		    SetSrcPixmap(GetRootPixmap(None));
		else
			XSetWindowBackgroundPixmap(Xdisplay, TermWin.vt, ParentRelative);
    }
#endif
    /* added by Sasha Vasko to enabling Tracking of the Background changes */
#if defined(TRANSPARENT)||defined(_MYSTYLE_)
#if !defined(_MYSTYLE_)
    if ((Options & Opt_transparent)||(Options&Opt_transparent_sb))
#endif
	XSelectInput( Xdisplay, Xroot, PropertyChangeMask );

#endif

/* scrollBar: size doesn't matter */
#ifdef HAVE_AFTERIMAGE
    scrollBar.win = create_visual_window( asv, TermWin.parent,
						0, 0,
						1, 1,
						0,
					   	InputOutput,
					   	CWBackPixel | CWBorderPixel,
					   	&attributes);
#else    
	scrollBar.win = XCreateSimpleWindow(Xdisplay, TermWin.parent,
						0, 0,
						1, 1,
						0,
					PixColors[Color_fg],
					PixColors[Color_bg]);
#endif

    XDefineCursor(Xdisplay, scrollBar.win, cursor);
    XSelectInput(Xdisplay, scrollBar.win,
		 (ExposureMask | ButtonPressMask | ButtonReleaseMask |
		  Button1MotionMask | Button2MotionMask | Button3MotionMask)
	);

    { /* ONLYIF MENUBAR */
	create_menuBar(cursor);
    }
#if defined(BACKGROUND_IMAGE)
    if (rs_backgroundPixmap != NULL
#ifdef TRANSPARENT
         && !(Options & Opt_transparent)
#endif
       )
        SetBackgroundPixmap((char*)rs_backgroundPixmap);
#endif
/* graphics context for the vt window */
    {
	XGCValues       gcvalue;
	gcvalue.font = TermWin.font->fid;
	gcvalue.foreground = PixColors[Color_fg];
	gcvalue.background = PixColors[Color_bg];
	gcvalue.function = ParseGCType(rs_textType, GXcopy);
	gcvalue.graphics_exposures = 0;
#ifdef HAVE_AFTERIMAGE
	TermWin.gc = create_visual_gc(asv, TermWin.vt,
			       GCFunction|
			       GCForeground | GCBackground |
			       GCFont | GCGraphicsExposures,
			       &gcvalue);
#else
	TermWin.gc = XCreateGC(Xdisplay, TermWin.vt,
			       GCFunction|
			       GCForeground | GCBackground |
			       GCFont | GCGraphicsExposures,
			       &gcvalue);
#endif
#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT)
	if( rs_color[Color_tint] )
        {
	    if( rs_tintType )
	    {
		if( strcmp( rs_tintType, TINT_TYPE_TRUE) == 0 )
		{
		    TermWin.tintGC = None ;
		    if( TermWin.background.trgType == BGT_None )
				TermWin.background.trgType = BGT_Cut ;
		}

	    }
	    if( TermWin.background.trgType == BGT_None )
	    {
	        gcvalue.function = ParseGCType(rs_tintType, GXand);
	        gcvalue.foreground = PixColors[Color_tint];
#ifdef HAVE_AFTERIMAGE
    	        TermWin.tintGC = create_visual_gc( asv, TermWin.vt,
							    			GCFunction|GCForeground|GCGraphicsExposures,
											&gcvalue);
#else
    	        TermWin.tintGC = XCreateGC(	Xdisplay, TermWin.vt,
							    			GCFunction|GCForeground|GCGraphicsExposures,
											&gcvalue);
#endif
	    }
        }
#endif
    }
}
/*}}} */
/*{{{ window resizing - assuming the parent window is the correct size */
/* PROTO */
Bool
resize_subwindows(int width, int height)
{
    int             x = 0, y = 0;
    int             old_width = TermWin.width;
    int             old_height = TermWin.height;

    TermWin.width = TermWin.ncol * TermWin.fwidth;
    TermWin.height = TermWin.nrow * TermWin.fheight;

/* size and placement */
    if (scrollbar_visible()) {
	scrollBar.beg = 0;
	scrollBar.end = height;
#ifndef XTERM_SCROLLBAR
# ifdef NEXT_SCROLLBAR
    /* arrows can be different */
	scrollBar.end -= GetScrollArrowsHeight();
# else
    /* arrows are as high as wide - leave 1 pixel gap */
	scrollBar.beg += (SB_WIDTH + 1) + sb_shadow;
	scrollBar.end -= (SB_WIDTH + 1) + sb_shadow;
# endif
#endif

	width -= (SB_WIDTH + 2 * sb_shadow);
	if (Options & Opt_scrollBar_right)
	    XMoveResizeWindow(Xdisplay, scrollBar.win, width, 0,
			      (SB_WIDTH + 2 * sb_shadow), height);
	else {
	    XMoveResizeWindow(Xdisplay, scrollBar.win, 0, 0,
			      (SB_WIDTH + 2 * sb_shadow), height);
	    x = (SB_WIDTH + 2 * sb_shadow);	/* placement of vt window */
	}
    }
    { /* ONLYIF MENUBAR */
	if (menubar_visible()) {
	    y = menuBar_TotalHeight();	/* for placement of vt window */
	    Resize_menuBar(x, 0, width, y);
	}
    }
    XMoveResizeWindow(Xdisplay, TermWin.vt, x, y, width, height + 1);
	
	request_background_update();

    if (old_width)
	Gr_Resize(old_width, old_height);

    XSync(Xdisplay, 0);
	return (width != old_width || height != old_height );
}

/* PROTO */
void
resize(void)
{
    szHint.base_width = (2 * TermWin_internalBorder);
    szHint.base_height = (2 * TermWin_internalBorder);

    szHint.base_width += (scrollbar_visible() ? (SB_WIDTH + 2 * sb_shadow) : 0);
    { /* ONLYIF MENUBAR */
	szHint.base_height += (menubar_visible() ? menuBar_TotalHeight() : 0);
    }

    szHint.min_width = szHint.base_width + szHint.width_inc;
    szHint.min_height = szHint.base_height + szHint.height_inc;

    szHint.width = szHint.base_width + TermWin.width;
    szHint.height = szHint.base_height + TermWin.height;

    szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;

    XSetWMNormalHints(Xdisplay, TermWin.parent, &szHint);
    XResizeWindow(Xdisplay, TermWin.parent, szHint.width, szHint.height);

    resize_subwindows(szHint.width, szHint.height);
    scr_clear();

}

/*
 * Redraw window after exposure or size change
 */
/* PROTO */
void
resize_window1(unsigned int width, unsigned int height)
{
    static short    first_time = 1;
    int             new_ncol = (width - szHint.base_width) / TermWin.fwidth;
    int             new_nrow = (height - szHint.base_height) / TermWin.fheight;

    if (first_time ||
	(new_ncol != TermWin.ncol) ||
	(new_nrow != TermWin.nrow)) {
	int             curr_screen = -1;

    /* scr_reset only works on the primary screen */
	if (!first_time) {	/* this is not the first time thru */
	    selection_clear();
	    curr_screen = scr_change_screen(PRIMARY);
	}

	set_terminal_size( new_ncol, new_nrow);

	resize_subwindows(width, height);
	scr_reset();

	if (curr_screen >= 0)	/* this is not the first time thru */
	    scr_change_screen(curr_screen);
	first_time = 0;
    }
}

/*
 * good for toggling 80/132 columns
 */
/* PROTO */
void
set_width(unsigned short width)
{
    unsigned short  height = TermWin.nrow;

    if (width != TermWin.ncol) {
	width = szHint.base_width + width * TermWin.fwidth;
	height = szHint.base_height + height * TermWin.fheight;

	XResizeWindow(Xdisplay, TermWin.parent, width, height);
	resize_window1(width, height);
#ifdef USE_XIM
        IMSetStatusPosition();
#endif
	scr_clear();
    }
}

/*
 * Redraw window after exposure or size change
 */
/* PROTO */
void
resize_window(XEvent* ev)
{
	int root_x = 0, root_y = 0; 
	Window        wdumm;
	XConfigureEvent *xconf = &(ev->xconfigure);
	
	while( XCheckTypedWindowEvent( Xdisplay, TermWin.parent, ConfigureNotify, ev ) );
	/*fprintf( stderr, "config_geom = %dx%d\n", xconf->width, xconf->height );*/
    resize_window1(xconf->width, xconf->height);
#if 1
	XTranslateCoordinates (Xdisplay, TermWin.parent, Xroot, 0, 0, &root_x, &root_y, &wdumm);

	/*fprintf( stderr, "root_geom = %dx%d%+d%+d, root_size = %dx%d\n", xconf->width, xconf->height, root_x, root_y, XdisplayWidth, XdisplayHeight ); */
	TermWin.root_x = root_x ; 
	TermWin.root_y = root_y ; 
	TermWin.root_width = xconf->width ; 
	TermWin.root_height = xconf->height ; 
	
	if( TransparentPixmapNeedsUpdate() )
		request_background_update();
	else
	{	
		Bool cancel = True ;
		Bool request = False ;

#ifdef _MYSTYLE_
		if( TermWin.background.trgType == BGT_MyStyle ) 
		{	
			cancel = False ;	
			if( TermWin.LastPixmap_width  != TermWin.root_width ||
				TermWin.LastPixmap_height != TermWin.root_height )
				request = True;
		}

#endif	
#ifdef HAVE_AFTERIMAGE
		if( TermWin.background.trgType == BGT_Scale || 
			TermWin.background.trgType == BGT_ScaleH || 
			TermWin.background.trgType == BGT_ScaleV || 
			TermWin.background.trgType == BGT_Cut ) 
		{
			cancel = False ;	
			if( TermWin.LastPixmap_width  != TermWin.root_width ||
				TermWin.LastPixmap_height != TermWin.root_height )
				request = True;
		}
#endif	
		if( cancel ) 	   
			cancel_background_update();
		else if( request )
			request_background_update();
	}
	
#else 
	request_background_update();
#endif
#if 0
#if defined(TRANSPARENT) || defined(BACKGROUND_IMAGE) || defined(_MYSTYLE_)
    XGetGeometry(Xdisplay, TermWin.vt,
		 &root, &x, &y, &vt_width, &vt_height, &border, &depth);

    refresh_transparent_scrollbar();

    if( (TermWin.bMapped && (Options & Opt_transparent)) ||
         TermWin.background.trgType != BGT_None )
    {
	  int abs_x, abs_y;
          static int old_abs_x=0, old_abs_y=0,
	             old_width=0, old_height=0;

	    if( GetMyPosition(&abs_x, &abs_y))
	    {
/*	    fprintf( stderr, "\naterm:resize_windows():old = %dx%d+%d+%d, new = %dx%d+%d+%d",
		     old_width, old_height, old_abs_x, old_abs_y,
		     vt_width, vt_height, abs_x, abs_y   );
*/
#ifdef _MYSTYLE_
	        if( TermWin.background.trgType == BGT_MyStyle )
		{ /* we want to avoid unneeded redraws if we
		     only have gradients */
		    if( TransparentMS(TermWin.background.mystyle) || vt_width != old_width || vt_height!= old_height )
			{
			    RenderPixmap(1);
			    old_width = vt_width ;
			    old_height = vt_height ;
			}
		    RenderPixmap(1);
	        scr_clear();
			scr_touch();
			return ;
		}
#endif
		if( TermWin.background.trgType != BGT_None )
		{
		    ValidateSrcPixmap( 1 );
	    	    if( abs_x != old_abs_x || abs_y != old_abs_y ||
	    		vt_width != old_width || vt_height!= old_height ||
			TermWin.LastPixmapUsed != TermWin.background.srcPixmap )
		    {
    			RenderPixmap(1);
			/* we've already validated source pixmap
			   of course user can change it in any time
			   but for performance considerations we
			   don't want to recheck it*/
    			scr_clear();
            		scr_touch();

			old_width = vt_width ;
			old_height = vt_height ;
            		old_abs_x = abs_x ;
			old_abs_y = abs_y ;
		    }
		}
#ifdef TRANSPARENT
		else if( abs_x != old_abs_x || abs_y != old_abs_y )
		{
		    scr_clear();
        	    scr_touch();

        	    old_abs_x = abs_x ;
		    old_abs_y = abs_y ;
		}
#endif
	    }
        }
    }
    else
#endif
#endif
    {
        scr_clear();
        scr_touch();
    }
}
/*}}} */
/*{{{ xterm sequences - title, iconName, color (exptl) */
#ifdef SMART_WINDOW_TITLE
/* PROTO */
void
set_title(const char *str)
{
    char           *name;

    if (XFetchName(Xdisplay, TermWin.parent, &name))
	name = NULL;
    if (name == NULL || strcmp(name, str))
	XStoreName(Xdisplay, TermWin.parent, str);
    if (name)
	XFree(name);
}
#else
# define set_title(str)	XStoreName (Xdisplay, TermWin.parent, str)
#endif

#ifdef SMART_WINDOW_TITLE
/* PROTO */
void
set_iconName(const char *str)
{
    char           *name;

    if (XGetIconName(Xdisplay, TermWin.parent, &name))
	name = NULL;
    if (name == NULL || strcmp(name, str))
	XSetIconName(Xdisplay, TermWin.parent, str);
    if (name)
	XFree(name);
}
#else
# define set_iconName(str) XSetIconName (Xdisplay, TermWin.parent, str)
#endif

#ifdef XTERM_COLOR_CHANGE
/* PROTO */
void
set_window_color(int idx, const char *color)
{
    const char     *const msg = "can't load color \"%s\"";
    XColor          xcol;
    int             i;

    if (color == NULL || *color == '\0')
	return;

/* handle color aliases */
    if (isdigit(*color)) {
	i = atoi(color);
	if (i >= 8 && i <= 15) {	/* bright colors */
	    i -= 8;
# ifndef NO_BRIGHTCOLOR
	    PixColorsFocused[idx] = PixColorsFocused[minBrightCOLOR + i];
	    goto Done;
# endif
	}
	if (i >= 0 && i <= 7) {	/* normal colors */
	    PixColorsFocused[idx] = PixColorsFocused[minCOLOR + i];
	    goto Done;
	}
    }
    if (!XParseColor(Xdisplay, Xcmap, color, &xcol) ||
	!XAllocColor(Xdisplay, Xcmap, &xcol)) {
	print_error(msg, color);
	return;
    }
/* XStoreColor (Xdisplay, Xcmap, XColor*); */

/*
 * FIXME: should free colors here, but no idea how to do it so instead,
 * so just keep gobbling up the colormap
 */
# if 0
    for (i = Color_Black; i <= Color_White; i++)
	if (PixColorsFocused[idx] == PixColorsFocused[i])
	    break;
    if (i > Color_White) {
    /* fprintf (stderr, "XFreeColors: PixColors [%d] = %lu\n", idx, PixColors [idx]); */
	XFreeColors(Xdisplay, Xcmap, (PixColorsFocused + idx), 1,
		    DisplayPlanes(Xdisplay, Xscreen));
    }
# endif

    PixColorsFocused[idx] = xcol.pixel;

/* XSetWindowAttributes attr; */
/* Cursor cursor; */
  Done:
#ifdef OFF_FOCUS_FADING
    PixColorsUnFocused[idx] = fade_color(PixColorsFocused[idx]);
#endif
    on_colors_changed(idx);
}
#else
# define set_window_color(idx,color)	((void)0)
#endif				/* XTERM_COLOR_CHANGE */

/*
 * XTerm escape sequences: ESC ] Ps;Pt BEL
 *       0 = change iconName/title
 *       1 = change iconName
 *       2 = change title
 *       4 = change color
 *      12 = change cursor color
 *      13 = change mouse foreground color 
 *      18 = change bold character color
 *      19 = change underlined character color 
 *      46 = change logfile (not implemented)
 *      50 = change font
 *
 * rxvt extensions:
 *      10 = menu
 *      20 = bg pixmap
 *      39 = change default fg color
 *      49 = change default bg color
 */
/* PROTO */
void
xterm_seq(int op, const char *str)
{
    int color;
    char *buf, *name;

    assert(str != NULL);
    switch (op) {
    case XTerm_name:
	set_title(str);		/* drop */
    case XTerm_iconName:
	set_iconName(str);
	break;
    case XTerm_title:
	set_title(str);
	break;
    case XTerm_Color:
	for (buf = (char *)str; buf && *buf;) {
	    if ((name = strchr(buf, ';')) == NULL)
		break;
	    *name++ = '\0';
	    color = atoi(buf);
	    if (color < 0 || color >= TOTAL_COLORS)
		break;
	    if ((buf = strchr(name, ';')) != NULL)
		*buf++ = '\0';
	    set_window_color(color + minCOLOR, name);
	}
	break;
#ifndef NO_CURSORCOLOR
    case XTerm_Color_cursor:
	set_window_color(Color_cursor, str);
	break;
#endif
    case XTerm_Color_pointer:
	set_window_color(Color_pointer, str);
	break;
#ifndef NO_BOLD_UNDERLINE
    case XTerm_Color_BD:
	set_window_color(Color_BD, str);
	break;
    case XTerm_Color_UL:
	set_window_color(Color_UL, str);
	break;
#endif

    case XTerm_Menu:
    /*
     * menubar_dispatch() violates the constness of the string,
     * so DON'T do it here
     */
	break;
    case XTerm_Pixmap:
#if defined(BACKGROUND_IMAGE)
        if( SetBackgroundPixmap((char*)str) )
			request_background_update();
#endif
	break;

    case XTerm_restoreFG:
	set_window_color(Color_fg, str);
	break;
    case XTerm_restoreBG:
	set_window_color(Color_bg, str);
	break;
    case XTerm_logfile:
	break;
    case XTerm_font:
	change_font(0, str);
	break;
    }
}
/*}}} */


unsigned int get_proportion_font_width( XFontStruct *font )
{
  int i, cw ;
  unsigned int fw = 0;
    if( font != NULL )
    {
	for (i = font->max_char_or_byte2 -
	         font->min_char_or_byte2 ;
	     i >= 0; i--)
	{
	    cw = font->per_char[i].width;
	    if( cw > 0 )
    	        MAX_IT(fw, cw);
	}
    }
    return fw ;
}
/*{{{ change_font() - Switch to a new font */
/*
 * init = 1   - initialize
 *
 * fontname == FONT_UP  - switch to bigger font
 * fontname == FONT_DN  - switch to smaller font
 */
/* PROTO */
void
change_font(int init, const char *fontname)
{
    const char     *const msg = "can't load font \"%s\"";
    XFontStruct    *xfont;
    static char    *newfont[NFONTS];

#ifndef NO_BOLDFONT
    static XFontStruct *boldFont = NULL;

#endif
    static int      fnum = FONT0_IDX;	/* logical font number */
    int             idx = 0;	/* index into rs_font[] */

#if (FONT0_IDX == 0)
# define IDX2FNUM(i)	(i)
# define FNUM2IDX(f)	(f)
#else
# define IDX2FNUM(i)	(i == 0 ? FONT0_IDX : (i <= FONT0_IDX ? (i-1) : i))
# define FNUM2IDX(f)	(f == FONT0_IDX ? 0 : (f < FONT0_IDX  ? (f+1) : f))
#endif
/*#define FNUM_RANGE(i)	(i <= 0 ? 0 : (i >= NFONTS ? (NFONTS-1) : i))*/
/* so to make it working on Sun Ultra  thanks to Hari Nair */
#define FNUM_RANGE(i)	(i < 0 ? (NFONTS-1) : (i >= NFONTS ? 0 : i))

    if (!init) {
	switch (fontname[0]) {
	case '\0':
	    fnum = FONT0_IDX;
	    fontname = NULL;
	    break;

	/* special (internal) prefix for font commands */
	case FONT_CMD:
	    idx = atoi(fontname + 1);
	    switch (fontname[1]) {
	    case '+':		/* corresponds to FONT_UP */
		fnum += (idx ? idx : 1);
		fnum = FNUM_RANGE(fnum);
		break;

	    case '-':		/* corresponds to FONT_DN */
		fnum += (idx ? idx : -1);
		fnum = FNUM_RANGE(fnum);
		break;

	    default:
		if (fontname[1] != '\0' && !isdigit(fontname[1]))
		    return;
		if (idx < 0 || idx >= (NFONTS))
		    return;
		fnum = IDX2FNUM(idx);
		break;
	    }
	    fontname = NULL;
	    break;

	default:
	    if (fontname != NULL) {
	    /* search for existing fontname */
		for (idx = 0; idx < NFONTS; idx++) {
		    if (!strcmp(rs_font[idx], fontname)) {
			fnum = IDX2FNUM(idx);
			fontname = NULL;
			break;
		    }
		}
	    } else
		return;
	    break;
	}
    /* re-position around the normal font */
	idx = FNUM2IDX(fnum);

	if (fontname != NULL) {
	    char           *name;

	    xfont = XLoadQueryFont(Xdisplay, fontname);
	    if (!xfont)
		return;

	    name = MALLOC(strlen(fontname + 1) * sizeof(char));

	    if (name == NULL) {
		XFreeFont(Xdisplay, xfont);
		return;
	    }
	    STRCPY(name, fontname);
	    if (newfont[idx] != NULL)
		FREE(newfont[idx]);
	    newfont[idx] = name;
	    rs_font[idx] = newfont[idx];
	}
    }
    if (TermWin.font)
	XFreeFont(Xdisplay, TermWin.font);

/* load font or substitute */
    xfont = XLoadQueryFont(Xdisplay, rs_font[idx]);
    if (!xfont) {
	print_error(msg, rs_font[idx]);
	rs_font[idx] = "fixed";
	xfont = XLoadQueryFont(Xdisplay, rs_font[idx]);
	if (!xfont) {
	    print_error(msg, rs_font[idx]);
	    goto Abort;
	}
    }
    TermWin.font = xfont;

#ifndef NO_BOLDFONT
/* fail silently */
    if (init && rs_boldFont != NULL)
	boldFont = XLoadQueryFont(Xdisplay, rs_boldFont);
#endif

#ifdef MULTICHAR_SET
    if (TermWin.mfont)
	XFreeFont(Xdisplay, TermWin.mfont);

/* load font or substitute */
    xfont = XLoadQueryFont(Xdisplay, rs_mfont[idx]);
    if (!xfont) {
	print_error(msg, rs_mfont[idx]);
	rs_mfont[idx] = "k14";
	xfont = XLoadQueryFont(Xdisplay, rs_mfont[idx]);
	if (!xfont) {
	    print_error(msg, rs_mfont[idx]);
	    goto Abort;
	}
    }
    TermWin.mfont = xfont;
#endif				/* MULTICHAR_SET */

/* alter existing GC */
    if (!init) {
	XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
	menubar_expose();
    }
/* set the sizes */
    {
	int             fh, fw = 0;

	fw = TermWin.font->min_bounds.width; /* can be error !!!! */
	if( fw > 1000 ) fw = 0 ;  /* added by suggestion from <suchness>*/

#ifdef USE_LINESPACE
      fh = TermWin.font->ascent + TermWin.font->descent + TermWin.lineSpace;
#else
      fh = TermWin.font->ascent + TermWin.font->descent;
#endif

	if (TermWin.font->min_bounds.width == TermWin.font->max_bounds.width)
	    TermWin.fprop = 0;	/* Mono-spaced (fixed width) font */
	else
	    TermWin.fprop = 1;	/* Proportional font */

	if (TermWin.fprop == 1)	/* also, if == 0, per_char[] might be NULL */
	    fw = get_proportion_font_width( TermWin.font );
    /* not the first time thru and sizes haven't changed */
	if (fw == TermWin.fwidth && fh == TermWin.fheight)
	    return;		/* TODO: not return; check MULTI if needed */

	TermWin.fwidth = fw;
	TermWin.fheight = fh;
    }

/* check that size of boldFont is okay */
#ifndef NO_BOLDFONT
    TermWin.boldFont = NULL;
    if (boldFont != NULL) {
	int             fh, fw = 0;

	fw = boldFont->min_bounds.width; /* can be error !!!! */
	if( fw > 1000 ) fw = 0 ;

	fh = boldFont->ascent + boldFont->descent;
	if (TermWin.fprop == 0) {	/* bold font must also be monospaced */
	    if (fw != boldFont->max_bounds.width)
		fw = -1;
	} else
	    fw = get_proportion_font_width( boldFont );

	if (fw == TermWin.fwidth && fh == TermWin.fheight)
	    TermWin.boldFont = boldFont;
    }
#endif				/* NO_BOLDFONT */

/* TODO: check that size of Kanji font is okay */

    set_colorfgbg();

    TermWin.width = TermWin.ncol * TermWin.fwidth;
    TermWin.height = TermWin.nrow * TermWin.fheight;

    szHint.width_inc = TermWin.fwidth;
    szHint.height_inc = TermWin.fheight;

    szHint.min_width = szHint.base_width + szHint.width_inc;
    szHint.min_height = szHint.base_height + szHint.height_inc;

    szHint.width = szHint.base_width + TermWin.width;
    szHint.height = szHint.base_height + TermWin.height;
    { /* ONLYIF MENUBAR */
	szHint.height += (delay_menu_drawing ? menuBar_TotalHeight() : 0);
    }

    szHint.flags = PMinSize | PResizeInc | PBaseSize | PWinGravity;

    if (!init) {
	resize();
    }
    return;
  Abort:
    print_error("aborting");	/* fatal problem */
    exit(EXIT_FAILURE);
#undef IDX2FNUM
#undef FNUM2IDX
#undef FNUM_RANGE
}
/*}}} */

#ifdef _MYSTYLE_
char *DefaultMyStyle = NULL ;

/* PROTO */
const char*
GetDefaultMyStyle()
{
    if( DefaultMyStyle == NULL )
    {
		DefaultMyStyle = malloc( 1+strlen(MyName)+1 );
		sprintf( DefaultMyStyle, "*%s", MyName );
    }
    return DefaultMyStyle ;
}

char* pixel2string( unsigned long pixel, char* old_string )
{
  XColor xcol ;
    xcol.pixel = pixel ;
    XQueryColor( Xdisplay, DefaultColormap( Xdisplay, Xscreen), &xcol );
    if( xcol.flags != 0 )
    {
      old_string = realloc( old_string, 1+2+2+2+1 );
      sprintf( old_string, "#%2.2X%2.2X%2.2X", (xcol.red)>>8, xcol.green>>8, xcol.blue>>8 );
    }
    return old_string ;
}

char* argb2string( unsigned long argb, char* old_string )
{
	old_string = realloc( old_string, 1+2+2+2+1 );
    sprintf( old_string, "#%2.2lX%2.2lX%2.2lX", (argb>>16)&0x000000FF, (argb>>8)&0x000000FF, argb&0x000000FF );
    return old_string ;
}


char* font2string( XFontStruct *font, char* old_string )
{
  char* font_name ;
  Atom name_value ;
    if(XGetFontProperty(font, XA_FONT, &name_value)) {
	font_name=XGetAtomName(Xdisplay, name_value);
	if( font_name )
	{
          /*fprintf( stderr, " font found : [%s]\n", font_name ); */
	    old_string = realloc( old_string, strlen( font_name)+1 );
	    strcpy( old_string, font_name );
	}
	XFree( font_name );
    }
    return old_string ;
}

static const char *
translate_tint_color( ARGB32 tint )
{
	ARGB32 clean_tint = (tint<<1)|0xFF000000;
	static struct 
	{
	 	ARGB32 tint; 
		const char *name;	  
	}tint_names[6] = {{0xFFFF0000, "red"},{0xFFFFFF00, "yellow"},{0xFFFF00FF, "magenta"},
					  {0xFF00FF00, "green"},{0xFF00FFFF, "cyan"},{0xFF0000FF, "blue"}};
	int i ;

	if( ARGB32_RED8(clean_tint) > 0 ) 
		clean_tint |= 0x00030000;
	if( ARGB32_GREEN8(clean_tint) > 0 ) 
		clean_tint |= 0x00000300;
	if( ARGB32_BLUE8(clean_tint) > 0 ) 
		clean_tint |= 0x00000003;

	for( i = 0 ; i < 6 ; ++i ) 
		if( tint_names[i].tint == clean_tint ) 
			return tint_names[i].name;
	return NULL;
}

/* PROTO */
void 
set_mystyle( struct MyStyle* mystyle, Bool reset )
{
  int repaint = 0 ;
  static char* fg_color = NULL, *bg_color = NULL;

    TermWin.background.mystyle = mystyle ;
    if( mystyle == NULL )
    {
		if( TermWin.background.trgType == BGT_MyStyle )
		{
	    	if( TermWin.background.trgPixmap != None &&
			TermWin.background.trgPixmap != TermWin.background.srcPixmap )
			XFreePixmap( Xdisplay, TermWin.background.trgPixmap );
	    	TermWin.background.trgType = BGT_None ;
		}
		return ;
    }

    if( mystyle->texture_type > TEXTURE_SOLID &&
		( !reset
#ifdef TRANSPARENT
          || ( !(Options & Opt_transparent) && TermWin.background.srcPixmap == None )
#endif
		)
      )
	{
#ifdef TRANSPARENT
		const char *tint_color = translate_tint_color( mystyle->tint );
		if( mystyle->texture_type == TEXTURE_TRANSPARENT && 
			(tint_color != NULL || mystyle->tint == 0) )
		{
			Options |= Opt_transparent ;
			rs_color[Color_tint] = tint_color ;
			TermWin.background.mystyle = NULL ;
			TermWin.background.trgType = BGT_None ;
		}else		 
#endif
        	TermWin.background.trgType = BGT_MyStyle ;
	}

    if( mystyle && !get_flags(Options, Opt_transparent) )
    {
		if(  (mystyle->set_flags & F_FORECOLOR) &&
	    	!(TermWin.background.user_flags & F_FORECOLOR)  )
		{
	    	rs_color[Color_fg] = fg_color = argb2string(mystyle->colors.fore, fg_color ) ;
			/* fprintf( stderr, "mystyle.fore = 0x%lX, text = \"%s\"", mystyle->colors.fore, rs_color[Color_fg] ); */
	    	if( reset )
	    	{
				PixColorsFocused[Color_fg] = mystyle->colors.fore ;
#ifdef OFF_FOCUS_FADING
				PixColorsUnFocused[Color_fg] = fade_color(mystyle->colors.fore);
#endif
	    	}
	    	repaint++ ;
		}
		if(  (mystyle->set_flags & F_BACKCOLOR) &&
	    	!(TermWin.background.user_flags & F_BACKCOLOR)  )
		{
	    	rs_color[Color_bg] = bg_color = argb2string(mystyle->colors.back, bg_color ) ;
	    	if( reset )
	    	{
			PixColorsFocused[Color_bg] = mystyle->colors.back ;
#ifdef OFF_FOCUS_FADING
			PixColorsUnFocused[Color_bg] = fade_color(mystyle->colors.back);
#endif
	    	}
	    	repaint++ ;
		}
	/*if( mystyle->set_flags & F_BACKPIXMAP )      */

        if( reset )
		{   
			if( repaint > 0 )
	    	{ /* we want to repaint ourselves */
				on_colors_changed( Color_bg );
				if(  (mystyle->set_flags & F_FONT) &&
		    		!(TermWin.background.user_flags & F_FONT)  )
		    		change_font( False, rs_font[0] );
	    	}
	    	if( mystyle->texture_type > TEXTURE_SOLID )
	    	{
				RenderPixmap(True);
				scr_clear();
    				scr_touch();
	    	}
		}
    }
}
#endif

#ifndef HAVE_AFTERSTEP

/* PROTO */
Bool 
read_32bit_property (Window w, Atom property, CARD32* trg)
{
	Bool          res = False;

	if (w != None && property != None && trg)
	{
		Atom          actual_type;
		int           actual_format;
        unsigned long bytes_after;
		union 
		{
			unsigned char *uc_ptr ;
			long 		  *long_ptr ;
		}data;
		unsigned long nitems;

		data.long_ptr = NULL ;
		res =
			(XGetWindowProperty
			 (Xdisplay, w, property, 0, 1, False, AnyPropertyType, &actual_type,
			  &actual_format, &nitems, &bytes_after, &data.uc_ptr) == 0);

		/* checking property sanity */
		res = (res && nitems > 0 && actual_format == 32);

		if (res)
			trg[0] = data.long_ptr[0];
		if (data.long_ptr && nitems > 0)
			XFree (data.long_ptr);
	}
	return res;
}

/* PROTO */
Bool
read_32bit_proplist (Window w, Atom property, long estimate, CARD32 ** list, long *nitems)
{
	Bool          res = False;

#ifndef X_DISPLAY_MISSING
	if (w != None && property != None && list && nitems)
	{
		Atom          actual_type;
		int           actual_format;
        unsigned long bytes_after;
		unsigned long unitems = 0 ;
		union 
		{
		 	unsigned char *uc_ptr ;	
		 	long *long_ptr ;	
		}buffer;

		buffer.long_ptr = NULL ; 
		if (estimate <= 0)
			estimate = 1;
		res =
			(XGetWindowProperty
			 (Xdisplay, w, property, 0, estimate, False, AnyPropertyType,
			  &actual_type, &actual_format, &unitems, &bytes_after, &buffer.uc_ptr) == 0);
		/* checking property sanity */
		res = (res && unitems > 0 && actual_format == 32);

		if (bytes_after > 0 && res)
		{
			XFree (buffer.long_ptr);
			res =
				(XGetWindowProperty
				 (Xdisplay, w, property, 0, estimate + (bytes_after >> 2), False,
				  actual_type, &actual_type, &actual_format, &unitems, &bytes_after, &buffer.uc_ptr) == 0);
			res = (res && (unitems > 0));	   /* bad property */
		}

		if (!res)
		{
			*nitems = 0;
			*list = NULL ;
		}else
		{
			register int i = unitems ;
			register CARD32 *data32  = malloc(unitems*sizeof(CARD32));
			while( --i >= 0 )
				data32[i] = buffer.long_ptr[i] ;
			*list = data32 ;
			*nitems = unitems ;
		}
		if ( buffer.long_ptr )
			XFree (buffer.long_ptr);
	}
#endif
	return res;
}

#endif

/* PROTO */
Bool
check_extended_wm_state()	
{
	CARD32         *items;
	long          nitems = 0;
	unsigned long new_state = 0;
	Bool changed = False ;

	if (read_32bit_proplist (TermWin.parent, _XA_NET_WM_STATE, 6, &items, &nitems))
	{
		int i ;
		for( i = 0 ; i < nitems ; ++i ) 
		{	
			if( items[i] == _XA_NET_WM_STATE_STICKY )
				set_flags( new_state, WM_AtermStateSticky	);
			else if( items[i] == _XA_NET_WM_STATE_SHADED )
				set_flags( new_state, WM_AtermStateShaded );
			else if( items[i] == _XA_NET_WM_STATE_HIDDEN )
				set_flags( new_state, WM_AtermStateHidden );
		}
		free (items);
		changed = (get_flags(ExtWM.flags, WM_AtermStateSticky|WM_AtermStateShaded|WM_AtermStateHidden) != new_state) ; 
		clear_flags(ExtWM.flags, WM_AtermStateSticky|WM_AtermStateShaded|WM_AtermStateHidden);
		set_flags( ExtWM.flags, new_state );
	}else
	{	
		changed = get_flags(ExtWM.flags, WM_AtermStateSticky|WM_AtermStateShaded|WM_AtermStateHidden);
		clear_flags(ExtWM.flags, WM_AtermStateSticky|WM_AtermStateShaded|WM_AtermStateHidden);
	}

	return changed;
}

/* PROTO */
void 
check_extended_wm_hints_support()
{
	memset( &ExtWM, 0x00, sizeof(ExtWM));

	if( read_32bit_property (Xroot, _XA_NET_SUPPORTING_WM_CHECK, &ExtWM.supporting_wm_check) )
		if( ExtWM.supporting_wm_check != None ) 
		{	
			Window w;
			if( !read_32bit_property (ExtWM.supporting_wm_check, _XA_NET_SUPPORTING_WM_CHECK, &w) )
				ExtWM.supporting_wm_check = None ;
			else if( w != ExtWM.supporting_wm_check ) 
				ExtWM.supporting_wm_check = None ;
			else
			{
				CARD32 *supported_props = NULL;
				long nitems	= 0 ;
				Bool curr_desk = False ; 
				Bool app_desk = False ; 
				if( read_32bit_proplist (Xroot, _XA_NET_SUPPORTED, 20, &supported_props, &nitems) )
				{
					int i ; 
					for( i = 0 ; i < nitems; ++i ) 
					{	
						if( supported_props[i] == _XA_NET_CURRENT_DESKTOP ) 
							curr_desk = True ; 
						else if( supported_props[i] == _XA_NET_WM_DESKTOP ) 
							app_desk = True ; 
					}
				}
				if( curr_desk && app_desk )
				{			
					set_flags( ExtWM.flags, WM_ClaimSupportsDesktops );
					if( read_32bit_property (Xroot, _XA_NET_CURRENT_DESKTOP, &ExtWM.current_desktop) )
						set_flags( ExtWM.flags, WM_SupportsDesktops	);
				}
			}
		}

}



/*{{{ main() */
/* PROTO */
int
main(int argc, char *argv[])
{
    int i, t;
    char           *val, **cmd_argv = NULL;

/* "WINDOWID=\0" = 10 chars, UINT_MAX = 10 chars */
    static char     windowid_string[20], *display_string, *term_string;

/* Hops - save original arglist for wdw property WM_COMMAND */
    int             saved_argc = argc;
    char          **saved_argv = (char **)MALLOC((argc + 1) * sizeof(char *));

    PixColors = &(PixColorsFocused[0]);

    for (i = 0; i < argc; i++)
	saved_argv[i] = argv[i];
    saved_argv[i] = NULL;	/* NULL terminate that sucker. */
    for (i = 0; i < argc; i++) {
	if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "-exec")) {
	    argc = i;
	    argv[argc] = NULL;
	    if (argv[argc + 1] != NULL)
		cmd_argv = (argv + argc + 1);
	    break;
	}
    }

/*
 * Save and then give up any super-user privileges
 * If we need privileges in any area then we must specifically request it.
 * We should only need to be root in these cases:
 *  1.  write utmp entries on some systems
 *  2.  chown tty on some systems
 */
    privileges(SAVE);
    privileges(IGNORE);

    Options = Opt_scrollBar | Opt_scrollTtyOutput;
    Xdisplay = NULL;
    display_name = NULL;
    rs_term_name = NULL;
    rs_cutchars = NULL;
#ifndef NO_BOLDFONT
    rs_boldFont = NULL;
#endif
#ifdef PRINTPIPE
    rs_print_pipe = NULL;
#endif
    rs_title = NULL;		/* title name for window */
    rs_iconName = NULL;		/* icon name for window */
    rs_geometry = NULL;		/* window geometry */
    rs_minBufferWidth = NULL;
    rs_saveLines = NULL;	/* scrollback buffer [lines] */
#ifdef USE_LINESPACE
    rs_lineSpace = NULL;
#endif
    rs_borderWidth = NULL;
    rs_internal_border = NULL;
    rs_modifier = NULL;	/* modifier */
#if defined (HOTKEY_CTRL) || defined (HOTKEY_META)
/* recognized when combined with HOTKEY */
    ks_bigfont = XK_greater;
    ks_smallfont = XK_less;
#endif

    rs_menu = NULL;
    rs_path = NULL;
#ifdef BACKGROUND_IMAGE
    rs_backgroundPixmap = NULL;
#endif

#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT)
	rs_backgroundType = NULL;
    rs_shade = NULL;
    rs_tintType = NULL;
#endif


#ifndef NO_BACKSPACE_KEY
    rs_backspace_key = NULL;
#endif
#ifndef NO_DELETE_KEY
    rs_delete_key = NULL;
#endif
#ifndef NO_BRIGHTCOLOR
    colorfgbg = DEFAULT_RSTYLE;
#endif

    rs_name = my_basename(argv[0]);
    if (cmd_argv != NULL && cmd_argv[0] != NULL)
	rs_iconName = rs_title = my_basename(cmd_argv[0]);

/*
 * Open display, get options/resources and create the window
 */
    get_options(argc, argv);

	if( display_name == NULL )
    	if ((display_name = getenv("DISPLAY")) == NULL)
			display_name = ":0";

#ifdef HAVE_AFTERSTEP
#ifdef MyArgs_IS_MACRO	
    MyArgsPtr = safecalloc(1, sizeof(ASProgArgs) );
#else
	memset( &MyArgs, 0x00, sizeof(ASProgArgs) );
#endif
#ifdef Scr_IS_MACRO	 
	ASDefaultScr = safecalloc(1, sizeof(ScreenInfo));
#else
    memset( &Scr, 0x00, sizeof(ScreenInfo) );
#endif
	
	Scr.RootClipArea.width = 1 ;
	Scr.RootClipArea.height = 1;


	MyArgs.verbosity_level = OUTPUT_DEFAULT_THRESHOLD ;
	set_output_threshold( MyArgs.verbosity_level );

	MyArgs.display_name = (char*)display_name ;
	SetMyName( argv[0] );
#ifdef NO_DEBUG_OUTPUT
	set_output_threshold( 0 );
#endif

	ConnectX ( &Scr, 0 );
    Xdisplay = dpy ;
    Xscreen = Scr.screen ;
	asv = Scr.asv ;

	Xcmap = asv->colormap;
	Xdepth = asv->visual_info.depth;
	Xvisual = asv->visual_info.visual;

	XdisplayWidth = Scr.MyDisplayWidth ;
	XdisplayHeight = Scr.MyDisplayHeight ;

#else
	Xdisplay = XOpenDisplay(display_name);
    
	if (!Xdisplay) {
		print_error("can't open display %s", display_name);
		exit(EXIT_FAILURE);
    }
  /* changed from _MOTIF_WM_INFO - Vaevictus - gentoo bug #139554 */
	_XA_MwmAtom = XInternAtom(Xdisplay, "_MOTIF_WM_HINTS", True);
	_XA_NET_WM_PID = XInternAtom(Xdisplay, "_NET_WM_PID", False);
	_XROOTPMAP_ID = XInternAtom(Xdisplay, "_XROOTPMAP_ID", False);
	_XA_NET_SUPPORTING_WM_CHECK = XInternAtom(Xdisplay, "_NET_SUPPORTING_WM_CHECK", False);
	_XA_NET_SUPPORTED           = XInternAtom(Xdisplay, "_NET_SUPPORTED", False);
	_XA_NET_CURRENT_DESKTOP     = XInternAtom(Xdisplay, "_NET_CURRENT_DESKTOP", False);
	_XA_NET_WM_DESKTOP			= XInternAtom(Xdisplay, "_NET_WM_DESKTOP", False);
	_XA_NET_WM_STATE			= XInternAtom(Xdisplay, "_XA_NET_WM_STATE", False);
	_XA_NET_WM_STATE_STICKY		= XInternAtom(Xdisplay, "_XA_NET_WM_STATE_STICKY", False);
	_XA_NET_WM_STATE_SHADED		= XInternAtom(Xdisplay, "_XA_NET_WM_STATE_SHADED", False);
	_XA_NET_WM_STATE_HIDDEN		= XInternAtom(Xdisplay, "_XA_NET_WM_STATE_HIDDEN", False);
	
	Xdepth = DefaultDepth(Xdisplay, Xscreen);
    Xcmap = DefaultColormap(Xdisplay, Xscreen);
    Xvisual = DefaultVisual(Xdisplay, Xscreen);
	XdisplayWidth = DisplayWidth (Xdisplay, Xscreen);
	XdisplayHeight = DisplayHeight (Xdisplay, Xscreen);

#ifdef HAVE_AFTERIMAGE
	dpy = Xdisplay ;
	asv = create_asvisual (Xdisplay, Xscreen, Xdepth, NULL);
	Xcmap = asv->colormap;
	Xdepth = asv->visual_info.depth;
	Xvisual = asv->visual_info.visual;
#else	  
#ifdef PREFER_24BIT
/* If depth is not 24, look for a 24bit visual. */
    if (Xdepth != 24) 
	{
		XVisualInfo     vinfo;
		if (XMatchVisualInfo(Xdisplay, Xscreen, 24, TrueColor, &vinfo)) 
		{
	    	Xdepth = 24;
	    	Xvisual = vinfo.visual;
	    	Xcmap = XCreateColormap(Xdisplay, RootWindow(Xdisplay, Xscreen),
				    	Xvisual, AllocNone);
		}
    }
#endif
#endif

#endif                         /* HAVE_AFTERSTEP */

	aterm_XA_TARGETS = XInternAtom(Xdisplay, "TARGETS", False);
	_XA_COMPAUND_TEXT = XInternAtom(Xdisplay, "COMPOUND_TEXT", False);
	aterm_XA_TEXT = XInternAtom(Xdisplay, "TEXT", False);
	aterm_XA_UTF8_STRING = XInternAtom(Xdisplay, "UTF8_STRING", False);
	aterm_XA_CLIPBOARD = XInternAtom(Xdisplay, "CLIPBOARD", False);
	aterm_XA_VT_SELECTION = XInternAtom(Xdisplay, "VT_SELECTION", False);
	aterm_XA_INCR		  = XInternAtom(Xdisplay, "INCR", False);

	check_extended_wm_hints_support();
	
#ifdef DEBUG_X
    XSynchronize(Xdisplay, True);
    XSetErrorHandler((XErrorHandler) abort);
#else
    XSetErrorHandler((XErrorHandler) xerror_handler);
#endif

    extract_resources(Xdisplay, rs_name);


#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT) || defined(_MYSTYLE_)
    InitBackground();
#endif

#ifdef HAVE_AFTERSTEP
    mystyle_get_property(Scr.wmprops );
    /* lets see what options user chosen to override with other
       command line parameters : */
    if( rs_font[0] != NULL )
		TermWin.background.user_flags |= F_FONT ;
    if( rs_color[Color_fg] != NULL )
		TermWin.background.user_flags |= F_FORECOLOR ;
    if( rs_color[Color_bg] != NULL )
		TermWin.background.user_flags |= F_BACKCOLOR ;

    /* reading everything we can from default MyStyle : */
    set_mystyle(mystyle_find( GetDefaultMyStyle()), False);

    /* overriding it with user specifyed MyStyle : */
    if( rs_mystyle )
    {
      	MyStyle * mystyle = mystyle_find( rs_mystyle );
        if( mystyle == NULL )
	    	fprintf( stderr, "%s: AfterStep MyStyle \"%s\" not available - check your look file.\n", MyName, rs_mystyle );
		else
	    	set_mystyle( mystyle, False );
    }
#endif


#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT)
    if( rs_shade )
        TermWin.background.Shading.shading = atoi(rs_shade);
    SetBackgroundType(rs_backgroundType);
#endif

#if defined(XTERM_SCROLLBAR) || defined(NEXT_SCROLLBAR)
    sb_shadow = 0;
#else
    sb_shadow = (Options & Opt_scrollBar_floating) ? 0 : SHADOW;
#endif

/*
 * set any defaults not already set
 */

    if (!rs_title)
	rs_title = rs_name;
    if (!rs_iconName)
	rs_iconName = rs_title;

    if (!rs_minBufferWidth || (t = atoi(rs_minBufferWidth)) < 0)
	TermWin.min_bcol = 1;
    else 
    	TermWin.min_bcol = t;

    if (!rs_saveLines || (t = atoi(rs_saveLines)) < 0)
    	TermWin.saveLines = SAVELINES;
    else
        TermWin.saveLines = t;
#ifdef USE_LINESPACE
    if (!rs_lineSpace || (TermWin.lineSpace = atoi(rs_lineSpace)) < 0)
        TermWin.lineSpace = LINESPACE;
#endif

    if (!rs_borderWidth || (t = atoi(rs_borderWidth)) < 0)
        TermWin.borderWidth = BORDERWIDTH;
    else
        TermWin.borderWidth = t;

    if (!rs_internal_border|| (t = atoi(rs_internal_border)) < 0)
        TermWin_internalBorder = 2;
    else
        TermWin_internalBorder = t;
    TermWin_internalBorders = 2 * TermWin_internalBorder;
    
/* no point having a scrollbar without having any scrollback! */
    if (!TermWin.saveLines)
	Options &= ~Opt_scrollBar;

#ifdef PRINTPIPE
    if (!rs_print_pipe)
	rs_print_pipe = PRINTPIPE;
#endif
    if (!rs_cutchars)
	rs_cutchars = CUTCHARS;
#ifndef NO_BACKSPACE_KEY
    if (!rs_backspace_key)
# ifdef DEFAULT_BACKSPACE
	rs_backspace_key = DEFAULT_BACKSPACE;
# else
	rs_backspace_key = "DEC";	/* can toggle between \033 or \177 */
# endif
    else
	(void) Str_escaped((char*)rs_backspace_key);
#endif
#ifndef NO_DELETE_KEY
    if (!rs_delete_key)
# ifdef DEFAULT_DELETE
	rs_delete_key = DEFAULT_DELETE;
# else
	rs_delete_key = "\033[3~";
# endif
    else
	(void) Str_escaped((char*)rs_delete_key);
#endif
#ifndef NO_BOLDFONT
    if (rs_font[0] == NULL && rs_boldFont != NULL) {
	rs_font[0] = rs_boldFont;
	rs_boldFont = NULL;
    }
#endif
    for (i = 0; i < NFONTS; i++) {
	if (!rs_font[i])
	    rs_font[i] = def_fontName[i];
#ifdef MULTICHAR_SET
	if (!rs_mfont[i])
	    rs_mfont[i] = def_mfontName[i];
#endif
    }
#ifdef USE_XIM
    TermWin.fontset = NULL;
#endif

#ifdef XTERM_REVERSE_VIDEO
/* this is how xterm implements reverseVideo */
    if (Options & Opt_reverseVideo) {
	if (!rs_color[Color_fg])
	    rs_color[Color_fg] = def_colorName[Color_bg];
	if (!rs_color[Color_bg])
	    rs_color[Color_bg] = def_colorName[Color_fg];
    }
#endif
	
    for ( i = 0 ; i < NRS_COLORS; i++)
		if (!rs_color[i])
	    	rs_color[i] = def_colorName[i];

#ifndef XTERM_REVERSE_VIDEO
/* this is how we implement reverseVideo */
    if (Options & Opt_reverseVideo) {
	const char     *name;

    /* swap foreground/background colors */

	name = rs_color[Color_fg];
	rs_color[Color_fg] = rs_color[Color_bg];
	rs_color[Color_bg] = name;

	name = def_colorName[Color_fg];
	def_colorName[Color_fg] = def_colorName[Color_bg];
	def_colorName[Color_bg] = name;
    }
#endif

/* convenient aliases for setting fg/bg to colors */
    color_aliases(Color_fg);
    color_aliases(Color_bg);
#ifndef NO_CURSORCOLOR
    color_aliases(Color_cursor);
    color_aliases(Color_cursor2);
#endif				/* NO_CURSORCOLOR */
    color_aliases(Color_pointer);
    color_aliases(Color_border);
#ifndef NO_BOLDUNDERLINE
    color_aliases(Color_BD);
    color_aliases(Color_UL);
#endif				/* NO_BOLDUNDERLINE */

/* add startup-menu: */
    { /* ONLYIF MENUBAR */
	delay_menu_drawing = 1;
	menubar_read(rs_menu);
	delay_menu_drawing--;
    }

    Create_Windows(saved_argc, saved_argv);
    scr_reset();		/* initialize screen */
    Gr_reset();			/* reset graphics */

/* add scrollBar, do it directly to avoid resize() */
    scrollbar_mapping(Options & Opt_scrollBar);
/* we can now add menuBar */
    { /* ONLYIF MENUBAR */
	if (delay_menu_drawing) {
	    delay_menu_drawing = 0;
	    menubar_mapping(1);
	}
    }
/* do it now to avoid unneccessary redrawing */
    XMapWindow(Xdisplay, TermWin.vt);
    XMapWindow(Xdisplay, TermWin.parent);

#if 0
#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT) || defined(_MYSTYLE_)
    if( TermWin.background.trgType != BGT_None )
    {
        refresh_transparent_scrollbar();
        RenderPixmap(1);
	scr_clear();
	scr_touch();
    }
#endif
#endif

#ifdef DISPLAY_IS_IP
/* Fixup display_name for export over pty to any interested terminal
 * clients via "ESC[7n" (e.g. shells).  Note we use the pure IP number
 * (for the first non-loopback interface) that we get from
 * network_display().  This is more "name-resolution-portable", if you
 * will, and probably allows for faster x-client startup if your name
 * server is beyond a slow link or overloaded at client startup.  Of
 * course that only helps the shell's child processes, not us.
 *
 * Giving out the display_name also affords a potential security hole
 */
    val = display_name = network_display(display_name);
    if (val == NULL)
#endif				/* DISPLAY_IS_IP */
	val = XDisplayString(Xdisplay);
    if (display_name == NULL)
	display_name = val;	/* use broken `:0' value */

    i = strlen(val);
    display_string = MALLOC((i + 9) * sizeof(char));

    sprintf(display_string, "DISPLAY=%s", val);
    sprintf(windowid_string, "WINDOWID=%u", (unsigned int)TermWin.parent);

/* add entries to the environment:
 * @ DISPLAY:   in case we started with -display
 * @ WINDOWID:  X window id number of the window
 * @ COLORTERM: terminal sub-name and also indicates its color
 * @ TERM:      terminal name
 */
    putenv(display_string);
    putenv(windowid_string);
/*    FREE(display_string); this cannot be freed */

#ifdef RXVT_TERMINFO
    putenv("TERMINFO=" RXVT_TERMINFO);
#endif

    if (Xdepth <= 2)
	putenv("COLORTERM=" COLORTERMENV "-mono");
    else
	putenv("COLORTERM=" COLORTERMENVFULL);
    if (rs_term_name != NULL) {
	i = strlen(rs_term_name);
	term_string = MALLOC((i + 6) * sizeof(char));

	sprintf(term_string, "TERM=%s", rs_term_name);
	putenv(term_string);
    } else {
	putenv("TERM=" TERMENV);
    }

    if (!setlocale(LC_CTYPE, "")) print_error("Cannot set locale");

    init_command(cmd_argv);

    main_loop();		/* main processing loop */
    return EXIT_SUCCESS;
}

#undef XGetGeometry
Status trace_XGetGeometry( char *file, int line, Display *dpy, Window w, Window *r, int *x, int *y,
                           unsigned int *width,unsigned int *height,unsigned int *bw,unsigned int *depth)
{
    Status res ;
    fprintf( stderr, "%s(%d):XGetGeometry(%lX) = ",file,line,w);
    res = XGetGeometry(dpy,w,r,x,y,width,height,bw,depth);
    fprintf( stderr, "(%ux%u%+d%+d),%d\n", *width, *height, *x, *y, res );
    return res ;
}


/*}}} */
/*----------------------- end-of-file (C source) -----------------------*/
