#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"
}
#include "gui_draw.h"
extern "C"
{
#include "color.h"
#include "video.h"
}


void GUI_HLine(int y,int x1,int x2,int col)
{
   unsigned char *b;

   col=GetColor(col);

   b=video.ScreenBuffer+y*video.ScreenWidth;
   if (x1<x2)
   {
      b+=x1;
      memset(b,col,x2-x1+1);
   }
   else
   {
      b+=x2;
      memset(b,col,x1-x2+1);
   }
}

void GUI_VLine(int x,int y1,int y2,int col)
{
   unsigned char *b;
   int y;

   col=GetColor(col);

   b=video.ScreenBuffer+x;
   if (y1<y2)
   {
      b+=y1*video.ScreenWidth;
      for (y=y2-y1+1;y;y--)
      {
         *b=col;
         b+=video.ScreenWidth;
      }
   }
   else
   {
      b+=y2*video.ScreenWidth;
      for (y=y1-y2+1;y;y--)
      {
         *b=col;
         b+=video.ScreenWidth;
      }
   }
}

void GUI_Frame(int x1,int y1,int x2,int y2)
{
   GUI_HLine(y1,x1,x2,4);
   GUI_HLine(y2,x2,x1,8);
   GUI_VLine(x1,y1,y2,4);
   GUI_VLine(x2,y1,y2,8);
}

void GUI_Box(int x1,int y1,int x2,int y2,int col)
{
   GUI_HLine(y1,x1,x2,col);
   GUI_HLine(y2,x1,x2,col);
   GUI_VLine(x1,y1,y2,col);
   GUI_VLine(x2,y1,y2,col);
}

void GUI_SolidBox(int x1,int y1,int x2,int y2,int col)
{
   int i,j;
   unsigned char *c;

   col=GetColor(col);
   for (i=y1,c=&video.ScreenBuffer[x1+y1*video.ScreenWidth],j=x2-x1+1;
        i<=y2;
        i++,c+=video.ScreenWidth)
      memset(c,col,j);
}

