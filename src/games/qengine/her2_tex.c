/*
her2_tex.c file of the Quest Source Code

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

#include "tex.h"
#include "tex_all.h"

#include "heretic2.h"

#include "q2_tex.h"
#include "q2_texi.h"
#include "q2_texi2.h"

#include "qmap.h"

#include "color.h"
#include "error.h"
#include "game.h"
#include "memory.h"
#include "pakdir.h"


static int Her2_TexSort(const char *t1,const char *t2)
{
   int i;
//   return strcmp(NameOnly(t1->name),NameOnly(t2->name));
   i=strcmp(t1,t2);
//   printf("'%s' '%s' %i\n",t1->name,t2->name,i);
   return i;
}


static int Dith(int r,int g,int b)
{
static int er,eg,eb;
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


#define MIPLEVELS 16

typedef struct
{
   int  id;
   char name[32];
   int  width[MIPLEVELS];
   int  height[MIPLEVELS];
   int  offsets[MIPLEVELS];
   char animname[32];
   unsigned char pal[768];
   unsigned int  flags;
   unsigned int  contents;
   unsigned int  value;
} her2_miptex_t;


static int Her2_LoadTexture(tex_name_t *tn)
{
   FILE *f;
   int baseofs;
   int closefile;

   her2_miptex_t h;
   texture_t *t;

   unsigned char *c;
   int size;
   int i,j;


   if (tn->tex)
      return 1;

   f=PD_Load(tn,&baseofs,&closefile);
   if (!f)
      return 0;

   t=Q_malloc(sizeof(texture_t));
   memset(t,0,sizeof(texture_t));

   fseek(f,baseofs,SEEK_SET);
   fread(&h,1,sizeof(h),f);

//   strcpy(t->name,h.name);
   strcpy(t->name,tn->name);
   /* Use the name of the file instead of the name in .wal file. Fix for
      broken .wal file makers. */
   strlwr(t->name);

   t->rsx=t->dsx=h.width[0];
   t->rsy=t->dsy=h.height[0];

   t->g.q2.flags   =h.flags;
   t->g.q2.contents=h.contents;
   t->g.q2.value   =h.value;

   size=t->dsx*t->dsy;
   t->data=Q_malloc(size);
   if (!t->data)
   {
      if (closefile) fclose(f);
      return 0;
   }
   fseek(f,h.offsets[0]+baseofs,SEEK_SET);
   fread(t->data,1,t->dsx*t->dsy,f);

   for (c=t->data,i=size;i;i--,c++)
   {
      j=*c *3;
      *c=Dith(h.pal[j+0]>>2,h.pal[j+1]>>2,h.pal[j+2]>>2);
   }

   t->color=-1;

   if (closefile) fclose(f);

   tn->tex=t;

   return 1;
}


static void AddTName(char *name,int nl,int ofs)
{
   tex_name_t *t;
   int i;

   for (i=0;i<n_tnames;i++)
   {
      if (!stricmp(tnames[i].filename,name))
         return;
   }

   tnames=Q_realloc(tnames,sizeof(tex_name_t)*(n_tnames+1));
   if (!tnames)
      Abort("Her2_Init","Out of memory!");
   t=&tnames[n_tnames];
   memset(t,0,sizeof(tex_name_t));
   n_tnames++;

   strcpy(t->filename,name);
   strcpy(t->name,&name[strlen("textures/")]);
   t->name[strlen(t->name)-3]=0;
   
   t->location=nl;
   t->ofs=ofs;
}


static void Her2_ModifyFlags(void)
{

/*
Materials (values for bit 24-31 of surface flags):

Gravel : 00
Metal  : 01
Stone  : 02
 Wood  : 03

*/

static const char *her2_contents[32]=
{
"Solid",        "Window",       "Aux",          "Lava",         // 00000001
"Slime",        "Water",        "Mist",         NULL,           // 00000010
NULL,           NULL,           NULL,           NULL,           // 00000100
NULL,           NULL,           NULL,           NULL,           // 00001000
"PlayerClip",   "MonsterClip",  "Current_0",    "Current_90",   // 00010000
"Current_180",  "Current_270",  "Current_up",   "Current_down", // 00100000
"Origin",       NULL,           NULL,           "Detail",       // 01000000
"Translucent",  "CameraBlock",  "CameraNoBlock",NULL            // 10000000
};
static const char *her2_flags[32]=
{
"Light",        "Slick",        "Sky",          "Warp",         // 00000001
"Trans33",      "Trans66",      "Flowing",      "NoDraw",       // 00000010
"Hint",         "Skip",         "Tall_wall",    "Alpha_texture",// 00000100
"AnumSpeed",    "Undulate",     "Sky reflect",  NULL,           // 00001000
NULL,           NULL,           NULL,           NULL,           // 00010000
NULL,           NULL,           NULL,           NULL,           // 00100000
"Material",     "Material",     "Material",     "Material",     // 01000000
"Material",     "Material",     "Material",     "Material"      // 10000000
};

   Quake2Flags_ModifyFlags(her2_contents,her2_flags);
}


static game_t Game_Heretic2=
{
   "Heretic2",
   {
      1,0,

      Her2_TexSort,
      NULL,
      Her2_LoadTexture,
      NULL,
      Q2_FlagsDefault,
      Her2_ModifyFlags,
      NULL, // TODO
      NULL  // TODO
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
      L_QUAKE2,
      1,
      0
   },
   {
      1
   },
   ".lin"
};


void Her2_Init(void)
{
   Game=Game_Heretic2;
   qmap_loadtexinfo=Q2_LoadTexInfo;
   qmap_savetexinfo=Q2_SaveTexInfo;

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

   if (!PD_Init())
   {
      PD_Search("textures/",".m8",AddTName);
      PD_Write();
   }
}

