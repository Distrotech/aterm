#ifndef LIBAFTERSTEP_HEADER_INCLUDED
#define LIBAFTERSTEP_HEADER_INCLUDED

#include "../config.h"

/* some global stuff that needs to be defined for libasimage */
#ifdef USE_LIBASIMAGE
#include <X11/X.h>

#ifdef LIBASIMAGE_HEADERS

#define MODULE_X_INTERFACE

#include <afterstep/aftersteplib.h>
#include <afterstep/afterstep.h>
#define name_list void
#include <afterstep/screen.h>
#include <afterstep/ascolor.h>
#include <afterstep/stepgfx.h>
#include <afterstep/font.h>
#include <afterstep/mystyle.h>
#include <afterstep/mystyle_property.h>
#undef LEFT
#undef RIGHT

#ifndef MY_STYLE_FONT_ID
#define MY_STYLE_FONT_ID(ms)  ms.font.font
#else
#include <afterstep/resources.h>
#endif

#else  /* !LIBASIMAGE_HEADERS */

typedef struct ScreenInfo
{
    unsigned long screen; 
    Window Root;
    int d_depth;
    int MyDisplayWidth, MyDisplayHeight;
    int CurrentDesk;
}ScreenInfo; 

#endif /* LIBASIMAGE_HEADERS */

#ifdef INTERN
int DeadPipe( int nonsense ){return 1;}
ScreenInfo Scr ;
Display *dpy ;
char *MyName;
#endif /* INTERN */
#endif /* USE_LIBASIMAGE */

#endif	/*LIBAFTERSTEP_HEADER_INCLUDED*/
