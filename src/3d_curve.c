#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "3d_curve.h"

#include "3d.h"
#include "brush.h"
#include "color.h"
#include "error.h"
#include "geom.h"
#include "memory.h"
#include "quest.h"
#include "video.h"


/*
 TODO : description incorrect, code has correct version, optimize code

Random notes on Quake 3 curves:

 Each 'bezier' brush consists on a rectangular array of bezier patches.
All corners are shared with neigbouring patches, as are edge control points.
All control points are paired, ie:

0---1---2---3---4
|   |   |   |   |
|   |   |   |   |
5---6---7---8---9
|   |   |   |   |
|   |   |   |   |
a---b---c---d---e

0,1,2,5,6,7,a,b,c make one bezier patch, the rest is another. To get a 'proper'
bezier patch, use the vertices like:

0--1--1--2
|  |  |  |
5--6--6--7
|  |  |  |
5--6--6--7
|  |  |  |
a--b--b--c

The tools may be able to handle more complex bezier patch collections, so I'm
going to try to keep stuff flexible here.

*/



#define VECADD2(a,b,c) \
   (\
      (a).x=((b).x+(c).x)/2, \
      (a).y=((b).y+(c).y)/2, \
      (a).z=((b).z+(c).z)/2 \
   )



static int color;
static int c_vport;


#define CURVE_LEN 17


static void SplitCurve_r(vec3_t p1,vec3_t p2,vec3_t p3,vec3_t p4,vec3_t *dest,int len)
{
   vec3_t p12,p23,p34;
   vec3_t p123,p234;
   vec3_t p1234;

   if (len<=2)
      return;

   VECADD2(p12,p1,p2);
   VECADD2(p23,p2,p3);
   VECADD2(p34,p3,p4);

   VECADD2(p123,p12,p23);
   VECADD2(p234,p23,p34);

   VECADD2(p1234,p123,p234);

/*   printf("len %i, split between (%g %g %g) and (%g %g %g), (%g %g %g)\n",
      len,
      p1.x,p1.y,p1.z,
      p4.x,p4.y,p4.z,
      p1234.x,p1234.y,p1234.z);*/

   dest[len/2]=p1234;

   SplitCurve_r(p1,p12,p123,p1234,&dest[0]    ,len/2+1);
   SplitCurve_r(p1234,p234,p34,p4,&dest[len/2],len/2+1);
}

static void SplitCurve(vec3_t p1,vec3_t p2,vec3_t p3,vec3_t p4,vec3_t *dest,int len)
{
   dest[0]=p1;
   dest[len-1]=p4;
   SplitCurve_r(p1,p2,p3,p4,dest,len);
}


static void DrawCurve(vec3_t p1,vec3_t p2,vec3_t p3,vec3_t p4,int len)
{
   int i;
   viewport_t *vp=&M.display.vport[c_vport];
   int dx,dy;
   float clipx,clipy;
   float x0,y0,x1,y1;
   int xs0,ys0,xs1,ys1;

   vec3_t *points;


   points=Q_malloc(sizeof(vec3_t)*len);
   if (!points)
      return;

   SplitCurve(p1,p2,p3,p4,points,len);

   for (i=0;i<len-1;i++)
   {
//      printf("p(%i) at (%g %g %g)\n",i,points[i].x,points[i].y,points[i].z);

		switch (M.display.vport[c_vport].mode)
		{
			case NOPERSP:
				dx=(vp->xmax-vp->xmin) >>1;
				dy=(vp->ymax-vp->ymin) >>1;

				clipx = dx / vp->zoom_amt;
				clipy = dy / vp->zoom_amt;

				x0 =  points[i  ].x * vp->zoom_amt + vp->xmin + dx;
				y0 = -points[i  ].y * vp->zoom_amt + vp->ymin + dy;
				x1 =  points[i+1].x * vp->zoom_amt + vp->xmin + dx;
				y1 = -points[i+1].y * vp->zoom_amt + vp->ymin + dy;

				xs0 = x0+0.5;
				ys0 = y0+0.5;
				xs1 = x1+0.5;
				ys1 = y1+0.5;

            ClipDrawLine2D(c_vport,xs0,ys0,xs1,ys1,color);
				break;

			case WIREFRAME:
         case SOLID:
				ClipDrawLine3D(c_vport,
								points[i  ].x,
								points[i  ].y,
								points[i  ].z,
								points[i+1].x,
								points[i+1].y,
								points[i+1].z,
								color);
				break;
      }
   }
   Q_free(points);
//   printf("p(%i) at (%g %g %g)\n",i,points[i].x,points[i].y,points[i].z);
}


#if 0
static void CurveMesh_DrawLine(brush_t *b,plane_t *p,int col,int v1,int v2)
{
   int dx,dy;
   float clipx,clipy;
   float x0,y0,x1,y1;
   int xs0,ys0,xs1,ys1;
   viewport_t *vp=&M.display.vport[c_vport];
   vec3_t *p1,*p2;

   v1=p->verts[v1];
   v2=p->verts[v2];
   p1=&b->tverts[v1];
   p2=&b->tverts[v2];

   switch (M.display.vport[c_vport].mode)
   {
   	case NOPERSP:
   		dx=(vp->xmax-vp->xmin) >>1;
   		dy=(vp->ymax-vp->ymin) >>1;

   		clipx = dx / vp->zoom_amt;
   		clipy = dy / vp->zoom_amt;

   		x0 =  p1->x * vp->zoom_amt + vp->xmin + dx;
   		y0 = -p1->y * vp->zoom_amt + vp->ymin + dy;
   		x1 =  p2->x * vp->zoom_amt + vp->xmin + dx;
   		y1 = -p2->y * vp->zoom_amt + vp->ymin + dy;
   
   		xs0 = x0+0.5;
   		ys0 = y0+0.5;
   		xs1 = x1+0.5;
   		ys1 = y1+0.5;

         ClipDrawLine2D(c_vport,xs0,ys0,xs1,ys1,col);
   		break;

   	case WIREFRAME:
      case SOLID:
   		ClipDrawLine3D(c_vport,
   						p1->x,
   						p1->y,
   						p1->z,
   						p2->x,
   						p2->y,
   						p2->z,
   						col);
   		break;
   }
}
#endif



static void DrawCurvePlane(brush_t *b,plane_t *p,int len,int splits)
{
   int i;

#define MAX_SPLITS 9
   vec3_t points[4][MAX_SPLITS];
   vec3_t patch[4][4];

   patch[0][0]=b->tverts[p->verts[  0]];
   patch[0][1]=b->tverts[p->verts[  1]];
   patch[0][2]=b->tverts[p->verts[  1]];
   patch[0][3]=b->tverts[p->verts[  2]];

   patch[1][0]=b->tverts[p->verts[3+0]];
   patch[1][1]=b->tverts[p->verts[3+1]];
   patch[1][2]=b->tverts[p->verts[3+1]];
   patch[1][3]=b->tverts[p->verts[3+2]];

   patch[2][0]=b->tverts[p->verts[3+0]];
   patch[2][1]=b->tverts[p->verts[3+1]];
   patch[2][2]=b->tverts[p->verts[3+1]];
   patch[2][3]=b->tverts[p->verts[3+2]];

   patch[3][0]=b->tverts[p->verts[6+0]];
   patch[3][1]=b->tverts[p->verts[6+1]];
   patch[3][2]=b->tverts[p->verts[6+1]];
   patch[3][3]=b->tverts[p->verts[6+2]];

#define MID(a,b,c) \
   (a).x=((b).x+(c).x)/2; \
   (a).y=((b).y+(c).y)/2; \
   (a).z=((b).z+(c).z)/2;

   MID(patch[0][1],patch[0][0],patch[0][1])
   MID(patch[0][2],patch[0][3],patch[0][2])

   MID(patch[1][1],patch[1][0],patch[1][1])
   MID(patch[1][2],patch[1][3],patch[1][2])

   MID(patch[2][1],patch[2][0],patch[2][1])
   MID(patch[2][2],patch[2][3],patch[2][2])

   MID(patch[3][1],patch[3][0],patch[3][1])
   MID(patch[3][2],patch[3][3],patch[3][2])


   MID(patch[1][0],patch[0][0],patch[1][0])
   MID(patch[2][0],patch[3][0],patch[2][0])

   MID(patch[1][1],patch[0][1],patch[1][1])
   MID(patch[2][1],patch[3][1],patch[2][1])

   MID(patch[1][2],patch[0][2],patch[1][2])
   MID(patch[2][2],patch[3][2],patch[2][2])

   MID(patch[1][3],patch[0][3],patch[1][3])
   MID(patch[2][3],patch[3][3],patch[2][3])


   for (i=0;i<4;i++)
      SplitCurve(
         patch[i][0],
         patch[i][1],
         patch[i][2],
         patch[i][3],
         points[i],splits);
   for (i=0;i<splits;i++)
      DrawCurve(
         points[0][i],
         points[1][i],
         points[2][i],
         points[3][i],
         len);

   for (i=0;i<4;i++)
      SplitCurve(
         patch[0][i],
         patch[1][i],
         patch[2][i],
         patch[3][i],
         points[i],splits);
   for (i=0;i<splits;i++)
      DrawCurve(
         points[0][i],
         points[1][i],
         points[2][i],
         points[3][i],
         len);
}


void DrawBrush_Q3Curve(int vport,brush_t *b,int col,int selected)
{
   int i;
   plane_t *p;

   color=col;
   c_vport=vport;

   for (i=0;i<b->num_verts;i++)
      if (b->sverts[i].onscreen)
         break;
   /* If no corners or control points are on the screen, we'll assume
   the patches can't be seen. */
   if (i==b->num_verts) return;

   for (i=0,p=b->plane;i<b->num_planes;i++,p++)
   {
      if (selected)
         DrawCurvePlane(b,p,9,5);
      else
         DrawCurvePlane(b,p,3,3);
   }


   /* If it's selected, draw a 'normal' for the first patch. */
   if (selected && M.display.vport[c_vport].mode==NOPERSP)
   {
      vec3_t norm,v1,v2;

      p=&b->plane[0];

      v1.x=b->tverts[p->verts[0]].x - b->tverts[p->verts[2]].x;
      v1.y=b->tverts[p->verts[0]].y - b->tverts[p->verts[2]].y;
      v1.z=b->tverts[p->verts[0]].z - b->tverts[p->verts[2]].z;
      
      v2.x=b->tverts[p->verts[0]].x - b->tverts[p->verts[6]].x;
      v2.y=b->tverts[p->verts[0]].y - b->tverts[p->verts[6]].y;
      v2.z=b->tverts[p->verts[0]].z - b->tverts[p->verts[6]].z;

      CrossProd(v1,v2,&norm);
      Normalize(&norm);

      v1=b->tverts[p->verts[0]];
      v2.x=v1.x+norm.x*8;
      v2.y=v1.y+norm.y*8;
      v2.z=v1.z+norm.z*8;
      col=GetColor(COL_WHITE);

      {
         int dx,dy;
         float x0,y0,x1,y1;
         int xs0,ys0,xs1,ys1;
         viewport_t *vp=&M.display.vport[c_vport];
      
         dx=(vp->xmax-vp->xmin) >>1;
         dy=(vp->ymax-vp->ymin) >>1;
      
         x0 =  v1.x * vp->zoom_amt + vp->xmin + dx;
         y0 = -v1.y * vp->zoom_amt + vp->ymin + dy;
         x1 =  v2.x * vp->zoom_amt + vp->xmin + dx;
         y1 = -v2.y * vp->zoom_amt + vp->ymin + dy;
         
         xs0 = x0+0.5;
         ys0 = y0+0.5;
         xs1 = x1+0.5;
         ys1 = y1+0.5;
      
         ClipDrawLine2D(c_vport,xs0,ys0,xs1,ys1,col);
      }
   }
}


void CurveGetPoints(brush_t *b,vec3_t *bverts,plane_t *p,
   vec3_t *dpoints,vec3_t *tpoints,int splits)
{
   int i;

   vec3_t *(points[4]);
   vec3_t patch[4][4];

   vec3_t *v;
   int *vi;

   for (i=0;i<4;i++)
   {
      points[i]=Q_malloc(sizeof(vec3_t)*splits);
      if (!points[i])
      {
         for (;i>=0;i--)
            Q_free(points[i]);
         HandleError("CurveGetPoints","Out of memory!");
         return;
      }
   }

   patch[0][0]=bverts[p->verts[  0]];
   patch[0][1]=bverts[p->verts[  1]];
   patch[0][3]=bverts[p->verts[  2]];

   patch[1][0]=bverts[p->verts[3+0]];
   patch[1][1]=bverts[p->verts[3+1]];
   patch[1][3]=bverts[p->verts[3+2]];

   patch[2][0]=bverts[p->verts[3+0]];
   patch[2][1]=bverts[p->verts[3+1]];
   patch[2][3]=bverts[p->verts[3+2]];

   patch[3][0]=bverts[p->verts[6+0]];
   patch[3][1]=bverts[p->verts[6+1]];
   patch[3][3]=bverts[p->verts[6+2]];


   MID(patch[0][2],patch[0][3],patch[0][1])
   MID(patch[0][1],patch[0][0],patch[0][1])

   MID(patch[1][2],patch[1][3],patch[1][1])
   MID(patch[1][1],patch[1][0],patch[1][1])

   MID(patch[2][2],patch[2][3],patch[2][1])
   MID(patch[2][1],patch[2][0],patch[2][1])

   MID(patch[3][2],patch[3][3],patch[3][1])
   MID(patch[3][1],patch[3][0],patch[3][1])


   MID(patch[1][0],patch[0][0],patch[1][0])
   MID(patch[2][0],patch[3][0],patch[2][0])

   MID(patch[1][1],patch[0][1],patch[1][1])
   MID(patch[2][1],patch[3][1],patch[2][1])

   MID(patch[1][2],patch[0][2],patch[1][2])
   MID(patch[2][2],patch[3][2],patch[2][2])

   MID(patch[1][3],patch[0][3],patch[1][3])
   MID(patch[2][3],patch[3][3],patch[2][3])


   for (i=0;i<4;i++)
      SplitCurve(
         patch[i][0],
         patch[i][1],
         patch[i][2],
         patch[i][3],
         points[i],splits);
   for (i=0;i<splits;i++)
      SplitCurve(
         points[0][i],
         points[1][i],
         points[2][i],
         points[3][i],
         &dpoints[splits*i],splits);


   if (!tpoints)
   {
      for (i=0;i<4;i++)
         Q_free(points[i]);
      return;
   }

/*
Now calculate the texture coordinates
*/
   for (v=&patch[0][0],i=0,vi=p->verts;i<12;i++,v++,vi++)
   {
      v->x=b->x.q3c.s[*vi];
      v->y=b->x.q3c.t[*vi];
      v->z=0;

      if ((i%3)==1) v++;
      if (i==5)
         vi-=3;
   }


   MID(patch[0][2],patch[0][3],patch[0][1])
   MID(patch[0][1],patch[0][0],patch[0][1])

   MID(patch[1][2],patch[1][3],patch[1][1])
   MID(patch[1][1],patch[1][0],patch[1][1])

   MID(patch[2][2],patch[2][3],patch[2][1])
   MID(patch[2][1],patch[2][0],patch[2][1])

   MID(patch[3][2],patch[3][3],patch[3][1])
   MID(patch[3][1],patch[3][0],patch[3][1])


   MID(patch[1][0],patch[0][0],patch[1][0])
   MID(patch[2][0],patch[3][0],patch[2][0])

   MID(patch[1][1],patch[0][1],patch[1][1])
   MID(patch[2][1],patch[3][1],patch[2][1])

   MID(patch[1][2],patch[0][2],patch[1][2])
   MID(patch[2][2],patch[3][2],patch[2][2])

   MID(patch[1][3],patch[0][3],patch[1][3])
   MID(patch[2][3],patch[3][3],patch[2][3])


   for (i=0;i<4;i++)
      SplitCurve(
         patch[i][0],
         patch[i][1],
         patch[i][2],
         patch[i][3],
         points[i],splits);
   for (i=0;i<splits;i++)
      SplitCurve(
         points[0][i],
         points[1][i],
         points[2][i],
         points[3][i],
         &tpoints[splits*i],splits);

   for (i=0;i<4;i++)
      Q_free(points[i]);
}

