/*
texpick.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "texpick.h"

#include "tex.h"
#include "tex_all.h"

#include "texcat.h"

#include "button.h"
#include "color.h"
#include "error.h"
#include "file.h"
#include "game.h"
#include "memory.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "texfull.h"
#include "video.h"

#include "times.h"


static char **mainlist;
static int nml;
/*
Textures in category 'All', ie. all textures in the cache or all tnames.
The pointers point directly to tnames or Cache.
*/

/*static int mlsort(const void *e1,const void *e2)
{
   char *n1,*n2;

   n1=*(char **)e1;
   n2=*(char **)e2;

   if (*n1==*n2)
   {
      int i;
      if (*n1=='+' || *n1=='-')
      {
         i=stricmp(&n1[2],&n2[2]);
         if (i) return i;
      }
   }

   if (strrchr(n1,'/'))
      n1=strrchr(n1,'/')+1;

   if (strrchr(n2,'/'))
      n2=strrchr(n2,'/')+1;

   return stricmp(n1,n2);
}*/

static int mlsort(const void *e1,const void *e2)
{
   return Game.tex.sort(*(char * const *)e1,*(char * const *)e2);
}

static int ReadMainList(int cache)
{
   int i;

   if (mainlist)
   {
      Q_free(mainlist);
      mainlist=NULL;
      nml=0;
   }

   if (!Game.tex.cache)
      cache=1;

   if (cache)
   {
      nml=M.num_textures;
      mainlist=Q_malloc(nml*sizeof(char *));
      if (!mainlist)
      {
         HandleError("TexturePicker","Out of memory!");
         return 0;
      }

      for (i=0;i<nml;i++)
      {
         mainlist[i]=M.Cache[i].name;
      }
   }
   else
   {
      GetTNames(&nml,&mainlist);
      if (!nml)
      {
         HandleError("TexturePicker","Out of memory!");
         return 0;
      }
   }

   qsort(mainlist,nml,sizeof(char *),mlsort);

   return 1;
}


static char **curlist;
static int ncl;
/*
Textures in current category, names are pointers to mainlist.
*/

#define START_TEXS_CAT -3

static int ReadCurList(int category)
{
   int i;

   if (curlist)
   {
      Q_free(curlist);
      curlist=NULL;
      ncl=0;
   }

   for (i=0;i<nml;i++)
   {
      if (category!=-2)
      {
         if (category<=START_TEXS_CAT)
         {
            if ((-GetTCat(mainlist[i])+START_TEXS_CAT)!=category)
               continue;
         }
         else
         {
            if (GetCategory(mainlist[i])!=category)
               continue;
         }
      }

      curlist=Q_realloc(curlist,sizeof(char *)*(ncl+1));
      if (!curlist)
      {
         HandleError("TexturePicker","Out of memory!");
         return 0;
      }
      curlist[ncl]=mainlist[i];
      ncl++;
   }

   return 1;
}


static int cx1,cx2;
static int dx1,dx2;
static int y1,y2;

static int rows;

static int cy,dy;
static int cat,tex;

static texture_t curtex;

static int onlycache;


typedef struct
{
   const char *name;
   int trans;    // internal category number
} cat_t;

static cat_t *clist;
static int ncat;


static const char *AllName="*All*";
static const char *UnassignedName="*Unassigned*";

static int clsort(const void *e1,const void *e2)
{
   return stricmp(((const cat_t *)e1)->name,((const cat_t *)e2)->name);
}

static int ReadCList(void)
{
   int i,j;

   if (clist)
   {
      Q_free(clist);
      clist=NULL;
      ncat=0;
   }

   ncat=n_categories+n_tcategories+2;
   clist=Q_malloc(sizeof(cat_t)*ncat);
   if (!clist)
   {
      HandleError("TexturePicker","Out of memory!");
      return 0;
   }

   clist[0].trans=-2;
   clist[0].name=AllName;

   clist[1].trans=-1;
   clist[1].name=UnassignedName;

   for (i=2,j=0;j<n_tcategories;j++,i++)
   {
      clist[i].name=tcategories[j].name;
      clist[i].trans=-j+START_TEXS_CAT;
   }

   for (j=0;j<n_categories;j++,i++)
   {
      clist[i].name=categories[j].name;
      clist[i].trans=j;
   }

   qsort(&clist[2+n_tcategories],n_categories,sizeof(cat_t),clsort);

   return 1;
}


static void DrawCList(void)
{
   int i,col;

   DrawSolidBox(cx1,y1,cx2+1,y2,GetColor2(BG_COLOR));
   QUI_Box(cx1,y1-1,cx2,y2+1,4,8);

   for (i=0;i<rows;i++)
   {
      if (i+cy>=ncat)
         break;

      if (i+cy==cat)
         col=15;
      else
         col=0;

      QUI_DrawStrM(cx1+2,y1+i*ROM_CHAR_HEIGHT,cx2-2,
         BG_COLOR,col,0,0,"%s",clist[i+cy].name);
   }
}

static void DrawDList(QUI_window_t *w)
{
   int i,col;
   texture_t *t;
   char tdesc[128];

   DrawSolidBox(dx1,y1,dx2+1,y2,GetColor2(BG_COLOR));
   QUI_Box(dx1,y1-1,dx2,y2+1,4,8);

   for (i=0;i<rows;i++)
   {
      if (i+dy>=ncl)
         break;

      if (i+dy==tex)
         col=15;
      else
      {
         col=0;
         if (!onlycache)
            if (FindTexture(curlist[i+dy])!=-1)
               col=11;
      }

      QUI_DrawStrM(dx1+2,y1+i*ROM_CHAR_HEIGHT,dx2,
         BG_COLOR,col,0,0,"%s",curlist[i+dy]);
   }

   if (curtex.dsx!=-1)
      DrawSolidBox(dx2+24,y1,dx2+24+curtex.dsx,y1+curtex.dsy,GetColor2(BG_COLOR));
   DrawSolidBox(dx2+24,y2-4-ROM_CHAR_HEIGHT*3,w->pos.x+w->size.x-2,y2-4,GetColor2(BG_COLOR));

   curtex.dsx=-1;
   if (tex>=0 && tex<ncl)
   {
      if (onlycache)
      {
         t=ReadMIPTex(curlist[tex],1);
         if (t)
            curtex=*t;
      }
      else
      {
         if (!LoadTexture(curlist[tex],&curtex))
            curtex.dsx=-1;
      }
      QUI_DrawStr(dx2+24,y2-4-ROM_CHAR_HEIGHT*3,BG_COLOR,0,0,0,
         "%s",curlist[tex]);
         
      if (curtex.dsx!=-1)
      {
         DrawTexture(&curtex,dx2+24,y1);
         QUI_DrawStr(dx2+24,y2-4-ROM_CHAR_HEIGHT*2,BG_COLOR,0,0,0,
            "%3i x %3i",
            curtex.rsx,curtex.rsy);

         GetTexDesc(tdesc,&curtex);
         QUI_DrawStr(dx2+24,y2-4-ROM_CHAR_HEIGHT,BG_COLOR,0,0,0,
            "%s",tdesc);
      }
   }

   RefreshPart(dx2+24,y1,w->pos.x+w->size.x,w->pos.y+w->size.y);
}


static void Full(void)
{
   int nt;
   texture_t *t;
   int i;
   char res[128];

   nt=ncl;
   t=Q_malloc(sizeof(texture_t)*nt);
   if (!t)
   {
      HandleError("TexturePicker","Out of memory!");
      return;
   }
   memset(t,0,sizeof(texture_t)*nt);

   if (onlycache || !Game.tex.cache)
   {
      for (i=0;i<nt;i++)
      {
         t[i]=*ReadMIPTex(curlist[i],0);
      }
      TexPickFull(res,nt,t);

      for (i=0;i<ncl;i++)
      {
         if (!stricmp(res,curlist[i]))
         {
            tex=i;
            dy=i-rows/2;
            if (dy+rows>=ncl) dy=ncl-rows;
            if (dy<0) dy=0;
         }
      }
   }
   else
   {
//      float st,et;

//      st=GetTime();
      for (i=0;i<nt;i++)
      {
         LoadTexture(curlist[i],&t[i]);
      }
//      et=GetTime();

      
      TexPickFull(NULL,nt,t);
   }

   Q_free(t);
}


static void NewCategory(void)
{
	QUI_window_t *w;

	unsigned char *temp_buf;
	char name[128];


   SetPal(PAL_QUEST);

	w = &Q.window[POP_WINDOW_1 + Q.num_popups];
	w->size.x = 250;
	w->size.y = 80;
	w->pos.x = (video.ScreenWidth - w->size.x) / 2;
	w->pos.y = (video.ScreenHeight - w->size.y) / 2;

	/* Actually draw the window */
	QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Category name", &temp_buf);
	Q.num_popups++;

   QUI_Frame(w->pos.x+20,w->pos.y+40,w->pos.x+230,w->pos.y+62);

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

	name[0] = 0;
	if (readgname(name, w->pos.x + 25, w->pos.y + 44, w->pos.x+225, 20))
   {
      CreateCategory(name);
   }

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

   SetPal(PAL_TEXTURE);
}

static void DeleteCategory(void)
{
   SetPal(PAL_QUEST);

   if (clist[cat].trans<0)
   {
      HandleError("DeleteCategory","That category can't be deleted!");
      SetPal(PAL_TEXTURE);
      return;
   }

   if (!QUI_YesNo("DeleteCategory",
                  "Are you sure you want to delete this category?",
                  "Yes","No"))
   {
      SetPal(PAL_TEXTURE);
      return;
   }

   RemoveCategory(clist[cat].trans);
   cat--;

   SetPal(PAL_TEXTURE);
}


void TexturePicker(const char *StartWithThis, char *EndWithThis)
{
   QUI_window_t *w;
   unsigned char *TempBuf;

   int bp;
   int b_done,b_cancel,b_full;
   int b_cache,b_add;
   int c_new,c_delete;
   int cdn,cup;
   int ddn,dup,dscroll;
   int scr_y0,scr_y1,scr_d;

   int update,read;


   int i;
   int done;


   CheckCache(1);
   SetPal(PAL_TEXTURE);

   /* Position the window */
	w = &Q.window[POP_WINDOW_1 + Q.num_popups];

	w->size.x = video.ScreenWidth-10;
	w->size.y = video.ScreenHeight-10;

   if (w->size.x>700) w->size.x=700;
   if (w->size.y>500) w->size.y=500;

	w->pos.x = (video.ScreenWidth  / 2) - (w->size.x / 2);
	w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

	/* Draw the window */
	QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups,"Pick texture",&TempBuf);
	Q.num_popups++;


	PushButtons();
   b_done  =AddButtonText(0,0,B_ENTER,"Done");
   b_cancel=AddButtonText(0,0,B_ESCAPE,"Cancel");
   b_full  =AddButtonText(0,0,0,"Fullscreen");

   if (Game.tex.cache)
   {
      b_add   =AddButtonText(0,0,0,"Add");
      b_cache =AddButtonText(0,0,B_TOGGLE,"Cache");
   }
   else
   {
      b_add=b_cache=-1;
   }

   c_new   =AddButtonText(0,0,0,"New");
   c_delete=AddButtonText(0,0,0,"Delete");

   cdn=AddButtonPic(0,0,B_RAPID,"button_tiny_down");
   cup=AddButtonPic(0,0,B_RAPID,"button_tiny_up");
   ddn=AddButtonPic(0,0,B_RAPID,"button_tiny_down");
   dup=AddButtonPic(0,0,B_RAPID,"button_tiny_up");
	dscroll=AddButtonPic(0,0,B_RAPID,"button_tiny_scroll");


   button[b_done].x=button[c_new].x=w->pos.x+4;
   button[b_cancel].x=button[b_done  ].x+button[b_done  ].sx+4;
   button[b_full  ].x=button[b_cancel].x+button[b_cancel].sx+4;

   button[c_delete].x=button[c_new   ].x+button[c_new   ].sx+4;

   button[b_done].y=button[b_cancel].y=button[b_full].y=
      w->pos.y+w->size.y-button[b_done].sy-4;

   if (Game.tex.cache)
   {
      button[b_add   ].x=button[b_full  ].x+button[b_full  ].sx+4;
      button[b_cache ].x=button[b_add   ].x+button[b_add   ].sx+4;
      button[b_add].y=button[b_cache].y=button[b_done].y;
   }

   button[c_new].y=button[c_delete].y=
      button[b_done].y-button[c_new].sy-4;


   cx1=w->pos.x+4;
   cx2=cx1+128;

   dx1=cx2+button[cup].sx+4;
   dx2=dx1+160;

   y1=w->pos.y+28;
   y2=w->pos.y+w->size.y-button[b_done].sy*2-12;

   rows=y2-y1;
   rows/=ROM_CHAR_HEIGHT;
   y2=y1+rows*ROM_CHAR_HEIGHT;

   button[cup].x=button[cdn].x=cx2+2;
   button[dscroll].x=button[dup].x=button[ddn].x=dx2+2;

   button[cup].y=button[dup].y=y1-1;
   button[cdn].y=button[ddn].y=y2-button[cdn].sy+1;

   scr_y0=button[dscroll].y=button[dup].y+button[dup].sy+1;
   scr_y1=button[ddn].y-button[ddn].sy-1;
   scr_d=scr_y1-scr_y0;


   cat=0;
   cy=dy=0;
   tex=-1;
   onlycache=1;
   done=0;

   ReadCList();
   ReadMainList(onlycache);
   ReadCurList(clist[cat].trans);
   for (i=0;i<ncl;i++)
   {
      if (!stricmp(StartWithThis,curlist[i]))
      {
         tex=i;
         dy=i-rows/2;
         if (dy+rows>=ncl) dy=ncl-rows;
         if (dy<0) dy=0;
      }
   }

   curtex.dsx=-1;

   if (Game.tex.cache)
   {
      ToggleButton(b_cache,onlycache);
   }
   DrawButtons();
   RefreshScreen();
   read=0;
   update=7;

   do
   {
      if (update || read)
      {
         update|=read;

         if (read&2)
            ReadCList();
         if (read&4)
            ReadMainList(onlycache);
         if (read&1)
            ReadCurList(clist[cat].trans);
      
         if (update&1)
         {
            DrawDList(w);
            RefreshPart(dx1,y1-1,dx2,y2+1);

            EraseButton(dscroll);
            RefreshButton(dscroll);

            button[dscroll].y=scr_y0;
            if (ncl>rows)
            {
               button[dscroll].y+=scr_d*(dy/(float)(ncl-rows));
            }
            DrawButton(dscroll);
            RefreshButton(dscroll);
         }
         if (update&2)
         {
            DrawCList();
            RefreshPart(cx1,y1-1,cx2,y2+1);
         }

         read=update=0;
      	DrawMouse(mouse.x,mouse.y);
      }

      UpdateMouse();
      bp=UpdateButtons();

      if (bp!=-1)
      {
         if (bp==b_done)
         {
            if (tex>=0 && tex<ncl)
               done=1;
            else
               HandleError("TexPick","You must select a texture first!");
         }

         if (bp==b_cancel)
            done=2;

         if (bp==b_full)
         {
            Full();
            read=1;
         }

         if (bp==c_new)
         {
            NewCategory();
            read=2;
         }

         if (bp==c_delete)
         {
            DeleteCategory();
            read=3;
         }


         if (Game.tex.cache)
         {
            if (bp==b_cache)
            {
               onlycache=!onlycache;
               ToggleButton(b_cache,onlycache);
               RefreshButton(b_cache);
               read=5;
               dy=0;
               tex=-1;
            }
   
            if (bp==b_add)
            {
               if (tex!=-1)
                  ReadMIPTex(curlist[tex],1);
            }
         }


         if ((bp==cup) && cy)
         {
            cy--;
            update=2;
         }
         if ((bp==cdn) && (cy+rows<ncat))
         {
            cy++;
            update=2;
         }

         if ((bp==dup) && dy)
         {
            dy--;
            update=1;
         }
         if ((bp==ddn) && (dy+rows<ncl))
         {
            dy++;
            update=1;
         }

         if (bp==dscroll)
         {
            while (mouse.button&1)
            {
               UpdateMouse();

               if (ncl>rows && mouse.y>scr_y0+button[dscroll].sy/2)
               {
                  dy=(mouse.y-button[dscroll].sy/2-scr_y0)*
                     (ncl-rows)/(float)scr_d;
                  if (dy>ncl-rows) dy=ncl-rows;
               }
               else
                  dy=0;

               DrawDList(w);
               RefreshPart(dx1,y1-1,dx2,y2+1);
   
               EraseButton(dscroll);
               RefreshButton(dscroll);

               button[dscroll].y=scr_y0;
               if (ncl>rows)
               {
                  button[dscroll].y+=scr_d*(dy/(float)(ncl-rows));
               }
               DrawButton(dscroll);
               RefreshButton(dscroll);
            }
            update=1;
         }
      }
      else
      if (mouse.button&1)
      {
         if (InBox(cx1,y1,cx2,y2))
         {
            i=(mouse.y-y1)/ROM_CHAR_HEIGHT+cy;
            if ((i!=cat) && (i>=0) && (i<ncat))
            {
               cat=i;
               dy=0;
               tex=-1;
               read=1;
               update=2;
            }
         }

         if (InBox(dx1,y1,dx2,y2))
         {
            i=(mouse.y-y1)/ROM_CHAR_HEIGHT+dy;
            if ((i!=tex) && (i>=0) && (i<ncl))
            {
               update=1;
               tex=i;
            }
         }
      }
      else
      if (mouse.button&2)
      {
         if (InBox(dx1,y1,dx2,y2))
         {
            tex=(mouse.y-y1)/ROM_CHAR_HEIGHT+dy;
            if ((tex>=0) && (tex<ncl))
            {
               DrawDList(w);
               RefreshPart(dx1,y1,dx2,y2);

               while (mouse.button&2)
               {
                  UpdateMouse();
                  if (InBox(cx1,y1,cx2,y2))
                  {
                     i=(mouse.y-y1)/ROM_CHAR_HEIGHT+cy;
                     read=cat;
                     cat=i;
                     DrawCList();
                     cat=read;
                     RefreshPart(cx1,y1,cx2,y2);
                  }
               }

               if (InBox(cx1,y1,cx2,y2))
               {
                  i=(mouse.y-y1)/ROM_CHAR_HEIGHT+cy;
                  if ((i>=0) && (i<ncat))
                  {
                     i=clist[i].trans;
                     if (i<=START_TEXS_CAT)
                        HandleError("TexPick","Can't move texture to that category!");
                     else
                     if (i<0)
                        SetCategory(curlist[tex],-1);
                     else
                        SetCategory(curlist[tex],i);
                  }
               }
            }
            tex=-1;
            read=1;
            update=2;
         }
      }
   } while (!done);


   RemoveButton(b_done);
   RemoveButton(b_cancel);
   RemoveButton(b_full);

   if (Game.tex.cache)
   {
      RemoveButton(b_add);
      RemoveButton(b_cache);
   }

   RemoveButton(c_new);
   RemoveButton(c_delete);

   RemoveButton(cup);
   RemoveButton(cdn);
   RemoveButton(dup);
   RemoveButton(ddn);
   RemoveButton(dscroll);

   PopButtons();


   /* Pop down the window (also copies back whats behind the window) */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups,&TempBuf);

	RefreshScreen();


   *EndWithThis=0;
   if (done==1)
   {
      strcpy(EndWithThis,curlist[tex]);
      ReadMIPTex(curlist[tex],1);
   }

   SaveCategories();

   Q_free(mainlist);
   Q_free(curlist);
   Q_free(clist);
   mainlist=NULL;
   curlist=NULL;
   clist=NULL;
   nml=ncl=ncat=0;

   SetPal(PAL_QUEST);
}

