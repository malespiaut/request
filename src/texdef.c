/*
texdef.c file of the Quest Source Code

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

#include "texdef.h"

#include "brush.h"
#include "camera.h"
#include "game.h"
#include "quest.h"
#include "status.h"
#include "tex.h"
#include "undo.h"


void SetTexture(texdef_t *t,char *name)
{
   memset(t->name,0,sizeof(t->name));
   t->shift[0]=t->shift[1]=t->rotate=0;
   t->scale[0]=t->scale[1]=1;
   if (!name[0])
   {
      strcpy(t->name,"no_texture");
   }
   else
   {
      strcpy(t->name,name);
   }
   Game.tex.settexdefdefault(t);
}

void InitTexdef(texdef_t *t)
{
   SetTexture(t,texturename);
}


static void MoveTexdef(texdef_t *td,int dir)
{
   int a=status.snap_size;

   switch (dir)
   {
   case MOVE_LEFT:
      td->shift[0]-=a;
      break;
   case MOVE_RIGHT:
      td->shift[0]+=a;
      break;
   case MOVE_UP:
      td->shift[1]-=a;
      break;
   case MOVE_DOWN:
      td->shift[1]+=a;
      break;
   }
}

static void MoveCurveV(brush_t *b,int i,int dir,float mx,float my)
{
   switch (dir)
   {
   case MOVE_LEFT:
      b->x.q3c.s[i]-=mx;
      break;
   case MOVE_RIGHT:
      b->x.q3c.s[i]+=mx;
      break;

   case MOVE_UP:
      b->x.q3c.t[i]-=my;
      break;
   case MOVE_DOWN:
      b->x.q3c.t[i]+=my;
      break;
   }
}

void MoveSelTVert(int dir)
{
   brushref_t *bs;
   brush_t *b;
   fsel_t *fs;
   int i;
   plane_t *p;
   vsel_t *v;
   int found;
   float s,t;
   texture_t *tex;

   SUndo(UNDO_NONE,UNDO_CHANGE);

   for (bs=M.display.bsel;bs;bs=bs->Next)
   {
      b=bs->Brush;
      switch (b->bt->type)
      {
      case BR_NORMAL:
         for (p=b->plane,i=b->num_planes;i;i--,p++)
            MoveTexdef(&p->tex,dir);
         break;
      case BR_Q3_CURVE:
         found=0;

         tex=ReadMIPTex(b->tex.name,0);
         if (tex)
         {
            s=status.snap_size/(float)tex->rsx;
            t=status.snap_size/(float)tex->rsy;
         }
         else
         {
            s=status.snap_size/(float)64;
            t=status.snap_size/(float)64;
         }

         for (v=M.display.vsel;v;v=v->Next)
         {
            if (v->vert>=b->verts && v->vert<b->verts+b->num_verts)
            {
               found=1;
               MoveCurveV(b,v->vert-b->verts,dir,s,t);
            }
         }
         if (!found)
         {
            for (i=0;i<b->num_verts;i++)
            {
               MoveCurveV(b,i,dir,s,t);
            }
         }
         break;
      }
   }
   for (fs=M.display.fsel;fs;fs=fs->Next)
   {
      if (fs->Brush->bt->type!=BR_NORMAL)
         continue;
      MoveTexdef(&fs->Brush->plane[fs->facenum].tex,dir);
   }
}


static void ScaleTexdef(texdef_t *td,int dir)
{
   switch (dir)
   {
   case MOVE_LEFT:
      td->scale[0]-=0.1;
      break;
   case MOVE_RIGHT:
      td->scale[0]+=0.1;
      break;
   case MOVE_UP:
      td->scale[1]-=0.1;
      break;
   case MOVE_DOWN:
      td->scale[1]+=0.1;
      break;
   }
}

void ScaleSelTVert(int dir)
{
   brushref_t *bs;
   fsel_t *fs;
   int i;
   plane_t *p;

   SUndo(UNDO_NONE,UNDO_CHANGE);

   for (bs=M.display.bsel;bs;bs=bs->Next)
   {
      if (bs->Brush->bt->type!=BR_NORMAL)
         continue;
      for (p=bs->Brush->plane,i=bs->Brush->num_planes;i;i--,p++)
         ScaleTexdef(&p->tex,dir);
   }
   for (fs=M.display.fsel;fs;fs=fs->Next)
   {
      if (fs->Brush->bt->type!=BR_NORMAL)
         continue;
      ScaleTexdef(&fs->Brush->plane[fs->facenum].tex,dir);
   }
}

