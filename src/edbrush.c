/*
edbrush.c file of the Quest Source Code

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

#include "edbrush.h"

#include "3d.h"
#include "brush.h"
#include "camera.h"
#include "check.h"
#include "clickbr.h"
#include "edit.h"
#include "edvert.h"
#include "error.h"
#include "jvert.h"
#include "keyboard.h"
#include "map.h"
#include "memory.h"
#include "mouse.h"
#include "newgroup.h"
#include "popup.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "texlock.h"
#include "texdef.h"
#include "times.h"
#include "undo.h"


static brushref_t *FindSelBrush(brush_t *b)
{
   brushref_t *bs;
   for (bs=M.display.bsel; bs; bs=bs->Next)
      if (bs->Brush == b)
         return bs;
   return NULL;
}

int AddSelBrush(brush_t *sel_brush,int addgroup)
{
	brushref_t *b;
   brush_t *br;

   if (FindSelBrush(sel_brush))
      return TRUE;

	if (M.display.bsel!=NULL)
	{
		b = (brushref_t *)Q_malloc(sizeof(brushref_t));
		if (b==NULL)
		{
			HandleError("Select Brush","Could not allocate select list");
			return FALSE;
		}
		b->Brush = sel_brush;
		b->Last = NULL;
		b->Next = M.display.bsel;
		M.display.bsel->Last=b;
		M.display.bsel = b;

		M.display.num_bselected++;
	}
	else
	{
		M.display.bsel = (brushref_t *)Q_malloc(sizeof(brushref_t));
		if (M.display.bsel==NULL)
		{
			HandleError("Select Brush","Could not allocate select list");
			return FALSE;
		}
		M.display.bsel->Brush = sel_brush;
		M.display.bsel->Last = NULL;
		M.display.bsel->Next = NULL;
		M.display.num_bselected=1;
	}

   if (addgroup)
   {
   	if (sel_brush->Group->flags & 0x04)
      {
         for (br=M.BrushHead; br; br=br->Next)
         {
            if (br->Group == sel_brush->Group)
            {
               if (!AddSelBrush(br,0))
                  return FALSE;
            }
         }
      }
   }

	return TRUE;
}

void RemoveSelBrush(brushref_t *b)
{
	vsel_t *v;

	for (v=M.display.vsel; v; v=v->Next)
	{
		if ((v->vert>=b->Brush->verts)&&(v->vert<(b->Brush->verts+sizeof(vec3_t)*(b->Brush->num_verts))))
		{
			if (v->Last!=NULL) (v->Last)->Next = v->Next;
			if (v->Next!=NULL) (v->Next)->Last = v->Last;
			if (v==M.display.vsel) M.display.vsel=v->Next;
			Q_free(v);
			M.display.num_vselected--;
		}
	}
	if (b->Last!=NULL) (b->Last)->Next = b->Next;
	if (b->Next!=NULL) (b->Next)->Last = b->Last;
	if (b==M.display.bsel) M.display.bsel=b->Next;
	Q_free(b);
	M.display.num_bselected--;
}

void SelectWindowBrush(int x0,int y0,int x1,int y1)
{
	int      numnew;
	int      i;
	int      x2, y2;
	brush_t  *b;

	if (x0 > x1)
	{
		i = x1;
		x1 = x0;
		x0 = i;
	}
	if (y0 > y1)
	{
		i = y1;
		y1 = y0;
		y0 = i;
	}

	numnew=0;
	for (b=M.BrushHead; b; b=b->Next)
	{
		if (b->drawn)
		{
         if (!b->scenter.onscreen)
            continue;
         x2=b->scenter.x;
         y2=b->scenter.y;

			/* Enter into possible click list if within aperture */
			if ((x2 > x0) && (x2 < x1) &&
				 (y2 > y0) && (y2 < y1))
         {
            if (!FindSelBrush(b))
            {
				   numnew++;
				   AddSelBrush(b,1);
            }
			}
		}
	}
	if (numnew==0) ClearSelBrushes();
}


static brush_t *SelectBrush(int mx, int my)
{
   brush_t **list,**l;
   int n_list;

	int      x2, y2;
	brush_t *b;

   int i,j;

   float time;


   list=NULL;
   n_list=0;
   if (M.display.vport[M.display.active_vport].mode==SOLID)
   {
      Solid_SelectBrushes(&list,&n_list);
   }
   else
   {
   	for (b=M.BrushHead; b; b=b->Next)
   	{
   		if (!b->drawn)
   			continue;
         if (!b->scenter.onscreen)
            continue;
   
         x2=b->scenter.x;
         y2=b->scenter.y;
   
   		/* Enter into possible click list if within aperture */
   		if ((mx > x2-APERTURE) &&
   			 (mx < x2+APERTURE) &&
   			 (my > y2-APERTURE) &&
   			 (my < y2+APERTURE))
         {
            l=Q_realloc(list,sizeof(brush_t *)*(n_list+1));
            if (!l)
            {
               HandleError("SelectBrush","Out of memory!");
               Q_free(list);
               break;
            }
            list=l;
   
            for (i=0;i<n_list;i++)
            {
               if (list[i]->tcenter.z>b->tcenter.z)
                  break;
            }
            for (j=n_list;j>i;j--)
            {
               list[j]=list[j-1];
            }
            list[i]=b;
            n_list++;
   		}
   	}
   }

   if (!n_list)
      return NULL;

   time=GetTime();

   b=NULL;
   while (1)
   {
      UpdateMouse();

      if ((GetTime()-time>0.4) && (n_list!=1))
         break;
      if (!mouse.button)
      {
         b=*list;
         break;
      }
   }

   if (b)
   {
      Q_free(list);
      return b;
   }

   Popup_Init(mouse.x,mouse.y);
   for (i=0;i<n_list;i++)
   {
      b=list[i];
      Popup_AddStr("%s\t(%g %g %g)",
         b->bt->flags&BR_F_BTEXDEF?b->tex.name:b->plane[0].tex.name,
         b->center.x,b->center.y,b->center.z);
   }
   i=Popup_Display();
   Popup_Free();

   if (i==-1)
      b=NULL;
   else
      b=list[i];
   Q_free(list);
	return b;
}

brush_t *FindBrush(int mx,int my)
{
	brush_t *best;
	brush_t *b;
	float    best_val;
	int      x2, y2;

   best=NULL;
   best_val=0;
	for (b=M.BrushHead; b; b=b->Next)
	{
		if (b->drawn)
		{
         if (!b->scenter.onscreen)
            continue;
         x2=b->scenter.x;
         y2=b->scenter.y;
			/* Enter into possible click list if within aperture */
			if ((mx > x2-APERTURE) &&
				 (mx < x2+APERTURE) &&
				 (my > y2-APERTURE) &&
			 	 (my < y2+APERTURE))
         {
            if (best)
               if (best_val < b->tcenter.z)
                  continue;
            best=b;
            best_val=b->tcenter.z;
			}
		}
	}

	return best;
}

void HandleLeftClickBrush(void)
{
	vsel_t     *v;
	vec3_t     *sel_vert;
	brush_t    *sel_brush;
	brushref_t *b;
	int         m_x,m_y;
	int         d_x, d_y;
	int         x_thresh, y_thresh;
	int         base_x, base_y;
   int         i;
	int         found;

	/* Click in active vport */
   if (TestKey(KEY_CONTROL) &&
       (TestKey(KEY_LF_SHIFT) || TestKey(KEY_RT_SHIFT)))
   {
      ClickBrush();
   }
   else
	if (TestKey(KEY_CONTROL))
	{
		sel_brush = SelectBrush(mouse.x, mouse.y);
		if (sel_brush != NULL)
		{
         b=FindSelBrush(sel_brush);
         if (b)
         {
            RemoveSelBrush(b);
			}
         else
			{
				AddSelBrush(sel_brush,1);
			}
		}
		else
		{
			base_x = mouse.x;
			base_y = mouse.y;
         SelectWindow();
			if ((mouse.x == base_x) && (mouse.y == base_y))
			{
				/* None, deselect all */
				ClearSelBrushes();
				ClearSelVerts();
			}
			else
         {
				SelectWindowBrush(base_x, base_y, mouse.x, mouse.y);
         }
		}

		/* Wait for let up of mouse, and redraw all vports */
		while (mouse.button==1) GetMousePos();
		UpdateAllViewports();
	}
	else
	if (TestKey(KEY_LF_SHIFT) || TestKey(KEY_RT_SHIFT))
	{
		sel_vert = FindVertex(M.display.active_vport, mouse.x, mouse.y);
		if (sel_vert != NULL)
		{
			/* Check if we're adding or removing */
         v=FindSelectVert(sel_vert);

         if (v)
         {
            /* Removing */
            found = TRUE;
            if (v->Last!=NULL) (v->Last)->Next = v->Next;
            if (v->Next!=NULL) (v->Next)->Last = v->Last;
            if (v==M.display.vsel) M.display.vsel=v->Next;

            Q_free(v);
            M.display.num_vselected--;
			}
         else
			{
            AddSelectVec(sel_vert);
			}
			while (mouse.button == 1)
				GetMousePos();
		}
		else
		{
			base_x = mouse.x;
			base_y = mouse.y;
         SelectWindow();
			/* Add all vertecies in window to selection list */
			if ((mouse.x == base_x) && (mouse.y == base_y))
			{
				ClearSelVerts();
			}
			else
         {
				SelectWindowVert(M.display.active_vport, base_x, base_y, mouse.x, mouse.y);
         }
		}
		while (mouse.button==1)
		   GetMousePos();
		UpdateAllViewports();

	}
	else
	{
		/* Check for vertex under mouse */
		sel_vert = FindVertex(M.display.active_vport, mouse.x, mouse.y);
		if (sel_vert == NULL)
		{
			/* None, deselect all vertices */
			ClearSelVerts();
			UpdateAllViewports();
		}
		else
		{
         v=FindSelectVert(sel_vert);
			if (!v)
			{
            AddSelectVec(sel_vert);
			}

			/* Now check for mouse drag */
			m_x = mouse.x;
			m_y = mouse.y;
			x_thresh = 2;
			y_thresh = 2;
			status.move = TRUE;
			status.move_amt.x = status.move_amt.y = status.move_amt.z = 0;
         i=0;
			while (mouse.button == 1)
			{
				QUI_RedrawWindow(STATUS_WINDOW);
				GetMousePos();
            d_x = m_x - mouse.x;
				d_y = m_y - mouse.y;
				if ((d_x > x_thresh) || (-d_x > x_thresh) ||
					 (d_y > y_thresh) || (-d_y > y_thresh))
            {
               if (!i)
               {
                  SUndo(UNDO_NONE,UNDO_CHANGE);
                  i=1;
               }
					while (d_x > x_thresh)
					{
						MoveSelVert(MOVE_LEFT,0);
						d_x -= x_thresh;
					}
					while (-d_x > x_thresh)
					{
						MoveSelVert(MOVE_RIGHT,0);
						d_x += x_thresh;
					}
					while (d_y > y_thresh)
					{
						MoveSelVert(MOVE_UP,0);
						d_y -= y_thresh;
					}
					while (-d_y > y_thresh)
					{
						MoveSelVert(MOVE_DOWN,0);
						d_y += y_thresh;
					}
					SetMousePos(m_x, m_y);
					UpdateAllViewports();
				}
			}
			status.move = FALSE;
			QUI_RedrawWindow(STATUS_WINDOW);
			/* Update all involved brush centers & normals */
         if (i)
         {
			   for (b=M.display.bsel; b; b=b->Next)
   			{
   				CalcBrushCenter(b->Brush);
   				RecalcNormals(b->Brush);
   			}

            UndoDone();

   			for (b=M.display.bsel; b; b=b->Next)
   			{
               JoinVertices(b->Brush);
   				CheckBrush(b->Brush, FALSE);
   			}
         }
   
			UpdateAllViewports();
		}
	}
}


void MoveSelBrush(int dir)
{
	int i;
	int dx, dy, dz;
	brushref_t *b;

	Move(M.display.active_vport, dir, &dx, &dy, &dz, status.snap_size);
	status.move_amt.x += dx;
	status.move_amt.y += dy;
	status.move_amt.z += dz;
	for (b=M.display.bsel; b; b=b->Next)
	{
		for (i=0;i<b->Brush->num_verts;i++)
		{
			b->Brush->verts[i].x += dx;
			b->Brush->verts[i].y += dy;
			b->Brush->verts[i].z += dz;
		}
		CalcBrushCenter(b->Brush);
		CheckBrush(b->Brush, FALSE);
	}
}


/**************
Clipboard stuff
**************/
int CopyBrush(void)
{
	brush_t *b;
	brush_t *b_new;
   brushref_t *br;
//	int         i, j;

	/* Clear old clipboard */
	for (b=Clipboard.brushes;b;b=Clipboard.brushes)
	{
		Clipboard.brushes = b->Next;
      b->Next=b->Last=NULL;
      B_Free(b);
	}
	Clipboard.brushes = NULL;

	/* Add brushes to clipboard */
	for (br=M.display.bsel; br; br=br->Next)
	{
      b_new=B_Duplicate(NULL,br->Brush,0);
		if (b_new == NULL)
		{
			HandleError("CopyBrush", "Unable to allocate clipboard brush.");
			return FALSE;
		}

		b_new->Last = NULL;
		b_new->Next = Clipboard.brushes;
		if (Clipboard.brushes)
		   Clipboard.brushes->Last = b_new;
		Clipboard.brushes = b_new;
	}

	/* Store base point for copy */
	Clipboard.b_base.x = M.display.vport[M.display.active_vport].camera_pos.x;
	Clipboard.b_base.y = M.display.vport[M.display.active_vport].camera_pos.y;
	Clipboard.b_base.z = M.display.vport[M.display.active_vport].camera_pos.z;

	return TRUE;
}

int PasteBrush(void)
{
	int         i;
	brush_t    *b_orig;
	brush_t    *b_new;
	vec3_t      delta;

	if (Clipboard.brushes == NULL)
	{
		QUI_Dialog("Paste Brush", "No brushes in Clipboard");
		return FALSE;
	}

	ClearSelVerts();
	ClearSelBrushes();

	delta.x = M.display.vport[M.display.active_vport].camera_pos.x -
				 Clipboard.b_base.x;
	delta.y = M.display.vport[M.display.active_vport].camera_pos.y -
				 Clipboard.b_base.y;
	delta.z = M.display.vport[M.display.active_vport].camera_pos.z -
				 Clipboard.b_base.z;

   delta.x=SnapPointToGrid(delta.x);
   delta.y=SnapPointToGrid(delta.y);
   delta.z=SnapPointToGrid(delta.z);

   if (!delta.x && !delta.y && !delta.z)
   {
      int dx,dy,dz;

      Move(M.display.active_vport,MOVE_UP,&dx,&dy,&dz,status.snap_size);
      delta.x=SnapPointToGrid(dx);
      delta.y=SnapPointToGrid(dy);
      delta.z=SnapPointToGrid(dz);
   }

   SUndo(UNDO_NONE,UNDO_NONE);

	for (b_orig=Clipboard.brushes; b_orig; b_orig=b_orig->Next)
	{
      b_new=B_Duplicate(NULL,b_orig,1);
		if (b_new==NULL)
		{
			HandleError("Copy Brush","Could not allocate new brush");
			return FALSE;
		}

		/* Adjust brush for new camera position */
		for (i=0; i<b_new->num_verts; i++)
		{
			b_new->verts[i].x += delta.x;
			b_new->verts[i].y += delta.y;
			b_new->verts[i].z += delta.z;
		}

      B_Link(b_new);
      
      AddDBrush(b_new);

		b_new->EntityRef = M.WorldSpawn;
		b_new->Group = FindVisGroup(M.WorldGroup);

      TexLock(b_new,b_orig);

      AddSelBrush(b_new,0);
	}

	return TRUE;
}


void DeleteABrush(brush_t *b)
{
   B_Unlink(b);
   B_Free(b);
}

int DeleteBrush(void)
{
	brushref_t *b;

	for (b=M.display.bsel; b; b=b->Next)
	{
		DeleteABrush(b->Brush);
	}

	ClearSelBrushes();
	ClearSelVerts();
   
	return TRUE;
}


void ApplyTexture(brush_t *b, char *texname)
{
	int i;

   if (b->bt->flags&BR_F_BTEXDEF)
   {
      SetTexture(&b->tex,texname);
   }
   else
   {
      for (i=0; i<b->num_planes; i++)
      {
         SetTexture(&b->plane[i].tex,texname);
      }
   }
}


void HighlightAllBrushes(void)
{
	int j;
	brushref_t *b;
	vsel_t *v;

	ClearSelVerts();

	for (b=M.display.bsel; b; b=b->Next)
	{
		for (j=0; j<b->Brush->num_verts; j++)
		{
			if (M.display.vsel!=NULL)
			{
				v = (vsel_t *)Q_malloc(sizeof(vsel_t));
				if (v==NULL)
				{
					HandleError("Select All Verts","Could not allocate select list");
					return;
				}
				v->vert  = &(b->Brush->verts[j]);
				v->tvert = &(b->Brush->tverts[j]);
				v->svert = &(b->Brush->sverts[j]);
				v->Last = NULL;
				v->Next = M.display.vsel;
				M.display.vsel->Last=v;
				M.display.vsel = v;
			}
			else
			{
				M.display.vsel = (vsel_t *)Q_malloc(sizeof(vsel_t));
				if (M.display.vsel==NULL)
				{
					HandleError("Select All Verts","Could not allocate select list");
					return;
				}
				M.display.vsel->vert  = &(b->Brush->verts[j]);
				M.display.vsel->tvert = &(b->Brush->tverts[j]);
				M.display.vsel->svert = &(b->Brush->sverts[j]);
				M.display.vsel->Last = NULL;
				M.display.vsel->Next = NULL;
			}
			M.display.num_vselected++;
		}
	}
	UpdateAllViewports();
}


void ClearSelBrushes(void)
{
	brushref_t *b;

	M.display.num_bselected = 0;
	for (b=M.display.bsel; b; b=M.display.bsel)
	{
		M.display.bsel=b->Next;
		Q_free(b);
	}
	M.display.bsel=NULL;

	ClearSelVerts();
}


void AutoZoom(void)
{
	brushref_t *b;
	int   i,j;
	float xmax,ymax,zmax;
	float xmin,ymin,zmin;
   float x,y,z;
	
	xmax=ymax=zmax=-999999;
	xmin=ymin=zmin= 999999;
	
	if (M.display.num_bselected==0) return;

	for (b=M.display.bsel;b;b=b->Next)
	{
		for (i=0;i<b->Brush->num_verts;i++)
		{
			if (b->Brush->verts[i].x>xmax) xmax=b->Brush->verts[i].x;
			if (b->Brush->verts[i].x<xmin) xmin=b->Brush->verts[i].x;

			if (b->Brush->verts[i].y>ymax) ymax=b->Brush->verts[i].y;
			if (b->Brush->verts[i].y<ymin) ymin=b->Brush->verts[i].y;

			if (b->Brush->verts[i].z>zmax) zmax=b->Brush->verts[i].z;
			if (b->Brush->verts[i].z<zmin) zmin=b->Brush->verts[i].z;
		}
	}

   x=(xmin+xmax)/2;
   y=(ymin+ymax)/2;
   z=(zmin+zmax)/2;

   for (i=0;i<M.display.num_vports;i++)
   {
   	M.display.vport[i].camera_pos.x = x;
   	M.display.vport[i].camera_pos.y = y;
   	M.display.vport[i].camera_pos.z = z;
   
      for (j=0;j<100;j+=status.pan_speed)
         MoveCamera(i,MOVE_BACKWARD);
   }
}

