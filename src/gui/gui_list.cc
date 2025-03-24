#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "color.h"
#include "error.h"
#include "keyboard.h"
#include "memory.h"
}

#include "gui.h"
#include "gui_draw.h"

gui_list::gui_list(int x1, int y1, int sx, int sy, gui_group* parent)
  : gui_group(x1, y1, sx, sy, parent)
{
  FUNC

    num_children = num_columns = 0;
  col_x = NULL;
}

gui_list::~gui_list(void)
{
  FUNC

    if (col_x)
      Q_free(col_x);
}

void
gui_list::Init(int columns)
{
  gui_c *c, *c2;
  gui_c* cstart;
  int x, y, maxy;

  int csx;
  int tsx;

  int i, j, k;

  num_columns = columns;
  col_x = (int*)Q_malloc(sizeof(int) * columns);
  if (!col_x)
    ABORT("Out of memory!");

  j = num_children / columns;
  if (num_children % columns)
    j++;

  x = y = 2;
  csx = tsx = maxy = 0;
  k = 0;
  for (cstart = c = Children, i = j; c; c = c->Next)
  {
    c->x1 = x;
    c->y1 = y;
    y += c->sy;

    if (c->sx > csx)
      csx = c->sx;
    i--;
    if (!i)
    {
      tsx += csx + 4;
      x += csx + 4;

      for (c2 = cstart; c2 != c; c2 = c2->Next)
      {
        c2->sx = csx;
        c2->UpdatePos();
      }
      c2->sx = csx;
      c2->UpdatePos();
      col_x[k++] = c2->rx2 + 2;

      if (y > maxy)
        maxy = y;
      csx = 0;
      y = 2;
      cstart = c->Next;
    }
  }

  tsx += csx + 4;

  for (c2 = cstart; c2 != c; c2 = c2->Next)
  {
    c2->sx = csx;
    c2->UpdatePos();
  }
  if (y > maxy)
    maxy = y;

  if (sx == -1)
    sx = tsx + 8;
  if (sy == -1)
    sy = maxy + 2;

  UpdatePos();
}

void
gui_list::AddChild(gui_c* child)
{
  gui_group::AddChild(child);
  num_children++;
}

void
gui_list::Draw(void)
{
  int i;

  GUI_Frame(rx1, ry1, rx2, ry2);
  for (i = 0; i < num_columns - 1; i++)
  {
    GUI_VLine(col_x[i], ry1 + 1, ry2 - 1, GetColor(7));
    GUI_VLine(col_x[i] + 1, ry1 + 1, ry2 - 1, GetColor(5));
  }

  gui_group::Draw();
}

void
gui_list::HandleEvent(event_t* e)
{
  gui_group::HandleEvent(e);
  if (e->what == evNothing)
    return;

  if (e->what == evKey)
  {
    switch (e->key)
    {
      case KEY_UP:
        e->what = evNothing;
        FocusPrev();
        break;
      case KEY_DOWN:
        e->what = evNothing;
        FocusNext();
        break;
    }
  }
}

void
gui_list::FocusOn(void)
{
  gui_group::FocusOn();

  FocusSet(Children);
}

void
gui_list::FocusOff(void)
{
  gui_group::FocusOff();

  FocusSet(NULL);
}
