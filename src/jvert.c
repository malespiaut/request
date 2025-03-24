/*
jvert.c file of the Quest Source Code

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

#include "jvert.h"

#include "3d.h"
#include "brush.h"
#include "edvert.h"
#include "map.h"
#include "memory.h"
#include "message.h"

static void
DumpBrushStat(brush_t* b)
{
  int i, j;
  plane_t* cplane;

  printf("Verts:\n");
  for (i = 0; i < b->num_verts; i++)
  {
    printf("%i: %2.2f %2.2f %2.2f\n", i, b->verts[i].x, b->verts[i].y, b->verts[i].z);
  }
  printf("Edges:\n");
  for (i = 0; i < b->num_edges; i++)
  {
    printf("%i: %i %i\n", i, b->edges[i].startvertex, b->edges[i].endvertex);
  }
  printf("Planes:\n");
  for (i = 0; i < b->num_planes; i++)
  {
    printf("%i\n", i);
    cplane = &b->plane[i];
    for (j = 0; j < cplane->num_verts; j++)
    {
      printf("%i ", cplane->verts[j]);
    }
    printf("\n");
  }
  fflush(stdout);
}

static void
DeletePoint(plane_t* pl, int p)
{
  int* nverts;
  int i;
  int nnv;

  nverts = Q_malloc(sizeof(int) * (pl->num_verts - 1));
  nnv = 0;
  for (i = 0; i < p; i++)
  {
    nverts[nnv] = pl->verts[i];
    nnv++;
  }
  for (i = p + 1; i < pl->num_verts; i++)
  {
    nverts[nnv] = pl->verts[i];
    nnv++;
  }
  Q_free(pl->verts);
  pl->verts = nverts;
  pl->num_verts = nnv;
}

static void
DeletePlane(brush_t* b, int p)
{
  plane_t* nplanes;
  int i;
  int nnp;

  nplanes = Q_malloc(sizeof(plane_t) * (b->num_planes - 1));
  nnp = 0;
  for (i = 0; i < p; i++)
  {
    nplanes[nnp] = b->plane[i];
    nnp++;
  }
  for (i = p + 1; i < b->num_planes; i++)
  {
    nplanes[nnp] = b->plane[i];
    nnp++;
  }
  Q_free(b->plane);
  b->plane = nplanes;
  b->num_planes = nnp;
}

void
JoinVertices(brush_t* b)
{
  int i, j, k, l;
  edge_t* nedges;
  vec3_t* nverts;
  plane_t* cplane;
  int nne;
  int nnv;
  int changed;
  int joined;

  if (b->bt->type != BR_NORMAL)
    return;

  changed = 0;
  joined = 0;
  for (i = 0; i < b->num_verts; i++)
  {
    for (j = 0; j < b->num_verts; j++)
    {
      if (j != i)
      {
        if ((fabs(b->verts[i].x - b->verts[j].x) < 0.01) &&
            (fabs(b->verts[i].y - b->verts[j].y) < 0.01) &&
            (fabs(b->verts[i].z - b->verts[j].z) < 0.01))
        {
          joined++;
          changed = 1;
          nverts = (vec3_t*)Q_malloc(sizeof(vec3_t) * b->num_verts);
          nnv = 0;
          for (k = 0; k < b->num_verts; k++)
          {
            if (k != j)
            {
              nverts[nnv] = b->verts[k];
              nnv++;
            }
          }
          Q_free(b->verts);
          b->verts = (vec3_t*)Q_malloc(sizeof(vec3_t) * nnv);
          for (k = 0; k < nnv; k++)
          {
            b->verts[k] = nverts[k];
          }
          Q_free(nverts);
          b->num_verts = nnv;

          nedges = (edge_t*)Q_malloc(sizeof(edge_t) * b->num_edges);
          nne = 0;
          for (k = 0; k < b->num_edges; k++)
          {
            if (!(((b->edges[k].startvertex == i) || (b->edges[k].startvertex == j)) &&
                  ((b->edges[k].endvertex == i) || (b->edges[k].endvertex == j))))
            {
              nedges[nne] = b->edges[k];
              if (nedges[nne].startvertex == j)
                nedges[nne].startvertex = i;
              if (nedges[nne].endvertex == j)
                nedges[nne].endvertex = i;
              if (nedges[nne].startvertex > j)
                nedges[nne].startvertex--;
              if (nedges[nne].endvertex > j)
                nedges[nne].endvertex--;
              nne++;
            }
          }
          Q_free(b->edges);
          b->edges = (edge_t*)Q_malloc(sizeof(edge_t) * nne);
          for (k = 0; k < nne; k++)
          {
            b->edges[k] = nedges[k];
          }
          Q_free(nedges);
          b->num_edges = nne;

          for (k = 0; k < b->num_planes; k++)
          {
            cplane = &b->plane[k];
            for (l = 0; l < cplane->num_verts; l++)
            {
              if (cplane->verts[l] == j)
                cplane->verts[l] = i;
              if (cplane->verts[l] > j)
                cplane->verts[l]--;
            }
            if (cplane->verts[0] == i && cplane->verts[cplane->num_verts - 1] == i)
            {
              DeletePoint(cplane, cplane->num_verts - 1);
            }
            for (l = 0; l < cplane->num_verts - 1; l++)
            {
              if (cplane->verts[l] == cplane->verts[l + 1])
              {
                DeletePoint(cplane, l + 1);
                l--;
              }
            }
          }
          for (k = 0; k < b->num_planes; k++)
          {
            cplane = &b->plane[k];
            if (cplane->num_verts < 3)
            {
              DeletePlane(b, k);
              k--;
            }
          }
        }
      }
    }
  }
  if (changed)
  {
    NewMessage("%i vertices joined.", joined);

    ClearSelVerts();

    Q_free(b->tverts);
    b->tverts = Q_malloc(b->num_verts * sizeof(vec3_t));
    Q_free(b->sverts);
    b->sverts = Q_malloc(b->num_verts * sizeof(svec_t));

    RecalcAllNormals();
    CalcBrushCenter(b);
    UpdateAllViewports();
  }
}
