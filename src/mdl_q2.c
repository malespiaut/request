#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "mdl.h"
#include "mdl_all.h"

#include "mdl_q2.h"

#include "error.h"
#include "memory.h"

#define IDALIASHEADER (('2' << 24) + ('P' << 16) + ('D' << 8) + 'I')
#define ALIAS_VERSION 8

typedef struct
{
  short index_xyz[3];
  short index_st[3];
} dtriangle_t;

typedef struct
{
  unsigned char v[3]; // scaled byte to fit in frame mins/maxs
  char lightnormalindex;
} dtrivertx_t;

typedef struct
{
  float scale[3];       // multiply byte verts by this
  float translate[3];   // then add this
  char name[16];        // frame name from grabbing
  dtrivertx_t verts[1]; // variable sized
} daliasframe_t;

typedef struct
{
  int ident;
  int version;

  int skinwidth;
  int skinheight;
  int framesize; // byte size of each frame

  int num_skins;
  int num_xyz;
  int num_st; // greater than num_xyz for seams
  int num_tris;
  int num_glcmds; // dwords in strip/fan command list
  int num_frames;

  int ofs_skins;  // each skin is a MAX_SKINNAME string
  int ofs_st;     // byte offset from start for stverts
  int ofs_tris;   // offset for dtriangles
  int ofs_frames; // offset for first frame
  int ofs_glcmds;
  int ofs_end; // end of file
} dmdl_t;

mdl_t*
Q2_LoadMD2(FILE* f, int baseofs)
{

  dmdl_t m;
  mdl_t* t;
  int i, j;
  int v1, v2;
  vec3_t* v;

  daliasframe_t* d;
  dtriangle_t* dt;

  fseek(f, baseofs, SEEK_SET);
  fread(&m, 1, sizeof(m), f);
  if (m.ident != IDALIASHEADER)
    return NULL;
  if (m.version != ALIAS_VERSION)
    return NULL;

  d = Q_malloc(m.framesize);
  if (!d)
    Abort("LoadMD2", "Out of memory!");

  t = Q_malloc(sizeof(mdl_t));
  if (!t)
    Abort("LoadMD2", "Out of memory!");
  memset(t, 0, sizeof(mdl_t));

  t->numvertices = m.num_xyz;
  t->vertlist = Q_malloc(t->numvertices * sizeof(vec3_t));
  if (!t->vertlist)
    Abort("LoadMD2", "Out of memory!");

  fseek(f, baseofs + m.ofs_frames, SEEK_SET);
  fread(d, 1, m.framesize, f);

  for (i = 0, v = t->vertlist; i < t->numvertices; i++, v++)
  {
    v->x = ((float)d->verts[i].v[0]) * d->scale[0] + d->translate[0];
    v->y = ((float)d->verts[i].v[1]) * d->scale[1] + d->translate[1];
    v->z = ((float)d->verts[i].v[2]) * d->scale[2] + d->translate[2];
  }

  Q_free(d);

  dt = Q_malloc(m.num_tris * sizeof(dtriangle_t));
  if (!dt)
    Abort("LoadMD2", "Out of memory!");
  fseek(f, baseofs + m.ofs_tris, SEEK_SET);
  fread(dt, 1, m.num_tris * sizeof(dtriangle_t), f);

  t->numedges = 0;
  t->edgelist = NULL;
  for (i = 0; i < m.num_tris; i++)
  {
    for (j = 0; j < 2; j++)
    {
      v1 = dt[i].index_xyz[j];
      if (j == 2)
        v2 = dt[i].index_xyz[0];
      else
        v2 = dt[i].index_xyz[j + 1];

      AddEdge(t, v1, v2);
    }
  }

  Q_free(dt);

  return t;
}
