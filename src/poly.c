/*
poly.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "poly.h"

#include "3d_curve.h"
#include "brush.h"
#include "display.h"
#include "editface.h"
#include "error.h"
#include "file.h"
#include "geom.h"
#include "memory.h"
#include "quest.h"
#include "tex.h"
#include "video.h"


#define  NEXT_L(x,n)  (((x)==0)?((n)-1):((x)-1))
#define  NEXT_R(x,n)  (((x)==((n)-1))?0:((x)+1))


#define MAX_POLY_VERTS 32

typedef struct
{
   float fx,fy,fz;
   int x,y;
   float s,t;
   int oc;
} pvec_t;

typedef struct
{
   pvec_t pt[MAX_POLY_VERTS];
   int num_pts;
} poly_t;


static poly_t poly,poly2;


static viewport_t *cur_vp;
static int size_x,size_y;

static texture_t *tex;


/*
Draw single-colored polygon in poly, which is already clipped.
*/
static void P_DrawPolyTex(void)
{
   int    i;
   int    next;
   int    min_vert,
          min_val;
   int    max_vert,
          max_val;
   int    cur_lvert,
          cur_rvert;
   int    l_dx, l_dy,
          r_dx, r_dy;
   int    l_cur_x, r_cur_x;
   int    cur_y;
   float  z_val;

   int s,t;

   vec3_t l_edge_d, r_edge_d;
   vec3_t l_edge_p, r_edge_p;
   vec3_t scan_d,
          scan_p;

   register char  *vid_ptr;
   register float *zbuf_ptr;

   int    scanlinelen;


   /* Find min vertex */
   min_vert = max_vert = 0;
   min_val = max_val = poly.pt[0].y;

   s=(int)(poly.pt[0].s/tex->dsx)*tex->dsx;
   t=(int)(poly.pt[0].t/tex->dsy)*tex->dsy;

   for (i=0; i<poly.num_pts; i++)
   {
      if (poly.pt[i].y < min_val)
      {
         min_vert = i;
         min_val = poly.pt[i].y;
      }
      if (poly.pt[i].y > max_val)
      {
         max_vert = i;
         max_val = poly.pt[i].y;
      }

      poly.pt[i].s-=s;
      poly.pt[i].fx=poly.pt[i].s*poly.pt[i].fz;
      poly.pt[i].t-=t;
      poly.pt[i].fy=poly.pt[i].t*poly.pt[i].fz;
   }

   if (min_val == max_val)
      return;

   cur_lvert = cur_rvert = min_vert;
   cur_y = min_val;


   /* Set up left edge trace */
   next=NEXT_L(cur_lvert,poly.num_pts);
   while (poly.pt[cur_lvert].y == poly.pt[next].y)
   {
      cur_lvert = next;
      next = NEXT_L(cur_lvert, poly.num_pts);
   }

   l_cur_x = poly.pt[cur_lvert].x << 16;
   l_dx = poly.pt[next].x - poly.pt[cur_lvert].x;
   l_dy = poly.pt[next].y - poly.pt[cur_lvert].y;

   l_dx = (l_dx << 16) / l_dy;
   l_edge_p.x = poly.pt[cur_lvert].fx;
   l_edge_p.y = poly.pt[cur_lvert].fy;
   l_edge_p.z = poly.pt[cur_lvert].fz;
   l_edge_d.x = (poly.pt[next].fx - poly.pt[cur_lvert].fx) / l_dy;
   l_edge_d.y = (poly.pt[next].fy - poly.pt[cur_lvert].fy) / l_dy;
   l_edge_d.z = (poly.pt[next].fz - poly.pt[cur_lvert].fz) / l_dy;


   /* Set up right edge trace */
   next=NEXT_R(cur_rvert,poly.num_pts);
   while (poly.pt[cur_rvert].y == poly.pt[next].y)
   {
      cur_rvert = next;
      next = NEXT_R(cur_rvert, poly.num_pts);
   }
   r_cur_x = poly.pt[cur_rvert].x << 16;
   r_dx = poly.pt[next].x - poly.pt[cur_rvert].x;
   r_dy = poly.pt[next].y - poly.pt[cur_rvert].y;

   r_dx = (r_dx << 16) / r_dy;
   r_edge_p.x = poly.pt[cur_rvert].fx;
   r_edge_p.y = poly.pt[cur_rvert].fy;
   r_edge_p.z = poly.pt[cur_rvert].fz;
   r_edge_d.x = (poly.pt[next].fx - poly.pt[cur_rvert].fx) / r_dy;
   r_edge_d.y = (poly.pt[next].fy - poly.pt[cur_rvert].fy) / r_dy;
   r_edge_d.z = (poly.pt[next].fz - poly.pt[cur_rvert].fz) / r_dy;

   /* Do main loop */
   while (cur_y <= max_val)
   {
      /* Check if we need to move to the next left vertex */
      if (cur_y == poly.pt[NEXT_L(cur_lvert, poly.num_pts)].y)
      {
         next=NEXT_L(cur_lvert,poly.num_pts);
         while (cur_y == poly.pt[next].y)
         {
            cur_lvert = next;
            next = NEXT_L(cur_lvert, poly.num_pts);
         }
         l_cur_x = poly.pt[cur_lvert].x << 16;
         l_dx = poly.pt[next].x - poly.pt[cur_lvert].x;
         l_dy = poly.pt[next].y - poly.pt[cur_lvert].y;

         l_dx = (l_dx << 16) / l_dy;
         l_edge_p.x = poly.pt[cur_lvert].fx;
         l_edge_p.y = poly.pt[cur_lvert].fy;
         l_edge_p.z = poly.pt[cur_lvert].fz;
         l_edge_d.x = (poly.pt[next].fx - poly.pt[cur_lvert].fx) / l_dy;
         l_edge_d.y = (poly.pt[next].fy - poly.pt[cur_lvert].fy) / l_dy;
         l_edge_d.z = (poly.pt[next].fz - poly.pt[cur_lvert].fz) / l_dy;
      }

      /* Check if we need to move to the next right vertex */
      if (cur_y == poly.pt[NEXT_R(cur_rvert, poly.num_pts)].y)
      {
         next=NEXT_R(cur_rvert,poly.num_pts);
         while (cur_y == poly.pt[next].y)
         {
            cur_rvert = next;
            next = NEXT_R(cur_rvert, poly.num_pts);
         }
         r_cur_x = poly.pt[cur_rvert].x << 16;
         r_dx = poly.pt[next].x - poly.pt[cur_rvert].x;
         r_dy = poly.pt[next].y - poly.pt[cur_rvert].y;

         r_dx = (r_dx << 16) / r_dy;
         r_edge_p.x = poly.pt[cur_rvert].fx;
         r_edge_p.y = poly.pt[cur_rvert].fy;
         r_edge_p.z = poly.pt[cur_rvert].fz;
         r_edge_d.x = (poly.pt[next].fx - poly.pt[cur_rvert].fx) / r_dy;
         r_edge_d.y = (poly.pt[next].fy - poly.pt[cur_rvert].fy) / r_dy;
         r_edge_d.z = (poly.pt[next].fz - poly.pt[cur_rvert].fz) / r_dy;
      }

      if (l_cur_x < r_cur_x)
      {
         scanlinelen = (r_cur_x - l_cur_x) >> 16;

         if (scanlinelen == 0)
            goto skip_line;

         scan_p.x = l_edge_p.x;
         scan_p.y = l_edge_p.y;
         scan_p.z = l_edge_p.z;

         scan_d.x = (r_edge_p.x - l_edge_p.x) / scanlinelen;
         scan_d.y = (r_edge_p.y - l_edge_p.y) / scanlinelen;
         scan_d.z = (r_edge_p.z - l_edge_p.z) / scanlinelen;

         vid_ptr = &video.ScreenBuffer[cur_y * video.ScreenWidth + (l_cur_x >> 16)];
         zbuf_ptr = &cur_vp->zbuffer[(cur_vp->xmax - cur_vp->xmin) *
                                     (cur_y - cur_vp->ymin) + (l_cur_x >> 16) - cur_vp->xmin];

         for (i=(l_cur_x >> 16); i<(r_cur_x >> 16); i++)
         {
            if (fabs(scan_p.z)>0.001)
               z_val = 1.0/scan_p.z;
            else
               z_val = 1.0/0.001;

            if (z_val <= *zbuf_ptr)
            {
               s=scan_p.x*z_val;
               t=scan_p.y*z_val;

               while (s<0) s+=tex->dsx;
               while (s>=tex->dsx) s-=tex->dsx;
               while (t<0) t+=tex->dsy;
               while (t>=tex->dsy) t-=tex->dsy;

               *zbuf_ptr = z_val;
               *vid_ptr = tex->data[s+t*tex->dsx];
            }

            zbuf_ptr++;
            vid_ptr++;

            scan_p.x += scan_d.x;
            scan_p.y += scan_d.y;
            scan_p.z += scan_d.z;
         }
      }

skip_line:

      l_edge_p.x += l_edge_d.x;
      l_edge_p.y += l_edge_d.y;
      l_edge_p.z += l_edge_d.z;

      r_edge_p.x += r_edge_d.x;
      r_edge_p.y += r_edge_d.y;
      r_edge_p.z += r_edge_d.z;

      l_cur_x += l_dx;
      r_cur_x += r_dx;

      cur_y++;
   }
}


/*
Draw single-colored polygon in poly, which is already clipped.
*/
static void P_DrawPolyCol(int color)
{
   int    i;
   int    next;
   int    min_vert,
          min_val;
   int    max_vert,
          max_val;
   int    cur_lvert,
          cur_rvert;
   int    l_dx, l_dy,
          r_dx, r_dy;
   int    l_cur_x, r_cur_x;
   int    cur_y;
   float  z_val;

   vec3_t l_edge_d, r_edge_d;
   vec3_t l_edge_p, r_edge_p;
   vec3_t scan_d,
          scan_p;

   register char  *vid_ptr;
   register float *zbuf_ptr;

   int    scanlinelen;


   /* Find min vertex */
   min_vert = max_vert = 0;
   min_val = max_val = poly.pt[0].y;
   for (i=1; i<poly.num_pts; i++)
   {
      if (poly.pt[i].y < min_val)
      {
         min_vert = i;
         min_val = poly.pt[i].y;
      }
      if (poly.pt[i].y > max_val)
      {
         max_vert = i;
         max_val = poly.pt[i].y;
      }
   }

   if (min_val == max_val)
      return;

   cur_lvert = cur_rvert = min_vert;
   cur_y = min_val;


   /* Set up left edge trace */
   next=NEXT_L(cur_lvert,poly.num_pts);
   while (poly.pt[cur_lvert].y == poly.pt[next].y)
   {
      cur_lvert = next;
      next = NEXT_L(cur_lvert, poly.num_pts);
   }

   l_cur_x = poly.pt[cur_lvert].x << 16;
   l_dx = poly.pt[next].x - poly.pt[cur_lvert].x;
   l_dy = poly.pt[next].y - poly.pt[cur_lvert].y;

   l_dx = (l_dx << 16) / l_dy;
   l_edge_p.x = poly.pt[cur_lvert].fx;
   l_edge_p.y = poly.pt[cur_lvert].fy;
   l_edge_p.z = poly.pt[cur_lvert].fz;
   l_edge_d.x = (poly.pt[next].fx - poly.pt[cur_lvert].fx) / l_dy;
   l_edge_d.y = (poly.pt[next].fy - poly.pt[cur_lvert].fy) / l_dy;
   l_edge_d.z = (poly.pt[next].fz - poly.pt[cur_lvert].fz) / l_dy;


   /* Set up right edge trace */
   next=NEXT_R(cur_rvert,poly.num_pts);
   while (poly.pt[cur_rvert].y == poly.pt[next].y)
   {
      cur_rvert = next;
      next = NEXT_R(cur_rvert, poly.num_pts);
   }
   r_cur_x = poly.pt[cur_rvert].x << 16;
   r_dx = poly.pt[next].x - poly.pt[cur_rvert].x;
   r_dy = poly.pt[next].y - poly.pt[cur_rvert].y;

   r_dx = (r_dx << 16) / r_dy;
   r_edge_p.x = poly.pt[cur_rvert].fx;
   r_edge_p.y = poly.pt[cur_rvert].fy;
   r_edge_p.z = poly.pt[cur_rvert].fz;
   r_edge_d.x = (poly.pt[next].fx - poly.pt[cur_rvert].fx) / r_dy;
   r_edge_d.y = (poly.pt[next].fy - poly.pt[cur_rvert].fy) / r_dy;
   r_edge_d.z = (poly.pt[next].fz - poly.pt[cur_rvert].fz) / r_dy;

   /* Do main loop */
   while (cur_y <= max_val)
   {
      /* Check if we need to move to the next left vertex */
      if (cur_y == poly.pt[NEXT_L(cur_lvert, poly.num_pts)].y)
      {
         next=NEXT_L(cur_lvert,poly.num_pts);
         while (cur_y == poly.pt[next].y)
         {
            cur_lvert = next;
            next = NEXT_L(cur_lvert, poly.num_pts);
         }
         l_cur_x = poly.pt[cur_lvert].x << 16;
         l_dx = poly.pt[next].x - poly.pt[cur_lvert].x;
         l_dy = poly.pt[next].y - poly.pt[cur_lvert].y;

         l_dx = (l_dx << 16) / l_dy;
         l_edge_p.x = poly.pt[cur_lvert].fx;
         l_edge_p.y = poly.pt[cur_lvert].fy;
         l_edge_p.z = poly.pt[cur_lvert].fz;
         l_edge_d.x = (poly.pt[next].fx - poly.pt[cur_lvert].fx) / l_dy;
         l_edge_d.y = (poly.pt[next].fy - poly.pt[cur_lvert].fy) / l_dy;
         l_edge_d.z = (poly.pt[next].fz - poly.pt[cur_lvert].fz) / l_dy;
      }

      /* Check if we need to move to the next right vertex */
      if (cur_y == poly.pt[NEXT_R(cur_rvert, poly.num_pts)].y)
      {
         next=NEXT_R(cur_rvert,poly.num_pts);
         while (cur_y == poly.pt[next].y)
         {
            cur_rvert = next;
            next = NEXT_R(cur_rvert, poly.num_pts);
         }
         r_cur_x = poly.pt[cur_rvert].x << 16;
         r_dx = poly.pt[next].x - poly.pt[cur_rvert].x;
         r_dy = poly.pt[next].y - poly.pt[cur_rvert].y;

         r_dx = (r_dx << 16) / r_dy;
         r_edge_p.x = poly.pt[cur_rvert].fx;
         r_edge_p.y = poly.pt[cur_rvert].fy;
         r_edge_p.z = poly.pt[cur_rvert].fz;
         r_edge_d.x = (poly.pt[next].fx - poly.pt[cur_rvert].fx) / r_dy;
         r_edge_d.y = (poly.pt[next].fy - poly.pt[cur_rvert].fy) / r_dy;
         r_edge_d.z = (poly.pt[next].fz - poly.pt[cur_rvert].fz) / r_dy;
      }

      if (l_cur_x < r_cur_x)
      {
         scanlinelen = (r_cur_x - l_cur_x) >> 16;

         if (scanlinelen == 0)
            goto skip_line;

         scan_p.x = l_edge_p.x;
         scan_p.y = l_edge_p.y;
         scan_p.z = l_edge_p.z;

         scan_d.x = (r_edge_p.x - l_edge_p.x) / scanlinelen;
         scan_d.y = (r_edge_p.y - l_edge_p.y) / scanlinelen;
         scan_d.z = (r_edge_p.z - l_edge_p.z) / scanlinelen;

         vid_ptr = &video.ScreenBuffer[cur_y * video.ScreenWidth + (l_cur_x >> 16)];
         zbuf_ptr = &cur_vp->zbuffer[(cur_vp->xmax - cur_vp->xmin) *
                                     (cur_y - cur_vp->ymin) + (l_cur_x >> 16) - cur_vp->xmin];

         for (i=(l_cur_x >> 16); i<(r_cur_x >> 16); i++)
         {
            if (fabs(scan_p.z)>0.001)
               z_val = 1.0/scan_p.z;
            else
               z_val = 1.0/0.001;

            if (z_val <= *zbuf_ptr)
            {
               *zbuf_ptr = z_val;
               *vid_ptr = color;
            }

            zbuf_ptr++;
            vid_ptr++;

/*                              scan_p.x += scan_d.x;*/
/*                              scan_p.y += scan_d.y;*/
            scan_p.z += scan_d.z;
         }
      }

skip_line:

      l_edge_p.x += l_edge_d.x;
      l_edge_p.y += l_edge_d.y;
      l_edge_p.z += l_edge_d.z;

      r_edge_p.x += r_edge_d.x;
      r_edge_p.y += r_edge_d.y;
      r_edge_p.z += r_edge_d.z;

      l_cur_x += l_dx;
      r_cur_x += r_dx;

      cur_y++;
   }
}


#if 0
static void DPoly(poly_t *p)
{
   int i;
   pvec_t *v;

   printf("   p(%i): ",p->num_pts);
   for (v=p->pt,i=p->num_pts;i;i--,v++)
   {
      printf("(%0.2f %0.2f %0.2f %2i %3i %3i) ",
         v->fx,v->fy,v->fz,v->oc,
         (int)v->s,(int)v->t);
   }
   printf("\n");
}
#endif


/*
Clip polygon in poly, store result in poly.
*/
static void P_ClipPoly(void)
{
   int     i, j, k, cb;
   float   fx, fy;
   vec3_t  inter;
   float   scale;
   int     andc,orc;

   poly_t *s,*d;

   pvec_t *v,*w,*u;


   andc=0xffffffff;
   orc=0;
   for (i=poly.num_pts,v=poly.pt;i;i--,v++)
   {
      if (cur_vp->fullbright)
         v->oc&=~(1<<4);

      andc&=v->oc;
      orc|=v->oc;
   }

   if (!orc) /* completely visible, no need to clip */
   {
      goto final;
   }
   if (andc) /* completely off screen, bail out */
   {
      poly.num_pts=0;
      return;
   }

   s=&poly;
   d=&poly2;

   /* Clip against each of 6 planes */
   for (i=0; i<6; i++)
   {
      cb=1<<i;

      if (!(orc&cb))
         continue;


/*      printf(" before case %i\n", i);
      DPoly(s);*/


      /* Check points */
      d->num_pts=0;
      for (j=0,v=s->pt; j<s->num_pts; j++,v++)
      {
         if (!(v->oc&cb))
         {
            d->pt[d->num_pts]=*v;
            d->num_pts++;
         }
         /* And check if edge will cross plane */
         k=NEXT_R(j,s->num_pts);
         w=&s->pt[k];
         if (cb&(v->oc^w->oc))
         {
            Clip3D(v->fx,v->fy,v->fz,
                   w->fx,w->fy,w->fz,
                   &inter.x, &inter.y, &inter.z, &scale, (1 << i));

            u=&d->pt[d->num_pts];
            u->fx=inter.x;
            u->fy=inter.y;
            u->fz=inter.z;

            u->s=(w->s-v->s)*scale+v->s;
            u->t=(w->t-v->t)*scale+v->t;

            OutCode3D(inter.x,inter.y,inter.z,u->oc);
            d->num_pts++;
         }
      }

      if (s==&poly)
      {
         s=&poly2;
         d=&poly;
      }
      else
      {
         s=&poly;
         d=&poly2;
      }
   }

   if (s==&poly2)
   {
      poly.num_pts=poly2.num_pts;
      memcpy(poly.pt,poly2.pt,sizeof(pvec_t)*poly.num_pts);
   }

final:
/*   printf(" final:\n");
   DPoly(&poly);*/

   for (i=0; i<poly.num_pts; i++)
   {
      if (fabs(poly.pt[i].fz)<0.0001) poly.pt[i].fz=0.0001;

      fx =  poly.pt[i].fx / poly.pt[i].fz;
      fy = -poly.pt[i].fy / poly.pt[i].fz;

      poly.pt[i].x = ((cur_vp->xmax - cur_vp->xmin - 1) >> 1) * (fx + 1) + cur_vp->xmin + 1;
      poly.pt[i].y = ((cur_vp->ymax - cur_vp->ymin - 1) >> 1) * (fy + 1) + cur_vp->ymin + 1;

      poly.pt[i].fz = 1 / poly.pt[i].fz;
   }
}


// TODO: All copies of this should be moved to some common place.
static void TextureAxisFromPlane(plane_t *pln,float vecs[2][3])
{
static float baseaxis[18][3] =
{
{ 0, 0, 1},{ 1, 0, 0},{ 0,-1, 0}, // floor
{ 0, 0,-1},{ 1, 0, 0},{ 0,-1, 0}, // ceiling
{ 1, 0, 0},{ 0, 1, 0},{ 0, 0,-1}, // west wall
{-1, 0, 0},{ 0, 1, 0},{ 0, 0,-1}, // east wall
{ 0, 1, 0},{ 1, 0, 0},{ 0, 0,-1}, // south wall
{ 0,-1, 0},{ 1, 0, 0},{ 0, 0,-1}  // north wall
};

   int   bestaxis;
   float dot,best;
   int   i;
   
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

   vecs[0][0]=baseaxis[bestaxis*3+1][0];
   vecs[0][1]=baseaxis[bestaxis*3+1][1];
   vecs[0][2]=baseaxis[bestaxis*3+1][2];
   vecs[1][0]=baseaxis[bestaxis*3+2][0];
   vecs[1][1]=baseaxis[bestaxis*3+2][1];
   vecs[1][2]=baseaxis[bestaxis*3+2][2];
}

static void P_DrawBrush(brush_t *b)
{
   vec3_t   *bv,*btv;
   vec3_t    los;

   int i,j;
   plane_t *p;
   pvec_t *v;
   int *pv;

   float vecs[2][3];
   float f1,f2;

   for (j=0; j<b->num_planes;j++)
   {
      p = &(b->plane[j]);
      if (p->num_verts>MAX_POLY_VERTS)
         continue;

      bv = b->verts;
      pv = p->verts;
      /* Calc line-of-sight vector to a point on the plane */
      los.x = bv[pv[0]].x - cur_vp->camera_pos.x;
      los.y = bv[pv[0]].y - cur_vp->camera_pos.y;
      los.z = bv[pv[0]].z - cur_vp->camera_pos.z;
   
      /* Backface cull */
      if (DotProd(los, p->normal)>-0.01)
         continue;

      /* fill in poly */
      poly.num_pts=p->num_verts;
      btv=b->tverts;


      tex=ReadMIPTex(p->tex.name,0);

      TextureAxisFromPlane(p,vecs);
      MakeVectors(vecs,&p->tex);
      if (tex)
      {
         f1=tex->dsx/(float)tex->rsx;
         f2=tex->dsy/(float)tex->rsy;
      }
      else
         f1=f2=1;

      for (i=0,v=poly.pt;i<p->num_verts;i++,v++)
      {
         v->fx=btv[pv[i]].x;
         v->fy=btv[pv[i]].y;
         v->fz=btv[pv[i]].z;

         v->s=bv[pv[i]].x*vecs[0][0]+
              bv[pv[i]].y*vecs[0][1]+
              bv[pv[i]].z*vecs[0][2]+
              p->tex.shift[0];
         v->s*=f1;

         v->t=bv[pv[i]].x*vecs[1][0]+
              bv[pv[i]].y*vecs[1][1]+
              bv[pv[i]].z*vecs[1][2]+
              p->tex.shift[1];
         v->t*=f2;

         OutCode3D(v->fx,v->fy,v->fz,v->oc);
      }

      P_ClipPoly();
      if (poly.num_pts != 0)
      {
         if (tex)
            P_DrawPolyTex();
         else
            P_DrawPolyCol((j % 11) * 16 + 12);
      }
   }
}

static void P_DrawCurve(brush_t *b)
{
   int i,j,k,l;
   int oc,andc;
   plane_t *pl;
   pvec_t *v;

/*
#define NUM_TRIS 8
#define NUM_SPLITS 3
#define NUM_VERTS 9
static int pts[NUM_TRIS][3]=
{
{0,3,1},{1,3,4},{1,4,2},{2,4,5},
{3,6,4},{4,6,7},{4,7,5},{5,7,8}
};
*/


#define NUM_TRIS 32
#define NUM_SPLITS 5
#define NUM_VERTS 25
static int pts[NUM_TRIS][3]=
{
{ 0, 5, 1},{ 1, 5, 6}, { 5,10, 6},{ 6,10,11}, {10,15,11},{11,15,16}, {15,20,16},{16,20,21},
{ 1, 6, 2},{ 2, 6, 7}, { 6,11, 7},{ 7,11,12}, {11,16,12},{12,16,17}, {16,21,17},{17,21,22},
{ 2, 7, 3},{ 3, 7, 8}, { 7,12, 8},{ 8,12,13}, {12,17,13},{13,17,18}, {17,22,18},{18,22,23},
{ 3, 8, 4},{ 4, 8, 9}, { 8,13, 9},{ 9,13,14}, {13,18,14},{14,18,19}, {18,23,19},{19,23,24},
};


   vec3_t p[NUM_VERTS],t[NUM_VERTS];
   int o[NUM_VERTS];


   andc=0xffffffff;
   for (i=0;i<b->num_verts;i++)
   {
      OutCode3D(b->tverts[i].x,b->tverts[i].y,b->tverts[i].z,oc);
      andc&=oc;
   }
   if (andc) return;

   tex=ReadMIPTex(b->tex.name,0);
   if (!tex) return;

   for (i=0,pl=b->plane;i<b->num_planes;i++,pl++)
   {
      CurveGetPoints(b,b->tverts,pl,p,t,NUM_SPLITS);

      for (j=0;j<NUM_VERTS;j++)
      {
         t[j].x*=tex->dsx;
         t[j].y*=tex->dsy;
         OutCode3D(p[j].x,p[j].y,p[j].z,o[j]);
      }

      for (j=0;j<NUM_TRIS;j++)
      {
         poly.num_pts=3;
         v=poly.pt;

         for (k=0;k<3;k++,v++)
         {
            l=pts[j][k];
            v->fx=p[l].x;
            v->fy=p[l].y;
            v->fz=p[l].z;
            v->s =t[l].x;
            v->t =t[l].y;
            v->oc=o[l];
         }

         P_ClipPoly();
         if (poly.num_pts!=0)
            P_DrawPolyTex();
      }
   }
}


void DrawPolyView(int cur_vport)
{
   int i;
   brush_t *b;

   cur_vp=&M.display.vport[cur_vport];

   /* Allocate and intialize zbuffer */
   size_x = cur_vp->xmax - cur_vp->xmin;
   size_y = cur_vp->ymax - cur_vp->ymin;
   if ((size_x == 0) || (size_y == 0))
      return;

   cur_vp->zbuffer = (float *)Q_malloc(sizeof(float) * size_x * size_y);
   if (cur_vp->zbuffer == NULL)
   {
      HandleError("DrawViewport", "Unable to allocate z-buffer");
      return;
   }
   for (i=0;i<(size_x*size_y);i++) cur_vp->zbuffer[i]=8192.0;

   for (b=M.BrushHead; b; b=b->Next)
   {
      if (!b->drawn)
         continue;

      if (b->bt->flags&BR_F_EDGES)
         P_DrawBrush(b);
      else
      if (b->bt->type==BR_Q3_CURVE)
         P_DrawCurve(b);
   }
   Q_free(cur_vp->zbuffer);
}

