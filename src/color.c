/*
color.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "color.h"

#include "display.h"
#include "memory.h"
#include "video.h"


unsigned char texture_pal[768];

unsigned char color_lookup[256];

int dith;


static int color_init=0;


static unsigned char *dith_pal;

static int Dith(int r,int g,int b)
{
   int best;
   int bval;
   int i;
   unsigned char *c;
   int val;

   bval=20000000;
   best=0;
   for (i=0,c=dith_pal;i<256;i++,c+=3)
   {
      val=(r-c[0])*(r-c[0])+
          (g-c[1])*(g-c[1])+
          (b-c[2])*(b-c[2]);
      if (!val)
         return i;
      if (val<bval)
         best=i,bval=val;
   }
   return best;
}

static void DithBitmap(bitmap_t *b)
{
   int i,j;
   unsigned char *c,*d;

   for (c=b->qdata,d=b->tdata,i=b->size.x*b->size.y;i;i--,c++,d++)
   {
      j=*c * 3;
      *d=Dith(video.pal[j+0],video.pal[j+1],video.pal[j+2]);
   }
}


static void SetDith(int ndith)
{
   int i;
   bitmap_t *b;

   dith=ndith;
   if (dith)
   {
      for (b=bitmap,i=0;i<MAX_NUM_BITMAPS;i++,b++)
         b->data=b->tdata;
   }
   else
   {
      for (b=bitmap,i=0;i<MAX_NUM_BITMAPS;i++,b++)
         b->data=b->qdata;
   }
}


void SetPal(int pal)
{
   if (pal==PAL_TEXTURE)
   {
      SetDith(1);
      SetGammaPal(texture_pal);
   }
   else
   {
      SetDith(0);
      SetGammaPal(video.pal);
   }
}


void SetTexturePal(unsigned char *pal)
{
   int i,j;

   if (!memcmp(pal,texture_pal,768))
      return;    // same palette, do nothing
   memcpy(texture_pal,pal,768);

   if (!color_init)
      return;

   dith_pal=texture_pal;
   for (i=0;i<MAX_NUM_BITMAPS;i++)
   {
      if (!bitmap[i].used)
         continue;

      DithBitmap(&bitmap[i]);
   }

   for (i=0,j=0;i<256;i++,j+=3)
   {
      color_lookup[i]=Dith(video.pal[j+0],video.pal[j+1],video.pal[j+2]);
   }
}


void InitColor(void)
{
   int i,j;
   bitmap_t *b;

   dith_pal=texture_pal;
   for (i=0;i<MAX_NUM_BITMAPS;i++)
   {
      if (!bitmap[i].used)
         continue;

      b=&bitmap[i];

      b->qdata=b->data;
      b->tdata=Q_malloc(b->size.x*b->size.y);
      DithBitmap(b);
   }

   for (i=0,j=0;i<256;i++,j+=3)
   {
      color_lookup[i]=Dith(video.pal[j+0],video.pal[j+1],video.pal[j+2]);
   }
   color_init=1;
}

