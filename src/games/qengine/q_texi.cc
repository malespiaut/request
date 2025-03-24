#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "q_texi.h"

#include "quake.h"

#include "edit.h"
#include "message.h"
#include "quest.h"
}

#include "gui/gui.h"

static int is_detail;

static void
GetValues(texdef_t* t)
{
  if (is_detail == -2)
    is_detail = t->g.q.detail;
  else if (t->g.q.detail != is_detail)
    is_detail = -1;
}

static void
SetValues(texdef_t* t)
{
  t->g.q.detail = is_detail;
}

void
Q_ModifyFlags(void)
{
  gui_win* win;
  gui_button *b_ok, *b_cancel;

  gui_list* ldetail;
  gui_checkbox* cdetail;

  event_t ev;

  int j;

  if (!M.display.num_bselected && !M.display.num_fselected)
  {
    NewMessage("Nothing selected!");
    return;
  }

  is_detail = -2;
  ForEachSelTexdef(GetValues);

  win = new gui_win(-1, -1, 200, 84);
  win->Init("Modify flags");

  ldetail = new gui_list(6, 32, -1, -1, win);

  if (is_detail == -1)
    j = cbvUndef;
  else if (is_detail)
    j = cbvOn;
  else
    j = cbvOff;

  cdetail = new gui_checkbox(-1, -1, -1, -1, ldetail);
  cdetail->Init(j, cbfCanUndef, "Detail");

  ldetail->Init(1);

  b_ok = new gui_button(4, win->sy - 28, -1, -1, win);
  b_ok->Init(btnDefault, "OK");

  b_cancel = new gui_button(b_ok->x2 + 4, b_ok->y1, -1, -1, win);
  b_cancel->Init(btnCancel, "Cancel");

  win->InitPost();
  win->ReDraw();

  do
  {
    win->Run(&ev);

    if ((ev.what == evCommand) && (ev.cmd == cmdbPressed))
      break;
  } while (1);

  if (ev.control == b_ok)
  {
    j = cdetail->GetValue();
    if (j != cbvUndef)
    {
      if (j == cbvOn)
        is_detail = 1;
      else
        is_detail = 0;
      ForEachSelTexdef(SetValues);
    }
  }

  win->UnDraw();
  win->Refresh();
  delete win;
}
