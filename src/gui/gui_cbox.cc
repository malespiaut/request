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
}

#include "gui.h"
#include "gui_draw.h"

gui_checkbox::gui_checkbox(int x1, int y1, int sx, int sy, gui_group* parent)
  : gui_c(x1, y1, sx, sy, parent)
{
  FUNC

    value = 0;
  text = NULL;
  cbFlags = 0;

  //   Flags&=~flCanFocus;
}

gui_checkbox::~gui_checkbox(void)
{
  FUNC

    if (text)
      Q_free(text);
}

void
gui_checkbox::Init(int avalue, int flags, const char* text)
{
  FUNC

    value = avalue;
  cbFlags = flags;
  gui_checkbox::text = Q_strdup(text);
  if (!gui_checkbox::text)
    ABORT("Out of memory!");

  sy = ROM_CHAR_HEIGHT;
  sx = 16 + QUI_strlen(0, text) + 2;
  UpdatePos();
}

void
gui_checkbox::NextValue(void)
{
  if (value == cbvOff)
  {
    value = cbvOn;
    return;
  }
  else if (value == cbvOn)
    if (cbFlags & cbfCanUndef)
    {
      value = cbvUndef;
      return;
    }
  value = cbvOff;
}

void
gui_checkbox::Draw(void)
{
  int col;

  if (cbFlags & cbfFocused)
    col = GetColor(15);
  else
    col = 0;

  QUI_DrawStrM(rx1 + 16, ry1, rx2, GetColor(BG_COLOR), col, 0, 0, "%s", text);

  GUI_SolidBox(rx1 + 4, ry1 + 4, rx1 + 11, ry2 - 5, 15);
  GUI_Box(rx1 + 3, ry1 + 3, rx1 + 12, ry2 - 4, 0);

  switch (value)
  {
    case cbvUndef:
      GUI_SolidBox(rx1 + 4, ry1 + 4, rx1 + 11, ry2 - 5, 8);
      /*      {
               unsigned char *c,*d;
               int i,j;

               c=&video.ScreenBuffer[rx1+4+(ry1+4)*video.ScreenWidth];
               for (i=7;i;i--)
               {
                  for (d=c,j=7;j;j--,d++)
                     if ((i^j)&1) *d=GetColor(8);
                  c+=video.ScreenWidth;
               }
            }*/
      break;
    case cbvOn:
      {
        unsigned char *c, *d;
        int i;

        c = &video.ScreenBuffer[rx1 + 3 + (ry1 + 3) * video.ScreenWidth];
        d = &video.ScreenBuffer[rx1 + 3 + (ry2 - 4) * video.ScreenWidth];
        for (i = 0; i < 9; i++)
        {
          d[0] = d[1] = c[0] = c[1] = 0;
          c += video.ScreenWidth + 1;
          d -= video.ScreenWidth - 1;
        }
      }
      break;
  }
}

void
gui_checkbox::HandleEvent(event_t* e)
{
  if ((e->what == evKey) && (e->key == (' ' + KS_CHAR)))
  {
    e->what = evNothing;
    NextValue();
    ReDraw();
    return;
  }

  if (e->what == evMouse1Down)
  {
    e->what = evNothing;
    NextValue();
    ReDraw();
    return;
  }
}

void
gui_checkbox::FocusOn(void)
{
  cbFlags |= cbfFocused;
  ReDraw();
}

void
gui_checkbox::FocusOff(void)
{
  cbFlags &= ~cbfFocused;
  ReDraw();
}

int
gui_checkbox::GetValue(void)
{
  return value;
}
