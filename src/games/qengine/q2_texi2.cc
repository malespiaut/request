#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "q2_texi2.h"

#include "edit.h"
#include "message.h"
#include "quest.h"
}


#include "gui/gui.h"


static int s_and,s_or;
static int c_and,c_or;
static int f_value,f_valid;

static void GetValues(texdef_t *t)
{
   int flags,contents,value;

   flags   =t->g.q2.flags;
   contents=t->g.q2.contents;
   value   =t->g.q2.value;

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

static void SetValues(texdef_t *t)
{
   t->g.q2.flags&=s_and;
   t->g.q2.flags|=s_or;

   t->g.q2.contents&=c_and;
   t->g.q2.contents|=c_or;

   if (f_valid!=-1)
   {
      t->g.q2.value=f_value;
   }
}


void Quake2Flags_ModifyFlags(const char *contents_str[32],const char *flags_str[32])
{
   gui_win *win;
   gui_button *b_ok,*b_cancel;
   gui_label *lbl;
   gui_list *lcont,*lsurf;
   gui_checkbox *cont[32],*surf[32];
   gui_textbox *tb_value;

   event_t ev;

   unsigned int i,j,bf;

   char v_str[128];
   char *v_s;


   if (!M.display.num_bselected && !M.display.num_fselected)
   {
      NewMessage("Nothing selected!");
      return;
   }

   s_and=c_and=-1;
   s_or=c_or=0;
   f_valid=f_value=0;
   ForEachSelTexdef(GetValues);


   win=new gui_win(-1,-1,600,400);
   win->Init("Modify flags");

   lcont=new gui_list(4,48,-1,-1,win);
   for (i=0,bf=1;i<32;i++,bf<<=1)
   {
      const char *tmp;

      if (c_and&bf)
         j=cbvOn;
      else
      if (!(c_or&bf))
         j=cbvOff;
      else
         j=cbvUndef;

      if (contents_str[i])
         tmp=contents_str[i];
      else
      {
         sprintf(v_str,"%08x",bf);
         tmp=v_str;
      }

      cont[i]=new gui_checkbox(-1,-1,-1,-1,lcont);
      cont[i]->Init(j,cbfCanUndef,tmp);
   }
   lcont->Init(2);

   lbl=new gui_label(-1,-1,-1,-1,win);
   lbl->Init(txtLeft,"~Contents flags:",lcont);


   lsurf=new gui_list(lcont->x2+4,lcont->y1,-1,-1,win);
   for (i=0,bf=1;i<32;i++,bf<<=1)
   {
      const char *tmp;

      if (s_and&bf)
         j=cbvOn;
      else
      if (!(s_or&bf))
         j=cbvOff;
      else
         j=cbvUndef;

      if (flags_str[i])
         tmp=flags_str[i];
      else
      {
         sprintf(v_str,"%08x",bf);
         tmp=v_str;
      }

      surf[i]=new gui_checkbox(-1,-1,-1,-1,lsurf);
      surf[i]->Init(j,cbfCanUndef,tmp);
   }
   lsurf->Init(2);

   lbl=new gui_label(-1,-1,-1,-1,win);
   lbl->Init(txtLeft,"~Surface flags:",lsurf);


   if (f_valid==1)
   {
      sprintf(v_str,"%i",f_value);
   }
   else
   {
      v_str[0]='?';
      v_str[1]=0;
   }

   tb_value=new gui_textbox(4,lcont->y2+24,win->sx-8,-1,win);
   tb_value->Init(127,v_str);

   lbl=new gui_label(-1,-1,-1,-1,win);
   lbl->Init(txtLeft,"~Value:",tb_value);


   b_ok=new gui_button(4,win->sy-28,-1,-1,win);
   b_ok->Init(btnDefault,"OK");

   b_cancel=new gui_button(b_ok->x2+4,b_ok->y1,-1,-1,win);
   b_cancel->Init(btnCancel,"Cancel");


   win->InitPost();
   win->ReDraw();

   do
   {
      win->Run(&ev);

      if ((ev.what==evCommand) && (ev.cmd==cmdbPressed))
         break;
   } while (1);

   if (ev.control==b_ok)
   {
      c_and=s_and=0;
      c_or=s_or=0;

      for (i=0,bf=1;i<32;i++,bf<<=1)
      {
         if (cont[i]->GetValue()==cbvOn)
         {
            c_or|=bf;
            c_and|=bf;
         }
         else
         if (cont[i]->GetValue()!=cbvOff)
         {
            c_and|=bf;
         }

         if (surf[i]->GetValue()==cbvOn)
         {
            s_or|=bf;
            s_and|=bf;
         }
         else
         if (surf[i]->GetValue()!=cbvOff)
         {
            s_and|=bf;
         }
      }

      f_value=strtol(tb_value->GetText(),&v_s,0);
      if (*v_s)
      {
         f_valid=-1;
      }
      else
      {
         f_valid=1;
      }

      ForEachSelTexdef(SetValues);
   }

   win->UnDraw();
   win->Refresh();
   delete win;
}

