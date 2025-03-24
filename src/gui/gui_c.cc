#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "error.h"
#include "mouse.h"
#include "qassert.h"
#include "video.h"
}

#include "gui.h"
#include "gui_draw.h"

/****************************************************************************
gui_c - Base GUI class.
****************************************************************************/

gui_c::gui_c(int x1, int y1, int sx, int sy, gui_group* parent)
{
  FUNC

    Next = Prev = Parent = NULL;

  gui_c::sx = sx;
  gui_c::sy = sy;
  gui_c::x1 = x1;
  gui_c::y1 = y1;

  Flags = flCanFocus;

  if (parent)
  {
    parent->AddChild(this);
  }

  UpdatePos();
}

gui_c::~gui_c(void)
{
  FUNC

    if (Next) Next->Prev = Prev;
  if (Prev)
    Prev->Next = Next;
}

void
gui_c::UpdatePos(void)
{
  FUNC

    int bx,
    by, maxx, maxy;

  if (x1 == -1)
  {
    x2 = y2 = rx1 = ry1 = rx2 = ry2 = -1;
    return;
  }

  if (Parent)
  {
    if (Parent->x1 == -1)
    {
      rx1 = ry1 = rx2 = ry2 = -1;
      x2 = x1;
      y2 = y1;
      return;
    }

    bx = Parent->rx1;
    by = Parent->ry1;
    if (Parent->sx == -1)
    {
      maxx = video.ScreenWidth - 1;
      maxy = video.ScreenHeight - 1;
    }
    else
    {
      maxx = Parent->rx2 - 1;
      maxy = Parent->ry2 - 1;
    }
  }
  else
  {
    bx = by = 0;
    maxx = video.ScreenWidth - 1;
    maxy = video.ScreenHeight - 1;
  }

  gui_c::rx1 = x1 + bx;
  gui_c::ry1 = y1 + by;

  if (sx == -1)
  {
    x2 = y2 = rx2 = ry2 = -1;
    return;
  }

  gui_c::x2 = x1 + sx - 1;
  gui_c::y2 = y1 + sy - 1;

  gui_c::rx2 = x2 + bx;
  gui_c::ry2 = y2 + by;

  /*
  TODO: Should check for coordinates out of bounds here (can't fit in parent or
  on screen, can cause ugly crashes if it happens). Hard to do, though, as this
  is often called when coordinates haven't been fully filled in.
  */
  QAssert((rx2 <= maxx) && (ry2 <= maxy));
}

void
gui_c::Draw(void)
{
  FUNC
}

void
gui_c::UnDraw(void)
{ // default, draw square BG_COLOR
  FUNC

    GUI_SolidBox(rx1, ry1, rx2, ry2, BG_COLOR);
}

void
gui_c::Refresh(void)
{
  FUNC

    RefreshPart(rx1, ry1, rx2, ry2);
  DrawMouse(mouse.x, mouse.y);
}

void
gui_c::ReDraw(void)
{
  FUNC

  Draw();
  Refresh();
}

void
gui_c::HandleEvent(event_t* e)
{ // Default: can't handle any events, let the caller handle it.
  FUNC
}

void
gui_c::SendEvent(event_t* e)
{ // Default: pass it to the parent
  FUNC

    if (Parent)
      Parent->SendEvent(e);
}

void
gui_c::SendEvent_Cmd(int cmd)
{
  FUNC

    event_t e;

  e.what = evCommand;
  e.control = this;
  e.cmd = cmd;
  SendEvent(&e);
}

void
gui_c::FocusOn(void)
{
  FUNC

    GUI_Box(rx1 - 1, ry1 - 1, rx2 + 1, ry2 + 1, 0);
  /*   unsigned char *c,*d;
     int i,j;
     int col;

     col=GetColor(12);

     c=&video.ScreenBuffer[rx1-1+(ry1-1)*video.ScreenWidth];
     d=&video.ScreenBuffer[rx2+1+(ry2+1)*video.ScreenWidth];
     for (i=sx+2,j=0;i;i--,j++,c++,d--)
        if ((j&3)<=1) *c=*d=0;

     c=&video.ScreenBuffer[rx1-1+(ry1-1)*video.ScreenWidth];
     d=&video.ScreenBuffer[rx2+1+(ry2+1)*video.ScreenWidth];
     for (i=sy+2    ;i;i--,j++,c+=video.ScreenWidth,d-=video.ScreenWidth)
        if ((j&3)<=1) *c=*d=0;*/

  RefreshPart(rx1 - 1, ry1 - 1, rx2 + 1, ry2 + 1);
}

void
gui_c::FocusOff(void)
{
  FUNC

    GUI_HLine(ry1 - 1, rx1 - 1, rx2 + 1, BG_COLOR);
  GUI_HLine(ry2 + 1, rx1 - 1, rx2 + 1, BG_COLOR);
  GUI_VLine(rx1 - 1, ry1 - 1, ry2 + 1, BG_COLOR);
  GUI_VLine(rx2 + 1, ry1 - 1, ry2 + 1, BG_COLOR);
  //   QUI_Box(rx1-1,ry1-1,rx2+1,ry2+1,GetColor(BG_COLOR),GetColor(BG_COLOR));
  /*   unsigned char *c,*d;
     int i,j;

     j=GetColor(BG_COLOR);

     c=&video.ScreenBuffer[rx1-1+(ry1-1)*video.ScreenWidth];
     d=&video.ScreenBuffer[rx1-1+(ry2+1)*video.ScreenWidth];
     for (i=sx+2;i;i--)
        *c++=*d++=j;

     c=&video.ScreenBuffer[rx1-1+(ry1-1)*video.ScreenWidth];
     d=&video.ScreenBuffer[rx2+1+(ry1-1)*video.ScreenWidth];
     for (i=sy+2;i;i--,c+=video.ScreenWidth,d+=video.ScreenWidth)
        *c=*d=j;*/

  RefreshPart(rx1 - 1, ry1 - 1, rx2 + 1, ry2 + 1);
}
