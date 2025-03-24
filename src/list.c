/*
list.c file of the Quest Source Code

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

#include "list.h"

#include "button.h"
#include "error.h"
#include "file.h"
#include "memory.h"
#include "mouse.h"
#include "qui.h"
#include "video.h"

/*
 Simple system to allow chosing from lists of stuff, with descriptions.
*/

typedef struct
{
  char value[128];
  char desc[256];
} listitem_t;

typedef struct
{
  char* name;
  int common_len;
  listitem_t* li;
  int num;
} list_t;

// list caching

static list_t** lists = NULL;
static int n_lists = 0;

static list_t*
LoadList(char* name)
{
  int i;
  list_t* l;
  list_t** ll;
  FILE* f;
  char v[128], desc[256];
  char filename[256];

  for (i = 0; i < n_lists; i++)
  {
    if (!strcmp(lists[i]->name, name))
      return lists[i];
  }

  l = Q_malloc(sizeof(list_t));
  if (!l)
  {
    HandleError("LoadList", "Out of memory!");
    return NULL;
  }

  ll = Q_realloc(lists, sizeof(list_t*) * (n_lists + 1));
  if (!ll)
  {
    HandleError("LoadList", "Out of memory!");
    return NULL;
  }
  lists = ll;
  lists[n_lists] = l;
  n_lists++;

  l->name = Q_strdup(name);
  if (!l->name)
  {
    n_lists--;
    HandleError("LoadList", "Out of memory!");
    return NULL;
  }

  FindFile(filename, name);
  f = fopen(filename, "rt");
  if (!f)
  {
    n_lists--;
    HandleError("LoadList", "Couldn't load '%s'!", name);
    return NULL;
  }

  l->li = NULL;
  l->num = 0;

  while (!feof(f))
  {
    if (!fgets(v, sizeof(v), f))
      break;

    if (!fgets(desc, sizeof(desc), f))
      break;

#define ClrStr(x)                          \
  while ((x[0] <= 32) && (x[0]))           \
    strcpy(x, &x[1]);                      \
  while ((x[strlen(x) - 1] <= 32) && x[0]) \
    x[strlen(x) - 1] = 0;

    ClrStr(v);
    ClrStr(desc);

    if (!v[0])
      break;

    l->li = Q_realloc(l->li, sizeof(listitem_t) * (l->num + 1));
    if (!l->li)
    {
      n_lists--;
      fclose(f);
      HandleError("LoadList", "Out of memory!");
      return NULL;
    }

    strcpy(l->li[l->num].value, v);
    strcpy(l->li[l->num].desc, desc);

    l->num++;
  }

  fclose(f);

  {
    char* common;
    int i, j, len;
    char *c, *d;

    common = l->li[0].value;
    len = strlen(common);
    for (i = 1; i < l->num; i++)
    {
      for (j = len, c = common, d = l->li[i].value; j; j--)
        if (*c++ != *d++)
          break;
      if (j)
        len = len - j;
      if (!len)
        break;
    }

    if (i == 1)
      l->common_len = 0;
    else
      l->common_len = len;
  }

  return l;
}

#define MAX_ROWS 20
int
PickFromList(char* buf, char* listfile)
{
  list_t* l;
  int bp;
  int b_ok, b_cancel;
  int b_up, b_dn;

  int y, sel;
  int update;

  int x1, y1, x2, y2, x3;

  QUI_window_t* w;
  unsigned char* temp_buf;

  int i, c;

  int ROWS;

  l = LoadList(listfile);
  if (!l)
    return 0;

  ROWS = l->num;
  if (ROWS > MAX_ROWS)
    ROWS = MAX_ROWS;

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];

  w->size.x = video.ScreenWidth - 10;
  w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);

  w->size.y = ROWS * 16 + 70;
  w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Pick from list", &temp_buf);
  Q.num_popups++;

  PushButtons();

  b_ok = AddButtonText(0, 0, B_ENTER, "OK");
  b_cancel = AddButtonText(0, 0, B_ESCAPE, "Cancel");

  button[b_ok].x = w->pos.x + 4;
  button[b_cancel].x = button[b_ok].x + button[b_ok].sx + 4;

  button[b_ok].y = button[b_cancel].y =
    w->pos.y + w->size.y - button[b_ok].sy - 4;

  b_up = AddButtonPic(0, 0, B_RAPID, "button_tiny_up");
  b_dn = AddButtonPic(0, 0, B_RAPID, "button_tiny_down");

  x1 = w->pos.x + 4;
  x2 = w->pos.x + w->size.x - button[b_up].sx - 5;
  y1 = w->pos.y + 32;
  y2 = y1 + 4 + ROWS * 16;
  x3 = x1 + 200;

  button[b_up].x = button[b_dn].x = x2 + 2;

  button[b_up].y = y1;
  button[b_dn].y = y2 - button[b_dn].sy;

  DrawButtons();

  sel = -1;
  y = 0;
  update = 1;

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  do
  {
    if (update)
    {
      for (i = y1; i < y2; i++)
        DrawLine(x1, i, x2, i, BG_COLOR);

      QUI_Frame(x1, y1, x3, y2);
      QUI_Frame(x3, y1, x2, y2);

      for (i = 0; i < ROWS; i++)
      {
        if (i + y >= l->num)
          break;

        if (i + y == sel)
          c = 15;
        else
          c = 0;

        QUI_DrawStrM(x1 + 2, y1 + 2 + i * 16, x3 - 2, BG_COLOR, c, 0, 0, "%s", &l->li[i + y].value[l->common_len]);

        QUI_DrawStrM(x3 + 2, y1 + 2 + i * 16, x2 - 2, BG_COLOR, c, 0, 0, "%s", l->li[i + y].desc);
      }

      RefreshPart(x1, y1, x2, y2);
      update = 0;
    }

    UpdateMouse();
    bp = UpdateButtons();

    if (bp == b_ok)
    {
      if (sel == -1)
        HandleError("PickFromList", "Nothing selected!");
      else
        break;
    }

    if (bp == b_cancel)
      break;

    if (bp == b_up)
    {
      if (y > 0)
      {
        y--;
        update = 1;
      }
    }

    if (bp == b_dn)
    {
      if (y + ROWS < l->num)
      {
        y++;
        update = 1;
      }
    }

    if ((bp == -1) && (mouse.button & 1))
    {
      if ((mouse.x > x1) && (mouse.x < x2) &&
          (mouse.y > y1 + 2) && (mouse.y < y2 - 2))
      {
        sel = (mouse.y - y1 - 2) / 16;
        sel += y;

        if (sel >= l->num)
          sel = -1;

        update = 1;
      }
    }
  } while (1);

  /* Pop down the window (also copies back whats behind the window) */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

  RemoveButton(b_ok);
  RemoveButton(b_cancel);
  RemoveButton(b_up);
  RemoveButton(b_dn);
  PopButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  DrawMouse(mouse.x, mouse.y);

  if (bp == b_ok)
  {
    strcpy(buf, l->li[sel].value);
    return 1;
  }

  return 0;
}
