#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "check.h"

#include "3d.h"
#include "brush.h"
#include "edbrush.h"
#include "error.h"
#include "geom.h"
#include "memory.h"
#include "message.h"
#include "quest.h"


static int IsOnPlane(vec3_t norm, float d, vec3_t pt)
{
	return (fabs(norm.x * pt.x + norm.y * pt.y + norm.z * pt.z + d) < 0.015);
}

int CheckBrush(brush_t *b, int verbose)
{
	int      i, j, k;
	vec3_t   normal;
	vec3_t   delta;
	vec3_t   center1, center2;
	float    dist;

#define Err(x) \
   { \
      if (verbose) \
         HandleError("CheckBrush",x); \
      else \
         NewMessage(x); \
      return FALSE; \
   }

/* TODO : some tests apply to other brush types */
   if (b->bt->type!=BR_NORMAL) return TRUE;

   if ((b->num_planes<4) || (b->num_verts<4))
   {
      Err("Degenerated brush!");
   }

   for (i=0;i<b->num_edges;i++)
   {
      vec3_t v1,v2;
      edge_t *e;

      e=&b->edges[i];

      v1=b->verts[e->startvertex];
      v2=b->verts[e->endvertex];
      delta.x=v2.x-v1.x;
      delta.y=v2.y-v1.y;
      delta.z=v2.z-v1.z;

      if ((fabs(delta.x)<0.0001) &&
          (fabs(delta.y)<0.0001) &&
          (fabs(delta.z)<0.0001))
      {
/*	      for (i=0; i<b->num_planes; i++)
         {
            printf("(%g %g %g)\n",
               b->plane[i].normal.x,
               b->plane[i].normal.y,
               b->plane[i].normal.z);
         }*/

         Err("Degenerated edge!");
      }
   }

	/* Check each face in the brush for coplanarity */
	for (i=0; i<b->num_planes; i++)
	{
      if (b->plane[i].num_verts<3)
      {
         NewMessage("plane=%i nv=%i",i,b->plane[i].num_verts);
         Err("Degenerated face!");
      }

		/* Generate normal-dist plane representation */
		normal.x = b->plane[i].normal.x;
		normal.y = b->plane[i].normal.y;
		normal.z = b->plane[i].normal.z;

      dist=normal.x*normal.x+normal.y*normal.y+normal.z*normal.z;
      if (dist<0.6)
      {
         Err("Face with no normal!");
      }

		dist = -DotProd(b->plane[i].normal, b->verts[b->plane[i].verts[0]]);

		/* Check each vertex on the plane */
		for (j=0; j<b->plane[i].num_verts; j++)
		{
			if (!IsOnPlane(normal, dist, b->verts[b->plane[i].verts[j]]))
			{
            Err("A vertex on the plane is non-coplanar");
			}
		}
	}

	/* Check for brush convexity */
	for (i=0; i<b->num_planes; i++)
	{
		/* Create centerpoint for plane */
		center1.x = center1.y = center1.z = 0;
		for (j=0; j<b->plane[i].num_verts; j++)
		{
			center1.x += b->verts[b->plane[i].verts[j]].x;
			center1.y += b->verts[b->plane[i].verts[j]].y;
			center1.z += b->verts[b->plane[i].verts[j]].z;
		}
		center1.x /= b->plane[i].num_verts;
		center1.y /= b->plane[i].num_verts;
		center1.z /= b->plane[i].num_verts;
		for (j=0; j<b->num_planes; j++)
		{
			/* Create centerpoint for other plane */
			center2.x = center2.y = center2.z = 0;
			for (k=0; k<b->plane[j].num_verts; k++)
			{
				center2.x += b->verts[b->plane[j].verts[k]].x;
				center2.y += b->verts[b->plane[j].verts[k]].y;
				center2.z += b->verts[b->plane[j].verts[k]].z;
			}
			center2.x /= b->plane[j].num_verts;
			center2.y /= b->plane[j].num_verts;
			center2.z /= b->plane[j].num_verts;

			/* Create vector from center1 to center2 */
			delta.x = center2.x - center1.x;
			delta.y = center2.y - center1.y;
			delta.z = center2.z - center1.z;

			/* If the dot prod of this delta and center2's normal
				is less than 0 (with some epsilon), it's not convex. */
			if (DotProd(delta, b->plane[j].normal) < -.015)
			{
            Err("Brush is not convex.");
			}
		}
	}

	return TRUE;
}


int ConsistencyCheck(void)
{
	brush_t *b;

	for (b=M.BrushHead;b;b=b->Next)
	{
		if (!CheckBrush(b,TRUE))
		{
			ClearSelBrushes();
			M.display.bsel = (brushref_t *)Q_malloc(sizeof(brushref_t));
         if (M.display.bsel)
         {
   			M.display.bsel->Brush=b;
   			M.display.bsel->Next = NULL;
   			M.display.bsel->Last = NULL;
   			M.display.num_bselected = 1;
   			AutoZoom();
         }
         else
            HandleError("ConsistencyCheck","Out of memory!");
			UpdateAllViewports();
			return FALSE;
		}
	}
	return TRUE;
}

