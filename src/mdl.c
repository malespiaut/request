/*
mdl.c file of the Quest Source Code

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

#include "mdl.h"
#include "mdl_all.h"

#include "mdl_q.h"
#include "mdl_q2.h"
#include "mdl_hl.h"

#include "color.h"
#include "display.h"
#include "entity.h"
#include "error.h"
#include "memory.h"
#include "entclass.h"
#include "quest.h"
#include "status.h"
#include "video.h"


#if 0
void removedirs(char *basestr,char *newstr)
/*
    Used to remove any directories in front of the filename
    / and \ are treated as the same thing
    Ex: /progs\blah\misc/a.html would return a.html
*/
{
   int i;
   int modify;

   modify=-1;
   for (i=strlen(basestr)-1;i>=0;i--)
   {
      if ((basestr[i]=='/')||(basestr[i]=='\\'))
      {
         modify=i+1;
         break;
      }
   }
   if (modify==-1)
   {
      strcpy(newstr,basestr);
      return;
   }
   for (i=modify;i<strlen(basestr);i++)
   {
      newstr[i-modify]=basestr[i];
   }
   newstr[i-modify]=0;
   return;
}

void getextension(char *basestr,char *extension)
/*
         returns the .whatever of basestr in extension
         Ex: /progs\blah\misc/a.html would return html
*/
{
   int i;
   int modify;

   modify=-1;
   for (i=strlen(basestr)-1;i>=0;i--)
   {
      if (basestr[i]=='.')
      {
         modify=i+1;
         break;
      }
   }

   if (modify==-1)
   {
      extension[0]=0;
      return;
   }

   for (i=modify;i<strlen(basestr);i++)
   {
      extension[i-modify]=basestr[i];
   }

   extension[i-modify]=0;
   return;
}
#endif


char **LoadModelNames(int types,int *numnames)
{
   char **MDLNames=NULL;
   int i;
   int numlines=0;

   for (i=1;i<num_classes;i++)
   {
      if (!(types&classes[i]->type))
         continue;

      MDLNames=Q_realloc(MDLNames,sizeof(char *)*(numlines+1));
      MDLNames[numlines] = (char *)Q_malloc(strlen(classes[i]->name)+1);
      memcpy(MDLNames[numlines],classes[i]->name,strlen(classes[i]->name)+1);
      numlines++;
   }
   *numnames=numlines;
   return MDLNames;
}


void DrawModel(int vport,entity_t *ent,matrix_t m,int selected)
{
   int         i;
   vec3_t      rp, tpos;
   float       cosang, sinang;
   int         size;
   mdl_t       mdl;
   int         from, to;
   float       x0, y0, x1, y1;
   int         dx,dy;
   float       clipx, clipy;
   float       angle;
   vec3_t      pos;
   class_t    *c;
   int         color;
   float       bound[2][3];
   int         ne,nv;
   vec3_t      temp;

   if (GetKeyValue(ent,"classname"))
      c=FindClass(GetKeyValue(ent,"classname"));
   else
      c=NULL;
   if (!c) c=classes[0];

   if (GetKeyValue(ent,"origin"))
   {
      sscanf(GetKeyValue(ent,"origin"),"%f %f %f",&pos.x,&pos.y,&pos.z);
      memcpy(bound,c->bound,sizeof(bound));
   }
   else
   {
      pos=ent->center;
      if ((c->type==CLASS_MODEL) || (c==classes[0]))
      {
         bound[0][0]=ent->min.x;
         bound[0][1]=ent->min.y;
         bound[0][2]=ent->min.z;
         bound[1][0]=ent->max.x;
         bound[1][1]=ent->max.y;
         bound[1][2]=ent->max.z;
      }
   }

   if (GetKeyValue(ent,"angle"))
      sscanf(GetKeyValue(ent,"angle"),"%f",&angle);
   else
      angle=0;

   /* Transform center point */
   tpos.x = ((m[0][0] * pos.x) + (m[0][1] * pos.y) + (m[0][2] * pos.z) + m[0][3]);
   tpos.y = ((m[1][0] * pos.x) + (m[1][1] * pos.y) + (m[1][2] * pos.z) + m[1][3]);
   tpos.z = ((m[2][0] * pos.x) + (m[2][1] * pos.y) + (m[2][2] * pos.z) + m[2][3]);

   if (selected)
      color=GetColor(COL_YELLOW);
   else
      color=GetColor(c->color);

   dx = (M.display.vport[vport].xmax - M.display.vport[vport].xmin) >> 1;
   dy = (M.display.vport[vport].ymax - M.display.vport[vport].ymin) >> 1;

   if (!color) goto draw_focal;

   if (!M.display.vport[vport].fullbright)
   {
      if (M.display.vport[vport].mode == WIREFRAME ||
          M.display.vport[vport].mode == SOLID )
      {
         i=ent->trans.z*9;
         if (i<0) i=0;
         if (i>15) i=15;
         color = COL_DARK(color,i);
      }
   }

   if (c->mdl && status.draw_models)
   {
      nv=c->mdl->numvertices;
      ne=c->mdl->numedges;
   }
   else
   {
      nv=ne=0;
   }

   mdl.numvertices=nv+10;
   mdl.vertlist=Q_malloc(sizeof(vec3_t)*mdl.numvertices);
   mdl.tvertlist=Q_malloc(sizeof(vec3_t)*mdl.numvertices);

   if (nv)
      memcpy(mdl.vertlist,c->mdl->vertlist,sizeof(vec3_t)*nv);

   mdl.vertlist[nv+0].x=bound[0][0];
   mdl.vertlist[nv+0].y=bound[0][1];
   mdl.vertlist[nv+0].z=bound[0][2];

   mdl.vertlist[nv+1].x=bound[1][0];
   mdl.vertlist[nv+1].y=bound[0][1];
   mdl.vertlist[nv+1].z=bound[0][2];

   mdl.vertlist[nv+2].x=bound[1][0];
   mdl.vertlist[nv+2].y=bound[1][1];
   mdl.vertlist[nv+2].z=bound[0][2];

   mdl.vertlist[nv+3].x=bound[0][0];
   mdl.vertlist[nv+3].y=bound[1][1];
   mdl.vertlist[nv+3].z=bound[0][2];

   mdl.vertlist[nv+4].x=bound[0][0];
   mdl.vertlist[nv+4].y=bound[0][1];
   mdl.vertlist[nv+4].z=bound[1][2];

   mdl.vertlist[nv+5].x=bound[1][0];
   mdl.vertlist[nv+5].y=bound[0][1];
   mdl.vertlist[nv+5].z=bound[1][2];

   mdl.vertlist[nv+6].x=bound[1][0];
   mdl.vertlist[nv+6].y=bound[1][1];
   mdl.vertlist[nv+6].z=bound[1][2];

   mdl.vertlist[nv+7].x=bound[0][0];
   mdl.vertlist[nv+7].y=bound[1][1];
   mdl.vertlist[nv+7].z=bound[1][2];

   mdl.vertlist[nv+8].x=0;
   mdl.vertlist[nv+8].y=0;
   mdl.vertlist[nv+8].z=0;

   cosang=1;
   sinang=0;

   if (angle==-1)
   {
      mdl.vertlist[nv+9].x=0;
      mdl.vertlist[nv+9].y=0;
      mdl.vertlist[nv+9].z=20;
   }
   else
   if (angle==-2)
   {
      mdl.vertlist[nv+9].x=0;
      mdl.vertlist[nv+9].y=0;
      mdl.vertlist[nv+9].z=-20;
   }
   else
   {
      cosang = cos(angle * PI / 180);
      sinang = sin(angle * PI / 180);
   
      mdl.vertlist[nv+9].x=cosang*20;
      mdl.vertlist[nv+9].y=sinang*20;
      mdl.vertlist[nv+9].z=0;
   }

   mdl.numedges=ne+13;
   mdl.edgelist=Q_malloc(sizeof(edge_t)*mdl.numedges);

   if (ne)
   {
      memcpy(mdl.edgelist,c->mdl->edgelist,sizeof(edge_t)*ne);
   }

   mdl.edgelist[ne+ 0]=(edge_t){nv+0,nv+1};
   mdl.edgelist[ne+ 1]=(edge_t){nv+1,nv+2};
   mdl.edgelist[ne+ 2]=(edge_t){nv+2,nv+3};
   mdl.edgelist[ne+ 3]=(edge_t){nv+3,nv+0};
   mdl.edgelist[ne+ 4]=(edge_t){nv+4,nv+5};
   mdl.edgelist[ne+ 5]=(edge_t){nv+5,nv+6};
   mdl.edgelist[ne+ 6]=(edge_t){nv+6,nv+7};
   mdl.edgelist[ne+ 7]=(edge_t){nv+7,nv+4};
   mdl.edgelist[ne+ 8]=(edge_t){nv+0,nv+4};
   mdl.edgelist[ne+ 9]=(edge_t){nv+1,nv+5};
   mdl.edgelist[ne+10]=(edge_t){nv+2,nv+6};
   mdl.edgelist[ne+11]=(edge_t){nv+3,nv+7};
   mdl.edgelist[ne+12]=(edge_t){nv+8,nv+9};

   for (i=0; i<mdl.numvertices; i++)
   {
      // rotate model, but not the bounding box
      if (i<nv)
      {
         rp.x=mdl.vertlist[i].x*cosang-mdl.vertlist[i].y*sinang;
         rp.y=mdl.vertlist[i].x*sinang+mdl.vertlist[i].y*cosang;
         rp.z=mdl.vertlist[i].z;
      }
      else
      {
         rp.x=mdl.vertlist[i].x;
         rp.y=mdl.vertlist[i].y;
         rp.z=mdl.vertlist[i].z;
      }

      // offset
      temp.x = rp.x + pos.x;
      temp.y = rp.y + pos.y;
      temp.z = rp.z + pos.z;

      /* Run point through the transformation matrix */
      mdl.tvertlist[i].x = ((m[0][0] * temp.x) +
                            (m[0][1] * temp.y) +
                            (m[0][2] * temp.z) +
                             m[0][3]);
      mdl.tvertlist[i].y = ((m[1][0] * temp.x) +
                            (m[1][1] * temp.y) +
                            (m[1][2] * temp.z) +
                             m[1][3]);
      mdl.tvertlist[i].z = ((m[2][0] * temp.x) +
                            (m[2][1] * temp.y) +
                            (m[2][2] * temp.z) +
                             m[2][3]);
   }

   if (M.display.vport[vport].mode == NOPERSP)
   {
      clipx = dx / M.display.vport[vport].zoom_amt;
      clipy = dy / M.display.vport[vport].zoom_amt;

      for (i=0; i<mdl.numedges; i++)
      {
         from = mdl.edgelist[i].startvertex;
         to = mdl.edgelist[i].endvertex;

         if (((mdl.tvertlist[from].x > clipx) && (mdl.tvertlist[to].x > clipx)) ||
             ((mdl.tvertlist[from].x < -clipx) && (mdl.tvertlist[to].x < -clipx)) ||
             ((mdl.tvertlist[from].y > clipy) && (mdl.tvertlist[to].y > clipy)) ||
             ((mdl.tvertlist[from].y < -clipy) && (mdl.tvertlist[to].y < -clipy)))
            continue;

         x0 = mdl.tvertlist[from].x * M.display.vport[vport].zoom_amt +
              M.display.vport[vport].xmin + dx;
         y0 = -mdl.tvertlist[from].y * M.display.vport[vport].zoom_amt +
              M.display.vport[vport].ymin + dy;
         x1 = mdl.tvertlist[to].x * M.display.vport[vport].zoom_amt +
              M.display.vport[vport].xmin + dx;
         y1 = -mdl.tvertlist[to].y * M.display.vport[vport].zoom_amt +
              M.display.vport[vport].ymin + dy;

         ClipDrawLine2D(vport, (int)(x0+.5), (int)(y0+.5), (int)(x1+.5), (int)(y1+.5), color);
      }
   }
   else
   if (M.display.vport[vport].mode == WIREFRAME ||
       M.display.vport[vport].mode == SOLID)
   {
      for (i=0; i<mdl.numedges; i++)
      {
         ClipDrawLine3D(vport, mdl.tvertlist[mdl.edgelist[i].startvertex].x,
                               mdl.tvertlist[mdl.edgelist[i].startvertex].y,
                               mdl.tvertlist[mdl.edgelist[i].startvertex].z,
                               mdl.tvertlist[mdl.edgelist[i].endvertex].x,
                               mdl.tvertlist[mdl.edgelist[i].endvertex].y,
                               mdl.tvertlist[mdl.edgelist[i].endvertex].z,
                               color);
      }
   }

   Q_free(mdl.edgelist);
   Q_free(mdl.vertlist);
   Q_free(mdl.tvertlist);

draw_focal:
   /* Draw model focal point */
   if (ent->strans.onscreen)
   {
      if (M.display.vport[vport].mode == NOPERSP)
      {
         size = 1.5 * M.display.vport[vport].zoom_amt;
   
         DrawBox(vport, ent->strans.x, ent->strans.y, size, GetColor(COL_WHITE-4));
      }
      else
      if (M.display.vport[vport].mode == WIREFRAME ||
          M.display.vport[vport].mode == SOLID )
      {
         if (tpos.z<0.001)
            size=1/0.001;
         else
            size = 1 / (float)tpos.z;
   
         DrawBox(vport, ent->strans.x, ent->strans.y, size, GetColor(COL_WHITE-4));
      }
   }

   return;
}



// MDL/MD2 loading and converting to internal format code

void AddEdge(mdl_t *m,int v1,int v2)
{
   int i;

   if (v2<v1)
   {   // make sure v1 is the smallest to make searching easier
      i=v2;
      v2=v1;
      v1=i;
   }

   for (i=0;i<m->numedges;i++)
      if ((v1==m->edgelist[i].startvertex) &&
          (v2==m->edgelist[i].endvertex))
         break;

   if (i==m->numedges)
   {
      m->edgelist=
         Q_realloc(m->edgelist,(m->numedges+1)*sizeof(edge_t));
      if (!m->edgelist)
      {
         printf("trying to get %i edges\n",m->numedges);
         Abort("LoadModel","Out of memory!");
      }
      m->edgelist[m->numedges].startvertex=v1;
      m->edgelist[m->numedges].endvertex=v2;
      m->numedges++;
   }
}


// stuff to load MDL/MD2 files from PAK files and handle model caching
typedef struct
{
   char id[4];
   int dstart;
   int dsize;
} pak_header_t;

typedef struct
{
   char name[56];
   int start;
   int size;
} pak_item_t;

typedef struct
{
   char name[56];
   int start;
   char *pakname;
} item_t;

static item_t *items;
static int n_items;

void InitMDLPak(void)
{
   char curpak[128];
   char *curname;
   char *c,*d;

   FILE *f;
   pak_header_t h;
   pak_item_t *e;

   int i;

   c=status.mdlpaks;
   while (*c)
   {
      d=curpak;
      while ((*c) && (*c!=';'))
         *d++=*c++;
      *d=0;

      if (*c==';')
         c++;

      f=fopen(curpak,"rb");
      if (!f)
         continue;

      fread(&h,1,sizeof(h),f);
      fseek(f,h.dstart,SEEK_SET);
      e=Q_malloc(h.dsize);
      if (!e)
         Abort("InitMDLPak","Out of memory!");

      fread(e,1,h.dsize,f);
      curname=NULL;
      for (i=0;i<h.dsize/sizeof(pak_item_t);i++)
      {
         if (strrchr(e[i].name,'.'))
         {
            if (!strcmp(strrchr(e[i].name,'.'),".mdl") ||
                !strcmp(strrchr(e[i].name,'.'),".md2"))
            {
               items=Q_realloc(items,(n_items+1)*sizeof(item_t));

               strcpy(items[n_items].name,e[i].name);
               items[n_items].start=e[i].start;
               if (!curname)
                  curname=Q_strdup(curpak);
               items[n_items].pakname=curname;

               n_items++;
            }
         }
      }

      fclose(f);
   }
}

void DoneMDLPak(void)
{
   if (n_items)
   {
      Q_free(items);
      items=NULL;
      n_items=0;
   }
}

static FILE *FindInPak(char *name,int *baseofs)
{
   int i;
   FILE *f;

   for (i=0;i<n_items;i++)
   {
      if (!strcmp(name,items[i].name))
      {
         *baseofs=items[i].start;
         f=fopen(items[i].pakname,"rb");
         return f;
      }
   }

   return NULL;
}

mdl_t *LoadModel(char *name)
{
   FILE *f;
   mdl_t *temp;
   char ext[16];
   int baseofs;

   if (strrchr(name,'.'))
      strcpy(ext,strrchr(name,'.')+1);
   else
      ext[0]=0;

   if (name[0]==':')
      f=FindInPak(&name[1],&baseofs);
   else
   {
      f=fopen(name,"rb");
      baseofs=0;
   }

   if (!f)
      return NULL;

   if (!strcmp(ext,"md2"))
      temp=Q2_LoadMD2(f,baseofs);
   else
   if (!strcmp(ext,"mdl"))
   {
      char id[4];

      fseek(f,baseofs,SEEK_SET);
      fread(id,1,4,f);
//      printf("id=%c%c%c%c\n",id[0],id[1],id[2],id[3]);
      if (!strncmp(id,"IDPO",4))
         temp=Q_LoadMDL(f,baseofs);
/*      else // not currently working
      if (!strncmp(id,"IDST",4))
         temp=HL_LoadMDL(f,baseofs);*/
      else
         temp=NULL;
   }
   else
      temp=NULL;

   fclose(f);

   return temp;
}

