#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "color.h"
#include "display.h"
#include "error.h"
#include "keyboard.h"
#include "memory.h"
#include "mouse.h"
#include "qui.h"
#include "video.h"
}

#include "gui.h"
#include "gui_draw.h"


/*
gui_button - Button class.
*/

gui_button::gui_button(int x1,int y1,int sx,int sy,gui_group *parent)
   : gui_c(x1,y1,sx,sy,parent)
{
   FUNC

   bFlags=bPressed=0;
   bState=btsNormal;
   bData=NULL;
   bType=0;
}

gui_button::~gui_button(void)
{
   FUNC

   if ((bType==btntText) && bData)
      Q_free(bData);
}

void gui_button::Init(int flags,char *text)
{
   FUNC

   bType=btntText;

   bFlags=flags;
   bData=(void *)Q_strdup(text);
   if (!bData)
      ABORT("Out of memory!");

   sx=QUI_strlen(0,(char *)bData)+16;
   sy=ROM_CHAR_HEIGHT+8;
   UpdatePos();
}

void gui_button::InitP(int flags,char *pic)
{
   FUNC

   bitmap_t *b;

   b=FindBitmap(pic);
   if (!b) ABORT("Can't find bitmap '%s'!",pic);

   bType=btntPic;

   bFlags=flags;
   bData=(void *)b;
   if (!bData)
      ABORT("Out of memory!");

   sx=b->size.x+6;
   sy=b->size.y+6;
   UpdatePos();
}


void gui_button::Draw(void)
{
   FUNC

   int c1,c2;
   int x1,y1/*,x2,y2*/;

   switch (bType)
   {
      case btntPic:
         DrawBitmapP((bitmap_t *)bData,rx1+3,ry1+3);
         break;

      case btntText:
         x1=rx1+(sx-QUI_strlen(0,(char *)bData))/2;
         y1=ry1+(sy-ROM_CHAR_HEIGHT)/2+1;

         if (bState==btsDisabled)
            c1=8;
         else
         if (bState==btsFocused)
            c1=15;
         else
         if (bFlags&btnDefault)
            c1=COL_YELLOW;
         else
            c1=13;

         QUI_DrawStrM(x1,y1+1,rx2,
            GetColor(BG_COLOR),GetColor(c1),0,0,(char *)bData);
         break;
   }

/*   x1=rx1;
   y1=ry1;
   x2=x1+sx-1;
   y2=y1+sy-1;*/

   GUI_VLine(rx1,ry1,ry2,4);
   GUI_HLine(ry1,rx1,rx2,4);

   GUI_VLine(rx2,ry1,ry2,8);
   GUI_HLine(ry2,rx1,rx2,8);

   if (bPressed)
      c1=4,c2=8;
   else
      c1=8,c2=4;

   GUI_VLine(rx1+1,ry1+1,ry2-1,c1);
   GUI_VLine(rx1+2,ry1+2,ry2-2,c1);
   GUI_HLine(ry1+1,rx1+1,rx2-1,c1);
   GUI_HLine(ry1+2,rx1+2,rx2-2,c1);

   GUI_VLine(rx2-1,ry2-1,ry1+1,c2);
   GUI_VLine(rx2-2,ry2-2,ry1+2,c2);
   GUI_HLine(ry2-1,rx2-1,rx1+1,c2);
   GUI_HLine(ry2-2,rx2-2,rx1+2,c2);
}


void gui_button::HandleEvent(event_t *e)
{
   FUNC

   int newpressed;

   if (e->what==evCommand)
   {
      if ((e->cmd==cmdbDefault) && (bFlags&btnDefault))
      {
         e->what=evNothing;
         SendEvent_Cmd(cmdbPressed);
      }
      if ((e->cmd==cmdbCancel) && (bFlags&btnCancel))
      {
         e->what=evNothing;
         SendEvent_Cmd(cmdbPressed);
      }
   }

   switch (e->what)
   {
   case evMouse1Down:
      bPressed=1;
      ReDraw();
      e->what=evNothing;

      if (bFlags&btnFast)
         SendEvent_Cmd(cmdbPressed);
      return;

   case evMouse1Pressed:
      if (GUI_InBox(rx1,ry1,rx2,ry2))
         newpressed=1;
      else
         newpressed=0;

      if (newpressed!=bPressed)
      {
         bPressed=newpressed;
         ReDraw();
      }

      e->what=evNothing;

      if (bFlags&btnFast)
         SendEvent_Cmd(cmdbPressed);

      return;

   case evMouse1Up:
      newpressed=bPressed;
      bPressed=0;
      ReDraw();
      e->what=evNothing;

      if (newpressed)
         if (!(bFlags&btnFast))
            SendEvent_Cmd(cmdbPressed);
      return;

   case evKey:
      if (e->key==KEY_ENTER)
      {
         e->what=evNothing;
         SendEvent_Cmd(cmdbPressed);
         return;
      }
      return;
   }
}

void gui_button::SetState(int newstate)
{
   FUNC

   if (newstate!=bState)
   {
      bState=newstate;
      ReDraw();
   }
}

int gui_button::GetState(void)
{
   FUNC

   return bState;
}


void gui_button::FocusOn(void)
{
   FUNC

   SetState(btsFocused);
}

void gui_button::FocusOff(void)
{
   FUNC

   SetState(btsNormal);
}

