/*
file.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DJGPP
#include <conio.h>
#include <dos.h>
#endif

#include "defines.h"
#include "types.h"

#include "file.h"

#include "color.h"
#include "error.h"
#include "filedir.h"
#include "keyboard.h"
#include "memory.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "video.h"

/* These no longer do mallocing, so you must do it before you call */
/* TODO : Make these malloc instead of doing a big, constant malloc
          in popupwin.c */
void
DirList(char** Files, int* NumDirs)
{
  int Total = 0;
  int flag;
  int i;
  char* temp;

  struct directory_s* d;
  filedir_t f;

#ifdef DJGPP
  int olddrive, curdrive;
  int totaldrives;
#endif

  d = DirOpen("*", FILE_DIREC);
  if (!d)
  {
    *NumDirs = 0;
    return;
  }

  while (DirRead(d, &f))
  {
    Files[Total] = Q_malloc(strlen(f.name) + 1);
    strcpy(Files[Total], f.name);
    Total++;
  }

  DirClose(d);

  if (Total > 1)
  {
    do
    {
      flag = FALSE;
      for (i = 0; i < Total - 1; i++)
      {
        if (stricmp(Files[i], Files[i + 1]) > 0)
        {
          flag = TRUE;
          temp = Files[i];
          Files[i] = Files[i + 1];
          Files[i + 1] = temp;
        }
      }
    } while (flag == TRUE);
  }

#ifdef DJGPP
  _dos_getdrive(&olddrive);
  for (i = 1; i <= 26; i++)
  {
    _dos_setdrive(i, &totaldrives);
    _dos_getdrive(&curdrive);
    if (curdrive == i)
    {
      Files[Total] = (char*)Q_malloc(4);
      Files[Total][0] = '[';
      Files[Total][1] = curdrive - 1 + 'a';
      Files[Total][2] = ']';
      Files[Total][3] = 0;
      Total++;
    }
  }
  _dos_setdrive(olddrive, &totaldrives);
#endif

  *NumDirs = Total;
}

void
FileList(char** Files, char* mask, int* NumFiles)
{
  int Total = 0;
  int flag;
  char* temp;
  int i;

  struct directory_s* d;
  filedir_t f;

  d = DirOpen(mask, FILE_NORMAL);
  if (!d)
  {
    *NumFiles = 0;
    return;
  }

  while (DirRead(d, &f))
  {
    Files[Total] = Q_malloc(strlen(f.name) + 1);
    strcpy(Files[Total], f.name);
    Total++;
  }

  DirClose(d);

  if (Total > 1)
  {
    do
    {
      flag = FALSE;
      for (i = 0; i < Total - 1; i++)
      {
        if (stricmp(Files[i], Files[i + 1]) > 0)
        {
          flag = TRUE;
          temp = Files[i];
          Files[i] = Files[i + 1];
          Files[i + 1] = temp;
        }
      }
    } while (flag);
  }

  *NumFiles = Total;
}

static int
isgoodfilechar(int c)
{
  switch (c)
  {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '0' ... '9':
    case '/':
    case ':':
    case '.':
    case '_':
      return 1;
  }
  return 0;
}

static int
isgoodtexchar(int c)
{
  switch (c)
  {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '0' ... '9':
    case '_':
    case '+':
    case '*':
    case '.':
    case '-':
    case ' ':
    case '/':
    case '\\':
      return 1;
  }
  return 0;
}

static int
isgoodgroupchar(int c)
{
  switch (c)
  {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '0' ... '9':
    case ' ':
    case '-':
      return 1;
  }
  return 0;
}

int
readstring(char* string, int x, int y, int maxx, int maxlen, int (*isgood)(int c))
/* returns FALSE if the entry was aborted with the escape key
   returns TRUE  if the entry was completed with the enter key */
{
  int ch;
  char *str, *c, *d;
  font_t* f;
  int pos, sel, scroll;
  int i, j, k, l;
  int cx;

  maxlen--;

  ClearKeys();

  str = (char*)Q_malloc(maxlen + 1);
  if (!str)
  {
    HandleError("readstring", "Out of memory!");
    return FALSE;
  }
  strcpy(str, string);

  f = &Q.font[0];

  scroll = 0;
  sel = 0;
  pos = strlen(str);

  do
  {
    if (sel == pos)
      sel = -1;

    if (scroll >= pos)
    {
      scroll = pos - 3;
      if (scroll < 0)
        scroll = 0;
    }
    else if (QUI_strlen(0, str) < maxx - x)
    {
      scroll = 0;
    }
    else
    {
      c = &str[pos];
      if (*c)
        c++;
      if (*c)
        c++;
      if (*c)
        c++;

      for (j = maxx - x; c > str; c--)
      {
        if (f->map[(int)*c] == -1)
          i = 8;
        else
          i = f->width[f->map[(int)*c]];
        if (j - i < 0)
          break;
        j -= i;
      }
      scroll = c - str;
    }

    DrawSolidBox(x, y, maxx, y + ROM_CHAR_HEIGHT, GetColor(BG_COLOR));

    cx = x;
    for (i = scroll, j = x, c = &str[scroll]; *c; c++, i++)
    {
      if (f->map[(int)*c] == -1)
        l = 8;
      else
        l = f->width[f->map[(int)*c]];

      if (j + l >= maxx)
        break;

      k = 15;
      if (sel != -1 && ((i >= sel && i < pos) || (i < sel && i >= pos)))
      {
        ch = COL_BLUE - 6;
      }
      else
      {
        ch = BG_COLOR;
      }
      QUI_DrawChar(j, y, ch, k, 0, 0, *c);
      j += l;
      if (i + 1 == pos)
        cx = j;
    }
    if (cx >= maxx - 1)
      cx--;
    DrawSolidBox(cx, y, cx + 2, y + ROM_CHAR_HEIGHT, GetColor(COL_YELLOW));
    RefreshPart(x, y, maxx, y + ROM_CHAR_HEIGHT);

    ch = GetKeyB();

    switch (ch)
    {
      case KEY_LEFT + KS_SHIFT:
      case KEY_LEFT:
        if (pos > 0)
        {
          if (ch & KS_SHIFT)
          {
            if (sel == -1)
              sel = pos;
          }
          else
            sel = -1;
          pos--;
        }
        break;

      case KEY_RIGHT + KS_SHIFT:
      case KEY_RIGHT:
        if (pos < strlen(str))
        {
          if (ch & KS_SHIFT)
          {
            if (sel == -1)
              sel = pos;
          }
          else
            sel = -1;
          pos++;
        }
        break;

      case KEY_DELETE:
        if (sel == -1)
        {
          for (i = pos; i < strlen(str); i++)
            str[i] = str[i + 1];
          break;
        }
        /* fall-thru */
      case KEY_BACKSPC:
        if (sel == -1)
        {
          if (!pos)
            break;
          pos--;
          for (i = pos; i < strlen(str); i++)
            str[i] = str[i + 1];
        }
        else
        {
          if (sel < pos)
          {
            i = sel;
            sel = pos;
            pos = i;
          }
          i = sel - pos;
          for (k = sel; k <= strlen(str); k++)
            str[k - i] = str[k];
          sel = -1;
        }
        break;

      default:
        if (ch & KS_CHAR)
        {
          ch &= 0xff;

          if (ch >= 128)
            break;
          if ((f->map[ch] == -1) && (ch != ' '))
            break;

          if (isgood)
            if (!isgood(ch))
              break;

          if (sel == -1)
          {
            if (strlen(str) < maxlen)
            {
              for (i = strlen(str); i >= pos; i--)
                str[i + 1] = str[i];
              str[i + 1] = ch;
              pos++;
            }
          }
          else
          {
            if (sel < pos)
            {
              i = pos;
              pos = sel;
              sel = i;
            }
            str[pos] = ch;
            pos++;
            j = strlen(str);
            k = sel - pos;
            if (k)
              for (i = sel; i < j + 1; i++)
                str[i - k] = str[i];
          }
          sel = -1;
        }
        break;
    }
  } while ((ch != KEY_ENTER) && (ch != KEY_ESCAPE));

  DrawSolidBox(x, y, maxx, y + ROM_CHAR_HEIGHT, GetColor(BG_COLOR));

  if (ch == KEY_ENTER)
  {
    while (TestKey(KEY_ENTER))
      UpdateMouse();
    strcpy(string, str);
    QUI_DrawStrM(x, y, maxx, BG_COLOR, 0, 0, 0, str);
    RefreshPart(x, y, maxx, y + ROM_CHAR_HEIGHT);
    Q_free(str);
    return TRUE;
  }
  else
  {
    while (TestKey(KEY_ESCAPE))
      UpdateMouse();
    QUI_DrawStrM(x, y, maxx, BG_COLOR, 0, 0, 0, string);
    RefreshPart(x, y, maxx, y + ROM_CHAR_HEIGHT);
    Q_free(str);
    return FALSE;
  }
}

int
readgname(char* string, int x, int y, int maxx, int maxlen)
{
  return readstring(string, x, y, maxx, maxlen, isgoodgroupchar);
}

int
readfname(char* string, int x, int y, int maxx, int maxlen)
{
  return readstring(string, x, y, maxx, maxlen, isgoodfilechar);
}

int
readtname(char* string, int x, int y, int maxx, int maxlen)
{
  return readstring(string, x, y, maxx, maxlen, isgoodtexchar);
}

void
FindFile(char* fullname, const char* org_name)
{
  char name[1024];
  FILE* f;

  strcpy(name, org_name);
  strcpy(fullname, name);

  f = fopen(name, "rb");
  if (f)
  {
    fclose(f);
    return;
  }

#ifdef _UNIX
//  printf("Using compiled-in quest path for '%s'.\n", filename);
#define QUESTDIR "/usr/local/quest/"
  sprintf(name, "%s%s", QUESTDIR, org_name);
  strcpy(fullname, name);

  f = fopen(name, "rb");
  if (f)
  {
    fclose(f);
    return;
  }
#endif

  strcpy(name, argv[0]);
  if (!strrchr(name, '/'))
    return;
  *(strrchr(name, '/') + 1) = 0;

  strcat(name, org_name);
  strcpy(fullname, name);
  /*
     f=fopen(name,"rb");
     if (f)
     {
        fclose(f);
        return;
     }*/
}
