#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "error.h"
#include "memory.h"
#include "qui.h"
#include "video.h"

int GUI_PopEntity(char *title,char **key,char **value,int num);
}


#include "gui.h"


int GUI_PopEntity(char *title,char **key,char **value,int num)
{
   int maxlen;
   gui_win *w;
   gui_button *b_ok,*b_cancel;
   gui_textbox **tb;
   gui_label *txt;
   int i,j;
   event_t e;

   maxlen=0;
   for (i=0;i<num;i++)
   {
      j=QUI_strlen(0,key[i]);
      if (j>maxlen) maxlen=j;
   }
   maxlen+=4;

   j=maxlen+256;
   if (j>video.ScreenWidth-32) j=video.ScreenWidth-32;

   w=new gui_win(-1,-1,j,num*22+32+32);
   w->Init(title);

   tb=(gui_textbox **)Q_malloc(sizeof(gui_textbox *)*num);
   if (!tb)
   {
      delete w;
      HandleError("GUI_PopEntity","Out of memory!");
      return 0;
   }

   j=w->sx-maxlen-16;
   for (i=0;i<num;i++)
   {
      tb[i]=new gui_textbox(maxlen+8,i*22+32,j,0,w);
      tb[i]->Init(63,value[i]);

      txt=new gui_label(maxlen+4,i*22+32+1,0,0,w);
      txt->Init(txtRight,key[i],tb[i]);
   }

   b_ok=new gui_button(4,w->sy-28,0,0,w);
   b_ok->Init(btnDefault,"OK");

   b_cancel=new gui_button(b_ok->x2+4,w->sy-28,0,0,w);
   b_cancel->Init(btnCancel,"Cancel");

   w->ReDraw();
   w->InitPost();

   while (1)
   {
      w->Run(&e);

      if (e.what==evCommand && e.cmd==cmdbPressed)
         break;
   }

   w->UnDraw();
   w->Refresh();

   if (e.control==b_ok)
   {
      for (i=0;i<num;i++)
      {
         strcpy(value[i],tb[i]->GetText());
      }
   }

   delete w;
   Q_free(tb);

   if (e.control==b_ok)
      return 1;
   else
      return 0;
}


