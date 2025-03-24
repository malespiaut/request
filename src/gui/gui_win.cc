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
#include "qui.h"
#include "video.h"

#include "quest.h"
}

#include "gui.h"
#include "gui_draw.h"

/****************************************************************************
gui_win - Window class for dialogs, etc.
****************************************************************************/

gui_win::gui_win(int x1, int y1, int sx, int sy)
  : gui_group(x1, y1, sx, sy, NULL)
{
  FUNC

    scrbuf = NULL;
  title = NULL;

  if (gui_win::x1 == -1)
    gui_win::x1 = (video.ScreenWidth - sx) / 2;
  if (gui_win::y1 == -1)
    gui_win::y1 = (video.ScreenHeight - sy) / 2;
  UpdatePos();
}

gui_win::~gui_win(void)
{
  FUNC

    if (scrbuf)
      Q_free(scrbuf);
  if (title)
    Q_free(title);
}

void
gui_win::Init(char* title, ...)
{
  FUNC

    char buf[256];
  va_list arg;

  va_start(arg, title);
  vsprintf(buf, title, arg);
  va_end(arg);

  gui_win::title = Q_strdup(buf);
  if (!gui_win::title)
    ABORT("Out of memory!");

  scrbuf = (unsigned char*)Q_malloc(sx * sy);
  if (!scrbuf)
    ABORT("Out of memory!");

  {
    unsigned char *tb, *sb;
    int i;

    for (i = sy,
        sb = &video.ScreenBuffer[rx1 + ry1 * video.ScreenWidth],
        tb = scrbuf;
         i;
         i--, tb += sx, sb += video.ScreenWidth)
    {
      memcpy(tb, sb, sx);
      memset(sb, GetColor(BG_COLOR), sx);
    }
  }
}

void
gui_win::InitPost(void)
{
  FUNC

  ClearKeys();
  FocusNext();
}

void
gui_win::Draw(void)
{
  FUNC

    GUI_HLine(ry1 + 1, rx1, rx2, 10);
  GUI_HLine(ry1 + 2, rx1 + 1, rx2, 8);
  GUI_HLine(ry2 - 1, rx1, rx2, 4);
  GUI_HLine(ry2, rx1, rx2, 3);

  GUI_VLine(rx1, ry1 + 1, ry2, 10);
  GUI_VLine(rx1 + 1, ry1 + 2, ry2, 8);
  GUI_VLine(rx2 - 1, ry1 + 1, ry2, 4);
  GUI_VLine(rx2, ry1, ry2, 3);

  /*	DrawLine(rx1  , ry1+1, rx2-1, ry1+1, GetColor(10));
    DrawLine(rx1+1, ry1+2, rx2-1, ry1+2, GetColor(8));
    DrawLine(rx1  , ry2-1, rx2-1, ry2-1, GetColor(3));
    DrawLine(rx1  , ry2-2, rx2-1, ry2-2, GetColor(4));

    DrawLine(rx1  , ry1+1, rx1  , ry2-1, GetColor(10));
    DrawLine(rx1+1, ry1+2, rx1+1, ry2-1, GetColor(8));
    DrawLine(rx2-1, ry1+1, rx2-1, ry2-1, GetColor(3));
    DrawLine(rx2-2, ry1  , rx2-2, ry2-1, GetColor(4));*/

  /* draw the title */
  QUI_DrawStr(rx1 + (sx - QUI_strlen(0, title)) / 2, ry1 + 9, GetColor(BG_COLOR), GetColor(26), 0, 0, title);

  /* box the title */
  GUI_Frame(rx1 + 8, ry1 + 8, rx2 - 11, ry1 + ROM_CHAR_HEIGHT + 8);
  //	        GetColor(4),GetColor(8));

  gui_group::Draw();
}

void
gui_win::UnDraw(void)
{
  FUNC

    if (Focus)
      FocusSet(NULL);

  {
    unsigned char *tb, *sb;
    int i;

    for (i = sy,
        sb = &video.ScreenBuffer[rx1 + ry1 * video.ScreenWidth],
        tb = scrbuf;
         i;
         i--, tb += sx, sb += video.ScreenWidth)
    {
      memcpy(sb, tb, sx);
    }
  }
}

void
gui_win::HandleEvent(event_t* e)
{
  FUNC

    gui_group::HandleEvent(e);

  if (!e->what)
    return;

  if (e->what == evKey)
  {
    if (e->key == KEY_TAB)
    {
      e->what = evNothing;
      FocusNext();
      return;
    }
    if (e->key == (KEY_TAB + KS_SHIFT))
    {
      e->what = evNothing;
      FocusPrev();
      return;
    }
    if (e->key == KEY_ENTER)
    {
      e->what = evNothing;
      SendEvent_Cmd(cmdbDefault);
    }
    if (e->key == KEY_ESCAPE)
    {
      e->what = evNothing;
      SendEvent_Cmd(cmdbCancel);
    }

    if (e->key == KEY_F8)
    {
      take_screenshot = 1;
    }
  }

  if (e->what & evMouse)
  {
    e->what = evNothing;
  }
}
