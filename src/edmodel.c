/*
edmodel.c file of the Quest Source Code

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

#include "edmodel.h"

#include "3d.h"
#include "brush.h"
#include "color.h"
#include "edbrush.h"
#include "edent.h"
#include "edentity.h"
#include "edit.h"
#include "edvert.h"
#include "entity.h"
#include "error.h"
#include "display.h"
#include "keyboard.h"
#include "keyhit.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "popupwin.h"
#include "entclass.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "undo.h"
#include "video.h"


void HandleLeftClickModel(void)
{
	entity_t    *sel_ent;
	entityref_t *e1;
	brush_t     *sel_brush;
	brushref_t  *b;

	int          base_x, base_y;
	int          found;

	/* Control for Brushes */
	if (TestKey(KEY_CONTROL))
	{
		sel_brush = FindBrush(mouse.x, mouse.y);
		if (sel_brush != NULL)
		{
			/* Check if we're adding or removing */
			found = FALSE;
			for (b=M.display.bsel; b; b=b->Next)
			{
				if (b->Brush == sel_brush)
				{
					/* Removing */
					found = TRUE;
					if (b->Last!=NULL) (b->Last)->Next = b->Next;
					if (b->Next!=NULL) (b->Next)->Last = b->Last;
					if (b==M.display.bsel) M.display.bsel=b->Next;

					Q_free(b);
					M.display.num_bselected--;
					break;
				}
			}
			if (!found)
			{
				/* Adding */
				if (M.display.bsel!=NULL)
				{
					b = (brushref_t *)Q_malloc(sizeof(brushref_t));
					if (b==NULL)
					{
						HandleError("Select Brush","Could not allocate select list");
						return;
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
						return;
					}
					M.display.bsel->Brush = sel_brush;
					M.display.bsel->Last = NULL;
					M.display.bsel->Next = NULL;
					M.display.num_bselected=1;
				}
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
		sel_ent = FindEntity(mouse.x, mouse.y);
		if (sel_ent != NULL)
		{
			/* Check if we're adding or removing */
			found = FALSE;
			for (e1=M.display.esel; e1; e1=e1->Next)
			{
				if (e1->Entity == sel_ent)
				{
					/* Removing */
					found = TRUE;
					if (e1->Last != NULL)   e1->Last->Next = e1->Next;
					if (e1->Next != NULL)   e1->Next->Last = e1->Last;
					if (e1 == M.display.esel) M.display.esel = e1->Next;

					Q_free(e1);
					M.display.num_eselected--;
					break;
				}
			}
			if (!found)
			{
				/* Adding */
				M.cur_entity = sel_ent;
				QUI_RedrawWindow(STATUS_WINDOW);
				if (M.display.esel != NULL)
				{
					e1 = (entityref_t *)Q_malloc(sizeof(entityref_t));
					e1->Entity=sel_ent;
					e1->Last = NULL;
					e1->Next = M.display.esel;
					M.display.esel->Last=e1;
					M.display.esel = e1;
				}
				else
				{
					M.display.esel = (entityref_t *)Q_malloc(sizeof(entityref_t));
					M.display.esel->Entity=sel_ent;
					M.display.esel->Last = NULL;
					M.display.esel->Next = NULL;
				}
				M.display.num_eselected++;
			}
			while (mouse.button == 1)
				GetMousePos();
			UpdateAllViewports();
		}
		else
		{
			base_x = mouse.x;
			base_y = mouse.y;
         SelectWindow();
			/* Add all vertecies in window to selection list */
			if ((mouse.x == base_x) && (mouse.y == base_y))
				ClearSelEnts();
			else
				SelectWindowEntity(base_x, base_y, mouse.x, mouse.y);
			UpdateAllViewports();
		}
	}
}

void CreateModel(void)
{
   char        classname[256];
   entity_t   *e;
	brushref_t *br;

	if (M.display.num_bselected == 0)
	{
		HandleError("CreateModel", "At least one brush must be selected.");
		return;
	}
   EntityPicker(classname,CLASS_MODEL);
   if (!classname[0]) return;

   e = (entity_t *)Q_malloc(sizeof(entity_t));
   if (!e)
   {
   	HandleError("CreateModel", "Unable to malloc() entity.");
   	return;
   }
   memset(e,0,sizeof(entity_t));
   InitEntity(e);

   if (M.EntityHead != NULL)
   	M.EntityHead->Last=e;
   e->Next = M.EntityHead;
   e->Last = NULL;
   M.EntityHead = e;
   M.num_entities++;

   SUndo(UNDO_NONE,UNDO_CHANGE);
   AddDEntity(e);
   
   SetKeyValue(e, "classname", classname);

   SetEntityDefaults(e);
   
   /* Assign the brushes to the entity */
   for (br=M.display.bsel; br; br=br->Next)
   {
   	br->Brush->EntityRef = M.EntityHead;
   }
   
   ClearSelBrushes();
   UpdateAllViewports();

   return;
}

int CreateLink(void)
{
	entityref_t *e;

	entityref_t *tlist_e;
	int          num_tlist_e;

	char         targetname[256];

	if (!M.display.num_eselected)
	{
		QUI_Dialog("Error", "No items to link selected.");
		return FALSE;
	}

	/* Copy all selected to temp lists */
	tlist_e = M.display.esel;
	num_tlist_e = M.display.num_eselected;

	M.display.esel = NULL;
	M.display.num_eselected = 0;

	UpdateAllViewports();

	NewMessage("Select the targets. Right click when finished.");
	do
	{
		CheckMoveKeys();
		UpdateMap();
		UpdateMouse();
//		ClearKeys();
	} while (mouse.button != 2);

   while (mouse.button) UpdateMouse();

	if (!M.display.num_eselected)
	{
		HandleError("CreateLink", "No targets selected.");
		M.display.esel = tlist_e;
		M.display.num_eselected = num_tlist_e;
		ClearSelEnts();
		return FALSE;
	}

	/* Check if they're trying to link from worldspawn */
	for (e=tlist_e; e; e=e->Next)
	{
		if (e->Entity==M.WorldSpawn)
		{
			NewMessage("Warning! Linking from worldspawn!");
		}
	}

	/* Check if they're trying to link to worldspawn */
	for (e=M.display.esel; e; e=e->Next)
	{
		if (e->Entity==M.WorldSpawn)
		{
			NewMessage("Warning! Linking to worldspawn!");
		}
	}

   targetname[0]=0;
   for (e=tlist_e;e;e=e->Next)
   {
      if (GetKeyValue(e->Entity,"target"))
      {
         if (targetname[0])
         {
            if (strcmp(targetname,GetKeyValue(e->Entity,"target")))
            {
               HandleError("CreateLink","Multiple targets.");
               ClearSelEnts();
         		M.display.esel = tlist_e;
         		M.display.num_eselected = num_tlist_e;
         		ClearSelEnts();
         		return FALSE;
            }
         }
         else
         {
            strcpy(targetname,GetKeyValue(e->Entity,"target"));
         }
      }
   }
   for (e=M.display.esel;e;e=e->Next)
   {
      if (GetKeyValue(e->Entity,"targetname"))
      {
         if (targetname[0])
         {
            if (strcmp(targetname,GetKeyValue(e->Entity,"targetname")))
            {
               HandleError("CreateLink","Multiple targetnames.");
               ClearSelEnts();
         		M.display.esel = tlist_e;
         		M.display.num_eselected = num_tlist_e;
         		ClearSelEnts();
         		return FALSE;
            }
         }
         else
         {
            strcpy(targetname,GetKeyValue(e->Entity,"targetname"));
         }
      }
   }

   if (!targetname[0])
   {
      sprintf(targetname,"t%i",GetFreeTarget());
   }

	for (e=tlist_e;e;e=e->Next)
		SetKeyValue(e->Entity,"target",targetname);
	for (e=M.display.esel;e;e=e->Next)
		SetKeyValue(e->Entity,"targetname",targetname);

	NewMessage("Link Created.");

	ClearSelEnts();
	M.display.esel = tlist_e;
	M.display.num_eselected = num_tlist_e;
	ClearSelEnts();

	UpdateAllViewports();

	return TRUE;
}

int EditModel(void)
{
   entity_t   **ents;
   int          nents;
   entityref_t *e;

   nents=0;
   ents=NULL;

   for (e=M.display.esel;e;e=e->Next)
   {
      nents++;
      ents=Q_realloc(ents,sizeof(entity_t *)*nents);
      ents[nents-1]=e->Entity;
   }

   if (!nents) return FALSE;
   EditAnyEntity(nents,ents);

   Q_free(ents);

   return TRUE;
}

void DrawAllModels(int vport)
{
	brush_t   *b;
	entity_t  *e;
   int        col;

	for (b=M.BrushHead; b; b=b->Next)
	{
		e = b->EntityRef;

		if ((b->Group->flags & 0x02) != 0)
			continue;

		if (e == M.WorldSpawn)
			continue;

      col=COL_PINK;
      if (GetKeyValue(e,"classname"))
      {
         if (FindClass(GetKeyValue(e,"classname")))
         {
            col=FindClass(GetKeyValue(e,"classname"))->color;
         }
         else
            col=COL_PINK;
      }
      if (col)
         DrawBrush(vport, b, GetColor(col), FALSE);
	}
}


static void DrawLinkLine(int vport,
                  vec3_t from,svec_t sfrom,
                  vec3_t to  ,svec_t sto,int col)
{
	switch (M.display.vport[vport].mode)
	{
		case NOPERSP:
         DrawArrow2D(vport,sfrom.x,sfrom.y,sto.x,sto.y,col);
         break;
		case WIREFRAME:
         DrawArrow3D(vport,from.x,from.y,from.z,to.x,to.y,to.z,col);
         break;
   }
}

void DrawAllLinks(int vport)
{
	entity_t  *e;
	entity_t  *escan;
	char       target[256];

#define ScanTN(col) \
   { \
      /* Scan for targetnames */ \
      for (escan=M.EntityHead;escan;escan=escan->Next) \
      { \
         if (!escan->drawn) \
            continue; \
   		if (GetKeyValue(escan,"targetname")) \
   		{ \
   			if (!strcmp(target,GetKeyValue(escan,"targetname"))) \
   			{ \
               DrawLinkLine(vport, \
                            e->trans,e->strans, \
                            escan->trans,escan->strans,col); \
   			} \
   		} \
   	} \
   }

	for (e=M.EntityHead;e;e=e->Next)
	{
      if (!e->drawn)
         continue;

		if (GetKeyValue(e,"target"))
		{
			strcpy(target,GetKeyValue(e,"target"));

         ScanTN(GetColor(COL_CYAN));
		}

		if (GetKeyValue(e,"pathtarget"))
		{
			strcpy(target,GetKeyValue(e,"pathtarget"));

         ScanTN(GetColor(COL_PINK));
		}

		if (GetKeyValue(e,"killtarget"))
		{
			strcpy(target,GetKeyValue(e,"killtarget"));

         ScanTN(GetColor(COL_YELLOW));
		}
	}
#undef ScanTN
}

void AddToModel(void)
{
	brushref_t  *b;
	brushref_t  *br;
	int          br_num;

	entity_t    *e;

	if (!M.display.num_bselected)
	{
		HandleError("AddToModel", "At least one brush must be selected.");
		return;
	}

   if (M.display.num_eselected!=1)
   {
		HandleError("AddToModel", "Exactly one model specified.");
		return;
	}

	br = M.display.bsel;
	br_num = M.display.num_bselected;

   e=M.display.esel->Entity;

	for (b=br;b;b=b->Next)
	{
		b->Brush->EntityRef = e;
	}

	ClearSelBrushes();
//	ClearSelEnts();

	UpdateAllViewports();
}

