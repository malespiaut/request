/*
menu.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"

#include "defines.h"
#include "types.h"

#include "menu.h"

#include "3d.h"
#include "cfgs.h"
#include "display.h"
#include "dvport.h"
#include "error.h"
#include "file.h"
#include "memory.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "token.h"
#include "video.h"

/* The menu stuff */
menu_t* Menus;
int NumMenus;
list_t* List;
int ListSize;

/*local to this module, globals*/

static int CurLight;
static int OldLight;
static int LastSelection;

void
PopUpMenuWin(void)
{
  int y;
  int old, i;

  for (y = Q.window[MAP_WINDOW].pos.y; y < Q.window[MAP_WINDOW].pos.y + Q.window[MAP_WINDOW].size.y; y++)
    memset(&(video.ScreenBuffer[y * video.ScreenWidth]), 0, video.ScreenWidth);
  Q.window[MENU_WINDOW].pos.x = video.ScreenWidth - 160;
  Q.window[MENU_WINDOW].size.x = 159;
  MenuShowing = TRUE;

  Q.window[MAP_WINDOW].pos.x = 0;
  Q.window[MAP_WINDOW].size.x = video.ScreenWidth - 160;

  old = cur_map;

  for (i = 0; i < MAX_MAPS; i++)
  {
    SwitchMap(i, 0);
    UpdateViewportPositions();
  }
  SwitchMap(old, 0);

  QUI_RedrawWindow(MENU_WINDOW);
  ListSize = 0;

  MenuToList(Menus, &List, 0, 0, &ListSize);

  DepthSort(&List, ListSize);
  DisplayList(Menus, List, ListSize);
  OldLight = -1;
  LastSelection = -1;
  CurLight = 0;
  UpdateAllViewports();
}

void
PopDownMenuWin(void)
{
  int y;
  int old, i;

  for (y = Q.window[MAP_WINDOW].pos.y; y < Q.window[MAP_WINDOW].pos.y + Q.window[MAP_WINDOW].size.y; y++)
    memset(&(video.ScreenBuffer[y * video.ScreenWidth]), 0, video.ScreenWidth);
  Q.window[MENU_WINDOW].pos.x = video.ScreenWidth;
  Q.window[MENU_WINDOW].size.x = 1;
  MenuShowing = FALSE;
  Q.window[MAP_WINDOW].pos.x = 0;
  Q.window[MAP_WINDOW].size.x = video.ScreenWidth;

  old = cur_map;

  for (i = 0; i < MAX_MAPS; i++)
  {
    SwitchMap(i, 0);
    UpdateViewportPositions();
  }
  SwitchMap(old, 0);

  QUI_RedrawWindow(MENU_WINDOW);
  UpdateAllViewports();
}

void
MenuToList(menu_t* M, list_t** L, int location, int indent, int* ListSize)
{
  int x;

  x = location;
  while (!(x == -1))
  {
    if (M[x].flag == 1)
    {
      strcpy((char*)&((*L)[*ListSize].title), (char*)&(M[x].title));
      (*L)[*ListSize].menu_num = x;
      (*L)[*ListSize].depth = indent;
      (*ListSize)++;
      if ((M[x].showkids == 1) && (M[x].numkids != 0))
        MenuToList(M, L, x + 1, indent + 1, ListSize);
    }
    x = M[x].next;
  }
}

void
DepthSort(list_t** L, int ListSize)
{
  int x, flag;
  list_t temp;

  do
  {
    flag = 0;
    for (x = 0; x < ListSize - 1; x++)
      if ((*L)[x].depth > (*L)[x + 1].depth)
      {
        temp = (*L)[x + 1];
        (*L)[x + 1] = (*L)[x];
        (*L)[x] = temp;
        flag = 1;
      }
  } while (flag == 1);
}

void
DisplayList(menu_t* M, list_t* L, int ListSize)
{
  int x;

  for (x = 0; x < ListSize; x++)
    QUI_DrawStr(L[x].depth * ROM_CHAR_WIDTH + Q.window[MENU_WINDOW].pos.x + 6,
                x * ROM_CHAR_HEIGHT + Q.window[MENU_WINDOW].pos.y + 6,
                BG_COLOR,
                M[L[x].menu_num].color,
                0,
                0,
                L[x].title);
  /*
    TH 7-9-96: list draws are now done only to the vid buffer
         and then copied using RefreshPart
  */
}

void
EraseList(menu_t* M, list_t* L, int ListSize)
{
  int x;

  for (x = 0; x < ListSize; x++)
    QUI_DrawStr(L[x].depth * ROM_CHAR_WIDTH + Q.window[MENU_WINDOW].pos.x + 6,
                x * ROM_CHAR_HEIGHT + Q.window[MENU_WINDOW].pos.y + 6,
                BG_COLOR,
                BG_COLOR,
                0,
                0,
                L[x].title);
}

static void
HighLight(menu_t* M, list_t* L, int Draw, int Erase, int ListSize)
{
  if ((Erase < ListSize) && (Erase >= 0))
    QUI_DrawStr(L[Erase].depth * ROM_CHAR_WIDTH + Q.window[MENU_WINDOW].pos.x + 6,
                Erase * ROM_CHAR_HEIGHT + Q.window[MENU_WINDOW].pos.y + 6,
                BG_COLOR,
                M[L[Erase].menu_num].color,
                0,
                1,
                L[Erase].title);

  if ((Draw < ListSize) && (Draw >= 0))
    QUI_DrawStr(L[Draw].depth * ROM_CHAR_WIDTH + Q.window[MENU_WINDOW].pos.x + 6,
                Draw * ROM_CHAR_HEIGHT + Q.window[1].pos.y + 6,
                BG_COLOR,
                11,
                0,
                1,
                L[Draw].title);

  DrawMouse(mouse.x, mouse.y);
}

static void
UnFlag(menu_t** M, int start)
{
  if ((start != -1) && ((*M)[start].numkids == 0))
    return;
  start = start + 1;
  do
  {
    (*M)[start].showkids = 0;
    (*M)[start].color = 0;
    start = (*M)[start].next;
  } while (!(start == -1));
}

static int
Flag(menu_t** M, int start)
{
  if (start == -1)
    return 0;
  (*M)[start].flag = 1;
  (*M)[start].showkids = 1;
  if ((*M)[start].numkids == 0)
    return 1;
  (*M)[start].color = 15;
  start = start + 1;
  do
  {
    (*M)[start].flag = 1;
    start = (*M)[start].next;
  } while (!(start == -1));
  return 0;
}

static int
ProcessSelection(menu_t** M, list_t** L, int* ListSize, int Selection, int exec)
{
  QUI_window_t* w;

  if (Selection < (*ListSize))
  {
    UnFlag(M, (*M)[(*L)[Selection].menu_num].parent);
    if (Flag(M, (*L)[Selection].menu_num) == 1)
    {
      if (exec)
      {
        Menu_Exe((*M)[(*L)[Selection].menu_num].command);
      }
      else
      {
        return (*M)[(*L)[Selection].menu_num].command;
      }
    }
    else
    {
      if (Selection == LastSelection)
        return -1;
      LastSelection = Selection;
      EraseList(*M, *L, *ListSize);
      *ListSize = 0;
      MenuToList(*M, L, 0, 0, ListSize);
      DepthSort(L, *ListSize);
      DisplayList(*M, *L, *ListSize);
      w = &Q.window[MENU_WINDOW];
      WaitRetr();
      RefreshPart(w->pos.x, w->pos.y - 1, w->pos.x + w->size.x, w->pos.y + w->size.y);
      DrawMouse(mouse.x, mouse.y);
    }
  }
  return -1;
}

int
UpdateMenu(menu_t** M, list_t** L, int* ListSize)
{
  CurLight = (mouse.y - Q.window[MENU_WINDOW].pos.y - 1) / ROM_CHAR_HEIGHT;

  if (CurLight != OldLight)
    HighLight(*M, *L, CurLight, OldLight, *ListSize);

  if (mouse.button != 0)
  {
    ProcessSelection(M, L, ListSize, CurLight, 1);
  }

  /* Wait for mouse button to be released */
  while (mouse.button != 0)
  {
    GetMousePos();
    if (mouse.moved)
    {
      WaitRetr();
      UndrawMouse(mouse.prev_x, mouse.prev_y);
      DrawMouse(mouse.x, mouse.y);
    }
  }

  OldLight = CurLight;

  return (mouse.button != 0);
}

int
LoadMenu(const char* filename, menu_t** M, int* NumMenus, list_t** L, int* ListSize)
{
  char name[256];
  int level, olevel;
  menu_t* m;
  int i;

  FindFile(name, filename);
  if (!TokenFile(name, T_MISC | T_STRING | T_NAME, NULL))
    return 0;

  *M = NULL;
  *NumMenus = 0;

  olevel = 0;
  while (TokenGet(1, -1))
  {
    *M = Q_realloc(*M, sizeof(menu_t) * (*NumMenus + 1));
    if (!*M)
      return 0;
    m = &((*M)[*NumMenus]);
    memset(m, 0, sizeof(menu_t));

    i = *NumMenus;
    level = -1;
    while (!strcmp(token, "@"))
    {
      level++;
      if (!TokenGet(0, -1))
        return 0;
    }
    if (level < 0)
      return 0;

    if (token[0] != '"')
      return 0;

    strcpy(token, &token[1]);
    token[strlen(token) - 1] = 0;
    strcpy(m->title, token);

    if (TokenAvailable(0))
    {
      if (!TokenGet(0, -1))
        return 0;
      if (strcmp(token, "="))
        return 0;
      if (!TokenGet(0, -1))
        return 0;
      m->command = FindDef(cmds, token);
    }
    else
    {
      m->command = -1;
    }

    if (!level)
      m->flag = 1;

    m->next = m->parent = -1;

    if (i)
    {
      if (level == olevel)
      {
        m->parent = (*M)[i - 1].parent;
        (*M)[i - 1].next = i;
      }
      else if (level > olevel)
      {
        m->parent = i - 1;
      }
      else if (level < olevel)
      {
        m->parent = i - 1;
        for (; olevel > level; olevel--)
          m->parent = (*M)[m->parent].parent;

        (*M)[m->parent].next = i;
        m->parent = (*M)[m->parent].parent;
      }

      if (m->parent != -1)
        (*M)[m->parent].numkids++;
    }

    olevel = level;

    (*NumMenus)++;
  }

  TokenDone();

  /*   {
        int i;
        menu_t *m;

        for (i=0;i<*NumMenus;i++)
        {
           m=&((*M)[i]);
           printf("%2i %20s %3i %3i %3i %3i %3i %3i\n",
              i,m->title,m->parent,m->numkids,m->next,m->flag,m->color,
              m->showkids);
        }
     }*/

  *L = (list_t*)Q_malloc(sizeof(list_t) * (*NumMenus));
  if (*L == NULL)
    return FALSE;

  if (MenuShowing)
  {
    *ListSize = 0;
    MenuToList(*M, L, 0, 0, ListSize);
    DepthSort(L, *ListSize);
    DisplayList(*M, *L, *ListSize);
    OldLight = -1;
    LastSelection = -1;
    CurLight = 0;
  }
  return 1;
}

void
Menu_Exe(int command)
{
  int redraw;

  redraw = ExecCmd(command);

  if (redraw == 1)
    UpdateViewport(M.display.active_vport, TRUE);
  if (redraw == 2)
    UpdateAllViewports();

  if (redraw)
  {
    DrawMouse(mouse.x, mouse.y);
  }
}

static void
WriteMenu_R(FILE* f, int w, int l)
{
  int i, j;

  do
  {
    j = 0;
    for (i = 0; i < l; i++)
      fputc(' ', f);
    j = l;
    j += fprintf(f, "%s", Menus[w].title);
    if (Menus[w].command != -1)
    {
      for (; j < 30; j++)
        fputc(' ', f);
      fprintf(f, "%s", GetDefName(cmds, Menus[w].command));
    }
    fprintf(f, "\n");

    if (Menus[w].numkids)
      WriteMenu_R(f, w + 1, l + 3);

    w = Menus[w].next;
  } while (w != -1);
}

void
WriteMenuHelp(void)
{
  FILE* f;

  f = fopen("menu.hlp", "wt");
  if (!f)
    return;

  fprintf(f, "menu.hlp generated by Quest " QUEST_VER "\n");

  WriteMenu_R(f, 0, 0);

  fclose(f);
}

void
MenuPopUp(void)
{
  int bx, by;
  int sx, sy;
  unsigned char* buf;
  int i;
  int cmd;

  bx = mouse.x;
  by = mouse.y;
  sx = 160;
  sy = 400;

  if (bx < 0)
    bx = 0;
  if (by < 0)
    by = 0;
  if (bx + sx >= video.ScreenWidth)
    bx = video.ScreenWidth - sx;
  if (by + sy >= video.ScreenHeight)
    by = video.ScreenHeight - sy;

  Q.window[MENU_WINDOW].pos.x = bx;
  Q.window[MENU_WINDOW].size.x = sx - 1;

  Q.window[MENU_WINDOW].pos.y = by;
  Q.window[MENU_WINDOW].size.y = sy - 1;

  buf = Q_malloc(sx * sy);
  if (!buf)
  {
    HandleError("MenuPopUp", "Out of memory!");
    return;
  }

  for (i = 0; i < sy; i++)
  {
    memcpy(&buf[i * sx],
           &video.ScreenBuffer[bx + (by + i) * video.ScreenWidth],
           sx);
  }

  MenuShowing = TRUE;

  QUI_RedrawWindow(MENU_WINDOW);
  ListSize = 0;

  MenuToList(Menus, &List, 0, 0, &ListSize);

  DepthSort(&List, ListSize);
  DisplayList(Menus, List, ListSize);
  OldLight = -1;
  LastSelection = -1;
  CurLight = 0;
  cmd = -1;

  RefreshScreen();
  do
  {
    UpdateMouse();
    if (mouse.button & 6)
      break;

    if (InBox(bx, by, bx + sx, by + sy))
    {
      CurLight = (mouse.y - Q.window[MENU_WINDOW].pos.y - 1) / ROM_CHAR_HEIGHT;

      if (CurLight != OldLight)
        HighLight(Menus, List, CurLight, OldLight, ListSize);

      if (mouse.button != 0)
      {
        cmd = ProcessSelection(&Menus, &List, &ListSize, CurLight, 0);
      }

      if (cmd != -1)
      {
        /* Wait for mouse button to be released */
        while (mouse.button != 0)
        {
          GetMousePos();
          if (mouse.moved)
          {
            WaitRetr();
            UndrawMouse(mouse.prev_x, mouse.prev_y);
            DrawMouse(mouse.x, mouse.y);
          }
        }
      }

      OldLight = CurLight;
    }
    else if (mouse.button)
    {
      break;
    }
  } while (cmd == -1);

  while (mouse.button)
    UpdateMouse();

  Q.window[MENU_WINDOW].pos.x = video.ScreenWidth;
  Q.window[MENU_WINDOW].size.x = 1;
  Q.window[MENU_WINDOW].pos.y = 30;
  Q.window[MENU_WINDOW].size.y = video.ScreenHeight - 100 - 1;

  MenuShowing = FALSE;

  for (i = 0; i < sy; i++)
  {
    memcpy(&video.ScreenBuffer[bx + (by + i) * video.ScreenWidth],
           &buf[i * sx],
           sx);
  }
  Q_free(buf);

  RefreshScreen();

  if (cmd != -1)
    Menu_Exe(cmd);
}
