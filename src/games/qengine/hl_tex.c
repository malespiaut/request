/*
hl_tex.c file of the Quest Source Code

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

#include "tex.h"
#include "tex_all.h"

#include "halflife.h"

#include "qmap.h"

#include "bsp.h"
#include "color.h"
#include "error.h"
#include "game.h"
#include "memory.h"
#include "quest.h"
#include "status.h"
#include "texcat.h"
#include "video.h"


typedef struct
{
	char         name[16];      /* Name of the texture */
	unsigned int width;         /* Width of texture, multiple of 8 */
	unsigned int height;        /* Height of texture, multiple of 8 */
	unsigned int offset1;       /* Offset to 1 x 1 data */
	unsigned int offset2;       /* Offset to 1/2 x 1/2 data */
	unsigned int offset4;       /* Offset to 1/4 x 1/4 data */
	unsigned int offset8;       /* Offset to 1/8 x 1/8 data */
} miptex_t;

typedef struct
{
   char magic[4];               /* "WAD3", Name of the new WAD format */
   int  numentries;             /* Number of entries */
   int  diroffset;              /* Position of WAD directory in file */
} wadhead_t;

typedef struct
{
   int   offset;                /* Position of the entry in WAD */
   int   dsize;                 /* Size of the entry in WAD file */
   int   size;                  /* Size of the entry in memory */
   char  type;                  /* type of entry */
   char  cmprs;                 /* Compression. 0 if none. */
   short dummy;                 /* Not used */
   char  name[16];              /* 1 to 16 characters, '\0'-padded */
} wadentry_t;

// 0x43 = Halflife texture with palette


static int last_location=-1;
static FILE *last_file=NULL;



static int HL_TexSort(const char *t1,const char *t2)
{
   int i;
   if (t1[0]==t2[0])
   {
      if ((t1[0]=='-') || (t1[0]=='+'))
      {
         i=strcmp(&t1[2],&t2[2]);
         if (i) return i;
      }
   }
   return strcmp(t1,t2);
}


static int er,eg,eb;

static int Dith(int r,int g,int b)
{
   int best;
//   unsigned char *c;

   r+=er;
   g+=eg;
   b+=eb;

   if (r<0) r=0;
   if (g<0) g=0;
   if (b<0) b=0;
   if (r>63) r=63;
   if (g>63) g=63;
   if (b>63) b=63;

   best=((r>>3)<<2)+
        ((g>>3)<<5)+
        ((b>>4)<<0);

//   c=&texture_pal[best*3];
//   er=r-c[0];
//   eg=g-c[1];
//   eb=b-c[2];

   er=r&7;
   eg=g&7;
   eb=b&15;

   return best;
}

static int HL_LoadTexture(tex_name_t *tn)
{
   location_t *l;
   unsigned char *tdata;
   unsigned char pal[768];
   miptex_t mip;
   texture_t *t;
   int size;

   unsigned char *c,*d;
   int i;


   if (tn->tex)
      return 1;

   t=Q_malloc(sizeof(texture_t));
   if (!t)
      return 0;
   memset(t,0,sizeof(texture_t));

   l=&locs[tn->location];

   if (tn->location!=last_location)
   {
      if (last_location!=-1)
         fclose(last_file);
      last_file=fopen(l->name,"rb");
      if (!last_file)
      {
         last_location=-1;
         return 0;
      }
      last_location=tn->location;
   }

   fseek(last_file,tn->ofs,SEEK_SET);
   fread(&mip,1,sizeof(miptex_t),last_file);

   size=mip.width*mip.height;

   tdata=Q_malloc(size);
   if (!tdata)
   {
      return 0;
   }
   t->data=Q_malloc(size);
   if (!t->data)
   {
      Q_free(t);
      Q_free(tdata);
      return 0;
   }

   fread(tdata,1,size,last_file);
   fseek(last_file,tn->ofs+mip.offset8+size/64+2,SEEK_SET);
   fread(pal,1,768,last_file);

   er=eg=eb=0;
   for (c=tdata,d=t->data,i=size;i;i--,c++,d++)
   {
      *d=Dith(pal[*c*3+0]>>2,pal[*c*3+1]>>2,pal[*c*3+2]>>2);
   }

   Q_free(tdata);

   strcpy(t->name,tn->name);
   t->dsx=t->rsx=mip.width;
   t->dsy=t->rsy=mip.height;
   t->color=-1;

   tn->tex=t;
   return 1;
}


static int HL_GetTextureCategory(char *name)
{
   int i;

   for (i=0;i<n_tnames;i++)
   {
      if (!strcmp(name,tnames[i].name))
         return tnames[i].location;
   }
   return -1;
}


static void AddTName(char *name,int nl,int ofs)
{
   tex_name_t *t;
   int i;

   for (i=0;i<n_tnames;i++)
   {
      if (!stricmp(tnames[i].name,name))
         return;
   }

   tnames=Q_realloc(tnames,sizeof(tex_name_t)*(n_tnames+1));
   if (!tnames)
      Abort("HL_Init","Out of memory!");
   t=&tnames[n_tnames];
   memset(t,0,sizeof(tex_name_t));
   n_tnames++;

   strcpy(t->name,name);
   t->location=nl;
   t->ofs=ofs;
}


static void AddWad(char *name)
{
   FILE *f;
   wadhead_t wh;
   wadentry_t *entry;
   int i;

   f=fopen(name,"rb");
   if (!f)
      Abort("HL_Init","Can't open '%s'!",name);

   fread(&wh,1,sizeof(wadhead_t),f);
   if (strncmp(wh.magic,"WAD3",4))
      Abort("HL_Init","Invalid WAD3 file: '%s'!",name);

   entry=Q_malloc(sizeof(wadentry_t)*wh.numentries);
   if (!entry)
      Abort("HL_Init","Out of memory!");

   fseek(f,wh.diroffset,SEEK_SET);
   fread(entry,1,sizeof(wadentry_t)*wh.numentries,f);

   fclose(f);

   for (i=0;i<wh.numentries;i++)
   {
      if (entry[i].type==0x43)
      {
         AddTName(entry[i].name,n_locs-1,entry[i].offset);
      }
   }
   Q_free(entry);
}


static void HL_FlagsDefault(texdef_t *t)
{ // no flags, thus do nothing
}

static void HL_ModifyFlags(void)
{
   HandleError("ModifyFlags","No surface flags in Halflife!");
}


static int HL_TexBSPFlags(texdef_t *t)
{
   if (t->name[0]=='*')
      return TEX_FULLBRIGHT|TEX_NONSOLID;
   if (!stricmp(t->name,"sky"))
      return TEX_FULLBRIGHT;
   if (!stricmp(t->name,"clip") ||
       !stricmp(t->name,"origin") ||
       !stricmp(t->name,"aaatrigger"))
      return TEX_NODRAW;

   return 0;
}


static game_t Game_Halflife=
{
   "Halflife",
   {
      1,0,

      HL_TexSort,
      NULL,
      HL_LoadTexture,
      HL_GetTextureCategory,
      HL_FlagsDefault,
      HL_ModifyFlags,
      HL_TexBSPFlags,
      NULL /* No texture description necessary */
   },
   {
      QMap_Load,
      QMap_Save,
      QMap_SaveVisible,
      QMap_LoadGroup,
      QMap_SaveGroup,
      QMap_Profile
   },
   {
      L_QUAKE,
      1,
      0
   },
   {
      0
   },
   ".lin"
};


void HL_Init(void)
{
   char *c,*d;
   char buf[256];
   location_t *l;


   Game=Game_Halflife;
   qmap_loadtexinfo=QMap_LoadTexInfo;
   qmap_savetexinfo=QMap_SaveTexInfo;


   for (c=status.tex_str;*c;)
   {
      d=buf;
      while ((*c!=';') && (*c))
         *d++=*c++;
      *d=0;

      if (*c) c++;

      if (!buf[0])
         continue;

      locs=Q_realloc(locs,(n_locs+1)*sizeof(location_t));
      if (!locs)
         Abort("HL_Init","Out of memory!");
      l=&locs[n_locs];
      memset(l,0,sizeof(location_t));
      n_locs++;

      strcpy(l->name,buf);
      AddWad(l->name);
   }

   {
      int i;
      int r,g,b;
      unsigned char tex_pal[768];

      for (i=0;i<256;i++)
      {
         r=((i>>2)&7)* 8;
         g=((i>>5)&7)* 8;
         b=((i>>0)&3)*16;

         tex_pal[i*3+0]=r;
         tex_pal[i*3+1]=g;
         tex_pal[i*3+2]=b;
      }
      SetTexturePal(tex_pal);
   }

   tcategories=Q_malloc(sizeof(category_t)*n_locs);
   if (!tcategories)
      Abort("HL_Init","Out of memory!");
   n_tcategories=n_locs;
   {
      int i;
      char *temp;

      for (i=0;i<n_locs;i++)
      {
         if (strrchr(locs[i].name,'/'))
            temp=strrchr(locs[i].name,'/')+1;
         else
            temp=locs[i].name;

         strcpy(tcategories[i].name,"- ");
         strcat(tcategories[i].name,temp);
      }
   }
}

