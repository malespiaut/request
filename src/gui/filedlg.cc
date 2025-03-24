#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "filedir.h"

  int SelectFile(char* title, char* extension, char* button_text, char* result);
}

#include "gui.h"

#if 0
int SelectFile(char *title,char *extension,char *button_text,char *result)
{
   gui_win *win;

   gui_scroll_list *files;

   gui_button *b_ok,*b_cancel;

   event_t ev;

   win=new gui_win(-1,-1,600,400);
   win->Init(title);


   files=new gui_scroll_list(8,32,200,200,win);
   files->Init();

   files->Data_New();
   {
      int i;
      for (i=0;i<30;i++) files->Data_Add(tmpnam(NULL));
   }
   files->Data_Done();


   b_ok=new gui_button(4,win->sy-28,-1,-1,win);
   b_ok->Init(btnDefault,button_text);

   b_cancel=new gui_button(b_ok->x2+4,b_ok->y1,-1,-1,win);
   b_cancel->Init(btnCancel,"Cancel");

   win->InitPost();
   win->ReDraw();

   while (1)
   {
      win->Run(&ev);

      if ((ev.what==evCommand) && (ev.cmd==cmdbPressed))
         break;
   }


   win->UnDraw();
   win->Refresh();
   delete win;

   return 0;
}
#endif
