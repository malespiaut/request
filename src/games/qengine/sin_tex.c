/*
sin_tex.c file of the Quest Source Code

Copyright 1997, 1998, 1999 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "tex.h"
#include "tex_all.h"

#include "sin.h"

#include "qmap.h"
#include "sin_texi.h"

#include "color.h"
#include "error.h"
#include "game.h"
#include "memory.h"
#include "pakdir.h"
#include "token.h"

/*static char *NameOnly(const char *name)
{
   if (strrchr(name,'/'))
      return strrchr(name,'/')+1;
   else
      return (char *)name;
}*/

static int
Sin_TexSort(const char* t1, const char* t2)
{
  int i;
  //   return strcmp(NameOnly(t1->name),NameOnly(t2->name));
  i = strcmp(t1, t2);
  //   printf("'%s' '%s' %i\n",t1->name,t2->name,i);
  return i;
}

static int
Dith(int r, int g, int b)
{
  static int er, eg, eb;
  int best;
  //   unsigned char *c;

  r += er;
  g += eg;
  b += eb;

  if (r < 0)
    r = 0;
  if (g < 0)
    g = 0;
  if (b < 0)
    b = 0;
  if (r > 63)
    r = 63;
  if (g > 63)
    g = 63;
  if (b > 63)
    b = 63;

  best = ((r >> 3) << 2) +
         ((g >> 3) << 5) +
         ((b >> 4) << 0);

  //   c=&texture_pal[best*3];
  //   er=r-c[0];
  //   eg=g-c[1];
  //   eb=b-c[2];

  er = r & 7;
  eg = g & 7;
  eb = b & 15;

  return best;
}

#define SIN_PALETTE_SIZE 1024
#define MIPLEVELS 4

typedef struct
{
  char name[64];
  unsigned width, height;
  unsigned char palette[SIN_PALETTE_SIZE];
  unsigned short palcrc;
  unsigned offsets[MIPLEVELS]; // four mip maps stored
  char animname[64];           // next frame in animation chain
  unsigned flags;
  unsigned contents;
  unsigned short value;
  unsigned short direct;
  float animtime;
  float nonlit;
  unsigned short directangle;
  unsigned short trans_angle;
  float directstyle;
  float translucence;
  float friction;
  float restitution;
  float trans_mag;
  float color[3];
} sin_miptex_t;

static int
Sin_LoadTexture(tex_name_t* tn)
{
  FILE* f;
  int baseofs;
  int closefile;

  sin_miptex_t h;
  texture_t* t;

  unsigned char* c;
  int size;
  int i, j;

  if (tn->tex)
    return 1;

  f = PD_Load(tn, &baseofs, &closefile);
  if (!f)
    return 0;

  t = Q_malloc(sizeof(texture_t));
  memset(t, 0, sizeof(texture_t));

  fseek(f, baseofs, SEEK_SET);
  fread(&h, 1, sizeof(h), f);

  //   strcpy(t->name,h.name);
  strcpy(t->name, tn->name);
  /* Use the name of the file instead of the name in .wal file. Fix for
     broken .wal file makers. */
  strlwr(t->name);

  t->rsx = t->dsx = h.width;
  t->rsy = t->dsy = h.height;

  strcpy(t->g.sin.animname, h.animname);
  t->g.sin.flags = h.flags;
  t->g.sin.contents = h.contents;
  t->g.sin.value = h.value;
  t->g.sin.direct = h.direct;
  t->g.sin.animtime = h.animtime;
  t->g.sin.nonlit = h.nonlit;
  t->g.sin.directangle = h.directangle;
  t->g.sin.trans_angle = h.trans_angle;
  t->g.sin.directstyle[0] = 0;
  if (h.directstyle)
  {
    printf("Hmm! h.directstyle=%g\n", h.directstyle);
  }
  t->g.sin.translucence = h.translucence;
  t->g.sin.friction = h.friction;
  t->g.sin.restitution = h.restitution;
  t->g.sin.trans_mag = h.trans_mag;
  t->g.sin.color[0] = h.color[0];
  t->g.sin.color[1] = h.color[1];
  t->g.sin.color[2] = h.color[2];

  size = t->dsx * t->dsy;
  t->data = Q_malloc(size);
  if (!t->data)
  {
    if (closefile)
      fclose(f);
    return 0;
  }
  fseek(f, h.offsets[0] + baseofs, SEEK_SET);
  fread(t->data, 1, t->dsx * t->dsy, f);

  for (c = t->data, i = size; i; i--, c++)
  {
    j = *c * 4;
    *c = Dith(h.palette[j + 0] >> 2, h.palette[j + 1] >> 2, h.palette[j + 2] >> 2);
  }

  t->color = -1;

  if (closefile)
    fclose(f);

  tn->tex = t;

  return 1;
}

static void
AddTName(char* name, int nl, int ofs)
{
  tex_name_t* t;
  int i;

  for (i = 0; i < n_tnames; i++)
  {
    if (!stricmp(tnames[i].filename, name))
      return;
  }

  tnames = Q_realloc(tnames, sizeof(tex_name_t) * (n_tnames + 1));
  if (!tnames)
    Abort("Sin_Init", "Out of memory!");
  t = &tnames[n_tnames];
  memset(t, 0, sizeof(tex_name_t));
  n_tnames++;

  strcpy(t->filename, name);
  strcpy(t->name, &name[strlen("textures/")]);
  t->name[strlen(t->name) - 4] = 0;

  t->location = nl;
  t->ofs = ofs;
}

typedef struct
{
  const char* name;
  int field; // contents (0) or flags (1)
  int value;
} ti_flag_t;

static ti_flag_t ti_flags[] =
  {
    {"solid", 0, 0},
    {"window", 0, 1},
    {"fence", 0, 2},
    {"lava", 0, 3},
    {"slime", 0, 4},
    {"water", 0, 5},
    {"mist", 0, 6},

    {"playerclip", 0, 16}, // 16
    {"monsterclip", 0, 17},
    {"current_0", 0, 18},
    {"current_90", 0, 19},
    {"current_180", 0, 20},
    {"current_270", 0, 21},
    {"current_up", 0, 22},
    {"current_dn", 0, 23},
    {"origin", 0, 24},
    {"monster", 0, 25},
    {"corpse", 0, 26},
    {"detail", 0, 27},
    {"translucent", 0, 28},
    {"ladder", 0, 29},

    {"light", 1, 0},
    {"masked", 1, 1},
    {"sky", 1, 2},
    {"warping", 1, 3},
    {"nonlit", 1, 4},
    {"nofilter", 1, 5},
    {"conveyor", 1, 6},
    {"nodraw", 1, 7},
    {"hint", 1, 8},
    {"skip", 1, 9},
    {"wavy", 1, 10},
    {"ricochet", 1, 11},
    {"prelit", 1, 12},
    {"mirror", 1, 13},
    {"console", 1, 14},
    {"usecolor", 1, 15},
    {"hardwareonly", 1, 16},
    {"damage", 1, 17},
    {"weak", 1, 18},
    {"normal", 1, 19},
    {"add", 1, 20}, // add-blend
    {"envmapped", 1, 21},
    {"random", 1, 22},
    {"animate", 1, 23},
    {"rndtime", 1, 24},
    {"translate", 1, 25},
    {"nomerge", 1, 26},
    {"surfbit0", 1, 27},
    {"surfbit1", 1, 28},
    {"surfbit2", 1, 29},
    {"surfbit3", 1, 30}};
#define NUM_TI_FLAGS (sizeof(ti_flags) / sizeof(ti_flags[0]))

typedef struct
{
  const char* name;
  int ofs_tex;
  int ofs_td;
} ti_field_t;

#define OFS(struct, field) ((int)&(((struct*)0)->field))

static ti_field_t ti_fields[] =
  {
    {"friction", OFS(texture_t, g.sin.friction), OFS(texdef_t, g.sin.friction)},
    {"restitution", OFS(texture_t, g.sin.restitution), OFS(texdef_t, g.sin.restitution)},
    {"direct", OFS(texture_t, g.sin.direct), OFS(texdef_t, g.sin.direct)},
    {"directangle", OFS(texture_t, g.sin.directangle), OFS(texdef_t, g.sin.directangle)},
    {"translucence", OFS(texture_t, g.sin.translucence), OFS(texdef_t, g.sin.translucence)},
    {"trans_mag", OFS(texture_t, g.sin.trans_mag), OFS(texdef_t, g.sin.trans_mag)},
    {"trans_angle", OFS(texture_t, g.sin.trans_angle), OFS(texdef_t, g.sin.trans_angle)},
    {"nonlitvalue", OFS(texture_t, g.sin.nonlit), OFS(texdef_t, g.sin.nonlit)}};
#define NUM_TI_FIELDS (sizeof(ti_fields) / sizeof(ti_fields[0]))

/*
directstyle test1
color 0.50 0.50 0.50
lightvalue 100
*/

static int
Sin_LoadTexInfo(texdef_t* td)
{
  int i, j;
  texture_t* texture;
  int plus;

  i = QMap_LoadTexInfo(td);
  if (i)
    return i;

  texture = ReadMIPTex(td->name, 0);
  if (texture)
  {
    td->g.sin = texture->g.sin;
  }
  else
  {
    memset(&td->g.sin, 0, sizeof(td->g.sin));
  }

  while (TokenAvailable(0))
  {
    GETTOKEN(0, T_NAME | T_MISC);

    if (!strcmp(token, "+") || !strcmp(token, "-"))
    {
      plus = !strcmp(token, "+");
      GETTOKEN(0, T_NAME | T_MISC);

      for (i = 0; i < NUM_TI_FLAGS; i++)
      {
        if (!strcmp(ti_flags[i].name, token))
          break;
      }
      if (i == NUM_TI_FLAGS)
        return ERROR_PARSE;

      j = 1 << ti_flags[i].value;
      /*         printf("'%s' %i %i %i\n",
                  ti_flags[i].name,ti_flags[i].field,
                  ti_flags[i].value,j);*/
      if (plus)
      {
        if (ti_flags[i].field)
          td->g.sin.flags |= j;
        else
          td->g.sin.contents |= j;
      }
      else
      {
        if (ti_flags[i].field)
          td->g.sin.flags &= ~j;
        else
          td->g.sin.contents &= ~j;
      }
      continue;
    }

    for (i = 0; i < NUM_TI_FIELDS; i++)
    {
      if (!strcmp(token, ti_fields[i].name))
        break;
    }

    if (i != NUM_TI_FIELDS)
    {
      GETTOKEN(0, T_NUMBER);
      *(float*)((int)td + ti_fields[i].ofs_td) = atof(token);
      continue;
    }

    if (!strcmp(token, "color"))
    {
      GETTOKEN(0, T_NUMBER);
      td->g.sin.color[0] = atof(token);
      GETTOKEN(0, T_NUMBER);
      td->g.sin.color[1] = atof(token);
      GETTOKEN(0, T_NUMBER);
      td->g.sin.color[2] = atof(token);
    }
    if (!strcmp(token, "lightvalue"))
    {
      GETTOKEN(0, T_NUMBER);
      td->g.sin.value = atoi(token);
    }
    if (!strcmp(token, "directstyle"))
    {
      GETTOKEN(0, T_ALLNAME);
      strcpy(td->g.sin.directstyle, token);
    }
  }

  return ERROR_NO;
}

static void
Sin_SaveTexInfo(texdef_t* td, FILE* fp)
{
  int i, j;
  texture_t* text;

  int cont, flag;
  int* f;

  float *f1, *f2;

  QMap_SaveTexInfo(td, fp);

  text = ReadMIPTex(td->name, 0);
  if (text)
  {
    cont = text->g.sin.contents ^ td->g.sin.contents;
    flag = text->g.sin.flags ^ td->g.sin.flags;
  }
  else
  {
    cont = td->g.sin.contents;
    flag = td->g.sin.flags;
  }

  for (i = 0; i < NUM_TI_FLAGS; i++)
  {
    j = 1 << ti_flags[i].value;
    if (ti_flags[i].field)
    {
      if (!(flag & j))
        continue;
      f = &td->g.sin.flags;
    }
    else
    {
      if (!(cont & j))
        continue;
      f = &td->g.sin.contents;
    }

    if (*f & j)
      fprintf(fp, " +%s", ti_flags[i].name);
    else
      fprintf(fp, " -%s", ti_flags[i].name);
  }

  for (i = 0; i < NUM_TI_FIELDS; i++)
  {
    f2 = (float*)((int)td + ti_fields[i].ofs_td);
    if (text)
    {
      f1 = (float*)((int)text + ti_fields[i].ofs_tex);
      if (*f1 != *f2)
        j = 1;
      else
        j = 0;
    }
    else
      j = 1;

    if (j)
    {
      fprintf(fp, " %s %g", ti_fields[i].name, *f2);
    }
  }

  if (text)
  {
    if ((fabs(td->g.sin.color[0] - text->g.sin.color[0]) > 0.05) ||
        (fabs(td->g.sin.color[1] - text->g.sin.color[1]) > 0.05) ||
        (fabs(td->g.sin.color[2] - text->g.sin.color[2]) > 0.05))
    {
      j = 1;
    }
    else
      j = 0;
  }
  else
    j = 1;

  if (j)
  {
    fprintf(fp, " color %g %g %g", td->g.sin.color[0], td->g.sin.color[1], td->g.sin.color[2]);
  }

  if (text)
  {
    if (td->g.sin.value != text->g.sin.value)
      j = 1;
    else
      j = 0;
  }
  else
    j = 1;

  if (j)
  {
    fprintf(fp, " lightvalue %i", td->g.sin.value);
  }

  if (td->g.sin.directstyle[0])
    fprintf(fp, " directstyle %s", td->g.sin.directstyle);
}

static game_t Game_Sin =
  {
    "Sin",
    {
      1,
      0,

      Sin_TexSort,
      NULL,
      Sin_LoadTexture,
      NULL,
      Sin_FlagsDefault,
      Sin_ModifyFlags,
      NULL, // TODO
      NULL  // TODO
    },
    {QMap_Load,
     QMap_Save,
     QMap_SaveVisible,
     QMap_LoadGroup,
     QMap_SaveGroup,
     QMap_Profile},
    {L_QUAKE2,
     1,
     0},
    {1},
    ".lin"};

void
Sin_Init(void)
{
  Game = Game_Sin;
  qmap_loadtexinfo = Sin_LoadTexInfo;
  qmap_savetexinfo = Sin_SaveTexInfo;

  {
    int i;
    int r, g, b;
    unsigned char tex_pal[768];

    for (i = 0; i < 256; i++)
    {
      r = ((i >> 2) & 7) * 8;
      g = ((i >> 5) & 7) * 8;
      b = ((i >> 0) & 3) * 16;

      tex_pal[i * 3 + 0] = r;
      tex_pal[i * 3 + 1] = g;
      tex_pal[i * 3 + 2] = b;
    }
    SetTexturePal(tex_pal);
  }

  if (!PD_Init())
  {
    PD_Search("textures/", ".swl", AddTName);
    PD_Write();
  }
}
