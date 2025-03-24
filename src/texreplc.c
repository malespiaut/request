#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "texreplc.h"

#include "brush.h"
#include "button.h"
#include "file.h"
#include "keyboard.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "tex.h"
#include "texdef.h"
#include "texpick.h"
#include "video.h"

void
ReplaceTexture(void)
{
  QUI_window_t* w;
  int b_ok, b_cancel, b_t1, b_t2;
  int bp;
  unsigned char* temp_buf;

  int b1[2][2], b2[2][2];

  char tex1[64], tex2[64], buf[64];

  strcpy(tex1, M.cur_texname);
  strcpy(tex2, tex1);

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];
  w->size.x = 320;
  w->size.y = 150;
  w->pos.x = (video.ScreenWidth - w->size.x) / 2;
  w->pos.y = (video.ScreenHeight - w->size.y) / 2;

  PushButtons();
  b_ok = AddButtonText(0, 0, B_ENTER, "OK");
  MoveButton(b_ok, w->pos.x + 8, w->pos.y + w->size.y - 30);

  b_cancel = AddButtonText(0, 0, B_ESCAPE, "Cancel");
  MoveButton(b_cancel, button[b_ok].x + button[b_ok].sx + 4, button[b_ok].y);

  b_t1 = AddButtonText(0, 0, 0, "Pick");
  b_t2 = AddButtonText(0, 0, 0, "Pick");

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Replace Texture", &temp_buf);
  Q.num_popups++;

  b1[0][0] = w->pos.x + 8;
  b1[0][1] = w->pos.y + 48;
  b1[1][0] = b1[0][0] + 192;
  b1[1][1] = b1[0][1] + 20;

  b2[0][0] = w->pos.x + 8;
  b2[0][1] = b1[1][1] + 24;
  b2[1][0] = b2[0][0] + 192;
  b2[1][1] = b2[0][1] + 20;

  QUI_Box(b1[0][0], b1[0][1], b1[1][0], b1[1][1], 4, 8);
  QUI_Box(b2[0][0], b2[0][1], b2[1][0], b2[1][1], 4, 8);

  QUI_DrawStr(b1[0][0], b1[0][1] - 18, BG_COLOR, 0, 0, 0, "Replace");
  QUI_DrawStr(b2[0][0], b2[0][1] - 18, BG_COLOR, 0, 0, 0, "with");

  QUI_DrawStrM(b1[0][0] + 2, b1[0][1] + 2, b1[1][0] - 2, BG_COLOR, 15, 0, 0, "%s", tex1);
  QUI_DrawStrM(b2[0][0] + 2, b2[0][1] + 2, b2[1][0] - 2, BG_COLOR, 15, 0, 0, "%s", tex2);

  MoveButton(b_t1, b1[1][0] + 8, b1[0][1]);
  MoveButton(b_t2, b2[1][0] + 8, b2[0][1]);

  DrawButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  while (1)
  {
    /* Check for left-click */
    UpdateMouse();

    if (mouse.button & 1 &&
        InBox(b1[0][0], b1[0][1], b1[1][0], b1[1][1]))
    {
      readstring(tex1, b1[0][0] + 2, b1[0][1] + 1, b1[1][0], sizeof(tex1), NULL);
      while (mouse.button || TestKey(KEY_ENTER) || TestKey(KEY_ESCAPE))
        UpdateMouse();
    }

    if (mouse.button & 1 &&
        InBox(b2[0][0], b2[0][1], b2[1][0], b2[1][1]))
    {
      readstring(tex2, b2[0][0] + 2, b2[0][1] + 2, b2[1][0], sizeof(tex2), NULL);
      while (mouse.button || TestKey(KEY_ENTER) || TestKey(KEY_ESCAPE))
        UpdateMouse();
    }

    bp = UpdateButtons();
    if (bp == b_ok || bp == b_cancel)
      break;

    if (bp == b_t1)
    {
      QUI_DrawStrM(b1[0][0] + 2, b1[0][1] + 2, b1[1][0] - 2, BG_COLOR, BG_COLOR, 0, 0, "%s", tex1);

      TexturePicker(tex1, buf);
      if (buf[0])
        strcpy(tex1, buf);

      QUI_DrawStrM(b1[0][0] + 2, b1[0][1] + 2, b1[1][0] - 2, BG_COLOR, 15, 0, 0, "%s", tex1);
      RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
    }
    if (bp == b_t2)
    {
      QUI_DrawStrM(b2[0][0] + 2, b2[0][1] + 2, b2[1][0] - 2, BG_COLOR, BG_COLOR, 0, 0, "%s", tex2);

      TexturePicker(tex2, buf);
      if (buf[0])
        strcpy(tex2, buf);

      QUI_DrawStrM(b2[0][0] + 2, b2[0][1] + 2, b2[1][0] - 2, BG_COLOR, 15, 0, 0, "%s", tex2);
      RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
    }
  }

  /* Pop down the window */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

  RemoveButton(b_ok);
  RemoveButton(b_cancel);
  RemoveButton(b_t1);
  RemoveButton(b_t2);
  PopButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  if (bp == b_ok)
  {
    brush_t* b;
    int i;
    plane_t* p;

    for (b = M.BrushHead; b; b = b->Next)
    {
      if (b->bt->type != BR_NORMAL)
        continue;

      for (i = b->num_planes, p = b->plane; i; i--, p++)
        if (!stricmp(p->tex.name, tex1))
          SetTexture(&p->tex, tex2);
    }
  }
}
