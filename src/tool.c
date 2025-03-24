/*
tool.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "tool.h"

#include "3d.h"
#include "button.h"
#include "camera.h"
#include "display.h"
#include "edbrush.h"
#include "edent.h"
#include "edface.h"
#include "edprim.h"
#include "edvert.h"
#include "message.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "texpick.h"
#include "undo.h"

static int LastToolHit;

static int b_turn_left, b_turn_right;
static int b_look_up, b_look_down;
static int b_brush, b_move, b_split, b_copy, b_texture;

/*
  TODO: Make these use ExecCmd
*/
void
UpdateTool(void)
{
  brushref_t* b;
  fsel_t* f;
  int dx, dy, dz;
  char tmpstr[256];
  int bp;

  bp = UpdateButtons();

  if (bp == -1)
    return;

  /* Hit an editing button */
  if (bp == b_brush)
  {
    SUndo(UNDO_NONE, UNDO_NONE);
    /* Draw brush in front of camera */
    Move(M.display.active_vport, MOVE_FORWARD, &dx, &dy, &dz, 100);
    AddBrush(TEXRECT,
             M.display.vport[M.display.active_vport].camera_pos.x + dx,
             M.display.vport[M.display.active_vport].camera_pos.y + dy,
             M.display.vport[M.display.active_vport].camera_pos.z + dz,
             32,
             32,
             32,
             0);
    status.edit_mode = BRUSH;
    UpdateAllViewports();
  }

  if (bp == b_copy)
  {
    ClearSelVerts();
    ClearSelEnts();

    CopyBrush();

    UpdateAllViewports();
  }

  if (bp == b_texture)
  {
    TexturePicker(texturename, tmpstr);
    if (tmpstr[0] != 0)
    {
      strcpy(texturename, tmpstr);

      if (M.display.num_bselected || M.display.num_fselected)
        SUndo(UNDO_NONE, UNDO_CHANGE);

      for (b = M.display.bsel; b; b = b->Next)
        ApplyTexture(b->Brush, texturename);
      for (f = M.display.fsel; f; f = f->Next)
        ApplyFaceTexture(f, texturename);

      if (M.display.num_bselected || M.display.num_fselected)
        sprintf(tmpstr, "%s applied to all selected brushes.", texturename);
      else
        sprintf(tmpstr, "%s selected as active texture.", texturename);
      NewMessage(tmpstr);
      QUI_RedrawWindow(STATUS_WINDOW);
    }
    UpdateAllViewports();
  }

  if (bp == b_move)
  {
    status.edit_mode = BRUSH;

    QUI_RedrawWindow(STATUS_WINDOW);
  }

  if (bp == b_split)
  {
    status.edit_mode = BRUSH;
    QUI_RedrawWindow(STATUS_WINDOW);
  }

  if (bp == b_turn_left)
  {
    TurnLeft(M.display.active_vport);
    UpdateViewport(M.display.active_vport, TRUE);
  }

  if (bp == b_turn_right)
  {
    TurnRight(M.display.active_vport);
    UpdateViewport(M.display.active_vport, TRUE);
  }

  if (bp == b_look_up)
  {
    LookUp(M.display.active_vport);
    UpdateViewport(M.display.active_vport, TRUE);
  }

  if (bp == b_look_down)
  {
    LookDown(M.display.active_vport);
    UpdateViewport(M.display.active_vport, TRUE);
  }
}

void
InitTool(void)
{
  QUI_window_t* w;

  w = &Q.window[TOOL_WINDOW];

  LastToolHit = -1;

  b_turn_left = AddButtonPic(w->pos.x + 3, w->pos.y + 33, 0, "button_turn_left");
  b_turn_right = AddButtonPic(w->pos.x + 33, w->pos.y + 33, 0, "button_turn_right");
  b_look_up = AddButtonPic(w->pos.x + 63, w->pos.y + 33, 0, "button_look_up");
  b_look_down = AddButtonPic(w->pos.x + 93, w->pos.y + 33, 0, "button_look_down");

  b_brush = AddButtonPic(w->pos.x + 3, w->pos.y + 5, 0, "button_brush");
  b_move = AddButtonPic(w->pos.x + 33, w->pos.y + 5, 0, "button_move");
  b_split = AddButtonPic(w->pos.x + 63, w->pos.y + 5, 0, "button_split");
  b_copy = AddButtonPic(w->pos.x + 93, w->pos.y + 5, 0, "button_copy");
  b_texture = AddButtonPic(w->pos.x + 123, w->pos.y + 5, 0, "button_texture");

  MoveButton(b_turn_left, w->pos.x + 3, w->pos.y + 33);
  MoveButton(b_turn_right, w->pos.x + 33, w->pos.y + 33);
  MoveButton(b_look_up, w->pos.x + 63, w->pos.y + 33);
  MoveButton(b_look_down, w->pos.x + 93, w->pos.y + 33);

  MoveButton(b_brush, w->pos.x + 3, w->pos.y + 5);
  MoveButton(b_move, w->pos.x + 33, w->pos.y + 5);
  MoveButton(b_split, w->pos.x + 63, w->pos.y + 5);
  MoveButton(b_copy, w->pos.x + 93, w->pos.y + 5);
  MoveButton(b_texture, w->pos.x + 123, w->pos.y + 5);

  DrawButtons();

  /*   QUI_AddButton("button_forward", w->pos.x+93, w->pos.y+63);
     QUI_AddButton("button_left", w->pos.x+3, w->pos.y+90);
     QUI_AddButton("button_down", w->pos.x+33, w->pos.y+90);
     QUI_AddButton("button_right", w->pos.x+63, w->pos.y+90);
     QUI_AddButton("button_up", w->pos.x+93, w->pos.y+63);
     QUI_AddButton("button_backward", w->pos.x+93, w->pos.y+90);*/
}
