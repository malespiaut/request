/*
weld.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "weld.h"

#include "3d.h"
#include "brush.h"
#include "check.h"
#include "clip.h"
#include "edbrush.h"
#include "edvert.h"
#include "error.h"
#include "geom.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "quest.h"
#include "undo.h"


static int WeldBrushes(brushref_t *br1,brushref_t *br2)
{
   int      i,j,k,l;
   plane_t *p1,*p2;
   int      w1,w2;
   brush_t *b1,*b2;

   b1=br1->Brush;
   b2=br2->Brush;

   if (b1->EntityRef!=b2->EntityRef) return 0;

   { // find the common face (could probably be made smarter)
      j=-1; // stop compiler warning
      for (i=0;i<b1->num_planes;i++)
      {
         p1=&b1->plane[i];
         for (j=0;j<b2->num_planes;j++)
         {
            p2=&b2->plane[j];
            if (p1->num_verts!=p2->num_verts)
               continue;

            if (fabs(p1->dist+p2->dist)>0.01)
               continue;

            if ((fabs(p1->normal.x+p2->normal.x)>0.01) ||
                (fabs(p1->normal.y+p2->normal.y)>0.01) ||
                (fabs(p1->normal.z+p2->normal.z)>0.01))
               continue;

            for (k=0;k<p1->num_verts;k++)
            {
               for (l=0;l<p1->num_verts;l++)
               {
                  if ((fabs(b1->verts[p1->verts[k]].x-b2->verts[p2->verts[l]].x)<0.01) &&
                      (fabs(b1->verts[p1->verts[k]].y-b2->verts[p2->verts[l]].y)<0.01) &&
                      (fabs(b1->verts[p1->verts[k]].z-b2->verts[p2->verts[l]].z)<0.01))
                     break;
               }
               if (l==p1->num_verts) break;
            }
            if (k==p1->num_verts)
               break;
         }
         if (j!=b2->num_planes)
         {
            break;
         }
      }
      if (i==b1->num_planes)
      {
//         NewMessage("No common face!");
         return 0;
      }
      w1=i;
      w2=j;
   }

   { // check resulting brush
   	vec3_t   delta;
   	vec3_t   center1, center2;

   	for (i=0;i<b1->num_planes;i++)
   	{
         if (i==w1)
            continue;

         p1=&b1->plane[i];
   		/* Create centerpoint for plane */
   		center1.x = center1.y = center1.z = 0;
   		for (j=0; j<p1->num_verts; j++)
   		{
   			center1.x += b1->verts[p1->verts[j]].x;
   			center1.y += b1->verts[p1->verts[j]].y;
   			center1.z += b1->verts[p1->verts[j]].z;
   		}
   		center1.x /= p1->num_verts;
   		center1.y /= p1->num_verts;
   		center1.z /= p1->num_verts;

   		for (j=0;j<b2->num_planes;j++)
   		{
            if (j==w2)
               continue;

            p2=&b2->plane[j];
   			/* Create centerpoint for other plane */
   			center2.x = center2.y = center2.z = 0;
   			for (k=0; k<p2->num_verts; k++)
   			{
   				center2.x += b2->verts[p2->verts[k]].x;
   				center2.y += b2->verts[p2->verts[k]].y;
   				center2.z += b2->verts[p2->verts[k]].z;
   			}
   			center2.x /= p2->num_verts;
   			center2.y /= p2->num_verts;
   			center2.z /= p2->num_verts;
   
   			/* Create vector from center1 to center2 */
   			delta.x = center2.x - center1.x;
   			delta.y = center2.y - center1.y;
   			delta.z = center2.z - center1.z;
   
   			/* If the dot prod of this delta and center2's normal
   				is less than 0 (with some epsilon), it's not convex */
   			if (DotProd(delta, p2->normal) < -.015)
            {
//               NewMessage("Result not convex!");
               return 0;
            }
   		}
   	}
   }

   { // check for identical planes with different textures
      for (i=0;i<b1->num_planes;i++)
      {
         p1=&b1->plane[i];
         for (j=0;j<b2->num_planes;j++)
         {
            p2=&b2->plane[j];

            if (p1->dist!=p2->dist)
               continue;

            if ((p1->normal.x!=p2->normal.x) ||
                (p1->normal.y!=p2->normal.y) ||
                (p1->normal.z!=p2->normal.z))
               continue;

            if (strcmp(p1->tex.name,p2->tex.name))
            {
/*               NewMessage(">1 texture/plane (%g %g %g) %g (%s and %s)",
                  p1->normal.x,p1->normal.y,p1->normal.z,
                  p1->dist,
                  p1->tex.name,p2->tex.name);*/
               return 0;
            }
         }
      }
   }

   { // weld the brushes!
      brush_my_t t;
      brush_t *b;

      t.numfaces=b1->num_planes+b2->num_planes-2;
      t.faces=Q_malloc(sizeof(face_my_t)*t.numfaces);
      if (!t.faces)
      {
         HandleError("WeldBrushes","Out of memory!");
         return -1;
      }

      j=0;
      for (i=0;i<b1->num_planes;i++)
      {
         if (i==w1)
            continue;

         t.faces[j].misc=t.faces[j].numpts=0;
         t.faces[j].normal=b1->plane[i].normal;
         t.faces[j].dist  =b1->plane[i].dist;
         t.faces[j].tex   =b1->plane[i].tex;
         j++;
      }

      for (i=0;i<b2->num_planes;i++)
      {
         if (i==w2)
            continue;

         t.faces[j].misc=t.faces[j].numpts=0;
         t.faces[j].normal=b2->plane[i].normal;
         t.faces[j].dist  =b2->plane[i].dist;
         t.faces[j].tex   =b2->plane[i].tex;
         j++;
      }

      b=B_New(BR_NORMAL);
      if (!b)
      {
         Q_free(t.faces);
         HandleError("WeldBrushes","Out of memory!");
         return -1;
      }

      if (!BuildBrush(&t,b))
      {
         HandleError("WeldBrushes","Error building brush!");
         return -1;
      }
      
      b->EntityRef = b1->EntityRef;
      b->Group = b1->Group;

      B_Link(b);

      Q_free(t.faces);

      br1->Brush=b;
      RemoveSelBrush(br2);
      DeleteABrush(b2);
      DeleteABrush(b1);
   }

   return 1;
}

void Weld(void)
{
   int i;
   int welds;
   brushref_t *br1,*br2;

   for (br1=M.display.bsel;br1;br1=br1->Next)
   {
      if (!CheckBrush(br1->Brush,1))
         return;
      if (br1->Brush->bt->type!=BR_NORMAL)
      {
         HandleError("Weld","Can only weld normal brushes!");
         return;
      }
   }

   i=0;
   welds=0;

   SUndo(UNDO_NONE,UNDO_DELETE);

   for (br1=M.display.bsel;br1;br1=br1->Next)
   {
      br2=br1->Next;
restart_weld:
      for (;br2;br2=br2->Next)
      {
         if (br1==br2)
            continue;

         i=WeldBrushes(br1,br2);
         if (i==1)
         {
            welds++;
            br2=M.display.bsel;
            goto restart_weld;
         }
         if (i==-1)
         {
            HandleError("Weld","Error welding!");
            UpdateAllViewports();
            return;
         }
      }
   }

   for (br1=M.display.bsel;br1;br1=br1->Next)
   {
      AddDBrush(br1->Brush);
   }

   NewMessage("%i welds done.",welds);
   if (welds)
      ClearSelVerts();

   UpdateAllViewports();
}

