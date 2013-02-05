/*--------------------------------*-C-*--------------------------------------*
 * File:      screen.c
 *---------------------------------------------------------------------------*
 * Copyright (C) 2001 (cursor color) Jordan DeLong <fracture at allusion dot net>
 * Copyright (C) 2000 Sasha Vasko <sashav at sprintmail dot com>
 * Copyright (C) 1997,1998 Geoff Wing <gcw at pobox dot com>
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
 *--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*
 * Originally written:
 *    1997,1998 Geoff Wing <mason@primenet.com.au>
 *              - The routine names and calling structure was based upon
 *                previous work by Robert Nation.  Most things have gone
 *                through major change.
 *
 *    Michael Bruun Petersen" <mbp@image.dk>
 *		- tinting fix
 *--------------------------------------------------------------------------*/
/*
 * We handle _all_ screen updates and selections
 */
/*#define DEBUG_SCREEN */

#ifndef lint
static const char rcsid[] = "$Id: screen.c,v 1.14 2006/03/29 22:32:07 sasha Exp $";
#endif

#define INTERN_SCREEN
#define LIBAFTERSTEP_HEADER_INCLUDED

/* Define to enforce 32 bit Atom type on 64 bit systems */
/* #define USE_ATOM32	*/


#include "rxvt.h"		/* NECESSARY */

#include <X11/Xatom.h>
#include <X11/Xmd.h>		/* get the typedef for CARD32 */

static screen_t    screen;


int colors_changed = 0 ; /* to indicate that foreground and background colors has changed*/

/* ------------------------------------------------------------------------- */
#ifdef MULTICHAR_SET
short       multi_byte;		/* set ==> currently using 2 bytes per glyph */
short       lost_multi;		/* set ==> we only got half a glyph */
enum {
    SBYTE, WBYTE
} chstat;

# define RESET_CHSTAT			\
    if (chstat == WBYTE)		\
	chstat = SBYTE, lost_multi = 1
#else
# define RESET_CHSTAT
#endif

typedef enum {
    EUCJ, SJIS,			/* KANJI methods */
    BIG5, CNS			/* BIG5 methods: CNS not implemented */
} ENC_METHOD;

#if defined(KANJI) || defined(ZH)
ENC_METHOD encoding_method;
#endif

/* ------------------------------------------------------------------------- */
#define PROP_SIZE		4096
#define TABSIZE			8	/* default tab size */

#ifdef DEBUG_SCREEN
# define D_SCREEN(x)		fprintf x ; fputc('\n', stderr)
#else
# define D_SCREEN(x)
#endif
#ifdef DEBUG_SELECT
# define D_SELECT(x)		fprintf x ; fputc('\n', stderr)
#else
# define D_SELECT(x)
#endif

/* ------------------------------------------------------------------------- *
 *             GENERAL SCREEN AND SELECTION UPDATE ROUTINES                  *
 * ------------------------------------------------------------------------- */
#define ZERO_SCROLLBACK					\
    if ((Options & Opt_scrollTtyOutput) == Opt_scrollTtyOutput)	\
	TermWin.view_start = 0
#define CHECK_SELECTION(x)				\
    if (selection.op)					\
	selection_check(x)
#define CLEAR_SELECTION					\
    selection.beg.row = selection.beg.col		\
	= selection.end.row = selection.end.col = 0
#define CLEAR_ALL_SELECTION				\
    selection.beg.row = selection.beg.col		\
	= selection.mark.row = selection.mark.col	\
	= selection.end.row = selection.end.col = 0

#define ROW_AND_COL_IS_AFTER(A, B, C, D)				\
    (((A) > (C)) || (((A) == (C)) && ((B) > (D))))
#define ROW_AND_COL_IS_BEFORE(A, B, C, D)				\
    (((A) < (C)) || (((A) == (C)) && ((B) < (D))))
#define ROW_AND_COL_IN_ROW_AFTER(A, B, C, D)				\
    (((A) == (C)) && ((B) > (D)))
#define ROW_AND_COL_IN_ROW_ON_OR_AFTER(A, B, C, D)			\
    (((A) == (C)) && ((B) >= (D)))
#define ROW_AND_COL_IN_ROW_BEFORE(A, B, C, D)				\
    (((A) == (C)) && ((B) < (D)))
#define ROW_AND_COL_IN_ROW_ON_OR_BEFORE(A, B, C, D)			\
    (((A) == (C)) && ((B) <= (D)))

/* these must be row_col_t */
#define ROWCOL_IS_AFTER(X, Y)						\
    ROW_AND_COL_IS_AFTER((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IS_BEFORE(X, Y)						\
    ROW_AND_COL_IS_BEFORE((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_AFTER(X, Y)					\
    ROW_AND_COL_IN_ROW_AFTER((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_BEFORE(X, Y)					\
    ROW_AND_COL_IN_ROW_BEFORE((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_ON_OR_AFTER(X, Y)					\
    ROW_AND_COL_IN_ROW_ON_OR_AFTER((X).row, (X).col, (Y).row, (Y).col)
#define ROWCOL_IN_ROW_ON_OR_BEFORE(X, Y)				\
    ROW_AND_COL_IN_ROW_ON_OR_BEFORE((X).row, (X).col, (Y).row, (Y).col)

/*
 * CLEAR_ROWS : clear <num> rows starting from row <row>
 * CLEAR_CHARS: clear <num> chars starting from pixel position <x,y>
 * ERASE_ROWS : set <num> rows starting from row <row> to the foreground colour
 */

# define drawBuffer	(TermWin.vt)

void CLEAR_ROWS(unsigned row, unsigned num)
{
  unsigned line_h = Height2Pixel(1)-1;
  register unsigned hy = (line_h+1)*num, y = Row2Pixel(row);

    if(!TermWin.bMapped) return ;

    if (y < TermWin_internalBorder+line_h)
    {y = 0; hy += TermWin_internalBorder;}
    if (y + hy > (TermWin.height+TermWin_internalBorder)-line_h)
	hy = (TermWin.height-y)+TermWin_internalBorders;
    XClearArea(Xdisplay, drawBuffer, 0, y, TermWin.width+TermWin_internalBorders, hy, 0);
#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT)
    if (TermWin.tintGC)
    {
        XFillRectangle(Xdisplay, drawBuffer, TermWin.tintGC,
                       0, y, TermWin.width+TermWin_internalBorders, hy);
    }
#endif
}

void CLEAR_CHARS(unsigned x, unsigned y, unsigned num)
{
  unsigned char_w = Width2Pixel(1)-1, line_h = Height2Pixel(1)-1;
  register unsigned hx = (char_w+1)*num ,hy = line_h+1;

    if(!TermWin.bMapped) return ;

    if (x < TermWin_internalBorder+char_w) {x = 0; hx += TermWin_internalBorder;}
    if (y < TermWin_internalBorder+line_h) {y = 0; hy += TermWin_internalBorder ;}

    if (x + hx > (TermWin.width+TermWin_internalBorder) -char_w)
	hx = (TermWin.width+TermWin_internalBorders+1)-x;
    if (y + hy > (TermWin.height+TermWin_internalBorder) - line_h)
	hy = (TermWin.height+TermWin_internalBorders+1)-y;
    XClearArea(Xdisplay, drawBuffer, x, y, hx, hy, 0);
#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT)
    if (TermWin.tintGC)
    {
        XFillRectangle(Xdisplay, drawBuffer, TermWin.tintGC, x, y, hx, hy);
    }
#endif
}


#define ERASE_ROWS(row, num) 						\
    XFillRectangle(Xdisplay, drawBuffer, TermWin.gc,			\
		   Col2Pixel(0), Row2Pixel(row),			\
		   TermWin.width, Height2Pixel(num))


/* ------------------------------------------------------------------------- *
 *                        SCREEN `COMMON' ROUTINES                           *
 * ------------------------------------------------------------------------- */
/* Fill part/all of a line with blanks. */
/* PROTO */
void
blank_line(text_t * et, rend_t * er, int width, rend_t efs)
{
    MEMSET(et, ' ', width);
    for (; width--;)
	*er++ = efs;
}

/* ------------------------------------------------------------------------- */
/* allocate memory for this screen line and blank it appropriately */
/* PROTO */
void
make_screen_mem(text_t ** tp, rend_t ** rp, int row, rend_t rstyle)
{
    tp[row] = MALLOC(sizeof(text_t) * TermWin.bcol);
    rp[row] = MALLOC(sizeof(rend_t) * TermWin.bcol);
    blank_line(tp[row], rp[row], TermWin.bcol, rstyle);
}

/* ------------------------------------------------------------------------- *
 *                          SCREEN INITIALISATION                            *
 * ------------------------------------------------------------------------- */
static int      prev_nrow = -1, prev_bcol = -1;

/* PROTO */
void
scr_reset(void)
{
    int             i, j, k, total_rows, prev_total_rows;
    rend_t	    setrstyle;

    D_SCREEN((stderr, "scr_reset()"));

    TermWin.view_start = 0;
    RESET_CHSTAT;

    if (TermWin.bcol == prev_bcol && TermWin.nrow == prev_nrow)
	return;
#if 0 
#ifdef DEBUG_STRICT
    assert(TermWin.saveLines >= 0);
#else				/* drive with your eyes closed */
    MAX_IT(TermWin.saveLines, 0);
#endif
#endif 

    total_rows = TermWin.nrow + TermWin.saveLines;
    prev_total_rows = prev_nrow + TermWin.saveLines;

    screen.tscroll = 0;
    screen.bscroll = (TermWin.nrow - 1);
/* *INDENT-OFF* */
    if (prev_nrow == -1) {
/*
 * A: first time called so just malloc everything : don't rely on realloc
 *    Note: this is still needed so that all the scrollback lines are NULL
 */
	screen.text	= CALLOC(text_t * , total_rows  );
	screen.tlen	= CALLOC(R_int16_t, total_rows  );
	buf_text	= CALLOC(text_t * , total_rows  );
	buf_tlen	= CALLOC(R_int16_t, total_rows  );
	drawn_text	= CALLOC(text_t * , TermWin.nrow);
	swap.text	= CALLOC(text_t * , TermWin.nrow);
	swap.tlen	= CALLOC(R_int16_t, TermWin.nrow);

	screen.rend = CALLOC(rend_t *, total_rows  );
	buf_rend    = CALLOC(rend_t *, total_rows  );
	drawn_rend  = CALLOC(rend_t *, TermWin.nrow);
	swap.rend   = CALLOC(rend_t *, TermWin.nrow);

	for (i = 0; i < TermWin.nrow; i++) {
	    j = i + TermWin.saveLines;
	    make_screen_mem(screen.text, screen.rend, j, DEFAULT_RSTYLE);
	    make_screen_mem(swap.text, swap.rend, i, DEFAULT_RSTYLE);
	    make_screen_mem(drawn_text, drawn_rend, i, DEFAULT_RSTYLE );
	    screen.tlen[j] = swap.tlen[i] = 0;
	}
	TermWin.nscrolled = 0;	/* no saved lines */
	screen.flags = swap.flags = Screen_DefaultFlags;
	save.cur.row = save.cur.col = 0;
	save.charset = 0;
	save.charset_char = 'B';
	rstyle = save.rstyle = DEFAULT_RSTYLE;
	selection.text = NULL;
	selection.op = SELECTION_CLEAR;
	selection.screen = PRIMARY;
	selection.clicks = 0;
	CLEAR_ALL_SELECTION;
	MEMSET(charsets, 'B', sizeof(charsets));
	current_screen = PRIMARY;
	rvideo = 0;
#ifdef MULTICHAR_SET
	multi_byte = 0;
	lost_multi = 0;
	chstat = SBYTE;
# ifdef KANJI
	encoding_method = EUCJ;
# else
	encoding_method = BIG5;
# endif
#endif

    } else {
/*
 * B1: add or delete rows as appropriate
 */
	setrstyle = DEFAULT_RSTYLE | (rvideo ? RS_RVid : 0);

	if ((int)TermWin.nrow < prev_nrow) {
	/* delete rows */
	    k = min((int)TermWin.nscrolled, prev_nrow - (int)TermWin.nrow);
	    scroll_text(0, prev_nrow - 1, k, 1);
	    for (i = TermWin.nrow; i < prev_nrow; i++) {
		j = i + TermWin.saveLines;
		if (screen.text[j])
		    FREE(screen.text[j]);
		if (screen.rend[j])
		    FREE(screen.rend[j]);
		if (swap.text[i])
		    FREE(swap.text[i]);
		if (swap.rend[i])
		    FREE(swap.rend[i]);
		FREE(drawn_text[i]);
		FREE(drawn_rend[i]);
	    }
	/* we have fewer rows so fix up number of scrolled lines */
	    MIN_IT(screen.cur.row, TermWin.nrow - 1);
	    MIN_IT(swap.cur.row, TermWin.nrow - 1);
	}
	if(TermWin.nrow != prev_nrow) {
	    screen.text = REALLOC(screen.text, total_rows  *sizeof(text_t*));
	    screen.tlen = REALLOC(screen.tlen, total_rows  *sizeof(R_int16_t));
	    buf_text  	= REALLOC(buf_text   , total_rows  *sizeof(text_t*));
	    buf_tlen  	= REALLOC(buf_tlen   , total_rows  *sizeof(R_int16_t));
	    drawn_text  = REALLOC(drawn_text , TermWin.nrow*sizeof(text_t*));
	    swap.text  	= REALLOC(swap.text  , TermWin.nrow*sizeof(text_t*));
	    swap.tlen   = REALLOC(swap.tlen  , total_rows  *sizeof(R_int16_t));

	    screen.rend = REALLOC(screen.rend, total_rows  *sizeof(rend_t*));
	    buf_rend  	= REALLOC(buf_rend   , total_rows  *sizeof(rend_t*));
	    drawn_rend  = REALLOC(drawn_rend , TermWin.nrow*sizeof(rend_t*));
	    swap.rend  	= REALLOC(swap.rend  , TermWin.nrow*sizeof(rend_t*));
	}
	if ((int)TermWin.nrow > prev_nrow) {
	/* add rows */
	    k = min((int)TermWin.nscrolled, (int)TermWin.nrow - prev_nrow);
	    for (i = prev_total_rows; i < total_rows - k; i++) {
		make_screen_mem(screen.text, screen.rend, i, setrstyle);
		screen.tlen[i] = 0;
	    }
	    for (i = total_rows - k; i < total_rows; i++) {
		screen.text[i] = NULL;
		screen.rend[i] = NULL;
		screen.tlen[i] = 0;
	    }
	    for (i = prev_nrow; i < TermWin.nrow; i++) {
		make_screen_mem(swap.text, swap.rend, i, setrstyle);
	        make_screen_mem(drawn_text, drawn_rend, i, setrstyle );
		swap.tlen[i] = 0;
	    }

	    if (k > 0) {
		scroll_text(0, TermWin.nrow - 1, -k, 1);
		screen.cur.row += k;
		TermWin.nscrolled -= k;
		for (i = TermWin.saveLines - TermWin.nscrolled; k--; i--)
		    if (screen.text[i] == NULL) {
			make_screen_mem(screen.text, screen.rend, i,
					setrstyle);
			screen.tlen[i] = 0;
		    }
	    }
#ifdef DEBUG_STRICT
	    assert(screen.cur.row < TermWin.nrow);
	    assert(swap.cur.row < TermWin.nrow);
#else				/* drive with your eyes closed */
	    MIN_IT(screen.cur.row, TermWin.nrow - 1);
	    MIN_IT(swap.cur.row, TermWin.nrow - 1);
#endif
	}

/* B2: resize columns */
	if (TermWin.bcol != prev_bcol) {
	    for (i = 0; i < total_rows; i++) {
		if (screen.text[i]) {
		    screen.text[i] = REALLOC(screen.text[i],
					      TermWin.bcol * sizeof(text_t));
		    screen.rend[i] = REALLOC(screen.rend[i],
					      TermWin.bcol * sizeof(rend_t));
		    MIN_IT(screen.tlen[i], TermWin.bcol);
		    if (TermWin.bcol > prev_bcol)
			blank_line(&(screen.text[i][prev_bcol]),
				   &(screen.rend[i][prev_bcol]),
				   TermWin.bcol - prev_bcol, setrstyle);
		}
	    }
	    for (i = 0; i < TermWin.nrow; i++) {
		drawn_text[i] = REALLOC(drawn_text[i],
					 TermWin.bcol * sizeof(text_t));
		drawn_rend[i] = REALLOC(drawn_rend[i],
					 TermWin.bcol * sizeof(rend_t));
		if (swap.text[i]) {
		    swap.text[i] = REALLOC(swap.text[i],
					    TermWin.bcol * sizeof(text_t));
		    swap.rend[i] = REALLOC(swap.rend[i],
					    TermWin.bcol * sizeof(rend_t));
		    MIN_IT(swap.tlen[i], TermWin.bcol);
		    if (TermWin.bcol > prev_bcol)
			blank_line(&(swap.text[i][prev_bcol]),
				   &(swap.rend[i][prev_bcol]),
				   TermWin.bcol - prev_bcol, setrstyle);
		}
		if (TermWin.bcol > prev_bcol)
		    blank_line(&(drawn_text[i][prev_bcol]),
			       &(drawn_rend[i][prev_bcol]),
			       TermWin.bcol - prev_bcol, setrstyle);
	    }
	    MIN_IT(screen.cur.col, TermWin.bcol - 1);
	    MIN_IT(swap.cur.col, TermWin.bcol - 1);
	}
	if (tabs)
	    FREE(tabs);
    }
/* *INDENT-ON* */

    tabs = MALLOC(TermWin.bcol * sizeof(char));

    for (i = 0; i < TermWin.bcol; i++)
	tabs[i] = (i % TABSIZE == 0) ? 1 : 0;

    prev_nrow = TermWin.nrow;
    prev_bcol = TermWin.bcol;

    tt_resize();
}

/* ------------------------------------------------------------------------- */
/*
 * Free everything.  That way malloc debugging can find leakage.
 */
/* PROTO */
void
scr_release(void)
{
    int             i, total_rows;

    total_rows = TermWin.nrow + TermWin.saveLines;
    for (i = 0; i < total_rows; i++) {
	if (screen.text[i]) {	/* then so is screen.rend[i] */
	    FREE(screen.text[i]);
	    FREE(screen.rend[i]);
	}
    }
    for (i = 0; i < TermWin.nrow; i++) {
	FREE(drawn_text[i]);
	FREE(drawn_rend[i]);
	FREE(swap.text[i]);
	FREE(swap.rend[i]);
    }
    FREE(screen.text);
    FREE(screen.tlen);
    FREE(screen.rend);
    FREE(drawn_text);
    FREE(drawn_rend);
    FREE(swap.text);
    FREE(swap.tlen);
    FREE(swap.rend);
    FREE(buf_text);
    FREE(buf_tlen);
    FREE(buf_rend);
    FREE(tabs);

/* NULL these so if anything tries to use them, we'll know about it */
    screen.text = drawn_text = swap.text = NULL;
    screen.rend = drawn_rend = swap.rend = NULL;
    screen.tlen = swap.tlen = buf_tlen = NULL;
    buf_text = NULL;
    buf_rend = NULL;
    tabs = NULL;
}

/* ------------------------------------------------------------------------- */
/* PROTO */
void
scr_poweron(void)
{
    D_SCREEN((stderr, "scr_poweron()"));

    MEMSET(charsets, 'B', sizeof(charsets));
    rvideo = 0;
    swap.tscroll = 0;
    swap.bscroll = TermWin.nrow - 1;
    screen.cur.row = screen.cur.col = swap.cur.row = swap.cur.col = 0;
    screen.charset = swap.charset = 0;
    screen.flags = swap.flags = Screen_DefaultFlags;

    scr_cursor(SAVE);

    scr_release();
    prev_nrow = -1;
    prev_bcol = -1;
    scr_reset();

    scr_clear();
    scr_refresh(SLOW_REFRESH);
    Gr_reset();
}

/* ------------------------------------------------------------------------- *
 *                         PROCESS SCREEN COMMANDS                           *
 * ------------------------------------------------------------------------- */
/*
 * Save and Restore cursor
 * XTERM_SEQ: Save cursor   : ESC 7
 * XTERM_SEQ: Restore cursor: ESC 8
 */
/* PROTO */
void
scr_cursor(int mode)
{
    D_SCREEN((stderr, "scr_cursor(%c)", mode));

    switch (mode) {
    case SAVE:
	save.cur.row = screen.cur.row;
	save.cur.col = screen.cur.col;
	save.rstyle = rstyle;
	save.charset = screen.charset;
	save.charset_char = charsets[screen.charset];
	break;
    case RESTORE:
	screen.cur.row = save.cur.row;
	screen.cur.col = save.cur.col;
	rstyle = save.rstyle;
	screen.charset = save.charset;
	charsets[screen.charset] = save.charset_char;
	set_font_style();
	break;
    }
/* boundary check in case screen size changed between SAVE and RESTORE */
    MIN_IT(screen.cur.row, TermWin.nrow - 1);
    MIN_IT(screen.cur.col, TermWin.bcol - 1);
#ifdef DEBUG_STRICT
    assert(screen.cur.row >= 0);
    assert(screen.cur.col >= 0);
#else				/* drive with your eyes closed */
    MAX_IT(screen.cur.row, 0);
    MAX_IT(screen.cur.col, 0);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Swap between primary and secondary screens
 * XTERM_SEQ: Primary screen  : ESC [ ? 4 7 h
 * XTERM_SEQ: Secondary screen: ESC [ ? 4 7 l
 */
/* PROTO */
int
scr_change_screen(int scrn)
{
    int             i, offset, tmp;
    text_t         *t0;
    rend_t         *r0;
    R_int16_t	    l0;

    D_SCREEN((stderr, "scr_change_screen(%d)", scrn));

    TermWin.view_start = 0;
    RESET_CHSTAT;

    if (current_screen == scrn)
	return current_screen;

    CHECK_SELECTION(2);		/* check for boundary cross */

    SWAP_IT(current_screen, scrn, tmp);
#if NSCREENS
    offset = TermWin.saveLines;
    for (i = TermWin.nrow; i--;) {
	SWAP_IT(screen.text[i + offset], swap.text[i], t0);
	SWAP_IT(screen.tlen[i + offset], swap.tlen[i], l0);
	SWAP_IT(screen.rend[i + offset], swap.rend[i], r0);
    }
    SWAP_IT(screen.cur.row, swap.cur.row, l0);
    SWAP_IT(screen.cur.col, swap.cur.col, l0);
# ifdef DEBUG_STRICT
    assert(screen.cur.row >= 0);
    assert(screen.cur.col >= 0);
    assert(screen.cur.row < TermWin.nrow);
    assert(screen.cur.col < TermWin.bcol);
# else				/* drive with your eyes closed */
    MAX_IT(screen.cur.row, 0);
    MAX_IT(screen.cur.col, 0);
    MIN_IT(screen.cur.row, TermWin.nrow - 1);
    MIN_IT(screen.cur.col, TermWin.bcol - 1);
# endif
    SWAP_IT(screen.charset, swap.charset, l0);
    SWAP_IT(screen.flags, swap.flags, tmp);
    screen.flags |= Screen_VisibleCursor;
    swap.flags |= Screen_VisibleCursor;

    if (Gr_Displayed()) {
	Gr_scroll(0);
	Gr_ChangeScreen();
    }
#else
# ifndef DONT_SCROLL_ME
    if (Gr_Displayed())
	Gr_ClearScreen();
    if (current_screen == PRIMARY) {
	if (!Gr_Displayed())
	    scroll_text(0, (TermWin.nrow - 1), TermWin.nrow, 0);
	for (i = TermWin.saveLines; i < TermWin.nrow + TermWin.saveLines; i++)
	    if (screen.text[i] == NULL) {
		make_screen_mem(screen.text, screen.rend, i, DEFAULT_RSTYLE);
		screen.tlen[i] = 0;
	    }
    }
# endif
#endif
    return scrn;
}

/* ------------------------------------------------------------------------- */
/*
 * Change the colour for following text
 */
/* PROTO */
void
scr_color(unsigned int color, unsigned int Intensity)
{
    if (color == restoreFG)
	color = Color_fg;
    else if (color == restoreBG)
	color = Color_bg;
    else {
	if (Xdepth <= 2) {	/* Monochrome - ignore colour changes */
	    switch (Intensity) {
	    case RS_Bold:
		color = Color_fg;
		break;
	    case RS_Blink:
		color = Color_bg;
		break;
	    }
	} else {
#ifndef NO_BRIGHTCOLOR
	    if ((rstyle & Intensity) && color >= minCOLOR && color <= maxCOLOR)
		color += (minBrightCOLOR - minCOLOR);
	    else if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
		if (rstyle & Intensity)
		    return;
		color -= (minBrightCOLOR - minCOLOR);
	    }
#endif
	}
    }
    switch (Intensity) {
    case RS_Bold:
	rstyle = SET_FGCOLOR(rstyle, color);
	break;
    case RS_Blink:
	rstyle = SET_BGCOLOR(rstyle, color);
	break;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Change the rendition style for following text
 */
/* PROTO */
void
scr_rendition(int set, int style)
{
    unsigned int    color;
    rend_t          font_attr;

    if (set) {
/* A: Set style */
	rstyle |= style;
	switch (style) {
	case RS_RVid:
	    if (rvideo)
		rstyle &= ~RS_RVid;
	    break;
#ifndef NO_BRIGHTCOLOR
	case RS_Bold:
	    color = GET_FGCOLOR(rstyle);
	    scr_color((color == Color_fg ? GET_FGCOLOR(colorfgbg) : color),
		      RS_Bold);
	    break;
	case RS_Blink:
	    color = GET_BGCOLOR(rstyle);
	    scr_color((color == Color_bg ? GET_BGCOLOR(colorfgbg) : color),
		      RS_Blink);
	    break;
#endif
	}
    } else {
/* B: Unset style */
	font_attr = rstyle & RS_fontMask;
	rstyle &= ~style;

	switch (style) {
	case ~RS_None:		/* default fg/bg colours */
	    rstyle = DEFAULT_RSTYLE | font_attr;
	/* FALLTHROUGH */
	case RS_RVid:
	    if (rvideo)
		rstyle |= RS_RVid;
	    break;
#ifndef NO_BRIGHTCOLOR
	case RS_Bold:
	    color = GET_FGCOLOR(rstyle);
	    if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
		scr_color(color, RS_Bold);
		if ((rstyle & RS_fgMask) == (colorfgbg & RS_fgMask))
		    scr_color(restoreFG, RS_Bold);
	    }
	    break;
	case RS_Blink:
	    color = GET_BGCOLOR(rstyle);
	    if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
		scr_color(color, RS_Blink);
		if ((rstyle & RS_bgMask) == (colorfgbg & RS_bgMask))
		    scr_color(restoreBG, RS_Blink);
	    }
	    break;
#endif
	}
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Scroll text between <row1> and <row2> inclusive, by <count> lines
 * count positive ==> scroll up
 * count negative ==> scroll down
 * spec == 0 for normal routines
 */
/* PROTO */
int
scroll_text(int row1, int row2, int count, int spec)
{
    int             i, j;

    D_SCREEN((stderr, "scroll_text(%d,%d,%d,%d): %s", row1, row2, count, spec, (current_screen == PRIMARY) ? "Primary" : "Secondary"));

    if (count == 0 || (row1 > row2))
	return 0;

    if ((count > 0) && (row1 == 0) && (current_screen == PRIMARY)) {
	if (TermWin.saveLines - TermWin.nscrolled < count)
		TermWin.nscrolled += TermWin.saveLines - TermWin.nscrolled;
	else
		TermWin.nscrolled += count;
	MIN_IT(TermWin.nscrolled, TermWin.saveLines);
    } else if (!spec)
	row1 += TermWin.saveLines;
    row2 += TermWin.saveLines;

    if (selection.op && current_screen == selection.screen) {
	i = selection.beg.row + TermWin.saveLines;
	j = selection.end.row + TermWin.saveLines;
	if ((i < row1 && j > row1)
	    || (i < row2 && j > row2)
	    || (i - count < row1 && i >= row1)
	    || (i - count > row2 && i <= row2)
	    || (j - count < row1 && j >= row1)
	    || (j - count > row2 && j <= row2)) {
	    CLEAR_ALL_SELECTION;
	    selection.op = SELECTION_CLEAR;	/* XXX: too aggressive? */
	} else if (j >= row1 && j <= row2) {
	/* move selected region too */
	    selection.beg.row -= count;
	    selection.end.row -= count;
	    selection.mark.row -= count;
	}
    }
    CHECK_SELECTION(0);		/* _after_ TermWin.nscrolled update */

    if (count > 0) {
/* A: scroll up */

	MIN_IT(count, row2 - row1 + 1);
/* A1: Copy lines that will get clobbered by the rotation */
	for (i = 0, j = row1; i < count; i++, j++) {
	    buf_text[i] = screen.text[j];
	    buf_tlen[i] = screen.tlen[j];
	    buf_rend[i] = screen.rend[j];
	}
/* A2: Rotate lines */
	for (j = row1; (j + count) <= row2; j++) {
	    screen.text[j] = screen.text[j + count];
	    screen.tlen[j] = screen.tlen[j + count];
	    screen.rend[j] = screen.rend[j + count];
	}
/* A3: Resurrect lines */
	for (i = 0; i < count; i++, j++) {
	    screen.text[j] = buf_text[i];
	    screen.tlen[j] = buf_tlen[i];
	    screen.rend[j] = buf_rend[i];
	}
    } else if (count < 0) {
/* B: scroll down */

	count = min(-count, row2 - row1 + 1);
/* B1: Copy lines that will get clobbered by the rotation */
	for (i = 0, j = row2; i < count; i++, j--) {
	    buf_text[i] = screen.text[j];
	    buf_tlen[i] = screen.tlen[j];
	    buf_rend[i] = screen.rend[j];
	}
/* B2: Rotate lines */
	for (j = row2; (j - count) >= row1; j--) {
	    screen.text[j] = screen.text[j - count];
	    screen.tlen[j] = screen.tlen[j - count];
	    screen.rend[j] = screen.rend[j - count];
	}
/* B3: Resurrect lines */
	for (i = 0, j = row1; i < count; i++, j++) {
	    screen.text[j] = buf_text[i];
	    screen.tlen[j] = buf_tlen[i];
	    screen.rend[j] = buf_rend[i];
	}
	count = -count;
    }
    if (Gr_Displayed())
	Gr_scroll(count);
    return count;
}

/* ------------------------------------------------------------------------- */
/*
 * Add text given in <str> of length <len> to screen struct
 */
/* PROTO */
void
scr_add_lines(const unsigned char *str, int nlines, int len)
{
    char            c;
    int             i, j, row, last_col, wherecursor, wrotespecial;
    text_t         *stp;
    rend_t         *srp;

    if (len <= 0)		/* sanity */
	return;

    last_col = TermWin.bcol;

    D_SCREEN((stderr, "scr_add_lines(*,%d,%d)", nlines, len));
    ZERO_SCROLLBACK;
    if (nlines > 0) 
	{
		nlines += (screen.cur.row - screen.bscroll);
		if ((nlines > 0) && (screen.tscroll == 0)&& (screen.bscroll == (TermWin.nrow - 1))) 
		{/* _at least_ this many lines need to be scrolled */
	    	scroll_text(screen.tscroll, screen.bscroll, nlines, 0);
	    	for (i = nlines, j = screen.bscroll + TermWin.saveLines; i--; j--) 
			{
				if (screen.text[j] == NULL)
		    		make_screen_mem(screen.text, screen.rend, j, rstyle);
				else
		    		blank_line(screen.text[j], screen.rend[j], TermWin.bcol, rstyle);
				screen.tlen[j] = 0;
	    	}
	    	screen.cur.row -= nlines;
		}
    }
#ifdef DEBUG_STRICT
    assert(screen.cur.col < last_col);
    assert(screen.cur.row < TermWin.nrow);
    assert(screen.cur.row >= -TermWin.nscrolled);
#else				/* drive with your eyes closed */
    MIN_IT(screen.cur.col, last_col - 1);
    MIN_IT(screen.cur.row, TermWin.nrow - 1);
    MAX_IT(screen.cur.row, -TermWin.nscrolled);
#endif
    row = screen.cur.row + TermWin.saveLines;

    if (ROW_AND_COL_IS_BEFORE(screen.cur.row, screen.cur.col,
		    	      selection.beg.row, selection.beg.col))
		wherecursor = -1;
    else if (ROW_AND_COL_IS_BEFORE(screen.cur.row, screen.cur.col,
    		                   selection.end.row, selection.end.col))
		wherecursor = 0;
    else
		wherecursor = 1;

    stp = screen.text[row];
    srp = screen.rend[row];

#ifdef MULTICHAR_SET
    if (lost_multi && screen.cur.col > 0
		&& ((srp[screen.cur.col - 1] & RS_multiMask) == RS_multi1)
		&& *str != '\n' && *str != '\r' && *str != '\t')
		chstat = WBYTE;
#endif

    for (wrotespecial = len, i = 0; i < len;) 
	{
		c = str[i++];
#ifdef MULTICHAR_SET
		if (chstat == WBYTE) 
		{
	    	rstyle |= RS_multiMask;	/* multibyte 2nd byte */
	    	chstat = SBYTE;
	    	if (encoding_method == EUCJ)
				c |= 0x80;	/* maybe overkill, but makes it selectable */
		} else if (chstat == SBYTE) 
		{
	    	if (multi_byte || (c & 0x80)) 
			{	/* multibyte 1st byte */
				rstyle &= ~RS_multiMask;
				rstyle |= RS_multi1;
				chstat = WBYTE;
				if (encoding_method == EUCJ)
		    		c |= 0x80;	/* maybe overkill, but makes it selectable */
	    	} else
#endif				/* MULTICHAR_SET */
				switch (c) 
				{
					case 127:
		    			wrotespecial--;
		    			continue;	/* yummmm..... */
					case '\t':
		    			wrotespecial--;
		    			scr_tab(1);
		    			continue;
					case '\n':
		    			wrotespecial--;
		    			if (screen.tlen[row] != -1)	/* XXX: think about this */
							MAX_IT(screen.tlen[row], screen.cur.col);
		    			screen.flags &= ~Screen_WrapNext;
		    			if (screen.cur.row == screen.bscroll) 
						{
							scroll_text(screen.tscroll, screen.bscroll, 1, 0);
							j = screen.bscroll + TermWin.saveLines;
							if (screen.text[j] == NULL)
			    				make_screen_mem(screen.text, screen.rend, j, rstyle);
							else
			    				blank_line(screen.text[j], screen.rend[j], TermWin.bcol, rstyle);
							screen.tlen[j] = 0;
		    			} else if (screen.cur.row < (TermWin.nrow - 1))
							row = (++screen.cur.row) + TermWin.saveLines;
		    			stp = screen.text[row];	/* _must_ refresh */
		    			srp = screen.rend[row];	/* _must_ refresh */
		    			continue;
					case '\r':
		    			wrotespecial--;
		    			if (screen.tlen[row] != -1)	/* XXX: think about this */
							MAX_IT(screen.tlen[row], screen.cur.col);
		    			screen.flags &= ~Screen_WrapNext;
		    			screen.cur.col = 0;
		    			continue;
					default:
#ifdef MULTICHAR_SET
					    rstyle &= ~RS_multiMask;
#endif
		    			break;
				}
#ifdef MULTICHAR_SET
		}
#endif

	if (screen.flags & Screen_WrapNext) {
	    screen.tlen[row] = -1;
	    if (screen.cur.row == screen.bscroll) {
		scroll_text(screen.tscroll, screen.bscroll, 1, 0);
		j = screen.bscroll + TermWin.saveLines;
		if (screen.text[j] == NULL)
		    make_screen_mem(screen.text, screen.rend, j, rstyle);
		else
		    blank_line(screen.text[j], screen.rend[j], TermWin.bcol,
			       rstyle);
		screen.tlen[j] = 0;
	    } else if (screen.cur.row < (TermWin.nrow - 1))
		row = (++screen.cur.row) + TermWin.saveLines;
	    stp = screen.text[row];	/* _must_ refresh */
	    srp = screen.rend[row];	/* _must_ refresh */
	    screen.cur.col = 0;
	    screen.flags &= ~Screen_WrapNext;
	}
	if (screen.flags & Screen_Insert)
	    scr_insdel_chars(1, INSERT);
	stp[screen.cur.col] = c;
	srp[screen.cur.col] = rstyle;
	/*
         * TODO: This might end up wrapping the tone sign in Thai
         */
	if (screen.cur.col < (last_col - 1))
	    screen.cur.col++;
	else {
	    screen.tlen[row] = last_col;
	    if (screen.flags & Screen_Autowrap)
		screen.flags |= Screen_WrapNext;
	    else
		screen.flags &= ~Screen_WrapNext;
	}
    }
    if (screen.tlen[row] != -1)	/* XXX: think about this */
	MAX_IT(screen.tlen[row], screen.cur.col);

/*
 * If we wrote anywhere in the selected area, kill the selection
 * XXX: should we kill the mark too?  Possibly, but maybe that
 *      should be a similar check.
 */
    if (ROW_AND_COL_IS_BEFORE(screen.cur.row, screen.cur.col,
		    	      selection.beg.row, selection.beg.col))
	i = -1;
    else if (ROW_AND_COL_IS_BEFORE(screen.cur.row, screen.cur.col,
    		                   selection.end.row, selection.end.col))
	i = 0;
    else
	i = 1;
    if (selection.op && current_screen == selection.screen
	&& wrotespecial != 0 && (i != wherecursor || i == 0))
	CLEAR_SELECTION;

#ifdef DEBUG_STRICT
    assert(screen.cur.row >= 0);
#else				/* drive with your eyes closed */
    MAX_IT(screen.cur.row, 0);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Process Backspace.  Move back the cursor back a position, wrap if have to
 * XTERM_SEQ: CTRL-H
 */
/* PROTO */
void
scr_backspace(void)
{
    RESET_CHSTAT;
    if (screen.cur.col == 0 && screen.cur.row > 0) {
	screen.cur.col = TermWin.bcol - 1;
	screen.cur.row--;
    } else if (screen.flags & Screen_WrapNext) {
	screen.flags &= ~Screen_WrapNext;
    } else
	scr_gotorc(0, -1, RELATIVE);
}

/* ------------------------------------------------------------------------- */
/*
 * Process Horizontal Tab
 * count: +ve = forward; -ve = backwards
 * XTERM_SEQ: CTRL-I
 */
/* PROTO */
void
scr_tab(int count)
{
    int             i, x;

    RESET_CHSTAT;
    x = screen.cur.col;
    if (count == 0)
	return;
    else if (count > 0) {
	for (i = x + 1; i < TermWin.bcol; i++) {
	    if (tabs[i]) {
		x = i;
		if (!--count)
		    break;
	    }
	}
    } else if (count < 0) {
	for (i = x - 1; i >= 0; i--) {
	    if (tabs[i]) {
		x = i;
		if (!++count)
		    break;
	    }
	}
    }
    if (x != screen.cur.col)
	scr_gotorc(0, x, R_RELATIVE);
}

/* ------------------------------------------------------------------------- */
/*
 * Goto Row/Column
 */
/* PROTO */
void
scr_gotorc(int row, int col, int relative)
{
    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (Gr_Displayed())
	Gr_scroll(0);

    D_SCREEN((stderr, "scr_gotorc(r:%d,c:%d,%d): from (r:%d,c:%d)", row, col, relative, screen.cur.row, screen.cur.col));

    screen.cur.col = ((relative & C_RELATIVE) ? (screen.cur.col + col) : col);
    MAX_IT(screen.cur.col, 0);
    MIN_IT(screen.cur.col, TermWin.bcol - 1);

    if (screen.flags & Screen_WrapNext) {
	screen.flags &= ~Screen_WrapNext;
    }
    if (relative & R_RELATIVE) {
	if (row > 0) {
	    if (screen.cur.row <= screen.bscroll
		&& (screen.cur.row + row) > screen.bscroll)
		screen.cur.row = screen.bscroll;
	    else
		screen.cur.row += row;
	} else if (row < 0) {
	    if (screen.cur.row >= screen.tscroll
		&& (screen.cur.row + row) < screen.tscroll)
		screen.cur.row = screen.tscroll;
	    else
		screen.cur.row += row;
	}
    } else {
	if (screen.flags & Screen_Relative) {	/* relative origin mode */
	    screen.cur.row = row + screen.tscroll;
	    MIN_IT(screen.cur.row, screen.bscroll);
	} else
	    screen.cur.row = row;
    }
    MAX_IT(screen.cur.row, 0);
    MIN_IT(screen.cur.row, TermWin.nrow - 1);
}

/* ------------------------------------------------------------------------- */
/*
 * direction  should be UP or DN
 */
/* PROTO */
void
scr_index(int direction)
{
    int             dirn;

    dirn = ((direction == UP) ? 1 : -1);
    D_SCREEN((stderr, "scr_index(%d)", dirn));

    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (Gr_Displayed())
	Gr_scroll(0);

    if (screen.flags & Screen_WrapNext) {
	screen.flags &= ~Screen_WrapNext;
    }
    if ((screen.cur.row == screen.bscroll && direction == UP)
	|| (screen.cur.row == screen.tscroll && direction == DN)) {
	scroll_text(screen.tscroll, screen.bscroll, dirn, 0);
	if (direction == UP)
	    dirn = screen.bscroll + TermWin.saveLines;
	else
	    dirn = screen.tscroll + TermWin.saveLines;
	if (screen.text[dirn] == NULL)	/* then so is screen.rend[dirn] */
	    make_screen_mem(screen.text, screen.rend, dirn, rstyle);
	else
	    blank_line(screen.text[dirn], screen.rend[dirn], TermWin.bcol,
		       rstyle);
	screen.tlen[dirn] = 0;
    } else
	screen.cur.row += dirn;
    MAX_IT(screen.cur.row, 0);
    MIN_IT(screen.cur.row, TermWin.nrow - 1);
    CHECK_SELECTION(0);
}

/* ------------------------------------------------------------------------- */
/*
 * Erase part or whole of a line
 * XTERM_SEQ: Clear line to right: ESC [ 0 K
 * XTERM_SEQ: Clear line to left : ESC [ 1 K
 * XTERM_SEQ: Clear whole line   : ESC [ 2 K
 */
/* PROTO */
void
scr_erase_line(int mode)
{
    int             row, col, num;

    D_SCREEN((stderr, "scr_erase_line(%d) at screen row: %d", mode, screen.cur.row));
    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (Gr_Displayed())
	Gr_scroll(0);
    CHECK_SELECTION(1);

    if (screen.flags & Screen_WrapNext)
	screen.flags &= ~Screen_WrapNext;

    row = TermWin.saveLines + screen.cur.row;
    switch (mode) {
    case 0:			/* erase to end of line */
	col = screen.cur.col;
	num = TermWin.bcol - col;
	MIN_IT(screen.tlen[row], col);
	if (ROWCOL_IN_ROW_ON_OR_AFTER(selection.beg, screen.cur)
	    || ROWCOL_IN_ROW_ON_OR_AFTER(selection.end, screen.cur))
	    CLEAR_SELECTION;
	break;
    case 1:			/* erase to beginning of line */
	col = 0;
	num = screen.cur.col + 1;
	if (ROWCOL_IN_ROW_ON_OR_BEFORE(selection.beg, screen.cur)
	    || ROWCOL_IN_ROW_ON_OR_BEFORE(selection.end, screen.cur))
	    CLEAR_SELECTION;
	break;
    case 2:			/* erase whole line */
	col = 0;
	num = TermWin.bcol;
	screen.tlen[row] = 0;
	if (selection.beg.row <= screen.cur.row
	    || selection.end.row >= screen.cur.row)
	    CLEAR_SELECTION;
	break;
    default:
	return;
    }
    blank_line(&(screen.text[row][col]), &(screen.rend[row][col]), num,
	       rstyle & ~RS_Uline);
}

/* ------------------------------------------------------------------------- */
/*
 * Erase part of whole of the screen
 * XTERM_SEQ: Clear screen after cursor : ESC [ 0 J
 * XTERM_SEQ: Clear screen before cursor: ESC [ 1 J
 * XTERM_SEQ: Clear whole screen        : ESC [ 2 J
 */
/* PROTO */
void
scr_erase_screen(int mode)
{
    int             row, num, row_offset;
    rend_t          ren;
    long            gcmask;
    XGCValues       gcvalue;

    D_SCREEN((stderr, "scr_erase_screen(%d) at screen row: %d", mode, screen.cur.row));
    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    row_offset = TermWin.saveLines;

    switch (mode) {
    case 0:			/* erase to end of screen */
	CHECK_SELECTION(1);
	scr_erase_line(0);
	row = screen.cur.row + 1;	/* possible OOB */
	num = TermWin.nrow - row;
	break;
    case 1:			/* erase to beginning of screen */
	CHECK_SELECTION(3);
	scr_erase_line(1);
	row = 0;		/* possible OOB */
	num = screen.cur.row;
	break;
    case 2:			/* erase whole screen */
	CHECK_SELECTION(3);
	Gr_ClearScreen();
	row = 0;
	num = TermWin.nrow;
	break;
    default:
	return;
    }
    if (selection.op && current_screen == selection.screen
	&& ((selection.beg.row >= row
	     && selection.beg.row <= row + num)
	    || (selection.end.row >= row
		&& selection.end.row <= row + num)))
	CLEAR_SELECTION;
    if (row >= 0 && row < TermWin.nrow) {	/* check OOB */
	MIN_IT(num, (TermWin.nrow - row));
	if (rstyle & (RS_RVid | RS_Uline))
	    ren = (rend_t)~RS_None;
	else if (GET_BGCOLOR(rstyle) == Color_bg) {
	    ren = DEFAULT_RSTYLE;
	    CLEAR_ROWS(row, num);
	} else {
	    ren = (rstyle & (RS_fgMask | RS_bgMask));
	    gcvalue.foreground = PixColors[GET_BGCOLOR(ren)];
	    gcmask = GCForeground;
	    XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
	    ERASE_ROWS(row, num);
	    gcvalue.foreground = PixColors[Color_fg];
	    XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
	}
	for (; num--; row++) {
	    blank_line(screen.text[row + row_offset],
		       screen.rend[row + row_offset], TermWin.bcol,
		       rstyle & ~RS_Uline);
	    screen.tlen[row + row_offset] = 0;
	    blank_line(drawn_text[row], drawn_rend[row], TermWin.bcol, ren);
	}
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Fill the screen with `E's
 * XTERM_SEQ: Screen Alignment Test: ESC # 8
 */
/* PROTO */
void
scr_E(void)
{
    int             i, j;
    text_t         *t;
    rend_t         *r, fs;

    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    CHECK_SELECTION(3);

    fs = rstyle;
    for (i = TermWin.saveLines; i < TermWin.nrow + TermWin.saveLines; i++) {
	t = screen.text[i];
	r = screen.rend[i];
	for (j = 0; j < TermWin.bcol; j++) {
	    *t++ = 'E';
	    *r++ = fs;
	}
	screen.tlen[i] = TermWin.bcol;	/* make the `E's selectable */
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Insert/Delete <count> lines
 */
/* PROTO */
void
scr_insdel_lines(int count, int insdel)
{
    int             end;

    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (Gr_Displayed())
	Gr_scroll(0);
    CHECK_SELECTION(1);

    if (screen.cur.row > screen.bscroll)
	return;

    end = screen.bscroll - screen.cur.row + 1;
    if (count > end) {
	if (insdel == DELETE)
	    return;
	else if (insdel == INSERT)
	    count = end;
    }
    if (screen.flags & Screen_WrapNext) {
	screen.flags &= ~Screen_WrapNext;
    }
    scroll_text(screen.cur.row, screen.bscroll, insdel * count, 0);

/* fill the inserted or new lines with rstyle. TODO: correct for delete? */
    if (insdel == DELETE) {
	end = screen.bscroll + TermWin.saveLines;
    } else if (insdel == INSERT) {
	end = screen.cur.row + count - 1 + TermWin.saveLines;
    }
    for (; count--; end--) {
	if (screen.text[end] == NULL)	/* then so is screen.rend[end] */
	    make_screen_mem(screen.text, screen.rend, end, rstyle);
	else
	    blank_line(screen.text[end], screen.rend[end], TermWin.bcol,
		       rstyle);
	screen.tlen[end] = 0;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Insert/Delete <count> characters from the current position
 */
/* PROTO */
void
scr_insdel_chars(int count, int insdel)
{
    int             col, row;
    rend_t	    tr;

    ZERO_SCROLLBACK;
    RESET_CHSTAT;
    if (Gr_Displayed())
	Gr_scroll(0);

    if (count <= 0)
	return;
    CHECK_SELECTION(1);
    MIN_IT(count, (TermWin.bcol - screen.cur.col));

    row = screen.cur.row + TermWin.saveLines;
    screen.flags &= ~Screen_WrapNext;

    switch (insdel) {
    case INSERT:
	for (col = TermWin.bcol - 1; (col - count) >= screen.cur.col; col--) {
	    screen.text[row][col] = screen.text[row][col - count];
	    screen.rend[row][col] = screen.rend[row][col - count];
	}
	if (screen.tlen[row] != -1) {
	    screen.tlen[row] += count;
	    MIN_IT(screen.tlen[row], TermWin.bcol);
	}
	if (selection.op && current_screen == selection.screen
	    && ROWCOL_IN_ROW_ON_OR_AFTER(selection.beg, screen.cur)) {
	    if (selection.end.row != screen.cur.row
		|| (selection.end.col + count >= TermWin.bcol))
		CLEAR_SELECTION;
	    else {		/* shift selection */
		selection.beg.col += count;
		selection.mark.col += count;	/* XXX: yes? */
		selection.end.col += count;
	    }
	}
	blank_line(&(screen.text[row][screen.cur.col]),
		   &(screen.rend[row][screen.cur.col]),
		   count, rstyle);
	break;
    case ERASE:
	screen.cur.col += count;	/* don't worry if > TermWin.bcol */
	CHECK_SELECTION(1);
	screen.cur.col -= count;
	blank_line(&(screen.text[row][screen.cur.col]),
		   &(screen.rend[row][screen.cur.col]),
		   count, rstyle);
	break;
    case DELETE:
	tr = screen.rend[row][TermWin.bcol - 1]
	     & (RS_fgMask | RS_bgMask | RS_baseattrMask);
	for (col = screen.cur.col; (col + count) < TermWin.bcol; col++) {
	    screen.text[row][col] = screen.text[row][col + count];
	    screen.rend[row][col] = screen.rend[row][col + count];
	}
	blank_line(&(screen.text[row][TermWin.bcol - count]),
		   &(screen.rend[row][TermWin.bcol - count]),
		   count, tr);
	if (screen.tlen[row] == -1)	/* break line continuation */
	    screen.tlen[row] = TermWin.bcol;
	screen.tlen[row] -= count;
	MAX_IT(screen.tlen[row], 0);
	if (selection.op && current_screen == selection.screen
	    && ROWCOL_IN_ROW_ON_OR_AFTER(selection.beg, screen.cur)) {
	    if (selection.end.row != screen.cur.row
		|| (screen.cur.col >= selection.beg.col - count)
		|| selection.end.col >= TermWin.bcol)
		CLEAR_SELECTION;
	    else {
		/* shift selection */
		selection.beg.col -= count;
		selection.mark.col -= count;	/* XXX: yes? */
		selection.end.col -= count;
	    }
	}
	break;
    }
#ifdef MULTICHAR_SET
    if ((screen.rend[row][0] & RS_multiMask) == RS_multi2) {
	screen.rend[row][0] &= ~RS_multiMask;
	screen.text[row][0] = ' ';
    }
    if ((screen.rend[row][TermWin.bcol - 1] & RS_multiMask) == RS_multi1) {
	screen.rend[row][TermWin.bcol - 1] &= ~RS_multiMask;
	screen.text[row][TermWin.bcol - 1] = ' ';
    }
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Set the scrolling region
 * XTERM_SEQ: Set region <top> - <bot> inclusive: ESC [ <top> ; <bot> r
 */
/* PROTO */
void
scr_scroll_region(int top, int bot)
{
    MAX_IT(top, 0);
    MIN_IT(bot, TermWin.nrow - 1);
    if (top > bot)
	return;
    screen.tscroll = top;
    screen.bscroll = bot;
    scr_gotorc(0, 0, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Make the cursor visible/invisible
 * XTERM_SEQ: Make cursor visible  : ESC [ ? 25 h
 * XTERM_SEQ: Make cursor invisible: ESC [ ? 25 l
 */
/* PROTO */
void
scr_cursor_visible(int mode)
{
    if (mode)
	screen.flags |= Screen_VisibleCursor;
    else
	screen.flags &= ~Screen_VisibleCursor;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/unset automatic wrapping
 * XTERM_SEQ: Set Wraparound  : ESC [ ? 7 h
 * XTERM_SEQ: Unset Wraparound: ESC [ ? 7 l
 */
/* PROTO */
void
scr_autowrap(int mode)
{
    if (mode)
	screen.flags |= Screen_Autowrap;
    else
	screen.flags &= ~Screen_Autowrap;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/unset margin origin mode
 * Absolute mode: line numbers are counted relative to top margin of screen
 *      and the cursor can be moved outside the scrolling region.
 * Relative mode: line numbers are relative to top margin of scrolling region
 *      and the cursor cannot be moved outside.
 * XTERM_SEQ: Set Absolute: ESC [ ? 6 h
 * XTERM_SEQ: Set Relative: ESC [ ? 6 l
 */
/* PROTO */
void
scr_relative_origin(int mode)
{
    if (mode)
	screen.flags |= Screen_Relative;
    else
	screen.flags &= ~Screen_Relative;
    scr_gotorc(0, 0, 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Set insert/replace mode
 * XTERM_SEQ: Set Insert mode : ESC [ ? 4 h
 * XTERM_SEQ: Set Replace mode: ESC [ ? 4 l
 */
/* PROTO */
void
scr_insert_mode(int mode)
{
    if (mode)
	screen.flags |= Screen_Insert;
    else
	screen.flags &= ~Screen_Insert;
}

/* ------------------------------------------------------------------------- */
/*
 * Set/Unset tabs
 * XTERM_SEQ: Set tab at current column  : ESC H
 * XTERM_SEQ: Clear tab at current column: ESC [ 0 g
 * XTERM_SEQ: Clear all tabs             : ESC [ 3 g
 */
/* PROTO */
void
scr_set_tab(int mode)
{
    if (mode < 0)
	MEMSET(tabs, 0, TermWin.bcol * sizeof(char));

    else if (screen.cur.col < TermWin.bcol)
	tabs[screen.cur.col] = (mode ? 1 : 0);
}

/* ------------------------------------------------------------------------- */
/*
 * Set reverse/normal video
 * XTERM_SEQ: Reverse video: ESC [ ? 5 h
 * XTERM_SEQ: Normal video : ESC [ ? 5 l
 */
/* PROTO */
void
scr_rvideo_mode(int mode)
{
    int             i, j, maxlines;

    if (rvideo != mode) {
	rvideo = mode;
	rstyle ^= RS_RVid;

	maxlines = TermWin.saveLines + TermWin.nrow;
	for (i = TermWin.saveLines; i < maxlines; i++)
	    for (j = 0; j < TermWin.bcol + 1; j++)
		screen.rend[i][j] ^= RS_RVid;
	scr_refresh(SLOW_REFRESH);
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Report current cursor position
 * XTERM_SEQ: Report position: ESC [ 6 n
 */
/* PROTO */
void
scr_report_position(void)
{
    tt_printf((unsigned char *) "\033[%d;%dR",
	      screen.cur.row + 1, screen.cur.col + 1);
}
/* ------------------------------------------------------------------------- *
 *                                  FONTS                                    *
 * ------------------------------------------------------------------------- */

/*
 * Set font style
 */
/* PROTO */
void
set_font_style(void)
{
    rstyle &= ~RS_fontMask;
    switch (charsets[screen.charset]) {
    case '0':			/* DEC Special Character & Line Drawing Set */
	rstyle |= RS_acsFont;
	break;
    case 'A':			/* United Kingdom (UK) */
	rstyle |= RS_ukFont;
	break;
    case 'B':			/* United States (USASCII) */
	break;
    case '<':			/* Multinational character set */
	break;
    case '5':			/* Finnish character set */
	break;
    case 'C':			/* Finnish character set */
	break;
    case 'K':			/* German character set */
	break;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Choose a font
 * XTERM_SEQ: Invoke G0 character set: CTRL-O
 * XTERM_SEQ: Invoke G1 character set: CTRL-N
 * XTERM_SEQ: Invoke G2 character set: ESC N
 * XTERM_SEQ: Invoke G3 character set: ESC O
 */
/* PROTO */
void
scr_charset_choose(int set)
{
    screen.charset = set;
    set_font_style();
}

/* ------------------------------------------------------------------------- */
/*
 * Set a font
 * XTERM_SEQ: Set G0 character set: ESC ( <C>
 * XTERM_SEQ: Set G1 character set: ESC ) <C>
 * XTERM_SEQ: Set G2 character set: ESC * <C>
 * XTERM_SEQ: Set G3 character set: ESC + <C>
 * See set_font_style for possible values for <C>
 */
/* PROTO */
void
scr_charset_set(int set, unsigned int ch)
{
#ifdef MULTICHAR_SET
    multi_byte = (set < 0);
    set = abs(set);
#endif
    charsets[set] = (unsigned char)ch;
    set_font_style();
}

/* ------------------------------------------------------------------------- *
 *          MULTIPLE-CHARACTER FONT SET MANIPULATION FUNCTIONS               *
 * ------------------------------------------------------------------------- */
#ifdef MULTICHAR_SET
# ifdef KANJI
static void     (*multichar_decode) (unsigned char *str, int len) = eucj2jis;
# else				/* then we must be BIG5 to get in here */
static void     (*multichar_decode) (unsigned char *str, int len) = big5dummy;
# endif

/* PROTO */
void
eucj2jis(unsigned char *str, int len)
{
    register int    i;

    for (i = 0; i < len; i++)
	str[i] &= 0x7F;
}

/* ------------------------------------------------------------------------- */
/* PROTO */
void
sjis2jis(unsigned char *str, int len)
{
    register int    i;
    unsigned char  *high, *low;

    for (i = 0; i < len; i += 2, str += 2) {
	high = str;
	low = str + 1;
	(*high) -= (*high > 0x9F ? 0xB1 : 0x71);
	*high = (*high) * 2 + 1;
	if (*low > 0x9E) {
	    *low -= 0x7E;
	    (*high)++;
	} else {
	    if (*low > 0x7E)
		(*low)--;
	    *low -= 0x1F;
	}
    }
}

/* PROTO */
void
big5dummy(unsigned char *str, int len)
{
}

/* PROTO */
void
set_multichar_encoding(const char *str)
{
    if (str && *str) {
	if (!strcmp(str, "sjis")) {
	    encoding_method = SJIS;	/* Kanji SJIS */
	    multichar_decode = sjis2jis;
	} else if (!strcmp(str, "eucj")) {
	    encoding_method = EUCJ;	/* Kanji EUCJ */
	    multichar_decode = eucj2jis;
	}
    }
}
#endif				/* MULTICHAR_SET */

/* ------------------------------------------------------------------------- *
 *                           GRAPHICS COLOURS                                *
 * ------------------------------------------------------------------------- */

#ifdef RXVT_GRAPHICS
/* PROTO */
int
scr_get_fgcolor(void)
{
    return GET_FGCOLOR(rstyle);
}

/* ------------------------------------------------------------------------- */
/* PROTO */
int
scr_get_bgcolor(void)
{
    return GET_BGCOLOR(rstyle);
}
#endif

/* ------------------------------------------------------------------------- *
 *                        MAJOR SCREEN MANIPULATION                          *
 * ------------------------------------------------------------------------- */

/*
 * Refresh an area
 */
/* PROTO */
void
scr_expose(int x, int y, int width, int height)
{
    int             i;
#if 0
    text_t	   *t;
#endif
    row_col_t       full_beg, full_end, part_beg, part_end;

    if (drawn_text == NULL)	/* sanity check */
	return;

/* round down */
    part_beg.col = Pixel2Col(x);
    part_beg.row = Pixel2Row(y);
    full_end.col = Pixel2Width(x + width);
    full_end.row = Pixel2Row(y + height);

/* round up */
    part_end.col = Pixel2Width(x + width + TermWin.fwidth - 1);
    part_end.row = Pixel2Row(y + height + TermWin.fheight - 1);
    full_beg.col = Pixel2Col(x + TermWin.fwidth - 1);
    full_beg.row = Pixel2Row(y + TermWin.fheight - 1);

/* sanity checks */
    MAX_IT(part_beg.col, 0);
    MAX_IT(full_beg.col, 0);
    MAX_IT(part_end.col, 0);
    MAX_IT(full_end.col, 0);
    MAX_IT(part_beg.row, 0);
    MAX_IT(full_beg.row, 0);
    MAX_IT(part_end.row, 0);
    MAX_IT(full_end.row, 0);
    MIN_IT(part_beg.col, TermWin.bcol - 1);
    MIN_IT(full_beg.col, TermWin.bcol - 1);
    MIN_IT(part_end.col, TermWin.bcol - 1);
    MIN_IT(full_end.col, TermWin.bcol - 1);
    MIN_IT(part_beg.row, TermWin.nrow - 1);
    MIN_IT(full_beg.row, TermWin.nrow - 1);
    MIN_IT(part_end.row, TermWin.nrow - 1);
    MIN_IT(full_end.row, TermWin.nrow - 1);

    D_SCREEN((stderr, "scr_expose(x:%d, y:%d, w:%d, h:%d) area (c:%d,r:%d)-(c:%d,r:%d)", x, y, width, height, part_beg.col, part_beg.row, part_end.col, part_end.row));

#if 1	/* XXX: check for backing store/save unders? */
    for (i = part_beg.row; i <= part_end.row; i++)
	    MEMSET(&(drawn_text[i][part_beg.col]), 0,
		   part_end.col - part_beg.col + 1);
#else
     if (full_end.col >= full_beg.col)
     /* set DEFAULT_RSTYLE for totally exposed characters */
       for (i = full_beg.row; i <= full_end.row; i++)
           blank_line(&(drawn_text[i][full_beg.col]),
                      &(drawn_rend[i][full_beg.col]),
                      full_end.col - full_beg.col + 1, DEFAULT_RSTYLE);
/* force an update for partially exposed characters */
    if (part_beg.row != full_beg.row) {
	t = &(drawn_text[part_beg.row][part_beg.col]);
	for (i = part_end.col - part_beg.col + 1; i--;)
	    *t++ = 0;
    }
    if (part_end.row != full_end.row) {
	t = &(drawn_text[part_end.row][part_beg.col]);
	for (i = part_end.col - part_beg.col + 1; i--;)
	    *t++ = 0;
    }
    if (part_beg.col != full_beg.col)
	for (i = full_beg.row; i <= full_end.row; i++)
	    drawn_text[i][part_beg.col] = 0;
    if (part_end.col != full_end.col)
	for (i = full_beg.row; i <= full_end.row; i++)
	    drawn_text[i][part_end.col] = 0;
#endif
    scr_refresh(SLOW_REFRESH);
}

/* ------------------------------------------------------------------------- */
/*
 * Refresh the entire screen
 */
/* PROTO */
void
scr_touch(void)
{
    scr_expose(0, 0, TermWin.width, TermWin.height);
}

/* ------------------------------------------------------------------------- */
/*
 * Move the display so that the line represented by scrollbar value Y is at
 * the top of the screen
 */
/* PROTO */
int
scr_move_to(int y, int len)
{
    int             start, t;

    start = TermWin.view_start;
    t = ((len - y) * (TermWin.nrow - 1 + TermWin.nscrolled)
			  / (len)) - (TermWin.nrow - 1);
    D_SCREEN((stderr, "scr_move_to(%d, %d) view_start:%d", y, len, t));

    MAX_IT(t, 0);
    MIN_IT(t, TermWin.nscrolled);
  
    TermWin.view_start = t;

    if (Gr_Displayed())
	Gr_scroll(0);
    return (TermWin.view_start - start);
}
/* ------------------------------------------------------------------------- */
/*
 * Page the screen up/down nlines
 * direction  should be UP or DN
 */
/* PROTO */
int
scr_page(int direction, int nlines)
{
    int             start, dirn;

    D_SCREEN((stderr, "scr_page(%s, %d) view_start:%d", ((direction == UP) ? "UP" : "DN"), nlines, TermWin.view_start));

    dirn = (direction == UP) ? 1 : -1;
    start = TermWin.view_start;
    MAX_IT(nlines, 1);
    MIN_IT(nlines, TermWin.nrow);

    if ((dirn == -1 && TermWin.view_start < nlines)) 
	    TermWin.view_start = 0;	
    else
	    TermWin.view_start += (nlines * dirn);
  
    if (dirn == 1 && start > TermWin.view_start)
    	TermWin.view_start = TermWin.nscrolled;    	
		
/*  MAX_IT(TermWin.view_start, 0); */
    MIN_IT(TermWin.view_start, TermWin.nscrolled);

    if (Gr_Displayed())
	Gr_scroll(0);
    return (TermWin.view_start - start);
}

/* ------------------------------------------------------------------------- */
/* PROTO */
void
scr_bell(void)
{
#ifndef NO_MAPALERT
# ifdef MAPALERT_OPTION
    if (Options & Opt_mapAlert)
# endif
	XMapWindow(Xdisplay, TermWin.parent);
#endif
    if (Options & Opt_visualBell) {
	scr_rvideo_mode(!rvideo);	/* scr_refresh() also done */
	scr_rvideo_mode(!rvideo);	/* scr_refresh() also done */
    } else
	XBell(Xdisplay, 0);
}

/* ------------------------------------------------------------------------- */
/* ARGSUSED */
/* PROTO */
void
scr_printscreen(int fullhist)
{
#ifdef PRINTPIPE
    int             i, r, nrows, row_offset;
    text_t         *t;
    FILE           *fd;

    if ((fd = popen_printer()) == NULL)
	return;
    nrows = TermWin.nrow;
    if (fullhist)
	nrows += TermWin.view_start;

    row_offset = TermWin.saveLines - TermWin.view_start;
    for (r = 0; r < nrows; r++) {
	t = screen.text[r + row_offset];
	for (i = TermWin.bcol - 1; i >= 0; i--)
	    if (!isspace(t[i]))
		break;
	fprintf(fd, "%.*s\n", (i + 1), t);
    }
    pclose_printer(fd);
#endif
}

/* ------------------------------------------------------------------------- */
/*
 * Refresh the screen
 * drawn_text/drawn_rend contain the screen information before the update.
 * screen.text/screen.rend contain what the screen will change to.
 */

#define DRAW_STRING(Func, x, y, str, len)				\
    Func(Xdisplay, drawBuffer, TermWin.gc, (x), (y), (str), (len))

#if defined (NO_BRIGHTCOLOR) || defined (VERYBOLD)
# define MONO_BOLD(x)	((x) & (RS_Bold|RS_Blink))
#else
# define MONO_BOLD(x)	(((x) & RS_Bold) && fore == Color_fg)
#endif

/* PROTO */
void
scr_refresh(int type)
{
    int             i, j,	/* tmp                                       */
                    col, row,	/* column/row we're processing               */
                    scrrow,	/* screen row offset                         */
                    row_offset,	/* basic offset in screen structure          */
		    currow,	/* cursor row at appropriate offset          */
                    boldlast,	/* last character in some row was bold       */
                    len, wlen,	/* text length screen/buffer                 */
                    fprop,	/* proportional font used                    */
                    is_cursor,	/* cursor this position                      */
                    rvid,	/* reverse video this position               */
                    rend,	/* rendition                                 */
                    fore, back,	/* desired foreground/background             */
                    wbyte,	/* we're in multibyte                        */
		    morecur = 0,
                    xpixel,	/* x offset for start of drawing (font)      */
                    ypixel = 0;	/* y offset for start of drawing (font)      */

    static int      focus = -1;	/* screen in focus?                          */
    long            gcmask;	/* Graphics Context mask                     */
    unsigned long   ltmp;
    rend_t          rt1, rt2;	/* tmp rend values                           */
#ifndef NO_CURSORCOLOR
    rend_t	    ccol1,	/* Cursor colour                             */
		    ccol2,	/* Cursor colour2                            */
		    cc1 = 0;	/* store colours at cursor position(s)       */
# ifdef MULTICHAR_SET
    rend_t	    cc2 = 0;	/* store colours at cursor position(s)       */
# endif
#endif
    rend_t         *drp, *srp;	/* drawn-rend-pointer, screen-rend-pointer   */
    text_t         *dtp, *stp;	/* drawn-text-pointer, screen-text-pointer   */
    XGCValues       gcvalue;	/* Graphics Context values                   */
    XFontStruct    *wf;		/* which font are we in                      */
    static char    *buffer = NULL;
    static int      currmaxcol = 0;
#ifdef MULTICHAR_SET
    static int	    oldcursormulti = 0;
#endif
    static row_col_t oldcursor = { -1, -1};
    				/* is there an old outline cursor on screen? */

#ifdef THAI  /* variables to clean un-touched space at the end of screen */
    int		    thaicol_stp_offset = 0;
    int		    thaicol_dtp_offset = 0;
#endif

#ifndef NO_BOLDFONT
    int             bfont = 0;	/* we've changed font to bold font           */
#endif
    int             (*draw_string) (),
		    (*draw_image_string) ();

    if (type == NO_REFRESH)
	return;

/*
 * A: set up vars
 */
    if (currmaxcol < TermWin.ncol) {
	currmaxcol = TermWin.ncol;
	if (buffer)
	    buffer = REALLOC(buffer, (sizeof(char) * (currmaxcol + 1)));
	else
	    buffer = MALLOC((sizeof(char) * (currmaxcol + 1)));
    }
    row_offset = TermWin.saveLines - TermWin.view_start;
    fprop = TermWin.fprop;
    is_cursor = 0;
    gcvalue.foreground = PixColors[Color_fg];
    gcvalue.background = PixColors[Color_bg];
    dtp = stp = NULL;

/*
 * always go back to the base font - it's much safer
 */
    wbyte = 0;
    XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
    draw_string = XDrawString;
    draw_image_string = XDrawImageString;
    boldlast = 0;
    wf = TermWin.font;

#ifndef NO_BOLDOVERSTRIKE
/*
 * B: Bold Overstrike pixel dropping avoidance.  Do this FIRST.
 *    Do a pass across each line at the start, require a refresh of anything
 *    that will need to be refreshed, due to pixels being dropped into our
 *    area by a previous character which has now been changed.
 */
    for (row = 0; row < TermWin.nrow; row++) {
	scrrow = row + row_offset;
	stp = screen.text[scrrow];
	srp = screen.rend[scrrow];
	dtp = drawn_text[row];
	drp = drawn_rend[row];
# ifndef NO_BOLDFONT
	if (TermWin.boldFont == NULL) {
# endif
	    j = wbyte;
	    for (col = TermWin.ncol - 2; col >= 0; col--) {
	      int do_stuff ;
# if ! defined (NO_BRIGHTCOLOR) && ! defined (VERYBOLD)
		fore = GET_FGCOLOR(drp[col]);
# endif
		if (!MONO_BOLD(drp[col]))
		    continue;
		if (dtp[col] == stp[col]
		    && drp[col] == srp[col])
		    continue;
		if (wbyte) {
		    ;		/* TODO: handle multibyte */
		    continue;	/* don't go past here */
		}
		if (dtp[col] == ' ') {	/* TODO: check character set? */
		    continue;
		}

		if( wf->per_char == NULL )
		    do_stuff = 1 ;
		else if( dtp[col] >= wf->min_char_or_byte2 &&
		    dtp[col] <  wf->max_char_or_byte2)
		{
		  int char_num = dtp[col]-wf->min_char_or_byte2 ;
		    do_stuff = (wf->per_char[char_num].width == wf->per_char[char_num].rbearing);
		}
		else
		    do_stuff = 0 ;

		if (do_stuff)
		{
#ifndef THAI    /* this mess up Thai chars, we have to implement ourselves */
		    dtp[col + 1] = 0;
#endif
# if defined(MULTICHAR_SET) && ! defined(NO_BOLDOVERSTRIKE_MULTI)
		    if ((srp[col] & RS_multiMask) == RS_multi2) {
		        col--;
		        wbyte = 1;
		        continue;
		    }
# endif
		}
	    }
# if ! defined (NO_BRIGHTCOLOR) && ! defined (VERYBOLD)
	    fore = GET_FGCOLOR(srp[TermWin.ncol - 1]);
# endif
	    if (MONO_BOLD(srp[TermWin.ncol - 1]))
		boldlast = 1;
	    wbyte = j;
# ifndef NO_BOLDFONT
	}
# endif
    }
#endif				/* ! NO_BOLDOVERSTRIKE */

/*
 * C: reverse any characters which are selected
 */
    scr_reverse_selection();

/*
 * D: set the cursor character(s)
 */
    currow = screen.cur.row + TermWin.saveLines;
    if (focus != TermWin.focus)
	focus = TermWin.focus;
    if (screen.flags & Screen_VisibleCursor && focus)
	{
		srp = &(screen.rend[currow][screen.cur.col]);
#ifdef THAI
		if(CharWidth(wf,screen.text[currow][screen.cur.col])<=0)
			*srp ^= RS_Bold;
#endif
#ifndef NO_CURSORCOLOR
		cc1 = *srp & (RS_fgMask | RS_bgMask);
		if (Xdepth <= 2 || !rs_color[Color_cursor])
	    	ccol1 = Color_fg;
		else
	    	ccol1 = Color_cursor;
		if (Xdepth <= 2 || !rs_color[Color_cursor2])
	    	ccol2 = Color_bg;
		else
	    	ccol2 = Color_cursor2;
	/*  changed by rev vide cursor patch */
		*srp = SET_BGCOLOR(*srp, ccol1);
		*srp = SET_FGCOLOR(*srp, ccol2);
#endif
#ifdef MULTICHAR_SET
		rt1 = *srp & RS_multiMask;
		if (rt1 == RS_multi1) {
	    	if (screen.cur.col < TermWin.bcol - 2
			&& ((srp[1] & RS_multiMask) == RS_multi2))
			morecur = 1;
		} else if (rt1 == RS_multi2) {
	    	if (screen.cur.col > 0
			&& ((srp[-1] & RS_multiMask) == RS_multi1))
			morecur = -1;
		}
		if (morecur) {
	    	srp += morecur;
	    	*srp ^= RS_RVid;
		}
# ifndef NO_CURSORCOLOR
		if (morecur) {
	    	cc2 = *srp & (RS_fgMask | RS_bgMask);
	    	*srp = SET_BGCOLOR(*srp, ccol1);
	    	*srp = SET_FGCOLOR(*srp, ccol2);
		}
# endif
#endif
    }
    i = 0;
    if (oldcursor.row != -1) {
    /* make sure no outline cursor is left around */
	if (screen.cur.row != oldcursor.row
	    || screen.cur.col != oldcursor.col) {
	    if (oldcursor.row < TermWin.nrow && oldcursor.col < TermWin.bcol) {
		drawn_text[oldcursor.row][oldcursor.col] = 0;
#ifdef MULTICHAR_SET
		if (oldcursormulti) {
		    col = oldcursor.col + oldcursormulti;
		    if (col < TermWin.ncol)
			drawn_text[oldcursor.row][col] = 0;
		}
#endif
	    }
	    if (focus || !(screen.flags & Screen_VisibleCursor))
		oldcursor.row = -1;
	    else
		i = 1;
	}
    } else if (!focus)
	i = 1;
    if (i) {
	oldcursor.row = screen.cur.row;
	oldcursor.col = screen.cur.col;
#ifdef MULTICHAR_SET
	oldcursormulti = morecur;
#endif
    }

/*
 * E: OK, now the real pass
 */
    for (row = 0; row < TermWin.nrow; row++) {
#ifdef THAI
        char thai_update[TermWin.ncol];    /* update map */
	/* compare drawn_text and screen.text and check which to update */
	ThaiUpdateMap(screen.text[row+row_offset],drawn_text[row],
		      screen.rend[row+row_offset],drawn_rend[row],
 		      thai_update,TermWin.ncol);

/*
 * TODO: Change this algo to scalefont compatible
 */
        thaicol_stp_offset = 0;  /* records how many column deficit */
        thaicol_dtp_offset = TermWin.ncol - Thai_ColMaxPaint(dtp,TermWin.ncol);
        /* clean strings before redraw */
        for(col = 0; col < TermWin.ncol; col++){
            if(!ThaiIsMiddleLineCh(stp[col])) thaicol_stp_offset++;
        }

        if(thaicol_stp_offset > thaicol_dtp_offset){
          CLEAR_CHARS(Col2Pixel( TermWin.ncol - thaicol_stp_offset ),
                      ypixel - TermWin.font->ascent, thaicol_stp_offset);

        }
#endif
	scrrow = row + row_offset;
	stp = screen.text[scrrow];
	srp = screen.rend[scrrow];
	dtp = drawn_text[row];
	drp = drawn_rend[row];
	for (col = 0; col < TermWin.ncol; col++) {
	/* compare new text with old - if exactly the same then continue */
	    rt1 = srp[col];	/* screen rendition */
	    rt2 = drp[col];	/* drawn rendition  */
#ifdef THAI  /* updating conditional */
	    if (!thai_update[col]) {
#else
	    if ((stp[col] == dtp[col])	/* must match characters to skip */
		&& ((rt1 == rt2)	/* either rendition the same or  */
		    || ((stp[col] == ' ')	/* space w/ no bg change */
			&& (GET_BGATTR(rt1) == GET_BGATTR(rt2))))) {
#endif
#ifdef MULTICHAR_SET
	    /* if first byte is Kanji then compare second bytes */
		if ((rt1 & RS_multiMask) != RS_multi1)
		    continue;
		else if (stp[col + 1] == dtp[col + 1]) {
		/* assume no corrupt characters on the screen */
		    col++;
		    continue;
		}
#else
		continue;
#endif
	    }
	/* redraw one or more characters */
#ifndef THAI  /* can not set dtp as we go because it causes
               * unpredictable results (bug)
               */
	    dtp[col] = stp[col];
#endif
	    rend = drp[col] = srp[col];

	    len = 0;
	    buffer[len++] = stp[col];
	    ypixel = TermWin.font->ascent + Row2Pixel(row);
#ifdef THAI   /* compute x coordinate */
	    xpixel = ThaiCol2Pixel( col, screen.text[scrrow]);
#else
	    xpixel = Col2Pixel(col);
#endif /* THAI */
	    wlen = 1;

/*
 * Find out the longest string we can write out at once
 */
	    if (fprop == 0) {	/* Fixed width font */
#ifdef MULTICHAR_SET
		if (((rend & RS_multiMask) == RS_multi1)
		    && col < TermWin.ncol - 1
		    && ((srp[col + 1]) & RS_multiMask) == RS_multi2) {
		    if (!wbyte) {
			wbyte = 1;
			XSetFont(Xdisplay, TermWin.gc, TermWin.mfont->fid);
			draw_string = XDrawString16;
			draw_image_string = XDrawImageString16;
		    }
		/* double stepping - we're in Kanji mode */
		    for (; ++col < TermWin.bcol;) {
		    /* XXX: could check sanity on 2nd byte */
			dtp[col] = stp[col];
			drp[col] = srp[col];
			buffer[len++] = stp[col];
			col++;
			if ((col == TermWin.bcol) || (srp[col] != rend))
			    break;
			if ((stp[col] == dtp[col])
			    && (srp[col] == drp[col])
			    && (stp[col + 1] == dtp[col + 1]))
			    break;
			if (len == currmaxcol)
			    break;
			dtp[col] = stp[col];
			drp[col] = srp[col];
			buffer[len++] = stp[col];
		    }
		    col--;
		    if (buffer[0] & 0x80)
			multichar_decode(buffer, len);
		    wlen = len / 2;
		} else {
		    if ((rend & RS_multiMask) == RS_multi1) {
		    /* XXX : maybe do the same thing for RS_multi2 */
		    /* corrupt character - you're outta there */
			rend &= ~RS_multiMask;
			drp[col] = rend;	/* TODO check: may also want */
			dtp[col] = ' ';		/* to poke into stp/srp      */
			buffer[0] = ' ';
		    }
		    if (wbyte) {
			wbyte = 0;
			XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
			draw_string = XDrawString;
			draw_image_string = XDrawImageString;
		    }
#endif
		/* single stepping - `normal' mode */
		    for (; ++col < TermWin.ncol - 1;) {
			if (rend != srp[col])
			    break;
			if ((stp[col] == dtp[col]) && (srp[col] == drp[col]))
			    break;
			if (len == currmaxcol)
			    break;
			dtp[col] = stp[col];
			drp[col] = srp[col];
			buffer[len++] = stp[col];
		    }
		    col--;
		    wlen = len;
#ifdef MULTICHAR_SET
		}
#endif
	    }
	    buffer[len] = '\0';

/*
 * Determine the attributes for the string
 */
	    fore = GET_FGCOLOR(rend);
	    back = GET_BGCOLOR(rend);
	    rend = GET_ATTR(rend);
	    gcmask = 0;
	    rvid = (rend & RS_RVid) ? 1 : 0;

	    switch (rend & RS_fontMask) {
	    case RS_acsFont:
		for (i = 0; i < len; i++)
		    if (buffer[i] == 0x5f)
			buffer[i] = 0x7f;
		    else if (buffer[i] > 0x5f && buffer[i] < 0x7f)
			buffer[i] -= 0x5f;
		break;
	    case RS_ukFont:
		for (i = 0; i < len; i++)
		    if (buffer[i] == '#')
			buffer[i] = 0x1e;
		break;
	    }
	    if (rvid)
		SWAP_IT(fore, back, i);

	    if( colors_changed)
	    {
		gcvalue.background = PixColors[back];
		gcmask |= GCBackground;
		gcvalue.foreground = PixColors[fore];
		gcmask |= GCForeground;
		colors_changed = 0 ;
	    }else if (back != Color_bg) {
		    gcvalue.background = PixColors[back];
		    gcmask |= GCBackground;
		}
	    if (fore != Color_fg) {
	    	    gcvalue.foreground = PixColors[fore];
	    	    gcmask |= GCForeground;
	    }
#ifndef NO_BOLDUNDERLINE
	    else if (rend & RS_Bold) {
		if (Xdepth > 2 && rs_color[Color_BD]
		    && PixColors[fore] != PixColors[Color_BD]
		    && PixColors[back] != PixColors[Color_BD]) {
		    gcvalue.foreground = PixColors[Color_BD];
		    gcmask |= GCForeground;
# ifndef VERYBOLD
		    rend &= ~RS_Bold;	/* we've taken care of it */
# endif
		}
	    } else if (rend & RS_Uline) {
		if (Xdepth > 2 && rs_color[Color_UL]
		    && PixColors[fore] != PixColors[Color_UL]
		    && PixColors[back] != PixColors[Color_UL]) {
		    gcvalue.foreground = PixColors[Color_UL];
		    gcmask |= GCForeground;
		    rend &= ~RS_Uline;	/* we've taken care of it */
		}
	    }
#endif

	    if (gcmask)
		XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);

#ifndef NO_BOLDFONT
	    if (!wbyte && MONO_BOLD(rend) && TermWin.boldFont != NULL) {
		bfont = 1;
		XSetFont(Xdisplay, TermWin.gc, TermWin.boldFont->fid);
		rend &= ~RS_Bold;	/* we've taken care of it */
	    } else if (bfont) {
		bfont = 0;
		XSetFont(Xdisplay, TermWin.gc, TermWin.font->fid);
	    }
#endif
/*
 * Actually do the drawing of the string here
 */
	    if (fprop) {
#ifdef THAI
		if (rvid && CharWidth(wf,stp[col])>0) {
#else
		if (rvid) {
#endif
		    SWAP_IT(gcvalue.foreground, gcvalue.background, ltmp);
		    gcmask |= (GCForeground | GCBackground);
		    XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
		    XFillRectangle(Xdisplay, drawBuffer, TermWin.gc,
				   xpixel, ypixel - TermWin.font->ascent,
				   Width2Pixel(1), Height2Pixel(1));
		    SWAP_IT(gcvalue.foreground, gcvalue.background, ltmp);
		    XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
		} else
#ifdef THAI
 		   if(CharWidth(wf,stp[col])>0)
#endif
		    CLEAR_CHARS(xpixel, ypixel - TermWin.font->ascent, 1);
		DRAW_STRING(draw_string, xpixel, ypixel, buffer, 1);
#ifdef THAI
/*
 * Trap error
 */
 		      if(wlen > 1) fprintf(stderr, "wlen > 1: %d\n", wlen);
#endif
#ifndef NO_BOLDOVERSTRIKE
		if (MONO_BOLD(rend))
		    DRAW_STRING(draw_string, xpixel + 1, ypixel, buffer, 1);
#endif
	    } else
#ifdef TRANSPARENT
	    if ((Options & Opt_transparent) && back == Color_bg) {
#ifdef THAI
	        if(CharWidth(wf,stp[col])>0)
#endif
		CLEAR_CHARS(xpixel, ypixel - TermWin.font->ascent, len);
		DRAW_STRING(draw_string, xpixel, ypixel, buffer, wlen);
	    } else
#endif
#if defined(BACKGROUND_IMAGE) || defined(_MYSTYLE_)
	    if (TermWin.background.trgPixmapSet && back == Color_bg) {
		CLEAR_CHARS(xpixel, ypixel - TermWin.font->ascent, len);
		DRAW_STRING(draw_string, xpixel, ypixel, buffer, wlen);
	    } else
#endif
		DRAW_STRING(draw_image_string, xpixel, ypixel, buffer, wlen);

#ifndef NO_BOLDOVERSTRIKE
# ifdef NO_BOLDOVERSTRIKE_MULTI
	    if (!wbyte)
# endif
		if (MONO_BOLD(rend))
		    DRAW_STRING(draw_string, xpixel + 1, ypixel, buffer, wlen);
#endif
	    if ((rend & RS_Uline) && (TermWin.font->descent > 1))
		XDrawLine(Xdisplay, drawBuffer, TermWin.gc,
			  xpixel, ypixel + 1,
			  xpixel + Width2Pixel(len) - 1, ypixel + 1);
	    if (gcmask) {	/* restore normal colours */
		gcvalue.foreground = PixColors[Color_fg];
		gcvalue.background = PixColors[Color_bg];
		XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
	    }
	}
    }
#ifdef THAI
/*
 * Hack: update drawn_text
 */
	for (row = 0; row < TermWin.nrow; row++)
	{
		scrrow = row + row_offset;
		stp = screen.text[scrrow];
		srp = screen.rend[scrrow];
		dtp = drawn_text[row];
		drp = drawn_rend[row];

		for(col=0 ; col<TermWin.ncol ; col++)
		{
			dtp[col] = stp[col];
			drp[col] = srp[col];
		}
	}
#endif

/*
 * F: cleanup cursor and display outline cursor in necessary
 */
    if (screen.flags & Screen_VisibleCursor)
	{
		if (focus)
		{
	    	srp = &(screen.rend[currow][screen.cur.col]);
#ifdef THAI
	    	if (CharWidth(wf,screen.text[currow][screen.cur.col])<=0)
	        	*srp ^= RS_Bold;
#endif
#ifndef NO_CURSORCOLOR
	    	*srp = (*srp & ~(RS_fgMask | RS_bgMask)) | cc1;
#endif
	    	if (morecur) {
				srp += morecur;
#if defined(MULTICHAR_SET) && ! defined(NO_CURSORCOLOR)
			*srp = (*srp & ~(RS_fgMask | RS_bgMask)) | cc2;
#endif
	    }
	} else {
	    currow = screen.cur.row - TermWin.view_start;
	    col = screen.cur.col + morecur;
	    wbyte = morecur ? 1 : 0;
	    if (currow >= 0 && currow < TermWin.nrow) {
#ifndef NO_CURSORCOLOR
		gcmask = 0;
		if (Xdepth > 2 && rs_color[Color_cursor]) {
		    gcvalue.foreground = PixColors[Color_cursor];
		    gcmask = GCForeground;
		    XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
		    gcvalue.foreground = PixColors[Color_fg];
		}
#endif
#ifdef THAI
		XDrawRectangle(Xdisplay, drawBuffer, TermWin.gc,
			       ThaiCol2Pixel(col,screen.text[screen.cur.row+TermWin.saveLines]),
			       Row2Pixel(currow),
			       Width2Pixel(1 + wbyte) - 1,
			       Height2Pixel(1) - 1);

#else
		XDrawRectangle(Xdisplay, drawBuffer, TermWin.gc,
			       Col2Pixel(col), Row2Pixel(currow),
			       Width2Pixel(1 + wbyte) - 1,
			       Height2Pixel(1) - 1);
#endif
#ifndef NO_CURSORCOLOR
		if (gcmask)	/* restore normal colours */
		    XChangeGC(Xdisplay, TermWin.gc, gcmask, &gcvalue);
#endif
	    }
	}
    }


/*
 * G: cleanup selection
 */
    scr_reverse_selection();

/*
 * H: other general cleanup
 */
    if (boldlast && TermWin.bMapped)  	/* clear the whole screen height */
	XClearArea(Xdisplay, TermWin.vt, TermWin_TotalWidth() - 2, 0,
		   1, TermWin_TotalHeight() - 1, 0);
    if (type & SMOOTH_REFRESH)
	XSync(Xdisplay, False);
}

/* PROTO */
void
scr_clear_tint(int bWithTinting)
{
    if( !TermWin.bMapped ) return ;
#ifdef TRANSPARENT
    if (Options & Opt_transparent)
    {
/*	int n ;
	fprintf( stderr, "\n Clearing Parents ...");
	for( n = 0 ; n < ParentWinNum ; n++ )
*/
        XClearWindow(Xdisplay, TermWin.parent);
/*	XFlush( Xdisplay );
	sleep(5);
	fprintf( stderr, "Done.");
*/    }
#endif
    XClearWindow(Xdisplay, TermWin.vt);

#if defined(BACKGROUND_IMAGE) || defined(TRANSPARENT)
    if (TermWin.tintGC && bWithTinting)
    {
            XFillRectangle(Xdisplay, TermWin.vt, TermWin.tintGC, 0, 0,
                           TermWin.width+TermWin_internalBorders, TermWin.height+TermWin_internalBorders);
    }
#endif
}

/* ------------------------------------------------------------------------- */
/* colors has changed so we have to repaint self			     */
/* PROTO */
void
on_colors_changed(int idx)
{
    colors_changed = 1 ;
    if ( idx == Color_bg &&
#ifdef BACKGROUND_IMAGE 
		 TermWin.background.trgPixmapSet == 0 &&
#endif
		 !(Options & Opt_transparent))
    {
		XSetWindowBackground(Xdisplay, TermWin.vt, PixColors[Color_bg]);
    }

/* handle Color_BD, scrollbar background, etc. */

    set_colorfgbg();
    set_cursor_color();
/* the only reasonable way to enforce a clean update : */
    scr_clear();
    scr_touch();
}






/* ------------------------------------------------------------------------- */
/* PROTO */
void
scr_reverse_selection(void)
{
    int		i, col, row, end_row;
    rend_t     *srp;

    end_row = TermWin.saveLines - TermWin.view_start;
    if (selection.op && current_screen == selection.screen) {
	i = selection.beg.row + TermWin.saveLines;
	row = selection.end.row + TermWin.saveLines;
	if (i >= end_row)
	    col = selection.beg.col;
	else {
	    col = 0;
	    i = end_row;
	}
	end_row += TermWin.nrow;
	for (; i < row && i < end_row; i++, col = 0)
	    for (srp = screen.rend[i]; col < TermWin.bcol; col++)
		srp[col] ^= RS_RVid;
	if (i == row && i < end_row)
	    for (srp = screen.rend[i]; col < selection.end.col; col++)
		srp[col] ^= RS_RVid;
    }
}

/* ------------------------------------------------------------------------- *
 *                           CHARACTER SELECTION                             *
 * ------------------------------------------------------------------------- */

/*
 * -TermWin.nscrolled <= (selection row) <= TermWin.nrow - 1
 */
/* PROTO */
void
selection_check(int check_more)
{
    row_col_t       pos;

    if ((selection.beg.row < -TermWin.nscrolled)
	|| (selection.beg.row >= TermWin.nrow)
	|| (selection.mark.row < -TermWin.nscrolled)
	|| (selection.mark.row >= TermWin.nrow)
	|| (selection.end.row < -TermWin.nscrolled)
	|| (selection.end.row >= TermWin.nrow))
	CLEAR_ALL_SELECTION;

    if (check_more == 1 && current_screen == selection.screen) {
    /* check for cursor position */
	pos.row = screen.cur.row;
	pos.col = screen.cur.col;
	if (!ROWCOL_IS_BEFORE(pos, selection.beg)
	    && ROWCOL_IS_BEFORE(pos, selection.end))
	    CLEAR_SELECTION;
    } else if (check_more == 2) {
	pos.row = 0;
	pos.col = 0;
	if (ROWCOL_IS_BEFORE(selection.beg, pos)
	    && ROWCOL_IS_AFTER(selection.end, pos))
	    CLEAR_SELECTION;
    } else if (check_more == 3) {
	pos.row = 0;
	pos.col = 0;
	if (ROWCOL_IS_AFTER(selection.end, pos))
	    CLEAR_SELECTION;
    }
}

/* ------------------------------------------------------------------------- */
/*
 * Paste a selection direct to the command
 */
/* PROTO */
void
PasteIt(unsigned char *data, unsigned int nitems)
{
    int             num;
    unsigned char  *p, cr;

    cr = '\r';
    for (p = data, num = 0; nitems--; p++)
	if (*p != '\n')
	    num++;
	else {
	    tt_write(data, num);
	    tt_write(&cr, 1);
	    data += (num + 1);
	    num = 0;
	}
    if (num)
	tt_write(data, num);
}

static void
paste_text_property(XTextProperty   *ct)
{
	int             dummy_count;
	char          **cl;
	if (XmbTextPropertyToTextList(Xdisplay, ct, &cl, &dummy_count) == Success && cl) 
	{
		PasteIt(cl[0], strlen(cl[0]));
		XFreeStringList(cl);
	}else 
		PasteIt(ct->value, (unsigned int)ct->nitems);
}	   

/* ------------------------------------------------------------------------- */
/*
 * Respond to a notification that a primary selection has been sent
 * EXT: SelectionNotify
 */
/* PROTO */
void
selection_paste(Window win, unsigned prop, int Delete)
{
	unsigned long   bytes_after;
	XTextProperty   ct;
	XEvent          ev;
	XWindowAttributes wa;
    
    if (prop == None) 
	{
		if (selection.type == 0) 
		{
	    	selection.type++; 
    	    XConvertSelection(Xdisplay, XA_PRIMARY, aterm_XA_UTF8_STRING, aterm_XA_VT_SELECTION, TermWin.vt,selection.request_time);
        } else if (selection.type == 1) 
		{
	    	selection.type++;
    	    XConvertSelection(Xdisplay, XA_PRIMARY, XA_STRING, aterm_XA_VT_SELECTION, TermWin.vt,selection.request_time);
		}
    	return;
    }

	ct.value = NULL ;
	if ((XGetWindowProperty(Xdisplay, win, prop, 0, ~0,
							False, AnyPropertyType, &ct.encoding, &ct.format,
							&ct.nitems, &bytes_after, &ct.value) == Success)) 
	{
		if (ct.encoding == aterm_XA_INCR) 
		{/* This is an INCR (incremental large) paste	*/
			Bool success = True;
  	   
			XGetWindowAttributes(Xdisplay, win, &wa);
			if ( !get_flags(wa.your_event_mask, PropertyChangeMask) ) /* We need to register the PropertyNotify event */
				XSelectInput(Xdisplay, win, (wa.your_event_mask | PropertyChangeMask) );

			XDeleteProperty(Xdisplay, win, prop);
			do
			{
				XWindowEvent(Xdisplay, win, PropertyChangeMask, &ev);
				if (ev.xproperty.state != PropertyNewValue)
					continue;
				success = ( XGetWindowProperty(Xdisplay, win, prop, 0, ~0,
							True, AnyPropertyType, &ct.encoding, &ct.format,
							&ct.nitems, &bytes_after, &ct.value) == Success);

				if( success && ct.nitems > 0)
				{	
					paste_text_property(&ct);
					XFree(ct.value);
					ct.value = NULL ;
				}
			}while ( success && ct.nitems > 0);

			if( !get_flags(wa.your_event_mask, PropertyChangeMask) ) /* We need to unregister the PropertyNotify event */
				XSelectInput(Xdisplay, win, wa.your_event_mask);
    	}else 
		{/* This is a normal 1-time paste */
			if (Delete) 
				XDeleteProperty(Xdisplay, win, prop);
		
			paste_text_property(&ct);
   		}
	}
	if( ct.value ) 
		XFree(ct.value);
}

/* ------------------------------------------------------------------------- */
/*
 * Request the current primary selection
 * EXT: button 2 release
 */
/* PROTO */
void
selection_request(Time tm, int x, int y)
{
    selection.request_time = tm;
    if (selection.text != NULL) 
	{
		PasteIt(selection.text, selection.len);		/* internal selection */
    } else if (XGetSelectionOwner(Xdisplay, XA_PRIMARY) == None) 
	{
		selection_paste(Xroot, XA_CUT_BUFFER0, False);
    } else 
	{
        selection.type = 1;
		/* "fix" for the high-ascii pasting? : */
		/* may need to try all three : aterm_XA_UTF8_STRING, _XA_COMPAUND_TEXT and XA_STRING in that order */
		XConvertSelection(Xdisplay, XA_PRIMARY, XA_STRING, aterm_XA_VT_SELECTION, TermWin.vt,tm);
/*	XConvertSelection(Xdisplay, XA_PRIMARY, _XA_COMPAUND_TEXT, prop, TermWin.vt,tm); */
    }
}

#if 0 
/* Here is how this is implemented in yudit : 
 */
SString
SX11Impl::getSelectionData (Atom dataName, Atom dataType, long id, Time _time)
{
  // use encoder to encode.
  XEvent    xevent;

  time_t  start;
  Atom    propty;

  propty = toAtom ("YUDIT_SELECTION");;

  XConvertSelection (display, dataName, dataType, 
    propty, (Window)id, _time );

  XFlush (display);
  for (start = time (0); ; )
  {
    if (XCheckTypedWindowEvent(display, (Window)id,
      SelectionNotify, &xevent))
    {
      break;
    }
    if (time (0) - start >= 8)
    {
      fprintf (stderr, "warn: clipboard timeout.\n");
      return SString();
    }
  }
  return getPropertyData (xevent.xselection.requestor, propty, 8);
}

/**
 * get an utf-8 string from clipboard.
 * FIXME
 */
SString
SX11Impl::getClipUTF8(long id)
{
  Window    owner;
  SEncoder utf8e;

  // If we own the selection ourselves, no need for interprocess
  // communication.
  if (clipOwner==(owner=XGetSelectionOwner (display, XA_PRIMARY)))
  {
    return SString(clipBuffer);
  }
  clipBuffer.clear();
  clipOwner = None;

  // Try UTF8_STRING first.
  clipBuffer = getSelectionData (XA_PRIMARY, toAtom("UTF8_STRING"), id, CurrentTime);
  if (clipBuffer.size())
  {
    //fprintf (stderr, "SX11Impl::getClipUTF8: got UTF8_STRING\n");
    return SString(clipBuffer);
  }

  // Then try COMPOUND_TEXT.
  clipBuffer = getSelectionData (XA_PRIMARY, toAtom("COMPOUND_TEXT"), id, CurrentTime);
  if (clipBuffer.size())
  {
    //fprintf (stderr, "SX11Impl::getClipUTF8: got COMPOUND_TEXT\n");
    clipBuffer = utf8e.encode (encoder.decode(clipBuffer));
    return SString (clipBuffer);
  }

  // Finally try STRING.
  clipBuffer = getSelectionData (XA_PRIMARY, XA_STRING, id, CurrentTime);
  if (clipBuffer.size())
  {
    //fprintf (stderr, "SX11Impl::getClipUTF8: got XA_STRING\n");
    SEncoder  iso_1("iso-8859-1");
    clipBuffer = utf8e.encode (iso_1.decode(clipBuffer));
    return SString (clipBuffer);
  }
  return SString(clipBuffer);
}

#endif

/* ------------------------------------------------------------------------- */
/*
 * Clear all selected text
 * EXT: SelectionClear
 */
/* PROTO */
void
selection_clear(void)
{
    D_SELECT((stderr, "selection_clear()"));

    if (selection.text)
	FREE(selection.text);
    selection.text = NULL;
    selection.len = 0;
    CLEAR_SELECTION;
}

/* ------------------------------------------------------------------------- */
/*
 * Copy a selection into the cut buffer
 * EXT: button 1 or 3 release
 */
/* PROTO */
void
selection_make(Time tm, unsigned int key_state)
{
    int             i, col, end_col, row, end_row;
    unsigned char  *new_selection_text;
    char           *str;
    text_t         *t;

    D_SELECT((stderr, "selection_make(): selection.op=%d, selection.clicks=%d", selection.op, selection.clicks));
    switch (selection.op) {
    case SELECTION_CONT:
	break;
    case SELECTION_INIT:
	CLEAR_SELECTION;
    /* FALLTHROUGH */
    case SELECTION_BEGIN:
	selection.op = SELECTION_DONE;
    /* FALLTHROUGH */
    default:
	return;
    }
    selection.op = SELECTION_DONE;

    if (selection.clicks == 4)
	return;			/* nothing selected, go away */

    i = (selection.end.row - selection.beg.row + 1) * (TermWin.bcol + 1) + 1;
    str = MALLOC(i * sizeof(char));
    new_selection_text = (unsigned char *) str;

    col = max((int)selection.beg.col, 0);
    row = selection.beg.row + TermWin.saveLines;
    end_row = selection.end.row + TermWin.saveLines;
/*
 * A: rows before end row
 */
    for (; row < end_row; row++) {
	t = &(screen.text[row][col]);
	if ((end_col = screen.tlen[row]) == -1)
	    end_col = TermWin.bcol;
	for (; col < end_col; col++)
	    *str++ = *t++;
	col = 0;
	if (screen.tlen[row] != -1)
	    *str++ = '\n';
    }
/*
 * B: end row
 */
    t = &(screen.text[row][col]);
    end_col = screen.tlen[row];
    if (end_col == -1 || selection.end.col <= end_col)
	end_col = selection.end.col;
    MIN_IT(end_col, TermWin.bcol);	/* CHANGE */
    for (; col < end_col; col++)
	*str++ = *t++;
    if (end_col != selection.end.col)
	*str++ = '\n';
    *str = '\0';
    if ((i = strlen((char *) new_selection_text)) == 0) {
	FREE(new_selection_text);
	return;
    }
    selection.len = i;
    if (selection.text)
	FREE(selection.text);
    selection.text = new_selection_text;

    // selecting with ALT will put the text to clipboard
    if (key_state & Mod1Mask) {
        XSetSelectionOwner(Xdisplay, aterm_XA_CLIPBOARD, TermWin.vt, tm);
    } else {
        XSetSelectionOwner(Xdisplay, XA_PRIMARY, TermWin.vt, tm);
    }
	
    if (XGetSelectionOwner(Xdisplay, XA_PRIMARY) != TermWin.vt)
	print_error("can't get primary selection");
    XChangeProperty(Xdisplay, Xroot, XA_CUT_BUFFER0, XA_STRING, 8,
		    PropModeReplace, selection.text, selection.len);
    D_SELECT((stderr, "selection_make(): selection.len=%d", selection.len));
}

/* ------------------------------------------------------------------------- */
/*
 * Mark or select text based upon number of clicks: 1, 2, or 3
 * EXT: button 1 press
 */
/* PROTO */
void
selection_click(int clicks, int x, int y)
{
/*    int             r, c;
 *   row_col_t             ext_beg, ext_end;
 */

    D_SELECT((stderr, "selection_click(%d, %d, %d)", clicks, x, y));

    clicks = ((clicks - 1) % 3) + 1;
    selection.clicks = clicks;	/* save clicks so extend will work */
#ifdef THAI
    selection_start_colrow(ThaiPixel2Col(x,y), Pixel2Row(y));
#else
    selection_start_colrow(Pixel2Col(x), Pixel2Row(y));
#endif
    if (clicks == 2 || clicks == 3)
	selection_extend_colrow(selection.mark.col,
				selection.mark.row + TermWin.view_start,
				0,	/* button 3     */
				1,	/* button press */
				0);	/* click change */
}

/* ------------------------------------------------------------------------- */
/*
 * Mark a selection at the specified col/row
 */
/* PROTO */
void
selection_start_colrow(int col, int row)
{
    selection.mark.col = col;
    selection.mark.row = row - TermWin.view_start;
    MAX_IT(selection.mark.row, -TermWin.nscrolled);
    MIN_IT(selection.mark.row, TermWin.nrow - 1);
    MAX_IT(selection.mark.col, 0);
    MIN_IT(selection.mark.col, TermWin.bcol - 1);

    if (selection.op) {		/* clear the old selection */
	selection.beg.row = selection.end.row = selection.mark.row;
	selection.beg.col = selection.end.col = selection.mark.col;
    }
    selection.op = SELECTION_INIT;
    selection.screen = current_screen;
}

/* ------------------------------------------------------------------------- */
/*
 * Word select: select text for 2 clicks
 * We now only find out the boundary in one direction
 */

/* what do we want: spaces/tabs are delimiters or cutchars or non-cutchars */
#define DELIMIT_TEXT(x) \
    (((x) == ' ' || (x) == '\t') ? 2 : (strchr(rs_cutchars, (x)) != NULL))
#ifdef MULTICHAR_SET
# define DELIMIT_REND(x)	(((x) & RS_multiMask) ? 1 : 0)
#else
# define DELIMIT_REND(x)	1
#endif

/* PROTO */
void
selection_delimit_word(int dirn, row_col_t * mark, row_col_t * ret)
{
    int             col, row, dirnadd, tcol, trow, w1, w2;
    row_col_t       bound;
    text_t         *stp;
    rend_t         *srp;

    if (selection.clicks != 2)
	return;			/* Go away: we only handle double clicks */

    if (dirn == UP) {
	bound.row = TermWin.saveLines - TermWin.nscrolled - 1;
	bound.col = 0;
	dirnadd = -1;
    } else {
	bound.row = TermWin.saveLines + TermWin.nrow;
	bound.col = TermWin.bcol - 1;
	dirnadd = 1;
    }
    row = mark->row + TermWin.saveLines;
    col = mark->col;
    MAX_IT(col, 0);
/* find the edge of a word */
    stp = &(screen.text[row][col]);
    w1 = DELIMIT_TEXT(*stp);
#ifdef OLD_WORD_SELECTION
    if (w1 == 1) {
	stp += dirnadd;
	if (DELIMIT_TEXT(*stp) == 1)
	    goto Old_Word_Selection_You_Die;
	col += dirnadd;
	srp += dirnadd;
    }
    w1 = 0;
#endif
    srp = (&screen.rend[row][col]);
    w2 = DELIMIT_REND(*srp);

    for (;;) {
	for (; col != bound.col; col += dirnadd) {
	    stp += dirnadd;
	    if (DELIMIT_TEXT(*stp) != w1)
		break;
	    srp += dirnadd;
	    if (DELIMIT_REND(*srp) != w2)
		break;
	}
	if ((col == bound.col) && (row != bound.row)) {
	    if (screen.tlen[(row - (dirn == UP))] == -1) {
		trow = row + dirnadd;
		tcol = (dirn == UP) ? (TermWin.bcol - 1) : 0;
		if (screen.text[trow] == NULL)
		    break;
		stp = &(screen.text[trow][tcol]);
		srp = &(screen.rend[trow][tcol]);
		if (DELIMIT_TEXT(*stp) != w1 || DELIMIT_REND(*srp) != w2)
		    break;
		row = trow;
		col = tcol;
		continue;
	    }
	}
	break;
    }
#ifdef OLD_WORD_SELECTION
Old_Word_Selection_You_Die:
#endif
    D_SELECT((stderr, "selection_delimit_word(%s,...) @ (r:%3d, c:%3d) has boundary (r:%3d, c:%3d)", (dirn == UP ? "up	" : "down"), mark->row, mark->col, row - TermWin.saveLines, col));

    if (dirn == DN)
	col++;			/* put us on one past the end */

/* Poke the values back in */
    ret->row = row - TermWin.saveLines;
    ret->col = col;
}

/* ------------------------------------------------------------------------- */
/*
 * Extend the selection to the specified x/y pixel location
 * EXT: button 3 press; button 1 or 3 drag
 * flag == 0 ==> button 1
 * flag == 1 ==> button 3 press
 * flag == 2 ==> button 3 motion
 */
/* PROTO */
void
selection_extend(int x, int y, int flag)
{
    int             col, row;

#ifdef THAI
    col = ThaiPixel2Col( x, y);
#else
    col = Pixel2Col(x);
#endif
    row = Pixel2Row(y);
    MAX_IT(row, 0);
    MIN_IT(row, TermWin.nrow - 1);
    MAX_IT(col, 0);
    MIN_IT(col, TermWin.bcol);
#ifndef OLD_SELECTION
/*
 * If we're selecting characters (single click) then we must check first
 * if we are at the same place as the original mark.  If we are then
 * select nothing.  Otherwise, if we're to the right of the mark, you have to
 * be _past_ a character for it to be selected.
 */
    if (((selection.clicks % 3) == 1) && !flag
	&& (col == selection.mark.col
	    && (row == selection.mark.row + TermWin.view_start))) {
    /* select nothing */
	selection.beg.row = selection.end.row = 0;
	selection.beg.col = selection.end.col = 0;
	selection.clicks = 4;
	D_SELECT((stderr, "selection_extend() selection.clicks = 4"));
	return;
    }
#endif
    if (selection.clicks == 4)
	selection.clicks = 1;
    selection_extend_colrow(col, row,
			    !!flag,	/* ? button 3      */
			    flag == 1 ? 1 : 0,	/* ? button press  */
			    0);	/* no click change */
}

/* ------------------------------------------------------------------------- */
/*
 * Extend the selection to the specified col/row
 */
/* PROTO */
void
selection_extend_colrow(int col, int row, int button3, int buttonpress, int clickchange)
{
    int             end_col;
    row_col_t       pos;
    enum {
	LEFT, RIGHT
    } closeto =     RIGHT;
#ifdef MULTICHAR_SET
    int             c, r;
#endif

    D_SELECT((stderr, "selection_extend_colrow(c:%d, r:%d, %d, %d) clicks:%d", col, row, button3, buttonpress, selection.clicks));
    D_SELECT((stderr, "selection_extend_colrow() ENT  b:(r:%d,c:%d) m:(r:%d,c:%d), e:(r:%d,c:%d)", selection.beg.row, selection.beg.col, selection.mark.row, selection.mark.col, selection.end.row, selection.end.col));

    switch (selection.op) {
    case SELECTION_INIT:
	CLEAR_SELECTION;
	selection.op = SELECTION_BEGIN;
    /* FALLTHROUGH */
    case SELECTION_BEGIN:
	if (row != selection.mark.row || col != selection.mark.col
	    || (!button3 && buttonpress))
	    selection.op = SELECTION_CONT;
	break;
    case SELECTION_DONE:
	selection.op = SELECTION_CONT;
    /* FALLTHROUGH */
    case SELECTION_CONT:
	break;
    case SELECTION_CLEAR:
	selection_start_colrow(col, row);
    /* FALLTHROUGH */
    default:
	return;
    }

    pos.col = col;
    pos.row = row;

    pos.row -= TermWin.view_start;	/* adjust for scroll */

#ifdef OLD_SELECTION
/*
 * This mimics some of the selection behaviour of version 2.20 and before.
 * There are no ``selection modes'', button3 is always character extension.
 * Note: button3 drag is always available, c.f. v2.20
 * Selection always terminates (left or right as appropriate) at the mark.
 */
    {
	static int      hate_those_clicks = 0;	/* a.k.a. keep mark position */

	if (selection.clicks == 1 || button3) {
	    if (hate_those_clicks) {
		selection.mark.row = selection.beg.row;
		selection.mark.col = selection.beg.col;
		hate_those_clicks = 0;
	    }
	    if (ROWCOL_IS_BEFORE(pos, selection.beg)) {
		selection.end.row = selection.beg.row;
		selection.end.col = selection.beg.col + 1;
		selection.beg.row = pos.row;
		selection.beg.col = pos.col;
	    } else {
		selection.beg.row = selection.mark.row;
		selection.beg.col = selection.mark.col;
		selection.end.row = pos.row;
		selection.end.col = pos.col + 1;
	    }
	} else if (selection.clicks == 2) {
	    selection_delimit_word(UP, &(selection.mark), &(selection.beg));
	    selection_delimit_word(DN, &(selection.mark), &(selection.end));
	    hate_those_clicks = 1;
	} else if (selection.clicks == 3) {
	    selection.beg.row = selection.end.row = selection.mark.row;
	    selection.beg.col = 0;
	    selection.end.col = TermWin.bcol;
	    hate_those_clicks = 1;
	}
    }
#else				/* ! OLD_SELECTION */
/*
 * This is mainly xterm style selection with a couple of differences, mainly
 * in the way button3 drag extension works.
 * We're either doing: button1 drag; button3 press; or button3 drag
 *  a) button1 drag : select around a midpoint/word/line - that point/word/line
 *     is always at the left/right edge of the selection.
 *  b) button3 press: extend/contract character/word/line at whichever edge of
 *     the selection we are closest to.
 *  c) button3 drag : extend/contract character/word/line - we select around
 *     a point/word/line which is either the start or end of the selection
 *     and it was decided by whichever point/word/line was `fixed' at the
 *     time of the most recent button3 press
 */
    if (button3 && buttonpress) {	/* button3 press */
    /*
     * first determine which edge of the selection we are closest to
     */
	if (ROWCOL_IS_BEFORE(pos, selection.beg)
	    || (!ROWCOL_IS_AFTER(pos, selection.end)
		&& (((pos.col - selection.beg.col)
		     + ((pos.row - selection.beg.row) * TermWin.bcol))
		    < ((selection.end.col - pos.col)
		       + ((selection.end.row - pos.row) * TermWin.bcol)))))
	    closeto = LEFT;
	if (closeto == LEFT) {
	    selection.beg.row = pos.row;
	    selection.beg.col = pos.col;
	    selection.mark.row = selection.end.row;
	    selection.mark.col = selection.end.col - (selection.clicks == 2);
	} else {
	    selection.end.row = pos.row;
	    selection.end.col = pos.col;
	    selection.mark.row = selection.beg.row;
	    selection.mark.col = selection.beg.col;
	}
    } else {			/* button1 drag or button3 drag */
	if (ROWCOL_IS_AFTER(selection.mark, pos)) {
	    if ((selection.mark.row == selection.end.row)
		&& (selection.mark.col == selection.end.col)
		&& clickchange && selection.clicks == 2)
		selection.mark.col--;
	    selection.beg.row = pos.row;
	    selection.beg.col = pos.col;
	    selection.end.row = selection.mark.row;
	    selection.end.col = selection.mark.col + (selection.clicks == 2);
	} else {
	    selection.beg.row = selection.mark.row;
	    selection.beg.col = selection.mark.col;
	    selection.end.row = pos.row;
	    selection.end.col = pos.col;
	}
    }

    if (selection.clicks == 1) {
	end_col = screen.tlen[selection.beg.row + TermWin.saveLines];
	if (end_col != -1 && selection.beg.col > end_col) {
#if 1
	    selection.beg.col = TermWin.bcol;
#else
	    if (selection.beg.row != selection.end.row)
		selection.beg.col = TermWin.bcol;
	    else
		selection.beg.col = selection.mark.col;
#endif
	}
	end_col = screen.tlen[selection.end.row + TermWin.saveLines];
	if (end_col != -1 && selection.end.col > end_col)
	    selection.end.col = TermWin.bcol;

# ifdef MULTICHAR_SET
	if (selection.beg.col > 0) {
	    r = selection.beg.row + TermWin.saveLines;
	    c = selection.beg.col;
	    if (((screen.rend[r][c] & RS_multiMask) == RS_multi2)
		&& ((screen.rend[r][c - 1] & RS_multiMask) == RS_multi1))
		selection.beg.col--;
	}
	if (selection.end.col < TermWin.bcol) {
	    r = selection.end.row + TermWin.saveLines;
	    c = selection.end.col;
	    if (((screen.rend[r][c - 1] & RS_multiMask) == RS_multi1)
		&& ((screen.rend[r][c] & RS_multiMask) == RS_multi2))
		selection.end.col++;
	}
# endif				/* MULTICHAR_SET */
    } else if (selection.clicks == 2) {
	if (ROWCOL_IS_AFTER(selection.end, selection.beg))
	    selection.end.col--;
	selection_delimit_word(UP, &(selection.beg), &(selection.beg));
	selection_delimit_word(DN, &(selection.end), &(selection.end));
    } else if (selection.clicks == 3) {
	if (ROWCOL_IS_AFTER(selection.mark, selection.beg))
	    selection.mark.col++;
	selection.beg.col = 0;
	selection.end.col = TermWin.bcol;
    }
    if (button3 && buttonpress) {	/* mark may need to be changed */
	if (closeto == LEFT) {
	    selection.mark.row = selection.end.row;
	    selection.mark.col = selection.end.col - (selection.clicks == 2);
	} else {
	    selection.mark.row = selection.beg.row;
	    selection.mark.col = selection.beg.col;
	}
    }
#endif				/* ! OLD_SELECTION */
    D_SELECT((stderr, "selection_extend_colrow() EXIT b:(r:%d,c:%d) m:(r:%d,c:%d), e:(r:%d,c:%d)", selection.beg.row, selection.beg.col, selection.mark.row, selection.mark.col, selection.end.row, selection.end.col));
}

/* ------------------------------------------------------------------------- */
/*
 * Double click on button 3 when already selected
 * EXT: button 3 double click
 */
/* PROTO */
void
selection_rotate(int x, int y)
{
    selection.clicks = selection.clicks % 3 + 1;
#ifdef THAI
    selection_extend_colrow(ThaiPixel2Col(x,y), Pixel2Row(y), 1, 0, 1);
#else
    selection_extend_colrow(Pixel2Col(x), Pixel2Row(y), 1, 0, 1);
#endif
}

/* ------------------------------------------------------------------------- */
#ifdef USE_ATOM32
/*
 * On some systems, the Atom typedef is 64 bits wide.  We need to have a type
 * that is exactly 32 bits wide, because a format of 64 is not allowed by
 * the X11 protocol.
 */
typedef CARD32  Atom32;
#endif
/* ------------------------------------------------------------------------- */
/*
 * Respond to a request for our current selection
 * EXT: SelectionRequest
 */
/* PROTO */
void
selection_send(XSelectionRequestEvent * rq)
{
    /* Changes are from rxvt. This fixes #205040. */
    XEvent          ev;
#ifdef USE_ATOM32		   
    Atom32          target_list[4];
#else
	Atom          target_list[4];
#endif 
    Atom            target;
    XTextProperty   ct;
    XICCEncodingStyle style;
    char           *cl[4];

    ev.xselection.type = SelectionNotify;
    ev.xselection.property = None;
    ev.xselection.display = rq->display;
    ev.xselection.requestor = rq->requestor;
    ev.xselection.selection = rq->selection;
    ev.xselection.target = rq->target;
    ev.xselection.time = rq->time;

    if (rq->target == aterm_XA_TARGETS) {
#ifdef USE_ATOM32		
	target_list[0] = (Atom32) aterm_XA_TARGETS;
	target_list[1] = (Atom32) XA_STRING;
	target_list[2] = (Atom32) aterm_XA_TEXT;
	target_list[3] = (Atom32) _XA_COMPAUND_TEXT;
	XChangeProperty(Xdisplay, rq->requestor, rq->property, rq->target,
			(8 * sizeof(target_list[0])), PropModeReplace,
			(unsigned char *)target_list,
			(sizeof(target_list) / sizeof(target_list[0])));
#else	  
	target_list[0] =  aterm_XA_TARGETS;
	target_list[1] =  XA_STRING;
	target_list[2] =  aterm_XA_TEXT;
	target_list[3] =  _XA_COMPAUND_TEXT;
	XChangeProperty(Xdisplay, rq->requestor, rq->property, XA_ATOM,
			32, PropModeReplace,
 			(unsigned char *)target_list,
 			(sizeof(target_list) / sizeof(target_list[0])));
#endif	 
	ev.xselection.property = rq->property;
    } else if (rq->target == XA_STRING
	       || rq->target == _XA_COMPAUND_TEXT
	       || rq->target == aterm_XA_TEXT) {
	if (rq->target == XA_STRING) {
	    style = XStringStyle;
	    target = XA_STRING;
	} else {
	    target = _XA_COMPAUND_TEXT;
	    style = (rq->target == _XA_COMPAUND_TEXT) ? XCompoundTextStyle
		: XStdICCTextStyle;
	}
	cl[0] = selection.text;
	XmbTextListToTextProperty(Xdisplay, cl, 1, style, &ct);
	XChangeProperty(Xdisplay, rq->requestor, rq->property,
			target, 8, PropModeReplace,
			ct.value, ct.nitems);
	ev.xselection.property = rq->property;
    }
    XSendEvent(Xdisplay, rq->requestor, False, 0, &ev);
}

/* ------------------------------------------------------------------------- *
 *                              MOUSE ROUTINES                               *
 * ------------------------------------------------------------------------- */

/*
 * return col/row values corresponding to x/y pixel values
 */
/* PROTO */
void
pixel_position(int *x, int *y)
{
#ifdef THAI
  *x = ThaiPixel2Col(*x, *y);
#else
  *x = Pixel2Col(*x);
#endif
  /* MAX_IT(*x, 0); MIN_IT(*x, TermWin.bcol - 1); */
  *y = Pixel2Row(*y);
  /* MAX_IT(*y, 0); MIN_IT(*y, TermWin.nrow - 1); */
}

/* ------------------------------------------------------------------------- */
/* ARGSUSED */
/* PROTO */
void
mouse_tracking(int report, int x, int y, int firstrow, int lastrow)
{
/* TODO */
}

/* ------------------------------------------------------------------------- *
 *                              DEBUG ROUTINES                               *
 * ------------------------------------------------------------------------- */
/* ARGSUSED */
/* PROTO */
void
debug_PasteIt(unsigned char *data, int nitems)
{
/* TODO */
}

/* ------------------------------------------------------------------------- */
/* PROTO */
void
debug_colors(void)
{
    int             color;
    char           *name[] =
    {"fg", "bg",
     "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};

    fprintf(stderr, "Color ( ");
    if (rstyle & RS_RVid)
	fprintf(stderr, "rvid ");
    if (rstyle & RS_Bold)
	fprintf(stderr, "bold ");
    if (rstyle & RS_Blink)
	fprintf(stderr, "blink ");
    if (rstyle & RS_Uline)
	fprintf(stderr, "uline ");
    fprintf(stderr, "): ");

    color = GET_FGCOLOR(rstyle);
#ifndef NO_BRIGHTCOLOR
    if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
	color -= (minBrightCOLOR - minCOLOR);
	fprintf(stderr, "bright ");
    }
#endif
    fprintf(stderr, "%s on ", name[color]);

    color = GET_BGCOLOR(rstyle);
#ifndef NO_BRIGHTCOLOR
    if (color >= minBrightCOLOR && color <= maxBrightCOLOR) {
	color -= (minBrightCOLOR - minCOLOR);
	fprintf(stderr, "bright ");
    }
#endif
    fprintf(stderr, "%s\n", name[color]);
}

#ifdef USE_XIM
/* PROTO */
void
setPosition(XPoint *pos)
{
  XWindowAttributes xwa ;

  XGetWindowAttributes( Xdisplay, TermWin.vt, &xwa ) ;
  pos->x = Col2Pixel   ( screen.cur.col ) + xwa.x;
  pos->y = Height2Pixel((screen.cur.row + 1)) + xwa.y;
  return ;
}
#endif

