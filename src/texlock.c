/*
texlock.c file of the Quest Source Code

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

#include "texlock.h"

#include "brush.h"
#include "geom.h"
#include "status.h"
#include "tex.h"


static float baseaxis[18][3] =
{
{ 0, 0, 1},{ 1, 0, 0},{ 0,-1, 0}, // floor
{ 0, 0,-1},{ 1, 0, 0},{ 0,-1, 0}, // ceiling
{ 1, 0, 0},{ 0, 1, 0},{ 0, 0,-1}, // west wall
{-1, 0, 0},{ 0, 1, 0},{ 0, 0,-1}, // east wall
{ 0, 1, 0},{ 1, 0, 0},{ 0, 0,-1}, // south wall
{ 0,-1, 0},{ 1, 0, 0},{ 0, 0,-1}	 // north wall
};

static void TextureAxisFromPlane(plane_t *pln,float xv[3],float yv[3])
{
	int	bestaxis;
	float	dot,best;
	int	i;
	
	best = 0;
	bestaxis = 0;

	for (i=0 ; i<6 ; i++)
	{
		dot = (pln->normal.x*baseaxis[i*3][0])+
            (pln->normal.y*baseaxis[i*3][1])+
            (pln->normal.z*baseaxis[i*3][2]);
		if (dot-best>0.01)
		{
			best = dot;
			bestaxis = i;
		}
	}
	
   xv[0]=baseaxis[bestaxis*3+1][0];
   xv[1]=baseaxis[bestaxis*3+1][1];
   xv[2]=baseaxis[bestaxis*3+1][2];
   yv[0]=baseaxis[bestaxis*3+2][0];
   yv[1]=baseaxis[bestaxis*3+2][1];
   yv[2]=baseaxis[bestaxis*3+2][2];

   return;
}

static void MakeVectors(float vecs[2][3],texdef_t *t)
{
// rotate axis
	int	sv, tv;
	float	ang, sinv, cosv;
	float	ns, nt;

   float rotate;
   float scale[2];
   float shift[2];
   int i,j;

   rotate=t->rotate;
   scale[0]=t->scale[0];
   scale[1]=t->scale[1];
   shift[0]=t->shift[0];
   shift[1]=t->shift[1];

   if (fabs(scale[0])<0.001) scale[0]=1;
   if (fabs(scale[1])<0.001) scale[1]=1;

	if (rotate == 0)
   {
      sinv = 0;
      cosv = 1;
   }
	else
	if (rotate == 90)
	{
	   sinv = 1;
	   cosv = 0;
   }
	else
	if (rotate == 180)
	{
	   sinv = 0;
	   cosv = -1;
   }
	else
	if (rotate == 270)
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
   {
		sv = 0;
   }
	else if (vecs[0][1])
   {
		sv = 1;
   }
	else
   {
		sv = 2;
   }
				
	if (vecs[1][0])
   {
		tv = 0;
   }
	else if (vecs[1][1])
   {
		tv = 1;
   }
	else
   {
		tv = 2;
   }

	for (i=0 ; i<2 ; i++)
	{
		ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
		nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
		vecs[i][sv] = ns;
		vecs[i][tv] = nt;
	}

   for (i=0 ; i<2 ; i++)
		for (j=0 ; j<3 ; j++)
			vecs[i][j] = vecs[i][j] / scale[i];
}


static int orientation(float p[2],float q[2],float r[2])
{
   int i;
   float a[2],b[2];

   for (i=0;i<2;i++)
   {
      a[i]=q[i]-p[i];
      b[i]=r[i]-p[i];
   }

   return (a[0]*b[0]+a[1]*b[1]>0);
}

/*
bearing of a line joining p and q points - in degrees
don't know what axis is 0, but it is not important
*/
static float angle(float p[2],float q[2])
{
   float x,y;
   float a;

   x=p[0]-q[0];
   y=p[1]-q[1];

   if (!fabs(x) && !fabs(y))
      return 0;

   if (fabs(x)>fabs(y))
   {
      a=atan(y/x);
   }
   else
   {
      a=PI/2-atan(x/y);
   }

   return a*180/PI;
}

#define Dot(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])

/*
orig plane has points and mapping parameters filled
new plane has points only filled
orig plane is the face before manipulation
new plane is the face after manipulation
only three points of the face are needed
but the points must be corresponding ones
and must not be colinear in both cases
the result of this routine is new plane's texture mapping set
*/
static void Solution(brush_t *orig_b,plane_t *orig_p,brush_t *new_b,plane_t *new_p)
{
/* all variants of selecting two points of three */
static int pset[3][2]={{0,1},{0,2},{1,2}};

   float orig_v[2][3];
   float  new_v[2][3],new_v1[2][3];

   int i,j;

   float p,val;

   int best;

   float orig_st[3][2];
   float orig_c[3][3];

   float new_st[3][2];
   float new_c[3][3];

/*
calculate S and T for orig_p - you also may have these values calculated
we will not change them
*/
   if (!orig_p->normal.x &&
       !orig_p->normal.y &&
       !orig_p->normal.z)
      return;

   TextureAxisFromPlane(orig_p,orig_v[0],orig_v[1]);
   MakeVectors(orig_v,&orig_p->tex);

   for (i=0;i<3;i++)
   {
      orig_c[i][0]=orig_b->verts[orig_p->verts[i]].x;
      orig_c[i][1]=orig_b->verts[orig_p->verts[i]].y;
      orig_c[i][2]=orig_b->verts[orig_p->verts[i]].z;

      orig_st[i][0]=Dot(orig_v[0],orig_c[i])+orig_p->tex.shift[0];
      orig_st[i][1]=Dot(orig_v[1],orig_c[i])+orig_p->tex.shift[1];
   }

// reset new_p's mapping
   new_p->tex.rotate=0;
   for (i=0;i<2;i++)
   {
      new_p->tex.scale[i]=1;
      new_p->tex.shift[i]=0;
   }

// prepare calculations for new_p
   TextureAxisFromPlane(new_p,new_v[0],new_v[1]);

   MakeVectors(new_v,&new_p->tex);
   memcpy(new_v1,new_v,sizeof(new_v1));

// save them for later

// and calculate for the first time
   for (i=0;i<3;i++)
   {
      new_c[i][0]=new_b->verts[new_p->verts[i]].x;
      new_c[i][1]=new_b->verts[new_p->verts[i]].y;
      new_c[i][2]=new_b->verts[new_p->verts[i]].z;

      new_st[i][0]=Dot(new_v[0],new_c[i])+new_p->tex.shift[0];
      new_st[i][1]=Dot(new_v[1],new_c[i])+new_p->tex.shift[1];
   }

// first, we will resolve the rotation value
   new_p->tex.rotate=angle( new_st[0], new_st[1])
                    -angle(orig_st[0],orig_st[1]);

   if (orientation(orig_st[0],orig_st[1],orig_st[2])
     ==orientation( new_st[0], new_st[1], new_st[2]))
   {
      new_p->tex.rotate=180-new_p->tex.rotate;
   }
/*
 it is crazy that I have to check for equal orientation
 but there are many possibilities...
 this place may be a source of some error though,
 but the idea is good
 So if wrong angles are calculated sometimes,
 check the "angle" procedure first
*/

/*
*** IMPORANT ***

If Quake doesn't handle real values for angle (e.g. whole degrees only
or 256 steps per full circle), this rounding has to be done *NOW*.
In such case, I suggest to put another parameter to this routine
which says if rounding has or has not to appear.
Then, while in program, no rounding is performed
and when saving, all mapped faces are recalculated again with rounding on.
This method minimizes texture un-alignment on manipulation
*/

/* and calculate for the second time - with known rotation */
   memcpy(new_v,new_v1,sizeof(new_v));
   MakeVectors(new_v,&new_p->tex);

   for (i=0;i<3;i++)
   {
      new_st[i][0]=Dot(new_v[0],new_c[i])+new_p->tex.shift[0];
      new_st[i][1]=Dot(new_v[1],new_c[i])+new_p->tex.shift[1];
   }

/* now we'll resolve scale values */
   for (j=0;j<2;j++)
   {
/* for each axis select a good pair of points first */
      val=0;
      best=0;
      for (i=0;i<3;i++)
      {
         p=fabs(
              ( new_st[pset[i][0]][j]- new_st[pset[i][1]][j])
             *(orig_st[pset[i][0]][j]-orig_st[pset[i][1]][j])
               );

         if (p>val)
         {
            val=p;
            best=i;
         }
      }
/* and then calculate the scale value for given axis */

      if (val)
      {
         new_p->tex.scale[j]=
             (new_st[pset[best][0]][j]- new_st[pset[best][1]][j])
           /(orig_st[pset[best][0]][j]-orig_st[pset[best][1]][j]);
      }
      else
      {
         new_p->tex.scale[j]=1;
      }
   }
/*
third calculation - with known rotation and scale
only one point is necessary
*/
   memcpy(new_v,new_v1,sizeof(new_v));
   MakeVectors(new_v,&new_p->tex);

   new_st[0][0]=Dot(new_v[0],new_c[0])+new_p->tex.shift[0];
   new_st[0][1]=Dot(new_v[1],new_c[0])+new_p->tex.shift[1];
/* find shift values */
   new_p->tex.shift[0]=orig_st[0][0]-new_st[0][0];
   new_p->tex.shift[1]=orig_st[0][1]-new_st[0][1];

/* that's all folks 8-) */

/*
after all, it is possible to calculate modulus of shift values
by texture size. Don't forget though that shift values are NOT integers.
*/
   {
      texture_t *t;

      t=ReadMIPTex(new_p->tex.name,0);
      if (t)
      {
         int is;

         is=(int)(new_p->tex.shift[0]/(float)t->rsx);
         new_p->tex.shift[0]-=is*t->rsx;

         is=(int)(new_p->tex.shift[1]/(float)t->rsy);
         new_p->tex.shift[1]-=is*t->rsy;
      }
   }
}

void TexLock(brush_t *n,brush_t *o)
{
   int i;

   if (!status.texlock)
      return;

   if (n->bt->type!=BR_NORMAL) return;

   for (i=0;(i<n->num_planes) && (i<o->num_planes);i++)
   {
      Solution(o,&o->plane[i],n,&n->plane[i]);
   }
}

