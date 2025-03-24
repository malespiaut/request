/*
color.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef COLOR_H
#define COLOR_H

#define PAL_QUEST 0
#define PAL_TEXTURE 1

extern unsigned char texture_pal[768];
extern unsigned char color_lookup[256];

#define COL_DARK(col, lev) (color_dark[((int)lev) * 256 + ((int)col)])
extern unsigned char* color_dark;

extern int dith;

#define GetColor(x) (dith ? color_lookup[x] : x)
#define GetColor2(x) (color_lookup[x])

void SetPal(int pal);

void InitColor(void);

void SetTexturePal(unsigned char* pal);

int AddColor(float rf, float gf, float bf, int update);

#endif
