#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


#include "defines.h"
#include "types.h"


#include "3d.h"
#include "error.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "newgroup.h"
#include "quest.h"
#include "texdef.h"
#include "token.h"


/* TODO : fix this? only useful if you know how vis/qvis3/etc. work, needs to
   be updated for new brush handling stuff */

#define GETTOKEN(x) \
   if (!TokenGet(x,-1)) Abort("LoadPortals","Error!")

void LoadPortals(char *name)
{
   brush_t *b;
   int num_portals;
   int i,j,k;
   int num_v;

   if (!TokenFile(name,T_NAME|T_MISC|T_NUMBER,NULL))
   {
      HandleError("LoadPortals","Can't open file '%s'!",name);
      return;
   }

//   GETTOKEN(1);
   if (!TokenGet(1,T_ALLNAME)) Abort("LoadPortals","Parse error!");

   if (strcmp(token,"PRT1"))
      if (strcmp(token,"PRT1-AM"))
         Abort("LoadPortals","PRT1 or PRT1-AM expected!");

   b=Q_malloc(sizeof(brush_t));
   if (!b) Abort("LoadPortals","Out of memory!");
   memset(b,0,sizeof(brush_t));

   b->Group=FindVisGroup(M.WorldGroup);
   b->EntityRef=M.WorldSpawn;
   b->Next=M.BrushHead;
   if (M.BrushHead) M.BrushHead->Last=b;
   M.BrushHead=b;

   GETTOKEN(1);
   GETTOKEN(1);
   num_portals=atoi(token);
   GETTOKEN(1);   // new field for detail brush .prt files

   b->plane=Q_malloc(sizeof(plane_t)*num_portals);
   if (!b->plane) Abort("!!!","!!!");
   memset(b->plane,0,sizeof(plane_t)*num_portals);
   b->num_planes=num_portals;

   for (i=0;i<num_portals;i++)
   {
      GETTOKEN(1);
      num_v=atoi(token);

      b->verts=Q_realloc(b->verts,sizeof(vec3_t)*(b->num_verts+num_v));
      if (!b->verts) Abort("!!!","!!!");
      memset(&b->verts[b->num_verts],0,sizeof(vec3_t)*num_v);

      b->edges=Q_realloc(b->edges,sizeof(edge_t)*(b->num_edges+num_v));
      if (!b->edges) Abort("!!!","!!!");
      memset(&b->edges[b->num_edges],0,sizeof(edge_t)*num_v);

      GETTOKEN(1);
      GETTOKEN(1);

      InitTexdef(&b->plane[i].tex);

      b->plane[i].num_verts=num_v;
      b->plane[i].verts=Q_malloc(sizeof(int)*num_v);
      memset(b->plane[i].verts,0,sizeof(int)*num_v);

      j=b->num_verts;
      k=0;
      for (;num_v;num_v--,k++)
      {
         GETTOKEN(1);

         GETTOKEN(1);
         b->verts[b->num_verts].x=atof(token);
         GETTOKEN(1);
         b->verts[b->num_verts].y=atof(token);
         GETTOKEN(1);
         b->verts[b->num_verts].z=atof(token);

         b->edges[b->num_edges].startvertex=b->num_verts;
         if (num_v>1)
            b->edges[b->num_edges].endvertex=b->num_verts+1;
         else
            b->edges[b->num_edges].endvertex=j;

         b->plane[i].verts[k]=b->num_verts;

         b->num_verts++;
         b->num_edges++;

         GETTOKEN(1);
      }
   }

   b->tverts=Q_malloc(sizeof(vec3_t)*b->num_verts);
   b->sverts=Q_malloc(sizeof(vec3_t)*b->num_verts);
   if (!b->tverts || !b->sverts) Abort("!!!","!!!");

   RecalcNormals(b);
   CalcBrushCenter(b);

   printf("%i verts, %i edges, %i faces\n",b->num_verts,b->num_edges,b->num_planes);
   NewMessage("%i verts, %i edges, %i faces\n",b->num_verts,b->num_edges,b->num_planes);
   NewMessage("center at (%g %g %g)\n",b->center.x,b->center.y,b->center.z);

   UpdateAllViewports();

   TokenDone();
}

