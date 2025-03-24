/*
edvert.c file of the Quest Source Code

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

#include "edvert.h"

#include "brush.h"
#include "camera.h"
#include "check.h"
#include "display.h"
#include "error.h"
#include "jvert.h"
#include "map.h"
#include "memory.h"
#include "quest.h"
#include "status.h"
#include "undo.h"

void
ClearSelVerts(void)
{
  vsel_t* v;

  M.display.num_vselected = 0;
  for (v = M.display.vsel; v; v = M.display.vsel)
  {
    M.display.vsel = v->Next;
    Q_free(v);
  }
  M.display.vsel = NULL;
}

vsel_t*
FindSelectVert(vec3_t* vec)
{
  vsel_t* v;

  for (v = M.display.vsel; v; v = v->Next)
  {
    if (v->vert == vec)
    {
      return v;
    }
  }
  return NULL;
}

void
AddSelectVert(brush_t* b, int num)
{
  vsel_t* v;

  if (FindSelectVert(&b->verts[num]))
    return;

  if (M.display.vsel != NULL)
  {
    v = (vsel_t*)Q_malloc(sizeof(vsel_t));
    if (v == NULL)
    {
      HandleError("Select Verts", "Could not allocate select list");
      return;
    }
    v->tvert = &(b->tverts[num]);
    v->svert = &(b->sverts[num]);
    v->vert = &(b->verts[num]);
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
    M.display.vsel->tvert = &(b->tverts[num]);
    M.display.vsel->svert = &(b->sverts[num]);
    M.display.vsel->vert = &(b->verts[num]);
    M.display.vsel->Last = NULL;
    M.display.vsel->Next = NULL;
    M.display.num_vselected = 1;
  }
}

void
AddSelectVec(vec3_t* v)
{
  brushref_t* b;
  int i;

  for (b = M.display.bsel; b; b = b->Next)
  {
    for (i = 0; i < b->Brush->num_verts; i++)
    {
      if (v == (&b->Brush->verts[i]))
      {
        AddSelectVert(b->Brush, i);
        return;
      }
    }
  }
}

vec3_t*
FindVertex(int vport, int mx, int my)
{
  int i;
  vec3_t* best;
  float best_val;
  brushref_t* b;
  brush_t* br;

  best = NULL;
  best_val = 0;
  for (b = M.display.bsel; b; b = b->Next)
  {
    br = b->Brush;
    for (i = 0; i < br->num_verts; i++)
    {
      if (!br->sverts[i].onscreen)
        continue;

      /* Enter into possible click list if within aperture */
      if ((mx > br->sverts[i].x - APERTURE) &&
          (mx < br->sverts[i].x + APERTURE) &&
          (my > br->sverts[i].y - APERTURE) &&
          (my < br->sverts[i].y + APERTURE))
      {
        if (best)
          if (best_val < br->tverts[i].z)
            continue;
        best = &br->verts[i];

        best_val = br->tverts[i].z;
      }
    }
  }

  return best;
}

void
SelectWindowVert(int vport, int x0, int y0, int x1, int y1)
{
  int numnew;
  int i, j;
  brushref_t* b;
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
  for (b = M.display.bsel; b; b = b->Next)
  {
    for (j = 0; j < b->Brush->num_verts; j++)
    {
      if (!b->Brush->sverts[j].onscreen)
        continue;
      x2 = b->Brush->sverts[j].x;
      y2 = b->Brush->sverts[j].y;
      if ((x2 > x0) && (x2 < x1) &&
          (y2 > y0) && (y2 < y1))
      {
        numnew++;
        AddSelectVert(b->Brush, j);
      }
    }
  }
  if (numnew == 0)
    ClearSelVerts();
}

void
MoveSelVert(int dir, int recalc)
{
  int dx, dy, dz;
  vsel_t* v;
  brushref_t* b;
  fsel_t* f;

  if (recalc)
    SUndo(UNDO_NONE, UNDO_CHANGE);

  Move90(M.display.active_vport, dir, &dx, &dy, &dz, status.snap_size);
  status.move_amt.x += dx;
  status.move_amt.y += dy;
  status.move_amt.z += dz;
  for (v = M.display.vsel; v; v = v->Next)
  {
    v->vert->x += dx;
    v->vert->y += dy;
    v->vert->z += dz;
  }
  for (b = M.display.bsel; b; b = b->Next)
  {
    CalcBrushCenter(b->Brush);
  }
  for (f = M.display.fsel; f; f = f->Next)
  {
    CalcBrushCenter(f->Brush);
  }

  if (recalc)
  {
    for (b = M.display.bsel; b; b = b->Next)
    {
      CalcBrushCenter(b->Brush);
      RecalcNormals(b->Brush);
    }
    for (f = M.display.fsel; f; f = f->Next)
    {
      CalcBrushCenter(f->Brush);
      RecalcNormals(f->Brush);
    }

    UndoDone();

    for (b = M.display.bsel; b; b = b->Next)
    {
      JoinVertices(b->Brush);
      CheckBrush(b->Brush, FALSE);
    }
    for (f = M.display.fsel; f; f = f->Next)
    {
      JoinVertices(f->Brush);
      CheckBrush(f->Brush, FALSE);
    }
  }
}
