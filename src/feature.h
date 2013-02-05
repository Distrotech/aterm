/*
 * File:	feature.h
 * $Id: feature.h,v 1.3 2005/12/06 17:08:44 sasha Exp $
 *
 * Compile-time configuration.
 *-----------------------------------------------------------------------
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
 *
 *----------------------------------------------------------------------*/
#ifndef _FEATURE_H
#define _FEATURE_H

/*-----------------------SCREEN OPTIONS AND COLOURS---------------------*/
/*
 * Define the name of the environment variable to be used in
 * addition to the "PATH" environment and the `path' resource.
 * Usually it should point to where you keep your background pixmaps and/or
 * your menu files
 * ATERM_PATH_ENV (if defined) gets searched first
 */
#define ATERM_PATH_ENV	"ATERMPATH"
#define PATH_ENV	"RXVTPATH"

/* enable by default - only needs to be disabled on 8bpp displays to 
 * conserve colors ! */
#define OFF_FOCUS_FADING 1

/*
 * Avoid enabling the colour cursor (-cr, cursorColor, cursorColor2)
 */
/* #define NO_CURSORCOLOR */
 
/*
 * Suppress use of BOLD and BLINK attributes for setting bright foreground
 * and background, respectively.  Simulate BOLD using colorBD, boldFont or
 * overstrike characters.
 */
/* #define NO_BRIGHTCOLOR */

/*
 * Disable separate colours for bold/underline
 */
/* #define NO_BOLDUNDERLINE */

/*
 * Disable using simulated bold using overstrike.  You can also turn off
 * overstrike just for multi char fonts
 * Note: NO_BOLDOVERSTRIKE implies NO_BOLDOVERSTRIKE_MULTI
 */
/* #define NO_BOLDOVERSTRIKE */
/* #define NO_BOLDOVERSTRIKE_MULTI */

/*
 * Also use bold font or overstrike even if we use colour for bold
 */
#define VERYBOLD 
 
/*
 * Compile without support for real bold fonts
 */
/* #define NO_BOLDFONT */
 
/*
 * Limit the number of screenfulls between screen refreshes during hard & fast
 * scrolling [default: 1]
 */
/* #define REFRESH_PERIOD	1 */

/*
 * If the screen has 24 bit mode, use that even if the default is 8 bit.
 */
#define PREFER_24BIT

/*
 * Printer pipe which will be used for emulation of attached vt100 printer
 */
/* #define PRINTPIPE	"lpr" */

/*------------------------------RESOURCES-------------------------------*/
/*
 * Define where to find installed application defaults for rxvt
 * Only if USE_XGETDEFAULT is not defined.
 */
#ifndef XAPPLOADDIR
/* #define XAPPLOADDIR	"/usr/lib/X11/app-defaults" */
#endif
 
/*
 * Add support for the Offix DND (Drag 'n' Drop) protocol
 */
/* #define OFFIX_DND */

/*---------------------------------KEYS---------------------------------*/

/*
 * Define defaults for backspace and delete keys - unless they have been
 * configured out with --disable-backspace-key / --disable-delete-key
 */
/* #define DEFAULT_BACKSPACE	"DEC"		*/ /* SPECIAL */
/* #define DEFAULT_BACKSPACE	"\177"		*/
/* #define DEFAULT_DELETE	"\033[3~"	*/

/*
 * Choose one of these values to be the `hotkey' for changing font.
 * This has been superceded and is only for you older users
 */
/* #define HOTKEY_CTRL */
/* #define HOTKEY_META */

/*
 * To use
 *	Home = "\E[1~", End = "\E[4~"
 * instead of
 *	Home = "\E[7~", End = "\E[8~"	[default]
 */
/* #define LINUX_KEYS */

/*
 * Enable the keysym resource which allows you to define strings associated
 * with various KeySyms (0xFF00 - 0xFFFF).
 * Only works with the default hand-rolled resources.
 */
#ifndef NO_RESOURCES
# define KEYSYM_RESOURCE
#endif

/*
 * Allow unshifted Next/Prior keys to scroll forward/back
 * (in addition to shift+Next/shift+Prior)       --pjh
 */
/* #define UNSHIFTED_SCROLLKEYS */
  
/*--------------------------------MOUSE---------------------------------*/
/*
 * Disable sending escape sequences (up, down, page up/down)
 * from the scrollbar when XTerm mouse reporting is enabled
 */
/* #define NO_SCROLLBAR_REPORT */

#ifdef NEXT_SCROLLBAR
#undef SB_WIDTH
/* could be anything from 13 to 19  but the true NeXT is 17 */ 
#define SB_WIDTH		17 
/* this will define somemore parameters for shaping NeXTish scrollbars */
/* NEXT_SCROLL_CLEAN if defined removes shades of gray from buttons */
#undef NEXT_SCROLL_CLEAN   
#define NEXT_SCROLL_SQUARE_ARROWS 
#define SB_BORDER_WIDTH 1
/* this makes buttons thinner then scrollbar's base ( if more then 0 ) */ 
#define SIDE_STEP_WIDTH 0
/* end NeXT scrollbar specific fetures */
#else
#ifdef XTERM_SCROLLBAR
#undef  SB_WIDTH
#define SB_WIDTH		15
#else
#if !defined (SB_WIDTH) || (SB_WIDTH < 8)
#undef SB_WIDTH
#define SB_WIDTH		11	/* scrollBar width */
#endif
#endif				/* XTERM_SCROLLBAR */
#endif

/*
 * Default separating chars for multiple-click selection
 * Space and tab are separate separating characters and are not settable
 */
#define CUTCHARS	"\"&'()*,;<=>?@[\\]^`{|}~"

/*
 * Add run-time support for changing the cutchars for double click selection
 */
#define CUTCHAR_RESOURCE

/*
 * Have mouse reporting include double-click info for button1
 */
/* #define MOUSE_REPORT_DOUBLECLICK */

/*
 * Set delay between multiple click events [default: 500]
 */
/* #define MULTICLICK_TIME 500 */

/*
 * Set delay periods for continuous scrolling with scrollbar buttons
 */
/* #define SCROLLBAR_INITIAL_DELAY 40 */
/* #define SCROLLBAR_CONTINUOUS_DELAY 2 */

/*--------------------------------BELL----------------------------------*/
/*
 * Disable automatic de-iconify when a bell is received
 */
/* #define NO_MAPALERT */

/*
 * Have mapAlert behaviour selectable with mapAlert resource
 */
#define MAPALERT_OPTION

/*-----------------------------SCROLL BAR-------------------------------*/
/*
 * Choose the scrollbar width - should be an even number [default: 10]
 * Except for XTERM_SCROLLBAR: it is *always* 15
 * 	and for NEXT_SCROLLBAR, which is *always* 19
 */
/* #define SB_WIDTH 10 */

/* 
 * When using Rxvt scrollbar, clicking above or below the slider will move
 * 1/4 of the screen height, if possible.  Setting RXVT_SCROLL_FULL will move
 * it one screen height less one line, if possible
 */
#define RXVT_SCROLL_FULL 1

/* 
 * (Hops) draw an internal border line on inside edge of the scrollbar
 */
/* #define SB_BORDER */

/*
 * (Hops)  Uncomment to revert to original funky behaviour of
 * of having scroll thumb align on thumb top rather than ptr
 * position in thumb (or center of thumb).
 * Default Behavior becomes alignment to where grab thumb.
 * Only for non XTERM scrollbar
 */
/* #define FUNKY_SCROLL_BEHAVIOUR */

/*------------------------------MENU BAR--------------------------------*/
/*
 * Choose how many of (experimental) menuBars you want to be able to stack at
 * one time.
 *  A value of 1 disables menuBar stacking.
 *  A value of 0 disables menuBar all together.
 *  Note that the amount of memory overhead is the same for any value >= 2.
 */
#define MENUBAR_MAX 8

/*
 * Change the default shadow style
 */
/* #define MENUBAR_SHADOW_IN */

/*
 * Change the default shadow style
 */
#define MENU_SHADOW_IN

/*---------------------------MULTILINGUAL-------------------------------*/
/*
 * Allow run-time selection of Meta (Alt) to set the 8th bit on
 */
#define META8_OPTION

/*---------------------------DISPLAY OPTIONS----------------------------*/
/*
 * Have DISPLAY environment variable & "\E[7n" transmit display with IP number
 */
/* #define DISPLAY_IS_IP */

/*
 * Have "\E[7n" transmit the display name.
 * This has been cited as a potential security hole.
 */
/* #define ENABLE_DISPLAY_ANSWER */

/* 
 * Change what ESC Z transmits instead of the default "\E[?1;2c"
 */
/* #define ESCZ_ANSWER	"\033[?1;2C" */

/*
 * Check the current value of the window-time/icon-name and avoid
 * re-setting it to the same value -- avoids unnecessary window refreshes
 */
#define SMART_WINDOW_TITLE

/*
 * Allow foreground/background colour to be changed with an
 * xterm escape sequence "\E]39;colour^G" -- still experimental
 */
#define XTERM_COLOR_CHANGE

/*
 * Width of the term border
 */
#define BORDERWIDTH	1

/* Add a run-time option to disable pixmap tiling. */
#define DONT_TILE_PIXMAP_OPTION 

/*
 * Default number of lines in the scrollback buffer
 */
#define SAVELINES	512

/*
 * Default line space
 */
#define LINESPACE 0


/* (Hops) Set to choose a number of lines of context between pages 
 *      (rather than a proportion (1/5) of savedlines buffer) 
 *      when paging the savedlines with SHIFT-{Prior,Next} keys.
 */
#define PAGING_CONTEXT_LINES 1 /* */

/*
 * List of default fonts available
 * NFONTS is the number of fonts in the list
 * FONT0_IDX is the default font in the list (starting at 0)
 * Sizes between multi-char fonts sets (MFONT_LIST) and single-char font
 * sets (NFONT_LIST) have been matched up
 */
#ifdef KANJI
# define NFONTS		5
# define FONT0_IDX	2
# define MFONT_LIST	"k14", "jiskan16", "jiskan18", "jiskan24", "jiskan26"
# define NFONT_LIST	"7x14", "8x16", "9x18", "12x24", "13x26"
#else
# ifdef ZH
#  define NFONTS	5
#  define FONT0_IDX	1
#  define MFONT_LIST	"taipei16", "taipeik20", "taipeik24", "taipeik20", \
			"taipei16"
#  define NFONT_LIST	"8x16", "10x20", "12x24", "10x20", "8x16"
# else				/* no Kanji or Big5 support */
#  define NFONTS	5
#  define FONT0_IDX	2
#  undef  MFONT_LIST
#  define NFONT_LIST	"7x14", "6x10", "6x13", "8x13", "9x15"
# endif
#endif

#endif
