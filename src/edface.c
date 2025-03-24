/*
edface.c file of the Quest Source Code

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

#include "edface.h"

#include "3d.h"
#include "brush.h"
#include "camera.h"
#include "check.h"
#include "edit.h"
#include "edvert.h"
#include "error.h"
#include "jvert.h"
#include "keyboard.h"
#include "map.h"
#include "memory.h"
#include "mouse.h"
#include "popup.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "tex.h"
#include "texdef.h"
#include "times.h"
#include "undo.h"

fsel_t*
FindFace(int vport, int mx, int my)
{
  int i;
  fsel_t best;
  brush_t* b;
  fsel_t* fnew;
  float best_val;
  int x2, y2;

  best.Brush = NULL;
  best_val = 0;
  for (b = M.BrushHead; b; b = b->Next)
  {
    if (b->drawn)
    {
      if (b->bt->type == BR_Q3_CURVE)
        continue;
      for (i = 0; i < b->num_planes; i++)
      {
        if (!b->plane[i].scenter.onscreen)
          continue;

        x2 = b->plane[i].scenter.x;
        y2 = b->plane[i].scenter.y;

        /* Enter into possible click list if within aperture */
        if ((mx > x2 - APERTURE) &&
            (mx < x2 + APERTURE) &&
            (my > y2 - APERTURE) &&
            (my < y2 + APERTURE))
        {
          if (best.Brush)
            if (best_val < b->plane[i].tcenter.z)
              continue;
          best.Brush = b;
          best.facenum = i;
          best_val = b->plane[i].tcenter.z;
        }
      }
    }
  }

  if (!best.Brush)
    return NULL;

  fnew = (fsel_t*)Q_malloc(sizeof(fsel_t));
  fnew->Brush = best.Brush;
  fnew->facenum = best.facenum;
  fnew->Last = NULL;
  fnew->Next = NULL;

  return fnew;
}

static fsel_t*
SelectFace(int vport, int mx, int my)
{
  fsel_t *list, *l;
  int n_list;

  int x2, y2;

  fsel_t *f, *f2;
  brush_t* b;
  plane_t* p;

  int i, j, k;

  float time;

  list = NULL;
  n_list = 0;
  if (M.display.vport[vport].mode == SOLID)
  {
    Solid_SelectFaces(&list, &n_list);
  }
  else
  {
    for (b = M.BrushHead; b; b = b->Next)
    {
      if (b->drawn)
      {
        if (b->bt->type == BR_Q3_CURVE)
          continue;
        for (k = 0; k < b->num_planes; k++)
        {
          p = &b->plane[k];
          if (!p->scenter.onscreen)
            continue;

          x2 = p->scenter.x;
          y2 = p->scenter.y;

          /* Enter into possible click list if within aperture */
          if ((mx > x2 - APERTURE) &&
              (mx < x2 + APERTURE) &&
              (my > y2 - APERTURE) &&
              (my < y2 + APERTURE))
          {
            l = Q_realloc(list, sizeof(fsel_t) * (n_list + 1));
            if (!l)
            {
              HandleError("SelectFace", "Out of memory!");
              Q_free(list);
              break;
            }
            list = l;

            for (i = 0; i < n_list; i++)
            {
              if (list[i].Brush->plane[list[i].facenum].tcenter.z >
                  p->tcenter.z)
                break;
            }
            for (j = n_list; j > i; j--)
            {
              list[j] = list[j - 1];
            }
            memset(&list[i], 0, sizeof(fsel_t));
            list[i].Brush = b;
            list[i].facenum = k;
            n_list++;
          }
        }
      }
    }
  }

  if (!n_list)
    return NULL;

  time = GetTime();

  f = NULL;
  while (1)
  {
    UpdateMouse();

    if ((GetTime() - time > 0.4) /*&& (n_list!=1)*/)
      break;
    if (!mouse.button)
    {
      f = list;
      break;
    }
  }

  if (f)
  {
    f2 = Q_malloc(sizeof(fsel_t));
    if (!f2)
    {
      HandleError("SelectFace", "Out of memory!");
      Q_free(list);
      return NULL;
    }
    *f2 = *f;
    //      printf("%i in list, bailing out, %p\n",n_list,e);
    Q_free(list);
    return f2;
  }

  Popup_Init(mouse.x, mouse.y);
  for (i = 0; i < n_list; i++)
  {
    f = &list[i];
    p = &f->Brush->plane[f->facenum];
    Popup_AddStr("%s\t(%g %g %g)",
                 p->tex.name,
                 p->center.x,
                 p->center.y,
                 p->center.z);
  }
  i = Popup_Display();
  Popup_Free();

  //   printf("%i in list, %i, %p\n",n_list,i,i==-1?NULL:list[i]);

  if (i == -1)
    f = NULL;
  else
  {
    f = Q_malloc(sizeof(fsel_t));
    if (!f)
    {
      Q_free(list);
      HandleError("SelectFace", "Out of memory!");
      return NULL;
    }
    *f = list[i];
  }
  Q_free(list);
  return f;
}

static void
SelectWindowFace(int vport, int x0, int y0, int x1, int y1)
{
  int numnew;
  int i;
  int x2, y2;
  brush_t* b;
  fsel_t* f;
  fsel_t* newf;
  int found;

  if (x0 > x1)
  {
    i = x1;
    x1 = x0;
    x0 = i;
  }
  if (y0 > y1)
  {
    i = y1;
    y1 = y0;
    y0 = i;
  }

  numnew = 0;
  for (b = M.BrushHead; b; b = b->Next)
  {
    if (b->drawn)
    {
      if (b->bt->type == BR_Q3_CURVE)
        continue;
      for (i = 0; i < b->num_planes; i++)
      {
        if (!b->plane[i].scenter.onscreen)
          continue;

        x2 = b->plane[i].scenter.x;
        y2 = b->plane[i].scenter.y;

        /* Enter into possible click list if within aperture */
        if ((x2 > x0) && (x2 < x1) &&
            (y2 > y0) && (y2 < y1))
        {
          /* Don't re-add a face */
          found = FALSE;
          for (f = M.display.fsel; f; f = f->Next)
          {
            if ((f->Brush == b) && (f->facenum == i))
            {
              found = TRUE;
              break;
            }
          }
          /* Add it  */
          if (!found)
          {
            if (M.display.fsel == NULL)
            {
              M.display.fsel = (fsel_t*)Q_malloc(sizeof(fsel_t));
              M.display.fsel->Last = NULL;
              M.display.fsel->Next = NULL;
              M.display.num_fselected = 1;
            }
            else
            {
              newf = (fsel_t*)Q_malloc(sizeof(fsel_t));
              newf->Last = NULL;
              newf->Next = M.display.fsel;
              M.display.fsel->Last = newf;
              M.display.fsel = newf;
              M.display.num_fselected++;
            }
            M.display.fsel->Brush = b;
            M.display.fsel->facenum = i;
          }
        }
      }
    }
  }
}

static void
SelectWindowFaceVert(int vport, int x0, int y0, int x1, int y1)
{
  int numnew;
  int i, j;
  fsel_t* f;
  vsel_t* v;
  brush_t* b;
  plane_t* p;
  int found;
  int x2, y2;

  if (x0 > x1)
  {
    i = x1;
    x1 = x0;
    x0 = i;
  }
  if (y0 > y1)
  {
    i = y1;
    y1 = y0;
    y0 = i;
  }

  numnew = 0;
  for (f = M.display.fsel; f; f = f->Next)
  {
    b = f->Brush;
    p = &b->plane[f->facenum];
    for (j = 0; j < p->num_verts; j++)
    {
      if (!b->sverts[p->verts[j]].onscreen)
        continue;

      x2 = b->sverts[p->verts[j]].x;
      y2 = b->sverts[p->verts[j]].y;

      if ((x2 > x0) && (x2 < x1) &&
          (y2 > y0) && (y2 < y1))
      {
        numnew++;
        found = FALSE;
        for (v = M.display.vsel; v; v = v->Next)
        {
          if (v->tvert == &(b->tverts[p->verts[j]]))
          {
            found = TRUE;
            break;
          }
        }
        if (!found)
        {
          if (M.display.vsel != NULL)
          {
            v = (vsel_t*)Q_malloc(sizeof(vsel_t));
            if (v == NULL)
            {
              HandleError("Select Verts", "Could not allocate select list");
              return;
            }
            v->tvert = &(b->tverts[p->verts[j]]);
            v->svert = &(b->sverts[p->verts[j]]);
            v->vert = &(b->verts[p->verts[j]]);
            v->Last = NULL;
            v->Next = M.display.vsel;
            M.display.vsel->Last = v;
            M.display.vsel = v;
            M.display.num_vselected++;
          }
          else
          {
            M.display.vsel = (vsel_t*)Q_malloc(sizeof(vsel_t));
            if (M.display.vsel == NULL)
            {
              HandleError("Select Verts", "Could not allocate select list");
              return;
            }
            M.display.vsel->tvert = &(b->tverts[p->verts[j]]);
            M.display.vsel->svert = &(b->sverts[p->verts[j]]);
            M.display.vsel->vert = &(b->verts[p->verts[j]]);
            M.display.vsel->Last = NULL;
            M.display.vsel->Next = NULL;
            M.display.num_vselected = 1;
          }
        }
      }
    }
  }
  /*	if (numnew==0) ClearSelVerts();*/
}

static vsel_t*
FindFaceVertex(int vport, int mx, int my)
{
  int j;
  vsel_t best;
  float best_val;
  int x2, y2;
  fsel_t* f;
  brush_t* b;
  plane_t* p;
  vsel_t* v;

  best.vert = NULL;
  best_val = 0;
  for (f = M.display.fsel; f; f = f->Next)
  {
    b = f->Brush;
    p = &f->Brush->plane[f->facenum];
    for (j = 0; j < p->num_verts; j++)
    {
      if (!b->sverts[p->verts[j]].onscreen)
        continue;

      x2 = b->sverts[p->verts[j]].x;
      y2 = b->sverts[p->verts[j]].y;

      /* Enter into possible click list if within aperture */
      if ((mx > x2 - APERTURE) &&
          (mx < x2 + APERTURE) &&
          (my > y2 - APERTURE) &&
          (my < y2 + APERTURE))
      {
        if (best.vert)
          if (best_val < b->tverts[p->verts[j]].z)
            continue;

        best.tvert = &(b->tverts[p->verts[j]]);
        best.svert = &(b->sverts[p->verts[j]]);
        best.vert = &(b->verts[p->verts[j]]);

        best_val = b->tverts[p->verts[j]].z;
      }
    }
  }

  if (!best.vert)
    return NULL;

  v = (vsel_t*)Q_malloc(sizeof(vsel_t));
  if (v == NULL)
  {
    HandleError("Select Verts", "Could not allocate select list");
    return NULL;
  }
  v->tvert = best.tvert;
  v->svert = best.svert;
  v->vert = best.vert;
  v->Next = NULL;
  v->Last = NULL;
  return v;
}

void
HandleLeftClickFace(void)
{
  vsel_t* v;
  vsel_t* sel_vert;
  fsel_t* sel_face;
  fsel_t* f;
  int m_x, m_y;
  int d_x, d_y;
  int x_thresh, y_thresh;
  int base_x, base_y;
  int found;
  int i;

  /* Click in active vport */
  if (TestKey(KEY_CONTROL))
  {
    sel_face = SelectFace(M.display.active_vport, mouse.x, mouse.y);
    if (sel_face != NULL)
    {
      /* Check if we're adding or removing */
      found = FALSE;
      for (f = M.display.fsel; f; f = f->Next)
      {
        if ((f->Brush == sel_face->Brush) && (f->facenum == sel_face->facenum))
        {
          /* Removing */
          found = TRUE;
          if (f->Last != NULL)
            f->Last->Next = f->Next;
          if (f->Next != NULL)
            f->Next->Last = f->Last;
          if (f == M.display.fsel)
            M.display.fsel = f->Next;
          Q_free(f);
          Q_free(sel_face);
          M.display.num_fselected--;
          break;
        }
      }
      if (!found)
      {
        /* Adding */
        if (M.display.fsel == NULL)
        {
          M.display.fsel = sel_face;
          M.display.fsel->Last = NULL;
          M.display.fsel->Next = NULL;
          M.display.num_fselected = 1;
        }
        else
        {
          sel_face->Last = NULL;
          sel_face->Next = M.display.fsel;
          M.display.fsel->Last = sel_face;
          M.display.fsel = sel_face;
          M.display.num_fselected++;
        }
      }
    }
    else
    {
      base_x = mouse.x;
      base_y = mouse.y;
      SelectWindow();
      if ((mouse.x == base_x) && (mouse.y == base_y))
      {
        /* None, deselect all */
        ClearSelFaces();
        ClearSelVerts();
      }
      else
        SelectWindowFace(M.display.active_vport, base_x, base_y, mouse.x, mouse.y);
    }

    /* Wait for let up of mouse, and redraw all vports */
    while (mouse.button == 1)
      GetMousePos();
    UpdateAllViewports();
  }
  else if (TestKey(KEY_LF_SHIFT) || TestKey(KEY_RT_SHIFT))
  {
    sel_vert = FindFaceVertex(M.display.active_vport, mouse.x, mouse.y);
    if (sel_vert != NULL)
    {
      /* Check if we're adding or removing */
      found = FALSE;
      for (v = M.display.vsel; v; v = v->Next)
      {
        if (v->tvert == sel_vert->tvert)
        {
          Q_free(sel_vert);
          /* Removing */
          found = TRUE;
          if (v->Last != NULL)
            (v->Last)->Next = v->Next;
          if (v->Next != NULL)
            (v->Next)->Last = v->Last;
          if (v == M.display.vsel)
            M.display.vsel = v->Next;

          Q_free(v);
          M.display.num_vselected--;
          break;
        }
      }
      if (!found)
      {
        /* Adding */
        if (M.display.vsel != NULL)
        {
          v = sel_vert;
          v->Last = NULL;
          v->Next = M.display.vsel;
          M.display.vsel->Last = v;
          M.display.vsel = v;
          M.display.num_vselected++;
        }
        else
        {
          M.display.vsel = sel_vert;
          M.display.vsel->Last = NULL;
          M.display.vsel->Next = NULL;
          M.display.num_vselected = 1;
        }
      }
      while (mouse.button == 1)
        GetMousePos();
    }
    else
    {
      base_x = mouse.x;
      base_y = mouse.y;
      SelectWindow();
      /* Add all vertecies in window to selection list */
      if ((mouse.x == base_x) && (mouse.y == base_y))
      {
        ClearSelVerts();
      }
      else
      {
        SelectWindowFaceVert(M.display.active_vport, base_x, base_y, mouse.x, mouse.y);
      }
    }
    while (mouse.button == 1)
      GetMousePos();
    UpdateAllViewports();
  }
  else
  {
    /* Check for vertex under mouse */
    sel_vert = FindFaceVertex(M.display.active_vport, mouse.x, mouse.y);
    if (sel_vert == NULL)
    {
      /* None, deselect all vertices */
      ClearSelVerts();
      UpdateAllViewports();
    }
    else
    {
      /* Vertex found, add to select list if it isn't
        already there */
      found = FALSE;
      for (v = M.display.vsel; v; v = v->Next)
      {
        if (v->tvert == sel_vert->tvert)
        {
          found = TRUE;
          break;
        }
      }
      if (!found)
      {
        if (M.display.vsel != NULL)
        {
          v = sel_vert;
          v->Last = NULL;
          v->Next = M.display.vsel;
          M.display.vsel->Last = v;
          M.display.vsel = v;
          M.display.num_vselected++;
        }
        else
        {
          M.display.vsel = sel_vert;
          M.display.vsel->Last = NULL;
          M.display.vsel->Next = NULL;
          M.display.num_vselected = 1;
        }
      }
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
            SUndo(UNDO_NONE, UNDO_CHANGE);
            i = 1;
          }
          while (d_x > x_thresh)
          {
            MoveSelVert(MOVE_LEFT, 0);
            d_x -= x_thresh;
          }
          while (-d_x > x_thresh)
          {
            MoveSelVert(MOVE_RIGHT, 0);
            d_x += x_thresh;
          }
          while (d_y > y_thresh)
          {
            MoveSelVert(MOVE_UP, 0);
            d_y -= y_thresh;
          }
          while (-d_y > y_thresh)
          {
            MoveSelVert(MOVE_DOWN, 0);
            d_y += y_thresh;
          }
          SetMousePos(m_x, m_y);
          UpdateAllViewports();
        }
      }
      status.move = FALSE;
      QUI_RedrawWindow(STATUS_WINDOW);
      /* Update all involved brush centers & normals */
      for (f = M.display.fsel; f; f = f->Next)
      {
        JoinVertices(f->Brush);
        CalcBrushCenter(f->Brush);
        RecalcNormals(f->Brush);
      }
      if (i)
        UndoDone();
      for (f = M.display.fsel; f; f = f->Next)
      {
        CheckBrush(f->Brush, FALSE);
      }

      UpdateAllViewports();
    }
  }
}

void
ClearSelFaces(void)
{
  fsel_t* f;

  M.display.num_fselected = 0;
  for (f = M.display.fsel; f; f = M.display.fsel)
  {
    M.display.fsel = f->Next;
    Q_free(f);
  }
  M.display.fsel = NULL;

  ClearSelVerts();
}

void
HighlightAllFaces(void)
{
  int j;
  fsel_t* f;
  brush_t* b;
  plane_t* p;

  ClearSelVerts();

  for (f = M.display.fsel; f; f = f->Next)
  {
    b = f->Brush;
    p = &b->plane[f->facenum];
    for (j = 0; j < p->num_verts; j++)
    {
      AddSelectVert(b, p->verts[j]);
    }
  }
  UpdateAllViewports();
}

void
ApplyFaceTexture(fsel_t* f, char* texname)
{
  if (f->Brush->bt->flags & BR_F_BTEXDEF)
    SetTexture(&f->Brush->tex, texname);
  else
    SetTexture(&f->Brush->plane[f->facenum].tex, texname);
}
