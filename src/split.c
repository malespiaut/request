/*
split.c file of the Quest Source Code

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

#include "split.h"

#include "3d.h"
#include "brush.h"
#include "display.h"
#include "edvert.h"
#include "error.h"
#include "keyboard.h"
#include "keyhit.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "quest.h"

static void
DoSplit(int v1, int v2)
{
  brush_t* b;
  int e1;
  int i, j;
  plane_t *f1, *f2;
  int nv1;
  int* verts1;
  int nv2;
  int* verts2;

  b = M.display.fsel->Brush;

  if (v2 < v1)
  {
    int i;

    i = v2;
    v2 = v1;
    v1 = i;
  }

  for (i = 0; i < b->num_edges; i++)
  {
    if ((b->edges[i].startvertex == v1 && b->edges[i].endvertex == v2) ||
        (b->edges[i].startvertex == v2 && b->edges[i].endvertex == v1))
    {
      HandleError("SplitFace", "Vertices on same line");
      return;
    }
  }

  e1 = b->num_edges++;

  b->edges = Q_realloc(b->edges, sizeof(edge_t) * b->num_edges);

  b->edges[e1].startvertex = v1;
  b->edges[e1].endvertex = v2;

  b->num_planes++;
  b->plane = Q_realloc(b->plane, sizeof(plane_t) * b->num_planes);

  f1 = &b->plane[M.display.fsel->facenum];
  f2 = &b->plane[b->num_planes - 1];

  *f2 = *f1;

  nv1 = nv2 = 0;
  verts1 = verts2 = NULL;
  j = 0;

  for (i = 0; i < f1->num_verts; i++)
  {
    if (!j)
    {
      nv1++;
      verts1 = Q_realloc(verts1, nv1 * sizeof(int));
      verts1[nv1 - 1] = f1->verts[i];
      if (f1->verts[i] == v1 || f1->verts[i] == v2)
      {
        nv2++;
        verts2 = Q_realloc(verts2, nv2 * sizeof(int));
        verts2[nv2 - 1] = f1->verts[i];
        j++;
      }
    }
    else
    {
      nv2++;
      verts2 = Q_realloc(verts2, nv2 * sizeof(int));
      verts2[nv2 - 1] = f1->verts[i];
      if (f1->verts[i] == v1 || f1->verts[i] == v2)
      {
        nv1++;
        verts1 = Q_realloc(verts1, nv1 * sizeof(int));
        verts1[nv1 - 1] = f1->verts[i];
        j = 0;
      }
    }
  }

  Q_free(f1->verts);

  f1->verts = verts1;
  f1->num_verts = nv1;
  f2->verts = verts2;
  f2->num_verts = nv2;
}

void
SplitFace(void)
{
  brush_t* b;
  int v1, v2;
  int i;

  if (M.display.num_fselected != 1)
  {
    HandleError("SplitFace", "Exactly one face must be selected");
    return;
  }
  if (M.display.num_vselected != 2)
  {
    HandleError("SplitFace", "Exactly two vertices must be selected");
    return;
  }

  b = M.display.fsel->Brush;
  if (b->bt->type != BR_NORMAL)
  {
    HandleError("SplitFace", "Can only split normal brushes");
    return;
  }

  v1 = v2 = -1;
  for (i = 0; i < b->num_verts; i++)
  {
    if ((int)&b->verts[i] == (int)M.display.vsel->vert)
      v1 = i;
    if ((int)&b->verts[i] == (int)M.display.vsel->Next->vert)
      v2 = i;
  }

  if (v1 == -1 || v2 == -1)
  {
    HandleError("SplitFace", "Couldn't find vertices in brush (internal error?)");
    return;
  }

  DoSplit(v1, v2);

  ClearSelVerts();
  RecalcAllNormals();
  CalcBrushCenter(b);
  UpdateAllViewports();
}

int
SplitLine(void)
{
  brush_t* b;
  plane_t* f;
  int v1, v2, v3;
  int i, j, k;
  int e1, e2;

  if (M.display.num_fselected != 1)
  {
    HandleError("SplitLine", "Exactly one face must be selected");
    return -1;
  }
  if (M.display.num_vselected != 2)
  {
    HandleError("SplitLine", "Exactly two vertices must be selected");
    return -1;
  }

  b = M.display.fsel->Brush;
  if (b->bt->type != BR_NORMAL)
  {
    HandleError("SplitFace", "Can only split normal brushes");
    return -1;
  }

  v1 = v2 = -1;
  for (i = 0; i < b->num_verts; i++)
  {
    if ((int)&b->verts[i] == (int)M.display.vsel->vert)
      v1 = i;
    if ((int)&b->verts[i] == (int)M.display.vsel->Next->vert)
      v2 = i;
  }

  if (v1 == -1 || v2 == -1)
  {
    HandleError("SplitLine", "Couldn't find vertices in brush (internal error?)");
    return -1;
  }

  if (v2 < v1)
  {
    int i;

    i = v2;
    v2 = v1;
    v1 = i;
  }

  for (i = 0; i < b->num_edges; i++)
  {
    if ((b->edges[i].startvertex == v1 && b->edges[i].endvertex == v2) ||
        (b->edges[i].startvertex == v2 && b->edges[i].endvertex == v1))
      break;
  }
  if (i == b->num_edges)
  {
    HandleError("SplitLine", "Vertices not on same line");
    return -1;
  }
  e1 = i;

  v3 = b->num_verts++;

  b->verts = Q_realloc(b->verts, sizeof(vec3_t) * b->num_verts);
  b->tverts = Q_realloc(b->tverts, sizeof(vec3_t) * b->num_verts);
  b->sverts = Q_realloc(b->sverts, sizeof(svec_t) * b->num_verts);

  b->verts[v3].x = (b->verts[v1].x + b->verts[v2].x) / 2;
  b->verts[v3].y = (b->verts[v1].y + b->verts[v2].y) / 2;
  b->verts[v3].z = (b->verts[v1].z + b->verts[v2].z) / 2;

  e2 = b->num_edges++;

  b->edges = Q_realloc(b->edges, sizeof(edge_t) * b->num_edges);

  if (b->edges[e1].startvertex == v1)
  {
    b->edges[e1].endvertex = v3;

    b->edges[e2].startvertex = v3;
    b->edges[e2].endvertex = v2;
  }
  else
  {
    b->edges[e1].endvertex = v3;

    b->edges[e2].startvertex = v3;
    b->edges[e2].endvertex = v1;
  }

  for (i = 0; i < b->num_planes; i++)
  {
    f = &b->plane[i];
    for (j = 0; j < f->num_verts - 1; j++)
    {
      if ((f->verts[j] == v1 && f->verts[j + 1] == v2) ||
          (f->verts[j] == v2 && f->verts[j + 1] == v1))
      {
        f->num_verts++;
        f->verts = Q_realloc(f->verts, f->num_verts * sizeof(int));

        for (k = f->num_verts - 1; k > j + 1; k--)
          f->verts[k] = f->verts[k - 1];
        f->verts[j + 1] = v3;
      }
    }
    if ((f->verts[0] == v1 && f->verts[f->num_verts - 1] == v2) ||
        (f->verts[0] == v2 && f->verts[f->num_verts - 1] == v1))
    {
      f->num_verts++;
      f->verts = Q_realloc(f->verts, f->num_verts * sizeof(int));
      f->verts[f->num_verts - 1] = v3;
    }
  }

  ClearSelVerts();
  RecalcAllNormals();
  CalcBrushCenter(b);
  UpdateAllViewports();

  return v3;
}

void
SplitLineFace(void)
{
  int v1, v2;

  if (M.display.num_fselected != 1)
  {
    HandleError("SplitLineFace", "Exactly one face must be selected");
    return;
  }

  v1 = SplitLine();
  if (v1 == -1)
    return;

  NewMessage("Select the other edge.");
  do
  {
    CheckMoveKeys();
    UpdateMap();
    UpdateMouse();
    //		ClearKeys();
  } while (mouse.button != 2);

  while (mouse.button)
    UpdateMouse();

  v2 = SplitLine();
  if (v2 == -1)
    return;

  DoSplit(v1, v2);

  ClearSelVerts();
  RecalcAllNormals();
  CalcBrushCenter(M.display.fsel->Brush);
  UpdateAllViewports();
}
