#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "color.h"
#include "error.h"
#include "keyboard.h"
#include "memory.h"
#include "qui.h"
#include "video.h"
}

#include "gui.h"
#include "gui_draw.h"


/*
gui_textbox - Base class for editable text objects.
*/

gui_textbox::gui_textbox(int x1,int y1,int sx,int sy,gui_group *parent)
   : gui_c(x1,y1,sx,sy,parent)
{
   FUNC

   maxlen=0;
   text=NULL;
   focused=0;
   sel_start=sel_end=-1;
   cur_pos=cur_inc=0;
}

gui_textbox::~gui_textbox(void)
{
   FUNC

   if (text)
      Q_free(text);
}

void gui_textbox::Init(unsigned int maxlen,char *default_text)
{
   FUNC

   gui_textbox::maxlen=maxlen;
   text=(char *)Q_malloc(maxlen+1);
   if (!text)
      ABORT("Out of memory!");
   if (default_text)
   {
      strncpy(text,default_text,maxlen);
      text[maxlen]=0;
   }
   else
      *text=0;

   sy=ROM_CHAR_HEIGHT+4;
   UpdatePos();
}


void gui_textbox::Draw(void)
{
   FUNC

   int x,y,dx2;
   int i,j;
   char *c;
   int col1,col2;
   font_t *f;
   int len;
   int cursx;
   int sel;


   GUI_Frame(rx1,ry1,rx2,ry2);
//   GUI_SolidBox(rx1+1,ry1+1,rx2-1,ry2-1,BG_COLOR);
   GUI_SolidBox(rx1+1,ry1+1,rx2-1,ry2-1,15);
   GUI_Frame(rx1+1,ry1+1,rx2-1,ry2-1);

   f=&Q.font[0];
   x=rx1+2;
   y=ry1+2;
   dx2=rx2-2;
   j=0;

   c=text;
   if (cur_inc)
   {
      QUI_DrawChar(x,y,GetColor(15),GetColor(COL_GREEN-2),0,0,'<');
      x+=f->width[f->map[(int)'<']];
      for (i=cur_inc;i && *c;i--) c++,j++;
   }

   if (QUI_strlen(0,c)+x>dx2)
      dx2-=f->width[f->map[(int)'>']];

   cursx=-1;
   if (j==cur_pos)
      cursx=x;
   while (*c)
   {
      if (f->map[(int)*c]!=-1)
         len=f->width[f->map[(int)*c]];
      else
         len=8;
      if (x+len>dx2)
         break;

      if ((j>=sel_start && j<sel_end) && focused)
         col2=GetColor(COL_BLUE-6),sel=1,col1=GetColor(15);
      else
         col2=GetColor(15),sel=0,col1=0;

      QUI_DrawChar(x,y,col2,col1,0,0,*c);
      x+=len;

/*      if (sel)
         if (j<sel_end-1)
            VLine(x-1,ry1+2,ry2-3,col2); // fix the last line*/

      c++,j++;
      if (j==cur_pos)
         cursx=x;
   }
   if (j==cur_pos)
      cursx=x;
   if (*c)
   {
      QUI_DrawChar(rx2-2-f->width[f->map[(int)'>']],y,
         GetColor(15),GetColor(COL_GREEN-2),0,0,'>');
   }

   if (focused && cursx!=-1)
   {
      GUI_VLine(cursx-1,ry1+1,ry2-1,0);
      GUI_VLine(cursx  ,ry1+1,ry2-1,0);
   }

//   QUI_DrawStrM(rx1+2,ry1+2,rx2-2,GetColor(BG_COLOR),col,0,0,"%s",text);
}


void gui_textbox::DeleteSel(void)
{
   char *c,*d;

   if (sel_start==-1)
      return;
   c=&text[sel_start];
   d=&text[sel_end];
   for (;*d;)
      *c++=*d++;
   *c=0;
   MoveTo(sel_start);
   sel_start=sel_end=-1;
}

void gui_textbox::InsertStr(int len,char *str)
{
   FUNC

   int slen;
   char *c,*d;

   if (sel_start!=-1)
      slen=sel_end-sel_start;
   else
      slen=0;
   if (strlen(text)-slen+len>maxlen)
      return;
//      return NULL;

   if (sel_start!=-1)
   {
      DeleteSel();
   }

   if (!text[cur_pos])
   { // at the end of the string, simply advance the end
      text[cur_pos+len]=0;
      c=&text[cur_pos];
      memcpy(c,str,len);
      MoveTo(cur_pos+len);
      return;
//      return c;
   }

// copy manually to make sure there are no overwrites
   slen=strlen(text)-cur_pos;
   c=&text[strlen(text)-1];
   d=&text[strlen(text)+len];
   *d--=0;
   for (;slen;slen--) *d--=*c--;

   c++;
   memcpy(c,str,len);
   MoveTo(cur_pos+len);
//   return c;
}


void gui_textbox::MoveTo(int pos)
{
   FUNC

   char *c;
   int x;
   int i,j;
   int len;
   font_t *f;


   if (pos>(int)strlen(text)+1)
   {
      cur_inc=cur_pos=0;
      return;
   }
   if (!strlen(text))
   {
      cur_inc=cur_pos=0;
      return;
   }

   f=&Q.font[0];

   c=text;
   x=sx-4;
   if (QUI_strlen(0,text)>x)
      x-=f->width[f->map[(int)'>']];

   j=0;
   if (cur_inc)
   {
      for (i=cur_inc;i && *c;i--,c++,j++) ;
      x-=f->width[f->map[(int)'<']];
   }
   if (j>=pos)
   {
      cur_inc=pos-4;
      if (cur_inc<0) cur_inc=0;
      cur_pos=pos;
      return;
   }

   while (*c)
   {
      if (f->map[(int)*c]!=-1)
         len=f->width[f->map[(int)*c]];
      else
         len=8;
      if (x-len<0)
         break;
      j++;
      c++;
      x-=len;
   }

   if (j>=pos)
   {
      cur_pos=pos;
      return;
   }

   j=pos+4;
   x=sx-4-f->width[f->map[(int)'>']]-f->width[f->map[(int)'<']];
   if (j>=(int)strlen(text))
   {
      j=strlen(text)-1;
   }
   c=&text[j];
   j++;
   while (x && j)
   {
      if (f->map[(int)*c]!=-1)
         len=f->width[f->map[(int)*c]];
      else
         len=8;
      if (x-len<=0)
         break;
      j--;
      c--;
      x-=len;
   }
   cur_inc=j;
   cur_pos=pos;
}


void gui_textbox::MoveSel(int npos)
{
   int i;

   if (sel_start==-1)
   {
      if (npos<cur_pos)
      {
         sel_start=npos;
         sel_end=cur_pos;
      }
      else
      {
         sel_end=npos;
         sel_start=cur_pos;
      }
      MoveTo(npos);
      return;
   }

   if (sel_start==cur_pos)
   {
      sel_start=npos;
      if (sel_start==sel_end)
      {
         sel_start=sel_end=-1;
         MoveTo(npos);
         return;
      }
      if (sel_start>sel_end)
      {
         i=sel_end;
         sel_end=sel_start;
         sel_start=i;
      }
      MoveTo(npos);
      return;
   }

   if (sel_end==cur_pos)
   {
      sel_end=npos;
      if (sel_start==sel_end)
      {
         sel_start=sel_end=-1;
         MoveTo(npos);
         return;
      }
      if (sel_start>sel_end)
      {
         i=sel_end;
         sel_end=sel_start;
         sel_start=i;
      }
      MoveTo(npos);
      return;
   }

// confused
   sel_start=sel_end=-1;
   MoveTo(npos);
}


int gui_textbox::SkipWord(int start,int dir)
{
   start+=dir;
   if (start<0)
      return 0;
   if (start>(int)strlen(text))
      return strlen(text);

   while (text[start]<=32)
   {
      start+=dir;
      if (start<0)
         return 0;
      if (start>(int)strlen(text))
         return strlen(text);
   }
   while (text[start]>32)
   {
      start+=dir;
      if (start<0)
         return 0;
      if (start>(int)strlen(text))
         return strlen(text);
   }
   if (start<(int)strlen(text))
      return start+1;
   return start;
}


void gui_textbox::HandleEvent(event_t *e)
{
   FUNC

   if (e->what==evKey)
   {
      int moveto;
      moveto=-1;
//      printf("key event %i '%c'\n",e->key,e->key);
      switch (e->key)
      {
/*      case KEY_ENTER:
         e->what=evNothing;
         Parent->FocusNext();
         return;*/

      case KEY_BACKSPC:
         e->what=evNothing;
         if (sel_start!=-1)
            DeleteSel();
         else
         {
            if (cur_pos)
            {
               char *c;

               for (c=&text[cur_pos-1];*c;c++)
                  *c=c[1];
               MoveTo(cur_pos-1);
            }
         }
         ReDraw();
         return;
      case KEY_DELETE:
         e->what=evNothing;
         if (sel_start!=-1)
            DeleteSel();
         else
         {
            if (cur_pos<(int)strlen(text))
            {
               char *c;

               for (c=&text[cur_pos];*c;c++)
                  *c=c[1];
            }
         }
         ReDraw();
         break;

      case KEY_LEFT:
      case KEY_LEFT+KS_SHIFT:
         e->what=evNothing;
         if (cur_pos)
            moveto=cur_pos-1;
         else
            moveto=0;
         break;
      case KEY_RIGHT:
      case KEY_RIGHT+KS_SHIFT:
         e->what=evNothing;
         if (cur_pos<(int)strlen(text))
            moveto=cur_pos+1;
         else
            moveto=cur_pos;
         break;
      case KEY_LEFT+KS_CTRL:
      case KEY_LEFT+KS_CTRL+KS_SHIFT:
         moveto=SkipWord(cur_pos,-1);
         break;
      case KEY_RIGHT+KS_CTRL:
      case KEY_RIGHT+KS_CTRL+KS_SHIFT:
         moveto=SkipWord(cur_pos, 1);
         break;
      case KEY_HOME:
      case KEY_HOME+KS_SHIFT:
         moveto=0;
         break;
      case KEY_END:
      case KEY_END+KS_SHIFT:
         moveto=strlen(text);
         break;

      default:
         if (e->key&KS_CHAR)
         {
            int k;
            k=e->key&~KS_CHAR;
            if (((k<=128) && (Q.font[0].map[k]!=-1)) ||
                (k==32))
            {
               char c;
   
               c=k;
               InsertStr(1,&c);
               ReDraw();
               e->what=evNothing;
            }
            return;
         }
      }
      if (moveto!=-1)
      {
         if (e->key&KS_SHIFT)
         {
            MoveSel(moveto);
         }
         else
         {
            sel_start=sel_end=-1;
            MoveTo(moveto);
         }
         ReDraw();
      }
   }
}

void gui_textbox::FocusOn(void)
{
   FUNC

   focused=1;
   sel_start=0;
   sel_end=strlen(text);
//   sel_start=sel_end=-1;
   MoveTo(strlen(text));
   ReDraw();
}

void gui_textbox::FocusOff(void)
{
   FUNC

   focused=0;
   cur_inc=cur_pos=0;
   sel_start=sel_end=-1;
   SendEvent_Cmd(cmdChanged);
   ReDraw();
}


void gui_textbox::SetText(char *ntext)
{
   strcpy(text,ntext);
   sel_start=sel_end=-1;
   cur_pos=cur_inc=0;
   ReDraw();
}

char *gui_textbox::GetText(void)
{
   return text;
}


/*
gui_textbox_int - Text edit object, but text will always be int.
*/

gui_textbox_int::gui_textbox_int(int x1,int y1,int sx,int sy,gui_group *parent)
   : gui_textbox(x1,y1,sx,sy,parent)
{
   value=0;
}

void gui_textbox_int::Init(int startvalue)
{
   char buf[128];

   sprintf(buf,"%i",startvalue);
   gui_textbox::Init(128,buf);
   value=startvalue;
}

void gui_textbox_int::SetValue(int value)
{
   char buf[128];

   gui_textbox_int::value=value;
   sprintf(buf,"%i",value);
   SetText(buf);
}

int gui_textbox_int::GetValue(void)
{
   value=atoi(text);
   return value;
}

void gui_textbox_int::FocusOff(void)
{
   char buf[128];

   value=atoi(text);
   sprintf(buf,"%i",value);
   SetText(buf);
   gui_textbox::FocusOff();
}


/*
gui_textbox_float - Text edit object, but text will always be int.
*/

gui_textbox_float::gui_textbox_float(int x1,int y1,int sx,int sy,gui_group *parent)
   : gui_textbox(x1,y1,sx,sy,parent)
{
   value=0;
}

void gui_textbox_float::Init(float startvalue)
{
   char buf[128];

   sprintf(buf,"%g",startvalue);
   gui_textbox::Init(128,buf);
   value=startvalue;
}

void gui_textbox_float::SetValue(float value)
{
   char buf[128];

   gui_textbox_float::value=value;
   sprintf(buf,"%g",value);
   SetText(buf);
}

float gui_textbox_float::GetValue(void)
{
   value=atof(text);
   return value;
}

void gui_textbox_float::FocusOff(void)
{
   char buf[128];

   value=atof(text);
   sprintf(buf,"%g",value);
   SetText(buf);
   gui_textbox::FocusOff();
}

