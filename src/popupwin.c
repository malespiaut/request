/*
popupwin.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef DJGPP
#include <dir.h>
#include <dos.h>
#endif

#ifdef _UNIX
#include <sys/stat.h>

extern void X_SetWindowTitle(char* filename);
#endif

#include "defines.h"
#include "types.h"

#include "popupwin.h"

#include "3d.h"
#include "button.h"
#include "display.h"
#include "entclass.h"
#include "entity.h"
#include "error.h"
#include "file.h"
#include "game.h"
#include "keyboard.h"
#include "map.h"
#include "mdl.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "popupwin.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "tex.h"
#include "video.h"

#define MAX_FILE_ENTRIES 1000
#define MAX_DIR_ENTRIES 1000

#define NUM_FILELOAD_LINES 10

#define FILE_TYPE 0
#define DIR_TYPE 1
#define UP_TYPE -1
#define DOWN_TYPE 1

static char** Files;
static char** Directories;
static int NUMLINES;
static int NumFiles;
static int NumDirectories;
static int b_load, b_cancel;
static int TinyUp1, TinyUp2, TinyDown1, TinyDown2;
static int TinyScroll1, TinyScroll2;
static int FileTop, DirTop;

static int Filex1, Filex2, Filey1, Filey2;
static int Dirx1, Dirx2, Diry1, Diry2;
static int Textx1, Textx2, Texty1, Texty2;
static int OldSelect, CurSelect;

static void
DrawList(char** List, int x, int y, int x2, int y2, int color, int Start, int Size, int ToScreen)
{
  int i;

  for (i = 0; ((i < Size) && (i < NUMLINES)); i++)
  {
    QUI_DrawStr(x, y + i * ROM_CHAR_HEIGHT, BG_COLOR, color, 0, FALSE, List[i + Start]);
  }
  if (ToScreen == TRUE)
    RefreshPart(x, y, x2, y2);
}

static void
WinScroll(int wt, int d)
{
  int TopY = 0, BottomY = 0, dist;
  int SButton;
  button_t* Up;
  button_t* Down;
  button_t* Scroll;
  int DoIt;

  DoIt = FALSE;

  if (wt == DIR_TYPE)
  {
    Up = &(button[TinyUp1]);
    Down = &(button[TinyDown1]);
    Scroll = &(button[TinyScroll1]);
    SButton = TinyScroll1;
    if (((DirTop < NumDirectories - NUMLINES) && (d == DOWN_TYPE)) ||
        ((DirTop > 0) && (d == UP_TYPE)))
      DoIt = TRUE;
  }
  else if (wt == FILE_TYPE)
  {
    Up = &(button[TinyUp2]);
    Down = &(button[TinyDown2]);
    Scroll = &(button[TinyScroll2]);
    SButton = TinyScroll2;
    if (((FileTop < NumFiles - NUMLINES) && (d == DOWN_TYPE)) ||
        ((FileTop > 0) && (d == UP_TYPE)))
      DoIt = TRUE;
  }
  else
  {
    return;
  }

  if (DoIt == TRUE)
  {
    dist = (Down->y - Scroll->sy - 1) - (Up->y + Up->sy + 1);
    if (wt == DIR_TYPE)
    {
      DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, BG_COLOR, DirTop, NumDirectories, FALSE);
      DirTop += d;
    }
    else if (wt == FILE_TYPE)
    {
      DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, BG_COLOR, FileTop, NumFiles, FALSE);
      FileTop += d;
      OldSelect -= d;
    }
    EraseButton(SButton);

    if (d == UP_TYPE)
      BottomY = Scroll->y + Scroll->sy;
    else if (d == DOWN_TYPE)
      TopY = Scroll->y;

    if (wt == DIR_TYPE)
      Scroll->y = (dist * DirTop / (NumDirectories - NUMLINES)) +
                  Up->y + Up->sy + 1;
    else if (wt == FILE_TYPE)
      Scroll->y = (dist * FileTop / (NumFiles - NUMLINES)) +
                  button[TinyUp2].y + button[TinyUp2].sy + 1;

    if (d == UP_TYPE)
      TopY = Scroll->y;
    else if (d == DOWN_TYPE)
      BottomY = Scroll->y + Scroll->sy;

    DrawButton(SButton);
    if (wt == DIR_TYPE)
      DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, 0, DirTop, NumDirectories, TRUE);
    else if (wt == FILE_TYPE)
    {
      DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, 0, FileTop, NumFiles, TRUE);
      if ((OldSelect < NUMLINES) && (OldSelect < NumFiles) && (OldSelect >= 0))
        QUI_DrawStr(Filex1 + 3, Filey1 + 2 + OldSelect * ROM_CHAR_HEIGHT, BG_COLOR, 30, 0, TRUE, Files[OldSelect + FileTop]);
    }
    RefreshPart(Scroll->x, TopY, Scroll->x + Scroll->sx, BottomY);
  }
  DrawMouse(mouse.x, mouse.y);
}

static void
WinScrollBar(int wt)
{
  int TopY, BottomY, dist, temp;
  int SButton;
  button_t* Up;
  button_t* Down;
  button_t* Scroll;

  if (wt == DIR_TYPE)
  {
    Up = &(button[TinyUp1]);
    Down = &(button[TinyDown1]);
    Scroll = &(button[TinyScroll1]);
    SButton = TinyScroll1;
  }
  else if (wt == FILE_TYPE)
  {
    Up = &(button[TinyUp2]);
    Down = &(button[TinyDown2]);
    Scroll = &(button[TinyScroll2]);
    SButton = TinyScroll2;
  }
  else
  {
    return;
  }

  dist = (Down->y - Scroll->sy - 1) - (Up->y + Up->sy + 1);
  temp = (mouse.prev_y - mouse.y);
  if ((temp > 0) && (Scroll->y == Up->y + Up->sy + 1))
  {
    SetMousePos(mouse.x, mouse.y + temp);
    WaitRetr();
    UndrawMouse(mouse.prev_x, mouse.prev_y);
    DrawMouse(mouse.x, mouse.y);
    return;
  }
  if ((temp < 0) && (Scroll->y + Scroll->sy + 1 == Down->y))
  {
    SetMousePos(mouse.x, mouse.y + temp);
    WaitRetr();
    UndrawMouse(mouse.prev_x, mouse.prev_y);
    DrawMouse(mouse.x, mouse.y);
    return;
  }
  EraseButton(SButton);
  Scroll->y -= temp;
  if (temp > 0)
  {
    BottomY = Scroll->y + Scroll->sy + temp;
    TopY = Scroll->y;
  }
  else
  {
    TopY = Scroll->y + temp;
    BottomY = Scroll->y + Scroll->sy;
  }
  if (Scroll->y < Up->y + Up->sy + 1)
  {
    temp = Up->y + Up->sy + 1 - Scroll->y;
    SetMousePos(mouse.x, mouse.y + temp);
    Scroll->y = Up->y + Up->sy + 1;
  }
  if (Scroll->y + Scroll->sy + 1 > Down->y)
  {
    temp = Down->y - (Scroll->y + Scroll->sy + 1);
    SetMousePos(mouse.x, mouse.y + temp);
    Scroll->y = Down->y - Scroll->sy - 1;
  }
  if (wt == DIR_TYPE)
  {
    temp = DirTop;
    DirTop = (NumDirectories - NUMLINES) *
             (Scroll->y - (Up->y + Up->sy + 1)) / dist;
    if (temp != DirTop)
    {
      /* erase the old ones */
      DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, BG_COLOR, temp, NumDirectories, FALSE);
      /* draw  the new ones */
      DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, 0, DirTop, NumDirectories, TRUE);
    }
  }
  else if (wt == FILE_TYPE)
  {
    temp = FileTop;
    FileTop = (NumFiles - NUMLINES) *
              (Scroll->y - (Up->y + Up->sy + 1)) / dist;
    if (temp != FileTop)
    {
      OldSelect -= (FileTop - temp);
      /* erase the old ones */
      DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, BG_COLOR, temp, NumFiles, FALSE);
      /* draw  the new ones */
      DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, 0, FileTop, NumFiles, TRUE);
      if ((OldSelect < NUMLINES) && (OldSelect < NumFiles) && (OldSelect >= 0))
        QUI_DrawStr(Filex1 + 3, Filey1 + 2 + OldSelect * ROM_CHAR_HEIGHT, BG_COLOR, 30, 0, TRUE, Files[OldSelect + FileTop]);
    }
  }
  DrawButton(SButton);
  WaitRetr();
  RefreshPart(Scroll->x, TopY, Scroll->x + Scroll->sx, BottomY);
  UndrawMouse(mouse.prev_x, mouse.prev_y);
  DrawMouse(mouse.x, mouse.y);
}

#if 1
int
SelectFile(const char* title, const char* extension, const char* button_text, char* result)
{
#ifdef DJGPP
  int prevdrive;
  int olddrive;
  int newdrive;
#endif
  char olddir[256];
  char newstring[256];
  char string[256];
  int Manual;
  QUI_window_t* w;
  int i, j;
  unsigned char* TempBuf;
  int HitIt, StillHit;
  int OldlightFile, HighlightFile;
  int OldlightDir, HighlightDir;
  int NewDirectory;
  char files[16];

  int bp;

  strcpy(files, "*");
  strcat(files, extension);

  /* get directory and file information, init variables */
  NUMLINES = NUM_FILELOAD_LINES;
  Files = (char**)Q_malloc(sizeof(char*) * MAX_FILE_ENTRIES);

  FileList(Files, files, &NumFiles);

  Directories = (char**)Q_malloc(sizeof(char*) * MAX_DIR_ENTRIES);
  DirList(Directories, &NumDirectories);
  FileTop = 0;
  DirTop = 0;
  getcwd(olddir, 256);
  strcpy(newstring, olddir);
  if (newstring[strlen(newstring) - 1] != '/')
    strcat(newstring, "/");
#ifdef DJGPP
  olddrive = getdisk();
#endif

  /* Create / call up the buttons */
  PushButtons();
  b_load = AddButtonText(0, 0, B_ENTER, button_text);
  b_cancel = AddButtonText(0, 0, B_ESCAPE, "Cancel");
  TinyUp1 = AddButtonPic(0, 0, B_RAPID, "button_tiny_up");
  TinyScroll1 = AddButtonPic(0, 0, B_RAPID, "button_tiny_scroll");
  TinyDown1 = AddButtonPic(0, 0, B_RAPID, "button_tiny_down");
  TinyUp2 = AddButtonPic(0, 0, B_RAPID, "button_tiny_up");
  TinyScroll2 = AddButtonPic(0, 0, B_RAPID, "button_tiny_scroll");
  TinyDown2 = AddButtonPic(0, 0, B_RAPID, "button_tiny_down");

  /* Setup the window */
  w = &Q.window[POP_WINDOW_1 + Q.num_popups];

  //	w->size.x = 320;
  w->size.x = 480;
  if (w->size.x < (QUI_strlen(0, title) + 40))
    w->size.x = QUI_strlen(0, title) + 40;
  w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);
  w->size.y = button[b_load].sy + (NUMLINES + 4) * ROM_CHAR_HEIGHT + 8;
  w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

  /* Position the load / cancel buttons */
  j = w->pos.y + w->size.y - button[b_load].sy - 5;
  i = button[b_load].sx + button[b_cancel].sx;
  MoveButton(b_load, w->pos.x + (w->size.x / 2) - (i / 2) - 4, j);
  MoveButton(b_cancel, button[b_load].x + button[b_load].sx + 8, j);

  /* Draw the window */
  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, title, &TempBuf);
  Q.num_popups++;
  /* Draw the file and directory sub-windows */
  Filex1 = w->pos.x + (w->size.x / 2) + 2;
  Filey1 = w->pos.y + ROM_CHAR_HEIGHT + 14;
  Filex2 = w->pos.x + w->size.x - 27;
  Filey2 = w->pos.y + ROM_CHAR_HEIGHT + 16 + NUMLINES * ROM_CHAR_HEIGHT;
  Dirx1 = w->pos.x + 8;
  Diry1 = w->pos.y + ROM_CHAR_HEIGHT + 14;
  Dirx2 = w->pos.x + (w->size.x / 2) - 18;
  Diry2 = w->pos.y + ROM_CHAR_HEIGHT + 16 + NUMLINES * ROM_CHAR_HEIGHT;
  Textx1 = Dirx1;
  Textx2 = Filex2 + 16;
  Texty1 = Diry2 + 6;
  Texty2 = Diry2 + 8 + ROM_CHAR_HEIGHT;

  QUI_Frame(Filex1, Filey1, Filex2, Filey2);
  QUI_Frame(Dirx1, Diry1, Dirx2, Diry2);
  QUI_Frame(Textx1, Texty1, Textx2, Texty2);

  /* Position the Up,Down,and Scroll Buttons */
  MoveButton(TinyUp1, Dirx2 + 1, Diry1);
  MoveButton(TinyDown1, Dirx2 + 1, Diry2 - 16);
  MoveButton(TinyScroll1, Dirx2 + 1, button[TinyUp1].y + button[TinyUp1].sy + 1);
  MoveButton(TinyUp2, Filex2 + 1, Filey1);
  MoveButton(TinyDown2, Filex2 + 1, Filey2 - 16);
  MoveButton(TinyScroll2, Filex2 + 1, button[TinyUp2].y + button[TinyUp2].sy + 1);

  /* Draw the text */
  DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, 0, FileTop, NumFiles, FALSE);
  DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, 0, DirTop, NumDirectories, FALSE);
  QUI_DrawStr(Textx1 + 2, Texty1 + 2, BG_COLOR, 0, 0, FALSE, newstring);

  /* Draw the buttons */
  DrawButtons();

  /* Refresh the window to the screen */
  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  /* Wait for mouse button to be released, if necessary */
  while (mouse.button != 0)
    UpdateMouse();

  OldlightFile = -1;
  OldlightDir = -1;
  OldSelect = -1;
  Manual = FALSE;
  StillHit = FALSE;
  HitIt = FALSE;

  while (1)
  {
    UpdateMouse();
    bp = UpdateButtons();

    if (bp == -1)
    {
      if (mouse.button == 0)
      {
        ClearKeys();
        /* Do some mouse checking for the file window*/
        if (InBox(Filex1, Filey1, Filex2, Filey2))
        {
          HighlightFile = (mouse.y - (Filey1 + 2)) / ROM_CHAR_HEIGHT;
          if (HighlightFile != OldlightFile)
          {
            if ((OldlightFile < NUMLINES) && (OldlightFile < NumFiles) && (OldlightFile >= 0))
            {
              if (OldlightFile != OldSelect)
                QUI_DrawStr(Filex1 + 3, Filey1 + 2 + OldlightFile * ROM_CHAR_HEIGHT, BG_COLOR, 0, 0, TRUE, Files[OldlightFile + FileTop]);
              else
                QUI_DrawStr(Filex1 + 3, Filey1 + 2 + OldlightFile * ROM_CHAR_HEIGHT, BG_COLOR, 30, 0, TRUE, Files[OldlightFile + FileTop]);
            }
            if ((HighlightFile < NUMLINES) && (HighlightFile < NumFiles) && (HighlightFile >= 0))
            {
              if (HighlightFile != OldSelect)
                QUI_DrawStr(Filex1 + 3, Filey1 + 2 + HighlightFile * ROM_CHAR_HEIGHT, BG_COLOR, 11, 0, TRUE, Files[HighlightFile + FileTop]);
              else
                QUI_DrawStr(Filex1 + 3, Filey1 + 2 + HighlightFile * ROM_CHAR_HEIGHT, BG_COLOR, 30, 0, TRUE, Files[HighlightFile + FileTop]);
            }
            DrawMouse(mouse.x, mouse.y);
            OldlightFile = HighlightFile;
          }
        }
        else
        {
          if (OldlightFile != -1)
          {
            if (OldlightFile != OldSelect)
            {
              if ((OldlightFile < NUMLINES) && (OldlightFile < NumFiles) && (OldlightFile >= 0))
                QUI_DrawStr(Filex1 + 3, Filey1 + 2 + OldlightFile * ROM_CHAR_HEIGHT, BG_COLOR, 0, 0, TRUE, Files[OldlightFile + FileTop]);
              DrawMouse(mouse.x, mouse.y);
              OldlightFile = -1;
            }
          }
        }

        /* Do mouse checking for directory window */
        if (InBox(Dirx1, Diry1, Dirx2, Diry2))
        {
          HighlightDir = (mouse.y - (Diry1 + 2)) / ROM_CHAR_HEIGHT;
          if (HighlightDir != OldlightDir)
          {
            if ((OldlightDir < NUMLINES) && (OldlightDir < NumDirectories) && (OldlightDir >= 0))
              QUI_DrawStr(Dirx1 + 3, Diry1 + 2 + OldlightDir * ROM_CHAR_HEIGHT, BG_COLOR, 0, 0, TRUE, Directories[OldlightDir + DirTop]);
            if ((HighlightDir < NUMLINES) && (HighlightDir < NumDirectories) && (HighlightDir >= 0))
              QUI_DrawStr(Dirx1 + 3, Diry1 + 2 + HighlightDir * ROM_CHAR_HEIGHT, BG_COLOR, 11, 0, TRUE, Directories[HighlightDir + DirTop]);

            DrawMouse(mouse.x, mouse.y);
            OldlightDir = HighlightDir;
          }
        }
        else
        {
          if (OldlightDir != -1)
          {
            if ((OldlightDir < NUMLINES) && (OldlightDir < NumDirectories) && (OldlightDir >= 0))
              QUI_DrawStr(Dirx1 + 3, Diry1 + 2 + OldlightDir * ROM_CHAR_HEIGHT, BG_COLOR, 0, 0, TRUE, Directories[OldlightDir + DirTop]);
            DrawMouse(mouse.x, mouse.y);
            OldlightDir = -1;
          }
        }
      } /* if (mouse.button==0) */
      else if (mouse.button == 1)
      {
        /* if they didnt click on a button */
        /* check the file window */
        if (InBox(Filex1, Filey1, Filex2, Filey2))
        {
          CurSelect = (mouse.y - (Filey1 + 2)) / ROM_CHAR_HEIGHT;
          if (CurSelect != OldSelect)
          {
            if ((OldSelect < NUMLINES) && (OldSelect < NumFiles) && (OldSelect >= 0))
              QUI_DrawStr(Filex1 + 3, Filey1 + 2 + OldSelect * ROM_CHAR_HEIGHT, BG_COLOR, 0, 0, 1, Files[OldSelect + FileTop]);
            if ((CurSelect < NUMLINES) && (CurSelect < NumFiles) && (CurSelect >= 0))
              QUI_DrawStr(Filex1 + 3, Filey1 + 2 + CurSelect * ROM_CHAR_HEIGHT, BG_COLOR, 30, 0, 1, Files[CurSelect + FileTop]);

            DrawMouse(mouse.x, mouse.y);
            if ((CurSelect < NUMLINES) && (CurSelect < NumFiles) && (CurSelect >= 0))
            {
              OldSelect = CurSelect;
              QUI_DrawStr(Textx1 + 2, Texty1 + 2, BG_COLOR, BG_COLOR, 0, 0, newstring);
              getcwd(newstring, 256);
              if (newstring[strlen(newstring) - 1] != '/')
                strcat(newstring, "/");
              strcat(newstring, Files[CurSelect + FileTop]);
              Manual = FALSE;
              QUI_DrawStr(Textx1 + 2, Texty1 + 2, BG_COLOR, 0, 0, 0, newstring);
              RefreshPart(Textx1, Texty1, Textx2, Texty2);
            }
            else
              OldSelect = -1;
          }
        }
        /* or maybe the directory window */
        if (InBox(Dirx1, Diry1, Dirx2, Diry2))
        {
          NewDirectory = (mouse.y - (Diry1 + 2)) / ROM_CHAR_HEIGHT;
          if ((NewDirectory >= 0) && (NewDirectory < NumDirectories) && (NewDirectory < NUMLINES))
          {
            /* Draw the old text */
            DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, BG_COLOR, FileTop, NumFiles, FALSE);
            DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, BG_COLOR, DirTop, NumDirectories, FALSE);
            QUI_DrawStr(Textx1 + 2, Texty1 + 2, BG_COLOR, BG_COLOR, 0, 0, newstring);

#ifdef DJGPP
            if ((Directories[NewDirectory + DirTop][0] == '[') &&
                (Directories[NewDirectory + DirTop][2] == ']'))
            {
              prevdrive = getdisk();
              setdisk(Directories[NewDirectory + DirTop][1] - 'a');
              newdrive = getdisk();
              if (newdrive != (Directories[NewDirectory + DirTop][1] - 'a'))
              {
                HandleError("SelectFile", "Invalid Drive");
                setdisk(prevdrive);
              }
            }
            else
#endif
              chdir(Directories[NewDirectory + DirTop]);

            /* Free up the old names but not the main list of char *'s */
            for (i = 0; i < NumDirectories; i++)
              Q_free(Directories[i]);
            for (i = 0; i < NumFiles; i++)
              Q_free(Files[i]);

            /* get directory and file information */

            FileList(Files, files, &NumFiles);

            DirList(Directories, &NumDirectories);
            getcwd(newstring, 256);
            if (newstring[strlen(newstring) - 1] != '/')
              strcat(newstring, "/");

            /* Update some vars */
            Manual = FALSE;
            OldlightFile = -1;
            OldlightDir = -1;
            OldSelect = -1;
            FileTop = 0;
            DirTop = 0;
            EraseButton(TinyScroll1);
            EraseButton(TinyScroll2);
            button[TinyScroll1].y = button[TinyUp1].y + button[TinyUp1].sy + 1;
            button[TinyScroll2].y = button[TinyUp2].y + button[TinyUp2].sy + 1;
            DrawButton(TinyScroll1);
            DrawButton(TinyScroll2);

            /* Draw the new text */
            DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, 0, FileTop, NumFiles, FALSE);
            DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, 0, DirTop, NumDirectories, FALSE);
            QUI_DrawStr(Textx1 + 2, Texty1 + 2, BG_COLOR, 0, 0, 0, newstring);

            /* Put it all to the screen */
            RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
            DrawMouse(mouse.x, mouse.y);
          }
        }
        /* or maybe the text entry window */
        if (InBox(Textx1, Texty1, Textx2, Texty2))
        {
          readfname(newstring, Textx1 + 2, Texty1 + 2, Textx2, 40);
          if (strstr(newstring, ".") == NULL)
          {
            strcat(newstring, extension);
          }
          QUI_DrawStr(Textx1 + 2, Texty1 + 2, BG_COLOR, 0, 0, TRUE, newstring);
          for (i = strlen(newstring) - 1; i >= 0; i--)
          {
            if (newstring[i] == '/')
            {
              j = i;
              break;
            }
          }
          strcpy(string, newstring);
          if (string[j - 1] != ':')
            string[j] = 0;

          DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, BG_COLOR, FileTop, NumFiles, FALSE);
          DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, BG_COLOR, DirTop, NumDirectories, FALSE);

          chdir(string);

          /* Free up the old names but not the main list of char *'s */
          for (i = 0; i < NumDirectories; i++)
            Q_free(Directories[i]);
          for (i = 0; i < NumFiles; i++)
            Q_free(Files[i]);

          /* get directory and file information */
          FileList(Files, files, &NumFiles);

          DirList(Directories, &NumDirectories);

          /* Update some vars */

          OldlightFile = -1;
          OldlightDir = -1;
          OldSelect = -1;
          FileTop = 0;
          DirTop = 0;
          EraseButton(TinyScroll1);
          EraseButton(TinyScroll2);
          button[TinyScroll1].y = button[TinyUp1].y + button[TinyUp1].sy + 1;
          button[TinyScroll2].y = button[TinyUp2].y + button[TinyUp2].sy + 1;
          DrawButton(TinyScroll1);
          DrawButton(TinyScroll2);

          /* Draw the new text */
          DrawList(Files, Filex1 + 3, Filey1 + 2, Filex2, Filey2, 0, FileTop, NumFiles, FALSE);
          DrawList(Directories, Dirx1 + 3, Diry1 + 2, Dirx2, Diry2, 0, DirTop, NumDirectories, FALSE);

          /* Put it all to the screen */
          RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

          DrawMouse(mouse.x, mouse.y);
          Manual = TRUE;
        }
        while (mouse.button == 1)
          UpdateMouse();
      }
    } /* if (bp==-1) */

    /* A button has been pressed */

    if (bp == TinyUp1)
      WinScroll(DIR_TYPE, UP_TYPE);
    if (bp == TinyDown1)
      WinScroll(DIR_TYPE, DOWN_TYPE);
    if (bp == TinyUp2)
      WinScroll(FILE_TYPE, UP_TYPE);
    if (bp == TinyDown2)
      WinScroll(FILE_TYPE, DOWN_TYPE);

    if ((bp == TinyScroll1) || (bp == TinyScroll2))
    {
      if (((bp == TinyScroll1) && (NumDirectories > NUMLINES)) ||
          ((bp == TinyScroll2) && (NumFiles > NUMLINES)))
      {
        while (mouse.button == 1)
        {
          GetMousePos();
          if (mouse.prev_y != mouse.y)
          {
            SetMousePos(mouse.prev_x, mouse.y);
            if (bp == TinyScroll1)
              WinScrollBar(DIR_TYPE);
            if (bp == TinyScroll2)
              WinScrollBar(FILE_TYPE);
          }
          else
          {
            SetMousePos(mouse.prev_x, mouse.y);
          }
        }
      }
    }

    if (bp == b_load)
      break;
    if (bp == b_cancel)
      break;
  } /* while (1) */

  /* Pop down the window (also copies back whats behind the window) */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &TempBuf);

  /* Get rid of the buttons */
  RemoveButton(b_load);
  RemoveButton(b_cancel);
  RemoveButton(TinyUp1);
  RemoveButton(TinyUp2);
  RemoveButton(TinyDown1);
  RemoveButton(TinyDown2);
  RemoveButton(TinyScroll1);
  RemoveButton(TinyScroll2);
  PopButtons();

  /* refresh the window portion of the screen */
  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  DrawMouse(mouse.x, mouse.y);

  /* Do whatever */
  if (bp == b_load)
  {
    if ((OldSelect != -1) || (Manual == TRUE))
    {
      strcpy(result, newstring);
    }
    else
    {
      HandleError("SelectFile", "No file selected!");
      newstring[0] = 0;
    }
  }
  else
  {
    newstring[0] = 0;
  }

  /* free up the list of files we loaded, as well as the array of char *'s */
  for (i = 0; i < NumDirectories; i++)
    Q_free(Directories[i]);
  Q_free(Directories);

  for (i = 0; i < NumFiles; i++)
    Q_free(Files[i]);
  Q_free(Files);

  /* Go back to the original directory */
#ifdef DJGPP
  setdisk(olddrive);
#endif
  chdir(olddir);

  if (newstring[0])
    return 1;
  else
    return 0;
}
#endif

int
FileCmd(int ST)
{
  char title[128];
  const char *ext, *btext;
  char filename[256];
  char string[128];

#ifdef _UNIX
  struct stat buf;
#endif

  switch (ST)
  {
    case LOAD_IT:
      ext = ".map";
      strcpy(title, "Load map");
      btext = "Load";
      break;

    case SAVE_IT:
      ext = ".map";
      strcpy(title, "Save map");
      btext = "Save";
      break;

    case GROUP_LOAD:
      ext = ".grp";
      strcpy(title, "Load group");
      btext = "Load";
      break;

    case GROUP_SAVE:
      ext = ".grp";
      strcpy(title, "Save group");
      btext = "Save";
      break;

    case WAD_IT:
      ext = ".wad";
      strcpy(title, "Select wad");
      btext = "OK";
      break;

    case PTS_LOAD:
      ext = Game.pts_ext;
      sprintf(title, "Load %s file", Game.pts_ext);
      btext = "Load";
      break;

    default:
      return 0;
  }

  if (SelectFile(title, ext, btext, filename))
  {
    switch (ST)
    {
      case GROUP_LOAD:
        NewMessage("Loading group %s...", filename);
        Game.map.loadgroup(filename);
        break;

      case GROUP_SAVE:
        NewMessage("Saving group %s...", filename);
        Game.map.savegroup(filename, M.CurGroup);
        break;

      case LOAD_IT:
        NewMessage("Loading %s...", filename);
        UnloadMap();
        if (!Game.map.loadmap(filename))
        {
          HandleError("FileCmd", "Unable to load MAP file.");
          return 0;
        }
        strcpy(M.mapfilename, filename);
#ifdef _UNIX
        X_SetWindowTitle(M.mapfilename);
#endif
        break;

      case WAD_IT:
        //         if (IsGoodWad(filename,0))
        {
          ClearCache();
          SetKeyValue(M.WorldSpawn, "wad", filename);

          texturename[0] = 0;
          NewMessage("Wadfile set to: %s", filename);
        }
        /*         else
                 {
                    NewMessage("Invalid wadfile selected. Try again.");
                 }*/
        break;

      case SAVE_IT:
#ifdef DJGPP
        if (__file_exists(filename))
#endif
#ifdef _UNIX
          if (!stat(filename, &buf))
#endif
          {
            strcpy(string, "File Exists - ");
            strcat(string, filename);
            strcat(string, " - Overwrite?");
            if (QUI_YesNo("Save Map File", string, "Yes", "No"))
            {
              strcpy(M.mapfilename, filename);
              Game.map.savemap(M.mapfilename);
              NewMessage("MAP saved over %s.", M.mapfilename);
            }
            else
            {
              filename[0] = 0;
              NewMessage("Aborted");
            }
          }
          else
          {
            strcpy(M.mapfilename, filename);
            Game.map.savemap(M.mapfilename);
            NewMessage("MAP saved as %s.", M.mapfilename);
          }
#ifdef _UNIX
        X_SetWindowTitle(M.mapfilename);
#endif
        break;

      case PTS_LOAD:
        NewMessage("Loading '%s'...", filename);
        ReadPts(filename);
        break;
    }
  }
  return TRUE;
}

typedef struct node_s
{
  char* name;
  int n_children;
  int level;
  struct node_s* children;
  struct node_s* parent;
} node_t;

static node_t head;
static char** names;
static int num_names;
static int level;

static void
MakeTree(node_t* b, const char* base)
{
  int i;
  char newbase[128];
  char *s, *d;

  level++;

  b->name = Q_strdup(base);
  if (!b->name)
  {
    Abort("EntityPicker", "Out of memory!\n");
  }
  b->n_children = 0;
  b->children = NULL;

  for (i = 0; i < num_names; i++)
  {
    if (!names[i])
      continue;
    if (!strcmp(names[i], base))
    {
      Q_free(names[i]);
      names[i] = NULL;
      break;
    }
    else if (!strncmp(names[i], base, strlen(base)) && (level < 3))
    {
      b->children = Q_realloc(b->children, sizeof(node_t) * (b->n_children + 1));
      if (!b->children)
      {
        Abort("EntityPicker", "Out of memory!\n");
      }

      strcpy(newbase, base);
      s = &names[i][strlen(base)];
      d = &newbase[strlen(newbase)];

      if (level == 2)
      {
        while (*s)
          *d++ = *s++;
      }
      else
      {
        while (*s && (*s != '_'))
          *d++ = *s++;
        if (*s)
          *d++ = *s++;
      }
      *d = 0;

      MakeTree(&b->children[b->n_children], newbase);

      b->n_children++;
    }
  }
  level--;
}

static void
OptTree(node_t* b, int level)
{
  int i;
  node_t* c;

  while (b->n_children == 1)
  {
    c = &b->children[0];

    Q_free(b->name);
    b->name = c->name;
    b->n_children = c->n_children;
    b->children = c->children;
    Q_free(c);
  }

  b->level = level;

  for (i = 0; i < b->n_children; i++)
    OptTree(&b->children[i], level + 1);

  for (i = 0; i < b->n_children; i++)
    b->children[i].parent = b;
}

#define MAXY 352

static node_t* sel;
static int scroll_y;

static int draw_y;
static int base_x, base_y;
static int maxx, maxy;

static node_t* drawn[MAXY / ROM_CHAR_HEIGHT];

static void
DrawNode(int lev, node_t* n)
{
  int col;

  drawn[draw_y / ROM_CHAR_HEIGHT] = n;
  if (n == sel)
    col = 15;
  else
    col = 0;

  QUI_DrawStr(base_x + lev * 8, base_y + draw_y, BG_COLOR, col, 0, 0, "%s", n->name);
  draw_y += ROM_CHAR_HEIGHT;
}

static void
DrawSel(void)
{
  node_t* draw[8];
  int n_draw;
  int i;
  node_t* cur;
  int lev;

  for (i = 0; i < maxy; i++)
    DrawLine(base_x, base_y + i, base_x + maxx, base_y + i, BG_COLOR);

  draw_y = 0;

  n_draw = 0;
  for (cur = sel; cur; cur = cur->parent)
  {
    if (n_draw == 8)
      Abort("EntityPicker", "Entity tree more than 8 levels deep!");
    draw[n_draw] = cur;
    n_draw++;
  }

  lev = 0;
  for (i = n_draw - 1; i >= 0; i--)
  {
    if (draw[i]->n_children)
    {
      DrawNode(lev, draw[i]);
    }
    lev++;
  }

  if (sel->n_children)
  {
    cur = sel;
  }
  else
  {
    cur = sel->parent;
    lev--;
    n_draw--;
  }

  for (i = 0; i < cur->n_children; i++)
  {
    if ((i >= scroll_y) && (i <= scroll_y + maxy / ROM_CHAR_HEIGHT - n_draw))
      DrawNode(lev, &cur->children[i]);
  }

  draw_y /= ROM_CHAR_HEIGHT;
}

static void
FreeTree(node_t* b)
{
  int i;

  Q_free(b->name);
  for (i = 0; i < b->n_children; i++)
    FreeTree(&b->children[i]);
  if (b->n_children)
    Q_free(b->children);
}

void
EntityPicker(char* EndWithThis, int types)
{
  int i, j;

  QUI_window_t* w;
  unsigned char* tempbuf;

  int b_ok, b_cancel;
  int b_up, b_down;
  int bp;

  names = LoadModelNames(types, &num_names);
  if (!num_names)
  {
    HandleError("EntityPicker", "No valid classnames found!");
    EndWithThis[0] = 0;
    return;
  }

  sel = &head;

  level = 0;
  MakeTree(&head, "");
  OptTree(&head, 0);
  scroll_y = 0;

  for (i = 0; i < num_names; i++)
    if (names[i])
      Q_free(names[i]);
  Q_free(names);

  Q_free(head.name);
  head.name = Q_strdup("Classnames");
  head.parent = NULL;

  maxx = 320 - 32;
  maxy = MAXY - 1;

  PushButtons();
  b_ok = AddButtonText(0, 0, B_ENTER, "OK");
  b_cancel = AddButtonText(0, 0, B_ESCAPE, "Cancel");
  b_up = AddButtonPic(0, 0, B_RAPID, "button_tiny_up");
  b_down = AddButtonPic(0, 0, B_RAPID, "button_tiny_down");

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];

  w->size.x = maxx + 24;
  w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);

  w->size.y = maxy + 60;
  w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

  i = button[b_ok].sx + button[b_cancel].sx + 8;
  j = w->pos.y + w->size.y - button[b_ok].sy - 5;
  MoveButton(b_ok, w->pos.x + (w->size.x / 2) - (i / 2) - 4, j);
  MoveButton(b_cancel, button[b_ok].x + button[b_ok].sx + 8, j);

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Classnames", &tempbuf);
  Q.num_popups++;

  base_x = w->pos.x + 8;
  base_y = w->pos.y + 30 + 2;
  maxx -= 4;
  maxy -= ROM_CHAR_HEIGHT;

  MoveButton(b_up, base_x + maxx + 2, base_y);
  MoveButton(b_down, base_x + maxx + 2, base_y + maxy - button[b_down].sy);

  DrawButtons();
  DrawSel();

  QUI_Frame(base_x - 2, base_y - 2, base_x - 2 + maxx - 2, base_y - 2 + maxy);

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  do
  {
    UpdateMouse();
    bp = UpdateButtons();

    if (bp == b_cancel)
      break;

    if ((bp == b_ok) && (!sel->n_children))
      break;

    if (bp == b_up)
    {
      if (scroll_y)
      {
        scroll_y--;
        DrawSel();
        QUI_Frame(base_x - 2, base_y - 2, base_x - 2 + maxx - 2, base_y - 2 + maxy);
        RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
      }
    }

    if (bp == b_down)
    {
      if (maxy / ROM_CHAR_HEIGHT + 1 == draw_y)
      {
        scroll_y++;
        DrawSel();
        QUI_Frame(base_x - 2, base_y - 2, base_x - 2 + maxx - 2, base_y - 2 + maxy);
        RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
      }
    }

    if ((mouse.button == 1) && (bp == -1))
    {
      if (InBox(base_x, base_y, base_x + maxx, base_y + maxy))
      {
        int sy;

        sy = (mouse.y - base_y) / ROM_CHAR_HEIGHT;

        if (sy < draw_y)
        {
          if (sel->level != drawn[sy]->level)
            scroll_y = 0;
          sel = drawn[sy];
        }
        DrawSel();
        QUI_Frame(base_x - 2, base_y - 2, base_x - 2 + maxx - 2, base_y - 2 + maxy);
        RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
        DrawMouse(mouse.x, mouse.y);
      }
      while (mouse.button == 1)
        UpdateMouse();
    }
  } while (1);

  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &tempbuf);

  RemoveButton(b_ok);
  RemoveButton(b_cancel);
  RemoveButton(b_up);
  RemoveButton(b_down);
  PopButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  DrawMouse(mouse.x, mouse.y);

  if (bp == b_cancel)
  {
    EndWithThis[0] = 0;
  }
  else
  {
    strcpy(EndWithThis, sel->name);
  }
  FreeTree(&head);
}

#if 0
void AutoBuildPopup(void)
{
	QUI_window_t *w;
	unsigned char *temp_buf;

	int op1, op2, op3, op4, op5, op6;
	int cancel;
   int bp;

	int i;
	char go_string[256];
	char temp_str[256];
	int just_visible;

   if (M.modified)
   {
      if (QUI_YesNo(
             "Modified map",
             "The map has been modified since the last save. Save now?",
             "Save",
             "No"))
      {
         SaveMap(M.mapfilename);
      }
   }

	/* Set up window Position */
	w = &Q.window[POP_WINDOW_1 + Q.num_popups];
	w->size.x = 300;
	w->size.y = 220;
	w->pos.x = (video.ScreenWidth - w->size.x) / 2;
	w->pos.y = (video.ScreenHeight - w->size.y) / 2;

	/* Make the option buttons */
   PushButtons();
	op1 = AddButtonText(0,0,0," ");
	op2 = AddButtonText(0,0,0," ");
	op3 = AddButtonText(0,0,0," ");
	op4 = AddButtonText(0,0,0," ");
	op5 = AddButtonText(0,0,0," ");
	op6 = AddButtonText(0,0,0," ");

	cancel = AddButtonText(0,0,0,"Cancel");

	MoveButton(op1, w->pos.x + 20, w->pos.y + 35);
	MoveButton(op2, w->pos.x + 20, w->pos.y + 57);
	MoveButton(op3, w->pos.x + 20, w->pos.y + 79);
	MoveButton(op4, w->pos.x + 20, w->pos.y + 101);
	MoveButton(op5, w->pos.x + 20, w->pos.y + 123);
	MoveButton(op6, w->pos.x + 20, w->pos.y + 145);

	MoveButton(cancel,w->pos.x+w->size.x/2-button[cancel].sx/2,w->pos.y+w->size.y-30);

	/* Actually draw the window */
	QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Select Build", &temp_buf);
	Q.num_popups++;

	/* Draw the buttons */
   DrawButtons();

	QUI_DrawStr(w->pos.x + 45, w->pos.y +  40, BG_COLOR, 0,0, FALSE,"Fast (no error checking)");
	QUI_DrawStr(w->pos.x + 45, w->pos.y +  62, BG_COLOR, 0,0, FALSE,"Fast (with error checking)");
	QUI_DrawStr(w->pos.x + 45, w->pos.y +  84, BG_COLOR, 0,0, FALSE,"Entities Only");
	QUI_DrawStr(w->pos.x + 45, w->pos.y + 106, BG_COLOR, 0,0, FALSE,"Lighting Only");
	QUI_DrawStr(w->pos.x + 45, w->pos.y + 128, BG_COLOR, 0,0, FALSE,"Full Build");
	QUI_DrawStr(w->pos.x + 45, w->pos.y + 150, BG_COLOR, 0,0, FALSE,"Build Visible");

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

   just_visible=FALSE;
	while (1)
	{
      UpdateMouse();
      bp=UpdateButtons();

      if (bp==op1)
      {
         strcpy(go_string, "go1");
         just_visible = FALSE;
         break;
      }

      if (bp==op2)
      {
         strcpy(go_string, "go2");
         just_visible = FALSE;
         break;
      }

      if (bp==op3)
      {
         strcpy(go_string, "go3");
         just_visible = FALSE;
         break;
      }

      if (bp==op4)
      {
         strcpy(go_string, "go4");
         just_visible = FALSE;
         break;
      }

      if (bp==op5)
      {
         strcpy(go_string, "go5");
         just_visible = FALSE;
         break;
      }

      if (bp==op6)
      {
         strcpy(go_string, "go1");
         just_visible = TRUE;
         break;
      }

      if (bp==cancel)
      {
         go_string[0]=0;
         break;
      }
	}

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

	RemoveButton(op1);
	RemoveButton(op2);
	RemoveButton(op3);
	RemoveButton(op4);
	RemoveButton(op5);
	RemoveButton(op6);
	RemoveButton(cancel);
   PopButtons();

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

	if (strlen(go_string) > 0)
   {
		SetMode(RES_TEXT);

		if (just_visible)
      {
			CreateVisibleMap("questtmp.map");
      	KBD_RemoveHandler();
			system("go1 questtmp");
	      KBD_InstallHandler();
			remove("questtmp.map");
		}
		else
      {
			sprintf(temp_str, "%s %s", go_string, M.mapfilename);
			for (i=0; i<strlen(temp_str); i++)
         {
				if (temp_str[i] == '.')
					temp_str[i] ='\0';
			}
      	KBD_RemoveHandler();
			system(temp_str);
	      KBD_InstallHandler();
		}

		SetMode(options.vid_mode);

		InitMouse();
		SetPal(PAL_CURRENT);
		QUI_RedrawWindow(MESG_WINDOW);
		QUI_RedrawWindow(TOOL_WINDOW);
		QUI_RedrawWindow(STATUS_WINDOW);
		UpdateAllViewports();
	}
}
#endif
