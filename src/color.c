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
#include "error.h"
#include "memory.h"
#include "video.h"


unsigned char texture_pal[768];

unsigned char color_lookup[256];

int dith;


unsigned char *color_dark;
static unsigned char dark_v[16*256],dark_t[16*256];


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
      color_dark=dark_t;
   }
   else
   {
      SetDith(0);
      SetGammaPal(video.pal);
      color_dark=dark_v;
   }
}


static void UpdateColors(void)
{
   int i,j;
   float v;

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

   dith_pal=video.pal;
   for (i=0;i<16;i++)
   {
      v=1-(float)i/16.0;
      for (j=0;j<256;j++)
      {
         dark_v[i*256+j]=Dith(
            video.pal[j*3+0]*v,
            video.pal[j*3+1]*v,
            video.pal[j*3+2]*v);
      }
   }

   dith_pal=texture_pal;
   for (i=0;i<16;i++)
   {
      v=1-(float)i/16.0;
      for (j=0;j<256;j++)
      {
         dark_t[i*256+j]=Dith(
            texture_pal[j*3+0]*v,
            texture_pal[j*3+1]*v,
            texture_pal[j*3+2]*v);
      }
   }
}


void SetTexturePal(unsigned char *pal)
{
   if (!memcmp(pal,texture_pal,768))
      return;    // same palette, do nothing
   memcpy(texture_pal,pal,768);

   if (!color_init)
      return;

   UpdateColors();
}


void InitColor(void)
{
   int i;
   bitmap_t *b;

   dith_pal=texture_pal;
   for (i=0;i<MAX_NUM_BITMAPS;i++)
   {
      if (!bitmap[i].used)
         continue;

      b=&bitmap[i];

      b->qdata=b->data;
      b->tdata=Q_malloc(b->size.x*b->size.y);
      if (!b->tdata)
      {
         Abort("InitColor","Out of memory!");
      }
   }

   UpdateColors();
   color_dark=dark_v;
   color_init=1;
}


int AddColor(float rf,float gf,float bf,int update)
{
   int r,g,b;
   int i,j;
   float v;

   r=rf*64;
   g=gf*64;
   b=bf*64;
   if (r>63) r=63;
   if (r<0) r=0;
   if (g>63) g=63;
   if (g<0) g=0;
   if (b>63) b=63;
   if (b<0) b=0;

   for (i=0;i<256;i++)
   {
      if ((video.pal[i*3+0]==r) &&
          (video.pal[i*3+1]==g) &&
          (video.pal[i*3+2]==b))
      {
         return i;
      }
   }
   for (i=176;i<255;i++)
   {
      if ((!video.pal[i*3+0]) &&
          (!video.pal[i*3+1]) &&
          (!video.pal[i*3+2]))
      {
         video.pal[i*3+0]=r;
         video.pal[i*3+1]=g;
         video.pal[i*3+2]=b;

         if (update) SetGammaPal(video.pal);

         dith_pal=texture_pal;
         color_lookup[i]=Dith(r,g,b);

         dith_pal=video.pal;
         for (j=0;j<16;j++)
         {
            v=1-(float)j/16.0;
            dark_v[j*256+i]=Dith(v*r,v*g,v*b);
         }

         return i;
      }
   }
   dith_pal=video.pal;
   i=Dith(r,g,b);
   return i;
}

