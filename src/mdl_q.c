#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "mdl.h"
#include "mdl_all.h"

#include "mdl_q.h"

#include "error.h"
#include "memory.h"

#define IDPOLYHEADER (('O' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define ALIAS_VERSION 6

typedef struct
{
  int ident;
  int version;
  float scale[3];
  float scale_origin[3];
  float boundingradius;
  float eyeposition[3];
  int numskins;
  int skinwidth;
  int skinheight;
  int numverts;
  int numtris;
  int numframes;
  int synctype;
  int flags;
  float size;
} id_mdl_t;

typedef struct dtriangle_s
{
  int facesfront;
  int vertindex[3];
} dtriangle_t;

typedef struct
{
  unsigned char v[3];
  char lightnormalindex;
} trivertx_t;

typedef struct
{
  trivertx_t bboxmin; /* lightnormal isn't used*/
  trivertx_t bboxmax; /* lightnormal isn't used*/
  char name[16];      /* frame name from grabbing*/
} daliasframe_t;

typedef struct
{
  int numframes;
  trivertx_t bboxmin; /* lightnormal isn't used*/
  trivertx_t bboxmax; /* lightnormal isn't used*/
} daliasgroup_t;

mdl_t*
Q_LoadMDL(FILE* f, int baseofs)
{

  id_mdl_t m;
  mdl_t* t;
  int v1, v2;
  int i, j;
  dtriangle_t* dt;
  daliasgroup_t g;
  trivertx_t* v;

  fseek(f, baseofs, SEEK_SET);
  fread(&m, 1, sizeof(m), f);

  fseek(f,
        baseofs +
          sizeof(m) +
          (m.skinwidth * m.skinheight + 4) * m.numskins +
          12 * m.numverts,
        SEEK_SET);

  dt = Q_malloc(sizeof(dtriangle_t) * m.numtris);
  if (!dt)
    Abort("LoadMDL", "Out of memory!");

  fread(dt, 1, sizeof(dtriangle_t) * m.numtris, f);

  t = Q_malloc(sizeof(mdl_t));
  if (!t)
    Abort("LoadMDL", "Out of memory!");
  memset(t, 0, sizeof(mdl_t));

  for (i = 0; i < m.numtris; i++)
  {
    for (j = 0; j < 3; j++)
    {
      v1 = dt[i].vertindex[j];
      v2 = dt[i].vertindex[(j + 1) % 3];

      AddEdge(t, v1, v2);
    }
  }

  Q_free(dt);

  // the file should now be at the position to read the frames
  fread(&i, 1, sizeof(i), f);
  if (i)
  {
    fread(&g, 1, sizeof(g), f);
    fseek(f, 4 * g.numframes, SEEK_CUR);
  }
  else
  {
    fseek(f, sizeof(daliasframe_t), SEEK_CUR);
  }

  t->numvertices = m.numverts;
  t->vertlist = Q_malloc(t->numvertices * sizeof(vec3_t));
  if (!t->vertlist)
    Abort("LoadMDL", "Out of memory!");

  v = Q_malloc(sizeof(trivertx_t) * m.numverts);
  if (!v)
    Abort("LoadMDL", "Out of memory!");
  fread(v, 1, sizeof(trivertx_t) * m.numverts, f);

  for (i = 0; i < t->numvertices; i++)
  {
    t->vertlist[i].x = ((float)v[i].v[0]) * m.scale[0] + m.scale_origin[0];
    t->vertlist[i].y = ((float)v[i].v[1]) * m.scale[1] + m.scale_origin[1];
    t->vertlist[i].z = ((float)v[i].v[2]) * m.scale[2] + m.scale_origin[2];
  }

  Q_free(v);

  return t;
}
