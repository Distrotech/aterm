/*--------------------------------*-C-*---------------------------------*
 * File:        thai.c
 *----------------------------------------------------------------------*
 *
 * All portions of code are copyright by their respective author/s.
 * Copyright (C) 2000,2001 Teepanis Chachiyo <teepanis@physics.purdue.edu>
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

#include "rxvt.h"
#include "thai.h"

#ifdef THAI

#if defined (NO_BRIGHTCOLOR) || defined (VERYBOLD)
# define MONO_BOLD(x)   ((x) & (RS_Bold|RS_Blink))
#else
# define MONO_BOLD(x)   (((x) & RS_Bold) && fore == Color_fg)
#endif

/*
 * ThaiIsMiddleLineCh returns TRUE if character 'ch'
 * is to be displayed in the middle line.
 */
/* PROTO */
int
ThaiIsMiddleLineCh(char ch)
{
	if(thai_map[(unsigned char)ch]==0)
		return 1;
	else
		return 0;
}



/*
 * ThaiPixel2Col converts x-y coordiates in pixel
 * into byte offset that store character data.
 * This function replaces Pixel2Col in orinal
 * ATERM.
 */
/* PROTO */
int
ThaiPixel2Col(int x, int y)
{
	int row;             /* row storing data */
	int i;               /* byte offset index */
	int xpixel;          /* dummy x pixel */
	char *str;           /* drawn_text pointer */
	XFontStruct *wf;     /* font */

	/* locate buffer storing data */
	row = Pixel2Row(y);
	MAX_IT(row, 0);
	MIN_IT(row, TermWin.nrow - 1);
	str = drawn_text[row];

	/* access to font structure */
	wf =  TermWin.font;

	/* increase byte offset until we get to target x */
	i = 0;
	xpixel = CharWidth(wf,str[0]);
	x -= TermWin_internalBorder;
	while(xpixel <= x && i < TermWin.ncol){
		i++;
		xpixel += CharWidth(wf,str[i]); /* wf->per_char[char_num].width;  */
	}

	MAX_IT(i, 0);
	MIN_IT(i, TermWin.ncol);
	return i;
}


/*
 * ThaiCol2Pixel returns x cooridate of Thai strings.
 * It also adjusts the to the upper/lowwer Thai character.
 * This replaces Col2Pixel and is called repeatedly so
 * I use 'static' variable to speed it up.
 */
/* PROTO */
int
ThaiCol2Pixel(int col, char *start)
/*
 * TODO: Still have some problem with scaled font
 */
{
    static int i=0;                /* index */
	static char *laststart=NULL;   /* strings from last call */
	static unsigned int x=0;       /* x coordinate for proportional font */
	static XFontStruct *wf;        /* font */

	/* new string, so restart from the beginning */
	if(laststart!=start || col < i){
		i=0;
		x=TermWin_internalBorder;
		laststart=start;
		wf = TermWin.font;
	}

	/* old string, continue counting */
    for(; i < col; i++){
		x+= CharWidth(wf,start[i]);
    }

	return x;
}


/* PROTO */
int
ThaiUpdateMap2(text_t *stp, text_t *dtp, rend_t *srp, rend_t *drp, char *map, int len)
{
	int stpx, dtpx;  /* xpixel position of screen & drawntext */
	int stpi, dtpi;
	int char_num;
	XFontStruct *wf;

	wf = TermWin.font;
	stpx = dtpx = 0;
	stpi = dtpi = 0;

	for(stpi = 0; stpi < len;stpi++,
							 char_num = (unsigned char)stp[stpi]-wf->min_char_or_byte2,
							 stpx += wf->per_char[char_num].width){

		while(dtpx < stpx && dtpi < len-1){
			dtpi++;
			char_num = (unsigned char)dtp[dtpi]-wf->min_char_or_byte2;
			dtpx+= wf->per_char[char_num].width;
		}

		map[stpi] = (stp[stpi]==dtp[dtpi] && srp[stpi]==drp[dtpi]) ? 0 : 1;

	}

	return 0;
}

/*
 * ThaiUpdateMap decides which character should be redraw on
 * screen. It returns the number of characters to update.
 * The algorithm is follow:
 *    (1) At current Thai column, check if all characters at
 *        this column is the same
 *    (2) If yes, no need to update
 *    (3) If no, update entire Thai column
 *    (4) go to next Thai column, --> (1)
 */
/* PROTO */
int
ThaiUpdateMap(text_t *stp, text_t *dtp, rend_t *srp, rend_t *drp, char *map, int len)
{
	int stp_col=0, dtp_col=0;    /* index */
	int stp_lev=0, dtp_lev=0;    /* # of characters at the column */
	int i=0,match=0;             /* matching flag */
	int lastbold=0;              /* BOLD_OVERSTRIKE feature */
	int nupdate;                 /* # chars to update */

	nupdate = len;
	/* update by default */
	for(i=0;i<len;i++) map[i]=1;

	/* Bug Fixed: 12/27/2000  */
	/* sync the first column for both stp and dtp */
	for(stp_col=0; stp_col < len && thai_map[(unsigned char)stp[stp_col]]!=0; stp_col++);
	for(dtp_col=0; dtp_col < len && thai_map[(unsigned char)dtp[dtp_col]]!=0; dtp_col++);

	while(stp_col < len && dtp_col < len){

		/* check if identical column */
		match = 0;
  	  	if(stp_lev==dtp_lev){
	        for(i=0;   i < stp_lev
					&& stp[stp_col+i]==dtp[dtp_col+i]   /* same chars */
					&& (srp[stp_col+i]==drp[dtp_col+i]  /* either same ATTR */
                        || (stp[stp_col+i]==' '         /* or space with same background */
                            && GET_BGATTR(srp[stp_col+i])==GET_BGATTR(drp[dtp_col+i]))); i++);


			/* set matching flag */
	        if(i==stp_lev) match = 1;
	    }

	    /* if match don't update */
	   	 if(match){
			/*
			 * previous Bold character can leave excess dot on screen
             * so we must update char to clean up the mess
             */
			if(!lastbold)
	    	    for(i=0;i < stp_lev; i++){
  		          	map[stp_col+i] = 0;
					nupdate--;
				}

			lastbold = 0;
   	     }else    /* set bold flag */
			lastbold = MONO_BOLD(srp[stp_col]);

        stp_col += stp_lev;
        dtp_col += dtp_lev;

		/* go to next Thai column and also fing # of levels too */
		for(stp_lev=1; thai_map[(unsigned char)stp[stp_col+stp_lev]]!=0
						&& (stp_col+stp_lev)<len; stp_lev++);

	    for(dtp_lev=1; thai_map[(unsigned char)dtp[dtp_col+dtp_lev]]!=0
    	                && (dtp_col+dtp_lev)<len; dtp_lev++);

	}

	/* in drawn_text buffer is full, won't update */
	while(stp_col < dtp_col){
		/*
		 * Bug fixed: only skip update only its a blankspace.
		 * (12/27/2000)
		 */
		if(stp[stp_col] == ' '){
			map[stp_col]=0;
			nupdate--;
		}
		stp_col++;
	}

	return nupdate;
}

/*
 * Thai_CursorArea returns map of area around cursor.
 * We need this to take care of tone sign updating in
 * Thai language.
 */
/* PROTO */
int
Thai_CursorArea(char *stp, char *map, int len, int col)
{
	int i,j;

	/* not update by default */
	for(i=0;i<len;i++) map[i]=0;

	return 0;

	/* forward */
	i=col;
	for(j=0;j<3;j++)
		for(map[i++]=1; i<len && thai_map[(unsigned char)stp[i]]!=0;i++)
			map[i]=1;

	/* backward */
	i=col;
	for(j=0;j<3;j++)
		for(map[i--]=1; i >=0 && thai_map[(unsigned char)stp[i]]!=0;i--)
			map[i]=1;

	return 0;
}


/*
 * Thai_ColMaxPaint finds the truncated spaces needed to
 * paint strings str on screen.
 */
/* PROTO */
int
Thai_ColMaxPaint(text_t *str, int len)
{
	int i, col=0;

	/* truncate end space */
	for(i = len-1; i>=0 && str[i]==' '; i--);

	for(; i>=0; i--)
		if(ThaiIsMiddleLineCh(str[i])) col++;

	return col;
}

#endif /* #ifdef THAI */
