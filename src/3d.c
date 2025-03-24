/*
3d.c file of the Quest Source Code

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

#include "3d.h"

#include "3d_curve.h"
#include "brush.h"
#include "bspspan.h"
#include "camera.h"
#include "color.h"
#include "display.h"
#include "edmodel.h"
#include "entity.h"
#include "error.h"
#include "geom.h"
#include "map.h"
#include "mdl.h"
#include "memory.h"
#include "message.h"
#include "poly.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "video.h"

static int colors[] =
  {
#define C_BACK (colors[0])
#define CR_BACK (rcolors[0])
    0,

#define C_GRID_1 (colors[1])
#define CR_GRID_1 (rcolors[1])
    COL_BLUE - 8,
#define C_GRID_2 (colors[2])
#define CR_GRID_2 (rcolors[2])
    COL_BLUE - 5,
#define C_GRID_TEXT (colors[3])
#define CR_GRID_TEXT (rcolors[3])
    COL_WHITE,

#define C_BRUSH_SELECT (colors[4])
#define CR_BRUSH_SELECT (rcolors[4])
    COL_YELLOW,
#define C_BRUSH_FOCAL_SUB (colors[5])
#define CR_BRUSH_FOCAL_SUB (rcolors[5])
    COL_PURPLE,
#define C_BRUSH_FOCAL (colors[6])
#define CR_BRUSH_FOCAL (rcolors[6])
    COL_RED,

#define C_FACE_SELECT (colors[7])
#define CR_FACE_SELECT (rcolors[7])
    COL_YELLOW,
#define C_FACE_FOCAL (colors[8])
#define CR_FACE_FOCAL (rcolors[8])
    COL_GREEN,

#define C_VERT (colors[9])
#define CR_VERT (rcolors[9])
    COL_CYAN,
#define C_VERT_SEL (colors[10])
#define CR_VERT_SEL (rcolors[10])
    COL_RED,

#define C_BORDER_ACTIVE (colors[11])
#define CR_BORDER_ACTIVE (rcolors[11])
    COL_WHITE,
#define C_BORDER_OTHER (colors[12])
#define CR_BORDER_OTHER (rcolors[12])
    COL_WHITE - 8,

#define C_PTS_DIST (colors[13])
#define CR_PTS_DIST (rcolors[13])
    COL_YELLOW,
#define C_PTS_DIST_TEXT (colors[14])
#define CR_PTS_DIST_TEXT (rcolors[14])
    COL_WHITE,

#define C_PTS_BR_END (colors[15])
#define CR_PTS_BR_END (rcolors[15])
    COL_RED,
#define C_PTS_BR (colors[16])
#define CR_PTS_BR (rcolors[16])
    COL_YELLOW,

#define C_PTS_LEAK_START (colors[17])
#define CR_PTS_LEAK_START (rcolors[17])
    COL_GREEN,
#define C_PTS_LEAK_END (colors[18])
#define CR_PTS_LEAK_END (rcolors[18])
    COL_BLUE,
#define C_PTS_LEAK (colors[19])
#define CR_PTS_LEAK (rcolors[19])
    COL_RED,

#define C_AXIS_X (colors[20])
#define CR_AXIS_X (rcolors[20])
    COL_RED,
#define C_AXIS_Y (colors[21])
#define CR_AXIS_Y (rcolors[21])
    COL_GREEN,
#define C_AXIS_Z (colors[22])
#define CR_AXIS_Z (rcolors[22])
    COL_CYAN,

#define C_MAX 23
};

static int rcolors[C_MAX];

// #define  ALPHA  (25*PI)/180
#define ALPHA 40 * PI / 180

// Transform vec3_t s to vec3_t d using matrix_t m
#define Transform(d, s, m) \
  ({                       \
    d.x = m[0][0] * s.x +  \
          m[0][1] * s.y +  \
          m[0][2] * s.z +  \
          m[0][3];         \
    d.y = m[1][0] * s.x +  \
          m[1][1] * s.y +  \
          m[1][2] * s.z +  \
          m[1][3];         \
    d.z = m[2][0] * s.x +  \
          m[2][1] * s.y +  \
          m[2][2] * s.z +  \
          m[2][3];         \
  })

static viewport_t* cur_vp; // current viewport we're drawing to
static int cur_vport;
static int dx, dy;

// Calculate a svec_t from a vec3_t.
static inline int
ToScreen(svec_t* d, vec3_t s)
{
  switch (cur_vp->mode)
  {
    case NOPERSP:
      if (!cur_vp->fullbright)
      {
        if ((s.z < 0) || (s.z > status.depth_clip))
        {
          d->onscreen = 0;
          return 0;
        }
      }
      d->x = s.x * cur_vp->zoom_amt + cur_vp->xmin + dx;
      d->y = -s.y * cur_vp->zoom_amt + cur_vp->ymin + dy;
      break;

    case WIREFRAME:
    case SOLID:
      if ((s.z <= 0) ||
          (s.x > s.z) ||
          (s.x < -s.z) ||
          (s.y > s.z) ||
          (s.y < -s.z))
      {
        d->onscreen = 0;
        return 0;
      }
      if ((!cur_vp->fullbright) && (s.z > 1))
      {
        d->onscreen = 0;
        return 0;
      }

      d->x = dx * (s.x / s.z + 1) + cur_vp->xmin + 1;
      d->y = dy * (s.y / -s.z + 1) + cur_vp->ymin + 1;
      break;

    default:
      d->onscreen = 0;
      return 0;
  }
  d->onscreen = 1;
  return 1;
}

void
InitMatrix(matrix_t matrix)
{
  int i, j;

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      if (i == j)
        matrix[i][j] = 1;
      else
        matrix[i][j] = 0;
    }
  }
}

void
MultMatrix(matrix_t one, matrix_t two, matrix_t three)
{
  int i, j;
  matrix_t prod;

  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      prod[i][j] = (one[i][0] * two[0][j]) + (one[i][1] * two[1][j]) +
                   (one[i][2] * two[2][j]) + (one[i][3] * two[3][j]);
    }
  }
  for (i = 0; i < 4; i++)
  {
    for (j = 0; j < 4; j++)
    {
      three[i][j] = prod[i][j];
    }
  }
}

#if 0
static void PrintMatrix(char *name,matrix_t m)
{
   int i,j;

   printf("%s=\n",name);
   for (i=0;i<4;i++)
   {
      printf("(");
      for (j=0;j<4;j++)
      {
         printf(" %8.2f",m[i][j]);
      }
      printf(" )\n");
   }
}
#endif

void
GetRotValues(int vport, int* rx, int* ry, int* rz)
{
  viewport_t* v;

  v = &M.display.vport[vport];

  if (v->axis_aligned)
  {
    v->rot_x = v->rot_y = v->rot_z = 0;
    if (v->camera_dir & LOOK_UP)
      v->rot_x = 0;
    else if (v->camera_dir & LOOK_DOWN)
      v->rot_x = 180;
    else
      v->rot_x = 90;

    switch (v->camera_dir & 0x03)
    {
      case LOOK_NEG_Y:
        v->rot_z = 0;
        break;
      case LOOK_NEG_X:
        v->rot_z = 90;
        break;
      case LOOK_POS_Y:
        v->rot_z = 180;
        break;
      case LOOK_POS_X:
        v->rot_z = 270;
        break;
    }
  }
  v->rot_x %= 360;
  v->rot_y %= 360;
  v->rot_z %= 360;
  *rx = v->rot_x;
  *ry = v->rot_y;
  *rz = v->rot_z;
}

void
GenerateIRotMatrix(matrix_t m, int rx, int ry, int rz)
{
  matrix_t rot1, rot2, rot3, rot4;
  float r_sx, r_cx;
  float r_sy, r_cy;
  float r_sz, r_cz;
  int i, j;

  r_sx = sin(rx * PI / 180);
  r_cx = cos(rx * PI / 180);

  r_sy = sin(ry * PI / 180);
  r_cy = cos(ry * PI / 180);

  r_sz = sin(rz * PI / 180);
  r_cz = cos(rz * PI / 180);

  InitMatrix(rot1);
  rot1[0][0] = 1;
  rot1[0][1] = 0;
  rot1[0][2] = 0;

  rot1[1][0] = 0;
  rot1[1][1] = r_cx;
  rot1[1][2] = r_sx;

  rot1[2][0] = 0;
  rot1[2][1] = -r_sx;
  rot1[2][2] = r_cx;

  InitMatrix(rot2);
  rot2[0][0] = r_cz;
  rot2[0][1] = r_sz;
  rot2[0][2] = 0;

  rot2[1][0] = -r_sz;
  rot2[1][1] = r_cz;
  rot2[1][2] = 0;

  rot2[2][0] = 0;
  rot2[2][1] = 0;
  rot2[2][2] = 1;

  InitMatrix(rot3);
  rot3[0][0] = r_cy;
  rot3[0][1] = r_sy;
  rot3[0][2] = 0;

  rot3[1][0] = -r_sy;
  rot3[1][1] = r_cy;
  rot3[1][2] = 0;

  rot3[2][0] = 0;
  rot3[2][1] = 0;
  rot3[2][2] = 1;

  MultMatrix(rot2, rot1, rot4);

  MultMatrix(rot4, rot3, rot2);

  for (i = 0; i < 4; i++)
    rot2[1][i] = -rot2[1][i];

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      m[i][j] = rot2[i][j];
}

void
GenerateRotMatrix(matrix_t m, int rx, int ry, int rz)
{
  matrix_t rot1, rot2, rot3;
  float r_sx, r_cx;
  float r_sy, r_cy;
  float r_sz, r_cz;
  int i, j;

  r_sx = sin(rx * PI / 180);
  r_cx = cos(rx * PI / 180);

  r_sy = sin(ry * PI / 180);
  r_cy = cos(ry * PI / 180);

  r_sz = sin(rz * PI / 180);
  r_cz = cos(rz * PI / 180);

  InitMatrix(rot1);
  rot1[0][0] = 1;
  rot1[0][1] = 0;
  rot1[0][2] = 0;

  rot1[1][0] = 0;
  rot1[1][1] = r_cx;
  rot1[1][2] = r_sx;

  rot1[2][0] = 0;
  rot1[2][1] = -r_sx;
  rot1[2][2] = r_cx;

  InitMatrix(rot2);
  rot2[0][0] = r_cz;
  rot2[0][1] = r_sz;
  rot2[0][2] = 0;

  rot2[1][0] = -r_sz;
  rot2[1][1] = r_cz;
  rot2[1][2] = 0;

  rot2[2][0] = 0;
  rot2[2][1] = 0;
  rot2[2][2] = 1;

  MultMatrix(rot1, rot2, rot3);

  InitMatrix(rot2);
  rot2[0][0] = r_cy;
  rot2[0][1] = r_sy;
  rot2[0][2] = 0;

  rot2[1][0] = -r_sy;
  rot2[1][1] = r_cy;
  rot2[1][2] = 0;

  rot2[2][0] = 0;
  rot2[2][1] = 0;
  rot2[2][2] = 1;

  MultMatrix(rot2, rot3, rot1);

  for (i = 0; i < 4; i++)
    rot1[i][1] = -rot1[i][1];

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      m[i][j] = rot1[i][j];
}

void
GenerateMatrix(matrix_t m, int vport)
{
  int i, j;
  matrix_t mmain, /* Matrices used for rotations, tranformations */
    rottrans,     /* between spaces */
    trans,
    rot,
    canon;
  float ar; /* Aspect ration of screen (width/height) */
  int rx, ry, rz;

  viewport_t* vp;

  vp = &M.display.vport[vport];

  ar = (float)(vp->xmax - vp->xmin) /
       (float)(vp->ymax - vp->ymin);

  /* First build main transformation matrix */
  InitMatrix(mmain);

  InitMatrix(trans);
  trans[0][3] = -vp->camera_pos.x;
  trans[1][3] = -vp->camera_pos.y;
  trans[2][3] = -vp->camera_pos.z;

  GetRotValues(vport, &rx, &ry, &rz);
  GenerateRotMatrix(rot, rx, ry, rz);

  InitMatrix(canon);
  if (vp->mode != NOPERSP)
  {
    canon[0][0] = (float)1 / (status.depth_clip * tan(ALPHA));
    canon[1][1] = (float)ar / (status.depth_clip * tan(ALPHA));
    canon[2][2] = (float)1 / status.depth_clip;
  }

  /* Multiply Matricies into one tranformation matrix */
  MultMatrix(rot, trans, rottrans);
  MultMatrix(canon, rottrans, mmain);

  /* Copy into result matrix */
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      m[i][j] = mmain[i][j];
}

void
GenerateIMatrix(matrix_t m, int vport)
{
  int i, j;
  matrix_t mmain, /* Matrices used for rotations, tranformations */
    rottrans,     /* between spaces */
    trans,
    rot,
    canon;
  float ar; /* Aspect ration of screen (width/height) */
  int rx, ry, rz;

  viewport_t* vp;

  vp = &M.display.vport[vport];

  ar = (float)(vp->xmax - vp->xmin) /
       (float)(vp->ymax - vp->ymin);

  /* First build main transformation matrix */
  InitMatrix(mmain);

  InitMatrix(trans);
  trans[0][3] = vp->camera_pos.x;
  trans[1][3] = vp->camera_pos.y;
  trans[2][3] = vp->camera_pos.z;

  GetRotValues(vport, &rx, &ry, &rz);
  GenerateIRotMatrix(rot, -rx, -ry, -rz);

  InitMatrix(canon);
  if (vp->mode != NOPERSP)
  {
    canon[0][0] = (float)(status.depth_clip * tan(ALPHA));
    canon[1][1] = (float)(status.depth_clip * tan(ALPHA)) / ar;
    canon[2][2] = (float)status.depth_clip;
  }

  /* Multiply Matricies into one tranformation matrix */
  MultMatrix(trans, rot, rottrans);
  MultMatrix(rottrans, canon, mmain);

  /* Copy into result matrix */
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      m[i][j] = mmain[i][j];
}

static void
DrawGrid(int snap, int col)
{
  int x1;
  int mx, my, mz;
  int ofs;
  int s;
  int ds;

  float pos, delta;

  int i;

  delta = (float)snap * cur_vp->zoom_amt;
  if (delta <= 4)
    return;

  /*   if (delta<56)
        c=0;*/

  // x grid
  x1 = (((float)-dx) / cur_vp->zoom_amt);

  Move(cur_vport, MOVE_RIGHT, &mx, &my, &mz, 1);
  if (mx)
  {
    ofs = cur_vp->camera_pos.x;
    i = mx;
  }
  else if (my)
  {
    ofs = cur_vp->camera_pos.y;
    i = my;
  }
  else
  {
    ofs = cur_vp->camera_pos.z;
    i = mz;
  }
  x1 += i * ofs;
  x1 -= x1 % snap;

  pos = (x1 - i * ofs) * cur_vp->zoom_amt + cur_vp->xmin + dx;

  x1 *= i;

  s = x1;
  if (i == -1)
    ds = -snap;
  else
    ds = snap;

  i = 0;
  while (pos < cur_vp->xmin)
  {
    s += ds;
    pos += delta;
    i++;
  }
  /*   if (i>1)
        NewMessage("x: %i lines not drawn!",i);*/

  while (pos < cur_vp->xmax)
  {
    DrawLine(pos, cur_vp->ymin, pos, cur_vp->ymax, col);
    pos += delta;
    s += ds;
  }

  // y grid
  x1 = -(((float)-dy) / cur_vp->zoom_amt);

  Move(cur_vport, MOVE_DOWN, &mx, &my, &mz, 1);
  if (mx)
  {
    ofs = cur_vp->camera_pos.x;
    i = mx;
  }
  else if (my)
  {
    ofs = cur_vp->camera_pos.y;
    i = my;
  }
  else
  {
    ofs = cur_vp->camera_pos.z;
    i = mz;
  }
  x1 -= i * ofs;
  x1 -= x1 % snap;

  pos = -(x1 + i * ofs) * cur_vp->zoom_amt + cur_vp->ymin + dy;

  x1 *= -i;

  s = x1;
  if (i == -1)
    ds = -snap;
  else
    ds = snap;

  i = 0;
  while (pos < cur_vp->ymin)
  {
    s += ds;
    pos += delta;
    i++;
  }
  //   if (i>1)
  //      NewMessage("y: %i lines not drawn!",i);

  while (pos < cur_vp->ymax)
  {
    DrawLine(cur_vp->xmin, pos, cur_vp->xmax, pos, col);
    pos += delta;
    s += ds;
  }
}

static void
DrawGridNum(void)
{
  int x1;
  int mx, my, mz;
  int ofs;
  int s;
  int ds;

  float pos, delta;

  int i;

  char temp[16];
  int len;

  int snap;

  delta = 64 / cur_vp->zoom_amt;
  for (snap = 1; snap < delta; snap <<= 1)
    ;

  delta = (float)snap * cur_vp->zoom_amt;

  // x grid
  x1 = (((float)-dx) / cur_vp->zoom_amt);

  Move(cur_vport, MOVE_RIGHT, &mx, &my, &mz, 1);
  if (mx)
  {
    ofs = cur_vp->camera_pos.x;
    i = mx;
  }
  else if (my)
  {
    ofs = cur_vp->camera_pos.y;
    i = my;
  }
  else
  {
    ofs = cur_vp->camera_pos.z;
    i = mz;
  }
  x1 += i * ofs;
  x1 -= x1 % snap;

  pos = (x1 - i * ofs) * cur_vp->zoom_amt + cur_vp->xmin + dx;

  x1 *= i;

  s = x1;
  if (i == -1)
    ds = -snap;
  else
    ds = snap;

  i = 0;
  while (pos < cur_vp->xmin)
  {
    s += ds;
    pos += delta;
    i++;
  }
  /*   if (i>1)
        NewMessage("x: %i lines not drawn!",i);*/

  //   i=0;
  while (pos < cur_vp->xmax)
  {
    sprintf(temp, "%i", s);
    len = QUI_strlen(0, temp);
    i = pos - len / 2;
    if ((i > cur_vp->xmin) && (i + len < cur_vp->xmax))
    {
      QUI_DrawStr(i, cur_vp->ymin + 2, -1, C_GRID_TEXT, 0, 0, "%s", temp);
    }

    pos += delta;
    s += ds;
  }

  // y grid
  x1 = -(((float)-dy) / cur_vp->zoom_amt);

  Move(cur_vport, MOVE_DOWN, &mx, &my, &mz, 1);
  if (mx)
  {
    ofs = cur_vp->camera_pos.x;
    i = mx;
  }
  else if (my)
  {
    ofs = cur_vp->camera_pos.y;
    i = my;
  }
  else
  {
    ofs = cur_vp->camera_pos.z;
    i = mz;
  }
  x1 -= i * ofs;
  x1 -= x1 % snap;

  pos = -(x1 + i * ofs) * cur_vp->zoom_amt + cur_vp->ymin + dy;

  x1 *= -i;

  s = x1;
  if (i == -1)
    ds = -snap;
  else
    ds = snap;

  i = 0;
  while (pos < cur_vp->ymin)
  {
    s += ds;
    pos += delta;
    i++;
  }
  //   if (i>1)
  //      NewMessage("y: %i lines not drawn!",i);

  while (pos < cur_vp->ymax)
  {
    i = pos - ROM_CHAR_HEIGHT / 2;
    if (pos + ROM_CHAR_HEIGHT / 2 > cur_vp->ymax)
      break;

    if (i > cur_vp->ymin)
    {
      QUI_DrawStr(cur_vp->xmin + 2, i, -1, C_GRID_TEXT, 0, 0, "%i", s);
    }
    pos += delta;
    s += ds;
  }
}

static void
DrawGrids(void)
{
  if (!cur_vp->axis_aligned)
    return;
  DrawGrid(status.snap_size, CR_GRID_1);
  DrawGrid(status.snap_size * 4, CR_GRID_2);
  DrawGridNum();
}

void
UpdateViewportBorder(int vport)
{
  int border_color;
  viewport_t* v;

  /* Draw viewport border */
  if (vport == M.display.active_vport)
    border_color = C_BORDER_ACTIVE;
  else
    border_color = C_BORDER_OTHER;

  v = &M.display.vport[vport];

  QUI_Box(v->xmin, v->ymin, v->xmax, v->ymax, border_color, border_color);
}

static int
OutsideFrustrum(vec3_t from, vec3_t to)
{
  if ((from.z <= 0) && (to.z <= 0))
    return TRUE;
  if ((from.x > from.z) && (to.x > to.z))
    return TRUE;
  if ((from.x < -from.z) && (to.x < -to.z))
    return TRUE;
  if ((from.y > from.z) && (to.y > to.z))
    return TRUE;
  if ((from.y < -from.z) && (to.y < -to.z))
    return TRUE;
  return FALSE;
}

static void
DrawBrushFaceEdges(brush_t* b, int face, int base_color, int force_bright)
{
  int color;
  int from, to;
  float x0, y0, x1, y1;
  float clipx, clipy;
  edge_t edge;
  int found;
  int was_fullbright;
  int i, j;
  vec3_t* tverts;
  int xs0, ys0;
  int xs1, ys1;
  int xs2, ys2;
  int c0, c1;

  if (cur_vp->fullbright)
    was_fullbright = TRUE;
  else
    was_fullbright = FALSE;

  if (force_bright)
    cur_vp->fullbright = TRUE;

  tverts = b->tverts;
  for (i = 0; i < b->num_edges; i++)
  {
    from = b->edges[i].startvertex;
    to = b->edges[i].endvertex;

    /* Check if edge is in face */
    found = FALSE;
    for (j = 0; j < b->plane[face].num_verts; j++)
    {
      edge.startvertex = b->plane[face].verts[j];
      edge.endvertex = b->plane[face].verts[(j + 1) % b->plane[face].num_verts];
      if (EdgesAreEqual(edge, b->edges[i]))
      {
        found = TRUE;
        break;
      }
    }
    if (!found)
      continue;

    switch (cur_vp->mode)
    {
      case NOPERSP:
        if (!cur_vp->fullbright)
        {
          if (((tverts[from].z < 0) && (tverts[to].z < 0)) ||
              ((tverts[from].z > status.depth_clip) &&
               (tverts[to].z > status.depth_clip)))
          {
            continue;
          }
        }

        clipx = dx / cur_vp->zoom_amt;
        clipy = dy / cur_vp->zoom_amt;

        if (((tverts[from].x > clipx) && (tverts[to].x > clipx)) ||
            ((tverts[from].x < -clipx) && (tverts[to].x < -clipx)) ||
            ((tverts[from].y > clipy) && (tverts[to].y > clipy)) ||
            ((tverts[from].y < -clipy) && (tverts[to].y < -clipy)))
          continue;

        x0 = tverts[from].x * cur_vp->zoom_amt + cur_vp->xmin + dx;
        y0 = -tverts[from].y * cur_vp->zoom_amt + cur_vp->ymin + dy;
        x1 = tverts[to].x * cur_vp->zoom_amt + cur_vp->xmin + dx;
        y1 = -tverts[to].y * cur_vp->zoom_amt + cur_vp->ymin + dy;

        xs0 = x0 + 0.5;
        ys0 = y0 + 0.5;
        xs1 = x1 + 0.5;
        ys1 = y1 + 0.5;

restart_clip2d:

        c0 = OutCode2D(cur_vport, xs0, ys0);
        c1 = OutCode2D(cur_vport, xs1, ys1);

        if ((c0 | c1) == 0)
        {
          if (cur_vp->grid_type == ALIGN)
            Draw2DSpecialLine(cur_vport, xs0, ys0, xs1, ys1, base_color);
          else
            DrawLine(xs0, ys0, xs1, ys1, base_color);
          break;
        }
        else if ((c0 & c1) != 0)
          break;
        else if (c0 != 0)
        {
          Clip2D(cur_vport, xs0, ys0, xs1, ys1, &xs2, &ys2, c0);
          xs0 = xs2;
          ys0 = ys2;
        }
        else
        {
          Clip2D(cur_vport, xs0, ys0, xs1, ys1, &xs2, &ys2, c1);
          xs1 = xs2;
          ys1 = ys2;
        }
        goto restart_clip2d;
        break;

      case WIREFRAME:
      case SOLID:
        /* Immediately throw out lines outside view frustum */

        if (OutsideFrustrum(tverts[from], tverts[to]))
          continue;

        if (!cur_vp->fullbright)
        {
          if ((tverts[from].z > 1) && (tverts[to].z > 1))
            continue;
        }

        if ((!cur_vp->fullbright) && (!force_bright))
        {
          if (tverts[to].z < 0)
            color = base_color - tverts[from].z * 6;
          else if (b->tverts[from].z < 0)
            color = base_color - tverts[to].z * 6;
          else
          {
            color = (tverts[from].z + tverts[to].z) * 5;
            if (color > 15)
              color = 15;
            color = base_color - color;
          }
        }
        else
          color = base_color;

        ClipDrawLine3D(cur_vport,
                       tverts[from].x,
                       tverts[from].y,
                       tverts[from].z,
                       tverts[to].x,
                       tverts[to].y,
                       tverts[to].z,
                       color);
        break;
    }
  }

  if (force_bright && !was_fullbright)
    cur_vp->fullbright = FALSE;
}

static void
DrawBrushFace(brush_t* b, int face, int base_color, int force_bright)
{
  if (b->bt->flags & BR_F_EDGES)
  {
    DrawBrushFaceEdges(b, face, base_color, force_bright);
    return;
  }
}

static void
DrawBrushEdges(int vport, brush_t* b, int base_color, int selected)
{
  int color;    /* Color of line*/
  int from, to; /* Index of "from" and "to" vertices*/
  float x0, y0, x1, y1;
  int dx, dy;
  float clipx, clipy;
  int was_fullbright;
  int i;
  vec3_t* tverts;
  int xs0, ys0;
  int xs1, ys1;
  int xs2, ys2;
  int c0, c1;

  if (M.display.vport[vport].fullbright)
    was_fullbright = TRUE;
  else
    was_fullbright = FALSE;

  if (selected)
    M.display.vport[vport].fullbright = TRUE;

  tverts = b->tverts;
  for (i = 0; i < b->num_edges; i++)
  {
    from = b->edges[i].startvertex;
    to = b->edges[i].endvertex;

    switch (M.display.vport[vport].mode)
    {
      case NOPERSP:
        if (!M.display.vport[vport].fullbright)
        {
          if (((tverts[from].z < 0) && (tverts[to].z < 0)) ||
              ((tverts[from].z > status.depth_clip) &&
               (tverts[to].z > status.depth_clip)))
            continue;
        }

        dx = (M.display.vport[vport].xmax - M.display.vport[vport].xmin) >> 1;
        dy = (M.display.vport[vport].ymax - M.display.vport[vport].ymin) >> 1;

        clipx = dx / M.display.vport[vport].zoom_amt;
        clipy = dy / M.display.vport[vport].zoom_amt;

        if (((tverts[from].x > clipx) && (tverts[to].x > clipx)) ||
            ((tverts[from].x < -clipx) && (tverts[to].x < -clipx)) ||
            ((tverts[from].y > clipy) && (tverts[to].y > clipy)) ||
            ((tverts[from].y < -clipy) && (tverts[to].y < -clipy)))
          continue;

        x0 = tverts[from].x * M.display.vport[vport].zoom_amt + M.display.vport[vport].xmin + dx;
        y0 = -tverts[from].y * M.display.vport[vport].zoom_amt + M.display.vport[vport].ymin + dy;
        x1 = tverts[to].x * M.display.vport[vport].zoom_amt + M.display.vport[vport].xmin + dx;
        y1 = -tverts[to].y * M.display.vport[vport].zoom_amt + M.display.vport[vport].ymin + dy;

        xs0 = x0 + 0.5;
        ys0 = y0 + 0.5;
        xs1 = x1 + 0.5;
        ys1 = y1 + 0.5;

restart_clip2d:

        c0 = OutCode2D(vport, xs0, ys0);
        c1 = OutCode2D(vport, xs1, ys1);

        if ((c0 | c1) == 0)
        {
          if (M.display.vport[vport].grid_type == ALIGN)
            Draw2DSpecialLine(vport, xs0, ys0, xs1, ys1, base_color);
          else
            DrawLine(xs0, ys0, xs1, ys1, base_color);
          break;
        }
        else if ((c0 & c1) != 0)
          break;
        else if (c0 != 0)
        {
          Clip2D(vport, xs0, ys0, xs1, ys1, &xs2, &ys2, c0);
          xs0 = xs2;
          ys0 = ys2;
        }
        else
        {
          Clip2D(vport, xs0, ys0, xs1, ys1, &xs2, &ys2, c1);
          xs1 = xs2;
          ys1 = ys2;
        }
        goto restart_clip2d;
        break;

      case WIREFRAME:
      case SOLID:
        /* Immediately throw out lines outside view frustum */

        if (OutsideFrustrum(tverts[from], tverts[to]))
          continue;

        if (!M.display.vport[vport].fullbright)
        {
          if ((tverts[from].z > 1) && (tverts[to].z > 1))
            continue;
        }

        if ((!M.display.vport[vport].fullbright) && (!selected))
        {
          if (tverts[to].z < 0)
            color = COL_DARK(base_color, tverts[from].z * 6);
          else if (b->tverts[from].z < 0)
            color = COL_DARK(base_color, tverts[to].z * 6);
          else
          {
            color = (tverts[from].z + tverts[to].z) * 5;
            if (color > 15)
              color = 15;
            color = COL_DARK(base_color, color);
          }
        }
        else
          color = base_color;

        ClipDrawLine3D(vport,
                       tverts[from].x,
                       tverts[from].y,
                       tverts[from].z,
                       tverts[to].x,
                       tverts[to].y,
                       tverts[to].z,
                       color);
        break;
    }
  }

  if (selected && !was_fullbright)
    M.display.vport[vport].fullbright = FALSE;
}

void
DrawBrush(int vport, brush_t* b, int color, int selected)
{
  if (b->bt->flags & BR_F_EDGES)
  {
    DrawBrushEdges(vport, b, color, selected);
    return;
  }
  if (b->bt->type == BR_Q3_CURVE)
  {
    DrawBrush_Q3Curve(vport, b, color, selected);
    return;
  }
  /* We don't know how to draw this brush, so ignore it */
}

void
ReadPts(char* name)
{
  FILE* f;
  vec3_t t;

  if (M.npts)
  {
    Q_free(M.pts);
    Q_free(M.tpts);
  }

  f = fopen(name, "rt");
  if (!f)
    return;

  M.npts = 0;
  M.pts = M.tpts = NULL;
  while (!feof(f))
  {
    if (fscanf(f, "%f %f %f", &t.x, &t.y, &t.z) != 3)
      break;
    M.pts = Q_realloc(M.pts, sizeof(vec3_t) * (M.npts + 1));
    if (!M.pts)
    {
      fclose(f);
      HandleError("ReadPts", "Out of memory!");
      M.npts = 0;
      return;
    }
    M.pts[M.npts] = t;
    M.npts++;
  }
  fclose(f);

  M.tpts = Q_malloc(sizeof(vec3_t) * M.npts);
  if (!M.tpts)
  {
    Q_free(M.pts);
    HandleError("ReadPts", "Out of memory!");
    M.npts = 0;
    return;
  }

  NewMessage("%i points read", M.npts);
}

void
FreePts(void)
{
  if (M.npts)
  {
    Q_free(M.pts);
    Q_free(M.tpts);
    M.npts = 0;
    M.pts = M.tpts = NULL;
  }
}

static void
DrawPts(matrix_t m)
{
  int i;

  int color;
  float x0, y0, x1, y1;
  float clipx, clipy;
  int xs0, xs1, xs2;
  int ys0, ys1, ys2;
  int c0, c1;

  svec_t s;

  if (!status.draw_pts)
    return;

  if (!M.npts)
    return;

  for (i = 0; i < M.npts; i++)
  {
    Transform(M.tpts[i], M.pts[i], m);
  }

  for (i = 0; i < M.npts - 1; i++)
  {
    if (status.draw_pts == 3)
    {
      color = CR_PTS_DIST;

      ToScreen(&s, M.tpts[i + 1]);
      if (s.onscreen)
      {
        float dx, dy, dz;
        int x, y;

        dx = M.pts[i].x - M.pts[i + 1].x;
        dy = M.pts[i].y - M.pts[i + 1].y;
        dz = M.pts[i].z - M.pts[i + 1].z;
        dx *= dx;
        dy *= dy;
        dz *= dz;
        dx = sqrt(dx + dy + dz);

        x = s.x - 32;
        if (x < cur_vp->xmin + 1)
          x = cur_vp->xmin + 1;

        y = s.y - 32;
        if (y < cur_vp->ymin + 1)
          y = cur_vp->ymin + 1;

        if (y + 20 < cur_vp->ymax)
          QUI_DrawStrM(x, y, cur_vp->xmax - 2, -1, C_PTS_DIST_TEXT, 0, 0, "%0.1f", dx);
      }
    }
    else if (status.draw_pts == 2)
    {
      if (i == M.npts - 2)
        color = CR_PTS_BR_END;
      else
        color = CR_PTS_BR;

      ToScreen(&s, M.tpts[i]);
      if (s.onscreen)
        DrawVertex(cur_vport, s, CR_VERT);
    }
    else
    {
      if (i == (M.npts - 2))
        color = CR_PTS_LEAK_END;
      else if (i == 0)
        color = CR_PTS_LEAK_START;
      else
        color = CR_PTS_LEAK;
    }

    switch (cur_vp->mode)
    {
      case NOPERSP:
        if (!cur_vp->fullbright)
        {
          if (((M.tpts[i].z < 0) && (M.tpts[i + 1].z < 0)) ||
              ((M.tpts[i].z > status.depth_clip) &&
               (M.tpts[i + 1].z > status.depth_clip)))
            continue;
        }

        clipx = dx / cur_vp->zoom_amt;
        clipy = dy / cur_vp->zoom_amt;

        if (((M.tpts[i].x > clipx) && (M.tpts[i + 1].x > clipx)) ||
            ((M.tpts[i].x < -clipx) && (M.tpts[i + 1].x < -clipx)) ||
            ((M.tpts[i].y > clipy) && (M.tpts[i + 1].y > clipy)) ||
            ((M.tpts[i].y < -clipy) && (M.tpts[i + 1].y < -clipy)))
          continue;

        x0 = M.tpts[i].x * cur_vp->zoom_amt + cur_vp->xmin + dx;
        y0 = -M.tpts[i].y * cur_vp->zoom_amt + cur_vp->ymin + dy;
        x1 = M.tpts[i + 1].x * cur_vp->zoom_amt + cur_vp->xmin + dx;
        y1 = -M.tpts[i + 1].y * cur_vp->zoom_amt + cur_vp->ymin + dy;

        xs0 = x0 + 0.5;
        ys0 = y0 + 0.5;
        xs1 = x1 + 0.5;
        ys1 = y1 + 0.5;

restart_clip2d:

        c0 = OutCode2D(cur_vport, xs0, ys0);
        c1 = OutCode2D(cur_vport, xs1, ys1);

        if ((c0 | c1) == 0)
        {
          DrawLine(xs0, ys0, xs1, ys1, color);
          break;
        }
        else if ((c0 & c1) != 0)
          break;
        else if (c0 != 0)
        {
          Clip2D(cur_vport, xs0, ys0, xs1, ys1, &xs2, &ys2, c0);
          xs0 = xs2;
          ys0 = ys2;
        }
        else
        {
          Clip2D(cur_vport, xs0, ys0, xs1, ys1, &xs2, &ys2, c1);
          xs1 = xs2;
          ys1 = ys2;
        }
        goto restart_clip2d;
        break;

      case WIREFRAME:
      case SOLID:
        /* Immediately throw out lines outside view frustum */

        if (OutsideFrustrum(M.tpts[i], M.tpts[i + 1]))
          continue;

        if (!cur_vp->fullbright)
        {
          if ((M.tpts[i].z > 1) && (M.tpts[i + 1].z > 1))
            continue;
        }

        ClipDrawLine3D(cur_vport,
                       M.tpts[i].x,
                       M.tpts[i].y,
                       M.tpts[i].z,
                       M.tpts[i + 1].x,
                       M.tpts[i + 1].y,
                       M.tpts[i + 1].z,
                       color);
        break;
    }
  }
}

static void
DrawViewport(matrix_t m)
{
  int i, j;
  int axis_x, axis_y;
  vec3_t origin;
  vsel_t* vs;
  brush_t* b;
  brushref_t* bref;
  fsel_t* fsel;

  entity_t* e;
  entityref_t* eref;

  plane_t* p;

  if ((status.edit_mode == ENTITY) || (status.edit_mode == MODEL))
  {
    for (e = M.EntityHead; e; e = e->Next)
    {
      e->center = (vec3_t){0, 0, 0};
      e->min = (vec3_t){10000, 10000, 10000};
      e->max = (vec3_t){-10000, -10000, -10000};
      e->nbrushes = 0;
    }
  }

  for (b = M.BrushHead; b; b = b->Next)
  {
    b->drawn = 0;

    if (!b->Group)
      continue;
    if ((b->Group->flags & 0x2))
      continue;
    if (M.showsel && b->hidden)
      continue;

    b->drawn = 1;

    e = b->EntityRef;
    if ((status.edit_mode == ENTITY) || (status.edit_mode == MODEL))
    {
      e->nbrushes++;

      for (i = 0; i < b->num_verts; i++)
      {
        if (b->verts[i].x < e->min.x)
          e->min.x = b->verts[i].x;
        if (b->verts[i].y < e->min.y)
          e->min.y = b->verts[i].y;
        if (b->verts[i].z < e->min.z)
          e->min.z = b->verts[i].z;
        if (b->verts[i].x > e->max.x)
          e->max.x = b->verts[i].x;
        if (b->verts[i].y > e->max.y)
          e->max.y = b->verts[i].y;
        if (b->verts[i].z > e->max.z)
          e->max.z = b->verts[i].z;
      }
    }

    for (i = 0; i < b->num_verts; i++)
    {
      /* Multiply vertices by transformation matrix*/
      Transform(b->tverts[i], b->verts[i], m);
      ToScreen(&b->sverts[i], b->tverts[i]);
    }
    /* Also transform brush centers, but not in entity mode*/
    if ((status.edit_mode == BRUSH) || (status.edit_mode == MODEL))
    {
      Transform(b->tcenter, b->center, m);
      ToScreen(&b->scenter, b->tcenter);
    }
    /* Do face centers if in Face mode*/
    if (status.edit_mode == FACE)
    {
      for (i = 0; i < b->num_planes; i++)
      {
        Transform(b->plane[i].tcenter, b->plane[i].center, m);
        ToScreen(&b->plane[i].scenter, b->plane[i].tcenter);
      }
    }
  }

  for (bref = M.display.bsel; bref; bref = bref->Next)
  {
    if (bref->Brush->drawn)
      bref->Brush->drawn = 2;
  }

  /* Transform all entities, but not in brush or face mode */
  if ((status.edit_mode == ENTITY) || (status.edit_mode == MODEL))
  {
    for (e = M.EntityHead; e; e = e->Next)
    {
      e->drawn = 0;

      if (!e->Group)
        continue;
      if ((e->Group->flags & 0x02))
        continue;
      if (M.showsel && e->hidden)
        continue;

      if (GetKeyValue(e, "origin") != NULL)
      {
        e->drawn = 1;
        sscanf(GetKeyValue(e, "origin"), "%f %f %f", &origin.x, &origin.y, &origin.z);
        Transform(e->trans, origin, m);
      }
      else
      {
        if (e->min.x == 10000)
          continue;
        e->drawn = 1;

        e->center.x = (e->min.x + e->max.x) / 2;
        e->center.y = (e->min.y + e->max.y) / 2;
        e->center.z = (e->min.z + e->max.z) / 2;

        e->min.x -= e->center.x;
        e->min.y -= e->center.y;
        e->min.z -= e->center.z;

        e->max.x -= e->center.x;
        e->max.y -= e->center.y;
        e->max.z -= e->center.z;

        if (e->nbrushes == 1)
        {
          e->min.x -= 4;
          e->min.y -= 4;
          e->min.z -= 4;

          e->max.x += 4;
          e->max.y += 4;
          e->max.z += 4;
        }

        Transform(e->trans, e->center, m);
      }
      if (!e->drawn)
        continue;

      ToScreen(&e->strans, e->trans);

      //         if (!ToScreen(&e->strans,e->trans))
      //            e->drawn=0;
    }

    for (eref = M.display.esel; eref; eref = eref->Next)
    {
      if (eref->Entity->drawn)
        eref->Entity->drawn = 2;
    }
  }

// Highlight all selected brushes.
#define DBrushSel()                                             \
  for (bref = M.display.bsel; bref; bref = bref->Next)          \
  {                                                             \
    if (bref->Brush->drawn)                                     \
      DrawBrush(cur_vport, bref->Brush, CR_BRUSH_SELECT, TRUE); \
  }

// Draw brush focal points.
#define DBrushFocal()                                       \
  for (b = M.BrushHead; b; b = b->Next)                     \
  {                                                         \
    if (!b->drawn)                                          \
      continue;                                             \
                                                            \
    switch (b->bt->type)                                    \
    {                                                       \
      case BR_SUBTRACT:                                     \
        DrawDot(cur_vport, b->scenter, CR_BRUSH_FOCAL_SUB); \
        break;                                              \
      default:                                              \
        DrawDot(cur_vport, b->scenter, CR_BRUSH_FOCAL);     \
        break;                                              \
    }                                                       \
  }

// Draw vertices of selected brushes.
#define DBrushSelVer()                                        \
  for (bref = M.display.bsel; bref; bref = bref->Next)        \
  {                                                           \
    for (j = 0; j < bref->Brush->num_verts; j++)              \
    {                                                         \
      DrawVertex(cur_vport, bref->Brush->sverts[j], CR_VERT); \
    }                                                         \
  }

// Draw selected vertices.
#define DVerSel()                                     \
  for (vs = M.display.vsel; vs; vs = vs->Next)        \
  {                                                   \
    DrawVertex(cur_vport, *(vs->svert), CR_VERT_SEL); \
  }

  /*
    This is where the real code starts.
  */
  switch (cur_vp->mode)
  {
    /* ------------------- */
    /* No Perspective Mode */
    /* ------------------- */
    case NOPERSP:
      if (cur_vp->grid_type == GRID)
        DrawGrids();

    /* -------------- */
    /* Wireframe Mode */
    /* -------------- */
    case WIREFRAME:

      for (b = M.BrushHead; b; b = b->Next)
      {
        if (b->drawn == 1)
          DrawBrush(cur_vport, b, GetColor((int)b->Group->color), FALSE);
      }

      switch (status.edit_mode)
      {
        case BRUSH:

          DBrushSel();

          DBrushFocal();

          DBrushSelVer();

          DVerSel();

          break;

        case FACE:
          /* Highlight all selected faces */
          for (fsel = M.display.fsel; fsel; fsel = fsel->Next)
          {
            DrawBrushFace(fsel->Brush, fsel->facenum, CR_FACE_SELECT, TRUE);
          }
          /* Draw face focal points */
          for (b = M.BrushHead; b; b = b->Next)
          {
            if (!b->drawn)
              continue;
            if (b->bt->type == BR_Q3_CURVE)
              continue;

            for (i = 0; i < b->num_planes; i++)
            {
              DrawDot(cur_vport, b->plane[i].scenter, CR_FACE_FOCAL);
            }
          }
          /* Highlight selected focal points */
          for (fsel = M.display.fsel; fsel; fsel = fsel->Next)
          {
            DrawDot(cur_vport, fsel->Brush->plane[fsel->facenum].scenter, CR_FACE_SELECT);
          }
          /* Draw vertices of selected faces */
          for (fsel = M.display.fsel; fsel; fsel = fsel->Next)
          {
            b = fsel->Brush;
            p = &fsel->Brush->plane[fsel->facenum];
            for (j = 0; j < p->num_verts; j++)
            {
              DrawVertex(cur_vport, b->sverts[p->verts[j]], CR_VERT);
            }
          }

          DVerSel();

          break;

        case MODEL:
          /* Draw all models */
          DrawAllModels(cur_vport);

          DrawAllLinks(cur_vport);

          DBrushSel();

          DBrushFocal();

        case ENTITY:

          for (e = M.EntityHead; e; e = e->Next)
          {
            if (e->drawn != 1)
              continue;

            DrawModel(cur_vport, e, m, 0);
          }

          for (eref = M.display.esel; eref; eref = eref->Next)
          {
            e = eref->Entity;

            if (!e->drawn)
              continue;

            DrawModel(cur_vport, e, m, 1);
          }

          break;
      }
      break;

    case SOLID:
      DrawPolyView(cur_vport);
      switch (status.edit_mode)
      {
        case BRUSH:
          DBrushSel();

          DBrushSelVer();

          DVerSel();

          break;

        case FACE:
          /* Highlight all selected faces */
          for (fsel = M.display.fsel; fsel; fsel = fsel->Next)
          {
            DrawBrushFace(fsel->Brush, fsel->facenum, CR_FACE_SELECT, TRUE);
          }
          /* Draw vertices of selected faces */
          for (fsel = M.display.fsel; fsel; fsel = fsel->Next)
          {
            b = fsel->Brush;
            p = &fsel->Brush->plane[fsel->facenum];
            for (j = 0; j < p->num_verts; j++)
            {
              DrawVertex(cur_vport, b->sverts[p->verts[j]], CR_VERT);
            }
          }

          DVerSel();

          break;

        case MODEL:
          DBrushSel();
          break;
      }
      break;
  }

  // Draw axises
  {
    vec3_t a1, a2;
    int i, col;
    scalar_t ax, ay;
    matrix_t m;
    int rx, ry, rz;

    axis_x = cur_vp->xmin + 20;
    axis_y = cur_vp->ymax - 20;

    col = 0;

    GetRotValues(cur_vport, &rx, &ry, &rz);
    GenerateRotMatrix(m, rx, ry, rz);

    for (i = 0; i < 3; i++)
    {
      a1.x = a1.y = a1.z = 0;
      switch (i)
      {
        case 0:
          a1.x = 1;
          break;
        case 1:
          a1.y = 1;
          break;
        case 2:
          a1.z = 1;
          break;
      }

      Transform(a2, a1, m);

      ax = a2.x * 15;
      ay = -a2.y * 15;

      switch (i)
      {
        case 0:
          col = CR_AXIS_X;
          break;
        case 1:
          col = CR_AXIS_Y;
          break;
        case 2:
          col = CR_AXIS_Z;
          break;
      }

      DrawLine(axis_x, axis_y, axis_x + ax, axis_y + ay, col);
    }
  }

  DrawPts(m);

  UpdateViewportBorder(cur_vport);
}

void
UpdateViewport(int vport, int refresh)
{
  matrix_t matrix;

  {
    int i;
    static int odith = -1;
    if (odith != dith)
    {
      odith = dith;
      if (dith)
        for (i = 0; i < C_MAX; i++)
          rcolors[i] = GetColor2(colors[i]);
      else
        for (i = 0; i < C_MAX; i++)
          rcolors[i] = colors[i];
    }
  }

  cur_vp = &M.display.vport[vport];
  cur_vport = vport;

  dx = (cur_vp->xmax - cur_vp->xmin - 1) >> 1;
  dy = (cur_vp->ymax - cur_vp->ymin - 1) >> 1;

  GenerateMatrix(matrix, vport);
  ClearViewport(vport);

  if (cur_vp->mode == BSPVIEW)
  {
    DrawBSPTree(matrix, vport);
  }
  else
  {
    DrawViewport(matrix);
  }

  if (refresh)
  {
    /*      { // TODO: clamp framerate in a smarter way
             WaitRetr();
          }*/
    RefreshPart(cur_vp->xmin, cur_vp->ymin, cur_vp->xmax, cur_vp->ymax);
  }
}

void
UpdateAllViewports(void)
{
  int i;

  /* Update active vport last */
  for (i = 0; i < M.display.num_vports; i++)
  {
    if (i == M.display.active_vport)
      continue;
    if (!M.display.vport[i].used)
      continue;
    UpdateViewport(i, FALSE);
  }
  UpdateViewport(M.display.active_vport, FALSE);
  /*   { // TODO: clamp framerate in a smarter way
        WaitRetr();
     }*/
  RefreshScreen();
}

void
RotateDisplay(int from, int to)
{
  int i, j;
  int sr[3];
  int er[3];
  float dr[3];
  float cr[3];
  int vport;
  viewport_t* v;

  vport = M.display.active_vport;
  v = &M.display.vport[vport];

  v->camera_dir = from;
  GetRotValues(vport, &sr[0], &sr[1], &sr[2]);

  v->camera_dir = to;
  GetRotValues(vport, &er[0], &er[1], &er[2]);

  if (status.turn_frames <= 0)
    status.turn_frames = 1;

  for (i = 0; i < 3; i++)
  {
    dr[i] = er[i] - sr[i];
    if (dr[i] > 180)
      dr[i] -= 360;
    if (dr[i] < -180)
      dr[i] += 360;

    dr[i] /= status.turn_frames;

    cr[i] = sr[i];
  }

  v->axis_aligned = 0;
  for (i = 0; i < status.turn_frames; i++)
  {
    v->rot_x = cr[0];
    v->rot_y = cr[1];
    v->rot_z = cr[2];
    UpdateViewport(vport, TRUE);
    for (j = 0; j < 3; j++)
      cr[j] += dr[j];
  }
  v->axis_aligned = 1;
}

void
Profile(int vport)
{
  int i;
  int old_vp;

  old_vp = M.display.active_vport;
  M.display.active_vport = vport;
  for (i = 0; i < 100; i++)
  {
    MoveCamera(M.display.active_vport, MOVE_FORWARD);
    UpdateViewport(M.display.active_vport, TRUE);
  }
  M.display.active_vport = old_vp;
}
