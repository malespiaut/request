#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "brush.h"

#include "error.h"
#include "map.h"
#include "memory.h"
#include "quest.h"
#include "uid.h"


static int Q3C_Copy(brush_t *dst,const brush_t *src)
{
   int size;
   
   dst->x.q3c=src->x.q3c;

   /* size of texture alignment information */
   size=sizeof(float)*(src->x.q3c.sizex*2+1)*(src->x.q3c.sizey*2+1);
   
   if (src->x.q3c.s)
   {
      dst->x.q3c.s=Q_malloc(size);
      if (!dst->x.q3c.s)
         return 0;
      memcpy(dst->x.q3c.s,src->x.q3c.s,size);
   }
   
   if (src->x.q3c.t)
   {
      dst->x.q3c.t=Q_malloc(size);
      if (!dst->x.q3c.t)
         return 0;
      memcpy(dst->x.q3c.t,src->x.q3c.t,size);
   }

   return 1;
}

static void Q3C_Free(brush_t *b)
{
   if (b->x.q3c.s)
      Q_free(b->x.q3c.s);
   if (b->x.q3c.t)
      Q_free(b->x.q3c.t);

   b->x.q3c.s=b->x.q3c.t=NULL;
}


brush_type_t brush_types[]=
{
{BR_NORMAL  ,"normal brush"       ,0x12},
{BR_SUBTRACT,"subtractive brush"  ,0x10},
{BR_POLYGON ,"single polygon"     ,0x10},

{BR_Q3_CURVE,"Quake 3 curve"      ,0x20,NULL    ,Q3C_Copy  ,Q3C_Free   },

{BR_CLIP    ,"clipping plane"     ,0x11},

{0}
};

static brush_type_t *B_LookupType(int type)
{
   brush_type_t *bt;
   
   for (bt=brush_types;bt->type;bt++)
      if (bt->type==type)
         return bt;

   HandleError("Brush_LookupType","Unknown brush type %i!",type);
   return brush_types;
}


brush_t *B_New(int type)
{
   brush_t *b;

   b=Q_malloc(sizeof(brush_t));
   if (!b) return NULL;
   memset(b,0,sizeof(brush_t));

   b->bt=B_LookupType(type);
   if (b->bt->b_new)
   {
      if (!b->bt->b_new(b))
      {
         B_Free(b);
         return NULL;
      }
   }

   b->uid=GetBrushUID();

   return b;
}

void B_FreeC(brush_t *b)
{
   int i;
   plane_t *p;

   if (b->Next || b->Last)
      HandleError("B_Free","Freeing linked brush!");

   if (b->bt->b_free) b->bt->b_free(b);

#define FREE(x) if (x) { Q_free((x)); (x)=NULL; }
   
   if (b->plane)
   {
      for (i=0,p=b->plane;i<b->num_planes;i++,p++)
      {
         FREE(p->verts)
      }
      Q_free(b->plane);
   }

   FREE(b->verts)
   FREE(b->tverts)
   FREE(b->sverts)
   
   FREE(b->edges)

#undef FREE
}

void B_Free(brush_t *b)
{
   B_FreeC(b);
   Q_free(b);
}


void B_Link(brush_t *b)
{
   if (b->Next || b->Last)
      HandleError("B_Link","Brush already linked!");

   if (M.BrushHead!=NULL) M.BrushHead->Last = b;
   b->Next = M.BrushHead;
   b->Last = NULL;
   M.BrushHead = b;

   CalcBrushCenter(b);
   RecalcNormals(b);
   
   M.num_brushes++;
   M.num_edges+=b->num_edges;
   M.num_vertices+=b->num_verts;
}

void B_Unlink(brush_t *b)
{
   M.num_brushes--;
   M.num_edges-=b->num_edges;
   M.num_vertices-=b->num_verts;
   
   if (b->Next != NULL) b->Next->Last = b->Last;
   if (b->Last != NULL) b->Last->Next = b->Next;
   if (b==M.BrushHead) M.BrushHead=b->Next;

   b->Next=NULL;
   b->Last=NULL;
}


void B_ChangeType(brush_t *b,int ntype)
{
   b->bt=B_LookupType(ntype);
}


brush_t *B_Duplicate(brush_t *dest, brush_t *source, int full)
{
	int i;

   if (!dest)
   {
      dest=Q_malloc(sizeof(brush_t));
      if (!dest)
      {
         HandleError("Duplicate Brush","Could not allocate brush");
         return NULL;
      }
   }
   memset(dest,0,sizeof(brush_t));
   
	/* Copy Plane Information */
	dest->plane = (plane_t *)Q_malloc(sizeof(plane_t) * (source->num_planes));
	if (dest->plane==NULL)
	{
		HandleError("Duplicate Brush","Could not allocate brush planes");
		return NULL;
	}
	dest->num_planes = source->num_planes;

	for (i=0;i<(source->num_planes);i++)
	{
		/* Copy Vertex Information from the plane */
		dest->plane[i].verts = (int *)Q_malloc(sizeof(int) * (source->plane[i].num_verts));
		if (dest->plane[i].verts==NULL)
		{
			HandleError("Duplicate Brush","Could not allocate brush plane verts");
			return NULL;
		}
      memcpy(dest->plane[i].verts,source->plane[i].verts, sizeof(int) * (source->plane[i].num_verts) );
		dest->plane[i].num_verts = source->plane[i].num_verts;

      memcpy(&dest->plane[i].tex,&source->plane[i].tex,sizeof(texdef_t));

      dest->plane[i].normal=source->plane[i].normal;
	}

   memcpy(&dest->tex,&source->tex,sizeof(texdef_t));

	dest->num_verts = source->num_verts;
   
	dest->verts = (vec3_t *)Q_malloc(sizeof(vec3_t) * (source->num_verts));
	if (dest->verts==NULL)
	{
		HandleError("Duplicate Brush","Could not allocate brush verts");
		return NULL;
	}
   memcpy(dest->verts,source->verts,sizeof(vec3_t) * (source->num_verts));
   
   if (full)
   {
   	dest->tverts = (vec3_t *)Q_malloc(sizeof(vec3_t) * (source->num_verts));
   	if (dest->tverts==NULL)
   	{
   		HandleError("Duplicate Brush","Could not allocate brush verts");
   		return NULL;
   	}
   	dest->sverts = (svec_t *)Q_malloc(sizeof(svec_t) * (source->num_verts));
   	if (dest->sverts==NULL)
   	{
   		HandleError("Duplicate Brush","Could not allocate brush verts");
   		return NULL;
   	}
   }
	

	dest->num_edges = source->num_edges;
   if (source->num_edges)
   {
   	dest->edges = (edge_t *)Q_malloc(sizeof(edge_t) * (source->num_edges));
   	if (dest->edges==NULL)
   	{
   		HandleError("Duplicate Brush","Could not allocate brush edges");
   		return NULL;
   	}
   	
   	for (i=0;i<(source->num_edges);i++)
   	{
   		dest->edges[i] = source->edges[i];
   	}
   }
	
	dest->EntityRef = source->EntityRef;
	dest->Group = source->Group;

   dest->bt=source->bt;

   if (dest->bt->b_copy)
      dest->bt->b_copy(dest,source);

   if (full)
   { /* if we're making a copy that'll be used in the map, give it a uid */
      dest->uid=GetBrushUID();
   }

   return dest;
}

