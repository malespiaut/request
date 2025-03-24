#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "edcurve.h"

#include "brush.h"
#include "camera.h"
#include "error.h"
#include "map.h"
#include "memory.h"
#include "newgroup.h"
#include "quest.h"
#include "qui.h"
#include "texdef.h"
#include "undo.h"

void
Curve_NewBrush(void)
{
  brush_t* b;
  plane_t* p;
  int i;
  int dx, dy, dz;
  int x, y;

  int sizex, sizey, nx, ny;

  {
    int i, j;
    char** keys;
    char** values;

    keys = (char**)Q_malloc(sizeof(char*) * 4);
    values = (char**)Q_malloc(sizeof(char*) * 4);

    if (!keys || !values)
    {
      HandleError("Curve_New", "Out of memory!");
      return;
    }

    for (i = 0; i < 4; i++)
    {
      keys[i] = (char*)Q_malloc(sizeof(char) * 30);
      values[i] = (char*)Q_malloc(sizeof(char) * 10);
      if (!keys[i] || !values[i])
      {
        HandleError("Curve_New", "Out of memory!");
        return;
      }
    }

    strcpy(keys[0], "X size per patch");
    strcpy(keys[1], "Y size per patch");
    strcpy(keys[2], "Patches in X axis");
    strcpy(keys[3], "Patches in Y axis");
    sprintf(values[0], "64");
    sprintf(values[1], "64");
    sprintf(values[2], "1");
    sprintf(values[3], "1");

    j = QUI_PopEntity("Create Curve", keys, values, 4);

    sizex = atoi(values[0]);
    sizey = atoi(values[1]);
    nx = atoi(values[2]);
    ny = atoi(values[3]);

    for (i = 0; i < 4; i++)
    {
      Q_free(keys[i]);
      Q_free(values[i]);
    }
    Q_free(keys);
    Q_free(values);

    if (!j)
      return;
  }

  b = B_New(BR_Q3_CURVE);
  if (!b)
  {
    HandleError("Curve_NewBrush", "Out of memory!");
    return;
  }

  b->x.q3c.sizex = nx * 2 + 1;
  b->x.q3c.sizey = ny * 2 + 1;

  b->num_planes = nx * ny;
  b->num_verts = (nx * 2 + 1) * (ny * 2 + 1);
  b->num_edges = 0;

  b->plane = p = Q_malloc(sizeof(plane_t) * b->num_planes);
  memset(p, 0, sizeof(plane_t) * b->num_planes);

  b->verts = Q_malloc(sizeof(vec3_t) * b->num_verts);
  b->tverts = Q_malloc(sizeof(vec3_t) * b->num_verts);
  b->sverts = Q_malloc(sizeof(vec3_t) * b->num_verts);

  b->edges = NULL;

  b->x.q3c.s = Q_malloc(sizeof(float) * b->num_verts);
  b->x.q3c.t = Q_malloc(sizeof(float) * b->num_verts);

  if (!p || !b->verts || !b->tverts || !b->sverts || !b->x.q3c.s || !b->x.q3c.t)
  {
    Q_free(p);
    Q_free(b->verts);
    Q_free(b->sverts);
    Q_free(b->tverts);
    Q_free(b->x.q3c.s);
    Q_free(b->x.q3c.t);
    Q_free(b);
    HandleError("Curve_NewBrush", "Out of memory!");
    return;
  }

  InitTexdef(&b->tex);

  for (i = 0, y = 0; y < ny * 2 + 1; y++)
  {
    for (x = 0; x < nx * 2 + 1; x++, i++)
    {
      b->verts[i].x = SnapPointToGrid(M.display.vport[M.display.active_vport].camera_pos.x);
      b->verts[i].y = SnapPointToGrid(M.display.vport[M.display.active_vport].camera_pos.y);
      b->verts[i].z = SnapPointToGrid(M.display.vport[M.display.active_vport].camera_pos.z);

      Move90(M.display.active_vport, MOVE_RIGHT, &dx, &dy, &dz, sizex / 2 * (x - nx));
      b->verts[i].x += dx;
      b->verts[i].y += dy;
      b->verts[i].z += dz;

      Move90(M.display.active_vport, MOVE_DOWN, &dx, &dy, &dz, sizey / 2 * (y - ny));
      b->verts[i].x += dx;
      b->verts[i].y += dy;
      b->verts[i].z += dz;

      Move90(M.display.active_vport, MOVE_FORWARD, &dx, &dy, &dz, 128);
      b->verts[i].x += dx;
      b->verts[i].y += dy;
      b->verts[i].z += dz;

      b->x.q3c.s[i] = x * 0.5;
      b->x.q3c.t[i] = y * 0.5;
    }
  }

  for (y = 0, p = b->plane; y < ny; y++)
  {
    for (x = 0; x < nx; x++, p++)
    {
      p->num_verts = 9;
      p->verts = Q_malloc(sizeof(int) * p->num_verts);
      if (!p->verts)
      {
        Q_free(b->plane);
        Q_free(b->verts);
        Q_free(b->sverts);
        Q_free(b->tverts);
        Q_free(b->edges);
        Q_free(b);
        HandleError("Curve_NewBrush", "Out of memory!");
        return;
      }

      p->verts[0] = y * 2 * (nx * 2 + 1) + x * 2 + 0;
      p->verts[1] = y * 2 * (nx * 2 + 1) + x * 2 + 1;
      p->verts[2] = y * 2 * (nx * 2 + 1) + x * 2 + 2;
      p->verts[3] = (y * 2 + 1) * (nx * 2 + 1) + x * 2 + 0;
      p->verts[4] = (y * 2 + 1) * (nx * 2 + 1) + x * 2 + 1;
      p->verts[5] = (y * 2 + 1) * (nx * 2 + 1) + x * 2 + 2;
      p->verts[6] = (y * 2 + 2) * (nx * 2 + 1) + x * 2 + 0;
      p->verts[7] = (y * 2 + 2) * (nx * 2 + 1) + x * 2 + 1;
      p->verts[8] = (y * 2 + 2) * (nx * 2 + 1) + x * 2 + 2;
    }
  }

  b->EntityRef = M.WorldSpawn;
  b->Group = FindVisGroup(M.WorldGroup);

  B_Link(b);

  SUndo(UNDO_NONE, UNDO_NONE);
  AddDBrush(b);
}
