/*
video.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "video.h"

#include "display.h"
#include "error.h"
#include "file.h"
#include "memory.h"
#include "quest.h"
#include "status.h"


video_t video;      /* Interface to hardware-related video*/


void DrawDot(int vport,svec_t v,int color)
{
   unsigned char *ptr;

   if (!v.onscreen)
      return;

   ptr = &video.ScreenBuffer[v.y*video.ScreenWidth + v.x];

   if ((v.x-2 < M.display.vport[vport].xmin) || (v.x+2 > M.display.vport[vport].xmax) ||
       (v.y-2 < M.display.vport[vport].ymin) || (v.y+2 > M.display.vport[vport].ymax))
      return;

   *(ptr-video.ScreenWidth-1) = color;
   *(ptr-video.ScreenWidth-video.ScreenWidth) = color;
   *(ptr-video.ScreenWidth+1) = color;
   *(ptr-2) = color;
   *(ptr+2) = color;
   *(ptr+video.ScreenWidth-1) = color;
   *(ptr+video.ScreenWidth+video.ScreenWidth) = color;
   *(ptr+video.ScreenWidth+1) = color;
}

void DrawBox(int vport,int x,int y,int size,int color)
{
   int i;

   if ((x-size < M.display.vport[vport].xmin) || (x+size > M.display.vport[vport].xmax))
      return;
   if ((y-size < M.display.vport[vport].ymin) || (y+size > M.display.vport[vport].ymax))
      return;

   for (i=x-size; i<x+size; i++)
      video.ScreenBuffer[((y-size) * video.ScreenWidth) + i] = color;
   for (i=y-size; i<y+size; i++)
      video.ScreenBuffer[(i * video.ScreenWidth) + x-size] = color;
   for (i=x-size; i<x+size; i++)
      video.ScreenBuffer[((y+size) * video.ScreenWidth) + i] = color;
   for (i=y-size; i<y+size+1; i++)
      video.ScreenBuffer[(i * video.ScreenWidth) + x+size] = color;
}

void DrawSolidBox(int x1,int y1,int x2,int y2,int color)
{
   int i, j;

   for (i=x1;i<x2;i++)
   {
      for (j=y1;j<y2;j++)
      {
         video.ScreenBuffer[j*video.ScreenWidth + i] = color;
      }
   }
}

void DrawSolidSquare(int x,int y,int size,int color)
{
   DrawSolidBox(x,y,x+size,y+size,color);
}

void DrawVertex(int vport,svec_t v,int color)
{
   unsigned char *ptr;

   if (!v.onscreen)
      return;

   ptr = &video.ScreenBuffer[v.y*video.ScreenWidth + v.x];

   if ((v.x-1 < M.display.vport[vport].xmin) ||
       (v.x+1 > M.display.vport[vport].xmax) ||
       (v.y-1 < M.display.vport[vport].ymin) ||
       (v.y+1 > M.display.vport[vport].ymax))
      return;

   *(ptr-video.ScreenWidth-1) = color;
   *(ptr-video.ScreenWidth) = color;
   *(ptr-video.ScreenWidth+1) = color;
   *(ptr-1) = color;
   *ptr = color;
   *(ptr+1) = color;
   *(ptr+video.ScreenWidth-1) = color;
   *(ptr+video.ScreenWidth) = color;
   *(ptr+video.ScreenWidth+1) = color;
}

/* Cohen-Sutherland line clipping */

void ClipDrawLine2D(int vport_num,int x0,int y0,int x1,int y1,int color)
{
   int x2, y2;
   int c0, c1;

restart_clip2d:

   c0 = OutCode2D(vport_num, x0, y0);
   c1 = OutCode2D(vport_num, x1, y1);

   if ((c0 | c1) == 0)
      DrawLine(x0, y0, x1, y1, color);
   else
   if ((c0 & c1) != 0)
      return;
   else
   {
      if (c0 != 0)
      {
         Clip2D(vport_num, x0, y0, x1, y1, &x2, &y2, c0);
         x0 = x2;
         y0 = y2;
      }
      else
      {
         Clip2D(vport_num, x0, y0, x1, y1, &x2, &y2, c1);
         x1 = x2;
         y1 = y2;
      }
      goto restart_clip2d;
   }
}


static void TransformLineToScreen3D(
   int vport_num,
   float x0,float y0,float z0,
   float x1,float y1,float z1,
   int *px0,int *py0,int *px1,int *py1)
{
   if ((z0 < 0.01) && (x0 < 0.01) && (y0 < 0.01))
      x0 = y0 = 0;
   else
   {
      x0 /= z0;
      y0 /= z0;
   }
   if ((z1 < 0.01) && (x1 < 0.01) && (y1 < 0.01))
      x1 = y1 = 0;
   else
   {
      x1 /= z1;
      y1 /= z1;
   }

   x0 = ((M.display.vport[vport_num].xmax - M.display.vport[vport_num].xmin - 1) >> 1) * (x0 + 1) + M.display.vport[vport_num].xmin + 1;
   y0 = ((M.display.vport[vport_num].ymax - M.display.vport[vport_num].ymin - 1) >> 1) * (y0 + 1) + M.display.vport[vport_num].ymin + 1;
   x1 = ((M.display.vport[vport_num].xmax - M.display.vport[vport_num].xmin - 1) >> 1) * (x1 + 1) + M.display.vport[vport_num].xmin + 1;
   y1 = ((M.display.vport[vport_num].ymax - M.display.vport[vport_num].ymin - 1) >> 1) * (y1 + 1) + M.display.vport[vport_num].ymin + 1;

   *px0=x0;
   *px1=x1;
   *py0=y0;
   *py1=y1;
}

static int ClipLine3D(
   int vport_num,
   float x0,float y0,float z0,
   float x1,float y1,float z1,
   int *px0,int *py0,int *px1,int *py1)
{
   float dx, dy, dz, t=0;
// float s;
   int c0;
   int c1;
// int clip0, clip1;

   y0 = -y0;
   y1 = -y1;

restart_clip3d:

   OutCode3D(x0, y0, z0, c0);
   OutCode3D(x1, y1, z1, c1);

   if (M.display.vport[vport_num].fullbright)
   {
      c0 &= 0xffffffef;
      c1 &= 0xffffffef;
   }

   if ((c0 | c1) == 0)
   {
      TransformLineToScreen3D(
         vport_num, x0, y0, z0, x1, y1, z1,
         px0,py0,px1,py1);
      return 1;
   }
   else
   if ((c0 & c1) != 0)
   {
      return 0;
   }
   else
   {
      if (c0 != 0)
      {
         dx = x1 - x0;
         dy = y1 - y0;
         dz = z1 - z0;
         if (c0 & 0x00000001)
            t = (-x0 + z0) / (dx - dz);
         else
         if (c0 & 0x00000002)
            t = (x0 + z0) / (-dx - dz);
         else
         if (c0 & 0x00000004)
            t = (-y0 + z0) / (dy - dz);
         else
         if (c0 & 0x00000008)
            t = (y0 + z0) / (-dy - dz);
         else
         if (c0 & 0x00000010)
            t = (1 - z0) / (dz);
         else
         if (c0 & 0x00000020)
            t = (z0) / (-dz);
         x0 = x0 + (t * dx);
         y0 = y0 + (t * dy);
         z0 = z0 + (t * dz);
      }
      else
      {
         dx = x1 - x0;
         dy = y1 - y0;
         dz = z1 - z0;
         if (c1 & 0x00000001)
            t = (-x0 + z0) / (dx - dz);
         else
         if (c1 & 0x00000002)
            t = (x0 + z0) / (-dx - dz);
         else
         if (c1 & 0x00000004)
            t = (-y0 + z0) / (dy - dz);
         else
         if (c1 & 0x00000008)
            t = (y0 + z0) / (-dy - dz);
         else
         if (c1 & 0x00000010)
            t = (1 - z0) / (dz);
         else
         if (c1 & 0x00000020)
            t = (z0) / (-dz);
         x1 = x0 + (t * dx);
         y1 = y0 + (t * dy);
         z1 = z0 + (t * dz);
      }
      goto restart_clip3d;
   }
}

void ClipDrawLine3D(int vport_num,float x0,float y0,float z0,float x1,float y1,float z1,int color)
{
   int dx0,dy0,dx1,dy1;

   if (ClipLine3D(vport_num,x0,y0,z0,x1,y1,z1,&dx0,&dy0,&dx1,&dy1))
      DrawLine(dx0,dy0,dx1,dy1,color);
}


void Clip2D(int vport_num,int x0,int y0,int x1,int y1,int *x2,int *y2,int code)
{
// float t;
   float dx, dy;

   dx = x1 - x0;
   dy = y1 - y0;

   if (code & 0x0001)
   {
      *x2 = M.display.vport[vport_num].xmin;
      *y2 = (float)y0 + (float)(*x2 - x0) * dy / dx;
   }
   else
   if (code & 0x0002)
   {
      *x2 = M.display.vport[vport_num].xmax;
      *y2 = (float)y0 + (float)(*x2 - x0) * dy / dx;
   }
   else
   if (code & 0x0004)
   {
      *y2 = M.display.vport[vport_num].ymin;
      *x2 = (float)x0 + (float)(*y2 - y0) * dx / dy;
   }
   else
   if (code & 0x0008)
   {
      *y2 = M.display.vport[vport_num].ymax;
      *x2 = (float)x0 + (float)(*y2 - y0) * dx / dy;
   }
}

void Clip3D(float  x0,float  y0,float  z0,float  x1,float  y1,float  z1,float *x2,float *y2,float *z2,float *t,int code)
{
   float dx, dy, dz;
   float sc;

   dx = x1 - x0;
   dy = y1 - y0;
   dz = z1 - z0;

   if (code & 0x00000001)
      sc = (-x0 + z0) / (dx - dz);
   else
   if (code & 0x00000002)
      sc = (x0 + z0) / (-dx - dz);
   else
   if (code & 0x00000004)
      sc = (-y0 + z0) / (dy - dz);
   else
   if (code & 0x00000008)
      sc = (y0 + z0) / (-dy - dz);
   else
   if (code & 0x00000010)
      sc = (1 - z0) / (dz);
   else
//   if (code & 0x00000020)
      sc = (z0) / (-dz);

   *x2 = x0 + (sc * dx);
   *y2 = y0 + (sc * dy);
   *z2 = z0 + (sc * dz);
   *t = sc;
}

int OutCode2D(int vport_num,int x,int y)
{
   int code = 0;

   if (x < M.display.vport[vport_num].xmin)
      code |= 0x0001;
   if (x > M.display.vport[vport_num].xmax)
      code |= 0x0002;
   if (y < M.display.vport[vport_num].ymin)
      code |= 0x0004;
   if (y > M.display.vport[vport_num].ymax)
      code |= 0x0008;

   return code;
}

void Draw2DSpecialLine(int vport,int x0,int y0,int x1,int y1,int color)
{
   int delta_x, delta_y;
   int x_dir;
   int d, incrE, incrNE;
   int temp;

   unsigned char *oldsp;
   unsigned char *screen_ptr = video.ScreenBuffer;

   /* Flip vertices so we're always drawing downward (increasing Y) */
   if (y1 < y0)
   {
      temp = x0;
      x0 = x1;
      x1 = temp;
      temp = y0;
      y0 = y1;
      y1 = temp;
   }
 
   /* Calc deltas */
   delta_x = x1 - x0;
   delta_y = y1 - y0;
 
   /* Check which way we're drawing */
   if (delta_x < 0)
   {
      delta_x = -delta_x;
      x_dir = -1;
   }
   else
      x_dir = 1;
 
   screen_ptr += (y0 * video.ScreenWidth) + x0;
 
   /* Check horizontal case */
   if (delta_y == 0)
   {
      if (x_dir == 1)
         memset(screen_ptr, color, delta_x);
      else
         memset(screen_ptr-delta_x, color, delta_x);
    
      screen_ptr=video.ScreenBuffer + y0*video.ScreenWidth + M.display.vport[vport].xmin;
      for (x0=M.display.vport[vport].xmin; x0<M.display.vport[vport].xmax; x0+=4)
      {
         if (*screen_ptr==0) *screen_ptr=COL_BLUE-5;
         screen_ptr += 4;
      }
      return;
   }
 
   /* Check vertical case */
   if (delta_x == 0)
   {
      while (y0 < y1)
      {
         *screen_ptr = color;
         screen_ptr += video.ScreenWidth;
         y0++;
      }
      y0=M.display.vport[vport].ymin;
      oldsp=video.ScreenBuffer + y0*video.ScreenWidth + x0;
      while (y0<M.display.vport[vport].ymax)
      {
         if (*oldsp==0) *oldsp = COL_BLUE-5;
         oldsp += (video.ScreenWidth<<2);
         y0+=4;
      }
      return;
   }
 
   /* Now check for X-Major or Y-Major */
   if (delta_x > delta_y)
   {
      d = (delta_y << 1) - delta_x;
      incrE = delta_y << 1;
      incrNE = (delta_y - delta_x) << 1;
  
      *screen_ptr = color;
  
      while (x0 != x1)
      {
         if (d <= 0)
            d += incrE;
         else
         {
            d += incrNE;
            y0++;
            screen_ptr += video.ScreenWidth;
         }
         *screen_ptr = color;
         x0 += x_dir;
         screen_ptr += x_dir;
      }
   }
   else
   {
      d = (delta_x << 1) - delta_y;
      incrE = delta_x << 1;
      incrNE = (delta_x - delta_y) << 1;
  
      *screen_ptr = color;
  
      while (y0 < y1)
      {
         if (d <= 0)
            d += incrE;
         else
         {
            d += incrNE;
            x0 += x_dir;
            screen_ptr += x_dir;
         }
         *screen_ptr = color;
         y0++;
         screen_ptr += video.ScreenWidth;
      }
   }
}

void DrawLine(int x0, int y0, int x1, int y1, int color)
{
   int delta_x, delta_y;
   int x_dir;
   int d, incrE, incrNE;
   int temp;
 
   unsigned char *screen_ptr = video.ScreenBuffer;


   if (y0<0) y0=0;    // QUI_InitWindow calls with y0==-1

   /* Flip vertices so we're always drawing downward (increasing Y) */
   if (y1 < y0)
   {
      temp = x0;
      x0 = x1;
      x1 = temp;
      temp = y0;
      y0 = y1;
      y1 = temp;
   }
 
   /* Calc deltas */
   delta_x = x1 - x0;
   delta_y = y1 - y0;
 
   /* Check which way we're drawing */
   if (delta_x < 0)
   {
      delta_x = -delta_x;
      x_dir = -1;
   }
   else
      x_dir = 1;
 
   screen_ptr += (y0 * video.ScreenWidth) + x0;
 
   /* Check horizontal case */
   if (delta_y == 0)
   {
      if (x_dir == 1)
         memset(screen_ptr,color,delta_x);
      else
         memset(screen_ptr-delta_x,color,delta_x);
      return;
   }
 
   /* Check vertical case */
   if (delta_x == 0)
   {
      while (y0 < y1)
      {
         *screen_ptr = color;
         screen_ptr += video.ScreenWidth;
         y0++;
      }
      return;
   }
 
   /* Now check for X-Major or Y-Major */
   if (delta_x > delta_y)
   {
      d = (delta_y << 1) - delta_x;
      incrE = delta_y << 1;
      incrNE = (delta_y - delta_x) << 1;
 
      *screen_ptr = color;
 
      while (x0 != x1)
      {
         if (d <= 0)
         {
            d += incrE;
         }
         else
         {
            d += incrNE;
            y0++;
            screen_ptr += video.ScreenWidth;
         }
         *screen_ptr = color;
         x0 += x_dir;
         screen_ptr += x_dir;
      }
   }
   else
   {
      d = (delta_x << 1) - delta_y;
      incrE = delta_x << 1;
      incrNE = (delta_x - delta_y) << 1;
 
      *screen_ptr = color;
 
      while (y0<y1)
      {
         if (d <= 0)
         {
            d += incrE;
         }
         else
         {
            d += incrNE;
            x0 += x_dir;
            screen_ptr += x_dir;
         }
         *screen_ptr = color;
         y0++;
         screen_ptr += video.ScreenWidth;
      }
   }
}


void DrawArrow2D(int vport,int x0,int y0,int x1,int y1,int col)
{
   float len;
   float lx1,ly1,lx2,ly2;
   float fx,fy;
   float wx,wy;

   ClipDrawLine2D(vport,
      (int)(x0+0.5),(int)(y0+0.5),
      (int)(x1+0.5),(int)(y1+0.5),
      col);

   fx=x1-x0;
   fy=y1-y0;

   if ((fabs(fx)<1) && (fabs(fy)<1))
      return;

   len=sqrt(fx*fx+fy*fy);
   len=1/len*25*M.display.vport[vport].zoom_amt;
   fx*=len;
   fy*=len;

   lx1=x1-fx;
   ly1=y1-fy;

   lx2=x1-fy;
   ly2=y1+fx;
   wx=(lx1+lx2)/2;
   wy=(ly1+ly2)/2;
   ClipDrawLine2D(vport,wx,wy,x1,y1,col);

   lx2=x1+fy;
   ly2=y1-fx;
   wx=(lx1+lx2)/2;
   wy=(ly1+ly2)/2;
   ClipDrawLine2D(vport,wx,wy,x1,y1,col);
}

void DrawArrow3D(int vport,float x0,float y0,float z0,
                           float x1,float y1,float z1,int col)
{
   int dx0,dy0,dx1,dy1;
   int tmp,tx,ty;
   float len;
   float lx1,ly1,lx2,ly2;
   float fx,fy;
   float wx,wy;

   if (!ClipLine3D(vport,x0,y0,z0,x1,y1,z1,&dx0,&dy0,&dx1,&dy1))
      return;

   DrawLine(dx0,dy0,dx1,dy1,col);

   TransformLineToScreen3D(vport,x0,y0,z0,x1,y1,z1,&tmp,&tmp,&tx,&ty);

   if (tx<=M.display.vport[vport].xmin || tx>=M.display.vport[vport].xmax ||
       ty<=M.display.vport[vport].ymin || ty>=M.display.vport[vport].ymax)
      return;


   fx=dx1-dx0;
   fy=dy1-dy0;

   if ((fabs(fx)<1) && (fabs(fy)<1))
      return;

   len=sqrt(fx*fx+fy*fy);
   len=1/len*25/(z1*3);
   fx*=len;
   fy*=len;

   lx1=dx1-fx;
   ly1=dy1-fy;

   lx2=dx1-fy;
   ly2=dy1+fx;
   wx=(lx1+lx2)/2;
   wy=(ly1+ly2)/2;
   ClipDrawLine2D(vport,wx,wy,dx1,dy1,col);

   lx2=dx1+fy;
   ly2=dy1-fx;
   wx=(lx1+lx2)/2;
   wy=(ly1+ly2)/2;
   ClipDrawLine2D(vport,wx,wy,dx1,dy1,col);
}

