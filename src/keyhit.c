/*
keyhit.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "keyhit.h"

#include "brush.h"
#include "button.h"
#include "cfgs.h"
#include "entity.h"
#include "keyboard.h"
#include "memory.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "undo.h"
#include "video.h"

void MapInfo(void)
{
   QUI_window_t *w;
   int b_ok;
   int b;
	unsigned char *temp_buf;
   int models;
   entity_t *e;

   int last_team;
   int last_target;

   int i;

   brush_t *br;

   int y;

/*   {
      int i,j;
      int c;

      printf("--- mapinfo ---\n");
      c=0;
      for (b=BrushHead;b;b=b->Next)
      {
         printf("-- brush %i\n",c++);

         for (i=0;i<b->num_verts;i++)
         {
            printf("%5i: %f %f %f\n",
               i,
               floor(b->verts[i].x+0.5),
               floor(b->verts[i].y+0.5),
               floor(b->verts[i].z+0.5));
         }
      }
   }*/

   last_target=GetLastTarget();
   last_team=-1;
   models=0;

   M.num_entities=0;
   for (e=M.EntityHead;e;e=e->Next)
   {
      M.num_entities++;
      if (!GetKeyValue(e,"origin"))
         models++;
      if (GetKeyValue(e,"team"))
      {
         if (sscanf(GetKeyValue(e,"team"),"team%i",&i)==1)
         {
            if (i>last_team)
               last_team=i;
         }
      }
   }
   models--;

   M.num_brushes=0;
   for (br=M.BrushHead;br;br=br->Next)
      M.num_brushes++;

	w=&Q.window[POP_WINDOW_1+Q.num_popups];
	w->size.x=350;
	w->size.y=240;
	w->pos.x=(video.ScreenWidth-w->size.x)/2;
	w->pos.y=(video.ScreenHeight-w->size.y)/2;

   PushButtons();
	b_ok=AddButtonText(0,0,B_ENTER|B_ESCAPE,"OK");
	MoveButton(b_ok,w->pos.x+(w->size.x-button[b_ok].sx)/2,
	           w->pos.y+w->size.y-30);

	QUI_PopUpWindow(POP_WINDOW_1+Q.num_popups,"Map info",&temp_buf);
	Q.num_popups++;

   y=w->pos.y+30;

   QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
      "Map name: %s",M.mapfilename[0]?M.mapfilename:"(unnamed)");
   y+=20;

   QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
      "Brushes: %i",M.num_brushes);
   y+=20;

   QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
      "Entities: %i",M.num_entities);
   y+=20;

   QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
      "Models: %i",models);
   y+=20;

   if (last_target==-1)
      QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
         "No targets!");
   else
      QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
         "Last target: t%i",last_target);
   y+=20;

   if (last_team==-1)
      QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
         "No teams!");
   else
      QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
         "Last team: team%i",last_team);
   y+=20;

   QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
      "Internal size: %0.2fkb",(float)MapSize()/1024.0);
   y+=20;

   QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
      "Memory used: %0.2fkb",(float)memused/1024.0);
   y+=20;
   QUI_DrawStr(w->pos.x+5,y,6,15,0,FALSE,
      "Memory peak: %0.2fkb",(float)maxused/1024.0);

	DrawButtons();

	RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);

	while (1)
	{
		/* Check for left-click */
      UpdateMouse();
      b=UpdateButtons();
      if (b==b_ok)
         break;
	}

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1+Q.num_popups,&temp_buf);

	RemoveButton(b_ok);
   PopButtons();

	RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);
}

void CheckKeyHit(void)
{
   CheckCfg(MODE_MOVE);
   CheckCfg(MODE_COMMON);

   switch (status.edit_mode)
   {
      case BRUSH:
         CheckCfg(MODE_BRUSH);
         break;
      case FACE:
         CheckCfg(MODE_FACE);
         break;
      case ENTITY:
         CheckCfg(MODE_ENTITY);
         break;
      case MODEL:
         CheckCfg(MODE_MODEL);
         break;
   }
}

void CheckMoveKeys(void)
{
   CheckCfg(MODE_MOVE);
}

