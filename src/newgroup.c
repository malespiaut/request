/*
newgroup.c file of the Quest Source Code

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

#include "newgroup.h"

#include "3d.h"
#include "brush.h"
#include "button.h"
#include "display.h"
#include "edbrush.h"
#include "error.h"
#include "file.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "video.h"


void CreateWorldGroup(void)
{
	M.WorldGroup = (group_t *)Q_malloc(sizeof(group_t));

	strcpy(M.WorldGroup->groupname,"World");
	M.WorldGroup->flags = 0x1;
	M.WorldGroup->color = COL_WHITE;

	M.GroupHead  = M.WorldGroup;
	M.num_groups = 1;

	M.GroupHead->Next=NULL;
	M.GroupHead->Last=NULL;

	M.CurGroup = M.GroupHead;
}


group_t *CreateGroup(char *groupname)
{
	group_t *g;

	for (g=M.GroupHead; g; g=g->Next)
	{
		if (strcmp(g->groupname, groupname) == 0)
		{
			return g;
		}
	}

	g = (group_t *)Q_malloc(sizeof(group_t));
	strcpy(g->groupname, groupname);
	g->flags = 0;
	g->color = COL_WHITE;

	g->Last = NULL;
	g->Next = M.GroupHead;
	M.GroupHead->Last = g;
	M.GroupHead = g;

	M.num_groups++;

	return g;
}


void RemoveGroup(char *groupname)
{
	group_t *g;
	int      found;


	found = FALSE;
	for (g=M.GroupHead; g; g=g->Next)
	{
		if (strcmp(g->groupname, groupname) == 0)
		{
			found = TRUE;

			if (g->Last != NULL)  g->Last->Next = g->Next;
			if (g->Next != NULL)  g->Next->Last = g->Last;
			if (g == M.GroupHead)   M.GroupHead = g->Next;

			Q_free(g);
			break;
		}
	}

	if (!found)
		HandleError("RemoveGroup", "Unable to find group");

}


group_t *FindVisGroup(group_t *first)
{
   group_t *g;

   if (first)
   {
      if (!(first->flags&0x02))
         return first;
   }

   for (g=M.GroupHead;g;g=g->Next)
   {
      if (!(g->flags&0x02))
         return g;
   }

   NewMessage("Can't find any visible group!");
   if (first)
      return first;

   return M.GroupHead;
}


static void BubbleSort(char list[200][50], int *ref, int num)
{
	int   i;
	char  temp[256];
	int   stemp;
	int   found;


	found = TRUE;
	while (found == TRUE)
	{
		found = FALSE;
		for (i=0; i<(num-1); i++)
		{
			if (strcmp(list[i], list[i+1]) > 0)
			{
				found = TRUE;
				strcpy(temp, list[i]);
				strcpy(list[i], list[i+1]);
				strcpy(list[i+1], temp);
				stemp = ref[i];
				ref[i] = ref[i+1];
				ref[i+1] = stemp;
			}
		}
	}
}


/* TODO : not use nested functions */
void GroupPopup(void)
{
	QUI_window_t *w;

	unsigned char *temp_buf;

	group_t      *newg;
	group_t      *g;
	brush_t      *b;
	brushref_t   *bsel;
	entityref_t  *esel;
   entity_t     *e;

	int     base_group;
	int     active_group = -1;

	char    new_group_name[50];
	char    group_list[200][50];
	int     group_ref[200];

	int     i, j, k;

	int     b_ok,b_create,b_add,b_remove,b_delete;
	int     b_tiny_up,b_tiny_down;
	int     b_hide_dot[20],b_smart_dot[20];
   int     bp;

void ReDraw(void)
{
   /* Clear text area */
   for (i=w->pos.y + 51; i<w->pos.y + 293; i++)
      DrawLine(w->pos.x + 71, i, w->pos.x + 359, i, BG_COLOR);

   /* Clear button area */
   for (i=w->pos.y + 51; i<w->pos.y + 293; i++)
      DrawLine(w->pos.x + 16, i, w->pos.x + 67, i, BG_COLOR);

   /* Draw group list */
   for (i=base_group, j=0; (j<17) && (i<M.num_groups); i++, j++)
      QUI_DrawStr(w->pos.x + 75, w->pos.y + 52 + (j*14),
                  BG_COLOR, (i==active_group) ? 14 : 0,
                  0,FALSE,group_list[i]);

   /* Toggle appropriate buttons and draw colors */
   for (i=base_group, j=0; (i<M.num_groups) && (j<17); i++, j++)
   {
      for (k=0, g=M.GroupHead; (k<group_ref[i]) && (g); k++, g=g->Next) ;

      if ((g->flags & 0x02) != 0)
         ToggleButton(b_hide_dot[j],1);
      else
         ToggleButton(b_hide_dot[j],0);

      if ((g->flags & 0x04) != 0)
         ToggleButton(b_smart_dot[j],1);
      else
         ToggleButton(b_smart_dot[j],0);

      DrawSolidSquare(w->pos.x + 17, w->pos.y + 54 + (j*14), 10, g->color);
   }

   DrawButtons();

   RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
   DrawMouse(mouse.x,mouse.y);
}

void ReadStruct(void)
{
   for (i=0;i<17;i++)
   {
      if (b_hide_dot[i]!=-1)
         RemoveButton(b_hide_dot[i]);
      if (b_smart_dot[i]!=-1)
         RemoveButton(b_smart_dot[i]);
   }

   /* Update popup structures */
   /* Allocate and sort group list */
   for (g=M.GroupHead, i=0; g; g=g->Next, i++)
   {
      strcpy(group_list[i], g->groupname);
      group_ref[i] = i;
   }
   BubbleSort(group_list, group_ref, M.num_groups);
   base_group = 0;

   if (M.num_groups>17)
      j=17;
   else
      j=M.num_groups;
   for (i=0;i<j;i++)
   {
		b_hide_dot[i]=AddButtonPic(0,0,B_TOGGLE,"button_hidden_toggle");
      if (b_hide_dot[i]==-1)
         HandleError("GroupPupup","Error creating button");
		MoveButton(b_hide_dot[i],w->pos.x+50,w->pos.y+52+(i*14));

		b_smart_dot[i]=AddButtonPic(0,0,B_TOGGLE,"button_hidden_toggle");
      if (b_smart_dot[i]==-1)
         HandleError("GroupPupup","Error creating button");
		MoveButton(b_smart_dot[i],w->pos.x+32,w->pos.y+52+(i*14));
   }
   for (;i<17;i++)
   {
      b_hide_dot[i]=b_smart_dot[i]=-1;
   }
}

	/* Set up window Position */
	w = &Q.window[POP_WINDOW_1 + Q.num_popups];

	w->size.x = 400;
	w->size.y = 350;
	w->pos.x = (video.ScreenWidth - w->size.x) / 2;
	w->pos.y = (video.ScreenHeight - w->size.y) / 2;

	/* Set up some buttons */
   PushButtons();
	b_ok     = AddButtonText(0,0,B_ENTER|B_ESCAPE,"OK");
	b_create = AddButtonText(0,0,0,"Create");
	b_delete = AddButtonText(0,0,0,"Delete");
	b_add    = AddButtonText(0,0,0,"Add");
	b_remove = AddButtonText(0,0,0,"Remove");

	MoveButton(b_ok, w->pos.x + 50, w->pos.y + w->size.y - 30);
	MoveButton(b_create,button[b_ok].x    +button[b_ok].sx + 5, w->pos.y + w->size.y - 30);
	MoveButton(b_delete,button[b_create].x+button[b_create].sx + 5, w->pos.y + w->size.y - 30);
	MoveButton(b_add,   button[b_delete].x+button[b_delete].sx + 5, w->pos.y + w->size.y - 30);
	MoveButton(b_remove,button[b_add].x   +button[b_add].sx + 5, w->pos.y + w->size.y - 30);

   for (i=0;i<17;i++)
   {
      b_smart_dot[i]=b_hide_dot[i]=-1;
   }

   ReadStruct();

	/* Set up scrolling buttons */
	b_tiny_up    =AddButtonPic(0,0,B_RAPID,"button_tiny_up");
	b_tiny_down  =AddButtonPic(0,0,B_RAPID,"button_tiny_down");
	MoveButton(b_tiny_up,w->pos.x+w->size.x-30,w->pos.y+52);
	MoveButton(b_tiny_down,w->pos.x+w->size.x-30,w->pos.y+w->size.y-70);

   /* Actually draw the window */
	QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Grouping", &temp_buf);
	Q.num_popups++;

	/* Box Group selection area & Titles */
	QUI_DrawStr(w->pos.x + 75, w->pos.y + 33, BG_COLOR, 14,0,0,"Name");
	QUI_DrawStr(w->pos.x + 17, w->pos.y + 33, BG_COLOR, 14,0,0,"C");
	QUI_DrawStr(w->pos.x + 35, w->pos.y + 33, BG_COLOR, 14,0,0,"S");
	QUI_DrawStr(w->pos.x + 53, w->pos.y + 33, BG_COLOR, 14,0,0,"H");

   QUI_Frame(w->pos.x+70,w->pos.y+50,w->pos.x+360,w->pos.y+294);

   ReDraw();

	while (1)
	{
      UpdateMouse();
      bp=UpdateButtons();
		/* Check for left-click */
		if (bp!=-1)
		{
         if (bp==b_ok)
            break;

			if (bp==b_create)
         {
				if ((M.display.num_bselected==0) && (M.display.num_eselected==0))
            {
					HandleError("Grouping", "Nothing selected to group.");
            }
				else
				{
					GetGroupName(new_group_name);
					if (strlen(new_group_name) != 0)
					{
						/* Allocate new group */
						newg = (group_t *)Q_malloc(sizeof(group_t));
						newg->Last = NULL;
						newg->Next = M.GroupHead;
						M.GroupHead->Last = newg;
						M.GroupHead = newg;
						strcpy(newg->groupname, new_group_name);
						newg->color = 15;
						newg->flags = 0;
						M.num_groups++;

                  ReadStruct();
                  ReDraw();

						/* Apply new group to all selected brushes and entities */
						for (bsel=M.display.bsel; bsel; bsel=bsel->Next)
							bsel->Brush->Group = newg;
						for (esel=M.display.esel; esel; esel=esel->Next)
							esel->Entity->Group = newg;
					}
				}
			}

         if (bp==b_delete)
         {
				if (active_group == -1)
            {
					HandleError("Grouping", "No group selected.");
            }
				else
				if (strcmp(group_list[active_group], "World") == 0)
            {
					HandleError("Grouping", "Cannot delete World!");
            }
				else
				{
					/* Find group selected */
					for (k=0, g=M.GroupHead; (k<group_ref[active_group]) && (g); k++, g=g->Next);
					if (g != NULL)
					{
						/* Move all brushes into M.WorldGroup */
						for (b=M.BrushHead; b; b=b->Next)
						{
							if (b->Group == g)
								b->Group = M.WorldGroup;
						}
                  /* Entities too. */
						for (e=M.EntityHead; e; e=e->Next)
						{
							if (e->Group == g)
								e->Group = M.WorldGroup;
						}
						/* Actually remove group */
						if (g->Last != NULL)  g->Last->Next = g->Next;
						if (g->Next != NULL)  g->Next->Last = g->Last;
						if (g == M.GroupHead)   M.GroupHead = g->Next;
						Q_free(g);
						M.num_groups--;

                  ReadStruct();
                  ReDraw();
					}
				}
			}

         if (bp==b_add)
         {
				if (active_group == -1)
            {
					HandleError("Grouping", "No group selected.");
            }
				else
				{
					/* Find group selected */
					for (k=0, g=M.GroupHead; (k<group_ref[active_group]) && (g); k++, g=g->Next);
					if ((M.display.num_bselected == 0) &&
					    (M.display.num_eselected == 0))
               {
						HandleError("Grouping", "Nothing selected to add.");
               }
					else
					{
						for (bsel=M.display.bsel; bsel; bsel=bsel->Next)
							bsel->Brush->Group = g;
						for (esel=M.display.esel; esel; esel=esel->Next)
							esel->Entity->Group = g;
					}
				}
			}

         if (bp==b_remove)
         {
				if ((M.display.num_bselected == 0) && (M.display.num_eselected == 0))
            {
               HandleError("Grouping", "Nothing selected to remove.");
            }
				else
				{
					/* Move all selected brushes and entities into M.WorldGroup */
					for (bsel=M.display.bsel; bsel; bsel=bsel->Next)
						bsel->Brush->Group = M.WorldGroup;
					for (esel=M.display.esel; esel; esel=esel->Next)
						esel->Entity->Group = M.WorldGroup;
				}
			}

         if (bp==b_tiny_up)
         {
            if (base_group > 0)
               base_group--;
            ReDraw();
			}

         if (bp==b_tiny_down)
         {
				if ((base_group+17) < M.num_groups)
				   base_group++;
            ReDraw();
			}

         for (i=0;i<17;i++)
         {
            if (bp==b_hide_dot[i])
            {
               k=group_ref[i+base_group];
   				for (j=0,g=M.GroupHead;(j<k) && (g); j++, g=g->Next);
   				if (g!=NULL)
   				{
   					g->flags^=0x02;
   					ToggleButton(b_hide_dot[i],(button[b_hide_dot[i]].on)?0:1);
                  RefreshButton(b_hide_dot[i]);
   				}
            }
            if (bp==b_smart_dot[i])
            {
               k=group_ref[i+base_group];
   				for (j=0,g=M.GroupHead;(j<k) && (g); j++, g=g->Next);
   				if (g!=NULL)
   				{
   					g->flags^=0x04;
   					ToggleButton(b_smart_dot[i],(button[b_smart_dot[i]].on)?0:1);
                  RefreshButton(b_smart_dot[i]);
   				}
            }
         }
      }
      else // Didn't click on a button
      if (mouse.button&1)
      {
			/* Check for click in group area */
			if (InBox(w->pos.x + 70, w->pos.y + 50, w->pos.x + 360, w->pos.y + 294))
			{
				/* Find which group they clicked on */
				for (i=base_group, j=0; (j<17) && (i<M.num_groups); i++, j++)
				{
					if (InBox(w->pos.x + 70, w->pos.y + 52 + j*14, w->pos.x + 360, w->pos.y + 66 + j*14))
						active_group = i;
				}
				/* Draw group list */
				for (i=base_group, j=0; (j<17) && (i<M.num_groups); i++, j++)
					QUI_DrawStr(w->pos.x + 75, w->pos.y + 52 + (j*14),
					            BG_COLOR, (i==active_group) ? 14 : 0,
					            0,FALSE,group_list[i]);
				RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
			}

			/* Check for click in color area */
			if (InBox(w->pos.x + 16, w->pos.y + 52, w->pos.x + 30, w->pos.y + 294))
			{
				j = ((mouse.y - w->pos.y - 52) / 14) + base_group;
				k = group_ref[j];
				for (i=0, g=M.GroupHead; (i<k) && (g); i++, g=g->Next);
				if (g != NULL)
				{
					g->color = (g->color + 16) % 176;
					DrawSolidSquare(w->pos.x + 17, w->pos.y + 54 + ((j-base_group)*14), 10, g->color);
				}
				RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
				while (mouse.button == 1)
					UpdateMouse();
			}
		}
	}

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

	/* Clean up buttons */
	for (i=0; i<17; i++)
	{
		if (b_hide_dot[i]!=-1)
		   RemoveButton(b_hide_dot[i]);
		if (b_smart_dot[i]!=-1)
		   RemoveButton(b_smart_dot[i]);
	}
	RemoveButton(b_ok);
	RemoveButton(b_create);
	RemoveButton(b_delete);
	RemoveButton(b_add);
	RemoveButton(b_remove);
	RemoveButton(b_tiny_up);
	RemoveButton(b_tiny_down);
   PopButtons();

   {
      brushref_t *b,*bn;

      for (b=M.display.bsel;b;b=bn)
      {
         bn=b->Next;

         if (b->Brush->Group)
            if (b->Brush->Group->flags&0x02)
               RemoveSelBrush(b);
      }
   }

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

	UpdateAllViewports();
}

void GetGroupName(char *name)
{
	QUI_window_t *w;

	unsigned char *temp_buf;
	char    newname[50];


	w = &Q.window[POP_WINDOW_1 + Q.num_popups];
	w->size.x = 250;
	w->size.y = 80;
	w->pos.x = (video.ScreenWidth - w->size.x) / 2;
	w->pos.y = (video.ScreenHeight - w->size.y) / 2;

	/* Actually draw the window */
	QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "New Group Name", &temp_buf);
	Q.num_popups++;

   QUI_Frame(w->pos.x+20,w->pos.y+40,w->pos.x+230,w->pos.y+62);

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

	newname[0] = '\0';
	if (readgname(newname, w->pos.x + 25, w->pos.y + 44, w->pos.x+225, 20))
		strcpy(name, newname);
	else
		name[0] = '\0';

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
}


group_t *GroupPicker(void)
{
	QUI_window_t *w;

	unsigned char *temp_buf;

	group_t      *g;

	int     base_group;

	char    group_list[200][50];
	int     group_ref[200];

	int     i, j, k;

	int     b_ok,b_cancel;
	int     b_tiny_up,b_tiny_down;
   int     b;

	int     active_group = -1;


	/* Set up window Position */
	w = &Q.window[POP_WINDOW_1 + Q.num_popups];
	w->size.x = 350;
	w->size.y = 350;
	w->pos.x = (video.ScreenWidth - w->size.x) / 2;
	w->pos.y = (video.ScreenHeight - w->size.y) / 2;

	/* Set up some buttons */
   PushButtons();
	b_ok     = AddButtonText(0,0,B_ENTER,"OK");
	b_cancel = AddButtonText(0,0,B_ESCAPE,"Cancel");

	MoveButton(b_ok, w->pos.x + 120, w->pos.y + w->size.y - 30);
	MoveButton(b_cancel,button[b_ok].x+button[b_ok].sx + 5, w->pos.y + w->size.y - 30);

	/* Set up and sort group list */
	for (g=M.GroupHead, i=0; g; g=g->Next, i++)
	{
		strcpy(group_list[i], g->groupname);
		group_ref[i] = i;
	}
	BubbleSort(group_list, group_ref, M.num_groups);
	base_group = 0;

	/* Set up scrolling buttons */
	b_tiny_up     = AddButtonPic(0,0,0,"button_tiny_up");
	b_tiny_down   = AddButtonPic(0,0,0,"button_tiny_down");
	MoveButton(b_tiny_up, w->pos.x + w->size.x - 30, w->pos.y + 37);
	MoveButton(b_tiny_down, w->pos.x + w->size.x - 30, w->pos.y + w->size.y - 70);

	/* Actually draw the window */
	QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Select Group", &temp_buf);
	Q.num_popups++;

	/* Box Group selection area & Titles */
   QUI_Frame(w->pos.x+20,w->pos.y+35,w->pos.x+310,w->pos.y+294);

	/* Draw group list */
	for (i=base_group, j=0; (j<18) && (i<M.num_groups); i++, j++)
		QUI_DrawStr(w->pos.x + 25, w->pos.y + 37 + (j*14), BG_COLOR, (i==active_group) ? 14 : 0,0,FALSE,group_list[i]);

	/* Draw Buttons */
   DrawButtons();

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

	while (1)
	{
      b=UpdateButtons();
		if (b!=-1)
		{
         if (b==b_ok)
         {
				if (active_group == -1)
				{
					HandleError("Select Group", "No group selected.");
				}
				else
            {
					break;
            }
			}

         if (b==b_cancel)
         {
				break;
			}

         if (b==b_tiny_up)
         {
            if (base_group > 0)
               base_group--;
            /* Clear text area */
            for (i=w->pos.y + 36; i<w->pos.y + 293; i++)
               DrawLine(w->pos.x + 21, i, w->pos.x + 309, i, BG_COLOR);
            /* Draw group list */
            for (i=base_group, j=0; (j<18) && (i<M.num_groups); i++, j++)
               QUI_DrawStr(w->pos.x + 25, w->pos.y + 37 + (j*14), BG_COLOR, (i==active_group) ? 14 : 0,0, FALSE, group_list[i]);
            RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
            UpdateMouse();
            DrawMouse(mouse.x, mouse.y);
			}

         if (b==b_tiny_down)
         {
            if ((base_group+18) < M.num_groups)
            base_group++;
            /* Clear text area */
            for (i=w->pos.y + 36; i<w->pos.y + 293; i++)
               DrawLine(w->pos.x + 21, i, w->pos.x + 309, i, BG_COLOR);
            /* Draw group list */
            for (i=base_group, j=0; (j<18) && (i<M.num_groups); i++, j++)
               QUI_DrawStr(w->pos.x + 25, w->pos.y + 37 + (j*14), BG_COLOR, (i==active_group) ? 14 : 0,0, FALSE, group_list[i]);
            RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
            UpdateMouse();
            DrawMouse(mouse.x, mouse.y);
			}
      }
      else
      if (mouse.button&1)
      {
			/* Check for click in group area */
			if (InBox(w->pos.x + 22, w->pos.y + 37, w->pos.x + 310, w->pos.y + 294))
			{
				/* Find which group they clicked on */
				for (i=base_group, j=0; (j<18) && (i<M.num_groups); i++, j++)
				{
					if (InBox(w->pos.x + 22, w->pos.y + 37 + j*14, w->pos.x + 310, w->pos.y + 51 + j*14))
						active_group = i;
				}
				/* Draw group list */
				for (i=base_group, j=0; (j<18) && (i<M.num_groups); i++, j++)
					QUI_DrawStr(w->pos.x + 25, w->pos.y + 37 + (j*14), BG_COLOR, (i==active_group) ? 14 : 0,0, FALSE, group_list[i]);
				RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
			}

		}

		UpdateMouse();
	}

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

	RemoveButton(b_ok);
	RemoveButton(b_cancel);
	RemoveButton(b_tiny_up);
	RemoveButton(b_tiny_down);
   PopButtons();

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

   if (active_group==-1)
   {
      return NULL;
   }
   else
   {
	   for (k=0, g=M.GroupHead; (k<group_ref[active_group]) && (g); k++, g=g->Next);
	   return g;
   }
}

