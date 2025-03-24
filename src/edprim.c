#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "edprim.h"

#include "brush.h"
#include "camera.h"
#include "check.h"
#include "edent.h"
#include "error.h"
#include "map.h"
#include "memory.h"
#include "newgroup.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "tex.h"
#include "texdef.h"
#include "undo.h"

#define GR ((sqrt(5) + 1) / 2)
#define GA (3 * PI / 10)
#define GR_2 (GR / 2)

/*
Used below to add edges using tables.
*/
#define ADDEDGES(num)                                 \
  edge_t* e;                                          \
  int* j;                                             \
  for (i = num, e = b->edges, j = edges; i; i--, e++) \
  {                                                   \
    e->startvertex = *j++;                            \
    e->endvertex = *j++;                              \
  }

static brush_t*
AllocBrush(int nverts, int nedges, int nplanes)
{
  brush_t* b;

  b = B_New(BR_NORMAL);
  if (!b)
  {
    HandleError("Add Brush", "Out of memory!");
    return NULL;
  }

  b->num_verts = nverts;
  b->num_edges = nedges;
  b->num_planes = nplanes;

  b->verts = (vec3_t*)Q_malloc(sizeof(vec3_t) * b->num_verts);
  b->tverts = (vec3_t*)Q_malloc(sizeof(vec3_t) * b->num_verts);
  b->sverts = (svec_t*)Q_malloc(sizeof(svec_t) * b->num_verts);
  if (!b->verts || !b->tverts || !b->sverts)
  {
    HandleError("Add Brush", "Out of memory!");
    return NULL;
  }

  b->edges = (edge_t*)Q_malloc(sizeof(edge_t) * b->num_edges);
  if (!b->edges)
  {
    HandleError("Add Brush", "Out of memory!");
    return NULL;
  }

  b->plane = (plane_t*)Q_malloc(sizeof(plane_t) * b->num_planes);
  if (!b->plane)
  {
    HandleError("Add Brush", "Out of memory!");
    return NULL;
  }

  return b;
}

static int
AddBrush_Cube(int x, int y, int z, int xsize, int ysize, int zsize)
{
  brush_t* b;
  plane_t* p;
  int nsides, i;

  nsides = 4;

  b = AllocBrush(8, 12, 6);
  if (!b)
    return 0;

  for (i = 0; i < b->num_planes; i++)
  {
    b->plane[i].verts = (int*)Q_malloc(sizeof(int) * nsides);
    b->plane[i].num_verts = nsides;
    if (!b->plane[i].verts)
    {
      HandleError("Add Brush", "Out of memory!");
      return 0;
    }
  }

  xsize /= 2;
  ysize /= 2;
  zsize /= 2;

  b->verts[0].x = x - xsize;
  b->verts[0].y = y - ysize;
  b->verts[0].z = z + zsize;
  b->verts[1].x = x - xsize;
  b->verts[1].y = y + ysize;
  b->verts[1].z = z + zsize;
  b->verts[2].x = x + xsize;
  b->verts[2].y = y + ysize;
  b->verts[2].z = z + zsize;
  b->verts[3].x = x + xsize;
  b->verts[3].y = y - ysize;
  b->verts[3].z = z + zsize;
  b->verts[4].x = x - xsize;
  b->verts[4].y = y - ysize;
  b->verts[4].z = z - zsize;
  b->verts[5].x = x - xsize;
  b->verts[5].y = y + ysize;
  b->verts[5].z = z - zsize;
  b->verts[6].x = x + xsize;
  b->verts[6].y = y + ysize;
  b->verts[6].z = z - zsize;
  b->verts[7].x = x + xsize;
  b->verts[7].y = y - ysize;
  b->verts[7].z = z - zsize;

  {
    static int edges[12 * 2] =
      {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};
    ADDEDGES(12)
  }

#define Cplane(a, i, j, k, l) \
  {                           \
    p->verts[0] = i;          \
    p->verts[1] = j;          \
    p->verts[2] = k;          \
    p->verts[3] = l;          \
    p++;                      \
  }
  p = b->plane;
  Cplane(0, 0, 1, 2, 3);
  Cplane(1, 4, 0, 3, 7);
  Cplane(2, 7, 3, 2, 6);
  Cplane(3, 6, 2, 1, 5);
  Cplane(4, 1, 0, 4, 5);
  Cplane(5, 4, 7, 6, 5);

  for (i = 0; i < b->num_planes; i++)
    InitTexdef(&b->plane[i].tex);

  b->EntityRef = M.WorldSpawn;
  b->Group = FindVisGroup(M.WorldGroup);

  B_Link(b);

  AddDBrush(b);

  return 1;
}

static int
AddBrush_Prism(int x, int y, int z, int nsides, int radius, int height, float eccent)
{
  brush_t* b;
  int i, j;
  float tempd;

  b = AllocBrush(nsides * 2, nsides * 3, 2 + nsides);
  if (!b)
    return 0;

  for (i = 0; i < 2; i++)
  {
    b->plane[i].verts = (int*)Q_malloc(sizeof(int) * nsides);
    b->plane[i].num_verts = nsides;
    if (!b->plane[i].verts)
    {
      HandleError("Add Brush", "Out of memory!");
      return FALSE;
    }
  }

  for (i = 0; i < nsides; i++)
  {
    b->plane[i + 2].verts = (int*)Q_malloc(sizeof(int) * 4);
    b->plane[i + 2].num_verts = 4;
    if (!b->plane[i + 2].verts)
    {
      HandleError("Add Brush", "Out of memory!");
      return FALSE;
    }
  }

  /* Calculate top vertices */
  for (i = 0; i < nsides; i++)
  {
    tempd = -((i * 2 * PI) / nsides);
    tempd = cos(tempd);
    tempd *= ((tempd < 0.01) && (tempd > -0.01)) ? 0 : radius;
    b->verts[i].x = SnapPointToGrid(tempd);

    tempd = -((i * 2 * PI) / nsides);
    tempd = sin(tempd);
    tempd *= ((tempd < 0.01) && (tempd > -0.01)) ? 0 : radius * eccent;
    b->verts[i].y = SnapPointToGrid(tempd);

    b->verts[i].z = (height / 2);
  }

  /* Calculate bottom vertices */
  for (i = 0; i < nsides; i++)
  {
    tempd = ((i * 2 * PI) / nsides);
    tempd = cos(tempd);
    tempd *= ((tempd < 0.01) && (tempd > -0.01)) ? 0 : radius;
    b->verts[i + nsides].x = SnapPointToGrid(tempd);

    tempd = ((i * 2 * PI) / nsides);
    tempd = sin(tempd);
    tempd *= ((tempd < 0.01) && (tempd > -0.01)) ? 0 : radius * eccent;
    b->verts[i + nsides].y = SnapPointToGrid(tempd);

    b->verts[i + nsides].z = -(height / 2);
  }

  /*
     SHOULD PASS THE VERTICES THROUGH A ROTATION MATRIX
     (orient the prism so that its top and bottom faces
     are parallel to the view plane)
   */

  for (i = 0; i < b->num_verts; i++)
  {
    b->verts[i].x += x;
    b->verts[i].y += y;
    b->verts[i].z += z;
  }

  /* Create top edges */
  for (i = 0; i < nsides - 1; i++)
  {
    b->edges[i].startvertex = i;
    b->edges[i].endvertex = i + 1;
  }
  b->edges[nsides - 1].startvertex = nsides - 1;
  b->edges[nsides - 1].endvertex = 0;

  /* Create bottom edges */
  for (i = 0; i < nsides - 1; i++)
  {
    b->edges[nsides + i].startvertex = nsides + i;
    b->edges[nsides + i].endvertex = nsides + i + 1;
  }
  b->edges[nsides * 2 - 1].startvertex = nsides * 2 - 1;
  b->edges[nsides * 2 - 1].endvertex = nsides;

  /* Create side edges */
  b->edges[nsides * 2].startvertex = 0;
  b->edges[nsides * 2].endvertex = nsides;
  for (i = 1; i < nsides; i++)
  {
    b->edges[nsides * 2 + i].startvertex = i;
    b->edges[nsides * 2 + i].endvertex = nsides * 2 - i;
  }

  /* Do the top plane's vertices */
  for (j = 0; j < nsides; j++)
  {
    b->plane[0].verts[j] = j;
  }

  /* Do the bottom plane's vertices */
  for (j = 0; j < nsides; j++)
  {
    b->plane[1].verts[j] = j + nsides;
  }

  /* Do the side plane's vertices */
  b->plane[2].verts[0] = 1;
  b->plane[2].verts[1] = 0;
  b->plane[2].verts[2] = nsides;
  b->plane[2].verts[3] = nsides * 2 - 1;
  for (i = 1; i < nsides - 1; i++)
  {
    b->plane[i + 2].verts[0] = i + 1;
    b->plane[i + 2].verts[1] = i;
    b->plane[i + 2].verts[2] = nsides * 2 - i;
    b->plane[i + 2].verts[3] = nsides * 2 - i - 1;
  }
  b->plane[nsides + 1].verts[0] = 0;
  b->plane[nsides + 1].verts[1] = nsides - 1;
  b->plane[nsides + 1].verts[2] = nsides + 1;
  b->plane[nsides + 1].verts[3] = nsides;

  /* update misc plane info */
  for (i = 0; i < b->num_planes; i++)
  {
    InitTexdef(&b->plane[i].tex);
  }

  b->EntityRef = M.WorldSpawn;
  b->Group = FindVisGroup(M.WorldGroup);

  B_Link(b);

  AddDBrush(b);

  return 1;
}

// Pyramid primitive by Gyro Gearloose.
static int
AddBrush_Pyramid(int x, int y, int z, int nsides, int radius, int height, float eccent)
{
  brush_t* b;
  int i, j;
  float tempd;

  b = AllocBrush(nsides + 1, nsides * 2, nsides + 1);
  if (!b)
    return 0;

  /* Allocate bottom plane */
  b->plane[0].verts = (int*)Q_malloc(sizeof(int) * nsides);
  b->plane[0].num_verts = nsides;
  if (!b->plane[0].verts)
  {
    HandleError("Add Brush", "Out of memory!");
    return FALSE;
  }

  /* Allocate side planes */
  for (i = 0; i < nsides; i++)
  {
    b->plane[i + 1].verts = (int*)Q_malloc(sizeof(int) * 3);
    b->plane[i + 1].num_verts = 3;
    if (!b->plane[i + 1].verts)
    {
      HandleError("Add Brush", "Out of memory!");
      return FALSE;
    }
  }

  /* Create top vertex */
  b->verts[0].x = 0.0;
  b->verts[0].y = 0.0;
  b->verts[0].z = (height / 2);

  /* Create bottom vertices */
  for (i = 0; i < nsides; i++)
  {
    tempd = ((i * 2 * PI) / nsides);
    tempd = cos(tempd);
    tempd *= ((tempd < 0.01) && (tempd > -0.01)) ? 0 : radius;
    b->verts[i + 1].x = SnapPointToGrid(tempd);

    tempd = ((i * 2 * PI) / nsides);
    tempd = sin(tempd);
    tempd *= ((tempd < 0.01) && (tempd > -0.01)) ? 0 : radius * eccent;
    b->verts[i + 1].y = SnapPointToGrid(tempd);

    b->verts[i + 1].z = -(height / 2);
  }

  // Translate vertices of the object over to where the camera is
  for (i = 0; i < nsides + 1; i++)
  {
    b->verts[i].x += x;
    b->verts[i].y += y;
    b->verts[i].z += z;
  }

  /* Create bottom edges (edges 0 to nsides-1) */
  for (i = 0; i < nsides - 1; i++)
  {
    b->edges[i].startvertex = i + 1;
    b->edges[i].endvertex = i + 2;
  }
  b->edges[nsides - 1].startvertex = nsides;
  b->edges[nsides - 1].endvertex = 1;

  /* Create side edges (edges nsides to nsides*2-1) */
  b->edges[nsides].startvertex = 0;
  b->edges[nsides].endvertex = nsides;
  for (i = 1; i < nsides; i++)
  {
    b->edges[nsides + i].startvertex = 0;
    b->edges[nsides + i].endvertex = nsides - i;
  }

  /* Do the bottom plane's vertices */
  for (j = 0; j < nsides; j++)
  {
    b->plane[0].verts[j] = j + 1;
  }

  /* Do the side planes' vertices */
  for (i = 1; i < nsides; i++)
  {
    b->plane[i].verts[0] = i;
    b->plane[i].verts[1] = 0;
    b->plane[i].verts[2] = i + 1;
  }
  b->plane[nsides].verts[0] = nsides;
  b->plane[nsides].verts[1] = 0;
  b->plane[nsides].verts[2] = 1;

  /* update misc plane info */
  for (i = 0; i < b->num_planes; i++)
  {
    InitTexdef(&b->plane[i].tex);
  }

  b->EntityRef = M.WorldSpawn;
  b->Group = FindVisGroup(M.WorldGroup);

  B_Link(b);

  AddDBrush(b);

  return 1;
}

static int
AddBrush_Dodec(int x, int y, int z, int radius, float eccent)
{
  brush_t* b;
  int i;
  float tempd;

  // 20 vertices, 30 edges, 12 (!) faces
  b = AllocBrush(20, 30, 12);
  if (!b)
    return 0;

  for (i = 0; i < b->num_planes; i++)
  {
    b->plane[i].verts = (int*)Q_malloc(sizeof(int) * 5);
    b->plane[i].num_verts = 5;
    if (!b->plane[i].verts)
    {
      HandleError("Add Brush", "Unable to allocate brush plane vertex list");
      return FALSE;
    }
  }

  for (i = 0; i < 5; i++)
  {
    tempd = i * 2 * PI / 5;
    if ((tempd < 0.01) && (tempd > -0.01))
      tempd = 0;

    /* Calculate top vertices */
    b->verts[i].x = radius * cos(tempd);
    b->verts[i].y = eccent * radius * sin(tempd);
    b->verts[i].z = 0;

    /* Calculate second level vertices */
    b->verts[i + 5].x = GR * radius * cos(tempd);
    b->verts[i + 5].y = eccent * GR * radius * sin(tempd);
    b->verts[i + 5].z = radius;

    /* Rotate by ... radians */
    tempd += PI / 5;
    if ((tempd < 0.01) && (tempd > -0.01))
      tempd = 0;

    /* Calculate third level vertices */
    b->verts[i + 10].x = 2 * sin(GA) * radius * cos(tempd);
    b->verts[i + 10].y = 2 * sin(GA) * eccent * radius * sin(tempd);
    b->verts[i + 10].z = radius * GR;

    /* Calculate bottom level vertices */
    b->verts[i + 15].x = radius * cos(tempd);
    b->verts[i + 15].y = eccent * radius * sin(tempd);
    b->verts[i + 15].z = radius * (1 + GR);
  }

  for (i = 0; i < b->num_verts; i++)
  {
    b->verts[i].x += x;
    b->verts[i].y += y;
    b->verts[i].z += z;
    b->verts[i].x = rint(b->verts[i].x * 10000) / 10000;
    b->verts[i].y = rint(b->verts[i].y * 10000) / 10000;
    b->verts[i].z = rint(b->verts[i].z * 10000) / 10000;
  }

  {
    static int edges[30 * 2] =
      {
        0,
        1,
        1,
        2,
        2,
        3,
        3,
        4,
        4,
        0,
        0,
        5,
        1,
        6,
        2,
        7,
        3,
        8,
        4,
        9,
        5,
        10,
        10,
        6,
        6,
        11,
        11,
        7,
        7,
        12,
        12,
        8,
        8,
        13,
        13,
        9,
        9,
        14,
        14,
        5,
        10,
        15,
        11,
        16,
        12,
        17,
        13,
        18,
        14,
        19,
        15,
        16,
        16,
        17,
        17,
        18,
        18,
        19,
        19,
        15,
      };
    ADDEDGES(30)
  }

  /*
     The vertices _must_ form each plane in the order given,
     otherwise the normal will have the wrong sign, and the
     object will fail the convex test.
  */

#define Dplane(a, x, y)       \
  {                           \
    b->plane[a].verts[x] = y; \
  }

  Dplane(0, 0, 0);
  Dplane(0, 1, 1);
  Dplane(0, 2, 2);
  Dplane(0, 3, 3);
  Dplane(0, 4, 4);

  Dplane(1, 0, 0);
  Dplane(1, 1, 5);
  Dplane(1, 2, 10);
  Dplane(1, 3, 6);
  Dplane(1, 4, 1);

  Dplane(2, 0, 1);
  Dplane(2, 1, 6);
  Dplane(2, 2, 11);
  Dplane(2, 3, 7);
  Dplane(2, 4, 2);

  Dplane(3, 0, 2);
  Dplane(3, 1, 7);
  Dplane(3, 2, 12);
  Dplane(3, 3, 8);
  Dplane(3, 4, 3);

  Dplane(4, 0, 3);
  Dplane(4, 1, 8);
  Dplane(4, 2, 13);
  Dplane(4, 3, 9);
  Dplane(4, 4, 4);

  Dplane(5, 0, 4);
  Dplane(5, 1, 9);
  Dplane(5, 2, 14);
  Dplane(5, 3, 5);
  Dplane(5, 4, 0);

  Dplane(6, 0, 16);
  Dplane(6, 1, 11);
  Dplane(6, 2, 6);
  Dplane(6, 3, 10);
  Dplane(6, 4, 15);

  Dplane(7, 0, 17);
  Dplane(7, 1, 12);
  Dplane(7, 2, 7);
  Dplane(7, 3, 11);
  Dplane(7, 4, 16);

  Dplane(8, 0, 18);
  Dplane(8, 1, 13);
  Dplane(8, 2, 8);
  Dplane(8, 3, 12);
  Dplane(8, 4, 17);

  Dplane(9, 0, 19);
  Dplane(9, 1, 14);
  Dplane(9, 2, 9);
  Dplane(9, 3, 13);
  Dplane(9, 4, 18);

  Dplane(10, 0, 15);
  Dplane(10, 1, 10);
  Dplane(10, 2, 5);
  Dplane(10, 3, 14);
  Dplane(10, 4, 19);

  Dplane(11, 0, 19);
  Dplane(11, 1, 18);
  Dplane(11, 2, 17);
  Dplane(11, 3, 16);
  Dplane(11, 4, 15);

  /* update misc plane info */
  for (i = 0; i < b->num_planes; i++)
  {
    InitTexdef(&b->plane[i].tex);
  }

  b->EntityRef = M.WorldSpawn;
  b->Group = FindVisGroup(M.WorldGroup);

  B_Link(b);

  AddDBrush(b);
  CheckBrush(b, 1);

  return 1;
}

static int
AddBrush_Icos(int x, int y, int z, int radius, float eccent)
{
  brush_t* b;
  int i;
  plane_t* p;

  // 12 vertices, 30 edges, 20  faces
  b = AllocBrush(12, 30, 20);
  if (!b)
    return 0;

  for (i = 0; i < b->num_planes; i++)
  {
    b->plane[i].verts = (int*)Q_malloc(sizeof(int) * 3);
    b->plane[i].num_verts = 3;
    if (!b->plane[i].verts)
    {
      HandleError("Add Brush", "Unable to allocate brush plane vertex list");
      return FALSE;
    }
  }

#define Ivert(a, j, k, l) \
  {                       \
    b->verts[a].x = j;    \
    b->verts[a].y = k;    \
    b->verts[a].z = l;    \
  }
  Ivert(0, 0, 0.5, GR_2);
  Ivert(1, 0, -0.5, GR_2);
  Ivert(2, 0, -0.5, -GR_2);
  Ivert(3, 0, 0.5, -GR_2);

  Ivert(4, GR_2, 0, 0.5);
  Ivert(5, -GR_2, 0, 0.5);
  Ivert(6, -GR_2, 0, -0.5);
  Ivert(7, GR_2, 0, -0.5);

  Ivert(8, 0.5, GR_2, 0);
  Ivert(9, 0.5, -GR_2, 0);
  Ivert(10, -0.5, -GR_2, 0);
  Ivert(11, -0.5, GR_2, 0);

  for (i = 0; i < b->num_verts; i++)
  {
    b->verts[i].x *= radius;
    b->verts[i].y *= eccent * radius;
    b->verts[i].z *= radius;
    b->verts[i].x += x;
    b->verts[i].y += y;
    b->verts[i].z += z;
    b->verts[i].x = SnapPointToGrid(b->verts[i].x);
    b->verts[i].y = SnapPointToGrid(b->verts[i].y);
    b->verts[i].z = SnapPointToGrid(b->verts[i].z);
  }

  {
    static int edges[30 * 2] =
      {
        0,
        1,
        0,
        11,
        0,
        4,
        0,
        5,
        0,
        8,
        1,
        10,
        1,
        4,
        1,
        5,
        1,
        9,
        2,
        10,
        2,
        3,
        2,
        6,
        2,
        7,
        2,
        9,
        3,
        11,
        3,
        6,
        3,
        7,
        3,
        8,
        4,
        7,
        4,
        8,
        4,
        9,
        5,
        10,
        5,
        11,
        5,
        6,
        6,
        10,
        6,
        11,
        7,
        8,
        7,
        9,
        8,
        11,
        9,
        10,
      };
    ADDEDGES(30)
  }

  /*
     The vertices _must_ form each plane in the order given,
     otherwise the normal will have the wrong sign, and the
     object will fail the convex test.
   */

#define Iplane(a, i, j, k) \
  {                        \
    p->verts[0] = i;       \
    p->verts[1] = j;       \
    p->verts[2] = k;       \
    p++;                   \
  }
  p = b->plane;
  Iplane(0, 0, 1, 5);
  Iplane(1, 0, 5, 11);
  Iplane(2, 0, 11, 8);
  Iplane(3, 8, 4, 0);
  Iplane(4, 0, 4, 1);
  Iplane(5, 10, 5, 1);
  Iplane(6, 1, 4, 9);
  Iplane(7, 1, 9, 10);
  Iplane(8, 7, 4, 8);
  Iplane(9, 9, 4, 7);
  Iplane(10, 6, 2, 3);
  Iplane(11, 11, 6, 3);
  Iplane(12, 8, 11, 3);
  Iplane(13, 3, 7, 8);
  Iplane(14, 2, 7, 3);
  Iplane(15, 2, 6, 10);
  Iplane(16, 9, 7, 2);
  Iplane(17, 10, 9, 2);
  Iplane(18, 11, 5, 6);
  Iplane(19, 6, 5, 10);

  /* update misc plane info */
  for (i = 0; i < b->num_planes; i++)
  {
    InitTexdef(&b->plane[i].tex);
  }

  b->EntityRef = M.WorldSpawn;
  b->Group = FindVisGroup(M.WorldGroup);

  B_Link(b);

  AddDBrush(b);
  CheckBrush(b, 1);

  return 1;
}

static int
AddBrush_Bucky(int x, int y, int z, int radius, float eccent)
{
  brush_t* b;
  plane_t* p;
  int hexagons, pentagons;
  int i;

  /*
     60 vertices, 90 edges, 32 faces composed of 20 hexagons, 12 pentagons
  */

  hexagons = 20;
  pentagons = 12;

  b = AllocBrush(60, 90, 32);
  if (!b)
    return 0;

  for (i = 0; i < hexagons; i++)
  {
    b->plane[i].verts = (int*)Q_malloc(sizeof(int) * 6);
    b->plane[i].num_verts = 6;
    if (!b->plane[i].verts)
    {
      HandleError("Add Brush", "Unable to allocate brush plane vertex list");
      return FALSE;
    }
  }

  for (i = hexagons; i < hexagons + pentagons; i++)
  {
    b->plane[i].verts = (int*)Q_malloc(sizeof(int) * 5);
    b->plane[i].num_verts = 5;
    if (!b->plane[i].verts)
    {
      HandleError("Add Brush", "Unable to allocate brush plane vertex list");
      return FALSE;
    }
  }

#define Bvert(a, j, k, l) \
  {                       \
    b->verts[a].x = j;    \
    b->verts[a].y = k;    \
    b->verts[a].z = l;    \
  }

  Bvert(0, 0.343279, 0.000000, 0.939234);
  Bvert(1, 0.106079, 0.326477, 0.939234);
  Bvert(2, -0.277718, 0.201774, 0.939234);
  Bvert(3, -0.277718, -0.201774, 0.939234);
  Bvert(4, 0.106079, -0.326477, 0.939234);
  Bvert(5, 0.686557, 0.000000, 0.727076);
  Bvert(6, 0.792636, 0.326477, 0.514918);
  Bvert(7, 0.555436, 0.652955, 0.514918);
  Bvert(8, 0.212158, 0.652955, 0.727076);
  Bvert(9, -0.065560, 0.854729, 0.514918);
  Bvert(10, -0.449358, 0.730026, 0.514918);
  Bvert(11, -0.555436, 0.403548, 0.727076);
  Bvert(12, -0.833155, 0.201774, 0.514918);
  Bvert(13, -0.833155, -0.201774, 0.514918);
  Bvert(14, -0.555436, -0.403548, 0.727076);
  Bvert(15, -0.449358, -0.730026, 0.514918);
  Bvert(16, -0.065560, -0.854729, 0.514918);
  Bvert(17, 0.212158, -0.652955, 0.727076);
  Bvert(18, 0.555436, -0.652955, 0.514918);
  Bvert(19, 0.792636, -0.326477, 0.514918);
  Bvert(20, 0.964275, -0.201774, 0.171639);
  Bvert(21, 0.964275, 0.201774, 0.171639);
  Bvert(22, 0.898715, 0.403548, -0.171639);
  Bvert(23, 0.661515, 0.730026, -0.171639);
  Bvert(24, 0.489876, 0.854729, 0.171639);
  Bvert(25, 0.106079, 0.979432, 0.171639);
  Bvert(26, -0.106079, 0.979432, -0.171639);
  Bvert(27, -0.489876, 0.854729, -0.171639);
  Bvert(28, -0.661515, 0.730026, 0.171639);
  Bvert(29, -0.898715, 0.403548, 0.171639);
  Bvert(30, -0.964275, 0.201774, -0.171639);
  Bvert(31, -0.964275, -0.201774, -0.171639);
  Bvert(32, -0.898715, -0.403548, 0.171639);
  Bvert(33, -0.661515, -0.730026, 0.171639);
  Bvert(34, -0.489876, -0.854729, -0.171639);
  Bvert(35, -0.106079, -0.979432, -0.171639);
  Bvert(36, 0.106079, -0.979432, 0.171639);
  Bvert(37, 0.489876, -0.854729, 0.171639);
  Bvert(38, 0.661515, -0.730026, -0.171639);
  Bvert(39, 0.898715, -0.403548, -0.171639);
  Bvert(40, 0.833155, -0.201774, -0.514918);
  Bvert(41, 0.833155, 0.201774, -0.514918);
  Bvert(42, 0.555436, 0.403548, -0.727076);
  Bvert(43, 0.449358, 0.730026, -0.514918);
  Bvert(44, 0.065560, 0.854729, -0.514918);
  Bvert(45, -0.212158, 0.652955, -0.727076);
  Bvert(46, -0.555436, 0.652955, -0.514918);
  Bvert(47, -0.792636, 0.326477, -0.514918);
  Bvert(48, -0.686557, 0.000000, -0.727076);
  Bvert(49, -0.792636, -0.326477, -0.514918);
  Bvert(50, -0.555436, -0.652955, -0.514918);
  Bvert(51, -0.212158, -0.652955, -0.727076);
  Bvert(52, 0.065560, -0.854729, -0.514918);
  Bvert(53, 0.449358, -0.730026, -0.514918);
  Bvert(54, 0.555436, -0.403548, -0.727076);
  Bvert(55, 0.277718, -0.201774, -0.939234);
  Bvert(56, 0.277718, 0.201774, -0.939234);
  Bvert(57, -0.106079, 0.326477, -0.939234);
  Bvert(58, -0.343279, 0.000000, -0.939234);
  Bvert(59, -0.106079, -0.326477, -0.939234);

  for (i = 0; i < b->num_verts; i++)
  {
    b->verts[i].x *= radius;
    b->verts[i].y *= eccent * radius;
    b->verts[i].z *= radius;
    b->verts[i].x += x;
    b->verts[i].y += y;
    b->verts[i].z += z;
    b->verts[i].x = rint(b->verts[i].x * 10000) / 10000;
    b->verts[i].y = rint(b->verts[i].y * 10000) / 10000;
    b->verts[i].z = rint(b->verts[i].z * 10000) / 10000;
  }

  {
    static int edges[90 * 2] =
      {
#define Bedge(i, x, y) x, y,
        Bedge(0, 0, 1) Bedge(1, 0, 4) Bedge(2, 0, 5) Bedge(3, 1, 2)
          Bedge(4, 1, 8) Bedge(5, 2, 3) Bedge(6, 2, 11) Bedge(7, 3, 4)
            Bedge(8, 3, 14) Bedge(9, 4, 17) Bedge(10, 5, 6) Bedge(11, 5, 19)
              Bedge(12, 6, 7) Bedge(13, 6, 21) Bedge(14, 7, 8) Bedge(15, 7, 24)
                Bedge(16, 8, 9) Bedge(17, 9, 10) Bedge(18, 9, 25) Bedge(19, 10, 11)
                  Bedge(20, 10, 28) Bedge(21, 11, 12) Bedge(22, 12, 13) Bedge(23, 12, 29)
                    Bedge(24, 13, 14) Bedge(25, 13, 32) Bedge(26, 14, 15) Bedge(27, 15, 16)
                      Bedge(28, 15, 33) Bedge(29, 16, 17) Bedge(30, 16, 36) Bedge(31, 17, 18)
                        Bedge(32, 18, 19) Bedge(33, 18, 37) Bedge(34, 19, 20) Bedge(35, 20, 21)
                          Bedge(36, 20, 39) Bedge(37, 21, 22) Bedge(38, 22, 23) Bedge(39, 22, 41)
                            Bedge(40, 23, 24) Bedge(41, 23, 43) Bedge(42, 24, 25) Bedge(43, 25, 26)
                              Bedge(44, 26, 27) Bedge(45, 26, 44) Bedge(46, 27, 28) Bedge(47, 27, 46)
                                Bedge(48, 28, 29) Bedge(49, 29, 30) Bedge(50, 30, 31) Bedge(51, 30, 47)
                                  Bedge(52, 31, 32) Bedge(53, 31, 49) Bedge(54, 32, 33) Bedge(55, 33, 34)
                                    Bedge(56, 34, 35) Bedge(57, 34, 50) Bedge(58, 35, 36) Bedge(59, 35, 52)
                                      Bedge(60, 36, 37) Bedge(61, 37, 38) Bedge(62, 38, 39) Bedge(63, 38, 53)
                                        Bedge(64, 39, 40) Bedge(65, 40, 41) Bedge(66, 40, 54) Bedge(67, 41, 42)
                                          Bedge(68, 42, 43) Bedge(69, 42, 56) Bedge(70, 43, 44) Bedge(71, 44, 45)
                                            Bedge(72, 45, 46) Bedge(73, 45, 57) Bedge(74, 46, 47) Bedge(75, 47, 48)
                                              Bedge(76, 48, 49) Bedge(77, 48, 58) Bedge(78, 49, 50) Bedge(79, 50, 51)
                                                Bedge(80, 51, 52) Bedge(81, 51, 59) Bedge(82, 52, 53) Bedge(83, 53, 54)
                                                  Bedge(84, 54, 55) Bedge(85, 55, 56) Bedge(86, 55, 59) Bedge(87, 56, 57)
                                                    Bedge(88, 57, 58) Bedge(89, 58, 59)}; /* end edges[] */

    ADDEDGES(90)
  }

  /*
     The vertices _must_ form each plane in the order given,
     otherwise the normal will have the wrong sign, and the
     object will fail the convex test.
  */

#define Bplane(a, i, j, k, l, m, n) \
  {                                 \
    p->verts[0] = i;                \
    p->verts[1] = j;                \
    p->verts[2] = k;                \
    p->verts[3] = l;                \
    p->verts[4] = m;                \
    p->verts[5] = n;                \
    p++;                            \
  }

  p = b->plane;
  Bplane(0, 1, 8, 7, 6, 5, 0);
  Bplane(1, 2, 11, 10, 9, 8, 1);
  Bplane(2, 3, 14, 13, 12, 11, 2);
  Bplane(3, 4, 17, 16, 15, 14, 3);
  Bplane(4, 0, 5, 19, 18, 17, 4);
  Bplane(5, 7, 24, 23, 22, 21, 6);
  Bplane(6, 10, 28, 27, 26, 25, 9);
  Bplane(7, 13, 32, 31, 30, 29, 12);
  Bplane(8, 16, 36, 35, 34, 33, 15);
  Bplane(9, 19, 20, 39, 38, 37, 18);
  Bplane(10, 40, 39, 20, 21, 22, 41);
  Bplane(11, 43, 23, 24, 25, 26, 44);
  Bplane(12, 46, 27, 28, 29, 30, 47);
  Bplane(13, 49, 31, 32, 33, 34, 50);
  Bplane(14, 52, 35, 36, 37, 38, 53);
  Bplane(15, 55, 54, 40, 41, 42, 56);
  Bplane(16, 56, 42, 43, 44, 45, 57);
  Bplane(17, 57, 45, 46, 47, 48, 58);
  Bplane(18, 58, 48, 49, 50, 51, 59);
  Bplane(19, 59, 51, 52, 53, 54, 55);

#undef Bplane
#define Bplane(a, i, j, k, l, m) \
  {                              \
    p->verts[0] = i;             \
    p->verts[1] = j;             \
    p->verts[2] = k;             \
    p->verts[3] = l;             \
    p->verts[4] = m;             \
    p++;                         \
  }

  Bplane(20, 4, 3, 2, 1, 0);
  Bplane(21, 6, 21, 20, 19, 5);
  Bplane(22, 9, 25, 24, 7, 8);
  Bplane(23, 12, 29, 28, 10, 11);
  Bplane(24, 15, 33, 32, 13, 14);
  Bplane(25, 18, 37, 36, 16, 17);
  Bplane(26, 42, 41, 22, 23, 43);
  Bplane(27, 45, 44, 26, 27, 46);
  Bplane(28, 48, 47, 30, 31, 49);
  Bplane(29, 51, 50, 34, 35, 52);
  Bplane(30, 54, 53, 38, 39, 40);
  Bplane(31, 55, 56, 57, 58, 59);

  /* update misc plane info */
  for (i = 0; i < b->num_planes; i++)
  {
    InitTexdef(&b->plane[i].tex);
  }

  b->EntityRef = M.WorldSpawn;
  b->Group = FindVisGroup(M.WorldGroup);

  B_Link(b);

  AddDBrush(b);
  CheckBrush(b, 1);

  return 1;
}

static int
AddBrush_Torus(int x, int y, int z, int nsides, int nradius, int osides, int oradius)
{
  brush_t* b;
  int i, j, k;
  float radius, height;
  float tempd1, tempd2, tempd3;

  for (k = 0; k < osides; k++)
  {
    b = AllocBrush(nsides * 2, nsides * 3, 2 + nsides);
    if (!b)
      return 0;

    /* Allocate top and bottom planes */
    for (i = 0; i < 2; i++)
    {
      b->plane[i].verts = (int*)Q_malloc(sizeof(int) * nsides);
      b->plane[i].num_verts = nsides;
      if (!b->plane[i].verts)
      {
        HandleError("Add Brush", "Out of memory!");
        return FALSE;
      }
    }

    /* Allocate side planes */
    for (i = 0; i < nsides; i++)
    {
      b->plane[i + 2].verts = (int*)Q_malloc(sizeof(int) * 4);
      b->plane[i + 2].num_verts = 4;
      if (!b->plane[i + 2].verts)
      {
        HandleError("Add Brush", "Out of memory!");
        return FALSE;
      }
    }

    /*
       Create the vertices.
       height is half the width of an torus edge
       tempd3 is the rotation angle to make the floor horizontal

       Method
       1. Make a prism about the y-axis.
       2. Translate it so that (x) is +ve
       3. Shape the ends w.r.t the number of edges.
       4. Translate it back by the torus radius
       5. Rotate each segment into position.

       Todo
       Make the math more efficient.
     */

    height = oradius * sin(PI / osides);
    tempd1 = 2 * PI / nsides;
    tempd3 = ((nsides & 1) == 1) ? PI / 2 : PI / 4;

    for (i = 0; i < nsides; i++)
    {
      tempd2 = cos(tempd3 - (i * tempd1));
      tempd2 *= ((tempd2 < 0.01) && (tempd2 > -0.01)) ? 0 : nradius;
      b->verts[i].x = tempd2 + nradius;

      tempd2 = sin(tempd3 - (i * tempd1));
      tempd2 *= ((tempd2 < 0.01) && (tempd2 > -0.01)) ? 0 : nradius;
      b->verts[i].z = b->verts[i + nsides].z = tempd2;

      b->verts[i].y = height - (b->verts[i].x * tan(PI / osides));
      b->verts[i].x += oradius * cos(PI / osides);

      radius = hypot(b->verts[i].x, b->verts[i].y);

      b->verts[i].y = radius * sin(2 * k * PI / osides);
      b->verts[i].x = radius * cos(2 * k * PI / osides);
      b->verts[i + nsides].y = radius * sin(2 * (k + 1) * PI / osides);
      b->verts[i + nsides].x = radius * cos(2 * (k + 1) * PI / osides);
    }

    /* Create top edges */
    for (i = 0; i < nsides - 1; i++)
    {
      b->edges[i].startvertex = i;
      b->edges[i].endvertex = i + 1;
    }
    b->edges[nsides - 1].startvertex = nsides - 1;
    b->edges[nsides - 1].endvertex = 0;

    /* Create bottom edges */
    for (i = 0; i < nsides - 1; i++)
    {
      b->edges[nsides + i].startvertex = nsides + i;
      b->edges[nsides + i].endvertex = nsides + i + 1;
    }
    b->edges[nsides * 2 - 1].startvertex = nsides * 2 - 1;
    b->edges[nsides * 2 - 1].endvertex = nsides;

    /* Create side edges */
    b->edges[nsides * 2].startvertex = 0;
    b->edges[nsides * 2].endvertex = nsides;
    for (i = 1; i < nsides; i++)
    {
      b->edges[nsides * 2 + i].startvertex = i;
      b->edges[nsides * 2 + i].endvertex = nsides + i;
    }

    /* Do the top plane's vertices */
    for (j = 0; j < nsides; j++)
    {
      b->plane[0].verts[j] = j;
    }

    /* Do the bottom plane's vertices */
    for (j = 0; j < nsides; j++)
    {
      b->plane[1].verts[j] = nsides + nsides - 1 - j;
    }

    /* Do the side plane's vertices */
    for (j = 0; j < nsides; j++)
    {
      b->plane[j + 2].verts[0] = j + nsides;
      b->plane[j + 2].verts[1] = j + nsides + 1;
      b->plane[j + 2].verts[2] = j + 1;
      b->plane[j + 2].verts[3] = j;
    }
    b->plane[nsides + 1].verts[1] = nsides;
    b->plane[nsides + 1].verts[2] = 0;

    for (i = 0; i < nsides + 2; i++)
    {
      InitTexdef(&b->plane[i].tex);
    }
    for (i = 0; i < nsides * 2; i++)
    {
      b->verts[i].x += x;
      b->verts[i].y += y;
      b->verts[i].z += z;
      b->verts[i].x = rint(b->verts[i].x * 10000) / 10000;
      b->verts[i].y = rint(b->verts[i].y * 10000) / 10000;
      b->verts[i].z = rint(b->verts[i].z * 10000) / 10000;
    }
    b->EntityRef = M.WorldSpawn;
    b->Group = FindVisGroup(M.WorldGroup);

    B_Link(b);

    AddDBrush(b);
    CheckBrush(b, 1);
  }
  return 1;
}

int
AddBrush(int type, int x, int y, int z, float info1, float info2, float info3, float info4)
{
  texture_t* Tex;

  /*
  TODO: should we really snap the center here? messes up AddRoom()
  */
  x = SnapPointToGrid(x);
  y = SnapPointToGrid(y);
  z = SnapPointToGrid(z);

  ClearSelEnts();
  status.edit_mode = BRUSH;
  QUI_RedrawWindow(STATUS_WINDOW);

  switch (type)
  {
    case TEXRECT:
      /* Make sure there is a texture to use, first */
      Tex = ReadMIPTex(texturename, 0);
      if (Tex != NULL)
      {
        info1 = Tex->rsx;
        info2 = Tex->rsx;
        info3 = Tex->rsy;
      }
      else
      {
        info1 = info2 = info3 = 64;
      }
    case CUBE:
      return AddBrush_Cube(x, y, z, info1, info2, info3);

    case NPRISM:
      return AddBrush_Prism(x, y, z, info1, info2, info3, info4);

    case PYRAMID:
      return AddBrush_Pyramid(x, y, z, info1, info2, info3, info4);

    case DODEC:
      return AddBrush_Dodec(x, y, z, info1, info2);

    case ICOS:
      return AddBrush_Icos(x, y, z, info1, info2);

    case BUCKY:
      return AddBrush_Bucky(x, y, z, info1, info2);

    case TORUS:
      return AddBrush_Torus(x, y, z, info1, info2, info3, info4);
  }
  HandleError("Add Brush", "Unknown brush type %i!", type);
  return TRUE;
}

void
CreateBrush(int type)
{
#define MAX_KEYS 4
  typedef struct
  {
    int typenum;
    int num;
    const char* name;
    const char*(keys[MAX_KEYS]);
    const char*(def_val[MAX_KEYS]);
  } prim_type_t;
  static prim_type_t prims[] =
    {

      /*
      These MUST match the order of parameters to the actual functions
      above.
      */
      {NPRISM, 4, "Prism", {"Sides", "Radius", "Height", "Eccentricity"}, {"12", "64", "64", "1.0"}},

      {PYRAMID, 4, "Pyramid", {"Sides", "Radius", "Height", "Eccentricity"}, {"4", "64", "64", "1.0"}},

      {DODEC, 2, "Dodecahedron", {"Radius", "Eccentricity"}, {"100", "1.0"}},

      {ICOS, 2, "Icosahedron", {"Length of edge", "Eccentricity"}, {"100", "1.0"}},

      {BUCKY, 2, "Buckyball", {"Radius", "Eccentricity"}, {"100", "1.0"}},

      {TORUS, 4, "Torus", {"Minor Sides", "Minor Radius", "Major Sides", "Major Radius"}, {"10", "20", "8", "100"}},

      {0}};

  int i;
  const char** keys;
  char** values;
  float a[MAX_KEYS];
  int dx, dy, dz;

  prim_type_t* p;

  for (p = prims; p->typenum; p++)
    if (p->typenum == type)
      break;
  if (!p->typenum)
  {
    HandleError("CreateBrush", "Unknown brush type %i!", type);
    return;
  }

  Move(M.display.active_vport, MOVE_FORWARD, &dx, &dy, &dz, 100);

  keys = p->keys;
  values = (char**)Q_malloc(sizeof(char*) * p->num);

  if (!values)
  {
    HandleError("CreateBrush", "Out of memory!");
    return;
  }

  for (i = 0; i < p->num; i++)
  {
    values[i] = (char*)Q_malloc(sizeof(char) * 10);
    if (!values[i])
    {
      HandleError("CreateBrush", "Out of memory!");
      return;
    }

    strcpy(values[i], p->def_val[i]);
  }

  if (QUI_PopEntity(p->name, (char**)keys, values, p->num))
  {
    for (i = 0; i < p->num; i++)
      a[i] = atof(values[i]);

    SUndo(UNDO_NONE, UNDO_NONE);
    AddBrush(type,
             M.display.vport[M.display.active_vport].camera_pos.x + dx,
             M.display.vport[M.display.active_vport].camera_pos.y + dy,
             M.display.vport[M.display.active_vport].camera_pos.z + dz,
             a[0],
             a[1],
             a[2],
             a[3]);
  }

  for (i = 0; i < p->num; i++)
  {
    Q_free(values[i]);
  }
  Q_free(values);
}

void
AddRoom(void)
{
  int i;
  char** keys;
  char** values;
  int x, y, z, t;
  int dx, dy, dz;

  Move(M.display.active_vport, MOVE_FORWARD, &dx, &dy, &dz, 100);

  keys = (char**)Q_malloc(sizeof(char*) * 4);
  values = (char**)Q_malloc(sizeof(char*) * 4);
  if (!keys || !values)
  {
    HandleError("AddRoom", "Out of memory!");
    return;
  }

  for (i = 0; i < 4; i++)
  {
    keys[i] = (char*)Q_malloc(sizeof(char) * 30);
    values[i] = (char*)Q_malloc(sizeof(char) * 10);
    if (!keys[i] || !values[i])
    {
      HandleError("AddRoom", "Out of memory!");
      return;
    }
  }

  strcpy(keys[0], "X size");
  strcpy(keys[1], "Y size");
  strcpy(keys[2], "Z size");
  strcpy(keys[3], "Wall thickness");
  sprintf(values[0], "256");
  sprintf(values[1], "256");
  sprintf(values[2], "256");
  sprintf(values[3], "16");

  if (QUI_PopEntity("Create Room", keys, values, 4))
  {
    x = atoi(values[0]);
    y = atoi(values[1]);
    z = atoi(values[2]);
    t = atoi(values[3]);

    SUndo(UNDO_NONE, UNDO_NONE);

    AddBrush(CUBE,
             M.display.vport[M.display.active_vport].camera_pos.x + dx + ((x + t) >> 1),
             M.display.vport[M.display.active_vport].camera_pos.y + dy,
             M.display.vport[M.display.active_vport].camera_pos.z + dz,
             t,
             y,
             z,
             1);
    AddBrush(CUBE,
             M.display.vport[M.display.active_vport].camera_pos.x + dx - ((x + t) >> 1),
             M.display.vport[M.display.active_vport].camera_pos.y + dy,
             M.display.vport[M.display.active_vport].camera_pos.z + dz,
             t,
             y,
             z,
             1);
    AddBrush(CUBE,
             M.display.vport[M.display.active_vport].camera_pos.x + dx,
             M.display.vport[M.display.active_vport].camera_pos.y + dy + ((y + t) >> 1),
             M.display.vport[M.display.active_vport].camera_pos.z + dz,
             x,
             t,
             z,
             1);
    AddBrush(CUBE,
             M.display.vport[M.display.active_vport].camera_pos.x + dx,
             M.display.vport[M.display.active_vport].camera_pos.y + dy - ((y + t) >> 1),
             M.display.vport[M.display.active_vport].camera_pos.z + dz,
             x,
             t,
             z,
             1);
    AddBrush(CUBE,
             M.display.vport[M.display.active_vport].camera_pos.x + dx,
             M.display.vport[M.display.active_vport].camera_pos.y + dy,
             M.display.vport[M.display.active_vport].camera_pos.z + dz + ((z + t) >> 1),
             x,
             y,
             t,
             1);
    AddBrush(CUBE,
             M.display.vport[M.display.active_vport].camera_pos.x + dx,
             M.display.vport[M.display.active_vport].camera_pos.y + dy,
             M.display.vport[M.display.active_vport].camera_pos.z + dz - ((z + t) >> 1),
             x,
             y,
             t,
             1);
  }

  for (i = 0; i < 4; i++)
  {
    Q_free(keys[i]);
    Q_free(values[i]);
  }
  Q_free(keys);
  Q_free(values);
}
