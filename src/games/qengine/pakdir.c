/*
games/qengine/pakdir.c file of the Quest Source Code

Copyright 1997, 1998, 1999 Alexander Malmberg
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

#include "pakdir.h"

#include "tex.h"
#include "tex_all.h"

#include "error.h"
#include "filedir.h"
#include "memory.h"
#include "status.h"


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

#define LOC_DIR 0
#define LOC_PAK 1


typedef struct
{
   FILE *f;
   pak_item_t *items;
} pak_location_t;

static pak_location_t *paks;


static void (*addtname)(char *name,int nl,int ofs);
static const char *base_dir,*ext;


static void AddTNameDir_r(char *dir,const char *base,int nl)
{
   struct directory_s *d;
   filedir_t f;

   char name[256],n2[128];

   strcpy(name,dir);
   strcat(name,"*");

   d=DirOpen(name,FILE_NORMAL|FILE_DIREC);
   if (!d)
      return;
   while (DirRead(d,&f))
   {
      if (!strcmp(f.name,".") || !strcmp(f.name,".."))
         continue;
      if (f.type==FILE_DIREC)
      {
         strcpy(name,dir);
         strcat(name,f.name);
         strcat(name,"/");

         strcpy(n2,base);
         strcat(n2,f.name);
         strcat(n2,"/");

         AddTNameDir_r(name,n2,nl);
      }

      strcpy(name,f.name);
      if (strlen(name)<strlen(ext))
         continue;
      if (stricmp(&name[strlen(name)-strlen(ext)],ext))
         continue;

      strcpy(name,base);
      strcat(name,f.name);

      addtname(name,nl,0);

      if (!(n_tnames&15))
      {
         printf(".");
         fflush(stdout);
      }
   }
   DirClose(d);
}

static int AddTNameDir(int nl)
{
   char buf[256];

   strcpy(buf,locs[nl].name);
   strcat(buf,base_dir);
   AddTNameDir_r(buf,base_dir,nl);

   return 1;
}


static int AddTNamePak(int nl)
{
   FILE *f;
   pak_header_t h;
   pak_item_t *items;
   int i,j;

   f=fopen(locs[nl].name,"rb");
   if (!f)
      return 0;

   fread(&h,1,sizeof(h),f);
   fseek(f,h.dstart,SEEK_SET);
   items=Q_malloc(h.dsize);
   if (!items)
   {
      fclose(f);
      return 0;
   }
   fread(items,1,h.dsize,f);

   j=h.dsize/sizeof(pak_item_t);
   for (i=0;i<j;i++)
   {
      if (!strnicmp(items[i].name,base_dir,strlen(base_dir)))
      {
         if (!stricmp(&items[i].name[strlen(items[i].name)-strlen(ext)],ext))
         {
            addtname(items[i].name,nl,i);

            if (!(n_tnames%10))
            {
               printf(".");
               fflush(stdout);
            }
         }
      }
   }

   Q_free(items);
   fclose(f);

   return 1;
}


static void LoadPaks(void)
{
   FILE *f;
   pak_header_t ph;
   int i;

   paks=Q_malloc(sizeof(pak_location_t)*n_locs);
   if (!paks)
      Abort("PD_Init","Out of memory!");
   memset(paks,0,sizeof(pak_location_t)*n_locs);

   for (i=0;i<n_locs;i++)
   {
      if (locs[i].type!=LOC_PAK)
         continue;
      f=fopen(locs[i].name,"rb");
      if (!f)
         Abort("PD_Init","Unable to open '%s'!",locs[i].name);
      paks[i].f=f;
      fread(&ph,1,sizeof(pak_header_t),f);
      fseek(f,ph.dstart,SEEK_SET);
      paks[i].items=Q_malloc(ph.dsize);
      if (!paks[i].items)
         Abort("PD_Init","Out of memory!");
      fread(paks[i].items,1,ph.dsize,f);
   }
}


int PD_Init(void)
{
   FILE *f;
   char buf[256];
   int ver;

   f=fopen("textures.dat","rb");
   if (f)
   {
      fread(buf,1,256,f);
      if (strcmp(buf,status.tex_str))
         goto bad;
      fread(&ver,1,sizeof(int),f);
      if (ver!=sizeof(location_t))
         goto bad;
      fread(&ver,1,sizeof(int),f);
      if (ver!=sizeof(tex_name_t))
         goto bad;

      fread(&n_locs,1,sizeof(int),f);
      locs=Q_malloc(n_locs*sizeof(location_t));
      if (!locs)
         Abort("PD_Init","Out of memory!");
      fread(locs,1,n_locs*sizeof(location_t),f);

      fread(&n_tnames,1,sizeof(int),f);
      tnames=Q_malloc(n_tnames*sizeof(tex_name_t));
      if (!tnames)
         Abort("PD_Init","Out of memory!");
      fread(tnames,1,n_tnames*sizeof(tex_name_t),f);

      fclose(f);

      LoadPaks();
      return 1;

bad:
      fclose(f);
   }
   printf("Generating textures.dat:\n");
   
   n_tnames=n_locs=0;
   tnames=NULL;
   locs=NULL;
   
   return 0;
}


/*
 TODO : locations are allocated once for every call to PD_Search(), should
 allocate them in PD_Init() and reuse them here. No big deal, though.
*/
void PD_Search(const char *p_dir,const char *p_ext,
               void (*p_addtname)(char *name,int nl,int ofs))
{
   char buf[256];
   char *c,*d;
   location_t *l;


   base_dir=p_dir;
   ext=p_ext;
   addtname=p_addtname;


   c=status.tex_str;
   while (*c)
   {
      d=buf;
      for (;*c && (*c!=';');c++)
         *d++=*c;

      if (*c==';')
         c++;

      *d=0;
      if (!strlen(buf)) break;

      locs=Q_realloc(locs,(n_locs+1)*sizeof(location_t));
      if (!locs)
         Abort("PD_Init","Out of memory!");
      l=&locs[n_locs];
      memset(l,0,sizeof(location_t));
      n_locs++;

      l->type=LOC_DIR;
      if (strlen(buf)>=5)
      {
         if (!stricmp(&buf[strlen(buf)-4],".pak"))
            l->type=LOC_PAK;
      }

      if (l->type==LOC_DIR)
         if (buf[strlen(buf)-1]!='/')
            strcat(buf,"/");

      strcpy(l->name,buf);

      printf("%s",l->name);
      fflush(stdout);

      switch (l->type)
      {
      case LOC_DIR:
         if (!AddTNameDir(n_locs-1))
            Abort("PD_Init","Unable to load textures from '%s'!",
               l->name);
         break;
      case LOC_PAK:
         if (!AddTNamePak(n_locs-1))
            Abort("PD_Init","Unable to load textures from '%s'!",
               l->name);
         break;
      }
      printf("\n");
   }
}

void PD_Write(void)
{
   FILE *f;
   int ver;

   if (!n_tnames)
      Abort("PD_Write","No textures found!");

   f=fopen("textures.dat","wb");
   if (!f)
   {
      HandleError("PD_Write","Unable to write textures.dat!");
      LoadPaks();
      return;
   }

   fwrite(status.tex_str,1,256,f);
   ver=sizeof(location_t);
   fwrite(&ver,1,sizeof(int),f);
   ver=sizeof(tex_name_t);
   fwrite(&ver,1,sizeof(int),f);

   fwrite(&n_locs,1,sizeof(int),f);
   fwrite(locs,1,n_locs*sizeof(location_t),f);

   fwrite(&n_tnames,1,sizeof(int),f);
   fwrite(tnames,1,n_tnames*sizeof(tex_name_t),f);

   fclose(f);

   LoadPaks();
}


FILE *PD_Load(tex_name_t *tn,int *baseofs,int *closefile)
{
   location_t *l;
   char name[512];
   FILE *f;


   if (tn->location==-1)
      return NULL;

   l=&locs[tn->location];

   switch (l->type)
   {
   case LOC_DIR:
      *closefile=1;

      strcpy(name,l->name);
      strcat(name,tn->filename);

      f=fopen(name,"rb");
      if (!f)
         return 0;

      *baseofs=0;
      return f;

   case LOC_PAK:
      *closefile=0;
      {
         pak_item_t *item;

         f=paks[tn->location].f;

         strcpy(name,tn->filename);

         item=&paks[tn->location].items[tn->ofs];

         if (stricmp(name,item->name))
         {
            HandleError("PD_Load","Shouldn't happen! Please report!");
            return NULL;   // even more stuff that should never happen
         }
         *baseofs=item->start;
      }
      return f;

   default:
      return NULL;
   }
}

