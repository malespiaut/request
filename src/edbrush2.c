#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "edbrush2.h"

#include "3d.h"
#include "brush.h"
#include "camera.h"
#include "check.h"
#include "display.h"
#include "edbrush.h"
#include "edvert.h"
#include "error.h"
#include "keyboard.h"
#include "keyhit.h"
#include "map.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "newgroup.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "undo.h"


void SnapSelectedBrushes(void)
{
	int i;
	brushref_t *b;
	brush_t *brushcopy;

	if (M.display.num_bselected==0)
	{
		HandleError("Snap Brushes","No brushes selected");
		return;
	}

   SUndo(UNDO_NONE,UNDO_CHANGE);

	for (b=M.display.bsel; b; b=b->Next)
	{
      brushcopy = B_Duplicate(NULL,b->Brush,0);
      
		for (i=0; i<brushcopy->num_verts; i++)
		{
			brushcopy->verts[i].x = SnapPointToGrid(brushcopy->verts[i].x);
			brushcopy->verts[i].y = SnapPointToGrid(brushcopy->verts[i].y);
			brushcopy->verts[i].z = SnapPointToGrid(brushcopy->verts[i].z);
		}
		CalcBrushCenter(brushcopy);
		RecalcNormals(brushcopy);
      
		if (CheckBrush(brushcopy,TRUE))
      {
         B_Free(brushcopy);
			/* snap the real brush */
			for (i=0; i<b->Brush->num_verts; i++)
			{
				b->Brush->verts[i].x = SnapPointToGrid(b->Brush->verts[i].x);
				b->Brush->verts[i].y = SnapPointToGrid(b->Brush->verts[i].y);
				b->Brush->verts[i].z = SnapPointToGrid(b->Brush->verts[i].z);
			}
			CalcBrushCenter(b->Brush);
			RecalcNormals(b->Brush);
			continue;
		}
      B_Free(brushcopy);
		if (!QUI_YesNo("CheckBrush Error","Revert to the old brush?", "Yes", "No"))
		{
			for (i=0; i<b->Brush->num_verts; i++)
			{
				b->Brush->verts[i].x = SnapPointToGrid(b->Brush->verts[i].x);
				b->Brush->verts[i].y = SnapPointToGrid(b->Brush->verts[i].y);
				b->Brush->verts[i].z = SnapPointToGrid(b->Brush->verts[i].z);
			}
			CalcBrushCenter(b->Brush);
			RecalcNormals(b->Brush);
		}
	}

   UndoDone();
}

void SnapSelectedVerts(void)
{
	vsel_t  *vs;
	brush_t *b;

   SUndo(UNDO_NONE,UNDO_CHANGE);

	for (vs=M.display.vsel; vs; vs=vs->Next)
	{
		vs->vert->x = SnapPointToGrid(vs->vert->x);
		vs->vert->y = SnapPointToGrid(vs->vert->y);
		vs->vert->z = SnapPointToGrid(vs->vert->z);
	}

   UndoDone();

	for (b=M.BrushHead; b; b=b->Next)
   {
	   CalcBrushCenter(b);
		RecalcNormals(b);
   }
}


typedef struct
{
	vec3_t pt;
	vec3_t *truept;
} op_vert_t;

int RotateBrush(int vport)
{
	int         base_x;
	int         d_x;
	float       ang;
	vec3_t      cpt;
	op_vert_t  *rvert;
	brushref_t *b;
	int         num_rverts;
	int         i, j, k;
   vsel_t     *v;

	if (status.edit_mode!=BRUSH) return TRUE;
	
	if (M.display.num_bselected==0)
	{
		HandleError("Rotate Brush","No brushes selected");
		return FALSE;
	}

   SUndo(UNDO_NONE,UNDO_CHANGE);

	NewMessage("Rotate brushes.");
	NewMessage("Left-click to place, right-click to abort.");

	base_x = mouse.x;
	ang = 0;
	status.rotate = 0;

	num_rverts = 0;
	cpt.x = cpt.y = cpt.z = 0;

   if (M.display.num_vselected)
   {
      for (v=M.display.vsel;v;v=v->Next)
      {
         num_rverts++;
         cpt.x+=v->vert->x;
         cpt.y+=v->vert->y;
         cpt.z+=v->vert->z;
      }
   }
   else
   {
   	for (b=M.display.bsel; b; b=b->Next)
   	{
   		num_rverts += b->Brush->num_verts;
   		for (j=0; j<b->Brush->num_verts; j++)
   		{
   			cpt.x += b->Brush->verts[j].x;
   			cpt.y += b->Brush->verts[j].y;
   			cpt.z += b->Brush->verts[j].z;
   		}
   	}
   }

	cpt.x /= num_rverts;
	cpt.y /= num_rverts;
	cpt.z /= num_rverts;

	/* Allocate original vertex list */
	rvert = (op_vert_t *)Q_malloc(sizeof(op_vert_t) * num_rverts);
	if (rvert==NULL)
	{
		HandleError("Rotate Brush","Unable to allocate point array");
		return FALSE;
	}
	k=0;

   if (M.display.num_vselected)
   {
      for (v=M.display.vsel;v;v=v->Next)
      {
         rvert[k].pt.x=v->vert->x-cpt.x;
         rvert[k].pt.y=v->vert->y-cpt.y;
         rvert[k].pt.z=v->vert->z-cpt.z;
         rvert[k].truept=v->vert;
         k++;
      }
   }
   else
   {
   	for (b=M.display.bsel; b; b=b->Next)
   	{
   		for (j=0; j<b->Brush->num_verts; j++)
   		{
   			rvert[k].pt.x = b->Brush->verts[j].x - cpt.x;
   			rvert[k].pt.y = b->Brush->verts[j].y - cpt.y;
   			rvert[k].pt.z = b->Brush->verts[j].z - cpt.z;
   			rvert[k].truept = &(b->Brush->verts[j]);
   			k++;
   		}
   	}
   }

	d_x = 0;
	
	while (mouse.button==1) UpdateMouse();
	
	while ((mouse.button != 1) && (mouse.button != 2) && (num_rverts > 0))
	{
		GetMousePos();
		if (mouse.moved)
			d_x += mouse.prev_x - mouse.x;
      else
         continue;

		SetMousePos(base_x, mouse.y);

      while (d_x<0) d_x+=360;
      while (d_x>=360) d_x-=360;

		status.rotate = floor(d_x / status.angle_snap_size) * status.angle_snap_size;
		ang = (PI * status.rotate / 180.0);

		QUI_RedrawWindow(STATUS_WINDOW);

		/* Rotate all vertices in brushes by ang amount */
		if ((M.display.vport[vport].camera_dir & LOOK_UP) ||
			 (M.display.vport[vport].camera_dir & LOOK_DOWN))
      {
			for (i=0; i<num_rverts; i++)
			{
				rvert[i].truept->x = (rvert[i].pt.x * cos(ang) - rvert[i].pt.y * sin(ang) + cpt.x);
				rvert[i].truept->y = (rvert[i].pt.x * sin(ang) + rvert[i].pt.y * cos(ang) + cpt.y);
			}
		}
		else
		{
			switch (M.display.vport[vport].camera_dir & 0x03)
			{
				case LOOK_POS_X :
					for (i=0; i<num_rverts; i++)
					{
						rvert[i].truept->y = (rvert[i].pt.y * cos(ang) - rvert[i].pt.z * sin(ang) + cpt.y);
						rvert[i].truept->z = (rvert[i].pt.y * sin(ang) + rvert[i].pt.z * cos(ang) + cpt.z);
					}
					break;
				case LOOK_POS_Y :
					for (i=0; i<num_rverts; i++)
					{
						rvert[i].truept->x = (rvert[i].pt.x * cos(ang) - rvert[i].pt.z * sin(ang) + cpt.x);
						rvert[i].truept->z = (rvert[i].pt.x * sin(ang) + rvert[i].pt.z * cos(ang) + cpt.z);
					}
					break;
				case LOOK_NEG_X :
					for (i=0; i<num_rverts; i++)
					{
						rvert[i].truept->y = (rvert[i].pt.y * cos(ang) - rvert[i].pt.z * sin(ang) + cpt.y);
						rvert[i].truept->z = (rvert[i].pt.y * sin(ang) + rvert[i].pt.z * cos(ang) + cpt.z);
					}
					break;
				case LOOK_NEG_Y :
					for (i=0; i<num_rverts; i++)
					{
						rvert[i].truept->x = (rvert[i].pt.x * cos(ang) - rvert[i].pt.z * sin(ang) + cpt.x);
						rvert[i].truept->z = (rvert[i].pt.x * sin(ang) + rvert[i].pt.z * cos(ang) + cpt.z);
					}
					break;
			}
		}

		for (b=M.display.bsel; b; b=b->Next)
			RecalcNormals(b->Brush);

		UpdateAllViewports();
	}

	if (mouse.button == 2)
	{
      for (i=0; i<num_rverts; i++)
      {
         rvert[i].truept->x = rvert[i].pt.x + cpt.x;
         rvert[i].truept->y = rvert[i].pt.y + cpt.y;
         rvert[i].truept->z = rvert[i].pt.z + cpt.z;
      }
	}

   while (mouse.button) UpdateMouse();

	Q_free(rvert);

	for (b=M.display.bsel; b; b=b->Next)
	{
		CalcBrushCenter(b->Brush);
		RecalcNormals(b->Brush);
	}

   UndoDone();

	for (b=M.display.bsel; b; b=b->Next)
	{
		CheckBrush(b->Brush, TRUE);
	}

	status.rotate = -1;
	QUI_RedrawWindow(STATUS_WINDOW);

	UpdateAllViewports();
	return TRUE;
}


int ScaleBrush(int brush_flag)
{
	int         base_x;
	int         d_x;
	vec3_t      cpt;
	op_vert_t  *svert;
	int         num_sverts;
	int         i, j, k;
	brushref_t *b;
	vsel_t     *v;

	if (status.edit_mode!=BRUSH) return TRUE;

	if ((brush_flag)&&(M.display.bsel==NULL))
	{
		HandleError("Scale Brush","No brushes selected");
		return TRUE;
	}
	
	if ((!brush_flag)&&(M.display.vsel==NULL))
	{
		HandleError("Scale Vertices","No vertices selected");
		return TRUE;
	}

	if (brush_flag)
	{
		NewMessage("Scale brushes.");
		NewMessage("Left-click to place, right-click to abort.");
	}
	else
	{
		NewMessage("Scale vertices.");
		NewMessage("Left-click to place, right-click to abort.");
	}

   SUndo(UNDO_NONE,UNDO_CHANGE);

	base_x = mouse.x;

	num_sverts = 0;
	cpt.x = cpt.y = cpt.z = 0;
	if (brush_flag)
	{
		for (b=M.display.bsel; b; b=b->Next)
		{
			num_sverts += b->Brush->num_verts;
			for (j=0; j<b->Brush->num_verts; j++)
			{
				cpt.x += b->Brush->verts[j].x;
				cpt.y += b->Brush->verts[j].y;
				cpt.z += b->Brush->verts[j].z;
			}
		}
	}
	else
	{
		num_sverts = 0;
		for (v=M.display.vsel; v; v=v->Next)
		{
			cpt.x += v->vert->x;
			cpt.y += v->vert->y;
			cpt.z += v->vert->z;
			num_sverts++;
		}
	}

	cpt.x /= num_sverts;
	cpt.y /= num_sverts;
	cpt.z /= num_sverts;

	/* Allocate original vertex list */
	svert = (op_vert_t *)Q_malloc(sizeof(op_vert_t) * num_sverts);
	if (svert==NULL)
	{
		HandleError("Scale Brush","Unable to allocate point array");
		return FALSE;
	}

	k=0;
	if (brush_flag)
	{
		for (b=M.display.bsel; b; b=b->Next)
		{
			for (j=0; j<b->Brush->num_verts; j++)
			{
				svert[k].pt.x = b->Brush->verts[j].x - cpt.x;
				svert[k].pt.y = b->Brush->verts[j].y - cpt.y;
				svert[k].pt.z = b->Brush->verts[j].z - cpt.z;
				svert[k].truept = &(b->Brush->verts[j]);
				k++;
			}
		}
	}
	else
	{
		for (v=M.display.vsel; v; v=v->Next)
		{
			svert[k].pt.x = v->vert->x - cpt.x;
			svert[k].pt.y = v->vert->y - cpt.y;
			svert[k].pt.z = v->vert->z - cpt.z;
			svert[k].truept = v->vert;
			k++;
		}
	}


	d_x = 0;
	
	while (mouse.button==1) UpdateMouse();

	while ((mouse.button != 1) && (mouse.button != 2) && (num_sverts > 0))
	{
		GetMousePos();
		if (mouse.moved)
			d_x += mouse.prev_x - mouse.x;
      else
         continue;

		if (1 + ((float)d_x / 100) < 0)
		   d_x = -100;

		status.scale = 100 + d_x;

		QUI_RedrawWindow(STATUS_WINDOW);

		SetMousePos(base_x, mouse.y);

		/* Scale all vertices in brushes */
		for (i=0; i<num_sverts; i++)
		{
			svert[i].truept->x = (svert[i].pt.x * (1 + ((float)d_x / 100)) + cpt.x);
			svert[i].truept->y = (svert[i].pt.y * (1 + ((float)d_x / 100)) + cpt.y);
			svert[i].truept->z = (svert[i].pt.z * (1 + ((float)d_x / 100)) + cpt.z);
		}

		for (b=M.display.bsel; b; b=b->Next)
			RecalcNormals(b->Brush);

		UpdateAllViewports();
	}
	if (mouse.button == 2)
	{
		for (i=0; i<num_sverts; i++)
		{
			svert[i].truept->x = svert[i].pt.x + cpt.x;
			svert[i].truept->y = svert[i].pt.y + cpt.y;
			svert[i].truept->z = svert[i].pt.z + cpt.z;
		}
	}

   while (mouse.button) UpdateMouse();

	Q_free(svert);

	for (b=M.display.bsel; b; b=b->Next)
	{
		CalcBrushCenter(b->Brush);
		RecalcNormals(b->Brush);
	}

   UndoDone();

	for (b=M.display.bsel; b; b=b->Next)
	{
		CheckBrush(b->Brush, TRUE);
	}

	status.scale = -1;
	QUI_RedrawWindow(STATUS_WINDOW);

	UpdateAllViewports();
	return TRUE;
}


int MirrorBrush(int vport)
{
	int         base_x, base_y;
	int         d_x, d_y;
	int         dx, dy, dz;
	vec3_t      cpt,min,max;
   vec3_t     *v;
	op_vert_t  *svert;
	int         num_sverts;
	int         i, j, k;
	brushref_t *b;

	if (status.edit_mode != BRUSH)
		return TRUE;

	if (M.display.num_bselected == 0)
	{
		HandleError("MirrorBrush", "No brushes selected.");
		return FALSE;
	}

	NewMessage("Mirror brushes.");
	NewMessage("Left-click to accept, right-click to abort.");

   SUndo(UNDO_NONE,UNDO_CHANGE);

	base_x = mouse.x;
	base_y = mouse.y;

	num_sverts = 0;
	cpt.x=cpt.y=cpt.z=0;
   min.x=min.y=min.z= 10000;
   max.x=max.y=max.z=-10000;

	for (b=M.display.bsel; b; b=b->Next)
	{
		num_sverts += b->Brush->num_verts;
		for (j=0; j<b->Brush->num_verts; j++)
		{
         v=&b->Brush->verts[j];
         if (v->x>max.x) max.x=v->x;
         if (v->y>max.y) max.y=v->y;
         if (v->z>max.z) max.z=v->z;

         if (v->x<min.x) min.x=v->x;
         if (v->y<min.y) min.y=v->y;
         if (v->z<min.z) min.z=v->z;
		}
	}

	cpt.x = (min.x+max.x)/2;
	cpt.y = (min.y+max.y)/2;
	cpt.z = (min.z+max.z)/2;

	/* Allocate original vertex list */
	svert = (op_vert_t *)Q_malloc(sizeof(op_vert_t) * num_sverts);
	if (svert == NULL)
	{
		HandleError("MirrorBrush", "Unable to allocate point array");
		return FALSE ;
	}

	k=0;
	for (b=M.display.bsel; b; b=b->Next)
	{
		for (j=0; j<b->Brush->num_verts; j++)
		{
			svert[k].pt.x = b->Brush->verts[j].x - cpt.x;
			svert[k].pt.y = b->Brush->verts[j].y - cpt.y;
			svert[k].pt.z = b->Brush->verts[j].z - cpt.z;
			svert[k].truept = &(b->Brush->verts[j]);
			k++;
		}
	}

	d_x = d_y = 0;

	while (mouse.button==1) UpdateMouse();

	while ((mouse.button != 1) && (mouse.button != 2) && (num_sverts > 0))
	{
		GetMousePos();
		if (mouse.moved)
		{
			d_x += mouse.prev_x - mouse.x;
			d_y += mouse.prev_y - mouse.y;
		}
      else
         continue;

		if (abs(d_x) > abs(d_y))
		{
			if (d_x < 0)
				Move(vport, MOVE_LEFT, &dx, &dy, &dz, 1);
			else
				Move(vport, MOVE_RIGHT, &dx, &dy, &dz, 1);
		}
		else
		{
			if (d_y < 0)
				Move(vport, MOVE_UP, &dx, &dy, &dz, 1);
			else
				Move(vport, MOVE_DOWN, &dx, &dy, &dz, 1);
		}

		/* Mirror in proper axes */
		for (i=0; i<num_sverts; i++)
		{
			if (dx != 0)
				svert[i].truept->x = (-dx * svert[i].pt.x) + cpt.x;
			else
				svert[i].truept->x = (svert[i].pt.x) + cpt.x;
			if (dy != 0)
				svert[i].truept->y = (-dy * svert[i].pt.y) + cpt.y;
			else
				svert[i].truept->y = (svert[i].pt.y) + cpt.y;
			if (dz != 0)
				svert[i].truept->z = (-dz * svert[i].pt.z) + cpt.z;
			else
				svert[i].truept->z = (svert[i].pt.z) + cpt.z;
		}

		SetMousePos(base_x, base_y);

		for (b=M.display.bsel; b; b=b->Next)
			RecalcNormals(b->Brush);

		UpdateAllViewports();
	}

	if (mouse.button == 2)
	{
		for (i=0; i<num_sverts; i++)
		{
			svert[i].truept->x = svert[i].pt.x + cpt.x;
			svert[i].truept->y = svert[i].pt.y + cpt.y;
			svert[i].truept->z = svert[i].pt.z + cpt.z;
		}
	}
   for (i=0; i<num_sverts; i++)
   {
      if (svert[i].truept->x!=svert[i].pt.x+cpt.x)
         break;
      if (svert[i].truept->y!=svert[i].pt.y+cpt.y)
         break;
      if (svert[i].truept->z!=svert[i].pt.z+cpt.z)
         break;
   }
   if (i<num_sverts)
   {
   	for (b=M.display.bsel; b; b=b->Next)
   	{
   		for (i=0;i<b->Brush->num_planes;i++)
         {
            plane_t *p;
            int     *temp;

            p=&b->Brush->plane[i];
            temp=Q_malloc(p->num_verts*sizeof(int));
            for (j=0;j<p->num_verts;j++)
               temp[j]=p->verts[p->num_verts-j-1];
            Q_free(p->verts);
            p->verts=temp;
         }
      }
   }

   while (mouse.button) UpdateMouse();

	Q_free(svert);

	for (b=M.display.bsel; b; b=b->Next)
	{
		CalcBrushCenter(b->Brush);
		RecalcNormals(b->Brush);
	}

   UndoDone();

	for (b=M.display.bsel; b; b=b->Next)
	{
		CheckBrush(b->Brush, TRUE);
	}

	status.scale = -1;
	QUI_RedrawWindow(STATUS_WINDOW);

	UpdateAllViewports();

	return TRUE;
}


void NewClipPlane(void)
{
   brush_t *b;
   plane_t *p;
   int i;
   int dx,dy,dz;

   b=B_New(BR_CLIP);
   if (!b)
   {
      HandleError("NewClipPlane","Out of memory!");
      return;
   }

   b->num_planes=1;
   b->num_verts=3;
   b->num_edges=3;

   b->plane=p=Q_malloc(sizeof(plane_t));
   memset(p,0,sizeof(plane_t));

   b->verts=Q_malloc(sizeof(vec3_t)*3);
   b->tverts=Q_malloc(sizeof(vec3_t)*3);
   b->sverts=Q_malloc(sizeof(vec3_t)*3);

   b->edges=Q_malloc(sizeof(edge_t)*3);

   if (!p || !b->verts || !b->tverts || !b->sverts || !b->edges)
   {
      Q_free(p);
      Q_free(b->verts);
      Q_free(b->sverts);
      Q_free(b->tverts);
      Q_free(b->edges);
      Q_free(b);
      HandleError("NewClipPlane","Out of memory!");
      return;
   }

   p->num_verts=3;
   p->verts=Q_malloc(sizeof(int)*3);
   if (!p->verts)
   {
      Q_free(p);
      Q_free(b->verts);
      Q_free(b->sverts);
      Q_free(b->tverts);
      Q_free(b->edges);
      Q_free(b);
      HandleError("NewClipPlane","Out of memory!");
      return;
   }

   for (i=0;i<3;i++)
   {
      p->verts[i]=i;
      b->edges[i].startvertex=i;
      b->edges[i].endvertex  =(i+1)%3;

      b->verts[i].x=SnapPointToGrid(M.display.vport[M.display.active_vport].camera_pos.x);
      b->verts[i].y=SnapPointToGrid(M.display.vport[M.display.active_vport].camera_pos.y);
      b->verts[i].z=SnapPointToGrid(M.display.vport[M.display.active_vport].camera_pos.z);

      switch (i)
      {
      case 0:
         Move90(M.display.active_vport,MOVE_UP,&dx,&dy,&dz,64);
         break;
      case 1:
         Move90(M.display.active_vport,MOVE_LEFT,&dx,&dy,&dz,64);
         break;
      case 2:
         Move90(M.display.active_vport,MOVE_RIGHT,&dx,&dy,&dz,64);
         break;
      }

      b->verts[i].x+=dx;
      b->verts[i].y+=dy;
      b->verts[i].z+=dz;

      Move90(M.display.active_vport,MOVE_FORWARD,&dx,&dy,&dz,128);
      b->verts[i].x+=dx;
      b->verts[i].y+=dy;
      b->verts[i].z+=dz;
   }
   
	b->EntityRef = M.WorldSpawn;
	b->Group = FindVisGroup(M.WorldGroup);

   B_Link(b);
}


/*
-----------------------------------
 This function is used to translate one or more selected vertices
 along a plane to another vertex.  It can be used to precisely join
 brush corners and edges when no brush vertices are snapped to a grid.

 The selected vertices are first copied off to a list of vertices
 that will be modified.
 The user is then asked to select a single handle vertex for the group.
 Finally, the user selects the destination vertex.  The amount to translate
 the original set by is the difference between the handle and the
 destination.
-----------------------------------
originally by Gyro Gearloose
*/
void SnapToVertex(void)
{
	vsel_t     *vlist;
	brushref_t *blist;
	vsel_t     *v;
	brushref_t *b;
	int         numv,numb;
	vsel_t     *vhandle;
	vec3_t      diff;


	vlist = M.display.vsel;			// Remember selected verts
	numv = M.display.num_vselected;
	M.display.num_vselected = 0;
	M.display.vsel = NULL;

	blist = M.display.bsel;			// Remember selected brushes
	numb = M.display.num_bselected;
	M.display.num_bselected = 0;
	M.display.bsel = NULL;

	// Now that we have the list of vertices to move, we need one to
	// serve as the "handle" to drag from
	//
	if (numv > 1)
	{
		// re-add the brushes that were selected to the display.list
		// because we usually want to choose one vertex from them
		//
		for (b=blist; b; b=b->Next)
		{
			AddSelBrush(b->Brush,0);
		}
		UpdateAllViewports();
		NewMessage("Select vertex to drag from. Right click when finished.");
		do
		{
			CheckMoveKeys();
			UpdateMap();
			UpdateMouse();
//			ClearKeys();
		} while (mouse.button!=2);

      while (mouse.button==2) UpdateMouse();

		if (M.display.num_vselected == 1)
		{
			vhandle = M.display.vsel;			// Remember it
			M.display.num_vselected = 0;
			M.display.vsel = NULL;
		}
		else
		{
         ClearSelVerts();
			ClearSelBrushes();
			M.display.num_vselected = numv;	// Put original selection back
			M.display.vsel = vlist;
			M.display.num_bselected = numb;
			M.display.bsel = blist;
			NewMessage("Snap To Vertex: No handle vertex selected. Aborted.");
			return;
		}
	}
	else
	{
		// Use the single selected vertex as the handle vertex
		//
		vhandle = vlist;
	}

	ClearSelBrushes();
	UpdateAllViewports();
	NewMessage("Select vertex to snap to. Right click when finished.");
	do
	{
		CheckMoveKeys();
		UpdateMap();
		UpdateMouse();
//		ClearKeys();
	} while (mouse.button!=2);

   while (mouse.button==2) UpdateMouse();

	if (M.display.num_vselected != 1)
	{
		if (numv > 1)
		{
			Q_free(vhandle);			// Free handle vertex
		}
		ClearSelBrushes();
		M.display.num_vselected = numv;	// Put original selection back
		M.display.vsel = vlist;
		M.display.num_bselected = numb;
		M.display.bsel = blist;
		NewMessage("Snap To Vertex: No snap to vertex selected. Aborted.");
		UpdateAllViewports();
		return;
	}

	// Compute amount to add to selected verts to snap them
	diff.x = M.display.vsel->vert->x - vhandle->vert->x;
	diff.y = M.display.vsel->vert->y - vhandle->vert->y;
	diff.z = M.display.vsel->vert->z - vhandle->vert->z;

	ClearSelBrushes();
	M.display.num_bselected = numb;
   M.display.bsel = blist;

   SUndo(UNDO_NONE,UNDO_CHANGE);

	// Modify originally selected vertices by the computed amount
	for (v=vlist; v; v=v->Next)
	{
		v->vert->x = v->vert->x + diff.x;
		v->vert->y = v->vert->y + diff.y;
		v->vert->z = v->vert->z + diff.z;
	}

	// Perform cleanup
	//
	if (numv > 1)
	{
		Q_free(vhandle);	// Free handle vertex selected earlier
	}

/*	// Clear original selected brushes and verts
	for (v=vlist; v; v=vlist) {
		vlist = v->Next;
		Q_free(v);
	}
	for (b=blist; b; b=blist) {
		CalcBrushCenter(b->Brush);	// Recalc brush centers
		if (!CheckBrush(b->Brush, FALSE)) {
			HandleError("SnapToVertex", "Non-coplanar vertices or non-convex brushes created.");
		}
		blist = b->Next;
		Q_free(b);
	}
	// Since some verts changed, some normals probably did too
	for(bh=BrushHead; bh; bh=bh->Next) {
		RecalcNormals(bh);
	}*/

	// Leave originally selected brushes selected so user can see the changes
	//
	M.display.vsel = vlist;			// Remember selected verts
	M.display.num_vselected = numv;
	M.display.bsel = blist;			// Remember selected brushes
	M.display.num_bselected = numb;
	for (b=blist; b; b=blist)
	{
		CalcBrushCenter(b->Brush);	// Recalc brush centers
		RecalcNormals(b->Brush);	// Recalc normals
		if (!CheckBrush(b->Brush, FALSE))
		{
			HandleError("SnapToVertex", "Non-coplanar vertices or non-convex brushes created.");
		}
		blist = b->Next;
	}
}

