/*--------------------------------*-C-*---------------------------------*
 * File:	pixmap.c
 *----------------------------------------------------------------------*
 * Copyright (c) 1999 Ethan Fischer <allanon@crystaltokyo.com>
 * Copyright (c) 1999 Sasha Vasko   <sasha at aftercode.net>
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
 *    1999	Sasha Vasko <sasha at aftercode.net>
 *----------------------------------------------------------------------*/

#ifndef lint
static const char rcsid[] = "$Id: pixmap.c,v 1.17 2005/06/21 20:08:16 sasha Exp $";
#endif

#include "rxvt.h"		/* NECESSARY */

int
pixmap_error_handler (Display * dpy, XErrorEvent * error)
{
#ifdef DEBUG_IMAGING
	show_error ("XError # %u, in resource %lu, Request: %d.%d",
				 error->error_code, error->resourceid, error->request_code, error->minor_code);
#endif
  return 0;
}



#ifndef HAVE_AFTERIMAGE

#include <X11/Xatom.h>
#define dpy Xdisplay
#define CREATE_TRG_PIXMAP(w,h) XCreatePixmap(Xdisplay, Xroot, w, h, Xdepth)

/* PROTO */
Pixmap
GetRootPixmap (Atom id)
{
	Pixmap currentRootPixmap = None;
#ifndef X_DISPLAY_MISSING
  	Atom act_type;
    int act_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;

/*fprintf(stderr, "\n aterm GetRootPixmap(): root pixmap is set");                  */
    if (XGetWindowProperty (  dpy, RootWindow(dpy,DefaultScreen(dpy)), _XROOTPMAP_ID, 0, 1, False, XA_PIXMAP,
							    &act_type, &act_format, &nitems, &bytes_after,
			    				&prop) == Success)
	{
		if (prop)
	  	{
	    	currentRootPixmap = *((Pixmap *) prop);
	    	XFree (prop);
/*fprintf(stderr, "\n aterm GetRootPixmap(): root pixmap is [%lu]", currentRootPixmap); */
		}
	}
#endif
    return currentRootPixmap;
}

/* PROTO */
Pixmap
ValidatePixmap (Pixmap p, int bSetHandler, int bTransparent, unsigned int *pWidth, unsigned int *pHeight)
{
#ifndef X_DISPLAY_MISSING
	int (*oldXErrorHandler) (Display *, XErrorEvent *) = NULL;
    /* we need to check if pixmap is still valid */
	Window root;
    int junk;
	if (bSetHandler)
		oldXErrorHandler = XSetErrorHandler (pixmap_error_handler);

    if (bTransparent)
	    p = GetRootPixmap (None);
	if (!pWidth)
  		pWidth = &junk;
    if (!pHeight)
	    pHeight = &junk;

    if (p != None)
	{
  		if (!XGetGeometry (dpy, p, &root, &junk, &junk, pWidth, pHeight, &junk, &junk))
			p = None;
    }
	if(bSetHandler)
  		XSetErrorHandler (oldXErrorHandler);

	return p;
#else
	return None ;
#endif
}

/* PROTO */
int
GetRootDimensions (int *width, int *height)
{
#ifndef X_DISPLAY_MISSING
	Window root;
	int w_x, w_y;
	unsigned int junk;
    if( dpy == NULL )
        return 0;
	if (!XGetGeometry (dpy, RootWindow(dpy,DefaultScreen(dpy)), &root,
					     &w_x, &w_y, width, height, &junk, &junk))
    {
    	*width = 0;
    	*height = 0;
    }
	return (*width > 0 && *height > 0) ? 1 : 0;
#else
	return 0;
#endif
}

/* PROTO */
int
GetWinPosition (Window w, int *x, int *y)
{
#ifndef X_DISPLAY_MISSING
	int bRes = 1;
	static int rootWidth = 0, rootHeight = 0;
	int my_x, my_y;
	Window wdumm;

	if (!x)
  		x = &my_x;
	if (!y)
  		y = &my_y;

	*x = 0;
	*y = 0;

	if (!rootWidth || !rootHeight)
  		if (!GetRootDimensions (&rootWidth, &rootHeight))
    		return 0;

	XTranslateCoordinates (dpy, w, RootWindow(dpy,DefaultScreen(dpy)), 0, 0, x, y, &wdumm);
	/* taking in to consideration virtual desktopping */
	if (*x < 0 || *x >= rootWidth || *y < 0 || *y >= rootHeight)
		bRes = 0;
	/* don't want to return position outside the screen even if we fail */
	while(*x < 0)
		*x += rootWidth;
	while (*y < 0)
		*y += rootHeight;
	if (*x > rootWidth)
		*x %= rootWidth;
	if (*y > rootHeight)
		*y %= rootHeight;
	return bRes;
#endif
	*x = 0;
	*y = 0;
	return 0;
}

void ShadeXImage( XImage* srcImage, ShadingInfo* shading, GC gc )
{
  int sh_r, sh_g, sh_b;
  RUINT32T mask_r, mask_g, mask_b;
  RUINT32T *lookup, *lookup_r, *lookup_g, *lookup_b;
  unsigned int lower_lim_r, lower_lim_g, lower_lim_b;
  unsigned int upper_lim_r, upper_lim_g, upper_lim_b;
  int i;

#ifdef DO_CLOCKING
  clock_t started = clock();
#endif

#ifdef ATERM
  Visual* visual = DefaultVisual(Xdisplay, Xscreen);
#else
  Visual* visual = DefaultVisual(dpy, DefaultScreen(dpy));
#endif

  if( visual->class != TrueColor || srcImage->format != ZPixmap ) return ;

  /* for convenience */
  mask_r = visual->red_mask;
  mask_g = visual->green_mask;
  mask_b = visual->blue_mask;

  /* boring lookup table pre-initialization */
  switch (srcImage->bits_per_pixel) {
    case 15:
      if ((mask_r != 0x7c00) ||
          (mask_g != 0x03e0) ||
          (mask_b != 0x001f))
        return;
        lookup = (RUINT32T *) malloc (sizeof (RUINT32T)*(32+32+32));
        lookup_r = lookup;
        lookup_g = lookup+32;
        lookup_b = lookup+32+32;
        sh_r = 10;
        sh_g = 5;
        sh_b = 0;
      break;
    case 16:
      if ((mask_r != 0xf800) ||
          (mask_g != 0x07e0) ||
          (mask_b != 0x001f))
        return;
        lookup = (RUINT32T *) malloc (sizeof (RUINT32T)*(32+64+32));
        lookup_r = lookup;
        lookup_g = lookup+32;
        lookup_b = lookup+32+64;
        sh_r = 11;
        sh_g = 5;
        sh_b = 0;
      break;
    case 24:
      if ((mask_r != 0xff0000) ||
          (mask_g != 0x00ff00) ||
          (mask_b != 0x0000ff))
        return;
        lookup = (RUINT32T *) malloc (sizeof (RUINT32T)*(256+256+256));
        lookup_r = lookup;
        lookup_g = lookup+256;
        lookup_b = lookup+256+256;
        sh_r = 16;
        sh_g = 8;
        sh_b = 0;
      break;
    case 32:
      if ((mask_r != 0xff0000) ||
          (mask_g != 0x00ff00) ||
          (mask_b != 0x0000ff))
        return;
        lookup = (RUINT32T *) malloc (sizeof (RUINT32T)*(256+256+256));
        lookup_r = lookup;
        lookup_g = lookup+256;
        lookup_b = lookup+256+256;
        sh_r = 16;
        sh_g = 8;
        sh_b = 0;
      break;
    default:
      return; /* we do not support this color depth */
  }

  /* prepare limits for color transformation (each channel is handled separately) */
  if (shading->shading < 0) {
    int shade;
    shade = -shading->shading;
    if (shade < 0) shade = 0;
    if (shade > 100) shade = 100;

    lower_lim_r = 65535-shading->tintColor.red;
    lower_lim_g = 65535-shading->tintColor.green;
    lower_lim_b = 65535-shading->tintColor.blue;

    lower_lim_r = 65535-(unsigned int)(((RUINT32T)lower_lim_r)*((RUINT32T)shade)/100);
    lower_lim_g = 65535-(unsigned int)(((RUINT32T)lower_lim_g)*((RUINT32T)shade)/100);
    lower_lim_b = 65535-(unsigned int)(((RUINT32T)lower_lim_b)*((RUINT32T)shade)/100);

    upper_lim_r = upper_lim_g = upper_lim_b = 65535;
  } else {
    int shade;
    shade = shading->shading;
    if (shade < 0) shade = 0;
    if (shade > 100) shade = 100;

    lower_lim_r = lower_lim_g = lower_lim_b = 0;

    upper_lim_r = (unsigned int)((((RUINT32T)shading->tintColor.red)*((RUINT32T)shading->shading))/100);
    upper_lim_g = (unsigned int)((((RUINT32T)shading->tintColor.green)*((RUINT32T)shading->shading))/100);
    upper_lim_b = (unsigned int)((((RUINT32T)shading->tintColor.blue)*((RUINT32T)shading->shading))/100);
  }

  /* switch red and blue bytes if necessary, we need it for some weird XServers like XFree86 3.3.3.1 */
  if ((srcImage->bits_per_pixel == 24) && (mask_r >= 0xFF0000 ))
  {
    unsigned int tmp;

    tmp = lower_lim_r;
    lower_lim_r = lower_lim_b;
    lower_lim_b = tmp;

    tmp = upper_lim_r;
    upper_lim_r = upper_lim_b;
    upper_lim_b = tmp;
  }

  /* fill our lookup tables */
  for (i = 0; i <= mask_r>>sh_r; i++)
  {
    RUINT32T tmp;
    tmp = ((RUINT32T)i)*((RUINT32T)(upper_lim_r-lower_lim_r));
    tmp += ((RUINT32T)(mask_r>>sh_r))*((RUINT32T)lower_lim_r);
    lookup_r[i] = (tmp/65535)<<sh_r;
  }
  for (i = 0; i <= mask_g>>sh_g; i++)
  {
    RUINT32T tmp;
    tmp = ((RUINT32T)i)*((RUINT32T)(upper_lim_g-lower_lim_g));
    tmp += ((RUINT32T)(mask_g>>sh_g))*((RUINT32T)lower_lim_g);
    lookup_g[i] = (tmp/65535)<<sh_g;
  }
  for (i = 0; i <= mask_b>>sh_b; i++)
  {
    RUINT32T tmp;
    tmp = ((RUINT32T)i)*((RUINT32T)(upper_lim_b-lower_lim_b));
    tmp += ((RUINT32T)(mask_b>>sh_b))*((RUINT32T)lower_lim_b);
    lookup_b[i] = (tmp/65535)<<sh_b;
  }

  /* apply table to input image (replacing colors by newly calculated ones) */
  switch (srcImage->bits_per_pixel)
  {
    case 15:
    {
      unsigned short *p1, *pf, *p, *pl;
      p1 = (unsigned short *) srcImage->data;
      pf = (unsigned short *) (srcImage->data + srcImage->height * srcImage->bytes_per_line);
      while (p1 < pf)
      {
        p = p1;
        pl = p1 + srcImage->width;
        for (; p < pl; p++)
        {
          *p = lookup_r[(*p & 0x7c00)>>10] |
               lookup_g[(*p & 0x03e0)>> 5] |
               lookup_b[(*p & 0x001f)];
        }
        p1 = (unsigned short *) ((char *) p1 + srcImage->bytes_per_line);
      }
      break;
    }
    case 16:
    {
      unsigned short *p1, *pf, *p, *pl;
      p1 = (unsigned short *) srcImage->data;
      pf = (unsigned short *) (srcImage->data + srcImage->height * srcImage->bytes_per_line);
      while (p1 < pf)
      {
        p = p1;
        pl = p1 + srcImage->width;
        for (; p < pl; p++)
        {
          *p = lookup_r[(*p & 0xf800)>>11] |
               lookup_g[(*p & 0x07e0)>> 5] |
               lookup_b[(*p & 0x001f)];
        }
        p1 = (unsigned short *) ((char *) p1 + srcImage->bytes_per_line);
      }
      break;
    }
    case 24:
    {
      unsigned char *p1, *pf, *p, *pl;
      p1 = (unsigned char *) srcImage->data;
      pf = (unsigned char *) (srcImage->data + srcImage->height * srcImage->bytes_per_line);
      while (p1 < pf)
      {
        p = p1;
        pl = p1 + srcImage->width * 3;
        for (; p < pl; p += 3)
        {
          p[0] = lookup_r[(p[0] & 0xff0000)>>16];
          p[1] = lookup_r[(p[1] & 0x00ff00)>> 8];
          p[2] = lookup_r[(p[2] & 0x0000ff)];
        }
        p1 = (unsigned char *) ((char *) p1 + srcImage->bytes_per_line);
      }
      break;
    }
    case 32:
    {
      RUINT32T *p1, *pf, *p, *pl;
      p1 = (RUINT32T *) srcImage->data;
      pf = (RUINT32T *) (srcImage->data + srcImage->height * srcImage->bytes_per_line);

      while (p1 < pf)
      {
        p = p1;
        pl = p1 + srcImage->width;
        for (; p < pl; p++)
        {
          *p = lookup_r[(*p & 0xff0000)>>16] |
               lookup_g[(*p & 0x00ff00)>> 8] |
               lookup_b[(*p & 0x0000ff)] |
               (*p & ~0xffffff);
        }
        p1 = (RUINT32T *) ((char *) p1 + srcImage->bytes_per_line);
      }
      break;
    }
  }

  free (lookup);

#ifdef DO_CLOCKING
    printf( "\n Shading time (clocks): %lu\n",clock()-started );
#endif

}

void CopyAndShadeArea( Drawable src, Pixmap trg,
                       int x, int y, int w, int h,
		       int trg_x, int trg_y,
		       GC gc, ShadingInfo* shading )
{
  int (*oldXErrorHandler) (Display *, XErrorEvent *) ;
   /* we need to check if pixmap is still valid */
    oldXErrorHandler = XSetErrorHandler (pixmap_error_handler);

    if( shading )
    {
      XImage* img ;

        if( x <0 || y <0  ) return ;
        if((img = XGetImage (Xdisplay, src, x, y, w, h, AllPlanes, ZPixmap))!= NULL )
        {
    	    ShadeXImage( img, shading, gc );
    	    XPutImage(Xdisplay, trg, gc, img, 0, 0, trg_x, trg_y, w, h);
#ifdef BENCHMARK_SHADING
	    {
		int i;
		time_t before, after;
		double diff;

		before = time (NULL);
		for (i = 0; i < BENCHMARK_SHADING; i++)
		    ShadeXImage (img, shading, gc);
		after = time (NULL);

		diff = difftime (after, before);
		printf ("CopyAndShadeArea(): %d shading runs took %.0f seconds\n", BENCHMARK_SHADING, diff);
	    }
#endif
	    XDestroyImage( img );
	    return ;
	}
    }
    if( !XCopyArea (Xdisplay, src, trg, gc, x, y, w, h, trg_x, trg_y))
		XFillRectangle( Xdisplay, trg, gc, trg_x, trg_y, w, h );
    XSetErrorHandler (oldXErrorHandler);
}

void ShadeTiledPixmap(Pixmap src, Pixmap trg, int src_w, int src_h, int x, int y, int w, int h, GC gc, ShadingInfo* shading)
{
  int tile_x, tile_y, left_w, bott_h;

	if( src_w > 0 && src_h > 0 )
	{
    	tile_x = x%src_w ;
	    tile_y = y%src_h ;
	    left_w = min(src_w-tile_x,w);
  		bott_h = min(src_h-tile_y,h);
/*fprintf( stderr, "\nShadeTiledPixmap(): tile_x = %d, tile_y = %d, left_w = %d, bott_h = %d, SRC = %dx%d TRG=%dx%d", tile_x, tile_y, left_w, bott_h, src_w, src_h, w, h);*/
	    CopyAndShadeArea( src, trg, tile_x, tile_y, left_w, bott_h, 0, 0, gc, shading );
  		if( bott_h < h )
	    {  /* right-top parts */
			CopyAndShadeArea( src, trg, tile_x, 0, left_w, h-bott_h, 0, bott_h, gc, shading );
	    }
  		if( left_w < w )
	    {  /* left-bott parts */
			CopyAndShadeArea( src, trg, 0, tile_y, w-left_w, bott_h, left_w, 0, gc, shading );
	        if( bott_h < h  )      /* left-top parts */
			    CopyAndShadeArea( src, trg, 0, 0, w-left_w, h-bott_h, left_w, bott_h, gc, shading );
	    }
	}
}

Pixmap
ShadePixmap (Pixmap src, int x, int y, int width, int height, GC gc, ShadingInfo* shading )
{
  Pixmap trg = CREATE_TRG_PIXMAP(width, height);
    if( trg != None )
    {
	CopyAndShadeArea( src, trg, x, y, width, height, 0, 0, gc, shading );
    }
    return trg ;
}


void
sleep_a_little (int n)
{
  struct timeval value;

  if (n <= 0)
    return;

  value.tv_usec = n % 1000000;
  value.tv_sec = n / 1000000;

  (void) select (1, 0, 0, 0, &value);
}


static Pixmap
CutPixmap ( Pixmap src, Pixmap trg,
            int x, int y,
	    unsigned int src_w, unsigned int src_h,
	    unsigned int width, unsigned int height,
	    GC gc, ShadingInfo * shading)
{
  Bool my_pixmap = (trg == None )?True:False ;
  int screen_w, screen_h ;
  int offset_x = 0, offset_y = 0 ;

  screen_w = DisplayWidth( Xdisplay, Xscreen );
  screen_h = DisplayHeight( Xdisplay, Xscreen );

  while( x+(int)width < 0 )  x+= screen_w ;
  while( x >= screen_w )  x-= screen_w ;
  while( y+(int)height < 0 )  y+= screen_h ;
  while( y >= screen_h )  y-= screen_h ;

  if (width < 2 || height < 2 )
    return trg;
  if( x < 0 )
  {
    offset_x = (-x) ;
    x = 0 ;
    width -= offset_x ;
  }
  if( y < 0 )
  {
    offset_y = (-y) ;
    y = 0 ;
    height -= offset_y ;
  }
  if( x+width >= screen_w ) width = screen_w - x ;
  if( y+height >= screen_h ) height = screen_h - y ;

  if (src == None) /* we don't have root pixmap ID */
    { /* we want to create Overrideredirect window overlapping out window
         with background type of Parent Relative and then grab it */
     XSetWindowAttributes attr ;
     XEvent event ;
     int tick_count = 0 ;
     Bool grabbed = False ;
        attr.background_pixmap = ParentRelative ;
	attr.backing_store = Always ;
	attr.event_mask = ExposureMask ;
	attr.override_redirect = True ;
	src = XCreateWindow(Xdisplay, Xroot, x, y, width, height,
	                    0,
			    CopyFromParent, CopyFromParent, CopyFromParent,
			    CWBackPixmap|CWBackingStore|CWOverrideRedirect|CWEventMask,
			    &attr);

	if( src == None ) return trg ;
	XGrabServer( Xdisplay );
	grabbed = True ;
	XMapRaised( Xdisplay, src );
	XSync(Xdisplay, False );
	/* now we have to wait for our window to become mapped - waiting for Expose */
	for( tick_count = 0 ; !XCheckWindowEvent( Xdisplay, src, ExposureMask, &event ) && tick_count < 100 ; tick_count++)
	    sleep_a_little(100);

	if( tick_count < 100 )
	{
	    if( trg == None )    trg = CREATE_TRG_PIXMAP (width+offset_x, height+offset_y);
	    if (trg != None)
	    {	/* custom code to cut area, so to ungrab server ASAP */
	        if (shading)
	        {
	          XImage *img;
		  img = XGetImage (Xdisplay, src, 0, 0, width, height, AllPlanes, ZPixmap);
    		  XDestroyWindow( Xdisplay, src );
	          src = None ;
		  XUngrabServer( Xdisplay );
		  grabbed = False ;
		  if (img != NULL)
		  {
    		    ShadeXImage (img, shading, gc);
		    XPutImage (Xdisplay, trg, gc, img, 0, 0, offset_x, offset_y, width, height);
#ifdef BENCHMARK_SHADING
	    {
		int i;
		time_t before, after;
		double diff;

		before = time (NULL);
		for (i = 0; i < BENCHMARK_SHADING; i++)
		    ShadeXImage (img, shading, gc);
		after = time (NULL);

		diff = difftime (after, before);
		printf ("CutPixmap(): %d shading runs took %.0f seconds\n", BENCHMARK_SHADING, diff);
	    }
#endif
		    XDestroyImage (img);
		  }else if( my_pixmap )
		  {

		    XFreePixmap( Xdisplay, trg );
		    trg = None ;
		  }
		}else
		    XCopyArea (Xdisplay, src, trg, gc, 0, 0, width, height, offset_x, offset_y);
	    }
        }

	if( src )
	    XDestroyWindow( Xdisplay, src );
	if( grabbed )
	    XUngrabServer( Xdisplay );
	return trg ;
    }
  /* we have root pixmap ID */
  /* find out our coordinates relative to the root window */
  if (x + width > src_w || y + height > src_h)
    {			/* tiled pixmap processing here */
      Pixmap tmp ;
      width = min (width, src_w);
      height = min (height, src_h);

      tmp = CREATE_TRG_PIXMAP (width, height);
      if (tmp != None)
      {
        ShadeTiledPixmap (src, tmp, src_w, src_h, x, y, width,
			  height, gc, shading);
        if( trg == None )
           trg = CREATE_TRG_PIXMAP (width+offset_x, height+offset_y);
	if( trg != None )
	    XCopyArea (Xdisplay, tmp, trg, gc, 0, 0, width, height, offset_x, offset_y);

	XFreePixmap( Xdisplay, tmp );
        return trg;
      }
    }

  /* create target pixmap of the size of the window */
  if( trg == None )    trg = CREATE_TRG_PIXMAP (width+offset_x, height+offset_y);
  if (trg != None)
    {
      /* cut area */
      CopyAndShadeArea (src, trg, x, y, width, height, offset_x, offset_y, gc, shading);

    }

  return trg;
}

/* PROTO */
Pixmap
CutWinPixmap (Window win, Drawable src, int src_w, int src_h, int width, int height, GC gc, ShadingInfo * shading)
{
  unsigned int x = 0, y = 0;


  if( src == None )
  {
#ifndef TRANSPARENT
    return None ;
#else
    if( !(Options & Opt_transparent))
	return None ;
#endif
  }

  if (!GetWinPosition (win, &x, &y))
	return None;

  return CutPixmap( src, None, x, y, src_w, src_h, width, height, gc, shading );
}

/* PROTO */
int
FillPixmapWithTile (Pixmap pixmap, Pixmap tile, int x, int y, int width, int height, int tile_x, int tile_y)
{
  if (tile != None && pixmap != None)
    {
      GC gc;
      XGCValues gcv;

      gcv.tile = tile;
      gcv.fill_style = FillTiled;
      gcv.ts_x_origin = -tile_x;
      gcv.ts_y_origin = -tile_y;
      gc =
	XCreateGC (Xdisplay, tile,
		   GCFillStyle | GCTile | GCTileStipXOrigin |
		   GCTileStipYOrigin, &gcv);
      XFillRectangle (Xdisplay, pixmap, gc, x, y, width, height);
      XFreeGC (Xdisplay, gc);
      return 1;
    }
  return 0;
}


#endif /* HAVE_AFTERIMAGE */

/***************************************************************************/
/*     Down below goes aterm specific functions                            */
/***************************************************************************/

#define BG TermWin.background /*for convinience*/

void FreeTargetPixmap()
{
    if( BG.trgPixmap != None )
    {
	XFreePixmap( Xdisplay, BG.trgPixmap ); /*just in case*/
	BG.trgPixmap = None ;
    }
}


/* PROTO */
void
SetSrcPixmap(Pixmap p)
{
#ifdef DEBUG_BACKGROUND_PMAP	   
	fprintf(stderr, "srcPixmap = %lx, new srcPixmap = %lx.\n", BG.srcPixmap, p );
#endif
    
	if( BG.srcPixmap != None && BG.bMySource && BG.srcPixmap != p )
    {
		XFreePixmap( Xdisplay, BG.srcPixmap );
		BG.srcPixmap = p ;
		BG.bMySource = 0 ;
    }

    BG.srcPixmap = p ;

    BG.Width = 0 ;
    BG.Height = 0 ;

    if( BG.srcPixmap != None )
    {
		Window root;
      	unsigned int dum, w, h;
      	int dummy;
#ifdef DEBUG_BACKGROUND_PMAP	   
		fprintf(stderr, "Querying geometry of the source pixmap %lX...", BG.srcPixmap );
#endif
		if (XGetGeometry (Xdisplay, BG.srcPixmap, &root, &dummy, &dummy, &w, &h, &dum, &dum))
		{
	    	BG.Width = w ;
	    	BG.Height = h ;
#ifdef DEBUG_BACKGROUND_PMAP	   
			fprintf(stderr, "success : geometry is %dx%d.\n", w, h );
#endif
		}else
		{
#ifdef DEBUG_BACKGROUND_PMAP	   
			fprintf(stderr, "failed.\n" );
#endif
		    BG.srcPixmap = None ;
		}	 
    }
}

/* PROTO */
void
ValidateSrcPixmap(int bSetHandler)
{

    if( !BG.bMySource )
    {    /* we need to check if pixmap is still valid */
    	Pixmap new_p ;

		new_p = ValidatePixmap( BG.srcPixmap , bSetHandler,
				     	        ((Options & Opt_transparent) &&
			    		      	BG.trgType != BGT_None),
								NULL, NULL );

		if( new_p != BG.srcPixmap )	SetSrcPixmap(new_p);
	}
}



/* PROTO */
int
GetMyPosition( int* x, int* y )
{
  int bRet = 0 ;
  int (*old) (Display *, XErrorEvent *) = XSetErrorHandler (pixmap_error_handler);

    bRet = GetWinPosition( TermWin.vt, x, y );
    XSetErrorHandler (old);

    return bRet ;
}

/* PROTO */
Bool 
IsTransparentPixmap()
{
#ifdef _MYSTYLE_
 	if( BG.trgType == BGT_MyStyle && TransparentMS(TermWin.background.mystyle))
		return True;
#endif
#ifdef TRANSPARENT
	if(get_flags(Options, Opt_transparent) && BG.trgType != BGT_None )
		return True;
#endif
	return False ;
}	 

/* PROTO */
Bool 
TransparentPixmapNeedsUpdate()
{
#ifdef DEBUG_BACKGROUND_PMAP	
	fprintf( stderr, "%s: checking if transparent\n", __FUNCTION__ );
#endif
	if( !IsTransparentPixmap() ) 
		return False;

#ifdef DEBUG_BACKGROUND_PMAP	   
	fprintf( stderr, "%s: checking if same desktop\n", __FUNCTION__ );
#endif
	if( ExtWM.current_desktop != ExtWM.aterm_desktop && ExtWM.aterm_desktop != 0xFFFFFFFF &&
		get_flags( ExtWM.flags, WM_SupportsDesktops )&& !get_flags( ExtWM.flags, WM_AtermStateSticky ) ) 	   
		return False;

	if( get_flags( ExtWM.flags, WM_AtermStateShaded|WM_AtermStateHidden ) ) 	   
		return False;

#ifdef DEBUG_BACKGROUND_PMAP	   
	fprintf( stderr, "%s: checking if pixmap/position changed\n", __FUNCTION__ );
	fprintf( stderr, "%s: last pixmap = %lX new pixmap = %lX\n", __FUNCTION__, TermWin.LastPixmapUsed, BG.srcPixmap );
#endif
	if( TermWin.LastPixmapUsed == BG.srcPixmap &&
		TermWin.LastPixmap_root_x == TermWin.root_x && 
		TermWin.LastPixmap_root_y == TermWin.root_y &&
		TermWin.LastPixmap_width  == TermWin.root_width &&
		TermWin.LastPixmap_height == TermWin.root_height )
		return False; 
#ifdef DEBUG_BACKGROUND_PMAP	   
   	fprintf( stderr, "%s: checking if visible\n", __FUNCTION__ );
#endif
	if( TermWin.root_x+TermWin.root_width <= 0 || TermWin.root_y+TermWin.root_height <= 0 ||
		TermWin.root_x >= XdisplayWidth || TermWin.root_y >= XdisplayHeight )
		return False;
#ifdef DEBUG_BACKGROUND_PMAP	   
	fprintf( stderr, "%s: all clear - needs update\n", __FUNCTION__ );
#endif

	return True;		   
}	 


#ifdef _MYSTYLE_
Pixmap
RenderMyStylePixmap( MyStyle *style, Pixmap root_pmap,
		     unsigned int root_pmap_width, unsigned int root_pmap_height,
		     unsigned int width, unsigned int height )
{
	Pixmap p = None;

/*  fprintf( stderr, "Entering RenderMyStylePixmap : texture_type = %d\n", style->texture_type );
*/
  
	if (style->texture_type > TEXTURE_SOLID)
  	{
    	int real_x, real_y ;
		ASImage *im ;
        GetMyPosition( &real_x, &real_y);
		
		if( Scr.RootImage != NULL ) 
		{
			if( Scr.RootClipArea.x != real_x ||	Scr.RootClipArea.y != real_y ||
				Scr.RootClipArea.width != width  ||	Scr.RootClipArea.height != height  )
			{
				destroy_asimage( &Scr.RootImage );
			}
		}	 
		Scr.RootClipArea.x = real_x ;
		Scr.RootClipArea.y = real_y ;
		Scr.RootClipArea.width = width ;
		Scr.RootClipArea.height = height ;
		
		im = mystyle_make_image( style, real_x, real_y, width, height, 0 );  	  
		p = asimage2pixmap( asv, Xroot, im, NULL, True );
		destroy_asimage( &im );
	}	   
	return p;
}
#endif



/* PROTO */
void
RenderPixmap(int DontCheckSource )
{
	XGCValues       gcvalue;
	GC              gc;
	int    width = TermWin_TotalWidth();
	int    height = TermWin_TotalHeight();
 	unsigned int    fin_width, fin_height ;
	int (*oldXErrorHandler) (Display *, XErrorEvent *) ;

   	set_background_updated();	
	
    /* we have some nice processing of all the X errors built in */
    /* so let's not let us crash if anything goes wrong          */
    oldXErrorHandler = XSetErrorHandler (pixmap_error_handler);
#ifdef DEBUG_BACKGROUND_PMAP	   
	XSetErrorHandler (oldXErrorHandler);
#endif	

    if( !DontCheckSource )  ValidateSrcPixmap( 0 );

	if( IsTransparentPixmap() )
	{
		if( ExtWM.current_desktop != ExtWM.aterm_desktop &&
			get_flags( ExtWM.flags, WM_SupportsDesktops ) ) 	   
		{
			XSetErrorHandler (oldXErrorHandler);
			return;
		}		
	}		   



    if( BG.srcPixmap == None
#ifdef _MYSTYLE_
	&& BG.trgType != BGT_MyStyle
#endif
      )
    {
#ifdef TRANSPARENT
		if(!(Options & Opt_transparent) || BG.trgType == BGT_None)
#endif
		{
	    	XSetErrorHandler (oldXErrorHandler);
	    	return ; /* nothing to do here */
		}
		if( DontCheckSource )  
			ValidateSrcPixmap( 0 );
    }
    
	/* for convinience only */
    fin_width = width ;
    fin_height = height ;
/*fprintf(stderr, "\n aterm: entering RenderPixmap, window size is %dx%d, trg_type = %d", width, height, BG.trgType );
*/
    gcvalue.foreground = PixColors[Color_bg];
#ifdef HAVE_AFTERIMAGE
    gc = create_visual_gc(asv, TermWin.vt, GCForeground, &gcvalue);
#else
	gc = XCreateGC(Xdisplay, TermWin.vt, GCForeground, &gcvalue);
#endif


/*fprintf(stderr, "\n aterm RenderPixmap(): freeing target pixmap ...");
*/
    if( BG.trgPixmap != BG.srcPixmap )	
		FreeTargetPixmap();
    
#define SHADING ((BG.bMySource)?NULL:&(BG.Shading))
	
	if( BG.trgType == BGT_Tile )  /* just copying source PixampID into trgPixmapID */
	{	
		if( BG.bMySource || NO_NEED_TO_SHADE(BG.Shading))
		{
    		    BG.trgPixmap = BG.srcPixmap ;
		    fin_width = BG.Width ;
		    fin_height = BG.Height;
		}else if( (BG.finWidth != width || BG.finHeight!=height ) &&
			  	  (BG.Width != BG.finWidth || BG.Height != BG.finHeight))
		{
		    fin_width = min(BG.Width, width);
		    fin_height = min(BG.Height, height);
		    BG.trgPixmap = ShadePixmap(BG.srcPixmap, 0, 0, fin_width, fin_height, gc, SHADING);
		}
	}
#ifdef HAVE_AFTERIMAGE
	else if( BG.trgType == BGT_Scale )
	{	
		if( BG.Width != width || BG.Height!=height )
		{
#ifdef DEBUG_BACKGROUND_PMAP	   
fprintf(stderr,  "aterm RenderPixmap(): (BGT_Scale) src pixmap is [%lX], scaling from %dx%d to %dx%d \n",BG.srcPixmap, BG.Width, BG.Height, width, height);
#endif
	    	    BG.trgPixmap = ScalePixmap( BG.srcPixmap,
											BG.Width, BG.Height,
		                                	width, height, gc, SHADING );
				fin_width = width ; 
				fin_height = height ; 
#ifdef DEBUG_BACKGROUND_PMAP	   
fprintf(stderr, "aterm RenderPixmap(): trg pixmap is [%lX]\n",BG.trgPixmap);		
#endif
		}
	}else if( BG.trgType == BGT_ScaleH )
	{	
		if( BG.Width != width )
		{
	    	    BG.trgPixmap = ScalePixmap( BG.srcPixmap,
						BG.Width, BG.Height,
		                                width, BG.Height, gc, SHADING );
			    fin_height = BG.Height;
				fin_width = width ; 
		}
	}else if( BG.trgType == BGT_ScaleV )
	{	
		if( BG.Height!=height )
		{
#ifdef DEBUG_BACKGROUND_PMAP	   
fprintf(stderr,  "aterm RenderPixmap(): (BGT_Scale) src pixmap is [%lX], scaling from %dx%d to %dx%d \n",BG.srcPixmap, BG.Width, BG.Height, BG.Width, height);
#endif
	    	    BG.trgPixmap = ScalePixmap( BG.srcPixmap,
						BG.Width, BG.Height,
		                                BG.Width, height, gc, SHADING );
		    fin_width = BG.Width ;
			fin_height = height ; 
		}
	}
#endif
	else if( BG.trgType == BGT_Cut )
	{	
#ifdef DEBUG_BACKGROUND_PMAP	   
		fprintf(stderr, "aterm RenderPixmap(): (BG_Cut)src pixmap is [%lX] %dx%d %dx%d\n", BG.srcPixmap, BG.Width, BG.Height, width, height);
#endif
		BG.trgPixmap = CutWinPixmap( TermWin.vt, BG.srcPixmap,
					  				 BG.Width, BG.Height, width, height, gc,
					  				 SHADING );
#ifdef DEBUG_BACKGROUND_PMAP	   
		fprintf(stderr, "aterm RenderPixmap(): (BG_Cut)trg pixmap is [%lX]\n", BG.trgPixmap);
#endif
	}		
#ifdef _MYSTYLE_
	else if( BG.trgType == BGT_MyStyle )
	{	
		BG.trgPixmap = RenderMyStylePixmap( BG.mystyle, BG.srcPixmap,
					            BG.Width, BG.Height,
		                                    width, height );
		if( BG.mystyle->texture_type == TEXTURE_PIXMAP)
		{
		    BG.srcPixmap = BG.mystyle->back_icon.pix ; /* so not to free it later */
		    BG.bMySource = False ;
		}

		/* fprintf(stderr, "Mystyle generated : %lX\n", BG.trgPixmap ); */
	}
#endif

    XFreeGC(Xdisplay, gc); /* don't need anymore */
    if( BG.trgPixmap != None )
    {
		BG.finWidth = fin_width ;
		BG.finHeight = fin_height;
		XSync(Xdisplay, 0);
#ifdef DEBUG_BACKGROUND_PMAP	   
		fprintf(stderr, "Setting background to %lX. srcPixmap = %lX\n", BG.trgPixmap, BG.srcPixmap );
#endif
        XSetWindowBackgroundPixmap(Xdisplay, TermWin.vt,
    	                           BG.trgPixmap);
    	XSync(Xdisplay, 0);	
		TermWin.LastPixmapUsed = BG.srcPixmap ;
		TermWin.LastPixmap_root_x = TermWin.root_x ; 
		TermWin.LastPixmap_root_y = TermWin.root_y ; 
		TermWin.LastPixmap_width  = TermWin.root_width ; 
		TermWin.LastPixmap_height = TermWin.root_height ; 
		BG.trgPixmapSet = 1 ;


		if( BG.trgPixmap != BG.srcPixmap )
		{/* don't need it anymore server has it */
	    	XFreePixmap( Xdisplay, BG.trgPixmap );
	    	XSync(Xdisplay, 0);
		}
		BG.trgPixmap = None ;
    }
    /* restore old handler so we can crash again ;) */
    XSetErrorHandler (oldXErrorHandler);

} /******************************* RenderPixmap **********************/


#ifdef BACKGROUND_IMAGE
/* we need this stuff only to load background image from file */

/* PROTO */
int
parse_pixmap_geom(const char *geom)
{
  int             w = 0, h = 0, x = 0, y = 0;
  int             flags, changed = 0;

    if (geom == NULL)	return 0;
    if (!strlen(geom))	return 0;

    if (!strcmp(geom, "?"))
    {
    	static char     str[] = "[10000x10000+10000+10000]";	/* should be big enough */
		sprintf(str, "[%dx%d+%d+%d]", BG.srcWidth, BG.srcHeight, BG.srcX, BG.srcY );
		xterm_seq(XTerm_title, str);
		return 0;
    }
/*fprintf( stderr, "\n parse_pixmap_geom(): geometry is [%s]", geom );*/
    flags = XParseGeometry(geom, &x, &y, (unsigned int *) &w, (unsigned int *) &h);
    if(!(flags & XValue))  x = 0 ;
    if(!(flags & YValue))  y = 0 ;
    if(!(flags & WidthValue))  w = -1 ;
    if(!(flags & HeightValue))  h = -1 ;
    MIN_IT(x, 10000);
    MIN_IT(y, 10000);
    MIN_IT(w, 10000);
    MIN_IT(h, 10000);

    if( w != BG.srcWidth )
    {
        changed++;
        BG.srcWidth = w ;
    }
    if( h != BG.srcHeight )
    {
        changed++;
        BG.srcHeight = h ;
    }
    if( x != BG.srcX )
    {
        changed++;
        BG.srcX = x ;
    }
    if( y != BG.srcY )
    {
        changed++;
        BG.srcY = y ;
    }
/*fprintf( stderr, "\n parse_pixmap_geom(): geometry is [%dx%d+%d+%d]", w,h,x,y );    */
    return changed;
}

/* PROTO */
void
LoadBGPixmap(const char *file)
{

#ifdef HAVE_AFTERIMAGE
	ASImage 		*im ;


    if (BG.srcPixmap != None && BG.bMySource )
    {
		XFreePixmap(Xdisplay, BG.srcPixmap);
		BG.srcPixmap = None;
    }

	im = file2ASImage( file, 0xFFFFFFFF, SCREEN_GAMMA, 0, getenv("PATH"), NULL );

    /* need to add geometry processing code here */
    if( im == NULL )
    {
		XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[Color_bg]);
		BG.bMySource = 0 ;
		BG.Width = 0 ;
		BG.Height = 0;
		BG.trgPixmapSet = 0 ;
    }else
    {
		BG.bMySource = 1 ;
		BG.Width = im->width;
		BG.Height = im->height;
       	BG.srcPixmap = asimage2pixmap( asv, Xroot, im, NULL, True);
		destroy_asimage( &im );	   
	}
	RenderPixmap(1);
#endif
    scr_touch();
}
#endif				/* BACKGROUND_IMAGE */
