#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "error.h"
#include "mouse.h"
}

#include "gui.h"


/*
gui_group - Base group class, for stuff like windows and controls with children.
*/

gui_group::gui_group(int x1,int y1,int sx,int sy,gui_group *parent)
   : gui_c(x1,y1,sx,sy,parent)
{
   FUNC

   Children=Focus=NULL;
   numevents=0;
   last_child=NULL;
}

gui_group::~gui_group(void)
{
   FUNC

   gui_c *gc,*next;

/*   if (Focus)
      Focus->FocusOff();*/

   for (gc=Children;gc;gc=next)
   {
      next=gc->Next;

      delete gc;
   }
}

void gui_group::AddChild(gui_c *child)
{
   FUNC

   child->Parent=this;
   if (!Children)
   {
      Children=child;
      return;
   }

#if 1
// stuff it at the end of the list, assume caller knows how he wants things
   gui_c *gc;

   for (gc=Children;gc->Next;gc=gc->Next) { }
   gc->Next=child;
   child->Prev=gc;
#else
// keep things sorted by primarily y coordinate and secondarily x coordinate
   gui_c *p;

   for (p=Children;p->Next;p=p->Next)
   {
      if (p->y1>child->y1)
      {
         p=p->Prev;
         break;
      }
      if ((p->y1==child->y1) == (p->x1>child->x1))
      {
         p=p->Prev;
         break;
      }
   }

   if (!p)
   {
      Children->Prev=child;
      child->Next=Children;
      Children=child;
      return;
   }
   if (p->Next) p->Next->Prev=child;
   child->Next=p->Next;
   child->Prev=p;
   p->Next=child;
#endif
}

gui_c *gui_group::FindChild(int x,int y)
{
   FUNC

   gui_c *gc;

   for (gc=Children;gc;gc=gc->Next)
   {
      if ((x>=gc->rx1) && (x<=gc->rx2) &&
          (y>=gc->ry1) && (y<=gc->ry2))
         return gc;
   }
   return NULL;
}

void gui_group::Draw(void)
{
   FUNC

   gui_c *p;
   for (p=Children;p;p=p->Next)
   {
      p->Draw();
   }
}


void gui_group::FocusSet(gui_c *c)
{
   FUNC

   if (Focus==c)
      return;

   if (Focus)
   {
      Focus->FocusOff();
   }
   if (!c)
   {
      Focus=NULL;
      return;
   }
   if (!(c->Flags&flCanFocus))
   {
      Focus=NULL;
      return;
   }
   Focus=c;
   if (Focus)
   {
      Focus->FocusOn();
   }
}

void gui_group::FocusUnset(gui_c *c)
{
   FUNC

   if (Focus!=c)
      return;

   Focus->FocusOff();
   Focus=NULL;
}

void gui_group::FocusNext(void)
{
   FUNC

   gui_c *c,*first;

   if (!Children)
      return;

   for (c=first=Focus;;)
   {
      if (!c)
         c=Children;
      else
      if (c->Next)
         c=c->Next;
      else
         c=Children;

      if (c->Flags&flCanFocus)
      {
         FocusSet(c);
         return;
      }
      if (c==first)
         break;
      if (!first) first=c;
   }
}

void gui_group::FocusPrev(void)
{
   FUNC

   gui_c *c;
   gui_c *last;
   gui_c *first;

   if (!Children)
      return;

   for (last=Children;last->Next;last=last->Next) ;

   for (c=first=Focus;;)
   {
      if (!c)
         c=last;
      else
      if (c->Prev)
         c=c->Prev;
      else
         c=last;

      if (c->Flags&flCanFocus)
      {
         FocusSet(c);
         return;
      }
      if (c==first)
         break;

      if (!first) first=c;
   }
}


void gui_group::Run(event_t *ev)
{
   FUNC

   do
   {
      if (numevents)
      {
         numevents--;
         *ev=EventQue[numevents];
         return;
      }

      GetEvent(ev);

      if (ev->what)
         HandleEvent(ev); // see if we can handle it, pass along to children

      if (ev->what)  // we couldn't handle it, let the caller try
         return;
   } while (1);
}


void gui_group::HandleEvent(event_t *ev)
{
   FUNC

   gui_c *gc;


   if (ev->what&(evKey|evCommand))
   {
// see if any child has the PreEvent bit set, if so give them the event
      for (gc=Children;gc;gc=gc->Next)
         if (gc->Flags&flPreEvent)
         {
            gc->HandleEvent(ev);
            if (!ev->what)
               return;
         }
   }


   // if it's a broadcasted command, let the children try handling it
   if (ev->what&evCommand)
   {
      for (gc=Children;gc;gc=gc->Next)
      {
         gc->HandleEvent(ev);
         if (!ev->what)
            return;
      }
      return;
   }

   // if it's a mouse event, give it to the child that the mouse is within
   if (ev->what&evMouse)
   {
      if (ev->what!=evMouseMove)
      {
// if it's a up or repeat command, give to the child that got the down command
         if ((ev->what!=evMouse1Down) && last_child)
         {
            last_child->HandleEvent(ev);
         }
         else
         {
            // first event to this child, pretend it's a mouse down command
            ev->what=evMouse1Down;
            for (gc=Children;gc;gc=gc->Next)
            {
               if (!(gc->Flags&flCanFocus))
                  continue;
               if (GUI_InBox(gc->rx1,gc->ry1,gc->rx2,gc->ry2))
                  break;
            }
            if (gc)
            {
               FocusSet(gc);
               last_child=gc;
               gc->HandleEvent(ev);
               if (!ev->what)
                  return;
            }
         }
      }

#if 0
// we aren't interested in it, and the parent shouldn't bother, so ignore it
      ev->what=evNothing;
#endif
      return;
   }
   last_child=NULL;

   // if it's a keyboard event, pass to focused child
   if (ev->what&evKey)
   {
      if (Focus)
      {
         Focus->HandleEvent(ev);
         if (!ev->what)
            return;
      }
//      printf("key event, key=%i '%c' shift=%i\n",ev->key,ev->key,ev->shift);
   }


   if (ev->what&(evKey|evCommand))
   {
// see if any child has the PostEvent bit set, if so give them the event
      for (gc=Children;gc;gc=gc->Next)
         if (gc->Flags&flPostEvent)
         {
            gc->HandleEvent(ev);
            if (!ev->what)
               return;
         }
   }
}

void gui_group::SendEvent(event_t *e)
{
   FUNC

   HandleEvent(e);
   if (e->what==evNothing)
      return;

   if (Parent)
   {
      Parent->SendEvent(e);
      return;
   }

   if (numevents>=MAX_EVENTQUE_EVENTS)
      ABORT("Too many events in event que!");

   EventQue[numevents++]=*e;
}

