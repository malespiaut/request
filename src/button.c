/*
button.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "button.h"

#include "color.h"
#include "display.h"
#include "error.h"
#include "keyboard.h"
#include "memory.h"
#include "mouse.h"
#include "qui.h"
#include "times.h"
#include "video.h"

button_t button[MAX_BUTTONS];
int lasthit;

static int
AddButton(int x, int y, int sx, int sy, int flags, int type, void* data)
{
  int i;
  button_t* b;

  for (i = 0; i < MAX_BUTTONS; i++)
  {
    if (!button[i].used)
      break;
  }
  if (i == MAX_BUTTONS)
    return -1;

  b = &button[i];
  b->used = 1;
  b->level = 0;

  b->x = x;
  b->y = y;
  b->sx = sx;
  b->sy = sy;

  b->flags = flags;
  b->type = type;
  b->on = 0;
  b->data = data;

  return i;
}

int
AddButtonPic(int x, int y, int flags, const char* name)
{
  bitmap_t* b;

  b = FindBitmap(name);
  if (!b)
    return -1;
  return AddButton(x, y, b->size.x + 6, b->size.y + 6, flags, B_PIC, b);
}

int
AddButtonText(int x, int y, int flags, const char* text)
{
  char* t;

  t = Q_malloc(strlen(text) + 1);
  if (!t)
    return -1;
  strcpy(t, text);
  return AddButton(x, y, QUI_strlen(0, t) + 16, ROM_CHAR_HEIGHT + 8, flags, B_TEXT, t);
}

void
RemoveButton(int i)
{
  button_t* b;

  b = &button[i];

  switch (b->type)
  {
    case B_TEXT:
      Q_free(b->data);
      break;
  }
  b->used = 0;
}

void
MoveButton(int i, int x, int y)
{
  if (!button[i].used)
  {
    HandleError("MoveButton", "Tried to move unused button %i\n", i);
  }
  button[i].x = x;
  button[i].y = y;
}

static void
HLine(int y, int x1, int x2, int col)
{
  unsigned char* b;

  b = video.ScreenBuffer + y * video.ScreenWidth;
  if (x1 < x2)
  {
    b += x1;
    memset(b, col, x2 - x1 + 1);
  }
  else
  {
    b += x2;
    memset(b, col, x1 - x2 + 1);
  }
}

static void
VLine(int x, int y1, int y2, int col)
{
  unsigned char* b;
  int y;

  b = video.ScreenBuffer + x;
  if (y1 < y2)
  {
    b += y1 * video.ScreenWidth;
    for (y = y2 - y1 + 1; y; y--)
    {
      *b = col;
      b += video.ScreenWidth;
    }
  }
  else
  {
    b += y2 * video.ScreenWidth;
    for (y = y1 - y2 + 1; y; y--)
    {
      *b = col;
      b += video.ScreenWidth;
    }
  }
}

static void
DrawButtonP(button_t* b, int pressed)
{
  int c1, c2;
  int x1, y1, x2, y2;

  switch (b->type)
  {
    case B_PIC:
      DrawBitmapP((bitmap_t*)b->data, b->x + 3, b->y + 3);
      break;
    case B_TEXT:
      QUI_DrawStr(b->x + (b->sx / 2) - (QUI_strlen(0, b->data) / 2),
                  b->y + (b->sy / 2) - (ROM_CHAR_HEIGHT / 2) + 1,
                  BG_COLOR,
                  14,
                  0,
                  0,
                  b->data);
      break;
  }

  x1 = b->x;
  y1 = b->y;
  x2 = x1 + b->sx - 1;
  y2 = y1 + b->sy - 1;

  VLine(x1, y1, y2, GetColor(4));
  HLine(y1, x1, x2, GetColor(4));

  VLine(x2, y1, y2, GetColor(8));
  HLine(y2, x1, x2, GetColor(8));

  if (pressed)
    c1 = GetColor(4), c2 = GetColor(8);
  else
    c1 = GetColor(8), c2 = GetColor(4);

  VLine(x1 + 1, y1 + 1, y2 - 1, c1);
  VLine(x1 + 2, y1 + 2, y2 - 2, c1);
  HLine(y1 + 1, x1 + 1, x2 - 1, c1);
  HLine(y1 + 2, x1 + 2, x2 - 2, c1);

  VLine(x2 - 1, y2 - 1, y1 + 1, c2);
  VLine(x2 - 2, y2 - 2, y1 + 2, c2);
  HLine(y2 - 1, x2 - 1, x1 + 1, c2);
  HLine(y2 - 2, x2 - 2, x1 + 2, c2);
}

void
DrawButton(int i)
{
  int p;

  p = 0;
  if (button[i].flags & B_TOGGLE)
    p = button[i].on;
  DrawButtonP(&button[i], p);
}

void
ToggleButton(int i, int on)
{
  button[i].on = on;
  DrawButton(i);
}

void
EraseButton(int i)
{
  DrawSolidBox(button[i].x, button[i].y, button[i].x + button[i].sx, button[i].y + button[i].sy, GetColor(BG_COLOR));
}

void
DrawButtons(void)
{
  int i;

  for (i = 0; i < MAX_BUTTONS; i++)
  {
    if (button[i].used && !button[i].level)
    {
      DrawButton(i);
    }
  }
}

void
PushButtons(void)
{
  int i;

  for (i = 0; i < MAX_BUTTONS; i++)
    if (button[i].used)
      button[i].level++;
}

void
PopButtons(void)
{
  int i;
  button_t* b;

  for (i = 0; i < MAX_BUTTONS; i++)
  {
    b = &button[i];
    if (b->used)
    {
      if (!b->level)
      {
        HandleError("PopButtons",
                    "level=0 type=%i name=%s",
                    b->type,
                    b->type == B_PIC ? ((bitmap_t*)(b->data))->name : (char*)b->data);
        RemoveButton(i);
      }
      else
      {
        b->level--;
      }
    }
  }
}

static int
MouseOn(button_t* b)
{
  return InBox(b->x, b->y, b->x + b->sx, b->y + b->sy);
}

static void
RefreshButtonB(button_t* b)
{
  RefreshPart(b->x, b->y, b->x + b->sx, b->y + b->sy);
  DrawMouse(mouse.x, mouse.y);
}

void
RefreshButton(int i)
{
  RefreshButtonB(&button[i]);
}

static button_t* cur_p;
static int cur_p_num;

static int lasttick = -1;

int
UpdateButtons(void)
{
  int i;
  button_t* b;
  int hit;
  int on;

  if (!(mouse.button & 1))
  {
    if (cur_p)
    {
      DrawButtonP(cur_p, 0);
      RefreshButtonB(cur_p);
      cur_p = NULL;
      lasttick = -1;
    }
    lasthit = -1;

    if (TestKey(KEY_ENTER))
    {
      for (i = 0, b = button; i < MAX_BUTTONS; i++, b++)
        if (b->used && !b->level &&
            b->flags & B_ENTER)
        {
          while (TestKey(KEY_ENTER))
            UpdateMouse();
          return i;
        }
    }
    if (TestKey(KEY_ESCAPE))
    {
      for (i = 0, b = button; i < MAX_BUTTONS; i++, b++)
        if (b->used && !b->level &&
            b->flags & B_ESCAPE)
        {
          while (TestKey(KEY_ESCAPE))
            UpdateMouse();
          return i;
        }
    }
    return -1;
  }

  if (cur_p && !MouseOn(cur_p))
  {
    DrawButtonP(cur_p, 0);
    RefreshButtonB(cur_p);
    cur_p = NULL;
    lasttick = -1;
  }

  for (i = 0, b = button; i < MAX_BUTTONS; i++, b++)
  {
    if (!b->used)
      continue;
    if (b->level)
      continue;

    if (!MouseOn(b))
      continue;

    hit = 1;

    if (b->flags & B_TOGGLE)
      on = !b->on;
    else
      on = 1;

    DrawButtonP(b, on);
    RefreshButtonB(b);
    if (b->flags & B_RAPID)
    {
      cur_p = b;
      cur_p_num = i;
      lasthit = i;
      if (lasttick == -1)
        lasttick = GetTick();
      else
      {
        // for rapid buttons, return button first tick, -2 for five
        // ticks, then button rest of the time
        if (lasttick + 5 > GetTick())
          return -2; // return -2, not -1
      }
      return i;
    }

    while (mouse.button & 1)
    {
      if (hit && !MouseOn(b))
      {
        hit = 0;
        DrawButtonP(b, !on);
        RefreshButtonB(b);
      }
      if (!hit && MouseOn(b))
      {
        hit = 1;
        DrawButtonP(b, on);
        RefreshButtonB(b);
      }
      UpdateMouse();
    }
    DrawButtonP(b, !on);
    RefreshButtonB(b);
    if (hit)
    {
      lasthit = i;
      return i;
    }
    lasthit = -1;
    return -1;
  }
  lasthit = -1;
  return -1;
}

void
InitButton(void)
{
  memset(button, 0, sizeof(button));
  cur_p = NULL;
  lasthit = -1;
}
