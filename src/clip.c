/*
clip.c file of the Quest Source Code

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

#include "clip.h"

#include "3d.h"
#include "brush.h"
#include "check.h"
#include "display.h"
#include "edbrush.h"
#include "edvert.h"
#include "error.h"
#include "geom.h"
#include "keyboard.h"
#include "keyhit.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "quest.h"
#include "undo.h"


static void AddPoint(face_my_t *F, vec3_t pt)
/*
   Adds a point to the face
*/
{
	F->pts[F->numpts] = pt;
	F->numpts++;
}

static int RemoveFace(brush_my_t *B, int pos)
/* 
   Removes the face from the brush
*/
{
	int i;

	if (B->numfaces==0)
	   return FALSE;

	for (i=pos;i<B->numfaces-1;i++)
	{
		B->faces[i] = B->faces[i+1];
	}
	B->numfaces--;
	return TRUE;
}


int DivideFaceByPlane(face_my_t F,plane_my_t P,face_my_t *Front,face_my_t *Back, int KeepBack)
/* 
  
  Returns:
           0 - Face was clipped successfully
	       -1 - Face was completely behind the plane
	       -2 - Face was completely in front of the plane
	       -3 - Error clipping / culling face 
	       -4 - Face was completely ON the plane
*/
{        
	face_my_t FrontFace;
	face_my_t BackFace;
	vec3_t p1,p2,p3;
	int i,j,/*k,*/tfpts,tbpts;
	double dot;
	int backtest=0;
	int fronttest=0;
	int ontest=0;
	int splitnum=0;
	int sides[MAX_FACE_POINTS];
	double dists[MAX_FACE_POINTS];
	
	if (F.numpts>MAX_FACE_POINTS) return -3;
	tfpts = 0;
	tbpts = 0;

	/* determine sides and distances for each point*/
	for (i=0;i<F.numpts;i++)
	{
		dot = DotProd(F.pts[i], P.normal);
		dot -= P.dist;
		dists[i] = dot;
		if (dot > 0.01)
      {
         sides[i] = 1;
         fronttest=1;
      }
		else
		if (dot < -0.01)
		{
		   sides[i] = -1;
		   backtest=1;
      }
		else
		{
		   dists[i]=0;
		   sides[i] = 0;
		   ontest=1;
      }
	}
	
	if ((ontest==1)&&(backtest==0)&&(fronttest==0))
	{
		return -4;
	}

	if (((backtest==0)&&(fronttest==1)) || ((backtest==1)&&(fronttest==0)))
	{
		/*if all in front*/
		if (fronttest)
		{
			*Front = F;
			return -2;
		}
		/*if all in back*/
		if (backtest)
		{
			if (KeepBack)
			{
				*Back = F;
			}
			return -1;
		}
	}
	
	FrontFace.numpts = 0;
	
	if (KeepBack)
	{
		BackFace.numpts = 0;
	}

	for (i=0;i<F.numpts;i++)
	{
		p1 = F.pts[i];
		
		if (i==F.numpts-1) j=0;
			else j=i+1;
		
		if (sides[i] == 0)
		{
			tfpts++; if (tfpts>MAX_FACE_POINTS) return -3;
			AddPoint(&FrontFace,p1);
			if (KeepBack)
			{
				tbpts++;
				if (tbpts>MAX_FACE_POINTS) return -3;
				AddPoint(&BackFace,p1);
			}
			if ((splitnum==2)||(sides[j]==0))
			{
				return -3;
			}
			splitnum++;
			continue;
		}
	
		if (sides[i] == 1)
		{
			tfpts++;
			if (tfpts>MAX_FACE_POINTS) return -3;
			AddPoint(&FrontFace,p1);
		}
		if (sides[i] == -1)
		{
			if (KeepBack)
			{
				tbpts++;
				if (tbpts>MAX_FACE_POINTS) return -3;
				AddPoint(&BackFace,p1);
			}
		}
		
		if ((sides[j]==sides[i])||(sides[j]==0)) continue;

		/* generate a split point*/
		p2 = F.pts[j];
		
		dot = dists[i] / (dists[i]-dists[j]);
		
		if (P.normal.x == 1)
		   p3.x = P.dist;
		else
		if (P.normal.x == -1)
		   p3.x = -P.dist;
		else
		   p3.x = p1.x + dot*(p2.x-p1.x);
		
		if (P.normal.y == 1)
		   p3.y = P.dist;
		else
		if (P.normal.y == -1)
         p3.y = -P.dist;
		else
		   p3.y = p1.y + dot*(p2.y-p1.y);
		
		if (P.normal.z == 1)
		   p3.z = P.dist;
		else
		if (P.normal.z == -1)
		   p3.z = -P.dist;
		else
		   p3.z = p1.z + dot*(p2.z-p1.z);
		
		tfpts++;
		if (tfpts>MAX_FACE_POINTS) return -3;
		AddPoint(&FrontFace,p3);
		if (KeepBack)
		{
			tbpts++;
			if (tbpts>MAX_FACE_POINTS) return -3;
			AddPoint(&BackFace,p3);
		}
		if ((splitnum==2)||(sides[j]==0))
		{
			return -3;
		}
		splitnum++;
	}
		
	if (splitnum!=2)
	{
		return -3;
	}
	Front->numpts = FrontFace.numpts;
	for (i=0;i<Front->numpts;i++) Front->pts[i] = FrontFace.pts[i];
	Front->normal = F.normal;
	Front->dist = F.dist;

   Front->tex = F.tex;

	if (KeepBack)
	{
		Back->numpts = BackFace.numpts;
		for (i=0;i<Back->numpts;i++) Back->pts[i] = BackFace.pts[i];
		Back->normal = F.normal;
		Back->dist = F.dist;

      Back->tex = F.tex;
	}

	return 0;
}


typedef float qbsp_vec3_t[3];

typedef struct
{
	qbsp_vec3_t  normal;
	float   dist;
	int     type;
} qbsp_plane_t;

static void VectorMA (qbsp_vec3_t va, float scale, qbsp_vec3_t vb, qbsp_vec3_t vc)
{
	vc[0] = va[0] + scale*vb[0];
	vc[1] = va[1] + scale*vb[1];
	vc[2] = va[2] + scale*vb[2];
}

static void CrossProduct (qbsp_vec3_t v1, qbsp_vec3_t v2, qbsp_vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

static float DotProduct (qbsp_vec3_t v1, qbsp_vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

static void VectorSubtract (qbsp_vec3_t va, qbsp_vec3_t vb, qbsp_vec3_t out)
{
	out[0] = va[0]-vb[0];
	out[1] = va[1]-vb[1];
	out[2] = va[2]-vb[2];
}

static void VectorAdd (qbsp_vec3_t va, qbsp_vec3_t vb, qbsp_vec3_t out)
{
	out[0] = va[0]+vb[0];
	out[1] = va[1]+vb[1];
	out[2] = va[2]+vb[2];
}


static void VectorNormalize (qbsp_vec3_t v)
{
	int             i;
	double  length;
	
	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrt (length);
	if (length == 0) return;
		
	for (i=0 ; i< 3 ; i++) v[i] /= length; 
}


static void VectorScale (qbsp_vec3_t v, float scale, qbsp_vec3_t out)
{
	out[0] = v[0] * scale;
	out[1] = v[1] * scale;
	out[2] = v[2] * scale;
}

void MakeBoxOnPlane (face_my_t *P)
{
	int             i, x;
	float           max, v;
	qbsp_vec3_t         org, vright, vup,temppt;
	qbsp_plane_t         p;

	p.normal[0] = P->normal.x;
	p.normal[1] = P->normal.y;
	p.normal[2] = P->normal.z;
	p.dist = P->dist;
	
	/* find the major axis*/
	max = -99999;
	x = -1;
	for (i=0 ; i<3; i++)
	{
		v = fabs(p.normal[i]);
		if (v > max)
		{
			x = i;
			max = v;
		}
	}
	if (x==-1)
	{
		Abort("MakeBoxOnPlane","Could not find major axis");
	}
		
	vup[0]=0; vup[1]=0; vup[2]=0;
	switch (x)
	{
		case 0:
		case 1:
			vup[2] = 1;
			break;          
		case 2:
			vup[0] = 1;
			break;          
	}

	v = DotProduct (vup, p.normal);
	VectorMA (vup, -v, p.normal, vup);
	VectorNormalize (vup);
		
	VectorScale (p.normal, p.dist, org);
	
	CrossProduct (vup, p.normal, vright);
	
	VectorScale (vup, 8192, vup);
	VectorScale (vright, 8192, vright);

	/* project a really big axis aligned box onto the plane*/
	P->numpts = 4;
	
	VectorSubtract (org, vright, temppt); 
	VectorAdd (temppt, vup, temppt);
	P->pts[0].x = temppt[0];
	P->pts[0].y = temppt[1];
	P->pts[0].z = temppt[2];
	
	VectorAdd (org, vright, temppt);
	VectorAdd (temppt, vup, temppt);
	P->pts[1].x = temppt[0];
	P->pts[1].y = temppt[1];
	P->pts[1].z = temppt[2];

	
	VectorAdd (org, vright, temppt);
	VectorSubtract (temppt, vup, temppt);
	P->pts[2].x = temppt[0];
	P->pts[2].y = temppt[1];
	P->pts[2].z = temppt[2];

	
	VectorSubtract (org, vright, temppt);
	VectorSubtract (temppt, vup, temppt);
	P->pts[3].x = temppt[0];
	P->pts[3].y = temppt[1];
	P->pts[3].z = temppt[2];

}


int CreateBrushFaces(brush_my_t *B)
{                               
	face_my_t Face1,Face2;
	face_my_t NewFace;
   face_my_t *f;
	plane_my_t P;
	int i,j,k,l;
	int flag;
	
	for (i=0; i<B->numfaces;i++) B->faces[i].misc=0;

	for (i=0 ; i<B->numfaces ; i++)
	{
		NewFace = B->faces[i];
		MakeBoxOnPlane(&NewFace);        

		flag=0;
		for (j=0;j<B->numfaces;j++)
		{
			if (j==i) continue;

// Why is this line here? Removing it seems to solve some problems.
/* Better not, removing it adds other problems. Instead, more changes. */
       
			if (B->faces[j].misc==2) continue;
			
			P = B->faces[j];
			P.normal.x = -P.normal.x;
			P.normal.y = -P.normal.y;
			P.normal.z = -P.normal.z;
			P.dist = -P.dist;
			
			k = DivideFaceByPlane(NewFace,P,&Face1,&Face2,FALSE);
			if (k==-4)
			{
//				printf("Duplicate plane encountered, face removed\n");
				flag=2;
				break;
			}
			if (k==-3)
			{
//   			printf("Error splitting face, face removed\n");
				flag=1;
				break;
			}
			if (k==-1)
			{
//				printf("Face exceeded brush bounds, face removed\n");
				flag=1;
				break;
			}
			NewFace = Face1;
		}
		if (flag)
		{
         B->faces[i].misc = flag;
			continue;
		}
		
		B->faces[i] = NewFace;
		B->faces[i].misc = 0;

      for (j=0;j<B->faces[i].numpts;j++)
      {
         if (fabs((int)B->faces[i].pts[j].x-B->faces[i].pts[j].x)<0.01)
            B->faces[i].pts[j].x=(int)B->faces[i].pts[j].x;

         if (fabs((int)B->faces[i].pts[j].y-B->faces[i].pts[j].y)<0.01)
            B->faces[i].pts[j].y=(int)B->faces[i].pts[j].y;

         if (fabs((int)B->faces[i].pts[j].z-B->faces[i].pts[j].z)<0.01)
            B->faces[i].pts[j].z=(int)B->faces[i].pts[j].z;
      }
	}

/*
Extra tests to get rid of bad faces. First, join vertices that are close
together. Then, remove any degenerated faces. Finally, check that the
face has a normal. If it doesn't, remove it.
*/

   for (i=0;i<B->numfaces;i++)
   {
      f=&B->faces[i];
      if (f->misc)
         continue;

      for (j=0;j<f->numpts;)
      {
         k=(j+1)%f->numpts;
         if ( (fabs(f->pts[j].x - f->pts[k].x)<0.1) &&
              (fabs(f->pts[j].y - f->pts[k].y)<0.1) &&
              (fabs(f->pts[j].z - f->pts[k].z)<0.1))
         {
//            printf("remove %i %i  (%i)\n",j,k,f->numpts);
            f->numpts--;
            if (k)
            {
               for (l=k;l<f->numpts;l++)
               {
//                  printf("%i\n",l);
                  f->pts[l]=f->pts[l+1];
               }
            }
         }
         else
         {
            j++;
         }
      }
      if (f->numpts<3)
         f->misc=1;
   }

	i=0;
	j=B->numfaces;
	flag=FALSE;

	do
	{
		if (B->faces[i].misc)
		{
			if (RemoveFace(B,i)==FALSE)
			{
				flag=TRUE;
				break;
			}
			j--;
		} else i++;
	} while (i<j);

	if ((flag) || (B->numfaces<4))
	{
		Q_free(B->faces);
		return FALSE;
	}

/*   printf("B->numfaces=%i\n",B->numfaces);
   for (i=0;i<B->numfaces;i++)
   {
      printf("%i  %i (",i,B->faces[i].numpts);
      for (j=0;j<B->faces[i].numpts;j++)
         printf(" (%g %g %g) ",
            B->faces[i].pts[j].x,
            B->faces[i].pts[j].y,
            B->faces[i].pts[j].z);
      printf(")\n");
   }*/

	return TRUE;
}


int BuildBrush(brush_my_t *B, brush_t *b)
{
	edge_t          tmpedge;
	int             i,j,k,l;
	int             flag;
	vec3_t *oldvert=NULL;
	edge_t *oldedge;

	if (!CreateBrushFaces(B))
	{
		return FALSE;
	}

	b->plane = (plane_t *)Q_malloc(sizeof(plane_t) * (B->numfaces));
	if (b->plane==NULL)
	{
		HandleError("BuildBrush","Could not allocate brush planes");
		Q_free(B->faces);
		return FALSE;
	}
	b->num_planes = B->numfaces;
	
	b->edges = NULL;
	b->num_edges = 0;
	b->verts = NULL;
	b->num_verts = 0;
	b->tverts = NULL;
   b->sverts = NULL;
	
	for (i=0;i<B->numfaces;i++)
	{
      b->plane[i].tex    = B->faces[i].tex;
		
		b->plane[i].normal = B->faces[i].normal;
		
		b->plane[i].verts = (int *)Q_malloc(sizeof(int) * (B->faces[i].numpts));
		if (b->plane[i].verts==NULL)
		{
			Q_free(B->faces);
			HandleError("BuildBrush","Could not allocate brush plane vertices");
			return FALSE;
		}
		
		b->plane[i].num_verts = B->faces[i].numpts;
		
		for (j=0;j<B->faces[i].numpts;j++)
		{
			flag=FALSE;
			for (k=0;k<b->num_verts;k++)
			{
				if (VertsAreEqual(b->verts[k],B->faces[i].pts[j]))
				{
					flag=TRUE;
					break;
				}
			}
			if (flag==TRUE) b->plane[i].verts[j] = k;
			else
			{
				oldvert = b->verts;
				b->verts = (vec3_t *)Q_realloc(b->verts,sizeof(vec3_t) * (b->num_verts+1));
				if (b->verts==NULL)
				{
					b->verts=oldvert;
					Q_free(B->faces);
					HandleError("BuildBrush","Could not allocate brush vertices");
					return FALSE;
				}
				oldvert=b->verts;
				b->verts[b->num_verts] = B->faces[i].pts[j];
				b->plane[i].verts[j] = b->num_verts;
				b->num_verts++;
			}
		}
		
		for (j=0;j<b->plane[i].num_verts;j++)
		{
			if (j==b->plane[i].num_verts-1)
			   l=0;
         else
            l=j+1;
			flag=FALSE;
			for (k=0;k<b->num_edges;k++)
			{
				tmpedge.startvertex = b->plane[i].verts[j];
				tmpedge.endvertex   = b->plane[i].verts[l];
				if (EdgesAreEqual(b->edges[k],tmpedge))
				{
					flag=TRUE;
					break;
				}
			}
			if (flag==TRUE)
			{
			} /* do nothing, the edge already has been made*/
			else
			{
				oldedge = b->edges;
				b->edges = (edge_t *)Q_realloc(b->edges,sizeof(edge_t) * (b->num_edges+1));
				if (b->edges==NULL)
				{
					b->edges=oldedge;
					b->verts=oldvert;
					Q_free(B->faces);
					HandleError("BuildBrush","Could not allocate brush edges");
					return FALSE;
				}
				oldedge=b->edges;
				
				b->edges[b->num_edges].startvertex = b->plane[i].verts[j];
				b->edges[b->num_edges].endvertex   = b->plane[i].verts[l];
				b->num_edges++;
			}
		}
	}
	Q_free(B->faces);
	b->tverts = (vec3_t *)Q_malloc(sizeof(vec3_t) * (b->num_verts));
	if (b->tverts==NULL)
	{
		HandleError("BuildBrush","Could not allocate brush verts");
		return FALSE;
	}
	b->sverts = (svec_t *)Q_malloc(sizeof(svec_t) * (b->num_verts));
	if (b->sverts==NULL)
	{
		HandleError("BuildBrush","Could not allocate brush verts");
		return FALSE;
	}
	
	return TRUE;
}


brush_t *MyBrushIntoMap(brush_my_t *B, entity_t *EntityRef, group_t *Group)
{
	edge_t          tmpedge;
	int             i,j,k,l;
	int             flag;
	brush_t *b;        

   b=B_New(BR_NORMAL);
   if (!b)
   {
      HandleError("BuildBrush","Out of memory!");
      return NULL;
   }
	
	b->plane = (plane_t *)Q_malloc(sizeof(plane_t) * (B->numfaces));
	if (b->plane==NULL)
	{
		HandleError("BuildBrush","Could not allocate brush planes");
		return NULL;
	}
	b->num_planes = B->numfaces;
	
	b->edges = NULL;
	b->num_edges = 0;
	b->verts = NULL;
	b->num_verts = 0;
	b->tverts = NULL;
	b->sverts = NULL;
	
	for (i=0;i<B->numfaces;i++)
	{
      b->plane[i].tex    = B->faces[i].tex;
		
		b->plane[i].normal = B->faces[i].normal;
		
		b->plane[i].verts = (int *)Q_malloc(sizeof(int) * (B->faces[i].numpts));
		if (b->plane[i].verts==NULL)
		{
			HandleError("BuildBrush","Could not allocate brush plane vertices");
			return NULL;
		}
		
		b->plane[i].num_verts = B->faces[i].numpts;
		
		for (j=0;j<B->faces[i].numpts;j++)
		{
			flag=FALSE;
			for (k=0;k<b->num_verts;k++)
			{
				if (VertsAreEqual(b->verts[k],B->faces[i].pts[j]))
				{
					flag=TRUE;
					break;
				}
			}
			if (flag==TRUE)
			   b->plane[i].verts[j] = k;
			else
			{
				b->verts = (vec3_t *)Q_realloc(b->verts,sizeof(vec3_t) * (b->num_verts+1));
				if (b->verts==NULL)
				{
					HandleError("BuildBrush","Could not allocate brush vertices");
					return NULL;
				}
				b->verts[b->num_verts] = B->faces[i].pts[j];
				b->plane[i].verts[j] = b->num_verts;
				b->num_verts++;
			}
		}

		for (j=0;j<b->plane[i].num_verts;j++)
		{
			if (j==b->plane[i].num_verts-1)
			   l=0;
         else
            l=j+1;
			flag=FALSE;
			for (k=0;k<b->num_edges;k++)
			{
				tmpedge.startvertex = b->plane[i].verts[j];
				tmpedge.endvertex   = b->plane[i].verts[l];
				if  (EdgesAreEqual(b->edges[k],tmpedge))
				{
					flag=TRUE;
					break;
				}
			}
			if (flag==TRUE)
			{
			} /* do nothing, the edge already has been made*/
			else
			{
				b->edges = (edge_t *)Q_realloc(b->edges,sizeof(edge_t) * (b->num_edges+1));
				if (b->edges==NULL)
				{
					HandleError("BuildBrush","Could not allocate brush edges");
					return NULL;
				}
				
				b->edges[b->num_edges].startvertex = b->plane[i].verts[j];
				b->edges[b->num_edges].endvertex   = b->plane[i].verts[l];
				b->num_edges++;
			}
		}
	}
	Q_free(B->faces);
	b->tverts = (vec3_t *)Q_malloc(sizeof(vec3_t) * (b->num_verts));
	if (b->tverts==NULL)
	{
		HandleError("BuildBrush","Could not allocate brush verts");
		return NULL;
	}
	b->sverts = (svec_t *)Q_malloc(sizeof(svec_t) * (b->num_verts));
	if (b->sverts==NULL)
	{
		HandleError("BuildBrush","Could not allocate brush verts");
		return NULL;
	}

   b->EntityRef=EntityRef;
   b->Group=Group;

   B_Link(b);

	return b;
}


int BrushesIntersect(brush_my_t *b1, brush_my_t *b2)
{
	int i,j,k;
	face_my_t Face1,Face2;
	int totalfront;
	int totalon;
	
	totalon=0;
	for (i=0;i<b1->numfaces;i++)
	{
		totalfront=0;
		for (j=0;j<b2->numfaces;j++)
		{
			k = DivideFaceByPlane(b2->faces[j],b1->faces[i],&Face1,&Face2,FALSE);
			if (k==-2) totalfront++;
			if (k==-4)
			{
			   totalon++;
			   totalfront++;
         }
		}
      if (totalfront==b2->numfaces)
      {
         return FALSE;
      }
	}
	if (totalon==b1->numfaces)
	{
	   return FALSE;
   }
	
	totalon=0;
	for (i=0;i<b2->numfaces;i++)
	{
		totalfront=0;
		for (j=0;j<b1->numfaces;j++)
		{
			k = DivideFaceByPlane(b1->faces[j],b2->faces[i],&Face1,&Face2,FALSE);
			if (k==-2) totalfront++;
			if (k==-4)
			{
			   totalon++;
			   totalfront++;
         }
		}
      if (totalfront==b1->numfaces)
      {
         return FALSE;
      }
	}
	if (totalon==b2->numfaces)
	{
	   return FALSE;
   }

	return TRUE;
}


static int SplitBrushWithPlane(brush_my_t *Bfront, face_my_t P,brush_my_t *Bback)
/* Returns:
    0 - The Brush (bfront) was split into 2 brushes, a front and a back
    -1 - The Brush (bfront) was entirely in front of the plane, not split
    -2 - The Brush (bfront) was entirely behind the plane, not split
    -3 - Error
*/
{                               
	brush_my_t Fbrush,Bbrush;
	face_my_t Face1,Face2;
	face_my_t NewFace;
	plane_my_t CutPlane;
	int j,k;
	int ontest=0;
	int fronttest=0;
	int backtest=0;
		
	Fbrush.numfaces = 0;
	Bbrush.numfaces = 0;
	Fbrush.faces=NULL;
	Bbrush.faces=NULL;
		
	for (j=0;j<Bfront->numfaces;j++)
	{
		k = DivideFaceByPlane(Bfront->faces[j],P,&Face1,&Face2,TRUE);
		if (k==-4)
		{
			/*printf("Duplicate plane encountered, face removed\n");*/
			Fbrush.faces = (face_my_t *)Q_realloc(Fbrush.faces,sizeof(face_my_t) * (Fbrush.numfaces+1));
			if (Fbrush.faces==NULL) return -3;
			Bbrush.faces = (face_my_t *)Q_realloc(Bbrush.faces,sizeof(face_my_t) * (Bbrush.numfaces+1));
			if (Bbrush.faces==NULL) return -3;
			Fbrush.faces[Fbrush.numfaces] = Bfront->faces[j];
			Bbrush.faces[Bbrush.numfaces] = Bfront->faces[j];
			Fbrush.numfaces++;
			Bbrush.numfaces++;
			ontest=1;
			continue;                                                
		}
		if (k==-3)
		{
			/*printf("Error splitting face, face removed\n");*/
			continue;
		}
		if (k==0)
		{
			/*printf("Face is split by plane, put split sides on*/
			/*corresponding brush\n");*/
			Bbrush.faces = (face_my_t *)Q_realloc(Bbrush.faces,sizeof(face_my_t) * (Bbrush.numfaces+1));
			if (Bbrush.faces==NULL) return -3;
			Bbrush.faces[Bbrush.numfaces] = Face2;
			Bbrush.numfaces++;
			backtest=1;
			Fbrush.faces = (face_my_t *)Q_realloc(Fbrush.faces,sizeof(face_my_t) * (Fbrush.numfaces+1));
			if (Fbrush.faces==NULL) return -3;
			Fbrush.faces[Fbrush.numfaces] = Face1;
			Fbrush.numfaces++;
			fronttest=1;
			continue;
		}
		if (k==-1)
		{
			/*printf("Face is behind plane, put on back brush\n");*/
			Bbrush.faces = (face_my_t *)Q_realloc(Bbrush.faces,sizeof(face_my_t) * (Bbrush.numfaces+1));
			if (Bbrush.faces==NULL) return -3;
			Bbrush.faces[Bbrush.numfaces] = Face2;
			Bbrush.numfaces++;
			backtest=1;
			continue;
		}
		if (k==-2)
		{
			/*printf("face is in front of plane, put on front brush\n");*/
			Fbrush.faces = (face_my_t *)Q_realloc(Fbrush.faces,sizeof(face_my_t) * (Fbrush.numfaces+1));
			if (Fbrush.faces==NULL) return -3;
			Fbrush.faces[Fbrush.numfaces] = Face1;
			Fbrush.numfaces++;
			fronttest=1;
			continue;
		}
	}
	
	/* set up return values, why not */
	if ((ontest==1)&&(fronttest==1)&&(backtest==1))
	{
		Q_free(Fbrush.faces);
		Q_free(Bbrush.faces);
		return -3;
	}
	if ((ontest==1)&&(fronttest==0)&&(backtest==0))
	{
		if (Fbrush.faces!=NULL) Q_free(Fbrush.faces);
		if (Bbrush.faces!=NULL) Q_free(Bbrush.faces);
		return -3;
	}
	if ((ontest==0)&&(fronttest==0)&&(backtest==0)) return -3;
	
	if ((fronttest==1)&&(backtest==0))
	{
		Q_free(Fbrush.faces);
		Q_free(Bfront->faces);
		return -2;
		/* in subtraction, there should be no planes that are
		completely behind the brush (the brush is completely in
		front of the plane */
	}
	if ((fronttest==0)&&(backtest==1))
	{
		/* The brush is entirely behind the plane. No splitting of the
		brush occured. */
		Q_free(Bbrush.faces);
		return -1;
	}
	
	/* the remaing case  ((fronttest==1)&&(backtest==1)&&(ontest==0))*/
	

	/* Cap the new "front brush" */
	NewFace.normal.x =  -P.normal.x;
	NewFace.normal.y =  -P.normal.y;
	NewFace.normal.z =  -P.normal.z;
	NewFace.dist =      -P.dist;
	MakeBoxOnPlane(&NewFace);

	for (j=0;j<Fbrush.numfaces;j++)
	{
		CutPlane.normal.x = -Fbrush.faces[j].normal.x;
		CutPlane.normal.y = -Fbrush.faces[j].normal.y;
		CutPlane.normal.z = -Fbrush.faces[j].normal.z;
		CutPlane.dist = -Fbrush.faces[j].dist;

		k = DivideFaceByPlane(NewFace,CutPlane,&Face1,&Face2,FALSE);
		if (k==-4)
		{
			printf("Shouldnt be here dude\n");
			continue;
		}
		if (k==-3)
		{
			printf("Error splitting face, plane discounted\n");
			continue;
		}
		if ((k==-1)||(k==-2))
		{
			/*printf("Face exceeded brush bounds, plane discounted\n");*/
			continue;
		}
		NewFace = Face1;
	}
   NewFace.tex = P.tex;
	
	Fbrush.faces = (face_my_t *)Q_realloc(Fbrush.faces,sizeof(face_my_t) * (Fbrush.numfaces+1));
	if (Fbrush.faces==NULL) return -3;
	Fbrush.faces[Fbrush.numfaces] = NewFace;
	Fbrush.numfaces++;
	
	/* Cap the new "back brush" */
	NewFace.normal.x =  P.normal.x;
	NewFace.normal.y =  P.normal.y;
	NewFace.normal.z =  P.normal.z;
	NewFace.dist =      P.dist;
	MakeBoxOnPlane(&NewFace);

	for (j=0;j<Bbrush.numfaces;j++)
	{
		CutPlane.normal.x = -Bbrush.faces[j].normal.x;
		CutPlane.normal.y = -Bbrush.faces[j].normal.y;
		CutPlane.normal.z = -Bbrush.faces[j].normal.z;
		CutPlane.dist = -Bbrush.faces[j].dist;

		k = DivideFaceByPlane(NewFace,CutPlane,&Face1,&Face2,FALSE);
		if (k==-4)
		{
			printf("Face already on plane, shouldnt be here dude\n");
			continue;
		}
		if (k==-3)
		{
			printf("Error splitting face, face removed\n");
			continue;
		}
		if ((k==-1)||(k==-2))
		{
			/*printf("Face exceeded brush bounds, face removed\n");*/
			continue;
		}
		NewFace = Face1;
	}
   NewFace.tex = P.tex;
	
	Bbrush.faces = (face_my_t *)Q_realloc(Bbrush.faces,sizeof(face_my_t) * (Bbrush.numfaces+1));
	if (Bbrush.faces==NULL) return -3;
	Bbrush.faces[Bbrush.numfaces] = NewFace;
	Bbrush.numfaces++;
	
	Q_free(Bfront->faces);

	Bfront->numfaces = Fbrush.numfaces;
	Bfront->faces = Fbrush.faces;

	Bback->numfaces = Bbrush.numfaces;
	Bback->faces = Bbrush.faces;
	return 0;
}

static int SubtractBrushes(brush_t *b1, brush_t *b2)
/* 
   Returns:  0 = succesful
	    -1 = Brushes do not intersect
	    -2 = Error Subtracting
*/
{
	int             i,j,k;
	brush_my_t B;
	brush_my_t B1;
	brush_my_t B2;
	entity_t *EntityRef;
	brush_t *b;
   int axis;
	
	if (b1==b2) return -1;
	
	EntityRef = b1->EntityRef;        
	
	/* The brush we are going to be subtracting from */
	B1.numfaces = b1->num_planes;
	B1.faces = (face_my_t *)Q_malloc(sizeof(face_my_t) * (B1.numfaces));
	if (B1.faces==NULL) return -2;
	
	/* The brush that will represent the intersection of the 2 originals */
   /* Why intersect? The brush we're subtracting with */
	B2.numfaces = b2->num_planes;
	B2.faces = (face_my_t *)Q_malloc(sizeof(face_my_t) * (B2.numfaces));
	if (B2.faces==NULL) return -2;
	
	for (i=0;i<b1->num_planes;i++)
	{
		B1.faces[i].numpts = b1->plane[i].num_verts;
		for (j=0;j<b1->plane[i].num_verts;j++)
		{
			B1.faces[i].pts[j] = b1->verts[b1->plane[i].verts[j]];
		}
      B1.faces[i].tex  = b1->plane[0].tex;

		B1.faces[i].misc = 0;
		B1.faces[i].normal = b1->plane[i].normal;
		B1.faces[i].dist = DotProd(B1.faces[i].normal,b1->verts[b1->plane[i].verts[0]]);
	}
	
	for (i=0;i<b2->num_planes;i++)
	{
      B2.faces[i].tex     = b1->plane[0].tex;
		
		B2.faces[i].misc = 0;
		B2.faces[i].normal = b2->plane[i].normal;
		B2.faces[i].dist = DotProd(B2.faces[i].normal,b2->verts[b2->plane[i].verts[0]]);
	}

	if (!CreateBrushFaces(&B2))
	{
		Q_free(B1.faces);
		return -2;
	}
	if (!BrushesIntersect(&B1,&B2))
	{
		Q_free(B1.faces);
		Q_free(B2.faces);
      NewMessage("Brushes don't intersect!");
		return -1;
	}

	/* get rid of the brush thats about to be split because we are
	   just going to re-add it in piece by piece */
	
	/* At this point, we have added the brushes, we have their intersecting        
	   region defined in B2 */
	/* When splitting with the planes from B2, B1 will contain 1 half,
	   and B the other. B1 will be the front half and B the back. Note
	   that in subtraction, there will never be just a front half. */
   for (axis=0;axis<2;axis++)
   {
   	for (i=0;i<B2.numfaces;i++)
   	{
         if ((fabs(B2.faces[i].normal.x)==1) ||
             (fabs(B2.faces[i].normal.y)==1) ||
             (fabs(B2.faces[i].normal.z)==1))
         {
            if (axis)
               continue;
         }
         else
         {
            if (!axis)
               continue;
         }
   
   		k = SplitBrushWithPlane(&B1,B2.faces[i],&B);
   		if (k==-2) return -2; /*should never happen*/
   		if (k==-1)
   		{
   		}/* Dont need to do anything, there is no split*/
   		if (k==0)
   		{
   			/* Create a new brush and continue working splitting B1 */
   			b=MyBrushIntoMap(&B1,EntityRef,b1->Group);
   			B1=B;
   		}
   	}
   }
	if (B1.faces!=NULL) Q_free(B1.faces);                      
	if (B2.faces!=NULL) Q_free(B2.faces);
	return 0;
}

void BooleanSubtraction(void)
{
	int numsubtracts=0;
	int k,newnum,newnum2;
	brushref_t *Smashlist;
	brushref_t *Hammerlist;
	brushref_t *b1;
	brushref_t *b2;
	brush_t *b3;
	brush_t *b4;
	brush_t *NewBrushList;
	brush_t *StopBrush=NULL;
   int deleted;

	if (M.display.num_bselected==0)
	{
		HandleError("Subtract Brushes","No brushes selected");
		return;
	}

	ClearSelVerts();
	Smashlist = M.display.bsel;
	newnum2 = M.display.num_bselected;
	for (b1=M.display.bsel;b1;b1=b1->Next)
	{
		if (!CheckBrush(b1->Brush,FALSE))
		{
			HandleError("Subtract Brushes","Brushes have errors. Cannot subtract");
			NewMessage("%d subtraction(s) performed.",numsubtracts);
			return;
		}
      if (!(b1->Brush->bt->flags&BR_F_CSG))
      {
         HandleError("Subtract Brushes","Can only use normal brushes when subtracting");
         return;
      }
	}

   SUndo(UNDO_NONE,UNDO_DELETE);

	M.display.bsel=NULL;
	M.display.num_bselected=0;
	UpdateAllViewports();
	NewMessage("Select the subtractor(s). Right click when finished.");
   do
   {
      CheckMoveKeys();
      UpdateMap();
		UpdateMouse();
	} while (mouse.button!=2);

   while (mouse.button==2) UpdateMouse();
	
	if (M.display.num_bselected==0)
	{
		NewMessage("Subtract Brushes: No subtractor. Aborted");
		M.display.bsel = Smashlist;
		M.display.num_bselected = newnum2;
		ClearSelBrushes();
		return;
	}
	for (b1=M.display.bsel;b1;b1=b1->Next)
	{
		if (!CheckBrush(b1->Brush,FALSE))
		{
			HandleError("Subtract Brushes","Brushes have errors. Cannot subtract");
			NewMessage("%d subtraction(s) performed.",numsubtracts);
			return;
		}
      
      if (!(b1->Brush->bt->flags&BR_F_CSG))
      {
         HandleError("Subtract Brushes","Can only use normal brushes when subtracting");
         return;
      }
      
		for (b2=Smashlist;b2;b2=b2->Next)
		{
			if (b2->Brush==b1->Brush)
			{
				HandleError("Subtract Brushes","Cannot have the same brush in 'from' and 'with' lists");
				NewMessage("%d subtraction(s) performed.",numsubtracts);
				return;
			}
		}
	}
	Hammerlist = M.display.bsel;
	newnum = M.display.num_bselected;

   M.display.bsel=Smashlist;
	M.display.num_bselected=newnum2;

	M.display.bsel=NULL;
	M.display.num_bselected=0;

	NewBrushList=NULL;
	for (b1=Smashlist; b1; b1=b1->Next)
	{
      deleted=0;
		for (b2=Hammerlist;b2;b2=b2->Next)
		{
			if (NewBrushList!=NULL)
			{
				for (b3=NewBrushList; b3; b3=b4)
				{
					k = SubtractBrushes(b3,b2->Brush);
					if (k==-2)
					{
						HandleError("Subtract Brushes","Error subtracting");
						NewMessage("%d subtraction(s) performed.",numsubtracts);
						ClearSelBrushes();
						M.display.bsel=Hammerlist;
						ClearSelBrushes();
						M.display.bsel=Smashlist;
						ClearSelBrushes();
						return;
					}                  
					if (b3==StopBrush)
					{
						b4=NULL;
						StopBrush=M.BrushHead;
					}
					else
					{
					   b4=b3->Last;
               }
					if (k==0)
					{
						numsubtracts++;
						if (NewBrushList==b3) NewBrushList=b3->Last;
						DeleteABrush(b3);
					}
				}
			}
			else
			{
				if (b1->Brush==M.BrushHead)
				   NewBrushList=M.BrushHead->Next;
				else
				   NewBrushList=M.BrushHead;
				k = SubtractBrushes(b1->Brush,b2->Brush);
				if (k==-2)
				{
					HandleError("Subtract Brushes","Error subtracting");
					NewMessage("%d subtraction(s) performed.",numsubtracts);
					ClearSelBrushes();
					M.display.bsel=Hammerlist;
					ClearSelBrushes();
					M.display.bsel=Smashlist;
					ClearSelBrushes();
					return;
				}
				if (k==0)
				{
					numsubtracts++;
					DeleteABrush(b1->Brush);
               deleted=1;
					NewBrushList=NewBrushList->Last;
					StopBrush=M.BrushHead;
				}
				else
				{
				   NewBrushList=NULL;
            }
			}
		}

      for (b3=NewBrushList;b3;b3=b4)
      {
		   if (b3==StopBrush)
		   {
			   b4=NULL;
		   }
		   else
		   {
		      b4=b3->Last;
         }
         AddDBrush(b3);
      }

      if (!deleted)
         AddDBrush(b1->Brush);
		NewBrushList=NULL;
	}

	/* clear out old lists*/
	M.display.bsel=Smashlist;
	ClearSelBrushes();
	M.display.bsel=Hammerlist;
	M.display.num_bselected=newnum;
	NewMessage("%d subtraction(s) performed.",numsubtracts);
}

