/*
editface.c file of the Quest Source Code

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

#include "editface.h"

#include "3d.h"
#include "brush.h"
#include "button.h"
#include "color.h"
#include "error.h"
#include "file.h"
#include "geom.h"
#include "keyboard.h"
#include "memory.h"
#include "mouse.h"
#include "popup.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "tex.h"
#include "texdef.h"
#include "texpick.h"
#include "video.h"

static float baseaxis[18][3] =
  {
    {0, 0, 1},
    {1, 0, 0},
    {0, -1, 0}, // floor
    {0, 0, -1},
    {1, 0, 0},
    {0, -1, 0}, // ceiling
    {1, 0, 0},
    {0, 1, 0},
    {0, 0, -1}, // west wall
    {-1, 0, 0},
    {0, 1, 0},
    {0, 0, -1}, // east wall
    {0, 1, 0},
    {1, 0, 0},
    {0, 0, -1}, // south wall
    {0, -1, 0},
    {1, 0, 0},
    {0, 0, -1} // north wall
};
static int flipped[6] = {0, 1, 0, 1, 1, 0};

static int
TextureAxisFromPlane(plane_t* pln, vec3_t* norm, vec3_t* xv, vec3_t* yv)
{
  int bestaxis;
  float dot, best;
  int i;

  best = 0;
  bestaxis = 0;

  for (i = 0; i < 6; i++)
  {
    dot = (pln->normal.x * baseaxis[i * 3][0]) +
          (pln->normal.y * baseaxis[i * 3][1]) +
          (pln->normal.z * baseaxis[i * 3][2]);
    if (dot - best > 0.01)
    {
      best = dot;
      bestaxis = i;
    }
  }
  norm->x = baseaxis[bestaxis * 3][0];
  norm->y = baseaxis[bestaxis * 3][1];
  norm->z = baseaxis[bestaxis * 3][2];

  xv->x = baseaxis[bestaxis * 3 + 1][0];
  xv->y = baseaxis[bestaxis * 3 + 1][1];
  xv->z = baseaxis[bestaxis * 3 + 1][2];
  yv->x = baseaxis[bestaxis * 3 + 2][0];
  yv->y = baseaxis[bestaxis * 3 + 2][1];
  yv->z = baseaxis[bestaxis * 3 + 2][2];

  return flipped[bestaxis];
}

typedef struct
{
  float x, y, z;
  float rx, ry, rz;
  int sx, sy;
  int tx, ty;
  int rtx, rty;
} texpt_t;

#define NEXT_L(x, n) (((x) == 0) ? ((n) - 1) : ((x) - 1))
#define NEXT_R(x, n) (((x) == ((n) - 1)) ? 0 : ((x) + 1))

/* Calculate texture vectors */
void
MakeVectors(float vecs[2][3], texdef_t* t)
{
  // rotate axis
  int sv, tv;
  float ang, sinv, cosv;
  float ns, nt;

  float rotate;
  float scale[2];
  int i, j;

  rotate = t->rotate;

  scale[0] = t->scale[0];
  scale[1] = t->scale[1];
  if (fabs(scale[0]) < 0.001)
    scale[0] = 1;
  if (fabs(scale[1]) < 0.001)
    scale[1] = 1;
  scale[0] = 1 / scale[0];
  scale[1] = 1 / scale[1];

  if (rotate == 0)
  {
    sinv = 0;
    cosv = 1;
  }
  else if (rotate == 90)
  {
    sinv = 1;
    cosv = 0;
  }
  else if (rotate == 180)
  {
    sinv = 0;
    cosv = -1;
  }
  else if (rotate == 270)
  {
    sinv = -1;
    cosv = 0;
  }
  else
  {
    ang = rotate / 180 * PI;
    sinv = sin(ang);
    cosv = cos(ang);
  }

  if (vecs[0][0])
    sv = 0;
  else if (vecs[0][1])
    sv = 1;
  else
    sv = 2;

  if (vecs[1][0])
    tv = 0;
  else if (vecs[1][1])
    tv = 1;
  else
    tv = 2;

  for (i = 0; i < 2; i++)
  {
    ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
    nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
    vecs[i][sv] = ns;
    vecs[i][tv] = nt;
  }

  for (i = 0; i < 2; i++)
    for (j = 0; j < 3; j++)
      vecs[i][j] *= scale[i];
}

static texpt_t pts[64];
static int num_pts;

static int twsx, twsy;
static vec3_t xv, yv;

static matrix_t facemain;

static void
DrawTexFacePoly(texture_t* texture)
{
  int y, lp, rp, nlp, nrp;
  float xl, xr;
  float dxl, dxr;
  float dx, dy;

  // trace texture along edges
  float txr, tyr;
  float tdxr, tdyr;
  float txl, tyl;
  float tdxl, tdyl;

  float tx, ty;
  float tdx, tdy;

  int min_y_pt, max_y_pt;
  int min_y_val, max_y_val;
  int scanlinelen;
  int i;

  unsigned char* vid_ptr;

  min_y_val = max_y_val = pts[0].sy;
  min_y_pt = max_y_pt = 0;

  for (i = 1; i < num_pts; i++)
  {
    if (pts[i].sy < min_y_val)
    {
      min_y_val = pts[i].sy;
      min_y_pt = i;
    }
    if (pts[i].sy > max_y_val)
    {
      max_y_val = pts[i].sy;
      max_y_pt = i;
    }
  }

  tdx = tdy = 0;
  for (i = num_pts + 1; i < num_pts + 4; i++)
  {
    dx = pts[i].sx - pts[num_pts].sx;
    dy = pts[i].sy - pts[num_pts].sy;

    if (dx && !dy)
    {
      tdx = (pts[i].tx - pts[num_pts].tx) / dx;
      tdy = (pts[i].ty - pts[num_pts].ty) / dx;
      break;
    }
  }
  if (i == num_pts + 4)
    return;

  if (min_y_val == max_y_val)
    return;

  y = min_y_val;
  xl = xr = pts[min_y_pt].sx;
  lp = rp = min_y_pt;

  /* Left edge */
  while (y >= pts[NEXT_L(lp, num_pts)].sy)
    lp = NEXT_L(lp, num_pts);
  nlp = NEXT_L(lp, num_pts);
  xl = pts[lp].sx;
  dx = pts[nlp].sx - xl;
  dy = pts[nlp].sy - y;
  dxl = dx / dy;

#define UpdTX(side)                   \
  tx##side = pts[side##p].tx;         \
  dx = pts[n##side##p].tx - tx##side; \
  tdx##side = dx / dy;                \
  ty##side = pts[side##p].ty;         \
  dx = pts[n##side##p].ty - ty##side; \
  tdy##side = dx / dy;

  UpdTX(l);

  /* Right edge */
  while (y >= pts[NEXT_R(rp, num_pts)].sy)
    rp = NEXT_R(rp, num_pts);
  nrp = NEXT_R(rp, num_pts);
  xr = pts[rp].sx;
  dx = pts[nrp].sx - xr;
  dy = pts[nrp].sy - y;
  dxr = dx / dy;

  UpdTX(r);

  while (y < max_y_val)
  {
    /* Check left edge */
    while (y >= pts[NEXT_L(lp, num_pts)].sy)
    {
      lp = NEXT_L(lp, num_pts);
      nlp = NEXT_L(lp, num_pts);
      xl = pts[lp].sx;
      dx = pts[nlp].sx - xl;
      dy = pts[nlp].sy - y;
      dxl = dx / dy;

      UpdTX(l);
    }

    /* Check right edge */
    while (y >= pts[NEXT_R(rp, num_pts)].sy)
    {
      rp = NEXT_R(rp, num_pts);
      nrp = NEXT_R(rp, num_pts);
      xr = pts[rp].sx;
      dx = pts[nrp].sx - xr;
      dy = pts[nrp].sy - y;
      dxr = dx / dy;

      UpdTX(r);
    }

    scanlinelen = xr - xl;
    if (!scanlinelen)
      goto nodraw;

    vid_ptr = &video.ScreenBuffer[y * video.ScreenWidth + (int)xl];

    tx = txl;
    ty = tyl;

    for (i = 0; i < scanlinelen; i++)
    {
      while (tx < 0)
        tx += texture->dsx;
      while (tx >= texture->dsx)
        tx -= texture->dsx;
      while (ty < 0)
        ty += texture->dsy;
      while (ty >= texture->dsy)
        ty -= texture->dsy;

      *vid_ptr = texture->data[(int)(ty)*texture->dsx + (int)(tx)];
      vid_ptr++;

      tx += tdx;
      ty += tdy;
    }

nodraw:
    xl += dxl;
    xr += dxr;

    txl += tdxl;
    tyl += tdyl;
    txr += tdxr;
    tyr += tdyr;

    y++;
  }

  for (i = 0; i < num_pts; i++)
  {
    pts[i].rtx %= texture->rsx;
    pts[i].rty %= texture->rsy;
    if (pts[i].rtx < 0)
      pts[i].rtx += texture->rsx;
    if (pts[i].rty < 0)
      pts[i].rty += texture->rsy;

    QUI_DrawStr(pts[i].sx, pts[i].sy, -1, 15, 0, 0, "%i,%i", pts[i].rtx, pts[i].rty);
  }
  /*   for (;i<num_pts+4;i++)
     {
        pts[i].tx%=texture->sx;
        pts[i].ty%=texture->sy;
        if (pts[i].tx<0) pts[i].tx+=texture->sx;
        if (pts[i].ty<0) pts[i].ty+=texture->sy;

        QUI_DrawStr(pts[i].sx,pts[i].sy,-1,COL_RED,0,0,
           "%i: %i,%i",i-num_pts,pts[i].tx,pts[i].ty);
     }*/
}

static void
DrawFacePoly(QUI_window_t* w, texdef_t* tex, texture_t* texture)
{
  float shift[2];
  float vecs[2][3];

  int i;

  // calculate exact texture coordinates
  shift[0] = tex->shift[0];
  shift[1] = tex->shift[1];

  vecs[0][0] = xv.x;
  vecs[0][1] = xv.y;
  vecs[0][2] = xv.z;
  vecs[1][0] = yv.x;
  vecs[1][1] = yv.y;
  vecs[1][2] = yv.z;
  MakeVectors(vecs, tex);

  for (i = 0; i < num_pts + 4; i++)
  {
    pts[i].rtx =
      pts[i].x * vecs[0][0] +
      pts[i].y * vecs[0][1] +
      pts[i].z * vecs[0][2] +
      shift[0];

    pts[i].rty =
      pts[i].x * vecs[1][0] +
      pts[i].y * vecs[1][1] +
      pts[i].z * vecs[1][2] +
      shift[1];

    pts[i].tx = pts[i].rtx * texture->dsx / (float)texture->rsx;
    pts[i].ty = pts[i].rty * texture->dsy / (float)texture->rsy;
  }

  DrawTexFacePoly(texture);
}

static void
DrawFaceWire(fsel_t* fs)
{
  brush_t* b;
  plane_t* p;
  int x[64];
  int y[64];
  int i, n;
  vec3_t* v;

  b = fs->Brush;
  p = &b->plane[fs->facenum];

  for (i = 0; i < p->num_verts; i++)
  {
    v = &b->verts[p->verts[i]];
    x[i] = facemain[0][0] * v->x +
           facemain[0][1] * v->y +
           facemain[0][2] * v->z +
           facemain[0][3];
    y[i] = facemain[1][0] * v->x +
           facemain[1][1] * v->y +
           facemain[1][2] * v->z +
           facemain[1][3];
  }

  for (i = 0; i < p->num_verts; i++)
  {
    n = i + 1;
    if (n == p->num_verts)
      n = 0;
    DrawLine(x[i], y[i], x[n], y[n], GetColor2(15));
  }
}

static void
RedrawFacePopup(QUI_window_t* w, texdef_t* tex, texture_t* texture)
{
  int i;
  fsel_t* fs;

  /* Erase old area */
  for (i = w->pos.y + 25; i < (w->pos.y + w->size.y - 3); i++)
    DrawLine(w->pos.x + 3, i, w->pos.x + w->size.x - 3, i, GetColor2(BG_COLOR));

  /* Draw texture border lines */
  QUI_Box(w->pos.x + 14, w->pos.y + 39, w->pos.x + 16 + twsx, w->pos.y + 41 + twsy, 4, 8);

  /* Draw Strings in the right side of window */
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 32, BG_COLOR, 0, 0, 0, "X Offset");
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 72, BG_COLOR, 0, 0, 0, "Y Offset");
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 112, BG_COLOR, 0, 0, 0, "X Scale");
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 152, BG_COLOR, 0, 0, 0, "Y Scale");
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 192, BG_COLOR, 0, 0, 0, "Angle");

  /* Draw data areas */
  for (i = 0; i < 5; i++)
  {
    QUI_Box(w->pos.x + twsx + 50, w->pos.y + 48 + i * 40, w->pos.x + twsx + 133, w->pos.y + 66 + i * 40, 4, 8);
  }

  /* Draw data */
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 50, BG_COLOR, 14, 0, 0, "%0.0f", tex->shift[0]);
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 90, BG_COLOR, 14, 0, 0, "%0.0f", tex->shift[1]);
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 130, BG_COLOR, 14, 0, 0, "%2.3f", tex->scale[0]);
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 170, BG_COLOR, 14, 0, 0, "%2.3f", tex->scale[1]);
  QUI_DrawStr(w->pos.x + twsx + 55, w->pos.y + 210, BG_COLOR, 14, 0, 0, "%0.0f", tex->rotate);

  /* Draw buttons */
  DrawButtons();

  DrawFacePoly(w, tex, texture);

  for (fs = M.display.fsel; fs; fs = fs->Next)
    DrawFaceWire(fs);

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
}

// automatically align a texture on a face???
static void
AutoAlign(texdef_t* tex, texture_t* t, int xyb)
{
  vec3_t min;
  vec3_t pt;
  int i;

  float vecs[2][3];

  pt.x = pts[0].x;
  pt.y = pts[0].y;
  pt.z = pts[0].z;
  min.x = pts[0].sx;
  min.y = pts[0].sy;
  for (i = 1; i < num_pts; i++)
  {
    if ((min.y > pts[i].sy) || ((min.y == pts[i].sy) && (min.x > pts[i].sx)))
    {
      pt.x = pts[i].x;
      pt.y = pts[i].y;
      pt.z = pts[i].z;
      min.x = pts[i].sx;
      min.y = pts[i].sy;
    }
  }

  vecs[0][0] = xv.x;
  vecs[0][1] = xv.y;
  vecs[0][2] = xv.z;
  vecs[1][0] = yv.x;
  vecs[1][1] = yv.y;
  vecs[1][2] = yv.z;
  MakeVectors(vecs, tex);

  if (xyb & 1)
  {
    tex->shift[0] = -(vecs[0][0] * pt.x +
                      vecs[0][1] * pt.y +
                      vecs[0][2] * pt.z);

    tex->shift[0] -= t->rsx * (int)(tex->shift[0] / (float)t->rsx);
    if (tex->shift[0] < 0)
      tex->shift[0] += t->rsx;
  }

  if (xyb & 2)
  {
    tex->shift[1] = -(vecs[1][0] * pt.x +
                      vecs[1][1] * pt.y +
                      vecs[1][2] * pt.z);

    tex->shift[1] -= t->rsy * (int)(tex->shift[1] / (float)t->rsy);
    if (tex->shift[1] < 0)
      tex->shift[1] += t->rsy;
  }
}

static void
AutoScale(texdef_t* tex, texture_t* t, int xyb)
{
  vec3_t min, max;
  vec3_t p[3];
  vec3_t pt[3];
  float tmp;
  int i;

  float vecs[2][3];

  min.x = min.y = 10000;
  max.x = max.y = -10000;
  for (i = 0; i < num_pts; i++)
  {
    tmp = pts[i].x * xv.x + pts[i].y * xv.y + pts[i].z * xv.z;
    if (tmp < min.x)
      min.x = tmp;
    if (tmp > max.x)
      max.x = tmp;

    tmp = pts[i].x * yv.x + pts[i].y * yv.y + pts[i].z * yv.z;
    if (tmp < min.y)
      min.y = tmp;
    if (tmp > max.y)
      max.y = tmp;
  }

  p[0].x = min.x * xv.x + min.y * yv.x;
  p[0].y = min.x * xv.y + min.y * yv.y;
  p[0].z = min.x * xv.z + min.y * yv.z;

  p[1].x = max.x * xv.x + min.y * yv.x;
  p[1].y = max.x * xv.y + min.y * yv.y;
  p[1].z = max.x * xv.z + min.y * yv.z;

  p[2].x = min.x * xv.x + max.y * yv.x;
  p[2].y = min.x * xv.y + max.y * yv.y;
  p[2].z = min.x * xv.z + max.y * yv.z;

  vecs[0][0] = xv.x;
  vecs[0][1] = xv.y;
  vecs[0][2] = xv.z;
  vecs[1][0] = yv.x;
  vecs[1][1] = yv.y;
  vecs[1][2] = yv.z;

  for (i = 0; i < 3; i++)
  {
    pt[i].x = vecs[0][0] * p[i].x +
              vecs[0][1] * p[i].y +
              vecs[0][2] * p[i].z;
    pt[i].y = vecs[1][0] * p[i].x +
              vecs[1][1] * p[i].y +
              vecs[1][2] * p[i].z;
  }

  tex->rotate = 0;

  if (xyb & 1)
  {
    tex->scale[0] = fabs((pt[1].x - pt[0].x) / (float)t->rsx);
    tex->shift[0] = 0;
  }
  if (xyb & 2)
  {
    tex->scale[1] = fabs((pt[2].y - pt[0].y) / (float)t->rsy);
    tex->shift[1] = 0;
  }
}

static int
GetXYB(void)
{
  int i;
  Popup_Init(mouse.x, mouse.y);
  Popup_AddStr("X");
  Popup_AddStr("Y");
  Popup_AddStr("Both");
  i = Popup_Display();
  Popup_Free();
  return i + 1;
}

static void EditFace_Curve(void);

int
EditFace(void)
{
  int b_ok;
  int b_cancel;
  int b_reset;
  int b_tex;
  int b_next, b_prev;
  int b_align, b_scale, b_flip;
  int bp;

  unsigned char* temp_buf;
  QUI_window_t* w;

  texture_t* texture;
  char new_tex_name[256];

  fsel_t* f;
  plane_t* p;

  texdef_t tex;

  int exit_flag;
  int many;

  int i;

start:
  if (M.display.num_bselected == 1 &&
      M.display.bsel->Brush->bt->type == BR_Q3_CURVE)
  {
    EditFace_Curve();
    return 1;
  }

  if (!M.display.num_fselected)
  {
    HandleError("EditFace", "At least one face must be selected!");
    return 0;
  }

  f = M.display.fsel;

  if (M.display.num_fselected != 1)
  {
    fsel_t* fs;
    plane_t* p1;
    vec3_t normal;
    vec3_t xv, yv;
    vec3_t min, max;
    brush_t* b;
    float x, y;
    int flip;

    many = 1;

    p = &f->Brush->plane[f->facenum];

    for (fs = M.display.fsel->Next; fs; fs = fs->Next)
    {
      if (fs->Brush->bt->type != BR_NORMAL)
      {
        HandleError("EditFace", "All faces must be normal if editing more than one");
        return 0;
      }

      p1 = &fs->Brush->plane[fs->facenum];
      if ((fabs(p1->normal.x - p->normal.x) > 0.01) ||
          (fabs(p1->normal.y - p->normal.y) > 0.01) ||
          (fabs(p1->normal.z - p->normal.z) > 0.01))
      {
        HandleError("EditFace", "All faces must be in the same plane!");
        return 0;
      }
    }

    num_pts = 4;
    flip = TextureAxisFromPlane(p, &normal, &xv, &yv);
    min.x = min.y = min.z = 10000;
    max.x = max.y = max.z = -10000;
    for (fs = M.display.fsel; fs; fs = fs->Next)
    {
      p1 = &fs->Brush->plane[fs->facenum];
      b = fs->Brush;

      for (i = 0; i < p1->num_verts; i++)
      {
        x = DotProd(b->verts[p1->verts[i]], xv);
        if (x > max.x)
          max.x = x;
        if (x < min.x)
          min.x = x;

        y = DotProd(b->verts[p1->verts[i]], yv);
        if (y > max.y)
          max.y = y;
        if (y < min.y)
          min.y = y;
      }
    }

    pts[0].x = xv.x * min.x + yv.x * min.y;
    pts[0].y = xv.y * min.x + yv.y * min.y;
    pts[0].z = xv.z * min.x + yv.z * min.y;

    pts[1].x = xv.x * min.x + yv.x * max.y;
    pts[1].y = xv.y * min.x + yv.y * max.y;
    pts[1].z = xv.z * min.x + yv.z * max.y;

    pts[2].x = xv.x * max.x + yv.x * max.y;
    pts[2].y = xv.y * max.x + yv.y * max.y;
    pts[2].z = xv.z * max.x + yv.z * max.y;

    pts[3].x = xv.x * max.x + yv.x * min.y;
    pts[3].y = xv.y * max.x + yv.y * min.y;
    pts[3].z = xv.z * max.x + yv.z * min.y;

    if (!flip)
    {
      texpt_t temp;

      temp = pts[3];
      pts[3] = pts[1];
      pts[1] = temp;
    }
  }
  else
  {
    brush_t* b;

    many = 0;

    b = f->Brush;
    p = &b->plane[f->facenum];

    num_pts = p->num_verts;
    for (i = 0; i < num_pts; i++)
    {
      pts[i].x = b->verts[p->verts[i]].x;
      pts[i].y = b->verts[p->verts[i]].y;
      pts[i].z = b->verts[p->verts[i]].z;
    }
  }

  tex = p->tex;

  texture = ReadMIPTex(tex.name, 0);
  if (!texture)
  {
    HandleError("EditFace", "Can't load texture!");
    return FALSE;
  }

  SetPal(PAL_TEXTURE);

  PushButtons();
  b_ok = AddButtonText(0, 0, 0, "Ok");
  b_cancel = AddButtonText(0, 0, 0, "Cancel");
  b_reset = AddButtonText(0, 0, 0, "Reset");

  b_tex = AddButtonText(0, 0, 0, texture->name);

  if (!many)
  {
    b_next = AddButtonText(0, 0, 0, "Next");
    b_prev = AddButtonText(0, 0, 0, "Prev");
  }
  else
  {
    b_next = b_prev = -1;
  }

  b_align = AddButtonText(0, 0, B_RAPID, "Align");
  b_scale = AddButtonText(0, 0, B_RAPID, "Scale");
  b_flip = AddButtonText(0, 0, B_RAPID, "Flip");

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];

  {
    matrix_t trans, rot;
    vec3_t temp;
    lvec3_t min, max, size;
    vec3_t normal;
    vec3_t norm;

    vec3_t fmin, fmax;

    float scale;

    fmax.x = fmin.x = pts[0].x;
    fmax.y = fmin.y = pts[0].y;
    fmax.z = fmin.z = pts[0].z;
    for (i = 1; i < num_pts; i++)
    {
#define ChkVec(a)        \
  if (pts[i].a < fmin.a) \
    fmin.a = pts[i].a;   \
  if (pts[i].a > fmax.a) \
    fmax.a = pts[i].a;
      ChkVec(x);
      ChkVec(y);
      ChkVec(z);
    }

    num_pts += 4;
    for (; i < num_pts; i++)
    {
      pts[i].x = fmin.x;
      pts[i].y = fmin.y;
      pts[i].z = fmin.z;
    }
    pts[i - 1].x = fmax.x;
    pts[i - 2].y = fmax.y;
    pts[i - 3].z = fmax.z;

    TextureAxisFromPlane(p, &normal, &xv, &yv);

    /* Transform points for straight-on view */
    InitMatrix(facemain);

    InitMatrix(trans);
    trans[0][3] = -p->center.x;
    trans[1][3] = -p->center.y;
    trans[2][3] = -p->center.z;

    InitMatrix(rot);
    normal.x = -normal.x;
    normal.y = -normal.y;
    normal.z = -normal.z;

    rot[2][0] = normal.x;
    rot[2][1] = normal.y;
    rot[2][2] = normal.z;

    if (fabs(normal.z) < 0.001)
    {
      temp.x = 0;
      temp.y = 0;
      temp.z = 1;
    }
    else
    {
      temp.x = 0;
      temp.y = 1;
      temp.z = 0;
    }
    rot[1][0] = temp.x;
    rot[1][1] = temp.y;
    rot[1][2] = temp.z;

    CrossProd(normal, temp, &norm);

    rot[0][0] = norm.x;
    rot[0][1] = norm.y;
    rot[0][2] = norm.z;

    MultMatrix(rot, trans, facemain);

    for (i = 0; i < 4; i++)
      facemain[1][i] = -facemain[1][i];

    /* Rotate points by matrix */
    for (i = 0; i < num_pts; i++)
    {
      pts[i].rx = ((facemain[0][0] * pts[i].x) +
                   (facemain[0][1] * pts[i].y) +
                   (facemain[0][2] * pts[i].z) +
                   facemain[0][3]);
      pts[i].ry = ((facemain[1][0] * pts[i].x) +
                   (facemain[1][1] * pts[i].y) +
                   (facemain[1][2] * pts[i].z) +
                   facemain[1][3]);
    }

    for (i = 0; i < num_pts; i++)
    {
      pts[i].sx = (int)pts[i].rx;
      pts[i].sy = (int)pts[i].ry;
    }

    /* Find min and max values */
    min.x = max.x = pts[0].sx;
    min.y = max.y = pts[0].sy;
    for (i = 1; i < num_pts; i++)
    {
      if (pts[i].sx < min.x)
        min.x = pts[i].sx;
      if (pts[i].sy < min.y)
        min.y = pts[i].sy;
      if (pts[i].sx > max.x)
        max.x = pts[i].sx;
      if (pts[i].sy > max.y)
        max.y = pts[i].sy;
    }

    /* Adjust so min is 0,0 */
    for (i = 0; i < num_pts; i++)
    {
      pts[i].sx -= min.x;
      pts[i].sy -= min.y;
    }

    scale = 2;
    size.x = (max.x - min.x) * scale;
    size.y = (max.y - min.y) * scale;

    /* Scale */
    twsx = size.x;
    twsy = size.y;

    if (twsx > video.ScreenWidth - 160 - 20)
      twsx = video.ScreenWidth - 160 - 20;

    if (twsx < 250)
      twsx = 300;

    if (twsy > video.ScreenHeight - 150 - 20)
      twsy = video.ScreenHeight - 150 - 20;

    if (twsy < 150)
      twsy = 200;

    if ((size.x > twsx) || (size.y > twsy))
    {
      if ((twsx / (float)size.x) > (twsy / (float)size.y))
        scale *= twsy / (float)size.y;
      else
        scale *= twsx / (float)size.x;
    }

    if ((size.x < twsx) && (size.y < twsy))
    {
      if ((twsx / (float)size.x) > (twsy / (float)size.y))
        scale *= twsy / (float)size.y;
      else
        scale *= twsx / (float)size.x;
    }

    for (i = 0; i < num_pts; i++)
    {
      pts[i].sx *= scale;
      pts[i].sy *= scale;
    }

    facemain[0][3] -= min.x;
    facemain[1][3] -= min.y;

    for (i = 0; i < 4; i++)
    {
      facemain[0][i] = facemain[0][i] *= scale;
      facemain[1][i] = facemain[1][i] *= scale;
    }

    max.x -= min.x;
    max.y -= min.y;
    max.x *= scale;
    max.y *= scale;

    min.x = (twsx - max.x) / 2;
    min.y = (twsy - max.y) / 2;

    facemain[0][3] += min.x;
    facemain[1][3] += min.y;

    for (i = 0; i < num_pts; i++)
    {
      pts[i].sx += min.x;
      pts[i].sy += min.y;
    }

    num_pts -= 4;
  }

  w->size.x = twsx + 160;
  w->size.y = twsy + 150;

  w->pos.x = 10;
  w->pos.y = video.ScreenHeight - 10 - w->size.y;

  /* Adjust to window */
  for (i = 0; i < num_pts + 4; i++)
  {
    pts[i].sx += w->pos.x + 15;
    pts[i].sy += w->pos.y + 40;
  }
  facemain[0][3] += w->pos.x + 15;
  facemain[1][3] += w->pos.y + 40;

  button[b_ok].x = w->pos.x + 8;
  button[b_cancel].x = button[b_ok].x + 4 + button[b_ok].sx;
  button[b_reset].x = button[b_cancel].x + 4 + button[b_cancel].sx;

  button[b_ok].y = button[b_cancel].y = button[b_reset].y =
    w->pos.y + w->size.y - button[b_ok].sy - 5;

  button[b_align].x = w->pos.x + 8;
  button[b_scale].x = button[b_align].x + 4 + button[b_align].sx;
  button[b_flip].x = button[b_scale].x + 4 + button[b_scale].sx;

  button[b_align].y = button[b_scale].y = button[b_flip].y =
    button[b_ok].y - 5 - button[b_align].sy;

  if (!many)
  {
    button[b_next].x = button[b_prev].x =
      w->pos.x + w->size.x - button[b_next].sx - 6;

    button[b_next].y = button[b_align].y;
    button[b_prev].y = button[b_ok].y;
  }

  button[b_tex].x = w->pos.x + 8;
  button[b_tex].y = w->pos.y + twsy + 65;

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Edit Face", &temp_buf);
  Q.num_popups++;

#define REDRAW() RedrawFacePopup(w, &tex, texture)
  REDRAW();

  /* Wait for mouse button to be released */
  while (mouse.button != 0)
    UpdateMouse();
  ClearKeys();

  /* Main wait loop */
  exit_flag = FALSE;
  while (!exit_flag)
  {
    if (TestKey(KEY_CONTROL))
    {
      if (TestKey(KEY_LEFT))
      {
        while (TestKey(KEY_LEFT))
          ;
        tex.scale[0] -= 0.05;
        REDRAW();
      }
      if (TestKey(KEY_RIGHT))
      {
        while (TestKey(KEY_RIGHT))
          ;
        tex.scale[0] += 0.05;
        REDRAW();
      }
      if (TestKey(KEY_UP))
      {
        while (TestKey(KEY_UP))
          ;
        tex.scale[1] += 0.05;
        REDRAW();
      }
      if (TestKey(KEY_DOWN))
      {
        while (TestKey(KEY_DOWN))
          ;
        tex.scale[1] -= 0.05;
        REDRAW();
      }
    }
    else
    {
      if (TestKey(KEY_ALT))
      {
        if (TestKey(KEY_LEFT))
        {
          while (TestKey(KEY_LEFT))
            ;
          tex.rotate -= status.angle_snap_size;
          REDRAW();
        }
        if (TestKey(KEY_RIGHT))
        {
          while (TestKey(KEY_RIGHT))
            ;
          tex.rotate += status.angle_snap_size;
          REDRAW();
        }
      }
      else
      {
        if (TestKey(KEY_LEFT))
        {
          while (TestKey(KEY_LEFT))
            ;
          tex.shift[0] += status.snap_size;
          REDRAW();
        }
        if (TestKey(KEY_RIGHT))
        {
          while (TestKey(KEY_RIGHT))
            ;
          tex.shift[0] -= status.snap_size;
          REDRAW();
        }
        if (TestKey(KEY_UP))
        {
          while (TestKey(KEY_UP))
            ;
          tex.shift[1] += status.snap_size;
          REDRAW();
        }
        if (TestKey(KEY_DOWN))
        {
          while (TestKey(KEY_DOWN))
            ;
          tex.shift[1] -= status.snap_size;
          REDRAW();
        }
      }
    }
    if (TestKey(KEY_ESCAPE))
    {
      while (TestKey(KEY_ESCAPE))
        UpdateMouse();
      exit_flag = TRUE;
    }
    if (TestKey(KEY_ENTER))
    {
      fsel_t* fs;

      while (TestKey(KEY_ENTER))
        UpdateMouse();

      exit_flag = TRUE;

      for (fs = M.display.fsel; fs; fs = fs->Next)
        fs->Brush->plane[fs->facenum].tex = tex;
    }

    UpdateMouse();
    bp = UpdateButtons();

    if (bp != -1)
    {
      if ((bp == b_ok) || (((bp == b_next) || (bp == b_prev)) && !many))
      {
        fsel_t* fs;

        exit_flag = TRUE;

        for (fs = M.display.fsel; fs; fs = fs->Next)
          fs->Brush->plane[fs->facenum].tex = tex;
      }
      if (bp == b_cancel)
      {
        exit_flag = TRUE;
      }
      if (bp == b_tex)
      {
        SetPal(PAL_QUEST);

        TexturePicker(tex.name, new_tex_name);
        if (new_tex_name[0] != '\0')
        {
          RemoveButton(b_tex);

          SetTexture(&tex, new_tex_name);

          b_tex = AddButtonText(w->pos.x + 8, w->pos.y + twsy + 65, 0, texture->name);
        }
        texture = ReadMIPTex(tex.name, 1);
        SetPal(PAL_TEXTURE);
        REDRAW();
      }

      if (bp == b_align)
      {
        i = GetXYB();
        if (i)
        {
          AutoAlign(&tex, texture, i);
          REDRAW();
        }
      }
      if (bp == b_scale)
      {
        i = GetXYB();
        if (i)
        {
          AutoScale(&tex, texture, i);
          REDRAW();
        }
      }

      if (bp == b_flip)
      {
        i = GetXYB();
        if (i)
        {
          if (i & 1)
            tex.scale[0] = -tex.scale[0];
          if (i & 2)
            tex.scale[1] = -tex.scale[1];
          REDRAW();
        }
      }

      if (bp == b_reset)
      {
        tex.shift[0] = tex.shift[1] = 0;
        tex.scale[0] = tex.scale[1] = 1;
        tex.rotate = 0;

        REDRAW();
      }
    }
    else
    {
      if (mouse.button == 1)
      {
        for (i = 0; i < 5; i++)
        {
          if (InBox(w->pos.x + twsx + 50, w->pos.y + 48 + i * 40, w->pos.x + twsx + 133, w->pos.y + 66 + i * 40))
            break;
        }
        if (i < 5)
        {
          float* f;
          char buf[128];

          switch (i)
          {
            case 0:
            case 1:
              f = &tex.shift[i];
              sprintf(buf, "%0.0f", *f);
              break;
            case 2:
            case 3:
              f = &tex.scale[i - 2];
              sprintf(buf, "%2.3f", *f);
              break;
            default: // Shouldn't happen.
            case 4:
              f = &tex.rotate;
              sprintf(buf, "%0.0f", *f);
              break;
          }
          if (readstring(buf, w->pos.x + twsx + 55, w->pos.y + 50 + i * 40, w->pos.x + twsx + 133, sizeof(buf), NULL))
          {
            *f = atof(buf);
          }
          REDRAW();
          while (TestKey(KEY_ENTER) || TestKey(KEY_ESCAPE))
            UpdateMouse();
        }
      }
    }
  }

  /* Pop down the window */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

  /* Removes of the buttons */
  RemoveButton(b_ok);
  RemoveButton(b_cancel);
  RemoveButton(b_reset);
  RemoveButton(b_tex);
  RemoveButton(b_align);
  RemoveButton(b_scale);
  RemoveButton(b_flip);
  if (!many)
  {
    RemoveButton(b_next);
    RemoveButton(b_prev);
  }
  PopButtons();

  SetPal(PAL_QUEST);

  /* Refresh the correct portion of the screen */
  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
  DrawMouse(mouse.x, mouse.y);

  SetPal(PAL_QUEST);

  if (!many)
  {
    if ((bp == b_next) || (bp == b_prev))
    {
      brush_t* b;

      b = f->Brush;

      if (bp == b_next)
      {
        f->facenum++;
        if (f->facenum >= b->num_planes)
          f->facenum = 0;
      }
      else
      {
        f->facenum--;
        if (f->facenum < 0)
          f->facenum = b->num_planes - 1;
      }
      UpdateAllViewports();
      goto start;
    }
  }

  return TRUE;
}

static void
Curve_FitEven(brush_t* b)
{
  float *dist, *d;
  float x, y, z, v, e, f;
  int nx, ny;
  int i, j, k;

  nx = b->x.q3c.sizex;
  ny = b->x.q3c.sizey;

  if (nx > ny)
    dist = Q_malloc(sizeof(float) * nx);
  else
    dist = Q_malloc(sizeof(float) * ny);

  if (!dist)
  {
    Q_free(dist);
    HandleError("Curve_FitEven", "Out of memory!");
    return;
  }

  for (i = 0; i < ny; i++)
  {
    b->x.q3c.s[i * nx] = b->x.q3c.s[0];
    b->x.q3c.s[i * nx + nx - 1] = b->x.q3c.s[ny * nx - 1];
  }
  e = b->x.q3c.s[nx - 1] - b->x.q3c.s[0];

  for (i = 0; i < ny; i++)
  {
    f = 0;
    k = i * nx + 1;
    d = dist;
    for (j = 1; j < nx; j++, d++, k++)
    {
      x = (b->verts[k - 1].x - b->verts[k].x);
      y = (b->verts[k - 1].y - b->verts[k].y);
      z = (b->verts[k - 1].z - b->verts[k].z);
      v = x * x + y * y + z * z;
      if (v <= 0)
        v = 0;
      else
        v = sqrt(v);

      *d = v;
      f += v;
    }

    for (j = 1, v = dist[0]; j < nx; v += dist[j], j++)
    {
      b->x.q3c.s[i * nx + j] = b->x.q3c.s[0] + e * v / f;
    }
  }

  for (i = 0; i < nx; i++)
  {
    b->x.q3c.t[i] = b->x.q3c.t[0];
    b->x.q3c.t[i + nx * (ny - 1)] = b->x.q3c.t[ny * nx - 1];
  }
  e = b->x.q3c.t[nx * (ny - 1)] - b->x.q3c.t[0];

  for (i = 0; i < nx; i++)
  {
    f = 0;
    k = i + nx;
    d = dist;
    for (j = 1; j < ny; j++, d++, k += nx)
    {
      x = (b->verts[k - nx].x - b->verts[k].x);
      y = (b->verts[k - nx].y - b->verts[k].y);
      z = (b->verts[k - nx].z - b->verts[k].z);
      v = x * x + y * y + z * z;
      if (v <= 0)
        v = 0;
      else
        v = sqrt(v);

      *d = v;
      f += v;
    }

    for (j = 1, v = dist[0]; j < ny; v += dist[j], j++)
    {
      b->x.q3c.t[i + nx * j] = b->x.q3c.t[0] + e * v / f;
    }
  }

  Q_free(dist);
}

static void
EditFace_Curve(void)
{
  QUI_window_t* w;
  int b_ok, b_reset, b_fit, b_natural, b_even;
  int bp;
  unsigned char* temp_buf;

  brush_t* b;

  int *x1, *y1, *x2, *y2;
  int nx, ny;
  int redraw;

  int i, j, k;

  char buf[64];

  texture_t* tex;
  int tsx, tsy;

  if (M.display.num_bselected != 1)
  {
    HandleError("EditFace_Curve", "Exactly one curve must be selected!");
    return;
  }

  b = M.display.bsel->Brush;

  nx = b->x.q3c.sizex;
  ny = b->x.q3c.sizey;

  x1 = Q_malloc(sizeof(int) * nx * ny);
  x2 = Q_malloc(sizeof(int) * nx * ny);
  y1 = Q_malloc(sizeof(int) * nx * ny);
  y2 = Q_malloc(sizeof(int) * nx * ny);

  if (!x1 || !x2 || !y1 || !y2)
  {
    Q_free(x1);
    Q_free(x2);
    Q_free(y1);
    Q_free(y2);
    HandleError("EditFace_Curve", "Out of memory!");
    return;
  }

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];
  w->size.x = 32 + nx * 64;
  if (w->size.x < 352)
    w->size.x = 352;
  w->size.y = 64 + ny * 48;
  w->pos.x = (video.ScreenWidth - w->size.x) / 2;
  w->pos.y = (video.ScreenHeight - w->size.y) / 2;

  PushButtons();
  b_ok = AddButtonText(0, 0, 0, "OK");
  MoveButton(b_ok, w->pos.x + 8, w->pos.y + w->size.y - 30);

  b_reset = AddButtonText(0, 0, 0, "Reset");
  MoveButton(b_reset, button[b_ok].x + button[b_ok].sx + 4, button[b_ok].y);

  b_fit = AddButtonText(0, 0, 0, "Fit");
  MoveButton(b_fit, button[b_reset].x + button[b_reset].sx + 4, button[b_ok].y);

  b_natural = AddButtonText(0, 0, 0, "Natural");
  MoveButton(b_natural, button[b_fit].x + button[b_fit].sx + 4, button[b_ok].y);

  b_even = AddButtonText(0, 0, 0, "Even");
  MoveButton(b_even, button[b_natural].x + button[b_natural].sx + 4, button[b_ok].y);

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "EditFace_Curve", &temp_buf);
  Q.num_popups++;

  tex = ReadMIPTex(b->tex.name, 0);
  if (tex)
  {
    tsx = tex->rsx;
    tsy = tex->rsy;
  }
  else
    tsx = tsy = 64;

  for (i = j = k = 0; i < nx * ny; i++)
  {
    x1[i] = 8 + 64 * j;
    y1[i] = 24 + 48 * k + 4;

    x2[i] = x1[i] + 56;
    y2[i] = y1[i] + 40;

    x1[i] += w->pos.x;
    x2[i] += w->pos.x;
    y1[i] += w->pos.y;
    y2[i] += w->pos.y;

    if (++j == nx)
      j = 0, k++;
  }

  DrawButtons();

  redraw = 1;

  while (1)
  {
    if (redraw)
    {
      DrawSolidBox(x1[0], y1[0], x2[nx * ny - 1], y2[nx * ny - 1], BG_COLOR);

      for (i = 0; i < nx * ny; i++)
      {
        QUI_Box(x1[i], y1[i], x2[i], y1[i] + 18, 4, 8);
        QUI_DrawStr(x1[i] + 2, y1[i] + 1, BG_COLOR, 14, 0, 0, "%g", b->x.q3c.s[i]);

        QUI_Box(x1[i], y2[i] - 18, x2[i], y2[i], 4, 8);
        QUI_DrawStr(x1[i] + 2, y2[i] - 17, BG_COLOR, 14, 0, 0, "%g", b->x.q3c.t[i]);
      }

      for (i = 1; i < nx; i++)
        DrawLine(x1[i] - 4, y1[0], x1[i] - 4, y2[nx * ny - 1], 0);
      for (i = nx; i < ny * nx; i += nx)
        DrawLine(x1[0], y1[i] - 4, x2[nx - 1], y1[i] - 4, 0);

      RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
      redraw = 0;
    }

    /* Test for keys */
    if (TestKey(KEY_ESCAPE))
    {
      while (TestKey(KEY_ESCAPE))
      {
      }
      break;
    }
    if (TestKey(KEY_ENTER))
    {
      while (TestKey(KEY_ENTER))
      {
      }
      break;
    }

    /* Check for left-click */
    UpdateMouse();

    if (mouse.button & 1 && InBox(x1[0], y1[0], x2[nx * ny - 1], y2[nx * ny - 1]))
    {
      for (i = 0; i < nx * ny; i++)
      {
        if (InBox(x1[i], y1[i], x2[i], y1[i] + 18))
        {
          sprintf(buf, "%g", b->x.q3c.s[i]);
          if (readstring(buf, x1[i] + 2, y1[i] + 1, x2[i], sizeof(buf), NULL))
            b->x.q3c.s[i] = atof(buf);
          while (mouse.button || TestKey(KEY_ENTER) || TestKey(KEY_ESCAPE))
            UpdateMouse();
          break;
        }

        if (InBox(x1[i], y2[i] - 18, x2[i], y2[i]))
        {
          sprintf(buf, "%g", b->x.q3c.t[i]);
          if (readstring(buf, x1[i] + 2, y2[i] - 17, x2[i], sizeof(buf), NULL))
            b->x.q3c.t[i] = atof(buf);
          while (mouse.button || TestKey(KEY_ENTER) || TestKey(KEY_ESCAPE))
            UpdateMouse();
          break;
        }
      }
    }

    bp = UpdateButtons();
    if (bp == b_ok)
      break;

    if (bp == b_reset)
    {
      bp = b_fit;
      b->x.q3c.s[0] = 0;
      b->x.q3c.t[0] = 0;

      b->x.q3c.s[nx * ny - 1] = (nx - 1) / 2;
      b->x.q3c.t[nx * ny - 1] = (ny - 1) / 2;
    }

    if (bp == b_fit)
    {
      float bx, by, tx, ty;

      bx = b->x.q3c.s[0];
      by = b->x.q3c.t[0];
      tx = b->x.q3c.s[nx * ny - 1];
      ty = b->x.q3c.t[nx * ny - 1];

      tx = (tx - bx) / (float)(nx - 1);
      ty = (ty - by) / (float)(ny - 1);

      for (i = 0; i < nx * ny; i++)
      {
        b->x.q3c.s[i] = bx + tx * (i % nx);
        b->x.q3c.t[i] = by + ty * (i / nx);
      }

      redraw = 1;
    }

    if (bp == b_natural)
    {
      int i, j;

      Popup_Init(mouse.x, mouse.y);
      Popup_AddStr("x");
      Popup_AddStr("y");
      Popup_AddStr("z");
      j = Popup_Display();
      Popup_Free();

      for (i = 0; i < b->num_verts; i++)
      {
        switch (j)
        {
          case 0:
            b->x.q3c.s[i] = 2 * b->verts[i].y / (float)tsx;
            b->x.q3c.t[i] = 2 * b->verts[i].z / (float)tsy;
            break;
          case 1:
            b->x.q3c.s[i] = 2 * b->verts[i].x / (float)tsx;
            b->x.q3c.t[i] = 2 * b->verts[i].z / (float)tsy;
            break;
          case 2:
            b->x.q3c.s[i] = 2 * b->verts[i].x / (float)tsx;
            b->x.q3c.t[i] = 2 * b->verts[i].y / (float)tsy;
            break;
        }
      }

      redraw = 1;
    }

    if (bp == b_even)
    {
      Curve_FitEven(b);
      redraw = 1;
    }
  }

  /* Pop down the window */
  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

  RemoveButton(b_ok);
  RemoveButton(b_reset);
  RemoveButton(b_fit);
  RemoveButton(b_natural);
  RemoveButton(b_even);
  PopButtons();

  RefreshPart(w->pos.x, w->pos.y, w->pos.x + w->size.x, w->pos.y + w->size.y);
}
