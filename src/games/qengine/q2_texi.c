#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "q2_texi.h"

#include "q2_tex.h"
#include "quake2.h"

#include "button.h"
#include "file.h"
#include "message.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "tex.h"
#include "video.h"


#if 0
typedef struct
{
   char *name;
   int value;
} flag_t;

#define CFlag(x) {#x,CONTENTS_##x}
static flag_t contents_flags[]=
{
CFlag(SOLID),
CFlag(WINDOW),
CFlag(AUX),
CFlag(LAVA),
CFlag(SLIME),
CFlag(WATER),
CFlag(MIST),
CFlag(AREAPORTAL),
CFlag(PLAYERCLIP),
CFlag(MONSTERCLIP),
CFlag(CURRENT_0),
CFlag(CURRENT_90),
CFlag(CURRENT_180),
CFlag(CURRENT_270),
CFlag(CURRENT_UP),
CFlag(CURRENT_DOWN),
CFlag(ORIGIN),
CFlag(DETAIL),
CFlag(TRANSLUCENT),
CFlag(LADDER),
};
#define NUM_CONTENTS (sizeof(contents_flags)/sizeof(contents_flags[0]))

#define SFlag(x) {#x,SURF_##x}
static flag_t surf_flags[]=
{
SFlag(LIGHT),
SFlag(SLICK),
SFlag(SKY),
SFlag(WARP),
SFlag(TRANS33),
SFlag(TRANS66),
SFlag(FLOWING),
SFlag(NODRAW),
SFlag(HINT),
SFlag(SKIP)
};
#define NUM_SURF (sizeof(surf_flags)/sizeof(surf_flags[0]))

static int s_and,s_or;
static int c_and,c_or;
static int f_value,f_valid;

static void GetValues(plane_t *p)
{
   int flags,contents,value;

   flags   =p->tex.g.q2.flags;
   contents=p->tex.g.q2.contents;
   value   =p->tex.g.q2.value;

   s_and&=flags;
   s_or |=flags;

   c_and&=contents;
   c_or |=contents;

   if (!f_valid)
   {
      f_value=value;
      f_valid=1;
   }
   else
   if (f_valid==1)
   {
      if (f_value!=value)
         f_valid=-1;
   }
}

static void SetValues(plane_t *p)
{
   p->tex.g.q2.flags&=s_and;
   p->tex.g.q2.flags|=s_or;

   p->tex.g.q2.contents&=c_and;
   p->tex.g.q2.contents|=c_or;

   if (f_valid!=-1)
   {
      p->tex.g.q2.value=f_value;
   }
}

static void ForEachFace(void (*func)(plane_t *p))
{
   brushref_t *b;
   fsel_t *f;
   int i;

   for (b=M.display.bsel;b;b=b->Next)
   {
      for (i=0;i<b->Brush->num_planes;i++)
      {
         func(&b->Brush->plane[i]);
      }
   }
   for (f=M.display.fsel;f;f=f->Next)
   {
      func(&f->Brush->plane[f->facenum]);
   }
}

void Q2_ModifyFlags(void)
{
   QUI_window_t *w;
   unsigned char *temp_buf;

   int b_ok,b_cancel;
   int b_contents[NUM_CONTENTS];
   int b_surf[NUM_SURF];
   int bp;

   char c_flags[NUM_CONTENTS];
   char s_flags[NUM_SURF];
   char temp[2];

   int i;
   int x,y;

   char v_str[128];
   char *v_s;
   int v_x1,v_x2,v_y1,v_y2;

   if (!M.display.num_bselected && !M.display.num_fselected)
   {
      NewMessage("Nothing selected!");
      return;
   }

   s_and=c_and=-1;
   s_or=c_or=0;
   f_valid=f_value=0;
   ForEachFace(GetValues);
   for (i=0;i<NUM_CONTENTS;i++)
   {
      if (c_and&contents_flags[i].value)
         c_flags[i]='1';
      else
      if (!(c_or&contents_flags[i].value))
         c_flags[i]='0';
      else
         c_flags[i]='?';
   }
   for (i=0;i<NUM_SURF;i++)
   {
      if (s_and&surf_flags[i].value)
         s_flags[i]='1';
      else
      if (!(s_or&surf_flags[i].value))
         s_flags[i]='0';
      else
         s_flags[i]='?';
   }

   if (f_valid==1)
   {
      sprintf(v_str,"%i",f_value);
   }
   else
   {
      v_str[0]='?';
      v_str[1]=0;
   }

	w=&Q.window[POP_WINDOW_1+Q.num_popups];
	w->size.x=600;
	w->size.y=400;
	w->pos.x=(video.ScreenWidth-w->size.x)/2;
	w->pos.y=(video.ScreenHeight-w->size.y)/2;
	QUI_PopUpWindow(POP_WINDOW_1+Q.num_popups,"Surface flags",&temp_buf);
	Q.num_popups++;

   PushButtons();
   b_ok=AddButtonText(0,0,0,"OK");
   b_cancel=AddButtonText(0,0,0,"Cancel");
   MoveButton(b_ok,
              w->pos.x+4,
              w->pos.y+w->size.y-button[b_ok].sy-4);
   MoveButton(b_cancel,
              w->pos.x+4+button[b_ok].sx+4,
              button[b_ok].y);

   x=w->pos.x+4;
   y=w->pos.y+50;
   QUI_DrawStr(x,y-20,BG_COLOR,15,0,FALSE,"Contents:");

   temp[1]=0;
   for (i=0;i<NUM_CONTENTS;i++)
   {
      temp[0]=c_flags[i];
      b_contents[i]=AddButtonText(x,y,0,temp);

      button[b_contents[i]].sx=25; // make all buttons the same size
      button[b_contents[i]].sy=24;

      QUI_DrawStr(
         x+26,
         y+12-ROM_CHAR_HEIGHT/2,
         BG_COLOR,0,0,FALSE,
         "%s",contents_flags[i].name);

      y+=button[b_contents[i]].sy;

      if (y>w->pos.y+w->size.y-64)
      {
         y=w->pos.y+50;
         x+=140;
      }
   }

   x+=180;
   y=w->pos.y+50;
   QUI_DrawStr(x,y-20,BG_COLOR,15,0,FALSE,"Flags:");
   for (i=0;i<NUM_SURF;i++)
   {
      temp[0]=s_flags[i];
      b_surf[i]=AddButtonText(x,y,0,temp);

      button[b_surf[i]].sx=25; // make all buttons the same size
      button[b_surf[i]].sy=24;

      QUI_DrawStr(
         x+26,
         y+12-ROM_CHAR_HEIGHT/2,
         BG_COLOR,0,0,FALSE,
         "%s",surf_flags[i].name);

      y+=button[b_surf[i]].sy;

      if (y>w->pos.y+w->size.y-64)
      {
         y=w->pos.y+50;
         x+=140;
      }
   }

   v_x1=w->pos.x+16;
   v_x2=w->pos.x+w->size.x-16;
   v_y1=w->pos.y+w->size.y-button[b_ok].sx-ROM_CHAR_HEIGHT-8;
   v_y2=v_y1+ROM_CHAR_HEIGHT+4;

   Frame(v_x1,v_y1,v_x2,v_y2);

   QUI_DrawStr(v_x1+4,v_y1+4,BG_COLOR,0,0,FALSE,
               "%s",v_str);

   DrawButtons();
   RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);

   do
   {
      UpdateMouse();
      bp=UpdateButtons();

      if ((bp==-1) && (mouse.button==1))
      {
			if (InBox(v_x1,v_y1,v_x2,v_y2))
			{
				if (!readtname(v_str,v_x1+4,v_y1+4,30))
            {
					QUI_DrawStr(v_x1+4,v_y1+4,BG_COLOR,0,0,FALSE,v_str);
               RefreshPart(v_x1,v_y1,v_x2,v_y2);
					DrawMouse(mouse.x,mouse.y);
				}
			}
      }

      if (bp==b_ok)
         break;
      if (bp==b_cancel)
         break;

      for (i=0;i<NUM_CONTENTS;i++)
      {
         if (bp==b_contents[i])
         {
            switch (c_flags[i])
            {
            case '0':
               c_flags[i]='1';
               break;
            case '1':
               c_flags[i]='?';
               break;
            default:
               c_flags[i]='0';
               break;
            }
            temp[0]=c_flags[i];
            temp[1]=0;
            x=button[b_contents[i]].x;
            y=button[b_contents[i]].y;
            RemoveButton(b_contents[i]);
            b_contents[i]=AddButtonText(x,y,0,temp);
            button[b_contents[i]].sx=25; // make all buttons the same size
            button[b_contents[i]].sy=24;

            DrawButtons();
            RefreshButton(b_contents[i]);
         }
      }

      for (i=0;i<NUM_SURF;i++)
      {
         if (bp==b_surf[i])
         {
            switch (s_flags[i])
            {
            case '0':
               s_flags[i]='1';
               break;
            case '1':
               s_flags[i]='?';
               break;
            default:
               s_flags[i]='0';
               break;
            }
            temp[0]=s_flags[i];
            temp[1]=0;
            x=button[b_surf[i]].x;
            y=button[b_surf[i]].y;
            RemoveButton(b_surf[i]);
            b_surf[i]=AddButtonText(x,y,0,temp);
            button[b_surf[i]].sx=25; // make all buttons the same size
            button[b_surf[i]].sy=24;

            DrawButtons();
            RefreshButton(b_surf[i]);
         }
      }
   } while (1);

   if (bp==b_ok)
   {
      c_and=s_and=0;
      c_or=s_or=0;
      for (i=0;i<NUM_CONTENTS;i++)
      {
         if (c_flags[i]=='1')
         {
            c_or|=contents_flags[i].value;
            c_and|=contents_flags[i].value;
         }
         else
         if (c_flags[i]!='0')
         {
            c_and|=contents_flags[i].value;
         }
      }
      for (i=0;i<NUM_SURF;i++)
      {
         if (s_flags[i]=='1')
         {
            s_or|=surf_flags[i].value;
            s_and|=surf_flags[i].value;
         }
         else
         if (s_flags[i]!='0')
         {
            s_and|=surf_flags[i].value;
         }
      }
      f_value=strtol(v_str,&v_s,0);
      if (*v_s)
      {
         f_valid=-1;
      }
      else
      {
         f_valid=1;
      }
      ForEachFace(SetValues);
   }

   RemoveButton(b_ok);
   RemoveButton(b_cancel);
   for (i=0;i<NUM_CONTENTS;i++)
      RemoveButton(b_contents[i]);
   for (i=0;i<NUM_SURF;i++)
      RemoveButton(b_surf[i]);
   PopButtons();

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1+Q.num_popups,&temp_buf);

	RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);
}
#endif


void Q2_FlagsDefault(texdef_t *tex)
{
   texture_t *t;

   t=ReadMIPTex(tex->name,0);
   if (!t)
      tex->g.q2.contents=tex->g.q2.flags=tex->g.q2.value=0;
   else
   {
      tex->g.q2.contents=t->g.q2.contents;
      tex->g.q2.flags   =t->g.q2.flags;
      tex->g.q2.value   =t->g.q2.value;
   }
}

