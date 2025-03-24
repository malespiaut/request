/*
message.c file of the Quest Source Code

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

#include "message.h"

#include "button.h"
#include "mouse.h"
#include "qui.h"
#include "video.h"

#define NUM_MSG_DISPLAY 3
#define MAX_MESSAGES 100

typedef struct msg_struct_t
{
  char* Text;
  int num;
  struct msg_struct_t* Last;
  struct msg_struct_t* Next;
} msg_t;

static msg_t* MesgEnd = NULL;
static msg_t* MesgHead = NULL;
static msg_t* CurMesg = NULL;
static int CurMesgNum = 0;
static int NumMessages = 0;

static int curnum = 0;

static int MesgUp, MesgDown, MesgScroll;

void
InitMesgWin(void)
{
  QUI_window_t* w;

  w = &Q.window[MESG_WINDOW];

  QUI_Frame(w->pos.x + 5, w->pos.y + 5, w->pos.x + w->size.x - 40, w->pos.y + w->size.y - 5);

  MesgUp = AddButtonPic(w->pos.x + w->size.x - 35, w->pos.y + 7, B_RAPID, "button_small_up");
  MesgDown = AddButtonPic(w->pos.x + w->size.x - 35, w->pos.y + 46, B_RAPID, "button_small_down");
  MesgScroll = AddButtonPic(w->pos.x + w->size.x - 35, w->pos.y + 7 + button[MesgUp].sy, B_RAPID, "button_small_scroll");

  DrawButtons();
}

void
EraseMessages(void)
{
  int i, x, y;
  msg_t* M;

  x = Q.window[MESG_WINDOW].pos.x + 8;
  y = Q.window[MESG_WINDOW].pos.y + 10;

  M = CurMesg;
  i = 0;
  while ((M) && (i < NUM_MSG_DISPLAY))
  {
    QUI_DrawStr(x, y, BG_COLOR, BG_COLOR, 0, 0, "%i", M->num);
    QUI_DrawStr(x + 48, y, BG_COLOR, BG_COLOR, 0, 0, M->Text);
    M = M->Next;
    i++;
    y += ROM_CHAR_HEIGHT;
  }
}

static void
ScrollUp(void)
{
  int TopY, BottomY;
  int dist;

  if (CurMesg)
  {
    if (CurMesg->Last)
    {
      EraseMessages();
      CurMesg = CurMesg->Last;
      CurMesgNum--;
      DrawMessages();
    }
    else
      return;
  }
  else
    return;

  dist = (button[MesgDown].y - button[MesgScroll].sy - 1) -
         (button[MesgUp].y + button[MesgUp].sy + 1);

  EraseButton(MesgScroll);
  BottomY = button[MesgScroll].y + button[MesgScroll].sy;
  button[MesgScroll].y = (dist * CurMesgNum / (NumMessages - NUM_MSG_DISPLAY)) +
                         button[MesgUp].y + button[MesgUp].sy + 1;
  TopY = button[MesgScroll].y;
  DrawButton(MesgScroll);
  WaitRetr();
  RefreshPart(button[MesgScroll].x, TopY, button[MesgScroll].x + button[MesgScroll].sx, BottomY);
  UndrawMouse(mouse.prev_x, mouse.prev_y);
  DrawMouse(mouse.x, mouse.y);
}

static void
ScrollDown(void)
{
  int i;
  int TopY, BottomY;
  int dist;
  msg_t* M;

  M = CurMesg;
  i = 0;
  while (i < (NUM_MSG_DISPLAY - 1))
  {
    if (M)
    {
      M = M->Next;
    }
    i++;
  }

  if (M)
  {
    if (M->Next)
    {
      EraseMessages();
      CurMesg = CurMesg->Next;
      DrawMessages();
      CurMesgNum++;
    }
    else
      return;
  }
  else
    return;

  dist = (button[MesgDown].y - button[MesgScroll].sy - 1) -
         (button[MesgUp].y + button[MesgUp].sy + 1);

  EraseButton(MesgScroll);
  TopY = button[MesgScroll].y;
  button[MesgScroll].y = (dist * CurMesgNum / (NumMessages - NUM_MSG_DISPLAY)) +
                         button[MesgUp].y + button[MesgUp].sy + 1;
  BottomY = button[MesgScroll].y + button[MesgScroll].sy;
  DrawButton(MesgScroll);
  RefreshPart(button[MesgScroll].x, TopY, button[MesgScroll].x + button[MesgScroll].sx, BottomY);
  UndrawMouse(mouse.prev_x, mouse.prev_y);
  DrawMouse(mouse.x, mouse.y);
}

static void
ScrollBar(void)
{
  int temp, temp1, temp2;
  int TopY, BottomY;
  int dist;
  int i;
  msg_t* M;
  int Redraw;

  Redraw = FALSE;

  dist = (button[MesgDown].y - button[MesgScroll].sy - 1) -
         (button[MesgUp].y + button[MesgUp].sy + 1);

  temp = (mouse.prev_y - mouse.y);

  if ((temp > 0) && (button[MesgScroll].y == button[MesgUp].y + button[MesgUp].sy + 1))
  {
    SetMousePos(mouse.x, mouse.y + temp);
    WaitRetr();
    UndrawMouse(mouse.prev_x, mouse.prev_y);
    DrawMouse(mouse.x, mouse.y);
    return;
  }
  if ((temp < 0) && (button[MesgScroll].y + button[MesgScroll].sy + 1 == button[MesgDown].y))
  {
    SetMousePos(mouse.x, mouse.y + temp);
    WaitRetr();
    UndrawMouse(mouse.prev_x, mouse.prev_y);
    DrawMouse(mouse.x, mouse.y);
    return;
  }

  EraseButton(MesgScroll);
  button[MesgScroll].y -= temp;
  if (temp > 0)
  {
    BottomY = button[MesgScroll].y + button[MesgScroll].sy + temp;
    TopY = button[MesgScroll].y;
  }
  else
  {
    TopY = button[MesgScroll].y + temp;
    BottomY = button[MesgScroll].y + button[MesgScroll].sy;
  }
  if (button[MesgScroll].y < button[MesgUp].y + button[MesgUp].sy + 1)
  {
    temp = button[MesgUp].y + button[MesgUp].sy + 1 - button[MesgScroll].y;
    SetMousePos(mouse.x, mouse.y + temp);
    button[MesgScroll].y = button[MesgUp].y + button[MesgUp].sy + 1;
  }
  if (button[MesgScroll].y + button[MesgScroll].sy + 1 > button[MesgDown].y)
  {
    temp = button[MesgDown].y - (button[MesgScroll].y + button[MesgScroll].sy + 1);
    SetMousePos(mouse.x, mouse.y + temp);
    button[MesgScroll].y = button[MesgDown].y - button[MesgScroll].sy - 1;
  }

  temp1 = CurMesgNum;
  temp2 = (NumMessages - NUM_MSG_DISPLAY) * (button[MesgScroll].y - (button[MesgUp].y + button[MesgUp].sy + 1)) / dist;
  if (temp1 != temp2)
  {
    EraseMessages();
    if (temp1 > temp2)
    {
      do
      {
        if (CurMesg)
        {
          if (CurMesg->Last)
          {
            CurMesg = CurMesg->Last;
            CurMesgNum--;
          }
        }
      } while (CurMesgNum != temp2);
    }
    else
    {
      do
      {
        M = CurMesg;
        i = 0;
        while (i < (NUM_MSG_DISPLAY - 1))
        {
          if (M)
          {
            M = M->Next;
          }
          i++;
        }

        if (M)
        {
          if (M->Next)
          {
            CurMesg = CurMesg->Next;
            CurMesgNum++;
          }
        }
      } while (CurMesgNum != temp2);
    }
    DrawMessages();
  }
  DrawButton(MesgScroll);
  RefreshPart(button[MesgScroll].x, TopY, button[MesgScroll].x + button[MesgScroll].sx, BottomY);
  UndrawMouse(mouse.prev_x, mouse.prev_y);
  DrawMouse(mouse.x, mouse.y);
}

void
UpdateMsg(void)
{
  int bp;

  bp = UpdateButtons();
  if (bp != -1)
  {
    if (bp == MesgUp)
      ScrollUp();
    if (bp == MesgDown)
      ScrollDown();
    if (bp == MesgScroll)
    {
      if (NumMessages > NUM_MSG_DISPLAY)
      {
        while (mouse.button == 1)
        {
          GetMousePos();
          if (mouse.prev_y != mouse.y)
          {
            SetMousePos(mouse.prev_x, mouse.y);
            ScrollBar();
          }
          else
          {
            SetMousePos(mouse.prev_x, mouse.y);
          }
        }
      }
    }
  }
}

void
DrawMessages(void)
{
  int i, x, y;
  msg_t* M;

  x = Q.window[MESG_WINDOW].pos.x + 8;
  y = Q.window[MESG_WINDOW].pos.y + 10;

  M = CurMesg;
  i = 0;
  while ((M) && (i < NUM_MSG_DISPLAY))
  {
    QUI_DrawStr(x, y, BG_COLOR, COL_GREEN, 0, 0, "%i", M->num);
    QUI_DrawStr(x + 48, y, BG_COLOR, 14, 0, 0, M->Text);
    M = M->Next;
    i++;
    y += ROM_CHAR_HEIGHT;
  }

  RefreshPart(Q.window[MESG_WINDOW].pos.x + 8,
              Q.window[MESG_WINDOW].pos.y + 10,
              Q.window[MESG_WINDOW].pos.x + Q.window[MESG_WINDOW].size.x - 36,
              Q.window[MESG_WINDOW].pos.y + Q.window[MESG_WINDOW].size.y);
}

void
NewMessage(const char* format, ...)
{
  int i;
  int dist;
  int TopY, BottomY;
  msg_t* M;
  char message[1024];
  va_list args;

  va_start(args, format);
  vsprintf(message, format, args);
  va_end(args);

  EraseMessages();

  while (NumMessages >= MAX_MESSAGES)
  {
    M = MesgHead;
    if (CurMesg == M)
      CurMesg = M->Next;
    MesgHead = M->Next;
    MesgHead->Last = NULL;
    free(M->Text);
    free(M);

    NumMessages--;
  }

  if (NumMessages == 0)
  {
    MesgEnd = (msg_t*)malloc(sizeof(msg_t));
    MesgEnd->Last = NULL;
    MesgEnd->Next = NULL;
    MesgHead = MesgEnd;
    CurMesg = MesgHead;
  }
  else
  {
    M = (msg_t*)malloc(sizeof(msg_t));
    M->Last = MesgEnd;
    M->Next = NULL;
    MesgEnd->Next = M;
    MesgEnd = M;
  }

  MesgEnd->Text = (char*)malloc(strlen(message) + 1);
  strcpy(MesgEnd->Text, message);
  MesgEnd->Text[strlen(message)] = 0;
  MesgEnd->num = curnum++;
  NumMessages++;
  CurMesgNum = 0;

  if (NumMessages > NUM_MSG_DISPLAY)
  {
    /* This scrolls to keep up with the new message */
    CurMesg = MesgEnd;
    CurMesgNum = NumMessages - 1;
    i = 0;
    while (i < (NUM_MSG_DISPLAY - 1))
    {
      CurMesg = CurMesg->Last;
      CurMesgNum--;
      i++;
    }
    dist = (button[MesgDown].y - button[MesgScroll].sy - 1) -
           (button[MesgUp].y + button[MesgUp].sy + 1);

    EraseButton(MesgScroll);
    TopY = button[MesgScroll].y;
    button[MesgScroll].y = (dist * CurMesgNum / (NumMessages - NUM_MSG_DISPLAY)) +
                           button[MesgUp].y + button[MesgUp].sy + 1;
    BottomY = button[MesgScroll].y + button[MesgScroll].sy;
    DrawButton(MesgScroll);
    RefreshPart(button[MesgScroll].x, TopY, button[MesgScroll].x + button[MesgScroll].sx, BottomY);
    UndrawMouse(mouse.prev_x, mouse.prev_y);
    DrawMouse(mouse.x, mouse.y);
  }

  DrawMessages();
}

void
DumpMessages(void)
{
  FILE* fp;
  msg_t* M;
  int size, num;

  size = num = 0;
  fp = fopen("message.out", "wt");
  for (M = MesgHead; M; M = M->Next)
  {
    fprintf(fp, "%s\n", M->Text);
    size += sizeof(msg_t) + strlen(M->Text) + 1;
    num++;
  }
  fclose(fp);

  /*   NewMessage("%i messages",num);
     NewMessage("%i bytes memory used (%i kb)",size,size/1024);*/
}
