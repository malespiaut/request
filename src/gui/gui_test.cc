#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

int GUI_SelectFile(char *title,char *extension,char *button_text,char *result);
}

#include "gui.h"


int GUI_SelectFile(char *title,char *extension,char *button_text,char *result)
{
   gui_win *win;
   gui_button *b_ok,*b_cancel,*b_test;
   gui_textbox *tb1;
   gui_textbox_int *tb2;
   gui_textbox_float *tb3;
   gui_label *txt1;

   gui_checkbox *cb[4];
   gui_list *list;

   event_t ev;

   int i;


// create the window
   win=new gui_win(-1,-1,256,256);
   win->Init("%s",title);

// create and add the children
   tb1=new gui_textbox(4,48,248,-1,win);
   tb1->Init(128,"Test1");

   txt1=new gui_label(-1,-1,-1,-1,win);
   txt1->Init(txtLeft,"Test string:",tb1);

   tb2=new gui_textbox_int(4,70,248,-1,win);
   tb2->Init(5);

   tb3=new gui_textbox_float(4,92,248,-1,win);
   tb3->Init(1.42);


   list=new gui_list(4,114+18,-1,-1,win);

   for (i=0;i<4;i++)
   {
      char str[256];

      sprintf(str,"Test %i (%i)",i,1<<i);

      cb[i]=new gui_checkbox(-1,-1,-1,-1,list);
      cb[i]->Init(cbvOff,0,str);
   }

   list->Init(1);


   txt1=new gui_label(-1,-1,-1,-1,win);
   txt1->Init(txtLeft,"~Surface flags:",list);


   b_ok=new gui_button(4,228,-1,-1,win);
   b_ok->Init(btnDefault,"OK");

   b_cancel=new gui_button(b_ok->x2+4,228,-1,-1,win);
   b_cancel->Init(btnCancel,"Cancel");

   b_test=new gui_button(b_cancel->x2+4,228,-1,-1,win);
   b_test->Init(0,"Test");

// draw the window (and its children)
   win->ReDraw();

   do
   {
      win->Run(&ev);

      if (ev.what==evCommand)
      {
         if (ev.control==b_ok)
            break;
         if (ev.control==b_cancel)
            break;
      }
   } while (1);

   win->UnDraw();
   win->Refresh();

   printf("'%s' %i %g\n",tb1->GetText(),tb2->GetValue(),tb3->GetValue());

   delete win;

   if (ev.control==b_ok)
      printf("OK\n");
   else
   if (ev.control==b_cancel)
      printf("Cancel\n");
   else
      printf("????\n");

   return 0;
}

