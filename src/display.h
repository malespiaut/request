/*
display.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef DISPLAY_H
#define DISPLAY_H

#define  MAX_NUM_BITMAPS    60

typedef struct bitmap_s
{
	char   name[40];
	box_t  size;
   int used;
	unsigned char *data;

   unsigned char *qdata; // bitmap with Quest's palette
   unsigned char *tdata; // bitmap with texture palette, generated
} bitmap_t;

/* Bitmaps */
extern int      num_bitmaps;
extern bitmap_t bitmap[MAX_NUM_BITMAPS];


bitmap_t *FindBitmap(const char *name);

int DrawBitmap(const char *name, int x, int y);
int DrawBitmapP(bitmap_t *bm, int x, int y);

int InitDisplay(void);
void DisposeDisplay(void);

void InitMapDisplay(void);
void ClearViewport(int vport_num);
void UpdateMap(void);

#endif

