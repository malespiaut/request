#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "map.h"

#include "3d.h"
#include "brush.h"
#include "bsp.h"
#include "edbrush.h"
#include "edent.h"
#include "edface.h"
#include "edvert.h"
#include "geom.h"
#include "memory.h"
#include "quest.h"
#include "status.h"
#include "undo.h"


float SnapPointToGrid(float pt)
{
	if (pt>0)
	{
		return floor(pt / status.snap_size + 0.5) * status.snap_size;
	}
	if (pt<0)
	{
		return  ceil(pt / status.snap_size - 0.5) * status.snap_size;
	}
	return pt;
}


int VertsAreEqual(vec3_t one,vec3_t two)
{
   if ((fabs(one.x - two.x) < 0.1) &&
       (fabs(one.y - two.y) < 0.1) &&
       (fabs(one.z - two.z) < 0.1))
      return 1;
   else
      return 0;
}

int EdgesAreEqual(edge_t one,edge_t two)
{
   if ((one.startvertex == two.startvertex) &&
       (one.endvertex   == two.endvertex))
      return 1;

   if ((one.startvertex == two.endvertex) &&
       (one.endvertex   == two.startvertex))
      return 1;

   return 0;
}

void CalcBrushCenter(brush_t *b)
{
   int i, j;

   b->center.x = 0;
   b->center.y = 0;
   b->center.z = 0;

   for (i=0; i<b->num_verts; i++)
   {
      b->center.x += b->verts[i].x;
      b->center.y += b->verts[i].y;
      b->center.z += b->verts[i].z;
   }
   b->center.x /= b->num_verts;
   b->center.y /= b->num_verts;
   b->center.z /= b->num_verts;

   for (i=0; i<b->num_planes; i++)
   {
      b->plane[i].center.x = 0;
      b->plane[i].center.y = 0;
      b->plane[i].center.z = 0;
      for (j=0; j<b->plane[i].num_verts; j++)
      {
         b->plane[i].center.x += b->verts[b->plane[i].verts[j]].x;
         b->plane[i].center.y += b->verts[b->plane[i].verts[j]].y;
         b->plane[i].center.z += b->verts[b->plane[i].verts[j]].z;
      }
      b->plane[i].center.x /= b->plane[i].num_verts;
      b->plane[i].center.y /= b->plane[i].num_verts;
      b->plane[i].center.z /= b->plane[i].num_verts;
   }
}

void UnloadMap(void)
{
   int i;
   brush_t  *b;
   entity_t *e;
   group_t *g;

   ClearUndo();
   FreePts();

   for (g=M.GroupHead; g; g=M.GroupHead)
   {
      M.GroupHead=g->Next;
      Q_free(g);
   }
   M.GroupHead=NULL;
   M.WorldGroup=NULL;
   M.CurGroup=NULL;
   M.num_groups=0;

   ClearSelVerts();
   ClearSelBrushes();
   ClearSelEnts();
   ClearSelFaces();
   
   for (; (b=M.BrushHead) ; )
   {
      B_Unlink(b);
      B_Free(b);
   }
   M.BrushHead = NULL;
   M.num_brushes = 0;

   for (e=M.EntityHead; e; e=M.EntityHead)
   {
      M.EntityHead = e->Next;

      for (i=0;i<e->numkeys;i++)
      {
         Q_free(e->key[i]);
         Q_free(e->value[i]);
      }

      Q_free(e);
   }
   M.EntityHead = NULL;
   M.WorldSpawn = NULL;
   M.num_entities = 0;

   M.num_vertices = 0;
   M.num_edges = 0;

   M.modified=0;

   if (M.BSPHead!=NULL) DeleteBSPTree();
}


void RecalcNormals(brush_t *b)
{
	int      j, k;
	int      k_next, k_prev, k_cur;
	plane_t  *p;
	vec3_t    vec1, vec2, norm;
	vec3_t    *vertarray;

	for (j=0; j<b->num_planes; j++)
	{
		p = &(b->plane[j]);
		vertarray = b->verts;

		norm.x = norm.y = norm.z = 0;
		k = 1;
		while ((norm.x == 0) &&
		       (norm.y == 0) &&
		       (norm.z == 0) &&
		       (k <= p->num_verts))
      {

			k_cur = k % p->num_verts;
			k_next = (k + 1)% p->num_verts;
			k_prev = (k + p->num_verts - 1) % p->num_verts;

			vec1.x = vertarray[p->verts[k_cur]].x - vertarray[p->verts[k_prev]].x;
			vec1.y = vertarray[p->verts[k_cur]].y - vertarray[p->verts[k_prev]].y;
			vec1.z = vertarray[p->verts[k_cur]].z - vertarray[p->verts[k_prev]].z;

			vec2.x = vertarray[p->verts[k_cur]].x - vertarray[p->verts[k_next]].x;
			vec2.y = vertarray[p->verts[k_cur]].y - vertarray[p->verts[k_next]].y;
			vec2.z = vertarray[p->verts[k_cur]].z - vertarray[p->verts[k_next]].z;

         CrossProd(vec1, vec2, &norm);
			Normalize(&norm);

			k++;
		}

		if (k <= p->num_verts)
		{
			p->normal.x = norm.x;
			p->normal.y = norm.y;
			p->normal.z = norm.z;
			p->dist = DotProd(p->normal,vertarray[p->verts[0]]);
		}
		else
		{
			p->normal.x = 0;
			p->normal.y = 0;
			p->normal.z = 0;
			p->dist     = 0;
		}
	}
}

void RecalcAllNormals(void)
{
	brush_t *b;

	for (b=M.BrushHead; b; b=b->Next)
		RecalcNormals(b);
}

