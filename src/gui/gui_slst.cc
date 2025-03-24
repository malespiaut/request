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
#include "mouse.h"
#include "qui.h"
}

#include "gui.h"
#include "gui_draw.h"

gui_scroll_list::gui_scroll_list(int x1, int y1, int sx, int sy, gui_group* parent)
  : gui_group(x1, y1, sx, sy, parent)
{
  FUNC

    pos = scroll = 0;
  data = NULL;
  num_data = 0;
  num_lines = 0;
}

gui_scroll_list::~gui_scroll_list(void)
{
  FUNC

  Data_Free();
}

void
gui_scroll_list::Init(void)
{
  num_lines = (sy - 4) / 16;

  tx2 = rx2 - 15;

  b_up = new gui_button(sx - 15, 2, -1, -1, this);
  b_up->InitP(btnFast, "button_tiny_up");

  b_dn = new gui_button(sx - 15, sy - 15, -1, -1, this);
  b_dn->InitP(btnFast, "button_tiny_down");
}

void
gui_scroll_list::Draw(void)
{
  int i, j;
  int col;

  GUI_SolidBox(rx1, ry1, rx2, ry2, BG_COLOR);

  GUI_Frame(rx1, ry1, rx2, ry2);
  GUI_VLine(tx2, ry1, ry2, 4);

  for (i = 0, j = scroll; i < num_lines && j < num_data; i++, j++)
  {
    if (j == pos)
      col = GetColor(15);
    else
      col = 0;

    QUI_DrawStrM(rx1 + 2, ry1 + 2 + i * 16, tx2, GetColor(BG_COLOR), col, 0, 0, "%s", data[j]);
  }

  gui_group::Draw();
}

void
gui_scroll_list::HandleEvent(event_t* e)
{
  gui_group::HandleEvent(e);
  if (e->what == evNothing)
    return;

  if (e->what & evMouse)
  {
    if ((e->what == evMouse1Down) || (e->what == evMouse1Pressed))
    {
      if (InBox(rx1 + 2, ry1 + 2, tx2 - 1, ry2 - 2))
      {
        int i;

        i = (e->my - ry1 - 2) / 16;
        i += scroll;
        MoveTo(i);
      }
    }
  }

  if (e->what == evKey)
  {
    switch (e->key)
    {
      case KEY_UP:
        e->what = evNothing;
        if (pos)
          MoveTo(pos - 1);
        break;
      case KEY_DOWN:
        e->what = evNothing;
        if (pos < num_data - 1)
          MoveTo(pos + 1);
        break;
    }
  }

  if ((e->what == evCommand) && (e->cmd == cmdbPressed))
  {
    if (e->control == b_up)
    {
      if (scroll)
      {
        scroll--;
        ReDraw();
      }
      e->what = evNothing;
    }

    if (e->control == b_dn)
    {
      if (scroll + num_lines < num_data)
      {
        scroll++;
        ReDraw();
      }
      e->what = evNothing;
    }
  }
}

void
gui_scroll_list::Data_Free(void)
{
  if (num_data)
  {
    int i;

    for (i = 0; i < num_data; i++)
      Q_free(data[i]);
    Q_free(data);
  }
  data = NULL;
  num_data = pos = scroll = 0;
}

void
gui_scroll_list::Data_New(void)
{
  Data_Free();
}

void
gui_scroll_list::Data_Add(char* txt)
{
  data = (char**)Q_realloc(data, sizeof(char*) * (num_data + 1));
  if (!data)
    ABORT("Out of memory!");

  data[num_data++] = Q_strdup(txt);
}

void
gui_scroll_list::Data_Done(void)
{
}

void
gui_scroll_list::MoveTo(int npos)
{
  if (npos >= num_data)
    npos = -1;
  if (npos < 0)
    npos = -1;

  if (npos == pos)
    return;

  if (npos == -1)
  {
    pos = -1;
    scroll = 0;
    ReDraw();
    return;
  }

  pos = npos;

  if (pos < scroll)
    scroll = pos;
  if (pos >= scroll + num_lines)
    scroll = pos - num_lines + 1;
  ReDraw();
}

char*
gui_scroll_list::Data_Get(int num)
{
  if (num < 0 || num >= num_data)
    return NULL;
  return data[num];
}

int
gui_scroll_list::GetPos(void)
{
  return pos;
}
