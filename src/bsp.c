/*
bsp.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

/*
 BSP UNIT. A few #defined functions, as well as variables declared
outside of functions. The variables are declared outside to avoid using
up stack space when recursive calls are made, and the define macros
are there to speed things up.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "bsp.h"
#include "bsptree.h"

#include "3d_curve.h"
#include "brush.h"
#include "editface.h"
#include "error.h"
#include "game.h"
#include "geom.h"
#include "memory.h"
#include "message.h"
#include "quest.h"
#include "status.h"
#include "tex.h"
#include "times.h"


static int bsp_error;
static const char *bsp_errors[]=
{
"No error!",
"Out of memory!",
"Error clipping face!",
"Invalid face encountered!"
};
#define BERR_NONE  0
#define BERR_MEM   1
#define BERR_CLIP  2
#define BERR_FACE  3


static int BSPFaceSplit(bsp_face_t *F,bsp_plane_t *p,bsp_face_t **FrontFace,bsp_face_t **BackFace)
/* 
  Returns:  0 - Face was clipped successfully
           -1 - Face was completely behind the plane
           -2 - Face was completely in front of the plane
           -3 - Error clipping / culling face
           -4 - Face was completely ON the plane
*/
{
#define MAX_FACE_PTS 512

   vec3_t p1,p2,p3;
   vtex_t t1,t2;
   int i,j;
   int backtest=0;
   int fronttest=0;
   int ontest=0;
   int splitnum=0;
   int    sides[MAX_FACE_PTS];
   float  dists[MAX_FACE_PTS];
   vec3_t newfront[MAX_FACE_PTS];
   vtex_t tfront[MAX_FACE_PTS];
   vec3_t newback[MAX_FACE_PTS];
   vtex_t tback[MAX_FACE_PTS];
   float  dot;
   int    numnewfront;
   int    numnewback;


   if (F->numpts+2>MAX_FACE_PTS)
   {
      HandleError("BSPFaceSplit","Face with %i vertices",F->numpts);
      return -3;
   }

/*   printf("BSPFaceSplit() (%g %g %g) %g\n",
      p->normal.x,p->normal.y,p->normal.z,p->dist);*/

   /* determine sides and distances for each point*/
   for (i=0;i<F->numpts;i++)
   {
      MY_DotProd(F->pts[i], p->normal, dot);
      dot -= (p->dist);

/*
This is becoming a mess. With the increasingly advanced geometry people are
using, all those weird planes and faces cause accumulating round-off errors
that usually end up here. For now, lets be strict about setting sides[],
and allow greater dots to set ontest et al.
*/

      dists[i]=dot;
      if (dot>0.001)
         sides[i]=1;
      else
      if (dot<-0.001)
         sides[i]=-1;
      else
         sides[i]=0;

      if (dot > 0.5)
         fronttest=1;
      else
      if (dot < -0.5)
         backtest=1;
      else
         ontest=1;

//      printf("%i : %g  %i %g\n",i,dot,sides[i],dists[i]);
   }

//   printf("ontest=%i  backtest=%i  fronttest=%i\n",ontest,backtest,fronttest);

   if ((ontest==1)&&(backtest==0)&&(fronttest==0))
   {
      (*FrontFace) = NULL;
      (*BackFace)  = NULL;
      return -4;
   }

   if ( ((backtest==0)&&(fronttest==1)) || ((backtest==1)&&(fronttest==0)) )
   {
      /*if all in front*/
      if (fronttest)
      {
         (*FrontFace) = NULL;
         (*BackFace)  = NULL;
         return -2;
      }
      /*if all in back*/
      if (backtest)
      {
         (*FrontFace) = NULL;
         (*BackFace)  = NULL;
         return -1;
      }
   }
   
   numnewfront=0;
   numnewback=0;
   for (i=0;i<F->numpts;i++)
   {
      p1 = F->pts[i];
      t1 = F->t_pts[i];
      
      if (i==F->numpts-1)
         j=0;
      else
         j=i+1;
      
      if (sides[i] == 0)
      {
         newfront[numnewfront] = p1; tfront[numnewfront] = t1; numnewfront++;
         newback[numnewback]   = p1; tback [numnewback ] = t1; numnewback++;
         if ((splitnum==2)||(sides[j]==0))
         {
//            printf("1 : splitnum==2 || sides[j]==0\n");

            bsp_error=BERR_CLIP;
            return -3;
         }
         splitnum++;
         continue;
      }
   
      if (sides[i] == 1)
      {
         newfront[numnewfront] = p1; tfront[numnewfront] = t1; numnewfront++;
      }
      if (sides[i] == -1)
      {
         newback[numnewback]   = p1; tback [numnewback ] = t1; numnewback++;
      }
      
      if ((sides[j]==sides[i])||(sides[j]==0)) continue;

      /* generate a split point*/
      p2 = F->pts[j];
      t2 = F->t_pts[j];
      
      dot = dists[i] / (dists[i]-dists[j]);
      
      if (p->normal.x == 1)
         p3.x = p->dist;
      else
         if (p->normal.x == -1)
            p3.x = -p->dist;
         else
            p3.x = p1.x + dot*(p2.x-p1.x);
      
      if (p->normal.y == 1)
         p3.y = p->dist;
      else
         if (p->normal.y == -1)
            p3.y = -p->dist;
         else
            p3.y = p1.y + dot*(p2.y-p1.y);
      
      if (p->normal.z == 1)
         p3.z = p->dist;
      else
         if (p->normal.z == -1)
            p3.z = -p->dist;
         else
            p3.z = p1.z + dot*(p2.z-p1.z);

      t1.s=t1.s+dot*(t2.s-t1.s);
      t1.t=t1.t+dot*(t2.t-t1.t);
      
      newfront[numnewfront] = p3; tfront[numnewfront] = t1; numnewfront++;
      newback[numnewback]   = p3; tback [numnewback ] = t1; numnewback++;
      if ((splitnum==2)||(sides[j]==0))
      {
//         printf("2 : splitnum==2 || sides[j]==0\n");

         bsp_error=BERR_CLIP;
         return -3;
      }
      splitnum++;
   }
   
   if (splitnum!=2)
   {
//      printf("splitnum!=2\n");

      bsp_error=BERR_CLIP;
      return -3;
   }
   
   (*FrontFace) = (bsp_face_t *)Q_malloc(sizeof(bsp_face_t));
   if ((*FrontFace)==NULL)
   {
//      printf("malloc\n");
      Q_free(F->pts);
      Q_free(F);

      bsp_error=BERR_MEM;
      return -3;
   }
   (*FrontFace)->mf=F->mf;
   
   (*BackFace) = (bsp_face_t *)Q_malloc(sizeof(bsp_face_t));
   if ((*BackFace)==NULL)
   {
//      printf("malloc\n");
      Q_free(F->pts);
      Q_free(F);
      Q_free((*FrontFace));

      bsp_error=BERR_MEM;
      return -3;
   }
   (*BackFace)->mf=F->mf;
   
   (*FrontFace)->pts = (vec3_t *)Q_malloc(sizeof(vec3_t) * numnewfront);
   (*FrontFace)->t_pts = (vtex_t *)Q_malloc(sizeof(vtex_t) * numnewfront);
   if ((*FrontFace)->pts==NULL)
   {
//      printf("malloc\n");
      Q_free((*FrontFace)->t_pts);
      Q_free((*FrontFace)->pts);
      
      Q_free(F->pts);
      Q_free(F);
      Q_free((*BackFace));
      Q_free((*FrontFace));

      bsp_error=BERR_MEM;
      return -3;
   }
   (*FrontFace)->numpts = numnewfront;
   
   (*BackFace)->pts = (vec3_t *)Q_malloc(sizeof(vec3_t) * numnewback);
   (*BackFace)->t_pts = (vtex_t *)Q_malloc(sizeof(vtex_t) * numnewback);
   if ((*BackFace)->pts==NULL)
   {
//      printf("malloc\n");
      Q_free(F->pts);
      Q_free(F);
      Q_free((*FrontFace)->t_pts);
      Q_free((*FrontFace)->pts);
      Q_free((*BackFace)->t_pts);
      Q_free((*BackFace)->pts);
      Q_free((*BackFace));
      Q_free((*FrontFace));

      bsp_error=BERR_MEM;
      return -3;
   }
   (*BackFace)->numpts = numnewback;

   for (i=0;i<numnewfront;i++)
   {
      (*FrontFace)->pts[i] = newfront[i];
      (*FrontFace)->t_pts[i] = tfront[i];
   }
   for (i=0;i<numnewback;i++)
   {
      (*BackFace)->pts[i] = newback[i];
      (*BackFace)->t_pts[i] = tback[i];
   }
   
   Q_free(F->pts);
   Q_free(F);
   
   return 0;
}


/*
 Hash stuff to find unique vertices. All vertices should be within
valid Quake space (-4096 - +4096), although it will work anyway
(but slower, many vertices get same hash value).
*/

static int HashVec(vec3_t *v)
{
   int x,y;

   x=(v->x+4096)/(8192/HASH_SIZE);
   y=(v->y+4096)/(8192/HASH_SIZE);

   if (x<0) x=0;
   if (x>HASH_SIZE-1) x=HASH_SIZE-1;

   if (y<0) y=0;
   if (y>HASH_SIZE-1) y=HASH_SIZE-1;

   return x+y*HASH_SIZE;
}


#define IsSameVert(v1,v2,result)                    \
{                                                   \
   (result)=0;                                      \
   if ((fabs(((v1).x)-((v2).x))<0.1)&&              \
       (fabs(((v1).y)-((v2).y))<0.1)&&              \
       (fabs(((v1).z)-((v2).z))<0.1)) {(result)=1;} \
}


static int FindVec(vec3_t *v)
{
   int c,l,done;
   vec3_t *BSPvold;
   int *BSPnvold;
   int vh;

   c=l=M.BSPhash[vh=HashVec(v)];
   while (c!=-1)
   {
      IsSameVert(*v,M.BSPverts[c],done);
      if (done)
      {
         return c;
      }
      l=c;
      c=M.BSPnextvec[c];
   }

   BSPvold = M.BSPverts;
   M.BSPverts = (vec3_t *)Q_realloc(M.BSPverts,sizeof(vec3_t) * (M.uniqueverts+1));
   if (M.BSPverts==NULL)
   {
      M.BSPverts=BSPvold;

      bsp_error=BERR_MEM;
      return -1;
   }

   BSPnvold=M.BSPnextvec;
   M.BSPnextvec=Q_realloc(M.BSPnextvec,sizeof(int)*(M.uniqueverts+1));
   if (!M.BSPnextvec)
   {
      M.BSPnextvec=BSPnvold;

      bsp_error=BERR_MEM;
      return -1;
   }
   M.BSPnextvec[M.uniqueverts]=M.BSPhash[vh];
   M.BSPhash[vh]=M.uniqueverts;

   M.BSPverts[M.uniqueverts] = *v;
   M.uniqueverts++;

   return M.uniqueverts-1;
}
 
static int MakeUniqueFaceVerts(bsp_face_t *F)
{
   int i,endnumber;

   if (F->numpts<3)
   {
      Q_free(F->pts);
      Q_free(F);

      bsp_error=BERR_FACE;
      return FALSE;
   }
   M.totalfaces++;
   M.totalverts += F->numpts;
   endnumber = M.uniqueverts;

   F->i_pts = (int *)Q_malloc(sizeof(int) * (F->numpts));
   if (F->i_pts==NULL)
   {
      Q_free(F->t_pts);
      Q_free(F->pts);
      Q_free(F);

      bsp_error=BERR_MEM;
      return FALSE;
   }
   for (i=0; i<F->numpts; i++)
   {
      F->i_pts[i]=FindVec(&F->pts[i]);
      if (F->i_pts[i]==-1)
      {
         Q_free(F->t_pts);
         Q_free(F->i_pts);
         Q_free(F->pts);
         Q_free(F);

         bsp_error=BERR_MEM;
         return FALSE;
      }
   }
   Q_free(F->pts);
   F->pts=NULL;
/* keep texture coordinates for the rendering code */
   return TRUE;
}


// The plane of the current face:
static bsp_plane_t curplane;

static bsp_node_t *NewNode(void)
{
   bsp_node_t *newnode;

   newnode = (bsp_node_t *)Q_malloc(sizeof(bsp_node_t));
   if (newnode==NULL)
   {
      bsp_error=BERR_MEM;
      return NULL;
   }
   memset(newnode,0,sizeof(bsp_node_t));
   newnode->plane=curplane;

   newnode->faces=NULL;
   newnode->numfaces=0;

   return newnode;
}

static bsp_face_t **oldf;

static int AddToTree(bsp_face_t *F,bsp_node_t *Root,int onnode)
{
   bsp_face_t *Front;
   bsp_face_t *Back;
   int i;

   if (onnode)
      i=-4;
   else
      i=BSPFaceSplit(F,&Root->plane,&Front,&Back);
   switch (i)
   {
   case -4 :
      if (!MakeUniqueFaceVerts(F)) return FALSE;

      oldf = Root->faces;
      Root->faces =
         (bsp_face_t **)Q_realloc(
            Root->faces,
            sizeof(bsp_face_t *) * (Root->numfaces + 1));

      if (Root->faces==NULL)
      {
         Root->faces=oldf;
         if (F->pts!=NULL) Q_free(F->pts);
         Q_free(F);

         bsp_error=BERR_MEM;
         return FALSE;
      }

      Root->faces[Root->numfaces] = F;
      Root->numfaces++;
      if (F->mf->flags&TEX_DOUBLESIDED)
         F->samenormal=3;
      else
      {
         IsSameVert(curplane.normal,Root->plane.normal,F->samenormal);
         F->samenormal++;
      }
      return TRUE;
      break;

   case -1 :
      if (Root->BackNode!=NULL) return AddToTree(F,Root->BackNode,0);
      Root->BackNode = NewNode();
      if (Root->BackNode==NULL) return FALSE;

      return AddToTree(F,Root->BackNode,1);
      break;

   case -2 :
      if (Root->FrontNode!=NULL) return AddToTree(F,Root->FrontNode,0);
      Root->FrontNode = NewNode();
      if (Root->FrontNode==NULL) return FALSE;

      return AddToTree(F,Root->FrontNode,1);
      break;

   case  0 :
      if (!Root->BackNode)
      {
         Root->BackNode = NewNode();
         if (!Root->BackNode) return FALSE;
      }
      if (!AddToTree(Back,Root->BackNode,0)) return FALSE;

      if (!Root->FrontNode)
      {
         Root->FrontNode = NewNode();
         if (!Root->FrontNode) return FALSE;
      }
      if (!AddToTree(Front,Root->FrontNode,0)) return FALSE;

      return TRUE;
      break;

   case -3:
      return FALSE;
   }
   return FALSE;
}


#ifdef BSP_CURVES
static int AddCurveFace(
   vec3_t *p0,vtex_t *t0,
   vec3_t *p1,vtex_t *t1,
   vec3_t *p2,vtex_t *t2,
   bsp_map_face_t *mf)
{
   bsp_face_t *F;
   vec3_t v1,v2;

/*   printf("adding: (%g %g %g  %g %g) (%g %g %g  %g %g) (%g %g %g  %g %g)\n",
      p0->x,p0->y,p0->z,t0->s,t0->t,
      p1->x,p1->y,p1->z,t1->s,t1->t,
      p2->x,p2->y,p2->z,t2->s,t2->t);*/

   F = Q_malloc(sizeof(bsp_face_t));
   if (!F)
   {
      bsp_error=BERR_MEM;
      return 1;
   }
   F->mf=mf;
   F->numpts=3;
   F->pts=Q_malloc(sizeof(vec3_t)*3);
   F->t_pts=Q_malloc(sizeof(vtex_t)*3);
   if (!F->pts || ! F->t_pts)
   {
      Q_free(F->pts);
      Q_free(F->t_pts);
      Q_free(F);
      bsp_error=BERR_MEM;
      return 1;
   }

   F->pts[0]=*p0;
   F->pts[1]=*p1;
   F->pts[2]=*p2;
   F->t_pts[0]=*t0;
   F->t_pts[1]=*t1;
   F->t_pts[2]=*t2;

   v1.x=p0->x-p1->x;
   v1.y=p0->y-p1->y;
   v1.z=p0->z-p1->z;
   
   v2.x=p0->x-p2->x;
   v2.y=p0->y-p2->y;
   v2.z=p0->z-p2->z;

   CrossProd(v1,v2,&curplane.normal);
   Normalize(&curplane.normal);
   curplane.dist=DotProd(curplane.normal,*p0);

   if (!M.BSPHead)
   {
      M.BSPHead = NewNode();
      if (!M.BSPHead)
         return 1;
   }

   if (!AddToTree(F,M.BSPHead,0))
      return 1;
   return 0;
}


static int BSP_AddCurve(brush_t *b)
{
   int i;
   int flags;
   vec3_t pts[9];
   vtex_t t0,t1,t2;
   plane_t *p;

   bsp_map_face_t *mf;
   bsp_map_face_t **t;

   float tx,ty;
   texture_t *tex;
   

   if (b->Group->flags & 2)
      return 0;
      
   if (Game.tex.gettexbspflags)
      flags=Game.tex.gettexbspflags(&b->tex);
   else
      flags=0;

   if (flags&TEX_NODRAW)
      return 0;

   tex=ReadMIPTex(b->tex.name,0);
   if (tex)
   {
      tx=tex->rsx;
      ty=tex->rsy;
   }
   else
      tx=ty=128;

      
   t=Q_realloc(M.BSPmf,sizeof(bsp_map_face_t *)*(M.num_mf+1));
   if (!t)
   {
      bsp_error=BERR_MEM;
      return 1;
   }

   M.BSPmf=t;

   mf=Q_malloc(sizeof(bsp_map_face_t));
   if (!mf)
   {
      bsp_error=BERR_MEM;
      return 1;
   }

   M.BSPmf[M.num_mf++]=mf;

   strcpy(mf->texname,b->tex.name);
   mf->flags=flags;

//   printf("BSP_AddCurve: '%s'\n",b->tex.name);
   
   for (i=0;i<b->num_planes;i++)
   {
      p=&b->plane[i];

      CurveGetPoints(b,b->verts,p,pts,NULL,3);

#define ADD(e,f,g) \
   t0.s=b->x.q3c.s[p->verts[e]]*tx; \
   t1.s=b->x.q3c.s[p->verts[f]]*tx; \
   t2.s=b->x.q3c.s[p->verts[g]]*tx; \
    \
   t0.t=b->x.q3c.t[p->verts[e]]*ty; \
   t1.t=b->x.q3c.t[p->verts[f]]*ty; \
   t2.t=b->x.q3c.t[p->verts[g]]*ty; \
    \
   if (AddCurveFace(&pts[e],&t0,&pts[f],&t1,&pts[g],&t2,mf)) \
         return 1;

      ADD(0,2,6)
      ADD(6,2,8)

/*      ADD(0,1,4)
      ADD(0,4,3)

      ADD(1,2,4)
      ADD(2,5,4)

      ADD(3,4,6)
      ADD(4,7,6)

      ADD(4,5,8)
      ADD(4,8,7)*/
   }
   return 0;
}
#endif


// TODO: All copies of this should be moved to some common place.
static void TextureAxisFromPlane(bsp_plane_t *pln,float vecs[2][3])
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


static bsp_map_face_t *NewMF(brush_t *b,plane_t *p)
{
   bsp_map_face_t *temp;
   bsp_map_face_t **t;

   t=Q_realloc(M.BSPmf,sizeof(bsp_map_face_t *)*(M.num_mf+1));
   if (!t)
   {
      bsp_error=BERR_MEM;
      return NULL;
   }

   M.BSPmf=t;

   temp=Q_malloc(sizeof(bsp_map_face_t));
   if (!temp)
   {
      bsp_error=BERR_MEM;
      return NULL;
   }

   M.BSPmf[M.num_mf++]=temp;

   if (b->bt->type==BR_SUBTRACT)
   {
      curplane.normal.x=-p->normal.x;
      curplane.normal.y=-p->normal.y;
      curplane.normal.z=-p->normal.z;
      curplane.dist    =-p->dist;
   }
   else
   {
      curplane.normal=p->normal;
      curplane.dist  =p->dist;
   }

   strcpy(temp->texname,p->tex.name);
//   temp->b=b;
   temp->flags=0;

   return temp;
}


static bsp_face_t *MakeBSPFace(brush_t *b,vec3_t *verts, plane_t *p)
{
   bsp_face_t *F;
   int i;
   
   F = (bsp_face_t *)Q_malloc(sizeof(bsp_face_t));
   if (!F)
   {
      bsp_error=BERR_MEM;
      return NULL;
   }
   memset(F,0,sizeof(bsp_face_t));

   F->numpts = p->num_verts;

   F->pts = (vec3_t *)Q_malloc(sizeof(vec3_t) * (F->numpts));
   F->t_pts = (vtex_t *)Q_malloc(sizeof(vtex_t)*F->numpts);
   if (!F->pts || !F->t_pts)
   {
      Q_free(F->pts);
      Q_free(F->t_pts);
      Q_free(F);

      bsp_error=BERR_MEM;
      return NULL;
   }

   if (b->bt->type==BR_SUBTRACT)
   {
      for (i=0;i<F->numpts;i++)
         F->pts[i] = verts[p->verts[F->numpts-1-i]];
   }
   else
   {
      for (i=0;i<F->numpts;i++)
         F->pts[i] = verts[p->verts[i]];
   }

   F->mf=NewMF(b,p);
   if (!F->mf)
   {
      Q_free(F->pts);
      Q_free(F);
      return NULL;
   }

   {
      float vecs[2][3];
      int i;
      vec3_t *v;

      TextureAxisFromPlane(&curplane,vecs);
      MakeVectors(vecs,&p->tex);

      for (i=0,v=F->pts;i<F->numpts;i++,v++)
      {
         F->t_pts[i].s=
            v->x*vecs[0][0]+
            v->y*vecs[0][1]+
            v->z*vecs[0][2]+
            p->tex.shift[0];
   
         F->t_pts[i].t=
            v->x*vecs[1][0]+
            v->y*vecs[1][1]+
            v->z*vecs[1][2]+
            p->tex.shift[1];
      }
   }
   
   return F;
}


int BSPAddBrush(brush_t *b)
{
   int i;

   bsp_face_t *F;
   plane_t *p;

   int flags;


   if (b->Group->flags & 2)
      return 0;

   if (!(b->bt->flags&BR_F_EDGES))
      return 0;

   M.startnumber = M.uniqueverts;

   for (i=0;i<b->num_planes;i++)
   {
      p=&b->plane[i];

      if ((p->normal.x*p->normal.x+
           p->normal.y*p->normal.y+
           p->normal.z*p->normal.z)<0.9)
      {
         continue;
      }

      if (Game.tex.gettexbspflags)
         flags=Game.tex.gettexbspflags(&p->tex);
      else
         flags=0;

      if (flags&TEX_NODRAW)
         continue;

      F = MakeBSPFace(b,b->verts,p);
      if (F==NULL)
         return 1;

      F->mf->flags=flags;

      if (!M.BSPHead)
      {
         M.BSPHead = NewNode();
         if (!M.BSPHead)
            return 1;
      }

      if (!AddToTree(F,M.BSPHead,0))
         return 1;
   }

   return 0;
}


/* Main Tree Functions */

static int BuildBSPTree(void)
{
   float start_time,end_time;

   brush_t *b;


   start_time = GetTime();
   M.totalverts = 0;
   M.uniqueverts = 0;
   M.totalfaces=0;
   M.num_mf=0;

   M.BSPhash=Q_malloc(sizeof(int)*HASH_SIZE*HASH_SIZE);
   if (!M.BSPhash)
   {
      HandleError("BSP Build","Out of memory!");
      return 0;
   }
   memset(M.BSPhash,-1,sizeof(int)*HASH_SIZE*HASH_SIZE);

   bsp_error=BERR_NONE;
   for (b=M.BrushHead; b; b=b->Next)
   {
      if (b->bt->type==BR_Q3_CURVE)
         continue;

      if (BSPAddBrush(b))
      {
         HandleError("BuildBSP","%s",bsp_errors[bsp_error]);
         if (bsp_error==BERR_MEM)
            break; /* no point in trying to recover, we'd probably run out
                      of memory right away again */
         bsp_error=BERR_NONE;
      }
   }

#ifdef BSP_CURVES
/* TODO: enable this once we know how the texture mapping works for curves */
   for (b=M.BrushHead; !bsp_error && b; b=b->Next)
   {
      if (b->bt->type!=BR_Q3_CURVE)
         continue;

      if (BSP_AddCurve(b))
      {
         HandleError("BuildBSP","%s",bsp_errors[bsp_error]);
         if (bsp_error==BERR_MEM)
            break; /* no point in trying to recover, we'd probably run out
                      of memory right away again */
         bsp_error=BERR_NONE;
      }
   }
#endif

   end_time = GetTime();

   if (M.uniqueverts==0)
   {
      HandleError("BSP Build","Nothing to build!");
      return FALSE;
   }

   NewMessage("%i total verts, %i unique verts",M.totalverts,M.uniqueverts);
   NewMessage("Total faces: %d",M.totalfaces);
   NewMessage("BSP built in %g seconds",end_time-start_time);

   /* allocate transformation vertex list */
   M.TBSPverts = (vec3_vt *)Q_malloc(sizeof(vec3_vt) * M.uniqueverts);
   if (M.TBSPverts==NULL)
   {
      DeleteBSPTree();
      HandleError("BSP Build","Unable to allocate transformation vertex list!");
      return FALSE;
   }
   return TRUE;
}


static void DeleteBSPNode(bsp_node_t *Root)
{
   int i;

   if (Root->FrontNode!=NULL) DeleteBSPNode(Root->FrontNode);
   if (Root->BackNode!=NULL)  DeleteBSPNode(Root->BackNode);
   if (Root->faces!=NULL)
   {
      for (i=0;i<Root->numfaces;i++)
      {
         if (Root->faces[i]!=NULL)
         {
            if (Root->faces[i]->pts!=NULL) Q_free(Root->faces[i]->pts);
            if (Root->faces[i]->i_pts!=NULL) Q_free(Root->faces[i]->i_pts);
            if (Root->faces[i]->t_pts!=NULL) Q_free(Root->faces[i]->t_pts);
            Q_free(Root->faces[i]);
         }
      }
      Q_free(Root->faces);
   }
   Q_free(Root);
}

void DeleteBSPTree(void)
{
   int i;

   if (M.BSPHead   ) DeleteBSPNode(M.BSPHead);
   if (M.BSPverts  ) Q_free(M.BSPverts);
   if (M.TBSPverts ) Q_free(M.TBSPverts);
   if (M.BSPhash   ) Q_free(M.BSPhash);
   if (M.BSPnextvec) Q_free(M.BSPnextvec);
   if (M.BSPmf     )
   {
      for (i=0;i<M.num_mf;i++)
         Q_free(M.BSPmf[i]);
      Q_free(M.BSPmf);
   }
   M.BSPHead=NULL;
   M.BSPverts=NULL;
   M.TBSPverts=NULL;
   M.BSPhash=NULL;
   M.BSPnextvec=NULL;
   M.BSPmf=NULL;
}


void RebuildBSP(void)
{
   DeleteBSPTree();
   BuildBSPTree();
}

