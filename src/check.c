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
#include "button.h"
#include "color.h"
#include "edbrush.h"
#include "edent.h"
#include "edentity.h"
#include "edface.h"
#include "edvert.h"
#include "entity.h"
#include "error.h"
#include "geom.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "video.h"


static int IsOnPlane(vec3_t norm, float d, vec3_t pt)
{
	return (fabs(norm.x * pt.x + norm.y * pt.y + norm.z * pt.z + d) < 0.015);
}


struct check_error_s;
typedef struct
{
   const char *msg;
   const char *desc;
   void (*fix)(struct check_error_s *err);
   int type;
#define CHK_BRUSH  0
#define CHK_ENTITY 1
} check_error_type_t;


typedef struct check_error_s
{
   check_error_type_t *t;
   union
   {
      brush_t *b;
      entity_t *e;
   } x;
} check_error_t;

static check_error_t *cerrors;
static int num_cerrors;

static void Check_Clear(void)
{
   Q_free(cerrors);
   cerrors=NULL;
   num_cerrors=0;
}

static void Check_AddErrorB(check_error_type_t *t,brush_t *b)
{
   check_error_t *ce;

   ce=Q_realloc(cerrors,sizeof(check_error_t)*(num_cerrors+1));
   if (!ce)
   {
      HandleError("Check_AddErrorB","Out of memory!");
      return;
   }
   cerrors=ce;
   ce=&ce[num_cerrors++];
   ce->t=t;
   ce->x.b=b;
}

static void Check_AddErrorE(check_error_type_t *t,entity_t *e)
{
   check_error_t *ce;

   ce=Q_realloc(cerrors,sizeof(check_error_t)*(num_cerrors+1));
   if (!ce)
   {
      HandleError("Check_AddErrorE","Out of memory!");
      return;
   }
   cerrors=ce;
   ce=&ce[num_cerrors++];
   ce->t=t;
   ce->x.e=e;
}


static void Check_Brush(brush_t *b)
{
/* TODO: some of these can be 'fixed' by re-csg'ing the brush */
static check_error_type_t
err_degen=
   {"Degenerated brush",
    "The brush is degenerated, i.e. it has\n"
    "less than 4 faces or less than 4\n"
    "vertices."
    ,NULL     ,CHK_BRUSH},
err_degen_edge=
   {"Degenerated edge",
    NULL
    ,NULL,CHK_BRUSH},
err_degen_face=
   {"Degenerated face",
    NULL
    ,NULL,CHK_BRUSH},
err_face_no_normal=
   {"Face has no normal",
    NULL
    ,NULL,CHK_BRUSH},
err_noncoplanar=
   {"Vertex on face non-coplanar",
    NULL
    ,NULL,CHK_BRUSH},
err_convex=
   {"Brush isn't convex",
    NULL
    ,NULL,CHK_BRUSH};

	int      i, j, k;
	vec3_t   normal;
	vec3_t   delta;
	vec3_t   center1, center2;
	float    dist;

#define Err(x) \
   { \
      Check_AddErrorB(x,b); \
      return; \
   }

/* TODO : some tests apply to other brush types */
   if (b->bt->type!=BR_NORMAL) return;

   if ((b->num_planes<4) || (b->num_verts<4))
   {
      Err(&err_degen);
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

         Err(&err_degen_edge);
      }
   }

	/* Check each face in the brush for coplanarity */
	for (i=0; i<b->num_planes; i++)
	{
      if (b->plane[i].num_verts<3)
      {
         Err(&err_degen_face);
      }

		/* Generate normal-dist plane representation */
		normal.x = b->plane[i].normal.x;
		normal.y = b->plane[i].normal.y;
		normal.z = b->plane[i].normal.z;

      dist=normal.x*normal.x+normal.y*normal.y+normal.z*normal.z;
      if (dist<0.6)
      {
         Err(&err_face_no_normal);
      }

		dist = -DotProd(b->plane[i].normal, b->verts[b->plane[i].verts[0]]);

		/* Check each vertex on the plane */
		for (j=0; j<b->plane[i].num_verts; j++)
		{
			if (!IsOnPlane(normal, dist, b->verts[b->plane[i].verts[j]]))
			{
            Err(&err_noncoplanar);
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
            Err(&err_convex);
			}
		}
	}
#undef Err
}


static void CE_SetOrigin(check_error_t *e)
{
   SetKeyValue(e->x.e,"origin","0 0 0");
}

static void Check_Entity(entity_t *e)
{
static check_error_type_t
err_no_origin=
   {"Entity has no origin and no brushes",
    "The entity has no origin key, and no\n"
    "brushes are attached to it. Thus, it\n"
    "can't be selected, and likely does\n"
    "nothing. If it's supposed to have an\n"
    "origin, edit it and add one. Otherwise,\n"
    "delete it."
    ,CE_SetOrigin,CHK_ENTITY};

   if (!GetKeyValue(e,"origin") && e!=M.WorldSpawn)
   {
      brush_t *b;
      for (b=M.BrushHead;b;b=b->Next)
         if (b->EntityRef==e)
            break;
      if (!b)
         Check_AddErrorE(&err_no_origin,e);
   }
}


static void Check_All(void)
{
	brush_t *b;
   entity_t *e;

   Check_Clear();

	for (b=M.BrushHead;b;b=b->Next)
	{
		if (!b->Group)
			continue;
		if ((b->Group->flags & 0x2))
         continue;
      if (M.showsel && b->hidden)
         continue;
      Check_Brush(b);
	}
   for (e=M.EntityHead;e;e=e->Next)
   {
      if (!e->Group)
         continue;
      if ((e->Group->flags & 0x02))
         continue;
      if (M.showsel && e->hidden)
         continue;
      Check_Entity(e);
   }
}


void ConsistencyCheck(void)
{
   int bp,b_done,b_fix,b_delete,b_edit,b_goto,b_up,b_dn;
   QUI_window_t *w;
	unsigned char *temp_buf;

   int l_x1,l_y1,l_x2,l_y2;

   int y,sy;
   int draw;
   int i,j;

   check_error_t *ce;


   Check_All();
   if (!num_cerrors)
   {
      NewMessage("No errors detected in map!");
      return;
   }

   ClearSelVerts();
   ClearSelFaces();
   ClearSelBrushes();
   ClearSelEnts();

	w=&Q.window[POP_WINDOW_1+Q.num_popups];
	w->size.x=600;
	w->size.y=256;
	w->pos.x=(video.ScreenWidth-w->size.x)/2;
	w->pos.y=(video.ScreenHeight-w->size.y)/2;

   l_x1=w->pos.x+4;
   l_x2=l_x1+560;
   l_y1=w->pos.y+32;
   l_y2=l_y1+16*10+4;

   PushButtons();
	b_done=AddButtonText(0,0,B_ENTER|B_ESCAPE,"Done");
   b_fix=AddButtonText(0,0,0,"Fix");
   b_delete=AddButtonText(0,0,0,"Delete");
   b_edit=AddButtonText(0,0,0,"Edit");
   b_goto=AddButtonText(0,0,0,"Goto");

	MoveButton(b_done  ,w->pos.x+4,w->pos.y+w->size.y-30);
   MoveButton(b_fix   ,button[b_done  ].x+button[b_done  ].sx+4,button[b_done].y);
   MoveButton(b_delete,button[b_fix   ].x+button[b_fix   ].sx+4,button[b_done].y);
   MoveButton(b_edit  ,button[b_delete].x+button[b_delete].sx+4,button[b_done].y);
   MoveButton(b_goto  ,button[b_edit  ].x+button[b_edit  ].sx+4,button[b_done].y);

   b_up=AddButtonPic(0,0,B_RAPID,"button_tiny_up");
   b_dn=AddButtonPic(0,0,B_RAPID,"button_tiny_down");
   MoveButton(b_up,l_x2+1,l_y1);
   MoveButton(b_dn,l_x2+1,l_y2-button[b_dn].sy);

	QUI_PopUpWindow(POP_WINDOW_1+Q.num_popups,"Consistency check",&temp_buf);
	Q.num_popups++;

	DrawButtons();

   y=sy=0;
   draw=1;
   bp=-1;

   while (1)
   {
      if (draw)
      {
         if (draw==2)
         {
            Check_All();
            if (!num_cerrors)
            {
               HandleError("ConsistencyCheck","No errors left!");
               break;
            }
         }
         if (y>=num_cerrors) y=num_cerrors-1;
         if (y<0) y=0;

         DrawSolidBox(l_x1,l_y1,l_x2,l_y2,GetColor(BG_COLOR));
         QUI_Frame(l_x1,l_y1,l_x2,l_y2);

         for (ce=&cerrors[sy],i=0;i<10;i++,ce++)
         {
            if (i+sy>=num_cerrors)
               break;
            if (i+sy==y)
               j=15;
            else
               j=0;

            QUI_DrawStrM(l_x1+2,l_y1+2+16*i,l_x2-164,BG_COLOR,j,0,0,
               "%s",ce->t->msg);

            switch (ce->t->type)
            {
            case CHK_BRUSH:
               QUI_DrawStrM(l_x2-162,l_y1+2+16*i,l_x2-2,BG_COLOR,j,0,0,
                  "b(%0.0f %0.0f %0.0f)",
                  ce->x.b->center.x,
                  ce->x.b->center.y,
                  ce->x.b->center.z);
               break;
            case CHK_ENTITY:
               if (GetKeyValue(ce->x.e,"origin"))
                  QUI_DrawStrM(l_x2-162,l_y1+2+16*i,l_x2-2,BG_COLOR,j,0,0,
                     "%s at %s",
                     GetKeyValue(ce->x.e,"classname"),
                     GetKeyValue(ce->x.e,"origin"));
               else
                  QUI_DrawStrM(l_x2-162,l_y1+2+16*i,l_x2-2,BG_COLOR,j,0,0,
                     "%s",
                     GetKeyValue(ce->x.e,"classname"));
               break;
            }
         }

	      RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);
         draw=0;
      }

      UpdateMouse();
      bp=UpdateButtons();
      if (bp==b_done)
         break;
      if (bp==b_fix)
      {
         if (cerrors[y].t->fix)
            cerrors[y].t->fix(&cerrors[y]);
         else
            HandleError("ConsistencyCheck",
               "This error can't be fixed automatically!");
         draw=2;
         continue;
      }

      if (bp==b_goto)
         break;

      if (bp==b_delete)
      {
         if (cerrors[y].t->type==CHK_ENTITY)
         {
            DeleteEntity(cerrors[y].x.e);
         }
         else
         if (cerrors[y].t->type==CHK_BRUSH)
         {
            DeleteABrush(cerrors[y].x.b);
         }
         else
         {
            HandleError("ConsistencyCheck",
               "Can't use delete on this error!");
         }
         draw=2;
         continue;
      }

      if (bp==b_edit)
      {
         if (cerrors[y].t->type==CHK_ENTITY)
         {
            EditAnyEntity(1,&cerrors[y].x.e);
         }
         else
         {
            HandleError("ConsistencyCheck",
               "Can't use edit on this error!");
         }
         draw=2;
      }

      if (bp==b_up)
      {
         if (sy>0) sy--;
         draw=1;
         continue;
      }
      if (bp==b_dn)
      {
         if (sy<num_cerrors-10)
            sy++;
         draw=1;
         continue;
      }

      if (mouse.button&1 && InBox(l_x1,l_y1,l_x2,l_y2))
      {
         i=(mouse.y-l_y1)/16;
         if (i<0) i=0;
         i+=sy;
         if (i<num_cerrors)
            y=i;
         draw=1;
         continue;
      }
   }

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1+Q.num_popups,&temp_buf);

	RemoveButton(b_done);
   RemoveButton(b_fix);
   RemoveButton(b_delete);
   RemoveButton(b_edit);
   RemoveButton(b_goto);
   RemoveButton(b_up);
   RemoveButton(b_dn);
   PopButtons();

	RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);

   if (bp==b_goto)
   {
      ce=&cerrors[y];
      switch (ce->t->type)
      {
      case CHK_BRUSH:
         status.edit_mode=BRUSH;
         AddSelBrush(ce->x.b,0);
         AutoZoom();
         break;
      }
   }

   Check_Clear();
}


/* Old system */

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


#if 0
int ConsistencyCheck(void)
{
	brush_t *b;

	for (b=M.BrushHead;b;b=b->Next)
	{
		if (!CheckBrush(b,TRUE))
		{
			ClearSelVerts();
			ClearSelFaces();
			ClearSelBrushes();
			ClearSelEnts();
         status.edit_mode=BRUSH;

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

		   QUI_RedrawWindow(STATUS_WINDOW);
			UpdateAllViewports();
			return FALSE;
		}
	}
	return TRUE;
}
#endif

