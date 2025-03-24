// MOUSE.C module of Quest v1.1 Source Code
// Info on the Quest source code can be found in qsrcinfo.txt

// Copyright 1996, Trey Harrison and Chris Carollo
// For non-commercial use only. More rules apply, read legal.txt
// for a full description of usage restrictions.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xos.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include "defines.h"
#include "types.h"

#include "hw_x11.h"
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

short LastWindow; // Hold the last window occupied by the mouse
FILE* mousef;
short* mousedata;
int mouse_polling = 0;
Cursor busycursor;

/* from key_x11.c */
extern void X_HandleEvents(void);
extern int keys[NUM_UNIQUE_KEYS];

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

static void
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

static void
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
}

void
UndrawMouse(int x, int y)
{
}

void
ShowMouseCursor(void)
{
}

void
HideMouseCursor(void)
{
}

void
ActiveMouseCursor(void)
{
  XUndefineCursor(xdisplay, xwindow);
  XFlush(xdisplay);
}

void
BusyMouseCursor(void)
{
  XDefineCursor(xdisplay, xwindow, busycursor);
  XFlush(xdisplay);
}

void
SetMouseSensitivity(float valuex, float valuey)
{
}

void
SetMousePos(int x, int y)
{
  mouse.x = x;
  mouse.y = y;
  XWarpPointer(xdisplay, None, xwindow, 0, 0, 0, 0, x, y);

  UpdateScreenShot();
}

void
SetMouseLimits(int x0, int y0, int x1, int y1)
{
}

void
GetMousePos(void)
{
  mouse.prev_x = mouse.x;
  mouse.prev_y = mouse.y;

  {
    XEvent xe;
    int i;

    for (i = 0; i < NUM_UNIQUE_KEYS; i++)
      if (keys[i])
        break;
    if (i == NUM_UNIQUE_KEYS)
    {
      /*
       No key is being held down (or at least we don't think so), so pretend
       nothing's happening and block.
      */

      XNextEvent(xdisplay, &xe);
      XPutBackEvent(xdisplay, &xe);
    }
  }

  X_HandleEvents();

  if ((mouse.x != mouse.prev_x) || (mouse.y != mouse.prev_y))
    mouse.moved = TRUE;
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
    WaitRetr();
    UndrawMouse(mouse.prev_x, mouse.prev_y);
    DrawMouse(mouse.x, mouse.y);
  }
}

int
InitMouse(void)
{
  LastWindow = -1;

  HideMouseCursor();
  SetMousePos(video.ScreenWidth / 2, video.ScreenHeight / 2);
  SetMouseLimits(0, 0, video.ScreenWidth - 1, video.ScreenHeight - 1);
  SetMouseSensitivity(mouse_sens_x, mouse_sens_y);

  busycursor = XCreateFontCursor(xdisplay, XC_watch);

  return TRUE;
}
