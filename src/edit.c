/*
edit.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "edit.h"

#include "3d.h"
#include "brush.h"
#include "display.h"
#include "error.h"
#include "geom.h"
#include "memory.h"
#include "mouse.h"
#include "quest.h"
#include "video.h"

void
SelectWindow(void)
{
  int base_x, base_y;

  base_x = mouse.x;
  base_y = mouse.y;

  DrawSelWindow(base_x, base_y, mouse.x, mouse.y, TRUE);
  while (mouse.button == 1)
  {
    GetMousePos();
    /* Constrain mouse to just viewport while window selecting */
    if (mouse.x >= M.display.vport[M.display.active_vport].xmax)
    {
      mouse.x = M.display.vport[M.display.active_vport].xmax - 1;
      SetMousePos(mouse.x, mouse.y);
    }
    if (mouse.x <= M.display.vport[M.display.active_vport].xmin)
    {
      mouse.x = M.display.vport[M.display.active_vport].xmin + 1;
      SetMousePos(mouse.x, mouse.y);
    }
    if (mouse.y >= M.display.vport[M.display.active_vport].ymax)
    {
      mouse.y = M.display.vport[M.display.active_vport].ymax - 1;
      SetMousePos(mouse.x, mouse.y);
    }
    if (mouse.y <= M.display.vport[M.display.active_vport].ymin)
    {
      mouse.y = M.display.vport[M.display.active_vport].ymin + 1;
      SetMousePos(mouse.x, mouse.y);
    }
    if (mouse.moved)
    {
      WaitRetr();
      UndrawMouse(mouse.prev_x, mouse.prev_y);
      DrawSelWindow(base_x, base_y, mouse.prev_x, mouse.prev_y, FALSE);
      DrawSelWindow(base_x, base_y, mouse.x, mouse.y, TRUE);
      DrawMouse(mouse.x, mouse.y);
    }
  }
  DrawSelWindow(base_x, base_y, mouse.prev_x, mouse.prev_y, FALSE);
}

void
ForEachSelTexdef(void (*func)(texdef_t* p))
{
  brushref_t* b;
  fsel_t* f;
  int i;

  for (b = M.display.bsel; b; b = b->Next)
  {
    if (b->Brush->bt->flags & BR_F_BTEXDEF)
      func(&b->Brush->tex);
    else
      for (i = 0; i < b->Brush->num_planes; i++)
        func(&b->Brush->plane[i].tex);
  }
  for (f = M.display.fsel; f; f = f->Next)
  {
    if (f->Brush->bt->flags & BR_F_BTEXDEF)
      func(&f->Brush->tex);
    else
      func(&f->Brush->plane[f->facenum].tex);
  }
}

/*
Selecting stuff in solid polygon mode
*/

/*
Trace along where mouse cursor is pointing, call func for all
planes and curves hit with b set to brush and face set to
plane number or -1 for curves.
*/
static void
TraceSelect(void (*func)(brush_t* b, int face, float dist))
{
  matrix_t m;
  vec3_t s, e;
  vec3_t d;
  vec3_t a;
  viewport_t* vp;

  brush_t* b;
  int i, j;

  float f, g, h;

  GenerateIMatrix(m, M.display.active_vport);
  vp = &M.display.vport[M.display.active_vport];

  s.x = (mouse.x - vp->xmin) * 2.0 / (vp->xmax - vp->xmin) - 1;
  s.y = -(mouse.y - vp->ymin) * 2.0 / (vp->ymax - vp->ymin) + 1;
  s.z = 1;

  d.x = s.x * m[0][0] +
        s.y * m[0][1] +
        s.z * m[0][2];
  d.y = s.x * m[1][0] +
        s.y * m[1][1] +
        s.z * m[1][2];
  d.z = s.x * m[2][0] +
        s.y * m[2][1] +
        s.z * m[2][2];

  Normalize(&d);

  s.x = vp->camera_pos.x;
  s.y = vp->camera_pos.y;
  s.z = vp->camera_pos.z;

  e.x = s.x + d.x * 1024;
  e.y = s.y + d.y * 1024;
  e.z = s.z + d.z * 1024;

  /* We trace from s to e, direction is d */
  for (b = M.BrushHead; b; b = b->Next)
  {
    if (!b->drawn)
      continue;

    /* TODO: handle curves */
    if (b->bt->flags & BR_F_EDGES)
    {
      for (i = 0; i < b->num_planes; i++)
      {
        f = DotProd(s, b->plane[i].normal) - b->plane[i].dist;
        g = DotProd(e, b->plane[i].normal) - b->plane[i].dist;

        if ((f < 0 && g < 0) || (f > 0 && g > 0))
          continue;

        g = DotProd(d, b->plane[i].normal);
        if (fabs(g) < 0.0001)
          continue;

        h = -f / g;

        a.x = s.x + d.x * h;
        a.y = s.y + d.y * h;
        a.z = s.z + d.z * h;

        /* now make sure a is actually in the face by checking it
         against the planes of the other faces in the brush */
        for (j = 0; j < b->num_planes; j++)
        {
          if (i == j)
            continue;

          f = DotProd(a, b->plane[j].normal) - b->plane[j].dist;
          if (f > 0)
            break;
        }

        if (j != b->num_planes)
          continue;

        func(b, i, h);
      }
    }
  }
}

static float* brf_dist;
static brush_t** br_list;
static int br_num;

static void
Solid_SelectBrush(brush_t* b, int face, float dist)
{
  int i;
  brush_t** br;
  float* f;

  for (i = 0, br = br_list; i < br_num; i++, br++)
    if (*br == b)
    {
      if (dist < brf_dist[i])
        brf_dist[i] = dist;
      return;
    }

  br = Q_realloc(br_list, sizeof(brush_t*) * (br_num + 1));
  f = Q_realloc(brf_dist, sizeof(float) * (br_num + 1));
  if (!br || !f)
  {
    HandleError("Solid_SelectBrush", "Out of memory!");
    return;
  }
  br_list = br;
  brf_dist = f;

  br[br_num] = b;
  f[br_num] = dist;
  br_num++;
}

void
Solid_SelectBrushes(brush_t*** list, int* num)
{
  br_list = NULL;
  brf_dist = NULL;
  br_num = 0;

  TraceSelect(Solid_SelectBrush);

  {
    int i, j;
    float f;
    brush_t* b;

    for (j = 1; j;)
    {
      j = 0;
      for (i = 0; i < br_num - 1; i++)
      {
        if (brf_dist[i] > brf_dist[i + 1])
        {
          j++;

          f = brf_dist[i + 1];
          brf_dist[i + 1] = brf_dist[i];
          brf_dist[i] = f;

          b = br_list[i + 1];
          br_list[i + 1] = br_list[i];
          br_list[i] = b;
        }
      }
    }
  }

  Q_free(brf_dist);

  *list = br_list;
  *num = br_num;
}

static fsel_t* f_list;
static int f_num;

static void
Solid_SelectFace(brush_t* b, int face, float dist)
{
  fsel_t* f;
  float* fl;

  if (face == -1)
    return;

  f = Q_realloc(f_list, sizeof(fsel_t) * (f_num + 1));
  fl = Q_realloc(brf_dist, sizeof(float) * (f_num + 1));
  if (!f || !fl)
  {
    HandleError("Solid_SelectFace", "Out of memory!");
    return;
  }
  f_list = f;
  brf_dist = fl;

  f[f_num].Brush = b;
  f[f_num].facenum = face;
  fl[f_num] = dist;
  f_num++;
}

void
Solid_SelectFaces(fsel_t** list, int* num)
{
  f_num = 0;
  f_list = NULL;
  brf_dist = NULL;

  TraceSelect(Solid_SelectFace);

  {
    int i, j;
    float f;
    fsel_t fs;

    for (j = 1; j;)
    {
      j = 0;
      for (i = 0; i < f_num - 1; i++)
      {
        if (brf_dist[i] > brf_dist[i + 1])
        {
          j++;

          f = brf_dist[i + 1];
          brf_dist[i + 1] = brf_dist[i];
          brf_dist[i] = f;

          fs = f_list[i + 1];
          f_list[i + 1] = f_list[i];
          f_list[i] = fs;
        }
      }
    }
  }

  Q_free(brf_dist);

  *list = f_list;
  *num = f_num;
}
