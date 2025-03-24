/*
qui.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DJGPP
#include <conio.h>
#endif

#include "defines.h"
#include "types.h"

#include "qui.h"

#include "brush.h"
#include "button.h"
#include "color.h"
#include "display.h"
#include "entity.h"
#include "error.h"
#include "file.h"
#include "keyboard.h"
#include "memory.h"
#include "menu.h"
#include "message.h"
#include "mouse.h"
#include "quest.h"
#include "status.h"
#include "tool.h"
#include "video.h"

QUI_t Q; /* Basic Quest User Interface element */

/* TODO: get rid of bad ways of doing stuff */
char oldtexname[256]; // bad way of remembering the last texture string
char oldentstr[256];  // another bad way of remembering the last entity
                     // string

void
QUI_Box(int x1, int y1, int x2, int y2, int col1, int col2)
{
  /* Draw to Buffer*/
  DrawLine(x1, y1, x2, y1, GetColor(col1));
  DrawLine(x1, y2, x2, y2, GetColor(col2));
  DrawLine(x1, y1, x1, y2, GetColor(col1));
  DrawLine(x2, y1, x2, y2, GetColor(col2));
}

void
QUI_Frame(int x1, int y1, int x2, int y2)
{
  DrawLine(x1, y1, x2, y1, GetColor(4));
  DrawLine(x1, y1, x1, y2, GetColor(4));
  DrawLine(x2, y2, x2, y1, GetColor(8));
  DrawLine(x2, y2, x1, y2, GetColor(8));
}

int
QUI_strlen(int font, const char* string)
{
  int Length = 0;
  font_t* f = &Q.font[font];
  const char* c;

  for (c = string; *c; c++)
  {
    if (*c == ' ')
      Length += 8;
    else if (f->map[(unsigned char)*c] != -1)
      Length += f->width[f->map[(unsigned char)*c]];
  }
  return Length;
}

static void
QUI_DrawChar_Int(int x, int y, int bg, int fg, int font, int to_screen, char c)
{
  int i;
  unsigned int j;
  int c_num;
  unsigned char* scr;
  unsigned char* data;
  int width;
  font_t* f;

  f = &Q.font[font];
  c_num = f->map[(unsigned char)c];

  if (c_num == -1)
  {
    if (bg != -1)
    {
      scr = &video.ScreenBuffer[y * video.ScreenWidth + x];
      j = (bg) +
          (bg << 8) +
          (bg << 16) +
          (bg << 24);
      for (i = f->data_height; i; i--)
      {
        *((unsigned int*)(scr)) = j;
        *((unsigned int*)(scr + 4)) = j;
        scr += video.ScreenWidth;
      }

      if (to_screen == TRUE)
        RefreshPart(x, y, x + 8, y + f->data_height);
    }
    return;
  }

  data = &f->data[f->div[c_num]];
  scr = &video.ScreenBuffer[y * video.ScreenWidth + x];
  width = f->width[c_num];

  if (bg == -1)
  {
    for (i = f->data_height; i; i--)
    {
      for (j = width; j; j--)
      {
        if (*data)
        {
          *scr = fg;
        }

        data++;
        scr++;
      }

      scr += video.ScreenWidth - width;
      data += f->data_width - width;
    }
  }
  else
  {
    for (i = f->data_height; i; i--)
    {
      for (j = width; j; j--)
      {
        if (*data)
        {
          *scr = fg;
        }
        else
        {
          *scr = bg;
        }
        data++;
        scr++;
      }
      scr += video.ScreenWidth - width;
      data += f->data_width - width;
    }
  }

  if (to_screen == TRUE)
    RefreshPart(x, y, x + width, y + f->data_height);
}

void
QUI_DrawChar(int x, int y, int bg, int fg, int font, int to_screen, char c)
{
  QUI_DrawChar_Int(x, y, GetColor(bg), GetColor(fg), font, to_screen, c);
}

static void
QUI_DrawString(int x, int y, int maxx, int bg, int fg, int font, int to_screen, const char* str)
{
  int pos = x;
  font_t* f = &Q.font[font];
  int len, clen;
  const char* c;

  static const char* dot = "...";

  bg = GetColor(bg);
  fg = GetColor(fg);

  if (x + QUI_strlen(font, str) < maxx)
    len = 0;
  else
    len = QUI_strlen(font, dot);

  for (c = str; *c; c++)
  {
start:
    switch (*c)
    {
      case ' ':
        clen = 8;
        break;
      default:
        clen = f->width[f->map[(unsigned char)*c]];
        break;
    }

    if (len)
    {
      if (len + clen + pos >= maxx)
      {
        c = dot;
        len = 0;
        goto start;
      }
    }

    QUI_DrawChar_Int(pos, y, bg, fg, font, 0, *c);

    pos += clen;
  }
  if (to_screen)
    RefreshPart(x, y, pos, y + ROM_CHAR_HEIGHT);
}

void
QUI_DrawStr(int x, int y, int bg, int fg, int font, int to_screen, const char* format, ...)
{
  char str[1024];
  va_list args;

  va_start(args, format);
  vsprintf(str, format, args);
  va_end(args);

  QUI_DrawString(x, y, video.ScreenWidth, bg, fg, font, to_screen, str);
}

void
QUI_DrawStrM(int x, int y, int maxx, int bg, int fg, int font, int to_screen, const char* format, ...)
{
  char str[1024];
  va_list args;

  va_start(args, format);
  vsprintf(str, format, args);
  va_end(args);

  QUI_DrawString(x, y, maxx, bg, fg, font, to_screen, str);
}

void
QUI_InitWindow(int win)
{
  QUI_window_t* w;
  int i, j;

  /*Draw Menu area*/
  w = &Q.window[win];

  for (i = 0, j = w->pos.y; i < w->size.y; i++, j++)
    memset(&video.ScreenBuffer[(j * video.ScreenWidth) + w->pos.x], GetColor(BG_COLOR), w->size.x);

  DrawLine(w->pos.x, w->pos.y, w->pos.x + w->size.x - 1, w->pos.y, GetColor(10));
  DrawLine(w->pos.x, w->pos.y, w->pos.x, w->pos.y + w->size.y - 1, GetColor(10));
  DrawLine(w->pos.x + 1, w->pos.y + 1, w->pos.x + w->size.x - 1, w->pos.y + 1, GetColor(8));
  DrawLine(w->pos.x + 1, w->pos.y + 1, w->pos.x + 1, w->pos.y + w->size.y - 1, GetColor(8));
  DrawLine(w->pos.x + w->size.x - 2, w->pos.y, w->pos.x + w->size.x - 2, w->pos.y + w->size.y - 1, GetColor(4));
  DrawLine(w->pos.x, w->pos.y + w->size.y - 2, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 2, GetColor(4));
  DrawLine(w->pos.x + w->size.x - 1, w->pos.y, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 1, GetColor(3));
  DrawLine(w->pos.x, w->pos.y + w->size.y - 1, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 1, GetColor(3));

  switch (win)
  {
    case MENU_WINDOW:
      break;

    case MESG_WINDOW:
      InitMesgWin();
      break;

    case TOOL_WINDOW:
      break;

    case STATUS_WINDOW:
      QUI_RedrawWindow(STATUS_WINDOW);
      break;
  }
}

void
QUI_RedrawWindow(int win)
{
  QUI_window_t* w;
  int i, j;

  if ((win == MENU_WINDOW) && (!MenuShowing))
    return;
  /*Draw Menu area*/
  w = &Q.window[win];

  for (i = 0, j = w->pos.y; i < w->size.y; i++, j++)
    memset(&video.ScreenBuffer[(j * video.ScreenWidth) + w->pos.x], GetColor(BG_COLOR), w->size.x);

  DrawLine(w->pos.x, w->pos.y, w->pos.x + w->size.x - 1, w->pos.y, GetColor(10));
  DrawLine(w->pos.x, w->pos.y, w->pos.x, w->pos.y + w->size.y - 1, GetColor(10));
  DrawLine(w->pos.x + 1, w->pos.y + 1, w->pos.x + w->size.x - 1, w->pos.y + 1, GetColor(8));
  DrawLine(w->pos.x + 1, w->pos.y + 1, w->pos.x + 1, w->pos.y + w->size.y - 1, GetColor(8));
  DrawLine(w->pos.x + w->size.x - 2, w->pos.y, w->pos.x + w->size.x - 2, w->pos.y + w->size.y - 1, GetColor(4));
  DrawLine(w->pos.x, w->pos.y + w->size.y - 2, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 2, GetColor(4));
  DrawLine(w->pos.x + w->size.x - 1, w->pos.y, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 1, GetColor(3));
  DrawLine(w->pos.x, w->pos.y + w->size.y - 1, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 1, GetColor(3));

  /*	DrawLine(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y,  10);
    DrawLine(w->pos.x, w->pos.y, w->pos.x, w->pos.y + w->size.y,  10);
    DrawLine(w->pos.x+1, w->pos.y+1, w->pos.x + w->size.x, w->pos.y+1, 8);
    DrawLine(w->pos.x+1, w->pos.y+1, w->pos.x+1, w->pos.y + w->size.y, 8);
    DrawLine(w->pos.x + w->size.x-1, w->pos.y-1, w->pos.x + w->size.x-1, w->pos.y + w->size.y,  4);
    DrawLine(w->pos.x, w->pos.y + w->size.y-1, w->pos.x + w->size.x, w->pos.y + w->size.y-1,  4);
    DrawLine(w->pos.x + w->size.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y,  3);
    DrawLine(w->pos.x, w->pos.y + w->size.y, w->pos.x + w->size.x, w->pos.y + w->size.y,  3);*/

  switch (win)
  {
    case MENU_WINDOW:
      QUI_Frame(w->pos.x + 4, w->pos.y + 4, w->pos.x + w->size.x - 5, w->pos.y + w->size.y - 5);

      UpdateMenu(&Menus, &List, &ListSize);
      break;

    case MESG_WINDOW:
      QUI_Frame(w->pos.x + 5, w->pos.y + 5, w->pos.x + w->size.x - 40, w->pos.y + w->size.y - 5);

      DrawButtons();
      DrawMessages();
      break;

    case TOOL_WINDOW:
      DrawButtons();
      break;

    case STATUS_WINDOW:
      DrawBitmap("status_bar_title", w->pos.x + 3, w->pos.y + 4);
      QUI_Frame(w->pos.x + w->size.x - 130, w->pos.y + 4, w->pos.x + w->size.x - 10, w->pos.y + w->size.y - 4);

      switch (status.edit_mode)
      {
        case BRUSH:
          QUI_DrawStr(w->pos.x + w->size.x - 120, w->pos.y + 8, BG_COLOR, 14, 0, 0, "Brush Mode");

          DrawLine(w->pos.x + 100, w->pos.y + 4, w->pos.x + 250, w->pos.y + 4, GetColor(4));
          DrawLine(w->pos.x + 100, w->pos.y + w->size.y - 4, w->pos.x + 250, w->pos.y + w->size.y - 4, GetColor(8));
          DrawLine(w->pos.x + 100, w->pos.y + 4, w->pos.x + 100, w->pos.y + w->size.y - 4, GetColor(4));
          DrawLine(w->pos.x + 250, w->pos.y + 4, w->pos.x + 250, w->pos.y + w->size.y - 4, GetColor(8));

          DrawLine(w->pos.x + 260, w->pos.y + 4, w->pos.x + w->size.x - 150, w->pos.y + 4, GetColor(4));
          DrawLine(w->pos.x + 260, w->pos.y + w->size.y - 4, w->pos.x + w->size.x - 150, w->pos.y + w->size.y - 4, GetColor(8));
          DrawLine(w->pos.x + 260, w->pos.y + 4, w->pos.x + 260, w->pos.y + w->size.y - 4, GetColor(4));
          DrawLine(w->pos.x + w->size.x - 150, w->pos.y + 4, w->pos.x + w->size.x - 150, w->pos.y + w->size.y - 4, GetColor(8));

          if (texturename[0] != 0)
          {
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, BG_COLOR, 0, 0, oldtexname);
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, 0, 0, 0, texturename);
            strcpy(oldtexname, texturename);
          }
          else
          {
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, BG_COLOR, 0, 0, oldtexname);
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, 0, 0, 0, "No Texture");
            oldtexname[0] = 0;
          }

          if (status.rotate != -1)
          {
            QUI_DrawStr(w->pos.x + 266, w->pos.y + 8, BG_COLOR, 0, 0, 0, "%d deg", status.rotate);
          }
          else if (status.scale != -1)
          {
            QUI_DrawStr(w->pos.x + 266, w->pos.y + 8, BG_COLOR, 0, 0, 0, "%d%%", status.scale);
          }
          else if (status.move == TRUE)
          {
            QUI_DrawStr(w->pos.x + 266, w->pos.y + 8, BG_COLOR, 0, 0, 0, "(%d %d %d)", status.move_amt.x, status.move_amt.y, status.move_amt.z);
          }
          else
          {
            if (M.cur_texname[0] != '\0')
            {
              if (M.cur_brush != NULL)
              {
                QUI_DrawStr(w->pos.x + 266, w->pos.y + 8, BG_COLOR, 0, 0, 0, "%s (%2.0f %2.0f %2.0f)", M.cur_texname, M.cur_brush->center.x, M.cur_brush->center.y, M.cur_brush->center.z);
              }
            }
          }
          break;

        case FACE:
          QUI_DrawStr(w->pos.x + w->size.x - 120, w->pos.y + 8, BG_COLOR, 14, 0, 0, "Face Mode");

          DrawLine(w->pos.x + 100, w->pos.y + 4, w->pos.x + 250, w->pos.y + 4, GetColor(4));
          DrawLine(w->pos.x + 100, w->pos.y + w->size.y - 4, w->pos.x + 250, w->pos.y + w->size.y - 4, GetColor(8));
          DrawLine(w->pos.x + 100, w->pos.y + 4, w->pos.x + 100, w->pos.y + w->size.y - 4, GetColor(4));
          DrawLine(w->pos.x + 250, w->pos.y + 4, w->pos.x + 250, w->pos.y + w->size.y - 4, GetColor(8));

          DrawLine(w->pos.x + 260, w->pos.y + 4, w->pos.x + w->size.x - 150, w->pos.y + 4, GetColor(4));
          DrawLine(w->pos.x + 260, w->pos.y + w->size.y - 4, w->pos.x + w->size.x - 150, w->pos.y + w->size.y - 4, GetColor(8));
          DrawLine(w->pos.x + 260, w->pos.y + 4, w->pos.x + 260, w->pos.y + w->size.y - 4, GetColor(4));
          DrawLine(w->pos.x + w->size.x - 150, w->pos.y + 4, w->pos.x + w->size.x - 150, w->pos.y + w->size.y - 4, GetColor(8));

          if (texturename[0] != 0)
          {
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, BG_COLOR, 0, 0, oldtexname);
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, 0, 0, 0, texturename);
            strcpy(oldtexname, texturename);
          }
          else
          {
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, BG_COLOR, 0, 0, oldtexname);
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, 0, 0, 0, "No Texture");
            oldtexname[0] = 0;
          }

          if (status.move == TRUE)
          {
            QUI_DrawStr(w->pos.x + 266, w->pos.y + 8, BG_COLOR, 0, 0, 0, "(%d %d %d)", status.move_amt.x, status.move_amt.y, status.move_amt.z);
          }
          else
          {
            if (M.cur_texname[0] != '\0')
            {
              if (M.cur_face.Brush != NULL)
              {
                QUI_DrawStr(w->pos.x + 266, w->pos.y + 8, BG_COLOR, 0, 0, 0, "%s (%2.0f %2.0f %2.0f)", M.cur_texname, M.cur_face.Brush->plane[M.cur_face.facenum].center.x, M.cur_face.Brush->plane[M.cur_face.facenum].center.y, M.cur_face.Brush->plane[M.cur_face.facenum].center.z);
              }
            }
          }

          break;

        case ENTITY:
          QUI_DrawStr(w->pos.x + w->size.x - 120, w->pos.y + 8, BG_COLOR, 14, 0, 0, "Entity Mode");

          QUI_Frame(w->pos.x + 100, w->pos.y + 4, w->pos.x + w->size.x - 140, w->pos.y + w->size.y - 4);

          if (M.cur_entity != NULL)
          {
            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, BG_COLOR, 0, 0, "%s", oldentstr);

            if (GetKeyValue(M.cur_entity, "origin"))
            {
              sprintf(oldentstr,
                      "%s (%s)",
                      GetKeyValue(M.cur_entity, "classname"),
                      GetKeyValue(M.cur_entity, "origin"));
            }
            else
            {
              sprintf(oldentstr,
                      "%s",
                      GetKeyValue(M.cur_entity, "classname"));
            }

            QUI_DrawStr(w->pos.x + 106, w->pos.y + 8, BG_COLOR, 0, 0, 0, "%s", oldentstr);
          }
          break;

        case MODEL:
          QUI_DrawStr(w->pos.x + w->size.x - 120, w->pos.y + 8, BG_COLOR, 14, 0, 0, "Model Mode");
          break;
      }
  }

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
}

void
QUI_PopUpWindow(int win, const char* Title, unsigned char** TempBuf)
{
  QUI_window_t* w;
  int i, j;

  /* create a nice little alias so code isnt so cryptic */
  w = &Q.window[win];

  /* Allocate Memory */
  *TempBuf = Q_malloc(w->size.x * w->size.y);

  /* Copy whats under the window to temp buffer */
  for (i = 0, j = w->pos.y; i < (w->size.y); i++, j++)
    memcpy(&((*TempBuf)[i * (w->size.x)]), &video.ScreenBuffer[(j * video.ScreenWidth) + w->pos.x], w->size.x);

  /* Fill the background color of the window */
  for (i = 0, j = w->pos.y; i < w->size.y; i++, j++)
    memset(&video.ScreenBuffer[(j * video.ScreenWidth) + w->pos.x], GetColor(BG_COLOR), w->size.x);

  /* Draw the window itself */
  DrawLine(w->pos.x, w->pos.y + 1, w->pos.x + w->size.x - 1, w->pos.y + 1, GetColor(10));
  DrawLine(w->pos.x, w->pos.y + 1, w->pos.x, w->pos.y + w->size.y - 1, GetColor(10));
  DrawLine(w->pos.x + 1, w->pos.y + 2, w->pos.x + w->size.x - 1, w->pos.y + 2, GetColor(8));
  DrawLine(w->pos.x + 1, w->pos.y + 2, w->pos.x + 1, w->pos.y + w->size.y - 1, GetColor(8));
  DrawLine(w->pos.x + w->size.x - 2, w->pos.y, w->pos.x + w->size.x - 2, w->pos.y + w->size.y - 1, GetColor(4));
  DrawLine(w->pos.x, w->pos.y + w->size.y - 2, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 2, GetColor(4));
  DrawLine(w->pos.x + w->size.x - 1, w->pos.y + 1, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 1, GetColor(3));
  DrawLine(w->pos.x, w->pos.y + w->size.y - 1, w->pos.x + w->size.x - 1, w->pos.y + w->size.y - 1, GetColor(3));

  /* draw the title */
  QUI_DrawStr(w->pos.x + (w->size.x / 2) - (QUI_strlen(0, Title) / 2), w->pos.y + 9, BG_COLOR, COL_RED - 5, 0, 0, Title);

  /* box the title */
  QUI_Box(w->pos.x + 8, w->pos.y + 8, w->pos.x + w->size.x - 11, w->pos.y + ROM_CHAR_HEIGHT + 8, 4, 8);
}

void
QUI_PopDownWindow(int win, unsigned char** TempBuf)
{
  QUI_window_t* w;
  int i, j;

  /* create a nice alias so code isnt so cryptic */
  w = &Q.window[win];

  /* copy what was behind the window back over it from the temp buffer*/
  for (i = 0, j = w->pos.y; i < (w->size.y); i++, j++)
    memcpy(&video.ScreenBuffer[(j * video.ScreenWidth) + w->pos.x], &((*TempBuf)[i * (w->size.x)]), w->size.x);

  /* free the temp buffer */
  Q_free(*TempBuf);
}

void
QUI_Dialog(const char* title, const char* string)
{
  QUI_window_t* w;
  unsigned char* TempBuf;
  int OKBUTTON;
  int buttonposx;
  int buttonposy;
  int buttonsizex;
  int buttonsizey;

  PushButtons();
  OKBUTTON = AddButtonText(0, 0, B_ENTER | B_ESCAPE, "Continue");

  buttonsizex = button[OKBUTTON].sx; /*used for sizing the window, sets a min value*/
  buttonsizey = button[OKBUTTON].sy;

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];

  w->size.x = QUI_strlen(0, string) + 20;
  if (w->size.x < (QUI_strlen(0, title) + 40))
    w->size.x = QUI_strlen(0, title) + 40;
  if ((w->size.x) < buttonsizex + 10)
    w->size.x = buttonsizex + 10;

  w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);

  w->size.y = buttonsizey + 32 + ROM_CHAR_HEIGHT + 8;
  w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

  buttonposx = w->pos.x + (w->size.x / 2) - (buttonsizex / 2);
  buttonposy = w->pos.y + w->size.y - buttonsizey - 5;

  MoveButton(OKBUTTON, buttonposx, buttonposy);

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, title, &TempBuf);
  Q.num_popups++;

  QUI_DrawStr(w->pos.x + (w->size.x / 2) - (QUI_strlen(0, string) / 2), w->pos.y + ROM_CHAR_HEIGHT * 2, BG_COLOR, 14, 0, 0, string);

  DrawButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  DrawMouse(mouse.x, mouse.y);

  /* Wait for mouse button to be released */
  while (mouse.button != 0)
    UpdateMouse();
  /* Clear out the keyboard buffer */
  ClearKeys();

  do
  {
    UpdateMouse();
  } while (UpdateButtons() != OKBUTTON);

  /* Pop down the window (also copies back whats behind the window) */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &TempBuf);

  /* Get rid of the button */
  RemoveButton(OKBUTTON);
  PopButtons();

  /* refresh the correct portion of the screen */
  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  DrawMouse(mouse.x, mouse.y);
}

int
YNgood(char c)
{
  if ((c == 'y') || (c == 'n') || (c == 'Y') || (c == 'N'))
    return TRUE;
  return FALSE;
}

int
QUI_YesNo(const char* title, const char* string, const char* op1, const char* op2)
{
  QUI_window_t* w;
  unsigned char* TempBuf;
  int YESBUTTON, NOBUTTON;
  int bp;
  int buttonsizex;
  int buttonsizey;

  PushButtons();
  YESBUTTON = AddButtonText(0, 0, 0, op1);
  NOBUTTON = AddButtonText(0, 0, 0, op2);

  buttonsizex = button[YESBUTTON].sx + button[NOBUTTON].sx; /*used for sizing the window, sets a min value*/
  buttonsizey = button[YESBUTTON].sy;

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];

  w->size.x = QUI_strlen(0, string) + 20;
  if (w->size.x < (QUI_strlen(0, title) + 40))
    w->size.x = QUI_strlen(0, title) + 40;
  if ((w->size.x) < buttonsizex + 10)
    w->size.x = buttonsizex + 10;

  w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);

  w->size.y = buttonsizey + 32 + ROM_CHAR_HEIGHT + 8;
  w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

  button[YESBUTTON].x = w->pos.x + (w->size.x / 2) - (buttonsizex / 2) - 2;
  button[NOBUTTON].x = button[YESBUTTON].x + button[YESBUTTON].sx + 4;
  button[YESBUTTON].y = w->pos.y + w->size.y - buttonsizey - 5;
  button[NOBUTTON].y = button[YESBUTTON].y;

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, title, &TempBuf);
  Q.num_popups++;

  QUI_DrawStr(w->pos.x + (w->size.x / 2) - (QUI_strlen(0, string) / 2), w->pos.y + ROM_CHAR_HEIGHT * 2, BG_COLOR, 14, 0, 0, string);

  DrawButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  /* Wait for mouse button to be released */
  while (mouse.button != 0)
    UpdateMouse();
  ClearKeys();

  do
  {
    UpdateMouse();
    bp = UpdateButtons();
    if (TestKey(KEY_Y))
      bp = YESBUTTON;
    if (TestKey(KEY_N))
      bp = NOBUTTON;
  } while (bp == -1);

  /* Pop down the window (also copies back whats behind the window) */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &TempBuf);
  /* Get rid of the button */
  RemoveButton(YESBUTTON);
  RemoveButton(NOBUTTON);
  PopButtons();
  /* refresh the correct portion of the screen */
  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  DrawMouse(mouse.x, mouse.y);
  if (bp == YESBUTTON)
    return TRUE;
  if (bp == NOBUTTON)
    return FALSE;
  return FALSE;
}

int
QUI_PopEntity(const char* title, char** string, char** value, int number)
{
#if 1 // call new GUI stuff
  return GUI_PopEntity(title, string, value, number);
#else
  QUI_window_t* w;
  //	bitmap_t *b;
  int i;
  unsigned char* TempBuf;
  int b_ok, b_cancel;
  int bp;
  //	int buttonposx;
  //	int buttonposy;
  int buttonsizex;
  int buttonsizey;
  int maxlen, maxlen1, maxlen2;
  int Stringx1, Stringx2, Stringy1, Stringy2;
  int Valuex1, Valuex2, Valuey1, Valuey2;
#ifdef DJGPP
  char c;
#endif

  PushButtons();
  b_ok = AddButtonText(0, 0, B_ENTER, "Ok");
  b_cancel = AddButtonText(0, 0, B_ESCAPE, "Cancel");

  buttonsizex = button[b_ok].sx + button[b_cancel].sx; /*used for sizing the window, sets a min value*/
  buttonsizey = button[b_ok].sy;

  maxlen1 = 192;
  maxlen2 = 256;

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];

  w->size.x = QUI_strlen(0, title) + 100;
  if ((w->size.x) < buttonsizex + 10)
    w->size.x = buttonsizex + 10;
  maxlen = maxlen1 + maxlen2;
  if (w->size.x < maxlen + 64)
    w->size.x = maxlen + 64;

  w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);

  w->size.y = buttonsizey + 32 + (number + 3) * ROM_CHAR_HEIGHT + 8;
  w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

  button[b_ok].x = w->pos.x + (w->size.x / 2) - (buttonsizex / 2) - 2;
  button[b_cancel].x = button[b_ok].x + button[b_ok].sx + 4;
  button[b_ok].y = w->pos.y + w->size.y - buttonsizey - 5;
  button[b_cancel].y = button[b_ok].y;

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, title, &TempBuf);
  Q.num_popups++;

  Stringx1 = w->pos.x + 8;
  Stringx2 = w->pos.x + (w->size.x / 3) - 8;
  Valuex1 = w->pos.x + (w->size.x / 3) + 8;
  Valuex2 = w->pos.x + w->size.x - 11;
  Stringy1 = Valuey1 = w->pos.y + ROM_CHAR_HEIGHT * 2;
  Stringy2 = Valuey2 = Stringy1 + (ROM_CHAR_HEIGHT + 3) * number;

  /* Box the windows */
  for (i = 0; i < number; i++)
  {
    QUI_Frame(Stringx1, Stringy1 + (ROM_CHAR_HEIGHT + 3) * i, Stringx2, Stringy1 + (ROM_CHAR_HEIGHT + 3) * (i + 1) - 1);

    QUI_Frame(Valuex1, Valuey1 + (ROM_CHAR_HEIGHT + 3) * i, Valuex2, Valuey1 + (ROM_CHAR_HEIGHT + 3) * (i + 1) - 1);

    QUI_DrawStr(Stringx1 + 5, Stringy1 + i * (ROM_CHAR_HEIGHT + 3) + 2, BG_COLOR, 14, 0, 0, string[i]);
    QUI_DrawStr(Valuex1 + 5, Valuey1 + i * (ROM_CHAR_HEIGHT + 3) + 2, BG_COLOR, 14, 0, 0, value[i]);
  }

  DrawButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  /* Wait for mouse button to be released */
  while (mouse.button != 0)
    UpdateMouse();
  ClearKeys();

  do
  {
    UpdateMouse();
    bp = UpdateButtons();

#ifdef DJGPP
    /*
     TODO: What's this doing here?
    */
    if (kbhit())
    {
      c = getch();
      if (YNgood(c))
      {
        ClearKeys();
        Q.num_popups--;
        QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &TempBuf);
        /* Get rid of the button */
        RemoveButton(b_ok);
        RemoveButton(b_cancel);
        PopButtons();
        /* refresh the correct portion of the screen */
        RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
        DrawMouse(mouse.x, mouse.y);
        if ((c == 'y') || (c == 'Y'))
          return TRUE;
        if ((c == 'n') || (c == 'N'))
          return FALSE;
        return FALSE;
      }
    }
#endif

    if ((bp == -1) && (mouse.button == 1))
    {
      /* check to see if he clicked in the value area */
      if (InBox(Valuex1, Valuey1, Valuex2, Valuey2))
      {
        i = (mouse.y - Valuey1) / (ROM_CHAR_HEIGHT + 3);
        if (readtname(value[i], Valuex1 + 5, Valuey1 + i * (ROM_CHAR_HEIGHT + 3) + 2, 30) == FALSE)
        {
          QUI_DrawStr(Valuex1 + 5, Valuey1 + i * (ROM_CHAR_HEIGHT + 3) + 2, BG_COLOR, 14, 0, 1, value[i]);
          DrawMouse(mouse.x, mouse.y);
        }
      }
    }
    /* Loop until they actually hit the button */
  } while (bp == -1);

  /* Pop down the window (also copies back whats behind the window) */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &TempBuf);
  /* Get rid of the button */
  RemoveButton(b_ok);
  RemoveButton(b_cancel);
  PopButtons();
  /* refresh the correct portion of the screen */
  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  DrawMouse(mouse.x, mouse.y);
  if (bp == b_ok)
    return TRUE;
  if (bp == b_cancel)
    return FALSE;
  return FALSE;
#endif
}

int
QUI_LoadFontMap(const char* filename, font_t* font)
{
  FILE* fp;
  int val;
  int i;
  char name[256];

  FindFile(name, filename);
  if ((fp = fopen(name, "rt")) == NULL)
  {
    HandleError("QUI_LoadFontMap", "Unable to open mapfile");
    return FALSE;
  }

  for (i = 0; i < 128; i++)
  {
    fscanf(fp, "%i\n", &val);
    font->map[i] = val;
  }

  fclose(fp);

  return TRUE;
}

int
QUI_LoadFontDiv(const char* filename, font_t* font)
{
  FILE* fp;
  int pos, wid;
  int num_divs = 0;
  char name[256];

  FindFile(name, filename);
  if ((fp = fopen(name, "rt")) == NULL)
  {
    HandleError("QUI_LoadFontDiv", "Unable to open div");
    return FALSE;
  }

  fscanf(fp, "%d %d", &pos, &wid);
  while (!feof(fp))
  {
    font->div = Q_realloc(font->div, sizeof(int) * (num_divs + 1));
    if ((font->div) == NULL)
    {
      Q_free(font->div);
      break;
    }
    font->width = Q_realloc(font->width, sizeof(int) * (num_divs + 1));
    if ((font->div) == NULL)
    {
      Q_free(font->width);
      break;
    }
    font->div[num_divs] = pos;
    font->width[num_divs] = wid - pos + 1;
    num_divs++;
    fscanf(fp, "%d %d", &pos, &wid);
  }

  if (font->div == NULL)
  {
    HandleError("QUI_LoadFontDiv", "Unable to allocate font division list.");
    return FALSE;
  }
  if (font->width == NULL)
  {
    HandleError("QUI_LoadFontDiv", "Unable to allocate font width list.");
    return FALSE;
  }

  fclose(fp);

  return TRUE;
}

int
QUI_LoadFontPCX(const char* filename, font_t* font)
{
  FILE* fp;
  int cur_byte, run_len;
  int size, i;
  char name[256];
  int minx, miny;

  FindFile(name, filename);
  if ((fp = fopen(name, "rb")) == NULL)
  {
    HandleError("QUI_LoadFontPCX", "Unable to open pcx");
    return FALSE;
  }

  fseek(fp, 4, SEEK_SET);
  font->data_width = font->data_height = 0;
  minx = miny = 0;
  fread(&minx, sizeof(short), 1, fp);
  fread(&miny, sizeof(short), 1, fp);
  fread(&font->data_width, sizeof(short), 1, fp);
  fread(&font->data_height, sizeof(short), 1, fp);

  font->data_width += 1 - minx;
  font->data_height += 1 - miny;

  /*	font->data_width++;
    font->data_height++;

    if ((font->data_width & 0x01) != 0)
      font->data_width++;
    if ((font->data_height & 0x01) != 0)
      font->data_height++;*/

  size = (int)font->data_width * (int)font->data_height;

  font->data = Q_malloc(sizeof(unsigned char) * size);

  if (font->data == NULL)
  {
    HandleError("QUI_LoadFontPCX", "Unable to allocate font data");
    return FALSE;
  }

  fseek(fp, 128, SEEK_SET);

  i = 0;
  while (i < size)
  {
    cur_byte = fgetc(fp);
    if ((cur_byte & 0xC0) == 0xC0)
    {
      run_len = cur_byte & 0x3F;
      cur_byte = fgetc(fp);
      for (; (run_len > 0) && (i < size); run_len--, i++)
      {
        if (cur_byte == 15)
          font->data[i] = 1;
        else
          font->data[i] = 0;
      }
    }
    else
    {
      if (cur_byte == 15)
        font->data[i++] = 1;
      else
        font->data[i++] = 0;
    }
  }

  fclose(fp);

  return TRUE;
}

int
QUI_RegisterFont(const char* fontloc)
{
  char filename[256];

  /* Load data */
  strcpy(filename, fontloc);
  strcat(filename, ".pcx");
  if (!QUI_LoadFontPCX(filename, &Q.font[Q.num_fonts]))
    return FALSE;

  /* Load Div */
  strcpy(filename, fontloc);
  strcat(filename, ".div");
  if (!QUI_LoadFontDiv(filename, &Q.font[Q.num_fonts]))
    return FALSE;

  /* Load Map */
  strcpy(filename, fontloc);
  strcat(filename, ".map");
  if (!QUI_LoadFontMap(filename, &Q.font[Q.num_fonts]))
    return FALSE;

  Q.num_fonts++;

  return TRUE;
}

int
QUI_Init(void)
{
  int MapXSize, MapYSize;
  int MenuXSize, MenuYSize;
  int ToolXSize, ToolYSize;
  int StatusXSize, StatusYSize;
  int MesgXSize, MesgYSize;

  /* Initialize Fonts */
  Q.num_fonts = 0;

  if (!QUI_RegisterFont("graphics/qfont1"))
  {
    HandleError("QUI_Init", "Unable to intitalize qfont1.");
    return FALSE;
  }
  if (!QUI_RegisterFont("graphics/qfont2"))
  {
    HandleError("QUI_Init", "Unable to intitalize qfont2.");
    return FALSE;
  }

  Q.num_windows = 5;

  MapXSize = video.ScreenWidth - 160;
  MapYSize = video.ScreenHeight - 100;

  MenuXSize = 160;
  MenuYSize = video.ScreenHeight - 100;

  ToolXSize = 160;
  ToolYSize = 70;

  StatusXSize = video.ScreenWidth;
  StatusYSize = 30;

  MesgXSize = video.ScreenWidth - 160;
  MesgYSize = 70;

  /*Main Map Editing Area*/
  Q.window[MAP_WINDOW].pos.x = 0;
  Q.window[MAP_WINDOW].pos.y = StatusYSize;
  /* changed for popup menu window: Q->window[MAP_WINDOW].size.x = MapXSize-1;*/
  Q.window[MAP_WINDOW].size.x = video.ScreenWidth;
  Q.window[MAP_WINDOW].size.y = MapYSize - 1;
  Q.window[MAP_WINDOW].type = 0;

  /*Menu Area*/
  Q.window[MENU_WINDOW].pos.x = MapXSize;
  Q.window[MENU_WINDOW].pos.y = StatusYSize;
  Q.window[MENU_WINDOW].size.x = MenuXSize - 1;
  Q.window[MENU_WINDOW].size.y = MenuYSize - 1;
  Q.window[MENU_WINDOW].type = 0;
  /* ADDED, pop up menu window */
  Q.window[MENU_WINDOW].pos.x = video.ScreenWidth;
  Q.window[MENU_WINDOW].size.x = 1;

  /*Message Display Area*/
  Q.window[MESG_WINDOW].pos.x = 0;
  Q.window[MESG_WINDOW].pos.y = StatusYSize + MapYSize;
  Q.window[MESG_WINDOW].size.x = MesgXSize - 1;
  Q.window[MESG_WINDOW].size.y = MesgYSize - 1;
  Q.window[MESG_WINDOW].type = 0;

  /*Tool Button Area*/
  Q.window[TOOL_WINDOW].pos.x = MapXSize;
  Q.window[TOOL_WINDOW].pos.y = StatusYSize + MenuYSize;
  Q.window[TOOL_WINDOW].size.x = ToolXSize - 1;
  Q.window[TOOL_WINDOW].size.y = ToolYSize - 1;
  Q.window[TOOL_WINDOW].type = 0;

  /*Status Bar Area*/
  Q.window[STATUS_WINDOW].pos.x = 0;
  Q.window[STATUS_WINDOW].pos.y = 0;
  Q.window[STATUS_WINDOW].size.x = StatusXSize - 1;
  Q.window[STATUS_WINDOW].size.y = StatusYSize - 1;
  Q.window[STATUS_WINDOW].type = 0;

  /*Refresh all windows*/
  QUI_InitWindow(MENU_WINDOW);
  QUI_InitWindow(MESG_WINDOW);
  QUI_InitWindow(TOOL_WINDOW);
  QUI_InitWindow(STATUS_WINDOW);

  return TRUE;
}
