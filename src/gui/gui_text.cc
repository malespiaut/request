#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "color.h"
#include "error.h"
#include "keyboard.h"
#include "memory.h"
#include "qui.h"
#include "video.h"
}

#include "gui.h"
#include "gui_draw.h"


/*
gui_text - Normal, static text object.
*/

gui_text::gui_text(int x1,int y1,int sx,int sy,gui_group *parent)
   : gui_c(x1,y1,sx,sy,parent)
{
   FUNC

   text=NULL;
   Flags&=~flCanFocus;
}

gui_text::~gui_text(void)
{
   FUNC

   if (text)
      Q_free(text);
}

void gui_text::Init(int align,char *stext)
{
   FUNC

   text=Q_strdup(stext);
   if (!text)
      ABORT("Out of memory!");

   sy=ROM_CHAR_HEIGHT;
   sx=QUI_strlen(0,text)+2;

   switch (align)
   {
   case txtLeft:
      break;
   case txtRight:
      x1-=sx;
      break;
   case txtCenter:
      x1-=sx/2;
      break;
   }

   UpdatePos();
}

void gui_text::Draw(void)
{
   FUNC

   QUI_DrawStrM(rx1,ry1,rx2,GetColor(BG_COLOR),0,0,0,text);
}


/*
gui_label - Static text, has hot-key and focuses another object.
*/

gui_label::gui_label(int x1,int y1,int sx,int sy,gui_group *parent)
   : gui_text(x1,y1,sx,sy,parent)
{
   FUNC

   link=NULL;
   hotkey=0;
}

void gui_label::Init(int align,char *stext,gui_c *slink)
{
   FUNC

   link=slink;

   if (x1==-1) x1=link->x1;
   if (y1==-1) y1=link->y1-16;
   UpdatePos();

   gui_text::Init(txtLeft,stext);

   char *c;
   for (c=stext;*c;c++)
      if (*c=='~') break;
   if (*c)
   {
      sx-=Q.font[0].width[Q.font[0].map[(int)*c]];
      c++;
      hotkey=k_ascii[(int)*c];
      if (hotkey)
      {
         hotkey|=KS_ALT;
         Flags|=flPreEvent;
      }
   }

   switch (align)
   {
   case txtLeft:
      break;
   case txtRight:
      x1-=sx;
      break;
   case txtCenter:
      x1-=sx/2;
      break;
   }
   UpdatePos();
}

void gui_label::Draw(void)
{
   font_t *f;
   int x;
   char *c;
   int col;

   x=rx1;
   f=&Q.font[0];
   for (c=text;*c;c++)
   {
      if (*c=='~')
         c++,col=GetColor(COL_RED-8);
      else
         col=0;

      QUI_DrawChar(x,ry1,GetColor(BG_COLOR),col,0,0,*c);
      if (f->map[(int)*c]!=-1)
         x+=f->width[f->map[(int)*c]];
      else
         x+=8;
   }
}

void gui_label::HandleEvent(event_t *e)
{
   if ((e->what==evKey) && (e->key==hotkey))
   {
      e->what=evNothing;
      Parent->FocusSet(link);
      return;
   }
}

