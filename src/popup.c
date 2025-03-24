/*
popup.c file of the Quest Source Code

Copyright 1997, 1998, 1999 Alexander Malmberg
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

#include "popup.h"

#include "color.h"
#include "error.h"
#include "memory.h"
#include "mouse.h"
#include "qui.h"
#include "video.h"

typedef struct
{
  char* left;
  char* right;
  int leftlen;
  int rightlen;
} popstr_t;

static popstr_t* popstrs;
static int num_popstrs;

// static char *ptitle;

static int px, py;
static int maxlen;
static int has_right, has_left;

int
Popup_Display(void)
{
  QUI_window_t* w;
  unsigned char* tempbuf;
  int i;
  popstr_t* ps;
  int px1, py1, px2, py2;
  int omb;
  int oldselect;

  int real_num_popstrs;

  if (!num_popstrs)
    return -1;

  if (has_right && has_left)
  {
    maxlen += 24;
    has_right = has_left = 0;
  }

  if (px + 9 + maxlen >= video.ScreenWidth)
    px = video.ScreenWidth - 9 - maxlen - 1;
  if (py + 9 + num_popstrs * 16 >= video.ScreenHeight)
    py = video.ScreenHeight - 9 - num_popstrs * 16 - 1;

  if (px < 0)
    px = 0;
  if (py < 0)
    py = 0;

  real_num_popstrs = num_popstrs;
  if (num_popstrs * 16 >= video.ScreenHeight)
    num_popstrs = (video.ScreenHeight - 16) / 16 - 1;

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];
  w->size.x = 9 + maxlen;
  w->size.y = 9 + num_popstrs * 16;
  w->pos.x = px;
  w->pos.y = py;

  tempbuf = Q_malloc(w->size.x * w->size.y);
  if (!tempbuf)
  {
    HandleError("Popup_Display", "Out of memory!");
    num_popstrs = real_num_popstrs;
    return -1;
  }

  Q.num_popups++;

  {
    unsigned char *tb, *sb;
    /* Copy whats under the window to temp buffer */
    for (i = w->size.y,
        sb = &video.ScreenBuffer[w->pos.x + w->pos.y * video.ScreenWidth],
        tb = tempbuf;
         i;
         i--, tb += w->size.x, sb += video.ScreenWidth)
    {
      memcpy(tb, sb, w->size.x);
      memset(sb, GetColor(BG_COLOR), w->size.x);
    }
  }

  DrawLine(w->pos.x, w->pos.y, w->pos.x + w->size.x - 1, w->pos.y, GetColor(10));
  DrawLine(w->pos.x, w->pos.y, w->pos.x, w->pos.y + w->size.y - 1, GetColor(10));
  DrawLine(w->pos.x + 1, w->pos.y + 1, w->pos.x + w->size.x - 1, w->pos.y + 1, GetColor(8));
  DrawLine(w->pos.x + 1, w->pos.y + 1, w->pos.x + 1, w->pos.y + w->size.y - 1, GetColor(8));
  DrawLine(w->pos.x + w->size.x - 2, w->pos.y, w->pos.x + w->size.x - 2, w->pos.y + w->size.y - 1, GetColor(4));
  DrawLine(w->pos.x, w->pos.y + w->size.y - 2, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 2, GetColor(4));
  DrawLine(w->pos.x + w->size.x - 1, w->pos.y, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 1, GetColor(3));
  DrawLine(w->pos.x, w->pos.y + w->size.y - 1, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 1, GetColor(3));

  QUI_Box(w->pos.x + 4, w->pos.y + 4, w->pos.x + w->size.x - 4, w->pos.y + w->size.y - 4, 4, 8);

  px1 = px + 5;
  px2 = px + w->size.x - 5;
  py1 = py + 5;
  py2 = py1 + num_popstrs * 16;
  for (i = 0, ps = popstrs; i < num_popstrs; i++, ps++)
  {
    if (ps->left)
      QUI_DrawStr(px1, py1 + i * 16, BG_COLOR, 0, 0, 0, "%s", ps->left);
    if (ps->right)
      QUI_DrawStr(px2 - ps->rightlen, py1 + i * 16, BG_COLOR, 0, 0, 0, "%s", ps->right);
  }
  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  oldselect = -1;
  omb = mouse.button;
  while (1)
  {
    UpdateMouse();
    if (mouse.button != omb)
    {
      if (!mouse.button)
        break;
      omb = mouse.button;
    }

    if (InBox(px1, py1, px2, py2))
    {
      i = (mouse.y - py1) / 16;
      if (i < 0 || i > num_popstrs)
        i = -1;
    }
    else
      i = -1;

    if (i != oldselect)
    {
      if (oldselect != -1)
      {
        ps = &popstrs[oldselect];
        if (ps->left)
          QUI_DrawStr(px1, py1 + oldselect * 16, BG_COLOR, 0, 0, 0, "%s", ps->left);
        if (ps->right)
          QUI_DrawStr(px2 - ps->rightlen, py1 + oldselect * 16, BG_COLOR, 0, 0, 0, "%s", ps->right);
      }
      oldselect = i;
      if (i != -1)
      {
        ps = &popstrs[oldselect];
        if (ps->left)
          QUI_DrawStr(px1, py1 + oldselect * 16, BG_COLOR, 15, 0, 0, "%s", ps->left);
        if (ps->right)
          QUI_DrawStr(px2 - ps->rightlen, py1 + oldselect * 16, BG_COLOR, 15, 0, 0, "%s", ps->right);
      }
      RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
    }
  }

  {
    unsigned char *tb, *sb;
    /* Recover screenbuffer from temp buffer */
    for (i = w->size.y,
        sb = &video.ScreenBuffer[w->pos.x + w->pos.y * video.ScreenWidth],
        tb = tempbuf;
         i;
         i--, tb += w->size.x, sb += video.ScreenWidth)
    {
      memcpy(sb, tb, w->size.x);
    }
  }
  Q_free(tempbuf);
  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  Q.num_popups--;

  num_popstrs = real_num_popstrs;

  return oldselect;
}

void
Popup_Init(int x, int y)
{
  px = x;
  py = y;
  //   ptitle=title;
  num_popstrs = 0;
  popstrs = NULL;
  maxlen = 0;
  has_right = has_left = 0;
}

void
Popup_AddStr(const char* format, ...)
{
  char temp[256];
  va_list* args;
  popstr_t* ps;

  va_start(args, format);
  vsprintf(temp, format, args);
  va_end(args);

  ps = Q_realloc(popstrs, sizeof(popstr_t) * (num_popstrs + 1));
  if (!ps)
  {
    HandleError("Popup_AddStr", "Out of memory!");
    return;
  }
  popstrs = ps;
  ps = &popstrs[num_popstrs];
  memset(ps, 0, sizeof(popstr_t));

  if (strchr(temp, '\t'))
  {
    ps->right = Q_strdup(strchr(temp, '\t') + 1);
    if (!ps->right)
    {
      HandleError("Popup_AddStr", "Out of memory!");
      return;
    }
    ps->rightlen = QUI_strlen(0, ps->right);
    *strchr(temp, '\t') = 0;
    has_right = 1;
  }

  if (temp[0])
  {
    ps->left = Q_strdup(temp);
    if (!ps->left)
    {
      HandleError("Popup_AddStr", "Out of memory!");
      Q_free(ps->right);
      return;
    }
    ps->leftlen = QUI_strlen(0, ps->left);
    has_left = 1;
  }

  if (ps->leftlen + ps->rightlen + 2 > maxlen)
    maxlen = ps->leftlen + ps->rightlen + 2;

  num_popstrs++;
}

void
Popup_Free(void)
{
  int i;
  popstr_t* ps;

  for (i = 0, ps = popstrs; i < num_popstrs; i++, ps++)
  {
    Q_free(ps->right);
    Q_free(ps->left);
  }
  Q_free(popstrs);
  popstrs = NULL;
  num_popstrs = 0;
}
