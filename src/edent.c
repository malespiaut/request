/*
edent.c file of the Quest Source Code

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

#include "edent.h"

#include "3d.h"
#include "brush.h"
#include "button.h"
#include "camera.h"
#include "display.h"
#include "edbrush.h"
#include "edentity.h"
#include "edit.h"
#include "edvert.h"
#include "entclass.h"
#include "entity.h"
#include "error.h"
#include "keyboard.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "newgroup.h"
#include "popup.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "texlock.h"
#include "times.h"
#include "undo.h"
#include "video.h"

void
ClearSelEnts(void)
{
  entityref_t* e;

  M.display.num_eselected = 0;
  for (e = M.display.esel; e; e = M.display.esel)
  {
    M.display.esel = e->Next;
    Q_free(e);
  }
  M.display.esel = NULL;

  M.cur_entity = NULL;
}

static entityref_t*
FindSelEnt(entity_t* e)
{
  entityref_t* er;

  for (er = M.display.esel; er; er = er->Next)
    if (er->Entity == e)
      return er;

  return NULL;
}

static void
AddSelEnt(entity_t* e, int addgroup)
{
  entityref_t* er;

  if (FindSelEnt(e))
    return;

  er = Q_malloc(sizeof(entityref_t));
  if (!er)
  {
    HandleError("AddSelEnt", "Out of memory!");
    return;
  }
  er->Entity = e;
  er->Next = M.display.esel;
  er->Last = NULL;
  if (M.display.esel)
    M.display.esel->Last = er;
  M.display.esel = er;
  M.display.num_eselected++;

  if ((e->Group->flags & 0x04) && (addgroup))
  {
    entity_t* en;

    for (en = M.EntityHead; en; en = en->Next)
    {
      if (e->Group == en->Group)
      {
        AddSelEnt(en, 0);
      }
    }
  }
}

static void
RemoveSelEnt(entity_t* e)
{
  entityref_t* er;

  er = FindSelEnt(e);
  if (!er)
    return;

  if (er->Next)
    er->Next->Last = er->Last;
  if (er->Last)
    er->Last->Next = er->Next;
  if (er == M.display.esel)
    M.display.esel = er->Next;
  Q_free(er);
  M.display.num_eselected--;
}

static entity_t*
SelectEntity(int mx, int my)
{
  entity_t **list, **l;
  int n_list;

  int x2, y2;
  entity_t* e;

  int i, j;

  float time;

  list = NULL;
  n_list = 0;
  for (e = M.EntityHead; e; e = e->Next)
  {
    if (!e->drawn)
      continue;
    if (!e->strans.onscreen)
      continue;

    x2 = e->strans.x;
    y2 = e->strans.y;

    /* Enter into possible click list if within aperture */
    if ((mx > x2 - APERTURE) &&
        (mx < x2 + APERTURE) &&
        (my > y2 - APERTURE) &&
        (my < y2 + APERTURE))
    {
      l = Q_realloc(list, sizeof(entity_t*) * (n_list + 1));
      if (!l)
      {
        HandleError("SelectEntity", "Out of memory!");
        Q_free(list);
        break;
      }
      list = l;

      for (i = 0; i < n_list; i++)
      {
        if (list[i]->trans.z > e->trans.z)
          break;
      }
      for (j = n_list; j > i; j--)
      {
        list[j] = list[j - 1];
      }
      list[i] = e;
      n_list++;
    }
  }

  if (!n_list)
    return NULL;

  time = GetTime();

  e = NULL;
  while (1)
  {
    UpdateMouse();

    if ((GetTime() - time > 0.4) && (n_list != 1))
      break;
    if (!mouse.button)
    {
      e = *list;
      break;
    }
  }

  if (e)
  {
    Q_free(list);
    return e;
  }

  Popup_Init(mouse.x, mouse.y);
  for (i = 0; i < n_list; i++)
  {
    e = list[i];
    if (GetKeyValue(e, "origin"))
      Popup_AddStr("%s\t(%s)",
                   GetKeyValue(e, "classname"),
                   GetKeyValue(e, "origin"));
    else
      Popup_AddStr("%s\t(%g %g %g)",
                   GetKeyValue(e, "classname"),
                   e->center.x,
                   e->center.y,
                   e->center.z);
  }
  i = Popup_Display();
  Popup_Free();

  if (i == -1)
    e = NULL;
  else
    e = list[i];
  Q_free(list);
  return e;
}

entity_t*
FindEntity(int mx, int my)
{
  entity_t* best;
  float best_val;
  int x2, y2;
  entity_t* e;

  best = NULL;
  best_val = 0;
  for (e = M.EntityHead; e; e = e->Next)
  {

    if (!e->drawn)
      continue;
    if (!e->strans.onscreen)
      continue;

    x2 = e->strans.x;
    y2 = e->strans.y;

    /* Enter into possible click list if within aperture */
    if ((mx > x2 - APERTURE) &&
        (mx < x2 + APERTURE) &&
        (my > y2 - APERTURE) &&
        (my < y2 + APERTURE))
    {
      if (best)
        if (best_val < e->trans.z)
          continue;
      best = e;
      best_val = e->trans.z;
    }
  }

  return best;
}

void
HandleLeftClickEntity(void)
{
  entity_t* sel_ent;
  int base_x, base_y;
  int m_x, m_y;
  int d_x, d_y;
  int x_thresh, y_thresh;
  int i;

  if (TestKey(KEY_LF_SHIFT) || TestKey(KEY_RT_SHIFT))
  {
    sel_ent = SelectEntity(mouse.x, mouse.y);
    if (sel_ent != NULL)
    {
      if (FindSelEnt(sel_ent))
      {
        RemoveSelEnt(sel_ent);
      }
      else
      {
        M.cur_entity = sel_ent;
        QUI_RedrawWindow(STATUS_WINDOW);
        AddSelEnt(sel_ent, 1);
      }
      while (mouse.button == 1)
        GetMousePos();
      UpdateAllViewports();
    }
    else
    {
      base_x = mouse.x;
      base_y = mouse.y;
      SelectWindow();
      /* Add all vertecies in window to selection list */
      if ((mouse.x == base_x) && (mouse.y == base_y))
        ClearSelEnts();
      else
        SelectWindowEntity(base_x, base_y, mouse.x, mouse.y);
      UpdateAllViewports();
    }
  }
  else
  {
    /* Check for entity under mouse */
    sel_ent = FindEntity(mouse.x, mouse.y);
    if (sel_ent == NULL)
    {
      /* None, deselect all */
      ClearSelEnts();
      UpdateAllViewports();
    }
    else
    {
      AddSelEnt(sel_ent, 0);
      /* Now check for mouse drag */
      m_x = mouse.x;
      m_y = mouse.y;
      x_thresh = 2;
      y_thresh = 2;
      status.move = TRUE;
      status.move_amt.x = status.move_amt.y = status.move_amt.z = 0;
      i = 0;
      while (mouse.button == 1)
      {
        QUI_RedrawWindow(STATUS_WINDOW);
        GetMousePos();
        d_x = m_x - mouse.x;
        d_y = m_y - mouse.y;
        if ((d_x > x_thresh) || (-d_x > x_thresh) ||
            (d_y > y_thresh) || (-d_y > y_thresh))
        {
          if (!i)
          {
            SUndo(UNDO_CHANGE, UNDO_CHANGE);
            i = 1;
          }
          while (d_x > x_thresh)
          {
            MoveSelEnt(MOVE_LEFT, 0);
            d_x -= x_thresh;
          }
          while (-d_x > x_thresh)
          {
            MoveSelEnt(MOVE_RIGHT, 0);
            d_x += x_thresh;
          }
          while (d_y > y_thresh)
          {
            MoveSelEnt(MOVE_UP, 0);
            d_y -= y_thresh;
          }
          while (-d_y > y_thresh)
          {
            MoveSelEnt(MOVE_DOWN, 0);
            d_y += y_thresh;
          }
          SetMousePos(m_x, m_y);
          UpdateAllViewports();
        }
      }

      if (i)
        UndoDone();

      status.move = FALSE;
      QUI_RedrawWindow(STATUS_WINDOW);
      /*			ClearSelEnts();*/
      UpdateAllViewports();
    }
  }
}

void
SelectWindowEntity(int x0, int y0, int x1, int y1)
{
  int i;
  int x2, y2;
  entity_t* e1;

  if (x1 < x0)
  {
    i = x1;
    x1 = x0;
    x0 = i;
  }
  if (y1 < y0)
  {
    i = y1;
    y1 = y0;
    y0 = i;
  }

  for (e1 = M.EntityHead; e1; e1 = e1->Next)
  {
    if (!e1->drawn)
      continue;
    if (!e1->strans.onscreen)
      continue;

    x2 = e1->strans.x;
    y2 = e1->strans.y;

    if ((x2 > x0) && (x2 < x1) &&
        (y2 > y0) && (y2 < y1))
    {
      AddSelEnt(e1, 1);
    }
  }
}

void
MoveSelEnt(int dir, int update)
{
  int i;
  int dx, dy, dz;
  lvec3_t origin;
  char tempstr[256];
  entityref_t* e;
  brush_t* b;

  if (update)
    SUndo(UNDO_CHANGE, UNDO_CHANGE);

  for (e = M.display.esel; e; e = e->Next)
  {
    Move90(M.display.active_vport, dir, &dx, &dy, &dz, status.snap_size);
    if (!GetKeyValue(e->Entity, "origin"))
    {
      for (b = M.BrushHead; b; b = b->Next)
      {
        if (b->EntityRef == e->Entity)
        {
          for (i = 0; i < b->num_verts; i++)
          {
            b->verts[i].x += dx;
            b->verts[i].y += dy;
            b->verts[i].z += dz;
          }
          CalcBrushCenter(b);
          RecalcNormals(b);
        }
      }
    }
    else
    {
      sscanf(GetKeyValue(e->Entity, "origin"), "%d %d %d", &origin.x, &origin.y, &origin.z);

      origin.x += dx;
      origin.y += dy;
      origin.z += dz;

      sprintf(tempstr, "%d %d %d", origin.x, origin.y, origin.z);

      SetKeyValue(e->Entity, "origin", tempstr);
    }
  }

  if (update)
    UndoDone();
}

int
AddEntity(char* name, int x, int y, int z)
{
  entity_t* e;
  char tempstr[256];

  e = (entity_t*)Q_malloc(sizeof(entity_t));
  if (e == NULL)
  {
    HandleError("AddEntity", "Out of memory!");
    return FALSE;
  }
  memset(e, 0, sizeof(entity_t));
  InitEntity(e);
  if (M.EntityHead != NULL)
    M.EntityHead->Last = e;
  e->Next = M.EntityHead;
  e->Last = NULL;

  M.EntityHead = e;

  SetKeyValue(e, "classname", name);

  x = SnapPointToGrid(x);
  y = SnapPointToGrid(y);
  z = SnapPointToGrid(z);
  sprintf(tempstr, "%d %d %d", x, y, z);
  SetKeyValue(e, "origin", tempstr);

  SetEntityDefaults(e);

  AddDEntity(e);

  M.num_entities++;

  ClearSelVerts();
  ClearSelBrushes();
  QUI_RedrawWindow(STATUS_WINDOW);
  UpdateAllViewports();

  return TRUE;
}

int
RotateEntity(void)
{
  int* orig_ang;
  int d_x;
  int base_x;
  int ang;
  int i;
  char tempstr[256];
  entityref_t* e;

  if (status.edit_mode != ENTITY)
    return TRUE;

  if ((M.display.num_eselected == 0) || (M.display.esel == 0))
  {
    HandleError("Rotate Entity", "No entities selected");
    return TRUE;
  }

  NewMessage("Rotate entities.");
  NewMessage("Left-click to place, right-click to abort.");

  orig_ang = (int*)Q_malloc(sizeof(int) * M.display.num_eselected);

  for (e = M.display.esel, i = 0; e && (i < M.display.num_eselected); e = e->Next, i++)
  {
    if (GetKeyValue(e->Entity, "angle"))
      orig_ang[i] = atoi(GetKeyValue(e->Entity, "angle"));
    else
      orig_ang[i] = 0;
  }

  d_x = 0;
  base_x = mouse.x;

  while (mouse.button == 1)
    UpdateMouse();

  while ((mouse.button != 1) && (mouse.button != 2))
  {
    GetMousePos();
    if (mouse.moved)
      d_x += (mouse.prev_x - mouse.x) >> 1;

    SetMousePos(base_x, mouse.y);

    ang = floor((float)d_x / (float)status.angle_snap_size) * status.angle_snap_size;

    /* Update entity rotation amt */
    for (e = M.display.esel, i = 0; e && (i < M.display.num_eselected); e = e->Next, i++)
    {
      sprintf(tempstr, "%d", orig_ang[i] + ang);
      SetKeyValue(e->Entity, "angle", tempstr);
    }

    UpdateAllViewports();
  }

  if (mouse.button == 2)
  {
    for (e = M.display.esel, i = 0; e && (i < M.display.num_eselected); e = e->Next, i++)
    {
      sprintf(tempstr, "%d", orig_ang[i]);
      SetKeyValue(e->Entity, "angle", tempstr);
    }
  }

  while (mouse.button)
    UpdateMouse();

  Q_free(orig_ang);

  for (e = M.display.esel; e; e = e->Next)
  {
    while (atoi(GetKeyValue(e->Entity, "angle")) < 0)
    {
      sprintf(tempstr, "%d", atoi(GetKeyValue(e->Entity, "angle")) + 360);
      SetKeyValue(e->Entity, "angle", tempstr);
    }
    while (atoi(GetKeyValue(e->Entity, "angle")) >= 360)
    {
      sprintf(tempstr, "%d", atoi(GetKeyValue(e->Entity, "angle")) - 360);
      SetKeyValue(e->Entity, "angle", tempstr);
    }
  }

  UpdateAllViewports();
  return TRUE;
}

static void
DuplicateEntity(entity_t* to, entity_t* from)
{
  int i;

  /* Clear out "to" entity */
  if (to->numkeys > 0)
  {
    for (i = 0; i < to->numkeys; i++)
    {
      Q_free(to->key[i]);
      Q_free(to->value[i]);
    }
    Q_free(to->key);
    Q_free(to->value);
  }

  to->numkeys = 0;

  for (i = 0; i < from->numkeys; i++)
    SetKeyValue(to, from->key[i], from->value[i]);
}

void
EditEntity(void)
{
  entity_t** ents;
  int nents;
  entityref_t* e;

  if ((M.display.esel == NULL))
    return;

  nents = 0;
  ents = NULL;
  for (e = M.display.esel; e; e = e->Next)
  {
    nents++;
    ents = Q_realloc(ents, sizeof(entity_t*) * nents);
    ents[nents - 1] = e->Entity;
  }

  EditAnyEntity(nents, ents);

  Q_free(ents);
}

int
CopyEntity(void)
{
  entity_t* e;
  entity_t* e_new;
  entityref_t* er;
  brush_t* b;
  brush_t* b_new;

  /* Clear old clipboard */
  for (e = Clipboard.entities; e; e = Clipboard.entities)
  {
    Clipboard.entities = e->Next;

    while (e->numkeys != 0)
      RemoveKeyValue(e, e->key[0]);
    Q_free(e);
  }
  for (b = Clipboard.ent_br; b; b = Clipboard.ent_br)
  {
    Clipboard.ent_br = b->Next;

    B_Free(b);
  }
  Clipboard.entities = NULL;
  Clipboard.ent_br = NULL;

  /* Add entities to clipboard */
  for (er = M.display.esel; er; er = er->Next)
  {
    e_new = (entity_t*)Q_malloc(sizeof(entity_t));
    if (e_new == NULL)
    {
      HandleError("CopyEntity", "Unable to allocate clipboard entity.");
      return FALSE;
    }
    memset(e_new, 0, sizeof(entity_t));

    DuplicateEntity(e_new, er->Entity);

    for (b = M.BrushHead; b; b = b->Next)
    {
      if (b->EntityRef == er->Entity)
      {
        b_new = B_Duplicate(NULL, b, 0);
        if (!b_new)
        {
          HandleError("CopyEntity", "Unable to allocate clipboard brush.");
          return FALSE;
        }

        b_new->EntityRef = e_new;

        b_new->Next = Clipboard.ent_br;
        b_new->Last = NULL;
        if (Clipboard.ent_br)
          Clipboard.ent_br->Last = b_new;
        Clipboard.ent_br = b_new;
      }
    }

    if (Clipboard.entities != NULL)
    {
      e_new->Last = NULL;
      e_new->Next = Clipboard.entities;
      Clipboard.entities->Last = e_new;
      Clipboard.entities = e_new;
    }
    else
    {
      Clipboard.entities = e_new;
      Clipboard.entities->Last = NULL;
      Clipboard.entities->Next = NULL;
    }
  }

  /* Store base point for copy */
  Clipboard.e_base.x = M.display.vport[M.display.active_vport].camera_pos.x;
  Clipboard.e_base.y = M.display.vport[M.display.active_vport].camera_pos.y;
  Clipboard.e_base.z = M.display.vport[M.display.active_vport].camera_pos.z;

  return TRUE;
}

static entity_t*
PasteEntity_Askws(entity_t* eo)
{
  QUI_window_t* w;
  unsigned char* tempbuf;
  int bp, b_keep, b_use1, b_use2, b_merge;
  int i, y;

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];
  w->size.x = 450;
  w->size.y = 140;
  w->pos.x = (video.ScreenWidth - w->size.x) / 2;
  w->pos.y = (video.ScreenHeight - w->size.y) / 2;

  PushButtons();
  b_merge = AddButtonText(0, 0, 0, "Merge");
  b_use1 = AddButtonText(0, 0, 0, "Use existing");
  b_use2 = AddButtonText(0, 0, 0, "Use new");
  b_keep = AddButtonText(0, 0, 0, "Keep both");

  button[b_merge].y = button[b_use1].y = button[b_use2].y = button[b_keep].y =
    w->pos.y + w->size.y - button[b_merge].sy - 4;
  button[b_merge].x = w->pos.x + 4;
  button[b_use1].x = button[b_merge].x + button[b_merge].sx + 4;
  button[b_use2].x = button[b_use1].x + button[b_use1].sx + 4;
  button[b_keep].x = button[b_use2].x + button[b_use2].sx + 4;

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Pasting 'worldspawn'", &tempbuf);
  Q.num_popups++;

  y = w->pos.y + 30;

  QUI_DrawStr(w->pos.x + 5, y, 6, 15, 0, FALSE, "You're attempting to paste a 'worldspawn' entity!");
  y += 20;

  QUI_DrawStr(w->pos.x + 5, y, 6, 15, 0, FALSE, "Existing:");
  QUI_DrawStr(w->pos.x + 90, y, 6, 15, 0, FALSE, "%i extra keys", M.WorldSpawn->numkeys - 1);
  y += 20;

  QUI_DrawStr(w->pos.x + 5, y, 6, 15, 0, FALSE, "New:");
  QUI_DrawStr(w->pos.x + 90, y, 6, 15, 0, FALSE, "%i extra keys", eo->numkeys - 1);
  y += 20;

  QUI_DrawStr(w->pos.x + 5, y, 6, 15, 0, FALSE, "If you're unsure about what to do, select merge.");
  y += 20;

  DrawButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  while (1)
  {
    UpdateMouse();
    bp = UpdateButtons();
    if (bp != -1)
      break;
  }

  /* Pop down the window */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &tempbuf);

  RemoveButton(b_merge);
  RemoveButton(b_use1);
  RemoveButton(b_use2);
  RemoveButton(b_keep);
  PopButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);

  if (bp == b_keep)
    return NULL;

  if (bp == b_use1)
    return M.WorldSpawn;

  if (bp == b_use2)
  {
    for (i = 0; i < M.WorldSpawn->numkeys; i++)
    {
      Q_free(M.WorldSpawn->key[i]);
      Q_free(M.WorldSpawn->value[i]);
    }
    Q_free(M.WorldSpawn->key);
    Q_free(M.WorldSpawn->value);
    M.WorldSpawn->numkeys = 0;
    M.WorldSpawn->key = M.WorldSpawn->value = NULL;
  }

  for (i = 0; i < eo->numkeys; i++)
  {
    if (!GetKeyValue(M.WorldSpawn, eo->key[i]))
      SetKeyValue(M.WorldSpawn, eo->key[i], eo->value[i]);
  }

  return M.WorldSpawn;
}

int
PasteEntity(void)
{
  int numnew;

  lvec3_t origin;
  char tempstr[256];

  entityref_t* e;
  entity_t* e_orig;
  entity_t* e_new;
  entityref_t* NewSelectList;
  brush_t* b;
  brush_t* b_new;

  int i;

  lvec3_t delta;

  if (Clipboard.entities == NULL)
  {
    QUI_Dialog("Paste Entity", "No entities in Clipboard");
    return FALSE;
  }

  delta.x = M.display.vport[M.display.active_vport].camera_pos.x -
            Clipboard.e_base.x;
  delta.y = M.display.vport[M.display.active_vport].camera_pos.y -
            Clipboard.e_base.y;
  delta.z = M.display.vport[M.display.active_vport].camera_pos.z -
            Clipboard.e_base.z;

  delta.x = SnapPointToGrid(delta.x);
  delta.y = SnapPointToGrid(delta.y);
  delta.z = SnapPointToGrid(delta.z);

  if (!delta.x && !delta.y && !delta.z)
  {
    Move(M.display.active_vport, MOVE_UP, &delta.x, &delta.y, &delta.z, status.snap_size);

    delta.x = SnapPointToGrid(delta.x);
    delta.y = SnapPointToGrid(delta.y);
    delta.z = SnapPointToGrid(delta.z);
  }

  SUndo(UNDO_NONE, UNDO_NONE);

  NewSelectList = NULL;
  numnew = 0;
  for (e_orig = Clipboard.entities; e_orig; e_orig = e_orig->Next)
  {
    e_new = NULL;
    if (!strcmp(GetKeyValue(e_orig, "classname"), "worldspawn"))
    {
      e_new = PasteEntity_Askws(e_orig);
    }

    if (!e_new)
    {
      e_new = (entity_t*)Q_malloc(sizeof(entity_t));
      if (e_new == NULL)
      {
        HandleError("Copy Entity", "Could not allocate new entity");
        return FALSE;
      }
      InitEntity(e_new);

      DuplicateEntity(e_new, e_orig);

      if (GetKeyValue(e_new, "origin"))
      {
        /* Adjust entity for new camera position */
        sscanf(GetKeyValue(e_new, "origin"), "%d %d %d", &origin.x, &origin.y, &origin.z);
        origin.x += delta.x;
        origin.y += delta.y;
        origin.z += delta.z;
        sprintf(tempstr, "%d %d %d", origin.x, origin.y, origin.z);
        SetKeyValue(e_new, "origin", tempstr);
      }

      e_new->Last = NULL;
      e_new->Next = M.EntityHead;
      if (M.EntityHead)
        M.EntityHead->Last = e_new;
      M.EntityHead = e_new;

      AddDEntity(e_new);

      M.num_entities++;
    }

    for (b = Clipboard.ent_br; b; b = b->Next)
    {
      if (b->EntityRef == e_orig)
      {
        b_new = B_Duplicate(NULL, b, 1);
        if (b_new == NULL)
        {
          HandleError("Copy Entity", "Could not allocate new brush.");
          return FALSE;
        }

        for (i = 0; i < b_new->num_verts; i++)
        {
          b_new->verts[i].x += delta.x;
          b_new->verts[i].y += delta.y;
          b_new->verts[i].z += delta.z;
        }

        b_new->EntityRef = e_new;
        b_new->Group = FindVisGroup(M.WorldGroup);

        B_Link(b_new);

        AddDBrush(b_new);

        TexLock(b_new, b);
      }
    }

    if (NewSelectList != NULL)
    {
      e = (entityref_t*)Q_malloc(sizeof(entityref_t));
      if (e == NULL)
      {
        HandleError("Copy Entity", "Could not allocate new select list");
        return FALSE;
      }
      e->Entity = e_new;

      e->Last = NULL;
      e->Next = NewSelectList;
      NewSelectList->Last = e;
      NewSelectList = e;
    }
    else
    {
      NewSelectList = (entityref_t*)Q_malloc(sizeof(entityref_t));
      if (NewSelectList == NULL)
      {
        HandleError("Copy Entity", "Could not allocate new select list");
        return FALSE;
      }
      NewSelectList->Entity = e_new;

      NewSelectList->Last = NULL;
      NewSelectList->Next = NULL;
    }
    numnew++;
  }

  ClearSelEnts();

  /* assign the new selected list */
  M.display.esel = NewSelectList;
  M.display.num_eselected = numnew;

  return TRUE;
}

static void
RemoveT(entity_t* ign, const char* n1, const char* n2)
{
  entity_t* e;
  char t[256];
  int found;

  if (GetKeyValue(ign, n1))
  {
    strcpy(t, GetKeyValue(ign, n1));

    found = 0;
    for (e = M.EntityHead; e; e = e->Next)
    {
      if (e != ign)
      {
        if (GetKeyValue(e, n1))
        {
          found = 1;
          break;
        }
      }
    }
    if (!found)
    {
      for (e = M.EntityHead; e; e = e->Next)
      {
        if (GetKeyValue(e, n2))
        {
          if (!strcmp(t, GetKeyValue(e, n2)))
          {
            RemoveKeyValue(e, n2);
          }
        }
      }
    }
  }
}

void
DeleteEntity(entity_t* e)
{
  brush_t* b;

  if (e == M.WorldSpawn)
  {
    HandleError("DeleteEntity", "Tried to delete worldspawn.");
    return;
  }

  RemoveT(e, "target", "targetname");
  RemoveT(e, "targetname", "target");

  /* Update brush-entity ref */
  for (b = M.BrushHead; b; b = b->Next)
  {
    if (b->EntityRef == e)
      b->EntityRef = M.WorldSpawn;
  }

  if (e->Next != NULL)
    e->Next->Last = e->Last;
  if (e->Last != NULL)
    e->Last->Next = e->Next;
  if (e == M.EntityHead)
    M.EntityHead = e->Next;
  while (e->numkeys != 0)
    RemoveKeyValue(e, e->key[0]);
  Q_free(e);
  M.num_entities--;
}

void
DeleteEntities(void)
{
  entityref_t* e;

  for (e = M.display.esel; e; e = e->Next)
  {
    DeleteEntity(e->Entity);
  }
  ClearSelEnts();
}
