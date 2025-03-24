/*
qmap.c file of the Quest Source Code

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

#include "qmap.h"

#include "3d.h"
#include "brush.h"
#include "bsp.h"
#include "camera.h"
#include "clip.h"
#include "dvport.h"
#include "edbrush.h"
#include "edent.h"
#include "edface.h"
#include "edvert.h"
#include "entity.h"
#include "error.h"
#include "game.h"
#include "geom.h"
#include "map.h"
#include "memory.h"
#include "newgroup.h"
#include "popupwin.h"
#include "quest.h"
#include "status.h"
#include "tex.h"
#include "token.h"
#include "undo.h"

int (*qmap_loadtexinfo)(texdef_t* tex);
void (*qmap_savetexinfo)(texdef_t* tex, FILE* fp);

/**********
Misc. stuff
**********/

static int
realtoint(float r)
{
  if ((r < 0.01) && (r > -0.01))
    return 0;
  if (r < 0)
  {
    r -= 0.5;
    return (int)ceil(r);
  }
  if (r > 0)
  {
    r += 0.5;
    return (int)floor(r);
  }
  return 0;
}

static float
snaptoint(float f)
{
  if (fabs(f - realtoint(f)) < 0.001)
    return realtoint(f);
  return f;
}

void
QMap_Profile(const char* name, char* profile)
{
  FILE* f;
  char mapline[1024];
  char* c;

  *profile = 0;

  f = fopen(name, "rt");
  if (!f)
    return;
  fgets(mapline, sizeof(mapline), f);
  fclose(f);

  if (!strstr(mapline, "//profile:"))
    return;

  c = strstr(mapline, "//profile:") + 10;
  while ((*c <= 32) && *c)
    c++;
  if (!*c)
    return;

  while (strlen(c) && (c[strlen(c) - 1] <= 32))
    c[strlen(c) - 1] = 0;
  if (!strlen(c))
    return;
  strcpy(profile, c);
}

/****************
Map loading stuff
****************/

static const char* errors[] =
  {
    "No error!",
    "Out of memory!",
    "Error parsing file!"};

static char buf[16384];

static char (*extra_tex)[32];
static int num_extra;

static int load_camera;

static group_t* mapgroup;
static group_t* infogroup;

int
QMap_LoadTexInfo(texdef_t* tex)
{
  int i;

  strcpy(tex->name, token);

  for (i = 0; i < 2; i++)
  {
    GETTOKEN(0, T_NUMBER);
    tex->shift[i] = atof(token);
  }

  GETTOKEN(0, T_NUMBER);
  tex->rotate = atof(token);

  for (i = 0; i < 2; i++)
  {
    GETTOKEN(0, T_NUMBER);
    tex->scale[i] = atof(token);
  }

  return ERROR_NO;
}

void
QMap_SaveTexInfo(texdef_t* tex, FILE* fp)
{
  fprintf(fp, "%s %0.0f %0.0f %0.0f %g %g", tex->name[0] ? tex->name : "no_texture", tex->shift[0], tex->shift[1], tex->rotate, tex->scale[0], tex->scale[1]);
}

/****************************************
Game independant map loading/saving stuff
****************************************/

static group_t*
FindGroup(char* name)
{
  group_t* g;

  for (g = M.GroupHead; g; g = g->Next)
  {
    if (!strcmp(name, g->groupname))
      return g;
  }
  return M.WorldGroup;
}

static void
Comments(char* buf)
{
#define CHK(y) !strncmp(buf, y, strlen(y))

  while (buf[strlen(buf) - 1] <= 32)
    buf[strlen(buf) - 1] = 0;

  // Group information,
  if (CHK("// Quest Group Name: "))
  {
    infogroup = CreateGroup(&buf[21]);
  }
  else if (CHK("// Quest Group Flags: "))
  {
    if (infogroup)
      infogroup->flags = atoi(&buf[22]);
  }
  else if (CHK("// Quest Group Color: "))
  {
    if (infogroup)
      infogroup->color = atoi(&buf[22]);
  }
  else
    // Brush/entity groups.
    if (CHK("//Quest Brush Group: "))
    {
      mapgroup = FindGroup(&buf[21]);
    }
    else if (CHK("//Quest Entity Group: "))
    {
      mapgroup = FindGroup(&buf[22]);
    }
    else
      // Viewport stuff.
      if (CHK("// ** "))
      {
        int vport;
        char keystr[128];
        float val1, val2, val3, val4;
        viewport_t* vp;

        load_camera = 1;

        sscanf(&buf[6], "%i %s %f %f %f %f\n", &vport, keystr, &val1, &val2, &val3, &val4);

        vp = &M.display.vport[vport];

        if (!strcmp(keystr, "num_vports"))
        {
          if (vport < MAX_NUM_VIEWPORTS)
            M.display.num_vports = vport;
          else
            M.display.num_vports = MAX_NUM_VIEWPORTS;
        }
        else if (!strcmp(keystr, "pos"))
        {
          vp->camera_pos.x = (int)val1;
          vp->camera_pos.y = (int)val2;
          vp->camera_pos.z = (int)val3;
        }
        else if (!strcmp(keystr, "dir"))
        {
          vp->camera_dir = (int)val1;
          vp->axis_aligned = 1;
        }
        else if (strcmp(keystr, "fullbright") == 0)
        {
          vp->fullbright = (int)val1;
        }
        else if (strcmp(keystr, "zoom_amt") == 0)
        {
          vp->zoom_amt = val1;
        }
        else if (strcmp(keystr, "mode") == 0)
        {
          vp->mode = (int)val1;
        }
        else if (!strcmp(keystr, "spos"))
        {
          vp->f_xmin = val1;
          vp->f_ymin = val2;
          vp->f_xmax = val3;
          vp->f_ymax = val4;
        }
        else if (!strcmp(keystr, "angle"))
        {
          vp->axis_aligned = 0;
          vp->rot_x = val1;
          vp->rot_y = val2;
          vp->rot_z = val3;
        }
      }
      else
        // Extra textures.
        if (CHK("// T: ") && Game.tex.cache)
        {
          if (num_extra != -1)
          {
            char(*temp)[32];

            temp = Q_realloc(extra_tex, sizeof(extra_tex[0]) * (num_extra + 1));
            if (!temp)
            {
              HandleError("LoadMap", "Out of memory loading extra textures!");
              Q_free(extra_tex);
              num_extra = -1;
            }
            else
            {
              extra_tex = temp;
              strcpy(extra_tex[num_extra], &buf[6]);
              num_extra++;
            }
          }
        }
}

static int
LoadBrush(entity_t* ent, brush_t** res)
{
  int i;
  vec3_t vec1, vec2, norm;

  brush_my_t B;
  face_my_t* f;

  brush_t* b;
  plane_t* p;

  mapgroup = M.WorldGroup;

  memset(&B, 0, sizeof(brush_my_t));

  GETTOKEN(1, T_NAME | T_MISC);
  if (!strcmp(token, "patchDef2"))
  { // Quake 3 style bezier patch 'brush'
    int nx, ny;
    int j, k;

    b = B_New(BR_Q3_CURVE);
    if (!b)
      return ERROR_NOMEM;
    *res = b;

    EXPECT(1, T_MISC, "{");

    GETTOKEN(1, T_ALLNAME);
    strcpy(b->tex.name, token);

    EXPECT(1, T_MISC, "(");

    /* Note: order of these two are not what you might expect */
    GETTOKEN(1, -1);
    ny = atoi(token);
    GETTOKEN(1, -1);
    nx = atoi(token);

    /* TODO : what are these numbers? flags, contents, value? */
    GETTOKEN(1, -1);
    GETTOKEN(1, -1);
    GETTOKEN(1, -1);

    EXPECT(1, T_MISC, ")");

    b->x.q3c.sizex = nx;
    b->x.q3c.sizey = ny;
    b->x.q3c.s = Q_malloc(sizeof(float) * nx * ny);
    if (!b->x.q3c.s)
      return ERROR_NOMEM;
    b->x.q3c.t = Q_malloc(sizeof(float) * nx * ny);
    if (!b->x.q3c.t)
      return ERROR_NOMEM;

    b->num_verts = nx * ny;

    b->verts = Q_malloc(sizeof(vec3_t) * b->num_verts);
    if (!b->verts)
      return ERROR_NOMEM;

    b->tverts = Q_malloc(sizeof(vec3_t) * b->num_verts);
    if (!b->tverts)
      return ERROR_NOMEM;

    b->sverts = Q_malloc(sizeof(svec_t) * b->num_verts);
    if (!b->sverts)
      return ERROR_NOMEM;

    b->num_planes = (nx - 1) * (ny - 1) / 4;
    b->plane = Q_malloc(sizeof(plane_t) * b->num_planes);
    if (!b->plane)
      return ERROR_NOMEM;

    memset(b->plane, 0, sizeof(plane_t) * b->num_planes);

    b->num_edges = 0;
    b->edges = NULL;

    EXPECT(1, T_MISC, "(");

    for (i = 0, k = 0; i < ny; i++)
    {
      EXPECT(1, T_MISC, "(");
      for (j = 0; j < nx; j++, k++)
      {
        EXPECT(0, T_MISC, "(");

        GETTOKEN(0, T_NUMBER);
        b->verts[k].x = atof(token);
        GETTOKEN(0, T_NUMBER);
        b->verts[k].y = atof(token);
        GETTOKEN(0, T_NUMBER);
        b->verts[k].z = atof(token);

        GETTOKEN(0, T_NUMBER);
        b->x.q3c.s[k] = atof(token);
        GETTOKEN(0, T_NUMBER);
        b->x.q3c.t[k] = atof(token);

        EXPECT(0, T_MISC, ")");
      }
      EXPECT(1, T_MISC, ")");
    }

    EXPECT(1, T_MISC, ")");
    EXPECT(1, T_MISC, "}");
    EXPECT(1, T_MISC, "}");

    k = (nx - 1) / 2;
    for (i = 0, p = b->plane; i < b->num_planes; i++, p++)
    {
      p->num_verts = 9;
      p->verts = Q_malloc(sizeof(int) * p->num_verts);
      if (!p->verts)
        return ERROR_NOMEM;

      j = (i / k) * nx * 2 + 2 * (i % k);

      p->verts[0] = j;
      p->verts[1] = j + 1;
      p->verts[2] = j + 2;

      p->verts[3] = j + nx;
      p->verts[4] = j + nx + 1;
      p->verts[5] = j + nx + 2;

      p->verts[6] = j + 2 * nx;
      p->verts[7] = j + 2 * nx + 1;
      p->verts[8] = j + 2 * nx + 2;
    }

    RecalcNormals(b);
    CalcBrushCenter(b);

    b->Group = mapgroup;
    b->EntityRef = ent;

    return ERROR_NO;
  }
  else if (!strcmp(token, "("))
  {
    b = B_New(BR_NORMAL);
    if (!b)
      return ERROR_NOMEM;
    *res = b;

    while (strcmp(token, "}"))
    {
      B.faces = Q_realloc(B.faces, sizeof(face_my_t) * (B.numfaces + 1));
      if (!B.faces)
        return ERROR_NOMEM;

      f = &B.faces[B.numfaces];

      for (i = 0; i < 3; i++)
      {
        if (strcmp(token, "("))
          return ERROR_PARSE;

        GETTOKEN(0, T_NUMBER);
        f->planepts[i].x = atof(token);

        GETTOKEN(0, T_NUMBER);
        f->planepts[i].y = atof(token);

        GETTOKEN(0, T_NUMBER);
        f->planepts[i].z = atof(token);

        EXPECT(0, T_MISC, ")");

        if (i == 2)
        {
          GETTOKEN(0, T_ALLNAME);
        }
        else
        {
          GETTOKEN(0, -1);
        }
      }

      i = qmap_loadtexinfo(&f->tex);
      if (i)
        return i;

      /* Calc normal */

      vec1.x = f->planepts[1].x - f->planepts[0].x;
      vec1.y = f->planepts[1].y - f->planepts[0].y;
      vec1.z = f->planepts[1].z - f->planepts[0].z;

      vec2.x = f->planepts[1].x - f->planepts[2].x;
      vec2.y = f->planepts[1].y - f->planepts[2].y;
      vec2.z = f->planepts[1].z - f->planepts[2].z;

      CrossProd(vec1, vec2, &norm);
      Normalize(&norm);

      /* Convert to normal / dist plane */
      f->normal.x = norm.x;
      f->normal.y = norm.y;
      f->normal.z = norm.z;

      f->dist = DotProd(f->normal, f->planepts[0]);

      B.numfaces++;

      GETTOKEN(1, -1);
    }

    if (!BuildBrush(&B, b))
    {
      return 0;
    }
    CalcBrushCenter(b);

    b->Group = mapgroup;
    b->EntityRef = ent;

    return ERROR_NO;
  }
  else // load a new style brush
  {
    int i, j, k;

    if (strcmp(token, ":"))
      return ERROR_PARSE;

    b = B_New(BR_NORMAL);
    if (!b)
      return ERROR_NOMEM;
    *res = b;

    GETTOKEN(0, T_NUMBER);

    b->num_verts = atoi(token);

    b->verts = Q_malloc(sizeof(vec3_t) * b->num_verts);
    if (!b->verts)
      return ERROR_NOMEM;

    b->tverts = Q_malloc(sizeof(vec3_t) * b->num_verts);
    if (!b->tverts)
      return ERROR_NOMEM;

    b->sverts = Q_malloc(sizeof(svec_t) * b->num_verts);
    if (!b->sverts)
      return ERROR_NOMEM;

    for (i = 0; i < b->num_verts; i++)
    {
      GETTOKEN(1, T_NUMBER);
      b->verts[i].x = atof(token);

      GETTOKEN(0, T_NUMBER);
      b->verts[i].y = atof(token);

      GETTOKEN(0, T_NUMBER);
      b->verts[i].z = atof(token);
    }

    b->num_planes = 0;
    b->plane = NULL;

    GETTOKEN(1, -1);

    while (strcmp(token, "}"))
    {
      b->plane = Q_realloc(b->plane, sizeof(plane_t) * (b->num_planes + 1));
      if (!b->plane)
        return ERROR_NOMEM;

      p = &b->plane[b->num_planes++];
      memset(p, 0, sizeof(plane_t));

      p->num_verts = atoi(token);

      p->verts = Q_malloc(sizeof(int) * p->num_verts);
      if (!p->verts)
        return ERROR_NOMEM;

      EXPECT(0, T_MISC, "(");

      for (i = 0; i < p->num_verts; i++)
      {
        GETTOKEN(0, T_NUMBER);
        p->verts[i] = atoi(token);
      }

      EXPECT(0, T_MISC, ")");

      GETTOKEN(0, T_ALLNAME);

      qmap_loadtexinfo(&p->tex);

      GETTOKEN(1, -1);
    }

    b->num_edges = 0;
    b->edges = NULL;

    for (i = 0; i < b->num_planes; i++)
    {
      int v1, v2;

      p = &b->plane[i];

      for (j = 0; j < p->num_verts; j++)
      {
        v1 = p->verts[j];
        if (j == p->num_verts - 1)
          v2 = p->verts[0];
        else
          v2 = p->verts[j + 1];

        if (v2 < v1)
        {
          k = v2;
          v2 = v1;
          v1 = k;
        }

        for (k = 0; k < b->num_edges; k++)
        {
          if ((b->edges[k].startvertex == v1) &&
              (b->edges[k].endvertex == v2))
            break;
        }
        if (k == b->num_edges)
        {
          b->edges = Q_realloc(b->edges, sizeof(edge_t) * (b->num_edges + 1));
          if (!b->edges)
            return ERROR_NOMEM;
          b->edges[b->num_edges].startvertex = v1;
          b->edges[b->num_edges].endvertex = v2;
          b->num_edges++;
        }
      }
    }

    RecalcNormals(b);
    CalcBrushCenter(b);

    b->Group = mapgroup;
    b->EntityRef = ent;

    return ERROR_NO;
  }
}

static int
LoadEntity(entity_t** res, brush_t** bfirst, brush_t** blast, int* num, int is_group)
{
  entity_t* e;
  int i;
  brush_t *first, *last;
  brush_t* b;

  mapgroup = M.WorldGroup;

  if (strcmp(token, "{"))
    return ERROR_PARSE;

  GETTOKEN(1, -1);

  if (strcmp(token, "*") || !is_group)
  {
    e = Q_malloc(sizeof(entity_t));
    if (!e)
      return ERROR_NOMEM;
    memset(e, 0, sizeof(entity_t));
    InitEntity(e);

    *res = e;

    e->Group = mapgroup;

    while (strcmp(token, "}") && strcmp(token, "{"))
    {
      strcpy(buf, &token[1]);
      buf[strlen(buf) - 1] = 0;

      GETTOKEN(0, T_STRING);

      strcpy(token, &token[1]);
      token[strlen(token) - 1] = 0;

      SetKeyValue(e, buf, token);

      GETTOKEN(1, -1);
    }
  }
  else
  {
    e = M.WorldSpawn;
    *res = NULL;

    GETTOKEN(1, -1);
  }

  first = last = NULL;
  *num = 0;

  while (!strcmp(token, "{"))
  {
    i = LoadBrush(e, &b);
    if (i)
      return i;

    if (!last)
      last = b;

    if (first)
      first->Last = b;
    b->Next = first;
    b->Last = NULL;
    first = b;

    (*num)++;

    GETTOKEN(1, -1);
  }

  if (strcmp(token, "}"))
    return ERROR_PARSE;

  *bfirst = first;
  *blast = last;

  return ERROR_NO;
}

int
QMap_Load(const char* filename)
{
  int i;
  entity_t* e;
  brush_t *b1, *b2;
  int n_brush;

  CreateWorldGroup();

  if (!TokenFile(filename, T_C | T_NUMBER | T_STRING | T_MISC, Comments))
    Abort("LoadMap", "Unable to load '%s'!", filename);

  infogroup = NULL;

  num_extra = 0;
  extra_tex = NULL;

  load_camera = 0;

  M.num_entities = 0;
  M.num_brushes = 0;
  M.BrushHead = NULL;
  M.EntityHead = NULL;
  M.WorldSpawn = NULL;

  InitCamera();
  InitViewports();

  while (TokenGet(1, -1))
  {
    i = LoadEntity(&e, &b1, &b2, &n_brush, 0);
    if (i)
    {
      HandleError("LoadMap", "'%s', line %i: %s", filename, token_linenum, errors[i]);
      return 0;
    }

    if (b1)
    {
      if (M.BrushHead)
        M.BrushHead->Last = b2;
      b2->Next = M.BrushHead;
      M.BrushHead = b1;
      M.num_brushes += n_brush;
    }

    if (M.EntityHead)
      M.EntityHead->Last = e;
    e->Next = M.EntityHead;
    e->Last = NULL;
    M.EntityHead = e;
    M.num_entities++;

    if (!M.WorldSpawn) // assume the first entity is the worldspawn entity
      M.WorldSpawn = e;
  }

  TokenDone();

  UpdateViewportPositions();
  if (!load_camera)
    InitCamera();

  /* TODO  if (M.num_brushes)
        strcpy(texturename,M.BrushHead->plane[0].tex.name);
     else*/
  texturename[0] = 0;

  RecalcAllNormals();

  ReadCache(1);

  if (Game.tex.cache)
  {
    for (i = 0; i < num_extra; i++)
      ReadMIPTex(extra_tex[i], 0);

    Q_free(extra_tex);
    num_extra = 0;
  }

  return 1;
}

int
QMap_LoadGroup(const char* filename)
{
  int i;
  entity_t* e;
  brush_t *b1, *b2;
  int n_brush;

  /*   int dx,dy,dz;
     int x,y,z;
     viewport_t *vp;
     char *value;
     brush_t *b;*/

  if (!TokenFile(filename, T_C | T_NUMBER | T_STRING | T_MISC, Comments))
  {
    HandleError("LoadGroup", "Unable to load '%s'!", filename);
    return 0;
  }

  /* This code assumes the saved group is centered at origin, which
     isn't true at the moment. Leave the group where it is.

     Move(M.display.active_vport,MOVE_FORWARD,&dx,&dy,&dz,128);
     vp=&M.display.vport[M.display.active_vport];

     dx+=SnapPointToGrid(vp->camera_pos.x);
     dy+=SnapPointToGrid(vp->camera_pos.y);
     dz+=SnapPointToGrid(vp->camera_pos.z);*/

  while (TokenGet(1, -1))
  {
    i = LoadEntity(&e, &b1, &b2, &n_brush, 1);
    if (i)
    {
      HandleError("LoadMap", "'%s', line %i: %s", filename, token_linenum, errors[i]);
      return 0;
    }

    /*      if (e)
          {
             value=GetKeyValue(e,"origin");
             if (value)
             {
                sscanf(value,"%i %i %i", &x, &y, &z);
                sprintf(value,"%i %i %i",x+dx,y+dy,z+dz);
                SetKeyValue(e,"origin",value);
             }
          }

          for (b=b1;b;b=b->Next)
          {
             for (i=0;i<b->num_verts;i++)
             {
                b->verts[i].x += dx;
                b->verts[i].y += dy;
                b->verts[i].z += dz;
             }
          }*/

    if (b1)
    {
      if (M.BrushHead)
        M.BrushHead->Last = b2;
      b2->Next = M.BrushHead;
      M.BrushHead = b1;
      M.num_brushes += n_brush;
    }

    if (e)
    {
      if (M.EntityHead)
        M.EntityHead->Last = e;
      e->Next = M.EntityHead;
      e->Last = NULL;
      M.EntityHead = e;
      M.num_entities++;
    }
  }

  TokenDone();

  RecalcAllNormals();

  return 1;
}

/* Map saving stuff. */

static void
SaveCurve(FILE* fp, brush_t* b)
{
  int nx, ny, i, j;

  if (!b->num_planes)
    return;

  nx = b->plane[0].verts[3];
  ny = (b->num_planes / ((nx - 1) / 2)) * 2 + 1;

  if ((b->num_planes % ((nx - 1) / 2)) || (b->num_planes % ((ny - 1) / 2)) ||
      (b->num_verts != nx * ny) ||
      (nx != b->x.q3c.sizex) || (ny != b->x.q3c.sizey))
  {
    HandleError("SaveCurve",
                "Curve too complex to save in Quake 3 .map format!");
    return;
  }

  fprintf(fp, "{\n");
  if (b->Group)
  {
    if ((b->Group->flags & 0x1) == 0)
    {
      fprintf(fp, "//Quest Brush Group: %s\n", b->Group->groupname);
    }
  }

  fprintf(fp, "patchDef2\n{\n%s\n( %i %i 0 0 0 )\n(\n", b->tex.name[0] ? b->tex.name : "no_texture", ny, nx);

  for (i = 0; i < ny; i++)
  {
    fprintf(fp, "( ");
    for (j = 0; j < nx; j++)
    {
      fprintf(fp, "( %g %g %g %g %g ) ", snaptoint(b->verts[i * nx + j].x), snaptoint(b->verts[i * nx + j].y), snaptoint(b->verts[i * nx + j].z),

              snaptoint(b->x.q3c.s[i * nx + j]),
              snaptoint(b->x.q3c.t[i * nx + j]));
    }
    fprintf(fp, ")\n");
  }

  fprintf(fp, ")\n}\n}\n");
}

static void
SaveBrush(FILE* fp, brush_t* b)
{
  int i, j;
  plane_t* p;
  int found;
  face_my_t F;

  switch (b->bt->type)
  {
    case BR_NORMAL:
      break;

    case BR_Q3_CURVE:
      SaveCurve(fp, b);
      return;

    default:
      return;
  }

  fprintf(fp, "{\n");
  if (b->Group)
  {
    if ((b->Group->flags & 0x1) == 0)
    {
      fprintf(fp, "//Quest Brush Group: %s\n", b->Group->groupname);
    }
  }

  // DEBUG
  //   fprintf(fp,"// UID=%i\n",b->uid);

  if (status.map2)
  {
    fprintf(fp, ":%i\n", b->num_verts);
    for (i = 0; i < b->num_verts; i++)
      fprintf(fp, "%g %g %g\n", snaptoint(b->verts[i].x), snaptoint(b->verts[i].y), snaptoint(b->verts[i].z));
  }

  for (i = 0; i < b->num_planes; i++)
  {
    p = &b->plane[i];

    if (status.map2)
    {
      fprintf(fp, "%i ( ", p->num_verts);

      for (j = 0; j < p->num_verts; j++)
        fprintf(fp, "%i ", p->verts[j]);

      fprintf(fp, ") ");
    }
    else
    { // old brush save
      /* See if we should use the actual vertex locations (if they
         are snapped to integers, they're preferable to use), or
         if we should generate a large box plane and use it. */
      found = FALSE;
      for (j = 0; (j < p->num_verts) && (found == FALSE); j++)
      {
        if (fabs(b->verts[p->verts[j]].x - floor(b->verts[p->verts[j]].x + .5)) > 0.01)
          found = TRUE;
        if (fabs(b->verts[p->verts[j]].y - floor(b->verts[p->verts[j]].y + .5)) > 0.01)
          found = TRUE;
        if (fabs(b->verts[p->verts[j]].z - floor(b->verts[p->verts[j]].z + .5)) > 0.01)
          found = TRUE;
      }

      if ((found == FALSE) || status.snap_to_int)
      {
        /* Using actual vertices */
        fprintf(fp, "( %i %i %i ) ( %i %i %i ) ( %i %i %i ) ", realtoint(b->verts[p->verts[0]].x), realtoint(b->verts[p->verts[0]].y), realtoint(b->verts[p->verts[0]].z), realtoint(b->verts[p->verts[1]].x), realtoint(b->verts[p->verts[1]].y), realtoint(b->verts[p->verts[1]].z), realtoint(b->verts[p->verts[2]].x), realtoint(b->verts[p->verts[2]].y), realtoint(b->verts[p->verts[2]].z));
      }
      else
      {
        /* Using MakeBoxOnPlane for save */
        F.normal.x = p->normal.x;
        F.normal.y = p->normal.y;
        F.normal.z = p->normal.z;
        F.dist = DotProd(p->normal, b->verts[p->verts[0]]);

        MakeBoxOnPlane(&F);

        fprintf(fp, "( %i %i %i ) ( %i %i %i ) ( %i %i %i ) ", realtoint(F.pts[0].x), realtoint(F.pts[0].y), realtoint(F.pts[0].z), realtoint(F.pts[1].x), realtoint(F.pts[1].y), realtoint(F.pts[1].z), realtoint(F.pts[2].x), realtoint(F.pts[2].y), realtoint(F.pts[2].z));
      }
    } // end old plane code

    qmap_savetexinfo(&p->tex, fp);

    fprintf(fp, "\n");
  }
  fprintf(fp, "}\n");
}

static void
WriteHeader(FILE* fp)
{
  viewport_t* vp;
  int i;

  if (profile[0])
  {
    fprintf(fp, "//profile: %s\n", profile);
  }

  fprintf(fp, "// --\n");
  fprintf(fp, "// -- Created with Quest\n");
  fprintf(fp, "// --\n");

  fprintf(fp, "// ** %i num_vports\n", M.display.num_vports);

  for (i = 0; i < M.display.num_vports; i++)
  {
    vp = &M.display.vport[i];

    fprintf(fp, "// ** %i spos %.5f %.5f %.5f %.5f\n", i, vp->f_xmin, vp->f_ymin, vp->f_xmax, vp->f_ymax);

    fprintf(fp, "// ** %i pos %i %i %i\n", i, vp->camera_pos.x, vp->camera_pos.y, vp->camera_pos.z);
    fprintf(fp, "// ** %i dir %i\n", i, vp->camera_dir);
    fprintf(fp, "// ** %i fullbright %i\n", i, vp->fullbright);
    fprintf(fp, "// ** %i zoom_amt %2.4f\n", i, vp->zoom_amt);
    fprintf(fp, "// ** %i mode %i\n", i, vp->mode);
    if (!vp->axis_aligned)
      fprintf(fp, "// ** %i angle %i %i %i\n", i, vp->rot_x, vp->rot_y, vp->rot_z);
  }
}

static void
WriteGroupInfo(FILE* fp, group_t* g)
{
  fprintf(fp, "// Quest Group Name: %s\n", g->groupname);
  fprintf(fp, "// Quest Group Flags: %i\n", g->flags);
  fprintf(fp, "// Quest Group Color: %i\n", (unsigned char)g->color);
}

static void
WriteGroups(FILE* fp)
{
  group_t* g;

  for (g = M.GroupHead; g->Next; g = g->Next)
  {
  }
  for (; g; g = g->Last)
    WriteGroupInfo(fp, g);
}

static void
WriteEntity(FILE* fp, entity_t* e)
{
  int i;

  fprintf(fp, "{\n");
  if (!(e->Group->flags & 0x01))
    fprintf(fp, "//Quest Entity Group: %s\n", e->Group->groupname);

  // DEBUG
  //   fprintf(fp,"// UID=%i\n",e->uid);

  for (i = 0; i < e->numkeys; i++)
  {
    fprintf(fp, "\"%s\" \"%s\"\n", e->key[i], e->value[i]);
  }
}

int
QMap_Save(const char* filename)
{
  int i;

  FILE* fp;

  entity_t* e;

  brush_t* LastBrush;
  brush_t* b;

  char backupname[1024];

  if (!filename[0])
    return FileCmd(SAVE_IT);

  {
    char *b, *c;

    strcpy(backupname, filename);
    c = NULL;
    for (b = backupname; *b; b++)
    {
      if (*b == '.')
        c = b;
      if (*b == '/')
        c = NULL;
      if (*b == '\\')
        c = NULL;
    }
    if (c)
      *c = 0;
    strcat(backupname, ".bak");
  }

  rename(filename, backupname);

  fp = fopen(filename, "wt");
  if (!fp)
  {
    HandleError("SaveMap", "Unable to create file '%s'!", filename);
    return 0;
  }

  WriteHeader(fp);

  WriteGroups(fp);

  if (Game.tex.cache)
  {
    for (i = 0; i < M.num_textures; i++)
    {
      fprintf(fp, "// T: %s\n", M.Cache[i].name);
    }
  }

  /* Locate the last entity (will be worldspawn since our lists were built
     backwards) */
  for (e = M.EntityHead; e->Next; e = e->Next)
  {
  }
  if (M.BrushHead)
  {
    for (b = M.BrushHead; b->Next; b = b->Next)
    {
    }
    LastBrush = b;
  }
  else
    LastBrush = NULL;

  for (; e; e = e->Last)
  {
    WriteEntity(fp, e);

    for (b = LastBrush; b; b = b->Last)
      if (b->EntityRef == e)
        SaveBrush(fp, b);

    fprintf(fp, "}\n");
  }

  fclose(fp);

  M.modified = 0;

  return 1;
}

int
QMap_SaveGroup(const char* filename, group_t* g)
{
  FILE* fp;

  brush_t* b;
  entity_t* e;

  int found;

  fp = fopen(filename, "wt");
  if (!fp)
  {
    HandleError("LoadPrefabBrush", "Unable to create '%s'!", filename);
    return 0;
  }

  WriteGroupInfo(fp, g);

  /* Write entities */
  for (e = M.EntityHead; e; e = e->Next)
  {
    found = 0;
    if ((e->Group == g) && (e != M.WorldSpawn))
    {
      WriteEntity(fp, e);
      found = 1;
    }

    /* Write brushes w/entity, if any exist */
    for (b = M.BrushHead; b; b = b->Next)
    {
      if ((b->Group == g) && (b->EntityRef == e))
      {
        /* If it's the first brush in the entity, write out all
           the key/values and brackets */
        if (!found)
        {
          found = 1;
          if (e == M.WorldSpawn)
          {
            fprintf(fp, "{*\n");
          }
          else
          {
            WriteEntity(fp, e);
          }
        }

        SaveBrush(fp, b);
      }
    }
    if (found)
      fprintf(fp, "}\n");
  }
  fclose(fp);

  return 1;
}

int
QMap_SaveVisible(const char* filename, int add_shell)
{
  int found_start;
  FILE* fp;
  entity_t* e;
  brush_t* LastBrush;
  brush_t* b;

  vec3_t min, max;

  int numb;

  if (add_shell)
  {
    /*
     Find min and max values to create a box around the level so vis
    will still work.
    */
    vec3_t temp;
    int i;

    min.x = min.y = min.z = 10000;
    max.x = max.y = max.z = -10000;
#define ChkVec(v, a) \
  if (v.a < min.a)   \
    min.a = v.a;     \
  if (v.a > max.a)   \
    max.a = v.a;
    for (b = M.BrushHead; b; b = b->Next)
    {
      if (b->Group->flags & 0x02)
        continue;

      for (i = 0; i < b->num_verts; i++)
      {
        ChkVec(b->verts[i], x);
        ChkVec(b->verts[i], y);
        ChkVec(b->verts[i], z);
      }
    }

    for (e = M.EntityHead; e; e = e->Next)
    {
      if (e->Group->flags & 0x02)
        continue;

      if (!GetKeyValue(e, "origin"))
        continue;

      sscanf(GetKeyValue(e, "origin"), "%f %f %f", &temp.x, &temp.y, &temp.z);
      ChkVec(temp, x);
      ChkVec(temp, y);
      ChkVec(temp, z);
    }

#undef ChkVec
    /*      min.x-=32;
          min.y-=32;
          min.z-=32;
          max.x+=32;
          max.y+=32;
          max.z+=32;*/
  }

  found_start = FALSE;

  fp = fopen(filename, "wt");
  if (!fp)
  {
    HandleError("CreateVisibleMap", "Unable to create '%s'!", filename);
    return 0;
  }

  WriteHeader(fp);
  WriteGroups(fp);

  /* Locate the last entity (will be WorldSpawn since our lists were built
     backwards) */
  for (e = M.EntityHead; e->Next; e = e->Next)
  {
  }
  if (M.BrushHead)
  {
    for (b = M.BrushHead; b->Next; b = b->Next)
    {
    }
    LastBrush = b;
  }
  else
    LastBrush = NULL;

  for (; e; e = e->Last)
  {
    if (e != M.WorldSpawn)
    {
      if (e->Group->flags & 0x02)
      {
        continue;
      }

      if (!GetKeyValue(e, "origin"))
      {
        for (b = LastBrush; b; b = b->Last)
        {
          if (b->Group->flags & 0x02)
            continue;
          if (b->EntityRef == e)
            break;
        }
        if (!b)
        {
          continue;
        }
      }
    }

    WriteEntity(fp, e);

    if (GetKeyValue(e, "classname"))
      if (!strcmp(GetKeyValue(e, "classname"), "info_player_start"))
        found_start = 1;

    if (GetKeyValue(e, "classname"))
    {
      if (add_shell && !strcmp(GetKeyValue(e, "classname"), "worldspawn"))
      {
#define AddPlane(p00, p01, p02, p10, p11, p12, p20, p21, p22) \
  fprintf(fp, "( %0.0f %0.0f %0.0f ) "                        \
              "( %0.0f %0.0f %0.0f ) "                        \
              "( %0.0f %0.0f %0.0f ) "                        \
              "region 0 0 0 1 1\n",                           \
          p00,                                                \
          p01,                                                \
          p02,                                                \
          p10,                                                \
          p11,                                                \
          p12,                                                \
          p20,                                                \
          p21,                                                \
          p22)

#define AddBrush(x1, y1, z1, x2, y2, z2)        \
  fprintf(fp, "{\n");                           \
                                                \
  AddPlane(x1, y1, z1, x2, y1, z1, x1, y2, z1); \
  AddPlane(x2, y2, z2, x2, y1, z2, x1, y2, z2); \
                                                \
  AddPlane(x1, y1, z1, x1, y2, z1, x1, y1, z2); \
  AddPlane(x2, y2, z2, x2, y2, z1, x2, y1, z2); \
                                                \
  AddPlane(x1, y1, z1, x1, y1, z2, x2, y1, z1); \
  AddPlane(x2, y2, z2, x1, y2, z2, x2, y2, z1); \
                                                \
  fprintf(fp, "}\n");

        AddBrush(min.x - 16, min.y, min.z, min.x, max.y, max.z);
        AddBrush(max.x, min.y, min.z, max.x + 16, max.y, max.z);

        AddBrush(min.x, min.y - 16, min.z, max.x, min.y, max.z);
        AddBrush(min.x, max.y, min.z, max.x, max.y + 16, max.z);

        AddBrush(min.x, min.y, min.z - 16, max.x, max.y, min.z);
        AddBrush(min.x, min.y, max.z, max.x, max.y, max.z + 16);
#undef AddBrush
#undef AddPlane
      }
    }

    numb = 0;
    for (b = LastBrush; b; b = b->Last)
    {
      if (b->Group->flags & 0x02)
        continue;
      if (b->EntityRef == e)
      {
        SaveBrush(fp, b);
      }
    }
    fprintf(fp, "}\n");
  }

  if (!found_start)
  {
    viewport_t* vp;

    vp = &M.display.vport[M.display.active_vport];

    fprintf(fp, "{\n");
    fprintf(fp, "\"classname\" \"info_player_start\"\n");
    fprintf(fp, "\"origin\" \"%i %i %i\"\n", vp->camera_pos.x, vp->camera_pos.y, vp->camera_pos.z);
    fprintf(fp, "}\n");
  }

  fclose(fp);

  return 1;
}
