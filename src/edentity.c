/*
edentity.c file of the Quest Source Code

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

#include "edentity.h"

#include "3d.h"
#include "brush.h"
#include "button.h"
#include "color.h"
#include "edent.h"
#include "entity.h"
#include "error.h"
#include "file.h"
#include "keyboard.h"
#include "list.h"
#include "memory.h"
#include "message.h"
#include "mouse.h"
#include "popup.h"
#include "popupwin.h"
#include "entclass.h"
#include "quest.h"
#include "qui.h"
#include "tex.h"
#include "texpick.h"
#include "video.h"


static int EditCol(char *buf,int col_max,int col_scale)
{
   QUI_window_t *w;

   unsigned char *TempBuf;

   int bp;
   int b_ok,b_cancel,b_tex;
   int b_inc[3];
   int b_dec[3];
   int b_scale;

   float col[3];
   float max;

   int i;
   int update;


   SetPal(PAL_QUEST);

   col[0]=col[1]=col[2]=0;
   sscanf(buf,"%f %f %f",&col[0],&col[1],&col[2]);

   if (col_scale)
   {
      max=col[0];
      for (i=1;i<3;i++)
         if (col[i]>max)
            max=col[i];
   
      if (max)
      {
         max=col_max/max;
         for (i=0;i<3;i++)
            col[i]*=max;
      }
   }

	w = &Q.window[POP_WINDOW_1+Q.num_popups];

	w->size.x = 200;
	w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);

	w->size.y = 192;
	w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

	QUI_PopUpWindow(POP_WINDOW_1+Q.num_popups,"Edit color",&TempBuf);
	Q.num_popups++;

   PushButtons();
   b_ok    =AddButtonText(0,0,B_ENTER,"OK");
   b_cancel=AddButtonText(0,0,B_ESCAPE,"Cancel");
   b_tex   =AddButtonText(0,0,0,"Texture");
   b_scale =AddButtonText(0,0,0,"Scale");

   button[b_ok].x=w->pos.x+4;
   button[b_cancel].x=button[b_ok].x+button[b_ok].sx+4;
   button[b_tex].x=button[b_cancel].x+button[b_cancel].sx+4;

   button[b_ok].y=button[b_cancel].y=button[b_tex].y=
      w->pos.y + w->size.y - button[b_ok].sy - 4;

   button[b_scale].x=button[b_ok].x;
   button[b_scale].y=button[b_ok].y-button[b_scale].sy-4;

   for (i=0;i<3;i++)
   {
      b_inc[i]=AddButtonPic(w->pos.x+8,w->pos.y+32+32*i,
         B_RAPID,"button_tiny_up");
      b_dec[i]=AddButtonPic(w->pos.x+8,w->pos.y+32+32*i+16,
         B_RAPID,"button_tiny_down");
   }

   DrawButtons();

   for (i=0;i<32;i++)
      DrawLine(w->pos.x+128   ,w->pos.y+48+i,
               w->pos.x+128+32,w->pos.y+48+i,255);

	RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);

   update=1;
   while (1)
   {
      if (update)
      {
         for (i=0;i<3;i++)
         {
            col[i]=(int)(col[i]);
            if (col[i]<0) col[i]=0;
            if (col[i]>col_max) col[i]=col_max;

            QUI_DrawStr(w->pos.x+24,w->pos.y+32+32*i+4,BG_COLOR,0,0,0,
               "%4.0f  ",col[i]);
         }

         for (i=0;i<3;i++)
            video.pal[255*3+i]=col[i]*(64.0/(float)col_max);
         SetGammaPal(video.pal);

         RefreshPart(w->pos.x,w->pos.y,
                     w->pos.x+w->size.x,w->pos.y+w->size.y);
         update=0;
      }

      UpdateMouse();
      bp=UpdateButtons();

      if (bp==b_ok)
         break;

      if (bp==b_cancel)
         break;

      if (bp==b_tex)
      {
         char texture[128];
         texture_t *tex;

         TexturePicker(NULL,texture);
         if (texture[0])
         {

            tex=ReadMIPTex(texture,1);
            if (tex)
            {
               GetTexColor(tex);

               col[0]=tex->colv[0]*col_max;
               col[1]=tex->colv[1]*col_max;
               col[2]=tex->colv[2]*col_max;
               update=1;
            }
         }
      }

      if (bp==b_scale)
      {
         max=col[0];
         for (i=1;i<3;i++)
            if (col[i]>max)
               max=col[i];
      
         if (max)
         {
            max=col_max/max;
            for (i=0;i<3;i++)
               col[i]*=max;
         }
         update=1;
      }

      for (i=0;i<3;i++)
      {
         if (bp==b_inc[i])
         {
            col[i]+=1;
            update=1;
         }
         if (bp==b_dec[i])
         {
            col[i]-=1;
            update=1;
         }
      }
   }

	/* Pop down the window (also copies back whats behind the window) */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1+Q.num_popups,&TempBuf);

	/* Get rid of the button */
	RemoveButton(b_ok);
	RemoveButton(b_cancel);
   RemoveButton(b_tex);
   RemoveButton(b_scale);
   for (i=0;i<3;i++)
   {
      RemoveButton(b_inc[i]);
      RemoveButton(b_dec[i]);
   }
   PopButtons();

	/* refresh the correct portion of the screen */
	RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);        
	DrawMouse(mouse.x,mouse.y);

   if (bp==b_ok)
   {
      if (col_scale)
      {
         max=col[0];
         for (i=1;i<3;i++)
            if (col[i]>max)
               max=col[i];
   
         if (max)
         {
            max=col_max/max;
            for (i=0;i<3;i++)
               col[i]*=max;
         }
      }

      sprintf(buf,"%i %i %i",(int)col[0],(int)col[1],(int)col[2]);
      return 1;
   }
   else
      return 0;
}


static int PickCL(char *buf)
{
   EntityPicker(buf,CLASS_MODEL|CLASS_POINT);
   if (buf[0])
      return 1;
   else
      return 0;
}


static int GetChoice(char *buf,Q_key_t *k)
{
   int i;

   Popup_Init(mouse.x,mouse.y);
   for (i=0;i<k->x.ch.num_choices;i++)
      Popup_AddStr("%s\t%s",k->x.ch.choices[i].name,k->x.ch.choices[i].val);
   i=Popup_Display();
   Popup_Free();
   if (i!=-1)
   {
      strcpy(buf,k->x.ch.choices[i].val);
      return 1;
   }
   return 0;
}


static int GetFromList(char *buf,char *list)
{
   char temp[256];

   if (PickFromList(temp,list))
   {
      strcpy(buf,temp);
      return 1;
   }
   else
      return 0;
}



static void EditKey(entity_t *e,char *key,const char *v)
{
   class_t *c;
   int i;
   Q_key_t *k;
   char temp[256];

   strcpy(temp,v);
   if (!strcmp(key,"classname"))
   {
      if (PickCL(temp))
         SetKeyValue(e,key,temp);
      return;
   }

   c=FindClass(GetKeyValue(e,"classname"));
   if (!c)
   {
      HandleError("EditKey","Unknown key!");
      return;
   }

   for (i=0;i<c->num_keys;i++)
      if (!strcmp(c->keys[i].name,key))
         break;

   if (i==c->num_keys)
   {
      HandleError("EditKey","Unknown key!");
      return;
   }

   k=&c->keys[i];
   switch (k->type)
   {
   case KEY_HL_LIGHT:
      {
         float r,g,b,a;
         int i;

         i=sscanf(temp,"%f %f %f %f",&r,&g,&b,&a);
         if (i==1)
         {
            a=r;
            g=b=r=255;
         }
         else
         if (i==3)
         {
            if (g>r)
               a=g;
            else
               a=r;
            if (b>a)
               a=b;

            r*=255/a;
            g*=255/a;
            b*=255/a;
         }
         else
         if (i!=4)
         {
            r=g=b=255;
            a=300;
         }

         sprintf(temp,"%g %g %g",r,g,b);
         if (EditCol(temp,255,0))
         {
            sscanf(temp,"%f %f %f",&r,&g,&b);
            sprintf(temp,"%g %g %g %g",r,g,b,a);
            SetKeyValue(e,key,temp);
         }
      }
      break;
   case KEY_HL_COLOR:
      if (EditCol(temp,255,0))
         SetKeyValue(e,key,temp);
      break;

   case KEY_Q2_COLOR:
      if (EditCol(temp,100,1))
         SetKeyValue(e,key,temp);
      break;

   case KEY_CHOICE:
      if (GetChoice(temp,k))
         SetKeyValue(e,key,temp);
      break;

   case KEY_LIST:
      if (GetFromList(temp,k->x.list.name))
         SetKeyValue(e,key,temp);
      break;

   case KEY_TEXTURE:
      {
         char temp2[256];

         TexturePicker(temp,temp2);
         if (temp2[0])
            SetKeyValue(e,key,temp2);
      }
      break;
   }
}

static int CanEdit(entity_t *e,char *key)
{
   class_t *c;
   int i;

   if (!strcmp(key,"classname"))
      return 1;

   c=FindClass(GetKeyValue(e,"classname"));
   if (!c)
      return 0;

   for (i=0;i<c->num_keys;i++)
      if (!strcmp(c->keys[i].name,key))
         return 1;

   return 0;
}


#define NUM_DISPLAYED_KEYS 8
static void PopEntity(entity_t *e)
{
   QUI_window_t *w;

   unsigned char *TempBuf;

   int bp;
   int b_done;
   int b_delete;
   int b_clear;
   int b_edit;

   int b_up,b_dn;
   int b_c_up,b_c_dn;

   int k_x1,k_y1;
   int k_x2,k_y2;
   int nk_y1,nk_y2;
   int v_x;

   int s_x1,s_x2,s_x3,s_x4;
   int s_y1,s_y2;

   int c_x1,c_y1,c_x2,c_y2;

   int key;
   int y;
   int num_keys;            // including 'phantom' keys

#define MAX_PH_KEYS 32      // more should never be needed
   int pkeys[MAX_PH_KEYS];

   char n_key[128];
   char n_value[256];

   int i;

   int spawnflags;

   char temp[128];

   class_t *eclass;

   int c_y;
   int c_can;


/* Note that this will work even when eclass=NULL, since i will always
   be <e->numkeys in that case */
#define GKEY(i) (i<e->numkeys?e->key[i]:eclass->keys[pkeys[i-e->numkeys]].name)
#define GVALUE(i) (i<e->numkeys?e->value[i]:"")


static void UpdateStuff(void)
{
   int i;
   class_t *cl;

   cl=FindClass(GetKeyValue(e,"classname"));
   if (cl!=eclass)
   {
      eclass=cl;
      c_y=0;
   }

   num_keys=0;
   if (eclass)
      for (i=0;i<eclass->num_keys;i++)
         if (!GetKeyValue(e,eclass->keys[i].name))
         {
            if (num_keys<MAX_PH_KEYS) pkeys[num_keys++]=i;
         }
   num_keys+=e->numkeys;
}


static void DrawKeys(void)
{
   int i,j;
   const char *name;
   int px,py;


   if (GetKeyValue(e,"spawnflags"))
      spawnflags=atoi(GetKeyValue(e,"spawnflags"));
   else
      spawnflags=0;

   for (i=c_y1;i<c_y2;i++)
      DrawLine(c_x1,i,c_x2,i,BG_COLOR);
   QUI_Frame(c_x1-2,c_y1-2,c_x2+2,c_y2+2);
   if (eclass)
      c_can=DrawClassInfo(c_x1,c_y1,c_x2-c_x1,c_y2-c_y1,eclass->name,c_y);
   else
      c_can=0;

   for (i=s_y1;i<s_y2;i++)
      DrawLine(s_x1,i,s_x4,i,BG_COLOR);
   QUI_Frame(s_x1-2,s_y1-2,s_x4+2,s_y2+2);

   for (i=0;i<12;i++)
   {
      if (i>=8)
         px=s_x3;
      else
      if (i>=4)
         px=s_x2;
      else
         px=s_x1;
      py=(i&3)*ROM_CHAR_HEIGHT+s_y1;

      if (eclass && i<eclass->num_flags)
         name=eclass->flags[i];
      else
         name="x";

      if (spawnflags&(1<<i))
         QUI_DrawStr(px,py,BG_COLOR,15,0,0,"1");
      else
         QUI_DrawStr(px,py,BG_COLOR,15,0,0,"0");

      QUI_DrawStr(px+16,py,BG_COLOR,15,0,0,"%s",name);
   }

   for (i=k_y1;i<nk_y2;i++)
      DrawLine(k_x1,i,k_x2,i,BG_COLOR);
   QUI_Frame(k_x1-2,k_y1-2,k_x2+2,k_y2+2);


   for (i=0;i<NUM_DISPLAYED_KEYS;i++)
   {
      if (i+y>=num_keys)
         break;

      if (i+y<e->numkeys)
      {
         if (i+y==key)
            j=15;
         else
         {
            if (CanEdit(e,e->key[i+y]))
               j=11;
            else
               j=0;
         }
   
         QUI_DrawStr(k_x1,k_y1+i*ROM_CHAR_HEIGHT,BG_COLOR,j,0,0,
            "%s",e->key[i+y]);
         QUI_DrawStr(v_x ,k_y1+i*ROM_CHAR_HEIGHT,BG_COLOR,j,0,0,
            "%s",e->value[i+y]);
      }
      else
      {
         if (i+y==key)
            j=COL_RED;
         else
            j=COL_RED-7;

/*         printf("i=%i y=%i num_keys=%i e->numkeys=%i %p\n",
            i,y,num_keys,e->numkeys,eclass->keys[i+y-e->numkeys].name);
         printf("%s\n",eclass->keys[i+y-e->numkeys].name);*/
         QUI_DrawStr(k_x1,k_y1+i*ROM_CHAR_HEIGHT,BG_COLOR,j,0,0,
            "%s",eclass->keys[pkeys[i+y-e->numkeys]].name);
      }
   }

   QUI_Frame(k_x1-2,nk_y1-2,v_x -2,nk_y2+2);
   QUI_Frame(v_x +2,nk_y1-2,k_x2+2,nk_y2+2);

   QUI_DrawStr(k_x1,nk_y1,BG_COLOR,14,0,0,
      "%s",n_key);
   QUI_DrawStr(v_x+4,nk_y1,BG_COLOR,14,0,0,
      "%s",n_value);

	RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);
}


   PushButtons();
   b_done  =AddButtonText(0,0,B_ENTER|B_ESCAPE,"Done");
   b_delete=AddButtonText(0,0,0,"Delete key");
   b_clear =AddButtonText(0,0,0,"Clear");
   b_edit  =AddButtonText(0,0,0,"Edit");

   b_up=AddButtonPic(0,0,B_RAPID,"button_tiny_up");
   b_dn=AddButtonPic(0,0,B_RAPID,"button_tiny_down");

   b_c_up=AddButtonPic(0,0,B_RAPID,"button_tiny_up");
   b_c_dn=AddButtonPic(0,0,B_RAPID,"button_tiny_down");

	w = &Q.window[POP_WINDOW_1+Q.num_popups];

	w->size.x = 600;
	w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);

	w->size.y  = 408;
	w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

   button[b_done].x=w->pos.x+4;
   button[b_delete].x=button[b_done].x+button[b_done].sx+4;
   button[b_clear].x=button[b_delete].x+button[b_delete].sx+4;
   button[b_edit].x=button[b_clear].x+button[b_clear].sx+4;

   button[b_edit].y=button[b_clear].y=button[b_delete].y=button[b_done].y=
      w->pos.y + w->size.y - button[b_done].sy - 4;

	QUI_PopUpWindow(POP_WINDOW_1+Q.num_popups,"Edit entity",&TempBuf);
	Q.num_popups++;

   k_x1=w->pos.x+6;
   k_x2=w->pos.x+w->size.x-6-button[b_up].sx-2;
   k_y2=w->pos.y+w->size.y-button[b_done].sy-8-ROM_CHAR_HEIGHT-8;
   k_y1=k_y2-ROM_CHAR_HEIGHT*NUM_DISPLAYED_KEYS-2;
   nk_y1=k_y2+8;
   nk_y2=k_y2+8+ROM_CHAR_HEIGHT;
   v_x=k_x1+(k_x2-k_x1)*5/20;

   s_x1=k_x1;
   s_x4=k_x2;
   s_x2=s_x1+(s_x4-s_x1)*1/3;
   s_x3=s_x1+(s_x4-s_x1)*2/3;

   s_y2=k_y1-8;
   s_y1=s_y2-ROM_CHAR_HEIGHT*4;

   c_x1=k_x1;
   c_x2=k_x2;
   c_y1=w->pos.y+32;
   c_y2=s_y1-8;

   MoveButton(b_up,k_x2+4,k_y1-2);
   MoveButton(b_dn,k_x2+4,k_y2+2-button[b_dn].sy);

   MoveButton(b_c_up,k_x2+4,c_y1-2);
   MoveButton(b_c_dn,k_x2+4,c_y2+2-button[b_c_dn].sy);

   y=0;
   key=-1;
   n_key[0]=n_value[0]=0;
   c_y=0;

   eclass=FindClass(GetKeyValue(e,"classname"));
   UpdateStuff();

   DrawButtons();

   DrawKeys();


   do
   {
      UpdateMouse();
      bp=UpdateButtons();

      if (bp==b_done)
         break;

      if ((bp==b_delete) && (key!=-1) && (key<e->numkeys))
      {
         RemoveKeyValue(e,e->key[key]);
         UpdateStuff();
         if (key>=num_keys) key=num_keys-1;
         if (key!=-1)
         {
            strcpy(n_key,GKEY(key));
            strcpy(n_value,GVALUE(key));
         }
         DrawKeys();
      }

      if (bp==b_edit)
      {
         if (key!=-1)
         {
            EditKey(e,GKEY(key),GVALUE(key));
            UpdateStuff();
            DrawKeys();
         }
      }

      if (bp==b_clear)
      {
         n_key[0]=n_value[0]=0;
         DrawKeys();
      }

      if (bp==b_up)
      {
         if (y)
         {
            y--;
            DrawKeys();
         }
      }

      if (bp==b_dn)
      {
         if (y+NUM_DISPLAYED_KEYS<num_keys)
         {
            y++;
            DrawKeys();
         }
      }

      if ((bp==b_c_up) && (c_y))
      {
         c_y--;
         DrawKeys();
      }
      if ((bp==b_c_dn) && (c_can))
      {
         c_y++;
         DrawKeys();
      }

      if ((bp==-1) && (mouse.button==1))
      {
         if (InBox(k_x1,nk_y1,v_x-1,nk_y2))
         {
            if (readstring(n_key,k_x1,nk_y1,v_x-1,127,NULL))
            {
               n_value[0]=0;
               if (readstring(n_value,v_x+4,nk_y1,k_x2,255,NULL))
               {
                  if ((n_key[0]) && (n_value[0]))
                  {
                     SetKeyValue(e,n_key,n_value);
                     UpdateStuff();
                  }
               }
            }
            DrawKeys();
         }
         if (InBox(v_x+2,nk_y1,k_x2,nk_y2))
         {
            if (readstring(n_value,v_x+4,nk_y1,k_x2,255,NULL))
            {
               if ((n_key[0]) && (n_value[0]))
               {
                  SetKeyValue(e,n_key,n_value);
                  UpdateStuff();
               }
            }
            DrawKeys();
         }
         if (InBox(s_x1,s_y1,s_x4,s_y2-1))
         {
            i=(mouse.y-s_y1)/ROM_CHAR_HEIGHT;
            if (mouse.x>s_x3)
               i+=8;
            else
            if (mouse.x>s_x2)
               i+=4;

            spawnflags^=1<<i;

            if (spawnflags)
            {
               sprintf(temp,"%i",spawnflags);
               SetKeyValue(e,"spawnflags",temp);
            }
            else
            {
               RemoveKeyValue(e,"spawnflags");
            }
            UpdateStuff();
            DrawKeys();

            while (mouse.button==1)
               UpdateMouse();
         }
      }
      if (bp==-1)
      {
         if (InBox(k_x1,k_y1,k_x2,k_y2-1))
         {
            i=(mouse.y-k_y1)/ROM_CHAR_HEIGHT;
            i+=y;
            if ((i<0) || (i>=num_keys))
               i=-1;

            if ((mouse.button&3) && (i!=key))
            {
               key=i;
               if (key!=-1)
               {
                  strcpy(n_key,GKEY(key));
                  strcpy(n_value,GVALUE(key));
               }
               DrawKeys();
            }
            if (mouse.button==2)
            {
               if (key!=-1)
               {
                  EditKey(e,GKEY(key),GVALUE(key));
                  UpdateStuff();
                  DrawKeys();
               }
            }
         }
      }
   } while (1);

	/* Pop down the window (also copies back whats behind the window) */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1+Q.num_popups,&TempBuf);

	/* Get rid of the button */
	RemoveButton(b_done);
	RemoveButton(b_delete);
	RemoveButton(b_clear);
	RemoveButton(b_edit);
	RemoveButton(b_up);
	RemoveButton(b_dn);
	RemoveButton(b_c_up);
	RemoveButton(b_c_dn);
   PopButtons();

	/* refresh the correct portion of the screen */
	RefreshPart(w->pos.x,w->pos.y,w->pos.x+w->size.x,w->pos.y+w->size.y);        
	DrawMouse(mouse.x,mouse.y);
}
#undef NUM_DISPLAYED_KEYS


void DumpEntity(entity_t *e)
{
   int i;

   for (i=0;i<e->numkeys;i++)
   {
      printf("   \"%s\" \"%s\"\n",e->key[i],e->value[i]);
   }
}


void EditAnyEntity(int nents,entity_t **ents)
{
	int i,j;
   entity_t *Ent;
   entity_t *a;

   Ent=Q_malloc(sizeof(entity_t));
   memset(Ent,0,sizeof(entity_t));
   Ent->numkeys=0;

   for (i=0;i<nents;i++)
   {
/*      printf("entity %i:\n",i);
      DumpEntity(ents[i]);*/

      a=ents[i];
      for (j=0;j<a->numkeys;j++)
      {
         if (!GetKeyValue(Ent,a->key[j]))
         {
            if (i)
               SetKeyValue(Ent,a->key[j],"?");
            else
               SetKeyValue(Ent,a->key[j],a->value[j]);
         }
         else
         {
            if (strcmp(a->value[j],GetKeyValue(Ent,a->key[j])))
            {
               SetKeyValue(Ent,a->key[j],"?");
            }
         }
      }
      for (j=0;j<Ent->numkeys;j++)
      {
         if (!GetKeyValue(a,Ent->key[j]))
            SetKeyValue(Ent,Ent->key[j],"?");
      }
   }
/*   printf("total: %i\n",nents);
   printf("Combined:\n");
   DumpEntity(Ent);*/

   PopEntity(Ent);

/*   printf("After:\n");
   DumpEntity(Ent);*/

   for (i=0;i<Ent->numkeys;i++)
   {
      if (strcmp(Ent->value[i],"?"))
      {
         for (j=0;j<nents;j++)
         {
            SetKeyValue(ents[j],Ent->key[i],Ent->value[i]);
         }
      }
   }
   for (i=0;i<nents;i++)
   {
      a=ents[i];

      for (j=0;j<a->numkeys;j++)
      {
         if (!GetKeyValue(Ent,a->key[j]))
         {
            RemoveKeyValue(a,a->key[j]);
            j--;
         }
      }
   }

/*   for (i=0;i<nents;i++)
   {
      printf("entity %i:\n",i);
      DumpEntity(ents[i]);
   }*/

   for (i=0;i<Ent->numkeys;i++)
   {
      Q_free(Ent->key[i]);
      Q_free(Ent->value[i]);
   }
   Q_free(Ent->key);
   Q_free(Ent->value);
   Q_free(Ent);
}


/* TODO : move this to a seperate file, get rid of nested functions */
void ModelList(void)
#define MAX_MODELS 512
{
   entity_t *ents[MAX_MODELS];
   int brushes[MAX_MODELS];
   entity_t *e;
   int models;
   brush_t *b;

	QUI_window_t *w;

	unsigned char *temp_buf;

	int base_ent;
	int active_ent=-1;

	int i,j;

   int b_ok,b_edit,b_delete;
   int b_tiny_up,b_tiny_down;
   int bp;

static void ClearModelList(void)
{
   /* Clear text area */
   for (i=w->pos.y+51;i<w->pos.y+293;i++)
   {
      DrawLine(w->pos.x+11,i,w->pos.x+359,i,BG_COLOR);
   }
}

static void DrawModelList(void)
{
	/* Draw model list */
	for (i=base_ent,j=0;(j<17) && (i<models);i++,j++)
   {
		QUI_DrawStr(w->pos.x+15,w->pos.y+52+(j*14),
		            BG_COLOR,(i==active_ent)?14:0,
		            0,0,"%s",GetKeyValue(ents[i],"classname"));
		QUI_DrawStr(w->pos.x+300,w->pos.y+52+(j*14),
		            BG_COLOR,(i==active_ent)?14:0,
		            0,0,"%i",brushes[i]);
   }
}

static void ReadModelList(void)
{
   models=0;
   for (e=M.EntityHead;e;e=e->Next)
   {
      if (!GetKeyValue(e,"origin"))
      {
         ents[models]=e;
         brushes[models]=0;
         for (b=M.BrushHead;b;b=b->Next)
         {
            if (b->EntityRef==e)
               brushes[models]++;
         }
         models++;
         if (models==MAX_MODELS)
         {
            HandleError("ModelList","Too many models");
            return;
         }
      }
   }
}

   ReadModelList();

	/* Set up window Position */
	w=&Q.window[POP_WINDOW_1+Q.num_popups];

	w->size.x=400;
	w->size.y=350;
	w->pos.x=(video.ScreenWidth-w->size.x)/2;
	w->pos.y=(video.ScreenHeight-w->size.y)/2;

	/* Set up some buttons */
   PushButtons();
	b_ok    =AddButtonText(0,0,B_ENTER|B_ESCAPE,"OK");
	b_edit  =AddButtonText(0,0,0,"Edit");
	b_delete=AddButtonText(0,0,0,"Delete");

	MoveButton(b_ok,
	           w->pos.x+50,
              w->pos.y+w->size.y-30);
	MoveButton(b_edit,
	           button[b_ok].x+button[b_ok].sx+5,
	           w->pos.y+w->size.y-30);
	MoveButton(b_delete,
	           button[b_edit].x+button[b_edit].sx+5,
              w->pos.y+w->size.y-30);

	base_ent=0;

	/* Set up scrolling buttons */
	b_tiny_up  =AddButtonPic(0,0,B_RAPID,"button_tiny_up");
	b_tiny_down=AddButtonPic(0,0,B_RAPID,"button_tiny_down");
	MoveButton(b_tiny_up,
              w->pos.x+w->size.x-30,
              w->pos.y+52);
	MoveButton(b_tiny_down,
	           w->pos.x+w->size.x-30,
	           w->pos.y+w->size.y-70);

	/* Actually draw the window */
	QUI_PopUpWindow(POP_WINDOW_1+Q.num_popups,"Models",&temp_buf);
	Q.num_popups++;

	/* Box Model selection area & Titles */
	QUI_DrawStr(w->pos.x+15,w->pos.y+33,
	            BG_COLOR,14,0,0,
	            "classname");
	QUI_DrawStr(w->pos.x+300,w->pos.y+33,
	            BG_COLOR,14,0,0,
	            "Brushes");

   DrawLine(w->pos.x+10,w->pos.y+50,
	         w->pos.x+10,w->pos.y+294,
	         4);
	DrawLine(w->pos.x+10,w->pos.y+50,
	         w->pos.x+360,w->pos.y+50,
	         4);
	DrawLine(w->pos.x+10,w->pos.y+294,
	         w->pos.x+360,w->pos.y+294,
	         8);
	DrawLine(w->pos.x+360,
	         w->pos.y+50,w->pos.x+360,
	         w->pos.y+294,
	         8);

	/* Draw Buttons */
   DrawButtons();

   DrawModelList();

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

	while (1)
	{
      UpdateMouse();
      bp=UpdateButtons();

		if (bp!=-1)
		{
         if (bp==b_ok)
         {
				break;
			}

			if (bp==b_edit)
         {
				if (active_ent==-1)
            {
					HandleError("ModelList","No model selected");
            }
				else
				{
               EditAnyEntity(1,&ents[active_ent]);

               ClearModelList();
               DrawModelList();

               RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
               UpdateMouse();
               DrawMouse(mouse.x, mouse.y);
				}
			}

         if (bp==b_delete)
         {
				if (active_ent == -1)
            {
					HandleError("ModelList","No model selected");
            }
				else
				{
               e=ents[active_ent];

               /* Make sure the entity we're about to delete isn't selected,
                should probably be done in DeleteEntity() */
               ClearSelEnts();

               DeleteEntity(e);

               ReadModelList();
               ClearModelList();
               DrawModelList();

               RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);
               UpdateMouse();
               DrawMouse(mouse.x, mouse.y);
				}
			}

         if (bp==b_tiny_up)
         {
            if (base_ent > 0)
               base_ent--;

            ClearModelList();
            DrawModelList();

            RefreshPart(w->pos.x, w->pos.y,
                        w->pos.x+w->size.x, w->pos.y+w->size.y);
            UpdateMouse();
            DrawMouse(mouse.x, mouse.y);
			}

			if (bp==b_tiny_down)
         {
            if ((base_ent+17) < models)
               base_ent++;

            ClearModelList();
            DrawModelList();

            RefreshPart(w->pos.x, w->pos.y,
                        w->pos.x+w->size.x, w->pos.y+w->size.y);
            UpdateMouse();
            DrawMouse(mouse.x, mouse.y);
         }
      }
      else // No button pressed (b==-1)
      if (mouse.button&1)
      {
			/* Check for click in model area */
			if (InBox(w->pos.x + 10,  w->pos.y + 50,
			          w->pos.x + 360, w->pos.y + 294))
			{
				/* Find which model they clicked on */
				for (i=base_ent, j=0; (j<17) && (i<models); i++, j++)
				{
					if (InBox(w->pos.x + 10,  w->pos.y + 52 + j*14,
					          w->pos.x + 360, w->pos.y + 66 + j*14))
						active_ent = i;
				}

            DrawModelList();

				RefreshPart(w->pos.x, w->pos.y,
				            w->pos.x+w->size.x, w->pos.y+w->size.y);
			}
		}
	}

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

	RemoveButton(b_ok);
	RemoveButton(b_edit);
	RemoveButton(b_delete);
	RemoveButton(b_tiny_up);
	RemoveButton(b_tiny_down);
   PopButtons();

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

	UpdateAllViewports();
#undef MAX_MODELS
}

