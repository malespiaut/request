/*
clickbr.c file of the Quest Source Code

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

#include "clickbr.h"

#include "3d.h"
#include "brush.h"
#include "clip.h"
#include "display.h"
#include "edbrush.h"
#include "edvert.h"
#include "error.h"
#include "geom.h"
#include "keyboard.h"
#include "keyhit.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "newgroup.h"
#include "quest.h"
#include "status.h"
#include "tex.h"
#include "texdef.h"
#include "undo.h"

static vec3_t normal;

static brush_t*
TryBuildBrush(int flip, int z0, int z1)
{
  brush_my_t t;
  brush_t* b;

  int i;

  vec3_t v1, v2, v3;
  vec3_t n;

  t.numfaces = 2 + M.npts;
  t.faces = Q_malloc(sizeof(face_my_t) * t.numfaces);
  if (!t.faces)
  {
    HandleError("ClickBrush", "Out of memory!");
    return NULL;
  }

  for (i = 0; i < t.numfaces; i++)
  {
    t.faces[i].misc = t.faces[i].numpts = 0;
    InitTexdef(&t.faces[i].tex);
  }

  for (i = 0; i < M.npts; i++)
  {
    v1 = M.pts[i];
    if (i == M.npts - 1)
      v2 = M.pts[0];
    else
      v2 = M.pts[i + 1];

    if (flip)
    {
      v3 = v1;
      v1 = v2;
      v2 = v3;
    }

    v1.x -= v2.x;
    v1.y -= v2.y;
    v1.z -= v2.z;

    CrossProd(v1, normal, &n);
    Normalize(&n);

    t.faces[i].normal = n;
    t.faces[i].dist = DotProd(n, v2);
  }

  t.faces[i].normal = normal;
  t.faces[i].dist = z1;
  i++;
  t.faces[i].normal.x = -normal.x;
  t.faces[i].normal.y = -normal.y;
  t.faces[i].normal.z = -normal.z;
  t.faces[i].dist = -z0;

  b = B_New(BR_NORMAL);
  if (!b)
  {
    Q_free(t.faces);
    HandleError("ClickBrush", "Out of memory!");
    return NULL;
  }

  if (!BuildBrush(&t, b))
  {
    B_Free(b);
    return NULL;
  }

  b->EntityRef = M.WorldSpawn;
  b->Group = FindVisGroup(M.WorldGroup);

  B_Link(b);

  return b;
}

#if 0
static void GetRealPos(int mx,int my,vec3_t *v)
{
   viewport_t *vp;
   int dx,dy;

   int rx,ry,rz;
   matrix_t rot,trans,m;

   float tx,ty,tz;

   vp=&M.display.vport[M.display.active_vport];

	InitMatrix(trans);
	trans[0][3] = vp->camera_pos.x;
	trans[1][3] = vp->camera_pos.y;
	trans[2][3] = vp->camera_pos.z;

   GetRotValues(M.display.active_vport,&rx,&ry,&rz);
   GenerateIRotMatrix(rot,-rx,-ry,-rz);

	/* Multiply Matricies into one tranformation matrix */
	MultMatrix(trans,rot,m);

   dx=(vp->xmax-vp->xmin) >>1;
   dy=(vp->ymax-vp->ymin) >>1;

   tx= (mx-dx-vp->xmin)/vp->zoom_amt;
   ty=-(my-dy-vp->ymin)/vp->zoom_amt;
   tz=1;

   v->x=m[0][0]*tx+
        m[0][1]*ty+
        m[0][2]*tz+
        m[0][3];
   v->y=m[1][0]*tx+
        m[1][1]*ty+
        m[1][2]*tz+
        m[1][3];
   v->z=m[2][0]*tx+
        m[2][1]*ty+
        m[2][2]*tz+
        m[2][3];

   v->x=SnapPointToGrid(v->x);
   v->y=SnapPointToGrid(v->y);
   v->z=SnapPointToGrid(v->z);
}
#endif

static void
GetRealPos(int mx, int my, vec3_t* v)
{
  viewport_t* vp;
  int dx, dy;

  matrix_t m;

  float tx, ty, tz;

  vp = &M.display.vport[M.display.active_vport];

  GenerateIMatrix(m, M.display.active_vport);

  dx = (vp->xmax - vp->xmin) >> 1;
  dy = (vp->ymax - vp->ymin) >> 1;

  tx = (mx - dx - vp->xmin) / vp->zoom_amt;
  ty = -(my - dy - vp->ymin) / vp->zoom_amt;
  tz = 1;

  v->x = m[0][0] * tx +
         m[0][1] * ty +
         m[0][2] * tz +
         m[0][3];
  v->y = m[1][0] * tx +
         m[1][1] * ty +
         m[1][2] * tz +
         m[1][3];
  v->z = m[2][0] * tx +
         m[2][1] * ty +
         m[2][2] * tz +
         m[2][3];

  v->x = SnapPointToGrid(v->x);
  v->y = SnapPointToGrid(v->y);
  v->z = SnapPointToGrid(v->z);
}

static void
SetPts(vec3_t v)
{
  M.pts[M.npts - 1] = v;
}

static void
AddPts(vec3_t v)
{
  vec3_t *pts, *tpts;

  pts = Q_realloc(M.pts, sizeof(vec3_t) * (M.npts + 1));
  tpts = Q_realloc(M.tpts, sizeof(vec3_t) * (M.npts + 1));
  if (!pts || !tpts)
  {
    HandleError("ClickBrush", "Out of memory!");
    return;
  }
  M.pts = pts;
  M.tpts = tpts;
  M.pts[M.npts] = v;
  M.npts++;
}

void
ClickBrush(void)
{
  viewport_t* vp;

  vec3_t v, ov;

  vec3_t *opts, *otpts;
  int onpts;
  int odraw;

  int done;

  int z0, z1;

  brush_t* b;

  brushref_t* bs;
  int nbs;

  vp = &M.display.vport[M.display.active_vport];
  if (vp->mode != NOPERSP)
  {
    HandleError("ClickBrush", "Must click in a 2d viewport!");
    return;
  }

  if (M.display.num_bselected)
    b = M.display.bsel->Brush;
  else
    b = NULL;

  ClearSelVerts();
  nbs = M.display.num_bselected;
  bs = M.display.bsel;
  M.display.bsel = NULL;
  M.display.num_bselected = 0;

  opts = M.pts;
  otpts = M.tpts;
  onpts = M.npts;
  odraw = status.draw_pts;

  M.pts = M.tpts = NULL;
  M.npts = 0;

  status.draw_pts = 2;

  GetRealPos(mouse.x, mouse.y, &v);
  AddPts(v);
  AddPts(v);

  ov = v;

  UpdateViewport(M.display.active_vport, 1);
  while (mouse.button)
    UpdateMouse();

  done = 0;

  do
  {
    CheckMoveKeys();
    UpdateMouse();
    //		ClearKeys();

    if (!InBox(vp->xmin, vp->ymin, vp->xmax, vp->ymax))
    {
      UndrawMouse(mouse.x, mouse.y);
      if (mouse.x >= vp->xmax)
      {
        mouse.x = vp->xmax - 1;
        SetMousePos(mouse.x, mouse.y);
      }
      if (mouse.x <= vp->xmin)
      {
        mouse.x = vp->xmin + 1;
        SetMousePos(mouse.x, mouse.y);
      }
      if (mouse.y >= vp->ymax)
      {
        mouse.y = vp->ymax - 1;
        SetMousePos(mouse.x, mouse.y);
      }
      if (mouse.y <= vp->ymin)
      {
        mouse.y = vp->ymin + 1;
        SetMousePos(mouse.x, mouse.y);
      }
      DrawMouse(mouse.x, mouse.y);
    }

    GetRealPos(mouse.x, mouse.y, &v);

    if ((v.x != ov.x) || (v.y != ov.y) || (v.z != ov.z))
    {
      SetPts(v);
      ov = v;
      UpdateViewport(M.display.active_vport, 1);
      DrawMouse(mouse.x, mouse.y);
    }

    if (mouse.button == 1)
    {
      vec3_t* p;

      if (TestKey(KEY_LF_SHIFT) || TestKey(KEY_RT_SHIFT))
      {
        int i;

        M.pts[M.npts - 1] = M.pts[M.npts - 2];

        for (p = M.pts, i = 0; i < M.npts - 1; i++, p++)
          if ((v.x == p->x) && (v.y == p->y) && (v.z == p->z))
            break;

        if (i < M.npts - 1)
        {
          while (mouse.button & 1)
          {
            if (!InBox(vp->xmin, vp->ymin, vp->xmax, vp->ymax))
            {
              UndrawMouse(mouse.x, mouse.y);
              if (mouse.x >= vp->xmax)
              {
                mouse.x = vp->xmax - 1;
                SetMousePos(mouse.x, mouse.y);
              }
              if (mouse.x <= vp->xmin)
              {
                mouse.x = vp->xmin + 1;
                SetMousePos(mouse.x, mouse.y);
              }
              if (mouse.y >= vp->ymax)
              {
                mouse.y = vp->ymax - 1;
                SetMousePos(mouse.x, mouse.y);
              }
              if (mouse.y <= vp->ymin)
              {
                mouse.y = vp->ymin + 1;
                SetMousePos(mouse.x, mouse.y);
              }
              DrawMouse(mouse.x, mouse.y);
            }

            GetRealPos(mouse.x, mouse.y, &v);

            *p = v;

            UpdateViewport(M.display.active_vport, 1);
            DrawMouse(mouse.x, mouse.y);

            do
            {
              UpdateMouse();
            } while (!mouse.moved && (mouse.button & 1));
          }
        }
      }
      else
      {
        p = &M.pts[M.npts - 2];
        if ((v.x == p->x) && (v.y == p->y) && (v.z == p->z))
        {
          M.npts--; // the reallocs should handle this OK
          if (M.npts == 1)
            done = 2;
        }
        else if ((v.x == M.pts->x) && (v.y == M.pts->y) && (v.z == M.pts->z))
        {
          done = 1;
        }
        else
        {
          AddPts(v);
        }

        UpdateViewport(M.display.active_vport, 1);
        DrawMouse(mouse.x, mouse.y);

        while (mouse.button)
          UpdateMouse();
      }
    }
    if (mouse.button & 2)
    {
      done = 1;
      while (mouse.button & 2)
        UpdateMouse();
    }
  } while (!done);

  M.npts--;

  M.display.num_bselected = nbs;
  M.display.bsel = bs;

  if (done == 1)
  {
    int i;
    vec3_t v1, v2, v3;
    vec3_t t1, t2;

    for (i = 0; i < M.npts - 1; i++)
    {
      v1 = M.pts[i];
      v2 = M.pts[i + 1];
      if (i == M.npts - 2)
        v3 = M.pts[i - M.npts + 2];
      else
        v3 = M.pts[i + 2];

      t1.x = v1.x - v2.x;
      t1.y = v1.y - v2.y;
      t1.z = v1.z - v2.z;

      t2.x = v3.x - v2.x;
      t2.y = v3.y - v2.y;
      t2.z = v3.z - v2.z;

      CrossProd(t1, t2, &normal);
      Normalize(&normal);

      if ((fabs(normal.x) > 0.01) ||
          (fabs(normal.y) > 0.01) ||
          (fabs(normal.z) > 0.01))
        break;
    }

    if ((normal.x < -0.01) ||
        (normal.y < -0.01) ||
        (normal.z < -0.01))
    {
      normal.x = -normal.x;
      normal.y = -normal.y;
      normal.z = -normal.z;
    }

    if (b)
    {
      float t;

      z0 = z1 = DotProd(normal, b->verts[0]);
      for (i = 1; i < b->num_verts; i++)
      {
        t = DotProd(normal, b->verts[i]);
        if (t > z1)
          z1 = t;
        if (t < z0)
          z0 = t;
      }
    }
    else
    {
      z0 = normal.x * vp->camera_pos.x +
           normal.y * vp->camera_pos.y +
           normal.z * vp->camera_pos.z;
      z0 = SnapPointToGrid(z0);
      z1 = z0 + 64;
    }

    // create the brush
    b = TryBuildBrush(0, z0, z1);
    if (!b)
    {
      b = TryBuildBrush(1, z0, z1);
      if (!b)
        HandleError("ClickBrush", "Error building brush!");
    }

    if (b)
    {
      ClearSelBrushes();
      SUndo(UNDO_NONE, UNDO_NONE);
      AddDBrush(b);

      AddSelBrush(b, 0);
    }
  }

  Q_free(M.pts);
  Q_free(M.tpts);

  M.pts = opts;
  M.tpts = otpts;
  M.npts = onpts;
  status.draw_pts = odraw;

  UpdateAllViewports();
}

void
CheckDistance(void)
{
  viewport_t* vp;

  vec3_t *opts, *otpts;
  int onpts;
  int odraw;

  vec3_t v, ov;

  vp = &M.display.vport[M.display.active_vport];
  if (vp->mode != NOPERSP)
  {
    HandleError("CheckDistance", "Must be in a 2d viewport!");
    return;
  }

  opts = M.pts;
  otpts = M.tpts;
  onpts = M.npts;
  odraw = status.draw_pts;

  M.pts = Q_malloc(sizeof(vec3_t) * 2);
  M.tpts = Q_malloc(sizeof(vec3_t) * 2);
  if (!M.pts || !M.tpts)
  {
    Q_free(M.pts);
    Q_free(M.tpts);

    M.pts = opts;
    M.tpts = otpts;
    M.npts = onpts;
    status.draw_pts = odraw;
  }
  M.npts = 2;

  GetRealPos(mouse.x, mouse.y, &v);
  M.pts[1] = M.pts[0] = v;

  ov = v;

  status.draw_pts = 3;

  UpdateViewport(M.display.active_vport, 1);

  do
  {
    CheckMoveKeys();
    UpdateMouse();

    if (!InBox(vp->xmin, vp->ymin, vp->xmax, vp->ymax))
    {
      UndrawMouse(mouse.x, mouse.y);
      if (mouse.x >= vp->xmax)
      {
        mouse.x = vp->xmax - 1;
        SetMousePos(mouse.x, mouse.y);
      }
      if (mouse.x <= vp->xmin)
      {
        mouse.x = vp->xmin + 1;
        SetMousePos(mouse.x, mouse.y);
      }
      if (mouse.y >= vp->ymax)
      {
        mouse.y = vp->ymax - 1;
        SetMousePos(mouse.x, mouse.y);
      }
      if (mouse.y <= vp->ymin)
      {
        mouse.y = vp->ymin + 1;
        SetMousePos(mouse.x, mouse.y);
      }
      DrawMouse(mouse.x, mouse.y);
    }

    GetRealPos(mouse.x, mouse.y, &v);

    if ((v.x != ov.x) || (v.y != ov.y) || (v.z != ov.z))
    {
      SetPts(v);
      ov = v;
      UpdateViewport(M.display.active_vport, 1);
      DrawMouse(mouse.x, mouse.y);
    }

    if (mouse.button)
      break;
  } while (1);

  Q_free(M.pts);
  Q_free(M.tpts);

  M.pts = opts;
  M.tpts = otpts;
  M.npts = onpts;
  status.draw_pts = odraw;

  UpdateAllViewports();
}
