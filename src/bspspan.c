/*
bspspan.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "bspspan.h"
#include "bsptree.h"

#include "3d.h"
#include "bsp.h"
#include "bsplight.h"
#include "color.h"
#include "editface.h"
#include "error.h"
#include "game.h"
#include "geom.h"
#include "memory.h"
#include "message.h"
#include "quest.h"
#include "status.h"
#include "tex.h"
#include "times.h"
#include "video.h"


typedef struct vec3_ds  // this is used for the actual drawing
{
   float x,y,z;
   int   sx,sy;
   int   tx,ty;    // texture coordinates, face dependent
   float w;        // 1/z (perspective correct texture mapping stuff)
   int   outcode;
   float rx,ry,rz; // 'real' untransformed coordinates, for lighting
} vec3_dt;


#define MY_NEXT_LEFT(x,n)  ((x)==0)?((n)-1):((x)-1)
#define MY_NEXT_RIGHT(x,n) ((x)==(n)-1)?(0):((x)+1)


//#define PROFILE


typedef struct
{
   int x1,x2;

   int next;
} span_t;


#define INIT_SPANS 5000
#define INC_SPANS 1000

/*
The spans array will start at INIT_SPANS spans, and will grow dynamically
by INC_SPANS at a time, if necessary. I haven't seen a frame require
more than ~3000 spans (at 1024x768), so starting at 5000 should be
enough, as it'll grow if needed anyway. Note that x-resolution doesn't
affect the number of spans, but y-resolution does.

The spanstart array will be checked at the beginning of the frame and
if it isn't big enough, it'll be enlarged to the right size.
*/


static bsp_plane_t curplane;


static span_t *spans=NULL;
static int *spanstart=NULL;

static int spans_size=0;
static int sstart_size=0;

static int curspan;


/* Debugging stuff, used in leak.c */
int D_BSPSpanSize(void)
{
   return spans_size*sizeof(span_t)+sstart_size*sizeof(int);
}


#ifdef PROFILE
static int maxspan;

static int pix_span;
static int pix_draw;
#endif


static int g_x1,g_dx;
static int g_y;


// texture mapping stuff
static texture_t *tex;
static int fs;     // 1 if any part of this span has been drawn


// texture mapped, correct
static float u1,v1,w1;
static float u2,v2,w2;
static float du,dv,dw;


// ligted preview
static float rx1,ry1,rz1;
static float rx2,ry2,rz2;
static float drx,dry,drz;

static int face_drawn;



#define LIGHT_LEVELS 16

static int calc_lightmap=0;
static unsigned char lightmap[LIGHT_LEVELS*256];
static unsigned char dith_pal[768];

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


static void (*PDrawSpan)(char *dest,int x1,int x2);


static int error;

static void AddSpan(int y,int x1,int x2,unsigned char *sl)
{
   int i;
   int prev;
   span_t *sp,*n1,*n2;

   int first;

   int cx;


/*   if (x2<x1)
      return; // This is checked for in the SpanBSPFace... functions. */

   g_y=y;

   if (curspan==spans_size)
   {
      sp=Q_realloc(spans,sizeof(span_t)*(spans_size+INC_SPANS));
      if (!sp)
      {
         error=1;
         return;
      }
      spans=sp;
      spans_size+=INC_SPANS;
   }

   g_x1=x1;
   fs=1;
#ifdef PROFILE
   pix_span+=x2-x1+1;
#endif

   if (spanstart[y]==-1)
   {
      sp=&spans[curspan];
      sp->x1=x1;
      sp->x2=x2;
      sp->next=-1;
      spanstart[y]=curspan++;
      PDrawSpan(sl,x1,x2);
      return;
   }

   i=spanstart[y];
   prev=-1;
   first=-2;
   while (1)
   {
      sp=&spans[i];
      if ((sp->x1<=x1) && (sp->x2>=x2))
         return;

      if (sp->x2>=x1)
      {
         if (sp->x1>x2)
         {
            n1=&spans[curspan];
            n1->x1=x1;
            n1->x2=x2;
            n1->next=i;
            if (prev==-1)
               spanstart[y]=curspan++;
            else
               spans[prev].next=curspan++;
            PDrawSpan(sl,x1,x2);
            return;
         }
         first=prev;
         break;
      }

      prev=i;
      i=sp->next;
      if (i==-1)
         break;
   }

   if (first!=-2)
   {
      if (first!=-1)
         first=spans[first].next;
      else
         first=spanstart[y];
   
      n1=&spans[first];

      i=first;

      cx=x1;
      while (1)
      {
         n1=&spans[i];
         if (n1->x1<=cx)
         {
            if (n1->x2>=x2)
               goto merge_ndraw;
            cx=n1->x2+1;
         }
         else
         if (n1->x1<=x2)
         {
            PDrawSpan(sl,cx,n1->x1-1);
            if (n1->x2>=x2)
               goto merge_ndraw;
            cx=n1->x2+1;
         }
         else
         {
            goto merge;
         }

         if (n1->next==-1)
            break;
         if (spans[n1->next].x1>x2)
            goto merge;
         i=n1->next;
      }

merge:
      PDrawSpan(sl,cx,x2);
merge_ndraw:
      n2=&spans[first];
      if (x1<n2->x1)
         n2->x1=x1;
      if (x2>n2->x2)
         n2->x2=x2;
      if (n1->x2>n2->x2)
         n2->x2=n1->x2;

      n2->next=n1->next;

      return;
   }
   else
   {
      PDrawSpan(sl,x1,x2);
      n1=&spans[curspan];
      n1->x1=x1;
      n1->x2=x2;
      if (prev==-1)
      {
         n1->next=spanstart[y];
         spanstart[y]=curspan++;
      }
      else
      {
         n1->next=spans[prev].next;
         spans[prev].next=curspan++;
      }
      return;
   }
}


/**************************************************************************
   Lighted preview.
**************************************************************************/
static void DrawSpanLight(char *dest,int x1,int x2)
{
   float u,v,w;
   vec3_t r,r2;
   float temp;
   int i,j;
   char *d;
   int tx,ty;
   int l;


#ifdef PROFILE
   pix_draw+=x2-x1+1;
#endif
   if (fs)
   {
      fs=0;
      temp=1/(float)g_dx;
      du=(u2-u1)*temp;
      dv=(v2-v1)*temp;
      dw=(w2-w1)*temp;

      drx=(rx2-rx1)*temp;
      dry=(ry2-ry1)*temp;
      drz=(rz2-rz1)*temp;

      if (!face_drawn)
      {
         face_drawn=1;
         FaceLight(curplane.normal,curplane.dist);
      }
   }

   if (x1==g_x1)
   {
      u=u1;
      v=v1;
      w=w1;

      r.x=rx1;
      r.y=ry1;
      r.z=rz1;
   }
   else
   {
      i=x1-g_x1;
      u=u1+du*i;
      v=v1+dv*i;
      w=w1+dw*i;

      r.x=rx1+drx*i;
      r.y=ry1+dry*i;
      r.z=rz1+drz*i;
   }

   i=x2-x1+1;
   d=&dest[x1];
   for (;i;i--)
   {
      temp=1/w;

      tx=u*temp;
      while (tx>=tex->dsx) tx-=tex->dsx;
      while (tx<0) tx+=tex->dsx;

      ty=v*temp;
      while (ty>=tex->dsy) ty-=tex->dsy;
      while (ty<0) ty+=tex->dsy;

      r2.x=r.x*temp;
      r2.y=r.y*temp;
      r2.z=r.z*temp;

      j=tex->data[tx+ty*tex->dsx];

      if ((j>=240) && (Game.light.fullbright))
         *d++=j;
      else
      {
//         j*=3;
         l=GetLight(r2)*LIGHT_LEVELS;
         if (l>LIGHT_LEVELS-1) l=LIGHT_LEVELS-1;
         *d++=lightmap[l*256+j];
/*         *d++=Dith(dith_pal[j+0]*cr,
                dith_pal[j+1]*cg,
                dith_pal[j+2]*cb);*/
      }

      u+=du;
      v+=dv;
      w+=dw;

      r.x+=drx;
      r.y+=dry;
      r.z+=drz;
   }
}

static void SpanBSPFaceLight(vec3_dt *pts, int numpts, texture_t *t)
{
   float srfloat=0,slfloat=0;
   float lslope=0,rslope=0;

   int   y,ly,ry;
   int   slint=0,srint=0,swidth;
   int   i;
   int   li,ri,nli,nri;
   int   dx,dy;
   int   top,rem;
   char *scanline;

   float temp;

// for texture mapping
   float lw=0,ldw=0;
   float rw=0,rdw=0;

   float ltx=0,lty=0;
   float rtx=0,rty=0;

   float ldx=0,ldy=0;
   float rdx=0,rdy=0;


   float lrx=0,lry=0,lrz=0;
   float rrx=0,rry=0,rrz=0;
   float ldrx=0,ldry=0,ldrz=0;
   float rdrx=0,rdry=0,rdrz=0;



   if (!t) return;
   if (numpts<3) return;

   tex=t;

   ly=(pts[0].tx/t->dsx)*t->dsx;
   ry=(pts[0].ty/t->dsy)*t->dsy;
   for (i=0;i<numpts;i++)
   {
      pts[i].w=1/pts[i].z;
      pts[i].tx=(pts[i].tx-ly)*pts[i].w;
      pts[i].ty=(pts[i].ty-ry)*pts[i].w;

      pts[i].rx*=pts[i].w;
      pts[i].ry*=pts[i].w;
      pts[i].rz*=pts[i].w;
   }

   y = pts[0].sy;
   top = 0;
   for (i=1;i<numpts;i++)
   {
      if (pts[i].sy < y)
      {
         y = pts[i].sy;
         top = i;
      }
   }

   nli=nri=li=ri=top;
   swidth = video.ScreenWidth;
   scanline = video.ScreenBuffer + y*swidth;
   ly=ry=y-1;
   rem=numpts;
   while (rem>0)
   {
      if (ly<=y && rem>0)
      { /*advance left edge?*/
         while ((ly<=y) && (rem>0))
         {
            rem--;
            li=nli;
            nli=MY_NEXT_LEFT(li,numpts);
            ly=pts[nli].sy;
         }
         if (ly<=y) return;
         dx = (pts[nli].sx - pts[li].sx);
         dy = (pts[nli].sy - pts[li].sy);
         if ((dx!=0)&&(dy!=0))
            lslope = ((float)(dx))/((float)(dy));
         else
            lslope=0;
         slint = pts[li].sx;
         slfloat = (float)slint;


         if (dy)
         {
            temp=1/(float)dy;
            ldx=(float)(pts[nli].tx-pts[li].tx)*temp;
            ldy=(float)(pts[nli].ty-pts[li].ty)*temp;
            ldw=(pts[nli].w-pts[li].w)*temp;

            ldrx=(pts[nli].rx-pts[li].rx)*temp;
            ldry=(pts[nli].ry-pts[li].ry)*temp;
            ldrz=(pts[nli].rz-pts[li].rz)*temp;
         }
         else
         {
            ldx=ldy=ldw=ldrx=ldry=ldrz=0;
         }

         ltx=pts[li].tx;
         lty=pts[li].ty;
         lw=pts[li].w;

         lrx=pts[li].rx;
         lry=pts[li].ry;
         lrz=pts[li].rz;
      }
      if (ry<=y && rem>0)
      { /*advance right edge?*/
         while ((ry<=y) && (rem>0))
         { /*advance left edge?*/
            rem--;
            ri=nri;
            nri=MY_NEXT_RIGHT(ri,numpts);
            ry=pts[nri].sy;
         }
         if (ry<=y) return;
         dx = (pts[nri].sx - pts[ri].sx);
         dy = (pts[nri].sy - pts[ri].sy);
         if ((dx!=0)&&(dy!=0))
            rslope = ((float)(dx))/((float)(dy));
         else
            rslope=0;
         srint = pts[ri].sx;
         srfloat = (float)srint;


         if (dy)
         {
            temp=1/(float)dy;
            rdx=(float)(pts[nri].tx-pts[ri].tx)*temp;
            rdy=(float)(pts[nri].ty-pts[ri].ty)*temp;
            rdw=(pts[nri].w-pts[ri].w)*temp;

            rdrx=(pts[nri].rx-pts[ri].rx)*temp;
            rdry=(pts[nri].ry-pts[ri].ry)*temp;
            rdrz=(pts[nri].rz-pts[ri].rz)*temp;
         }
         else
         {
            rdx=rdy=rdw=0;
         }

         rtx=pts[ri].tx;
         rty=pts[ri].ty;
         rw=pts[ri].w;

         rrx=pts[ri].rx;
         rry=pts[ri].ry;
         rrz=pts[ri].rz;
      }

      while (y<ly && y<ry)
      {
         if (srint<slint)
         {
            dx=slint-srint;
            if (dx)
            {
               du=dv=dw=0;
               u1=rtx;
               v1=rty;
               w1=rw;
               u2=ltx;
               v2=lty;
               w2=lw;
               g_dx=dx;

               rx1=rrx;
               ry1=rry;
               rz1=rrz;
               rx2=lrx;
               ry2=lry;
               rz2=lrz;

               AddSpan(y,srint,slint,scanline);
               if (!fs)
                  RefreshPart(srint,y,slint,y);
            }
         }
         else
         {
            dx=srint-slint;
            if (dx)
            {
               du=dv=dw=0;
               u1=ltx;
               v1=lty;
               w1=lw;
               u2=rtx;
               v2=rty;
               w2=rw;
               g_dx=dx;

               rx1=lrx;
               ry1=lry;
               rz1=lrz;
               rx2=rrx;
               ry2=rry;
               rz2=rrz;

               AddSpan(y,slint,srint,scanline);
               if (!fs)
               {
                  RefreshPart(slint,y,srint,y);
               }
            }
         }

         scanline += swidth;
         y++;
         slfloat += lslope;
         slint = (int)slfloat;
         srfloat += rslope;
         srint = (int)srfloat;

         ltx+=ldx;
         lty+=ldy;
         lw+=ldw;
         rtx+=rdx;
         rty+=rdy;
         rw+=rdw;

         lrx+=ldrx;
         lry+=ldry;
         lrz+=ldrz;
         rrx+=rdrx;
         rry+=rdry;
         rrz+=rdrz;
      }
   }
}


/**************************************************************************
   Floating point perspective correct texture mapped.
**************************************************************************/
static void DrawSpanTC(char *dest,int x1,int x2)
{
   float u,v,w;
   float temp;
   int i;
   char *d;
   int tx,ty;


#ifdef PROFILE
   pix_draw+=x2-x1+1;
#endif
   if (fs)
   {
      fs=0;
      temp=1/(float)g_dx;
      du=(u2-u1)*temp;
      dv=(v2-v1)*temp;
      dw=(w2-w1)*temp;
   }

   if (x1==g_x1)
   {
      u=u1;
      v=v1;
      w=w1;
   }
   else
   {
      i=x1-g_x1;
      u=u1+du*i;
      v=v1+dv*i;
      w=w1+dw*i;
   }

   i=x2-x1+1;
   d=&dest[x1];
   for (;i;i--)
   {
      temp=1/w;

      tx=u*temp;
      while (tx>=tex->dsx) tx-=tex->dsx;
      while (tx<0) tx+=tex->dsx;

      ty=v*temp;
      while (ty>=tex->dsy) ty-=tex->dsy;
      while (ty<0) ty+=tex->dsy;

      *d++=tex->data[tx+ty*tex->dsx];

      u+=du;
      v+=dv;
      w+=dw;
   }
}

static void SpanBSPFaceTC(vec3_dt *pts, int numpts, texture_t *t)
{
   float srfloat=0,slfloat=0;
   float lslope=0,rslope=0;

   int   y,ly,ry;
   int   slint=0,srint=0,swidth;
   int   i;
   int   li,ri,nli,nri;
   int   dx,dy;
   int   top,rem;
   char *scanline;

   float temp;

// for texture mapping
   float lw=0,ldw=0;
   float rw=0,rdw=0;

   float ltx=0,lty=0;
   float rtx=0,rty=0;

   float ldx=0,ldy=0;
   float rdx=0,rdy=0;


   if (!t) return;
   if (numpts<3) return;

   tex=t;

   ly=(pts[0].tx/t->dsx)*t->dsx;
   ry=(pts[0].ty/t->dsy)*t->dsy;
   for (i=0;i<numpts;i++)
   {
      pts[i].w=1/pts[i].z;
      pts[i].tx=(pts[i].tx-ly)*pts[i].w;
      pts[i].ty=(pts[i].ty-ry)*pts[i].w;
   }

   y = pts[0].sy;
   top = 0;
   for (i=1;i<numpts;i++)
   {
      if (pts[i].sy < y)
      {
         y = pts[i].sy;
         top = i;
      }
   }

   nli=nri=li=ri=top;
   swidth = video.ScreenWidth;
   scanline = video.ScreenBuffer + y*swidth;
   ly=ry=y-1;
   rem=numpts;
   while (rem>0)
   {
      if (ly<=y && rem>0)
      { /*advance left edge?*/
         while ((ly<=y) && (rem>0))
         {
            rem--;
            li=nli;
            nli=MY_NEXT_LEFT(li,numpts);
            ly=pts[nli].sy;
         }
         if (ly<=y) return;
         dx = (pts[nli].sx - pts[li].sx);
         dy = (pts[nli].sy - pts[li].sy);
         if ((dx!=0)&&(dy!=0))
            lslope = ((float)(dx))/((float)(dy));
         else
            lslope=0;
         slint = pts[li].sx;
         slfloat = (float)slint;


         if (dy)
         {
            temp=1/(float)dy;
            ldx=(float)(pts[nli].tx-pts[li].tx)*temp;
            ldy=(float)(pts[nli].ty-pts[li].ty)*temp;
            ldw=(pts[nli].w-pts[li].w)*temp;
         }
         else
         {
            ldx=ldy=ldw=0;
         }

         ltx=pts[li].tx;
         lty=pts[li].ty;
         lw=pts[li].w;
      }
      if (ry<=y && rem>0)
      { /*advance right edge?*/
         while ((ry<=y) && (rem>0))
         { /*advance left edge?*/
            rem--;
            ri=nri;
            nri=MY_NEXT_RIGHT(ri,numpts);
            ry=pts[nri].sy;
         }
         if (ry<=y) return;
         dx = (pts[nri].sx - pts[ri].sx);
         dy = (pts[nri].sy - pts[ri].sy);
         if ((dx!=0)&&(dy!=0))
            rslope = ((float)(dx))/((float)(dy));
         else
            rslope=0;
         srint = pts[ri].sx;
         srfloat = (float)srint;


         if (dy)
         {
            temp=1/(float)dy;
            rdx=(float)(pts[nri].tx-pts[ri].tx)*temp;
            rdy=(float)(pts[nri].ty-pts[ri].ty)*temp;
            rdw=(pts[nri].w-pts[ri].w)*temp;
         }
         else
         {
            rdx=rdy=rdw=0;
         }

         rtx=pts[ri].tx;
         rty=pts[ri].ty;
         rw=pts[ri].w;
      }

      while (y<ly && y<ry)
      {
         if (srint<slint)
         {
            dx=slint-srint;
            if (dx)
            {
               du=dv=dw=0;
               u1=rtx;
               v1=rty;
               w1=rw;
               u2=ltx;
               v2=lty;
               w2=lw;
               g_dx=dx;

               AddSpan(y,srint,slint,scanline);
            }
         }
         else
         {
            dx=srint-slint;
            if (dx)
            {
               du=dv=dw=0;
               u1=ltx;
               v1=lty;
               w1=lw;
               u2=rtx;
               v2=rty;
               w2=rw;
               g_dx=dx;

               AddSpan(y,slint,srint,scanline);
            }
         }

         scanline += swidth;
         y++;
         slfloat += lslope;
         slint = (int)slfloat;
         srfloat += rslope;
         srint = (int)srfloat;

         ltx+=ldx;
         lty+=ldy;
         lw+=ldw;
         rtx+=rdx;
         rty+=rdy;
         rw+=rdw;
      }
   }
}


/**************************************************************************
   Texture mapped, linear.
**************************************************************************/
static int g_ltx,g_lty;
static int g_rtx,g_rty;

static int g_tdx,g_tdy,g_tdyn;
static int g_tsx,g_tsy,g_itx;

static void DrawSpanT(char *dest,int x1,int x2)
{
   int i;
   int tx,ty;
   int tdyt;
   unsigned char *d;
   int dx;

#ifdef PROFILE
   pix_draw+=x2-x1+1;
#endif

   dx=x2-x1+1;
   if (fs)
   {
      fs=0;
      g_tdx=(g_rtx-g_ltx)/g_dx;

      tdyt=(g_rty-g_lty+65535)>>16;
      g_tdy=tdyt/g_dx;
      g_tdyn=tdyt-g_tdy*g_dx;
      g_tdy*=tex->dsx;

      g_itx=tex->dsx;
      g_tsx=g_itx<<16;
      g_tsy=g_itx*tex->dsy;

      g_lty=((g_lty+65535)>>16)*g_itx;
   }

   if (x1==g_x1)
   {
      tx=g_ltx;
      ty=g_lty;
      tdyt=0;
   }
   else
   {
      i=x1-g_x1;
      tx=g_ltx+g_tdx*i;
      ty=g_lty+g_tdy*i;
      tdyt=g_tdyn*i;
      while (tdyt>0)
      {
         tdyt-=g_dx;
         ty+=tex->dsx;
      }
      while (tdyt<0)
      {
         tdyt+=g_dx;
         ty-=tex->dsx;
      }
   }

   d=&dest[x1];

   for (i=dx;i;i--)
   {
      while (tx<0) tx+=g_tsx;
      while (tx>=g_tsx) tx-=g_tsx;

      while (ty<0) ty+=g_tsy;
      while (ty>=g_tsy) ty-=g_tsy;

      *d++=tex->data[(tx>>16)+ty];

      tx+=g_tdx;

      ty+=g_tdy;
      tdyt+=g_tdyn;
      if (tdyt>=g_dx)
      {
         tdyt-=g_dx;
         ty+=g_itx;
      }
      if (tdyt<0)
      {
         tdyt+=g_dx;
         ty-=g_itx;
      }
   }
}

static void SpanBSPFaceT(vec3_dt *pts, int numpts, texture_t *t)
{
   int srfloat=0,slfloat=0,lslope=0,rslope=0;
   int y,ly,ry,swidth;
   int i,li,ri,nli,nri,dx,dy;
   int top,rem;
   char *scanline;

// for texture mapping
   int ltx=0,lty=0;
   int rtx=0,rty=0;

   int ldx=0,ldy=0;
   int rdx=0,rdy=0;


   if (!t) return;
   if (numpts<3) return;

   tex=t;

   y = pts[0].sy;
   top = 0;

   ly=(pts[0].tx/t->dsx)*t->dsx;
   ry=(pts[0].ty/t->dsy)*t->dsy;
   for (i=0;i<numpts;i++)
   {
      if (pts[i].sy < y)
      {
         y = pts[i].sy;
         top = i;
      }
      pts[i].tx-=ly;
      pts[i].ty-=ry;
   }

   nli=nri=li=ri=top;
   swidth = video.ScreenWidth;
   scanline = video.ScreenBuffer + y*swidth;
   ly=ry=y-1;
   rem=numpts;
   while (rem>0)
   {
      if (ly<=y && rem>0)
      { /*advance left edge?*/
         while ((ly<=y) && (rem>0))
         {
            rem--;
            li=nli;
            nli=MY_NEXT_LEFT(li,numpts);
            ly=pts[nli].sy;
         }
         if (ly<=y) return;
         dx = (pts[nli].sx - pts[li].sx);
         dy = (pts[nli].sy - pts[li].sy);
         if ((dx!=0)&&(dy!=0))
            lslope = (dx<<16)/dy;
         else
            lslope=0;
         slfloat = pts[li].sx*65536;


         if (dy)
         {
            ldx=((pts[nli].tx-pts[li].tx)<<16)/dy;
            ldy=((pts[nli].ty-pts[li].ty)<<16)/dy;
         }
         else
         {
            ldx=ldy=0;
         }

         ltx=pts[li].tx*65536;
         lty=pts[li].ty*65536;
      }
      if (ry<=y && rem>0)
      { /*advance right edge?*/
         while ((ry<=y) && (rem>0))
         {
            rem--;
            ri=nri;
            nri=MY_NEXT_RIGHT(ri,numpts);
            ry=pts[nri].sy;
         }
         if (ry<=y) return;
         dx = (pts[nri].sx - pts[ri].sx);
         dy = (pts[nri].sy - pts[ri].sy);
         if ((dx!=0)&&(dy!=0))
            rslope = (dx<<16)/dy;
         else
            rslope=0;
         srfloat = pts[ri].sx*65536;


         if (dy)
         {
            rdx=((pts[nri].tx-pts[ri].tx)<<16)/dy;
            rdy=((pts[nri].ty-pts[ri].ty)<<16)/dy;
         }
         else
         {
            rdx=rdy=0;
         }

         rtx=pts[ri].tx*65536;
         rty=pts[ri].ty*65536;
      }
      while (y<ly && y<ry)
      {
         if (srfloat<slfloat)
         {
            dx=(slfloat>>16)-(srfloat>>16);

            if (dx)
            {
               g_dx=dx;

               g_rtx=ltx;
               g_rty=lty;
               g_ltx=rtx;
               g_lty=rty;

               AddSpan(y,srfloat>>16,slfloat>>16,scanline);
            }
         }
         else
         {
            dx=(srfloat>>16)-(slfloat>>16);

            if (dx)
            {
               g_dx=dx;

               g_ltx=ltx;
               g_lty=lty;
               g_rtx=rtx;
               g_rty=rty;

               AddSpan(y,slfloat>>16,srfloat>>16,scanline);
            }
         }

         scanline += swidth;
         y++;

         slfloat += lslope;

         srfloat += rslope;

         ltx+=ldx;
         lty+=ldy;

         rtx+=rdx;
         rty+=rdy;
      }
   }
}


/**************************************************************************
   Single colored.
**************************************************************************/
static int col;

static void DrawSpan(char *dest,int x1,int x2)
{
#ifdef PROFILE
   pix_draw+=x2-x1+1;
#endif
   memset((void *)((int)dest+x1),col,x2-x1+1);
}

static void SpanBSPFace(vec3_dt *pts, int numpts, int color)
{
   int lslope=0,rslope=0;
   int slint=0,srint=0;
   int y,ly,ry;
   int li,ri,nli,nri;
   int dy;
   int rem;
   unsigned char *scanline;
   int swidth;

   col=color;

   y = pts[0].sy;
   ri = 0;
   for (rem=1;rem<numpts;rem++)
   {
      if (pts[rem].sy < y)
      {
         y = pts[rem].sy;
         ri = rem;
      }
   }

   swidth=video.ScreenWidth;
   scanline=video.ScreenBuffer+y*swidth;

   nli=nri=li=ri;
   ly=ry=y-1;
   rem=numpts;

   while (rem>0)
   {
      if (ly<=y)
      {
         while ((ly<=y) && (rem))
         {
            rem--;
            li=nli;
            nli=MY_NEXT_LEFT(li,numpts);
            ly=pts[nli].sy;
         }
         if (ly<=y) return;

         dy = (ly - pts[li].sy);
         if (dy)
            lslope = ((pts[nli].sx - pts[li].sx)<<16)/dy;
         else
            lslope=0;

         slint = pts[li].sx<<16;
      }
      if (ry<=y) // advance right edge?
      {
         while ((ry<=y) && (rem))
         {
            rem--;
            ri=nri;
            nri=MY_NEXT_RIGHT(ri,numpts);
            ry=pts[nri].sy;
         }
         if (ry<=y) return;

         dy = (ry - pts[ri].sy);

         if (dy)
            rslope = ((pts[nri].sx - pts[ri].sx)<<16)/dy;
         else
            rslope=0;

         srint = pts[ri].sx<<16;
      }
      while (y<ly && y<ry)
      {
         if (srint<slint)
            AddSpan(y,srint>>16,slint>>16,scanline);
         else
            AddSpan(y,slint>>16,srint>>16,scanline);

         y++;
         scanline+=swidth;
         slint += lslope;
         srint += rslope;
      }
   }
}



// (More) General stuff:

/* Make these global so they're not re-allocated
   every time ClipDrawBSPFace is called */
static vec3_dt pts[2][64];

static int update_pal;

static vec3_t light={1,0.8,0.5};

static viewport_t *bspvport;
static int vmx,vmy,vax,vay;

static void ClipDrawBSPFace(bsp_face_t *F, int color)
{
   int    i, j;
   int    andcode,orcode;
   float  xfloat, yfloat;
   int    pt_in[64];
   float  scale;

   texture_t *t;
   int textured;

   vec3_dt *s,*d;
   vec3_dt *p,*p2;
   int src,dest;
   int nsrc,ndest;

   int andval;
   int next;

   vec3_t *v;

   vec3_vt *vt;


   /* Check if poly is completely non-visible. */
   andcode = -1;
   orcode=0;
   for (i=0; i<F->numpts; i++)
   {
      andcode &= M.TBSPverts[F->i_pts[i]].outcode;
      orcode |= M.TBSPverts[F->i_pts[i]].outcode;
   }

   if (bspvport->fullbright)
   {
      andcode&=~0x10;
      orcode&=~0x10;
   }

   if (andcode)
      return;


   textured=bspvport->textured;
   if (textured!=BSP_GREY)
   {
      t=ReadMIPTex(F->mf->texname,0);
      if (!t)
         textured=BSP_GREY;
   }
   else
      t=NULL;

   if (F->mf->flags&TEX_FULLBRIGHT && (textured>=BSP_LIGHT))
   {
      textured=BSP_TEXC;
   }


   if (textured<=BSP_COL)
   {
      if (textured==BSP_GREY)
      {
         float lval;

         lval=DotProd(curplane.normal,light);

         color=(lval+1)*8;
         if (color<1) color=1;
         if (color>15) color=15;
      }
      else
      {
         if (t)
         {
            if (t->color==-1)
               if (GetTexColor(t))
                  update_pal=1;

            color=t->color;
         }
      }
   }
   else
   {
      for (i=0;i<F->numpts;i++)
      {
         v=&M.BSPverts[F->i_pts[i]];

         pts[0][i].tx=F->t_pts[i].s;
         pts[0][i].ty=F->t_pts[i].t;
/*            v->x*mf->vecs[0][0]+
            v->y*mf->vecs[0][1]+
            v->z*mf->vecs[0][2]+
                 mf->vecs[0][3];
   
         pts[0][i].ty=
            v->x*mf->vecs[1][0]+
            v->y*mf->vecs[1][1]+
            v->z*mf->vecs[1][2]+
                 mf->vecs[1][3];*/

         if (t)
         { /* If the texture is scaled, we must adjust our coordinates */
            pts[0][i].tx*=t->dsx/(float)t->rsx;
            pts[0][i].ty*=t->dsy/(float)t->rsy;
         }

         pts[0][i].z = M.TBSPverts[F->i_pts[i]].z;
      }
   }

   /* If it's completely visible, don't clip it. */
   if (!orcode)
   {
      nsrc=F->numpts;
      src=0;
      for (i=0,p=pts[0]; i<nsrc; i++,p++)
      {
         vt=&M.TBSPverts[F->i_pts[i]];
         p->sx = vt->sx;
         p->sy = vt->sy;
      }

      if (textured>=BSP_LIGHT)
      {
         for (i=0,p=pts[0]; i<nsrc; i++,p++)
         {
            v=&M.BSPverts[F->i_pts[i]];
            p->rx=v->x;
            p->ry=v->y;
            p->rz=v->z;
         }
      }

      goto raster;
   }

   /* Clip it and then draw it */
   nsrc=F->numpts;
   for (i=0,s=pts[0]; i<nsrc; i++,s++)
   {
      vt=&M.TBSPverts[F->i_pts[i]];
      s->x =vt->x;
      s->y =vt->y;
      s->z =vt->z;
      s->outcode =vt->outcode;
   }
   if (textured>=BSP_LIGHT)
   {
      for (i=0,s=pts[0]; i<nsrc; i++,s++)
      {
         v=&M.BSPverts[F->i_pts[i]];
         s->rx=v->x;
         s->ry=v->y;
         s->rz=v->z;
      }
   }

   src=0;
   dest=1;

   i=0;
   while (orcode)
   {
      s=pts[src];
      d=pts[dest];

      while (!(orcode&(1<<i)))
         i++;

      andval=1<<i;

      /* Build pt_in list, which lists if it's on valid side of plane */
      ndest=0;
      for (j=0,p=pts[src]; j<nsrc; j++,p++)
      {
         if (p->outcode & andval)
            pt_in[j] = FALSE;
         else
         {
            pt_in[j] = TRUE;
            ndest++;
         }
      }
      if (!ndest)    // completely hidden by this plane
         return;

      ndest=0;
      /* Check points */
      for (j=0; j<nsrc; j++)
      {
         p=&s[j];

         if (pt_in[j])
         {
            memcpy(d,p,sizeof(vec3_dt));
            d++;
            ndest++;
         }

         next=j+1;
         if (next==nsrc)
            next=0;
         /* And check if edge will cross plane */
         if (pt_in[j] != pt_in[next])
         {
            p2=&s[next];

            Clip3D(p->x ,p->y ,p->z,
                   p2->x,p2->y,p2->z,
                   &d->x,&d->y,&d->z,
                   &scale,andval);

            if (textured>=BSP_TEX)
            {
               if (textured>=BSP_LIGHT)
               {
                  d->rx =p->rx+(p2->rx-p->rx)*scale;
                  d->ry =p->ry+(p2->ry-p->ry)*scale;
                  d->rz =p->rz+(p2->rz-p->rz)*scale;
               }
               d->tx =p->tx+(p2->tx-p->tx)*scale;
               d->ty =p->ty+(p2->ty-p->ty)*scale;
            }

            OutCode3D(d->x, d->y, d->z, d->outcode);
            d++;
            ndest++;
         }
      }

      if (ndest<3)
         return;

      nsrc=ndest;
      dest=src;
      src=!src;

      orcode&=~andval;
   }

   /* Transform to screen space */
   for (i=0,s=pts[src]; i<nsrc; i++,s++)
   {
      if (s->z < 0.01)
         yfloat = 1 / 0.01;
      else
         yfloat = 1 / s->z;

      xfloat = s->x * yfloat;
      yfloat*= s->y;

      s->sy = floor(vmy * (-yfloat+1) + vay + 0.5);
      s->sx = floor(vmx * ( xfloat+1) + vax + 0.5);
   }

raster:
   if (textured==BSP_TEX)
   {
      PDrawSpan=DrawSpanT;
      SpanBSPFaceT(pts[src], nsrc, t);
   }
   else
   if (textured==BSP_TEXC)
   {
      PDrawSpan=DrawSpanTC;
      SpanBSPFaceTC(pts[src], nsrc, t);
   }
   else
   if ((textured==BSP_LIGHT) || (textured==BSP_LIGHTS))
   {
      PDrawSpan=DrawSpanLight;
      SpanBSPFaceLight(pts[src], nsrc, t);
   }
   else
   {
      PDrawSpan=DrawSpan;
      SpanBSPFace(pts[src], nsrc, color);
   }
}


#ifdef PROFILE
static int numdrawn;
#endif

static void DrawBSPFace(bsp_face_t *F, int base_color)
{
   ClipDrawBSPFace(F, /*(F->facenumber%11)*16+12*/ base_color);
#ifdef PROFILE
   numdrawn++;
#endif
}

static vec3_t camerapt;


static void DrawBSPNode(bsp_node_t *Root)
{
static float dot;
static int i;

   MY_DotProd(Root->plane.normal,camerapt,dot);
   dot -= (Root->plane.dist);

   if (dot> 0.01)
   {
      /*draw the front node*/
      if (Root->FrontNode!=NULL) DrawBSPNode(Root->FrontNode);

      face_drawn=0;
      curplane=Root->plane;
      /* draw the leaves*/
      for (i=0;i<Root->numfaces;i++)
      {
         /* backface cull*/
         if (Root->faces[i]->samenormal&2)
            DrawBSPFace(Root->faces[i], COL_GREEN);
      }

      /*draw the back node*/
      if (Root->BackNode !=NULL) DrawBSPNode(Root->BackNode);
   }
   else
   if (dot<-0.01)
   {
      /*draw the back node*/
      if (Root->BackNode !=NULL) DrawBSPNode(Root->BackNode);

      face_drawn=0;
      /* flip the plane for this side */
      curplane=Root->plane;
      curplane.normal.x=-curplane.normal.x;
      curplane.normal.y=-curplane.normal.y;
      curplane.normal.z=-curplane.normal.z;
      curplane.dist    =-curplane.dist;

      /* draw the leaves*/
      for (i=0;i<Root->numfaces;i++)
      {
         /* backface cull*/
         if (Root->faces[i]->samenormal&1)
            DrawBSPFace(Root->faces[i], COL_GREEN);
      }

      /*draw the front node*/
      if (Root->FrontNode!=NULL) DrawBSPNode(Root->FrontNode);
   }
   else  // skip faces on this node
   {
      /*draw the front node*/
      if (Root->FrontNode!=NULL) DrawBSPNode(Root->FrontNode);

      /*draw the back node*/
      if (Root->BackNode !=NULL) DrawBSPNode(Root->BackNode);
   }
}


void DrawBSPTree(matrix_t m, int vport)
{
   int i;
   float xfloat,yfloat;
   vec3_vt *v;
   vec3_t *v2;
   float time;

   bspvport=&M.display.vport[vport];

   if (M.BSPHead==NULL)
   {
      RebuildBSP();
      if (!M.BSPHead)
      {
         bspvport->textured=0;
         bspvport->mode=WIREFRAME;
         return;
      }
   }

   vax=bspvport->xmin+1;
   vmx=(bspvport->xmax - vax) >> 1;

   vay=bspvport->ymin+1;
   vmy=(bspvport->ymax - vay) >> 1;

   for (i=0,v=M.TBSPverts,v2=M.BSPverts;i<M.uniqueverts;i++,v++,v2++)
   {
      /* run the bsp vertices through the matrix */
      v->x = ((m[0][0] * v2->x) +
              (m[0][1] * v2->y) +
              (m[0][2] * v2->z) +
              m[0][3]);
      v->y = ((m[1][0] * v2->x) +
              (m[1][1] * v2->y) +
              (m[1][2] * v2->z) +
              m[1][3]);
      v->z = ((m[2][0] * v2->x) +
              (m[2][1] * v2->y) +
              (m[2][2] * v2->z) +
              m[2][3]);
      OutCode3D(v->x, v->y, v->z,v->outcode);

      if (bspvport->fullbright)
         v->outcode&=~0x10;

      if (!v->outcode)
      {
         if (v->z<0.01)
            yfloat = 1 / 0.01;
         else
            yfloat = 1 / v->z;

         xfloat = v->x * yfloat;
         yfloat*= v->y;

         v->sy = floor(vmy * (-yfloat + 1) + vay + 0.5);
         v->sx = floor(vmx * ( xfloat + 1) + vax + 0.5);
      }
   }


   camerapt.x = (float)bspvport->camera_pos.x;
   camerapt.y = (float)bspvport->camera_pos.y;
   camerapt.z = (float)bspvport->camera_pos.z;

   update_pal=0;

   Normalize(&light);


   if (!spans)
   {
      spans=Q_malloc(sizeof(span_t)*INIT_SPANS);
      if (!spans)
      {
         HandleError("DrawBSPTree","Unable to allocate span buffer!");
         return;
      }
      spans_size=INIT_SPANS;
   }

   if (!spanstart)
   {
      sstart_size=video.ScreenHeight;
      spanstart=Q_malloc(sizeof(int)*sstart_size);
      if (!spanstart)
      {
         HandleError("DrawBSPTree","Unable to allocate span buffer!");
         return;
      }
   }

   memset(spanstart,-1,sizeof(int)*sstart_size);
   curspan=0;

#ifdef PROFILE
   pix_span=pix_draw=0;
#endif

   error=0;


   time=0;
   if (bspvport->textured>=BSP_LIGHT)
   {
      int chksum;
      unsigned int *c;
      int i;


      time=GetTime();

      chksum=0;
      for (i=192,c=(unsigned int *)dith_pal;i;i--,c++)
         chksum+=*c;

      if (!chksum) chksum=1;

      if (!(calc_lightmap) || (calc_lightmap!=chksum))
      {
         int l,i;
         float r,g,b;
         unsigned char *c;
   
         memcpy(dith_pal,texture_pal,sizeof(dith_pal));
         for (l=0;l<LIGHT_LEVELS;l++)
         {
            for (i=0,c=dith_pal;i<256;i++,c+=3)
            {
               if ((i>240) && (Game.light.fullbright))
               {
                  lightmap[l*256+i]=i;
               }
               else
               {
                  r=c[0]*l/(LIGHT_LEVELS-1);
                  g=c[1]*l/(LIGHT_LEVELS-1);
                  b=c[2]*l/(LIGHT_LEVELS-1);
                  lightmap[l*256+i]=Dith(r,g,b);
               }
   /*            printf("%i %i (%g %g %g) -> %i (%i %i %i)\n",
                  i,l,r,g,b,
                  lightmap[l*256+i],
                  dith_pal[lightmap[l*256+i]*3+0],
                  dith_pal[lightmap[l*256+i]*3+1],
                  dith_pal[lightmap[l*256+i]*3+2]);*/
            }
         }
   
         calc_lightmap=chksum;
      }
      if (bspvport->textured==BSP_LIGHTS)
         InitLights(1);
      else
         InitLights(0);
   }

#ifdef PROFILE
   numdrawn=0;
#endif


   DrawBSPNode(M.BSPHead);

   if (bspvport->textured>=BSP_LIGHT)
   {
      DoneLights();
      time=GetTime()-time;
      NewMessage("Pictured drawn in %g seconds.",time);
   }

   if (error)
   {
      HandleError("DrawBSPTree","Unable to enlarge span buffer!");
   }


#ifdef PROFILE
//   printf("%5i faces drawn\n",numdrawn);
   if (curspan>maxspan)
   {
      maxspan=curspan;
      printf("new maxspan=%i spans_size=%i\n",maxspan,spans_size);
   }

/*   printf("%5i/%5i spans, %10i pixels, %10i drawn (%3.2f%%)\n",
      span_draw,curspan,pix_span,pix_draw,pix_draw/(float)pix_span*100.0);*/

/*   if (pix_span)
   {
      printf("%10i/%10i drawn (%6.2f%%)\n",
         pix_draw,pix_span,pix_draw/(float)pix_span*100.0);
   }*/
#endif

   if (update_pal && (bspvport->textured<=BSP_COL))
      SetPal(PAL_QUEST);
   UpdateViewportBorder(vport);
}

