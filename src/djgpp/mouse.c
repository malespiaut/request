/*
mouse.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <dos.h>
#include <go32.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "types.h"

#include "mouse.h"

#include "display.h"
#include "keyboard.h"
#include "menu.h"
#include "message.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "video.h"

mouse_t mouse; /* Mouse buttons and location*/

static int LastWindow; /* Hold the last window occupied by the mouse*/

/*
  These probably don't have to be changed if you're porting.
*/
int
InBox(int x1, int y1, int x2, int y2)
{
  if (mouse.x > x1 + 1)
    if (mouse.x < x2 - 1)
      if (mouse.y > y1 + 1)
        if (mouse.y < y2 - 1)
          return TRUE;
  return FALSE;
}

int
FindMouse(int DrawBox)
{
  int i;
  int returnval = 0;
  int found;

  if ((MenuShowing == FALSE) && (status.pop_menu == 1))
  {
    if (mouse.x >= video.ScreenWidth - status.menu_limit)
    {
      if (mouse.y < Q.window[MENU_WINDOW].pos.y + Q.window[MENU_WINDOW].size.y)
      {
        if (mouse.y > Q.window[MENU_WINDOW].pos.y)
        {
          PopUpMenuWin();
          return MENU_WINDOW;
        }
      }
    }
  }

  found = FALSE;
  for (i = 0; i < Q.num_windows; i++)
  {
    if ((mouse.x) >= (Q.window[i].pos.x))
      if ((mouse.x) <= (Q.window[i].pos.x + Q.window[i].size.x))
        if ((mouse.y) >= (Q.window[i].pos.y))
          if ((mouse.y) <= (Q.window[i].pos.y + Q.window[i].size.y))
          {
            if (DrawBox)
            {
              if (LastWindow != i)
              {
                if (LastWindow != -1)
                  QUI_Box(Q.window[LastWindow].pos.x, Q.window[LastWindow].pos.y, Q.window[LastWindow].pos.x + Q.window[LastWindow].size.x, Q.window[LastWindow].pos.y + Q.window[LastWindow].size.y, 0, 0);
                QUI_Box(Q.window[i].pos.x, Q.window[i].pos.y, Q.window[i].pos.x + Q.window[i].size.x, Q.window[i].pos.y + Q.window[i].size.y, 14, 14);
              }
            }
            /*Update LastWindow variable*/
            found = TRUE;
            LastWindow = i;
            returnval = i;
            break;
          }
  }
  if (!found)
    return -1;

  if ((MenuShowing == TRUE) &&
      (returnval != MENU_WINDOW) &&
      (status.pop_menu))
  {
    PopDownMenuWin();
  }
  return returnval;
}

void
DrawMouseToBuf(int x, int y)
{
  int i;
  int x_dist;
  bitmap_t* bm;

  bm = &bitmap[0];

  for (i = 0, x_dist = 0; i < (bm->size.x * bm->size.y); i++, x_dist++)
  {
    if (x_dist >= bm->size.x)
    {
      x_dist = 0;
      y++;
      if (y >= video.ScreenHeight - 1)
        return;
    }
    if ((bm->data[i] != 0) && ((x + x_dist) <= video.ScreenWidth - 1))
      video.ScreenBuffer[x + x_dist + (y * video.ScreenWidth)] = bm->data[i];
  }
}

void
UpdateScreenShot(void)
{
  /*   if (TestKey(KEY_F12))
     {
        while (TestKey(KEY_F12)) ;
        take_screenshot=1;
     }*/

  if (take_screenshot)
  {
    ClearKeys();
    //		DrawMouseToBuf(mouse.x,mouse.y);
    TakeScreenShot();
    NewMessage("Screenshot taken.");
    take_screenshot = FALSE;
  }
}

void
DrawMouse(int x, int y)
{
  int i;
  int x_dist;
  bitmap_t* bm;

  bm = &bitmap[0];

  for (i = 0, x_dist = 0; i < (bm->size.x * bm->size.y); i++, x_dist++)
  {
    if (x_dist >= bm->size.x)
    {
      x_dist = 0;
      y++;
      if (y >= video.ScreenHeight - 1)
        return;
    }
    if ((bm->data[i] != 0) && ((x + x_dist) <= video.ScreenWidth - 1))
      PutPixel(x + x_dist, y, bm->data[i]);
  }
}

void
UndrawMouse(int x, int y)
{
  int i, j;
  int skip;
  bitmap_t* bm;
  unsigned char* alias;

  alias = &video.ScreenBuffer[(y * video.ScreenWidth) + x];
  bm = &bitmap[0];
  skip = video.ScreenWidth - bm->size.x;

  for (i = 0; i < bm->size.y; i++)
  {
    if ((y + i) >= video.ScreenHeight - 1)
      return;
    for (j = 0; j < bm->size.x; j++)
    {
      PutPixel(x + j, y + i, *alias);
      alias++;
    }
    alias += skip;
  }
}

void
ShowMouseCursor(void)
{
  union REGS r;

  r.x.ax = 0x01;
  int86(0x33, &r, &r);
  DrawMouse(mouse.x, mouse.y);
}

void
HideMouseCursor(void)
{
  union REGS r;

  r.x.ax = 0x02;
  int86(0x33, &r, &r);
  UndrawMouse(mouse.x, mouse.y);
}

void
SetMouseSensitivity(float valuex, float valuey)
{
  union REGS r;

  valuex = 1 / valuex;
  valuey = 1 / valuey;

  r.x.ax = 0x0F;
  r.x.cx = (int)(valuex * 8);
  r.x.dx = (int)(valuey * 16);
  int86(0x33, &r, &r);
}

void
SetMousePos(int x, int y)
{
  union REGS r;

  r.x.ax = 0x04;
  r.x.cx = x;
  r.x.dx = y;
  int86(0x33, &r, &r);

  mouse.x = x;
  mouse.y = y;

  UpdateScreenShot();
}

void
SetMouseLimits(int x0, int y0, int x1, int y1)
{
  union REGS r;

  r.x.ax = 0x07;
  r.x.cx = x0;
  r.x.dx = x1;
  int86(0x33, &r, &r);

  r.x.ax = 0x08;
  r.x.cx = y0;
  r.x.dx = y1;
  int86(0x33, &r, &r);
}

void
GetMousePos(void)
{
  union REGS r;

  mouse.prev_x = mouse.x;
  mouse.prev_y = mouse.y;

  r.x.ax = 0x03;
  int86(0x33, &r, &r);
  mouse.button = r.x.bx & 0xffff;
  mouse.x = r.x.cx & 0xffff;
  mouse.y = r.x.dx & 0xffff;

  if ((mouse.x != mouse.prev_x) || (mouse.y != mouse.prev_y))
  {
    mouse.moved = TRUE;
  }
  else
    mouse.moved = FALSE;
  UpdateScreenShot();
}

void
UpdateMouse(void)
{
  GetMousePos();
  if (mouse.moved)
  {
    UndrawMouse(mouse.prev_x, mouse.prev_y);
    DrawMouse(mouse.x, mouse.y);
  }
}

int
InitMouse(void)
{
  union REGS r;

  r.x.ax = 0x00;
  int86(0x33, &r, &r);

  if (r.x.ax == 0)
    return FALSE;

  LastWindow = -1;

  HideMouseCursor();
  SetMousePos(video.ScreenWidth / 2, video.ScreenHeight / 2);
  SetMouseLimits(0, 0, video.ScreenWidth - 1, video.ScreenHeight - 1);
  SetMouseSensitivity(mouse_sens_x, mouse_sens_y);

  return TRUE;
}
