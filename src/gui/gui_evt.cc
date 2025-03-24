#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "keyboard.h"
#include "mouse.h"
}

#include "gui.h"

void
GetEvent(event_t* e)
{
  static mouse_t old;

  e->what = evNothing;

  UpdateMouse();

  e->mx = mouse.x;
  e->my = mouse.y;
  e->mb = mouse.button;

  if (mouse.button && !old.button)
    e->what = evMouse1Down;
  else if (!mouse.button && old.button)
    e->what = evMouse1Up;
  else if (mouse.button)
    e->what = evMouse1Pressed;
  else if ((mouse.x != old.x) || (mouse.y != old.y))
    e->what = evMouseMove;

  if (e->what)
  {
    old = mouse;
    return;
  }

  e->key = GetKey();
  if (e->key)
  {
    e->what = evKey;
    return;
  }
}

int
GUI_InBox(int x1, int y1, int x2, int y2)
{
  if (mouse.x >= x1)
    if (mouse.x <= x2)
      if (mouse.y >= y1)
        if (mouse.y <= y2)
          return 1;
  return 0;
}
