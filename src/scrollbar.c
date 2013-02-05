/*--------------------------------*-C-*---------------------------------*
 * File:	scrollbar2.c
 *----------------------------------------------------------------------*
 * Copyright (C) 1998 Alfredo K. Kojima <kojima@windowmaker.org>
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
/*
 * Same code as scrollbar.c, but this one is rewritten to do
 * N*XTSTEP like scrollbars.
 *
 * 1998 Alfredo Kojima  <kojima@windowmaker.org>
 *                      Wrote base NeXT Scroll bar code
 * 1999 Sasha Vasko     <sasha at aftercode.net>
 * 			enhanced code for better portability and
 *			configurability
 */


#include "rxvt.h"		/* NECESSARY */

/*----------------------------------------------------------------------*
 */

static GC paintGC = None;
static GC grayGC = None;
static GC stippleGC = None;
static Pixel blackPixel, whitePixel, darkPixel;

static char *SCROLLER_DIMPLE[] = {
".%###.",
"%#%%%%",
"#%%...",
"#%..  ",
"#%.   ",
".%.  ."
};

#define SCROLLER_DIMPLE_WIDTH   6
#define SCROLLER_DIMPLE_HEIGHT  6



static char *SCROLLER_ARROW_UP[] = {
".............",
".............",
"......%......",
"......#......",
".....%#%.....",
".....###.....",
"....%###%....",
"....#####....",
"...%#####%...",
"...#######...",
"..%#######%..",
".............",
"............."
};

static char *SCROLLER_ARROW_DOWN[] = {
".............",
".............",
"..%#######%..",
"...#######...",
"...%#####%...",
"....#####....",
"....%###%....",
".....###.....",
".....%#%.....",
"......#......",
"......%......",
".............",
"............."
};


static char *HI_SCROLLER_ARROW_UP[] = {
"             ",
"             ",
"      %      ",
"      %      ",
"     %%%     ",
"     %%%     ",
"    %%%%%    ",
"    %%%%%    ",
"   %%%%%%%   ",
"   %%%%%%%   ",
"  %%%%%%%%%  ",
"             ",
"             "
};

static char *HI_SCROLLER_ARROW_DOWN[] = {
"             ",
"             ",
"  %%%%%%%%%  ",
"   %%%%%%%   ",
"   %%%%%%%   ",
"    %%%%%    ",
"    %%%%%    ",
"     %%%     ",
"     %%%     ",
"      %      ",
"      %      ",
"             ",
"             "
};

#define ARROW_SOURCE_WIDTH   13
#define ARROW_SOURCE_HEIGHT  13

typedef struct {
    Pixmap icon ;
    Pixmap icon_mask ;
    int origin_x, origin_y, width, height ;
} Icon;

typedef struct {
    unsigned arrow_width, arrow_height ;
    int bValid ;
    char** Data[4] ;
    Icon Arrows[4] ;
}ScrollArrows ;

static Icon dimple =
{None, None, 0, 0, SCROLLER_DIMPLE_WIDTH, SCROLLER_DIMPLE_HEIGHT};

static ScrollArrows NeXTScrollArrows =
{ ARROW_SOURCE_WIDTH, ARROW_SOURCE_HEIGHT, 0,
  {SCROLLER_ARROW_UP, HI_SCROLLER_ARROW_UP,
   SCROLLER_ARROW_DOWN, HI_SCROLLER_ARROW_DOWN } };
#define UP_ARROW	(NeXTScrollArrows.Arrows[0])
#define UP_ARROW_HI	(NeXTScrollArrows.Arrows[1])
#define DOWN_ARROW	(NeXTScrollArrows.Arrows[2])
#define DOWN_ARROW_HI	(NeXTScrollArrows.Arrows[3])


#define ARROW_WIDTH   (NeXTScrollArrows.arrow_width)
#define ARROW_HEIGHT  (NeXTScrollArrows.arrow_height)

#define BEVEL_HI_WIDTH 1
#ifdef NEXT_SCROLL_CLEAN
# define BEVEL_LO_WIDTH 1
#else
# define BEVEL_LO_WIDTH 2
#endif
#define BEVEL_SIZE (BEVEL_HI_WIDTH+BEVEL_LO_WIDTH)

#define SB_BUTTON_HEIGHT (BEVEL_SIZE+ARROW_HEIGHT)
#define SB_BUTTONS_HEIGHT (SB_BUTTON_HEIGHT<<1)

#ifndef SB_BORDER_WIDTH
# define SB_BORDER_WIDTH 1
#endif
#define SB_BORDER_SIZE  (SB_BORDER_WIDTH<<1)

#ifndef SIDE_STEP_WIDTH
# define SIDE_STEP_WIDTH 0
#endif
/* end unconfigurable stuff */


#define stp_width 8
#define stp_height 8
static unsigned char stp_bits[] = {
   0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};

#ifdef TRANSPARENT
#define IS_TRANSP_SCROLL (Options&Opt_transparent_sb)
#else
#define IS_TRANSP_SCROLL 0
#endif

typedef struct {
    GC blackGC;
    GC whiteGC;
    GC darkGC;
    GC maskGC;
    GC maskGC_0;
}IconGC;


static void
init_scroll_size(void)
{
    NeXTScrollArrows.arrow_width = (SB_WIDTH-BEVEL_SIZE-SB_BORDER_SIZE-SIDE_STEP_WIDTH);
    MIN_IT( NeXTScrollArrows.arrow_width, ARROW_SOURCE_WIDTH );

#ifdef NEXT_SCROLL_SQUARE_ARROWS
    NeXTScrollArrows.arrow_height = NeXTScrollArrows.arrow_width ;
#else
    NeXTScrollArrows.arrow_height =  ARROW_SOURCE_HEIGHT ;
#endif
    NeXTScrollArrows.bValid = 1 ;
}

/* PROTO */
unsigned
GetScrollArrowsHeight()
{
    if( !NeXTScrollArrows.bValid ) init_scroll_size();
    return (SB_BUTTONS_HEIGHT);
}

static void
CheckIconGC(IconGC* igc, Pixmap icon, Pixmap icon_mask)
{
  XGCValues values ;
  unsigned long valuemask = GCForeground|GCGraphicsExposures ;

    values.graphics_exposures = False;

    if( igc == NULL ) return ;
    if( igc->maskGC == None )
    {
        values.foreground = 1 ;
	igc->maskGC = XCreateGC( Xdisplay, icon_mask, valuemask, &values );
    }
    if( igc->maskGC_0 == None )
    {
        values.foreground = 0 ;
	igc->maskGC_0 = XCreateGC( Xdisplay, icon_mask, valuemask, &values );
    }
    if( igc->whiteGC == None )
    {
        values.foreground = whitePixel ;
	igc->whiteGC = XCreateGC( Xdisplay, icon, valuemask, &values );
    }
    if( igc->darkGC == None )
    {
        values.foreground = darkPixel ;
	igc->darkGC = XCreateGC( Xdisplay, icon, valuemask, &values );
    }
    if( igc->blackGC == None )
    {
        values.foreground = blackPixel ;
	igc->blackGC = XCreateGC( Xdisplay, icon, valuemask, &values );
    }
}

static void
FreeIconGC( IconGC* igc )
{
    if( igc )
    {
	if( igc->maskGC != None )
	{
	    XFreeGC( Xdisplay, igc->maskGC );
	    igc->maskGC  = None;
	}
	if( igc->maskGC_0 != None )
	{
	    XFreeGC( Xdisplay, igc->maskGC_0 );
	    igc->maskGC_0  = None;
	}
	if( igc->whiteGC == None )
	{
	    XFreeGC( Xdisplay, igc->whiteGC );
	    igc->whiteGC  = None;
	}
	if( igc->darkGC == None )
	{
	    XFreeGC( Xdisplay, igc->darkGC );
	    igc->darkGC  = None;
	}
        if( igc->blackGC != None )
	{
	    XFreeGC( Xdisplay, igc->blackGC );
	    igc->blackGC  = None;
	}
    }
}

static void
renderIcon(char **data, Icon* pIcon, IconGC* igc)
{
    Pixmap d, mask;
    register int i, k ;
    int x, y ;
    GC maskgc, paintgc ;

    d = XCreatePixmap(Xdisplay, scrollBar.win, pIcon->width, pIcon->height, Xdepth);
    mask = XCreatePixmap(Xdisplay, scrollBar.win, pIcon->width, pIcon->height, 1);

    CheckIconGC( igc, d, mask );
    y = pIcon->origin_y ;

    for (i = 0; i < pIcon->height ; y++, i++ ) {
	x = pIcon->origin_x ;
        for (k = 0; k < pIcon->width ; k++, x++ ) {
	    maskgc = igc->maskGC ;
            switch (data[y][x]) {
             case ' ':
             case 'w': paintgc = igc->whiteGC ;  break;
             case '%':
             case 'd': paintgc = igc->darkGC ;  break;
             case '#':
             case 'b': paintgc = igc->blackGC ;  break;
             case '.':
             case 'l':
             default:  paintgc = grayGC;
	               maskgc = igc->maskGC_0 ;
		       break;
            }
            XDrawPoint(Xdisplay, d, paintgc, k, i);
            XDrawPoint(Xdisplay, mask, maskgc, k, i);
        }
    }

    pIcon->icon = d ;
    pIcon->icon_mask = mask ;
}

static void
PlaceIcon( Icon* i, int x, int y, Drawable buffer )
{
#ifdef TRANSPARENT
    if( IS_TRANSP_SCROLL )
    {
	XSetClipMask( Xdisplay, paintGC, i->icon_mask );
	XSetClipOrigin( Xdisplay, paintGC, x, y );
    }
#endif
    XCopyArea(Xdisplay, i->icon, buffer, paintGC, 0, 0,
    	      i->width, i->height,x,y );

}

static void
init_stuff(void)
{
    XGCValues gcvalue;
    XColor xcol;
    Pixmap stipple;
    unsigned long light;
    unsigned arrow_x_offset, arrow_y_offset ;
    IconGC icongc = {None,None,None,None};
    int i ;

    gcvalue.graphics_exposures = False;

    blackPixel = BlackPixelOfScreen(DefaultScreenOfDisplay(Xdisplay));
    whitePixel = WhitePixelOfScreen(DefaultScreenOfDisplay(Xdisplay));

    xcol.red = 0xaeba;
    xcol.green = 0xaaaa;
    xcol.blue = 0xaeba;
    if (!XAllocColor (Xdisplay, Xcmap, &xcol))
    {
	print_error ("can't allocate %s", "light gray");
#ifndef NO_BRIGHTCOLOR
	xcol.pixel = PixColors [Color_AntiqueWhite];
#else
	xcol.pixel = PixColors [Color_White];
#endif
    }

    light = gcvalue.foreground = xcol.pixel;
    grayGC =  XCreateGC(Xdisplay, scrollBar.win, GCForeground|GCGraphicsExposures,
			&gcvalue);

    xcol.red = 0x51aa;
    xcol.green = 0x5555;
    xcol.blue = 0x5144;
    if (!XAllocColor (Xdisplay, Xcmap, &xcol))
    {
	print_error ("can't allocate %s", "dark gray");
#ifndef NO_BRIGHTCOLOR
	xcol.pixel = PixColors [Color_Grey25];
#else
	xcol.pixel = PixColors [Color_Black];
#endif

    }

    darkPixel = xcol.pixel;

    renderIcon(SCROLLER_DIMPLE, &dimple, &icongc);

    if( !NeXTScrollArrows.bValid ) init_scroll_size();

    arrow_x_offset = (ARROW_SOURCE_WIDTH-ARROW_WIDTH)>>1;
#ifdef NEXT_SCROLL_SQUARE_ARROWS
    arrow_y_offset = arrow_x_offset ;
#else
    arrow_y_offset = 0; /* not implemented yet */
#endif

    for( i = 0 ; i < 4 ; i++ )
    {
	NeXTScrollArrows.Arrows[i].origin_x = arrow_x_offset ;
	NeXTScrollArrows.Arrows[i].origin_y = arrow_y_offset ;
	NeXTScrollArrows.Arrows[i].width = ARROW_WIDTH ;
	NeXTScrollArrows.Arrows[i].height = ARROW_HEIGHT ;
	renderIcon(NeXTScrollArrows.Data[i], &(NeXTScrollArrows.Arrows[i]), &icongc );
    }

    FreeIconGC( &icongc );

    gcvalue.foreground = whitePixel;
    paintGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground|GCGraphicsExposures,
			&gcvalue);

    stipple = XCreateBitmapFromData(Xdisplay, scrollBar.win,
				    stp_bits, stp_width, stp_height);
    gcvalue.foreground = darkPixel;
    gcvalue.background = light;
    gcvalue.fill_style = FillStippled;
    gcvalue.stipple = stipple;

    stippleGC = XCreateGC(Xdisplay, scrollBar.win, GCForeground|GCBackground
			  |GCStipple|GCFillStyle|GCGraphicsExposures,
			  &gcvalue);

    scrollbar_show(1);
}


/* Draw bezel & arrows */
static void
drawBevel(Drawable d, int x, int y, int w, int h)
{
    XSetForeground( Xdisplay, paintGC, whitePixel );
    XDrawLine(Xdisplay, d, paintGC, x, y, x+w-1, y);
    XDrawLine(Xdisplay, d, paintGC, x, y, x, y+h-1);

    XSetForeground( Xdisplay, paintGC, blackPixel );
    XDrawLine(Xdisplay, d, paintGC, x+w-1, y, x+w-1, y+h-1);
    XDrawLine(Xdisplay, d, paintGC, x, y+h-1, x+w-1, y+h-1);
#ifndef NEXT_SCROLL_CLEAN

    XSetForeground( Xdisplay, paintGC, darkPixel );
    XDrawLine(Xdisplay, d, paintGC, x+1, y+h-2, x+w-2, y+h-2);
    XDrawLine(Xdisplay, d, paintGC, x+w-2, y+1, x+w-2, y+h-2);
#endif
}


extern char* MyName ;
/* PROTO */
int
scrollbar_mapping(int map)
{
    int             change = 0;

    if (map && !scrollbar_visible()) {
	scrollBar.state = 1;
	XMapWindow(Xdisplay, scrollBar.win);
	change = 1;
    } else if (!map && scrollbar_visible()) {
	scrollBar.state = 0;
	XUnmapWindow(Xdisplay, scrollBar.win);
	change = 1;
    }
    return change;
}

#ifdef TRANSPARENT
typedef struct {
    Pixmap root;
    unsigned int x, y, height ;

    Pixmap cache;
}TransparencyCache ;

#endif

Pixmap scrollbar_fill_back( unsigned int height, int check_cache )
{

  Pixmap buffer = None;
#ifdef TRANSPARENT
  static TransparencyCache tCache = {None, -1, -1, 0, None};
#endif

    /* create double buffer */
    buffer = XCreatePixmap(Xdisplay, scrollBar.win, SB_WIDTH+SB_BORDER_WIDTH, height, Xdepth);

#ifdef TRANSPARENT
    if( IS_TRANSP_SCROLL && scrollBar.state )
    {
      Pixmap root_pmap = None ;
      unsigned int root_width = 0, root_height = 0 ;
	if( check_cache == 1 )
	{
	  int cache_valid = 0 ;
    	    root_pmap = ValidatePixmap(root_pmap, 1, 1, &root_width, &root_height);
	    if( height == tCache.height && root_pmap == tCache.root )
	    {
	      int my_x, my_y ;
		if( GetWinPosition(scrollBar.win, &my_x, &my_y) )
		{
		    if( my_x== tCache.x && my_y == tCache.y ) cache_valid = 1 ;
		    else { tCache.x = my_x ; tCache.y = my_y ; }
		}else cache_valid = 1 ;
	    }else { tCache.root = root_pmap ; tCache.height = height ; }
	    if( cache_valid == 0 )
	    {
		if(tCache.cache) XFreePixmap( Xdisplay, tCache.cache );
		tCache.cache = CutWinPixmap( scrollBar.win, root_pmap, root_width, root_height, SB_WIDTH+SB_BORDER_WIDTH, height, grayGC, &(TermWin.background.Shading));
	    }
	}
	if( tCache.cache != None )
	{
	    FillPixmapWithTile( buffer, tCache.cache, 0, 0, SB_WIDTH+SB_BORDER_WIDTH, height, 0, 0 );
	    if( TermWin.tintGC)
    		XFillRectangle(Xdisplay, buffer, TermWin.tintGC, 0, 0, SB_WIDTH+SB_BORDER_WIDTH-1, height-1);
	    return buffer ;
	}
    }
#endif

    /* draw the background */
    XFillRectangle(Xdisplay, buffer, grayGC, 0, 0, SB_WIDTH+SB_BORDER_WIDTH, height);

    XSetForeground(Xdisplay, paintGC, blackPixel );
    XDrawRectangle(Xdisplay, buffer, paintGC, 0, 0, SB_WIDTH, height);

    if (TermWin.nscrolled > 0)
    {
        XFillRectangle( Xdisplay, buffer, stippleGC,
    	                SB_BORDER_WIDTH+SB_BORDER_WIDTH, SB_BORDER_WIDTH,
	                SB_WIDTH-SB_BORDER_SIZE-SIDE_STEP_WIDTH,
    	        	height-SB_BUTTONS_HEIGHT );

    }else {
        XFillRectangle(Xdisplay, buffer, stippleGC,
	           SB_BORDER_WIDTH+SB_BORDER_WIDTH, SB_BORDER_WIDTH,
	           SB_WIDTH-SB_BORDER_SIZE, height-SB_BORDER_SIZE);
    }

    return buffer ;
}


int
scrollbar_show_cached(int update, int check_cache)
{
    /* old (drawn) values */
    static int last_top, last_bot, last_len;
    static int scrollbar_len;		/* length of slider */
    Pixmap buffer;
    int height = scrollBar.end + SB_BUTTONS_HEIGHT+sb_shadow ;

    if (paintGC == None)
	init_stuff();

    if (update)
    {
	int             top = (TermWin.nscrolled - TermWin.view_start);
	int             bot = top + (TermWin.nrow - 1);
	int             len = max((TermWin.nscrolled + (TermWin.nrow - 1)),1);

#define MIN_SCROLL_LENGTH (SCROLLER_DIMPLE_HEIGHT+3)
	scrollBar.top = (scrollBar.beg +
			 (top * (scrollbar_size()-SB_BORDER_SIZE-MIN_SCROLL_LENGTH)) / len);
	scrollBar.bot = (scrollBar.beg + MIN_SCROLL_LENGTH+
			 (bot * (scrollbar_size()-SB_BORDER_SIZE-MIN_SCROLL_LENGTH)) / len);

	scrollbar_len = scrollBar.bot - scrollBar.top;
    /* no change */
	if ((scrollBar.top == last_top) && (scrollBar.bot == last_bot))
	    return 0;

    }

    last_top = scrollBar.top;
    last_bot = scrollBar.bot;
    last_len = scrollbar_len;

    buffer = scrollbar_fill_back( height, check_cache );
    if (TermWin.nscrolled > 0)
    {
#ifdef TRANSPARENT
	if( !IS_TRANSP_SCROLL )
	{
	    XFillRectangle(Xdisplay, buffer, grayGC,
		       SB_BORDER_WIDTH+BEVEL_HI_WIDTH,
	               scrollBar.top+BEVEL_HI_WIDTH+SB_BORDER_WIDTH-SIDE_STEP_WIDTH,
	               SB_WIDTH-SB_BORDER_WIDTH-BEVEL_LO_WIDTH, scrollbar_len);
	}
#endif
	drawBevel(buffer, SB_BORDER_WIDTH+BEVEL_HI_WIDTH,
			  scrollBar.top+BEVEL_HI_WIDTH+SB_BORDER_WIDTH,
			  SB_WIDTH-SB_BORDER_SIZE-SIDE_STEP_WIDTH,
			  scrollbar_len);

	drawBevel(buffer, SB_BORDER_WIDTH+BEVEL_HI_WIDTH,
			  height-SB_BUTTONS_HEIGHT+BEVEL_HI_WIDTH-SB_BORDER_WIDTH,
			  SB_WIDTH-SB_BORDER_SIZE-SIDE_STEP_WIDTH,
			  SB_BUTTON_HEIGHT);
	drawBevel(buffer, SB_BORDER_WIDTH+BEVEL_HI_WIDTH,
			  height-SB_BUTTON_HEIGHT+BEVEL_HI_WIDTH-SB_BORDER_WIDTH,
			  SB_WIDTH-SB_BORDER_SIZE-SIDE_STEP_WIDTH,
			  SB_BUTTON_HEIGHT);

	PlaceIcon(&dimple,
		  ((SB_WIDTH)>>1)-(SCROLLER_DIMPLE_WIDTH>>1)+1-SIDE_STEP_WIDTH,
		  scrollBar.top + BEVEL_HI_WIDTH + SB_BORDER_WIDTH +
		  ((scrollbar_len-SCROLLER_DIMPLE_HEIGHT)>>1),
		  buffer );

	PlaceIcon((scrollbar_isUp())?&UP_ARROW_HI:&UP_ARROW,
		  ((SB_WIDTH)>>1)-(ARROW_WIDTH>>1)+1-SIDE_STEP_WIDTH,
		  height - SB_BUTTONS_HEIGHT + BEVEL_HI_WIDTH+1-SB_BORDER_WIDTH,
		  buffer );

	PlaceIcon((scrollbar_isDn())?&DOWN_ARROW_HI:&DOWN_ARROW,
		  ((SB_WIDTH)>>1)-(ARROW_WIDTH>>1)+1-SIDE_STEP_WIDTH,
		  height - SB_BUTTON_HEIGHT+BEVEL_HI_WIDTH+1-SB_BORDER_WIDTH,
		  buffer );

#ifdef TRANSPARENT
	if( IS_TRANSP_SCROLL )
	    XSetClipMask( Xdisplay, paintGC, None );
#endif

    }

    if (Options & Opt_scrollBar_right)
	XCopyArea(Xdisplay, buffer, scrollBar.win, paintGC, 0, 0,
		  SB_WIDTH+SB_BORDER_WIDTH, height, 0, 0);
    else
	XCopyArea(Xdisplay, buffer, scrollBar.win, paintGC, 0, 0,
		  SB_WIDTH+SB_BORDER_WIDTH, height, 0-SB_BORDER_WIDTH, 0);

    XFreePixmap(Xdisplay, buffer);

    return 1;
}

/* PROTO */
int
scrollbar_show(int update)
{
    return scrollbar_show_cached(update, 0);
}


/* PROTO */
void
refresh_transparent_scrollbar()
{
#ifdef TRANSPARENT
    if( IS_TRANSP_SCROLL )	scrollbar_show_cached(0, 1);
#endif
}


/* PROTO */
void
map_scrollBar(int map)
{
    if (scrollbar_mapping(map)) {
	resize();
	scr_touch();
    }
}

/*----------------------- end-of-file (C source) -----------------------*/
