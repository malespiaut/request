/*
tex_q.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "tex.h"
#include "tex_all.h"

#include "q_texi.h"
#include "quake.h"

#include "qmap.h"

#include "bsp.h"
#include "color.h"
#include "entity.h"
#include "error.h"
#include "file.h"
#include "game.h"
#include "memory.h"
#include "quest.h"
#include "token.h"
#include "video.h"

typedef struct
{
  char name[16];        /* Name of the texture */
  unsigned int width;   /* Width of texture, multiple of 8 */
  unsigned int height;  /* Height of texture, multiple of 8 */
  unsigned int offset1; /* Offset to 1 x 1 data */
  unsigned int offset2; /* Offset to 1/2 x 1/2 data */
  unsigned int offset4; /* Offset to 1/4 x 1/4 data */
  unsigned int offset8; /* Offset to 1/8 x 1/8 data */
} miptex_t;

typedef struct
{
  char magic[4];  /* "WAD2", Name of the new WAD format */
  int numentries; /* Number of entries */
  int diroffset;  /* Position of WAD directory in file */
} wadhead_t;

typedef struct
{
  int offset;    /* Position of the entry in WAD */
  int dsize;     /* Size of the entry in WAD file */
  int size;      /* Size of the entry in memory */
  char type;     /* type of entry */
  char cmprs;    /* Compression. 0 if none. */
  short dummy;   /* Not used */
  char name[16]; /* 1 to 16 characters, '\0'-padded */
} wadentry_t;

// 0x40 = Color Palette
// 0x42 = Pictures for status bar
// 0x44 = Mip Texture
// 0x45 = Console picture (flat)

static int
Q_TexSort(const char* t1, const char* t2)
{
  return stricmp(t1, t2);
}

static int
Q_ReadCache(int verbose)
{
  FILE* f;
  int i;
  int flag;
  texture_t temp;
  int found_pal;

  wadhead_t head;
  wadentry_t* entry;
  miptex_t miptex;

  unsigned char tex_pal[768];

  const unsigned char* wadfilename;

  /*
   TODO : this won't work if called when a map from the command line is being
   loaded, as video.pal is still blank then
  */
  SetTexturePal(video.pal);

  ClearCache();

  //   if (!IsGoodWad(M.wadfilename,verbose))
  //      return 0;

  wadfilename = GetKeyValue(M.WorldSpawn, "wad");
  if (!wadfilename)
  {
    if (verbose)
      HandleError("Q_ReadCache", "No wad file defined in 'worldspawn'!");
    return 0;
  }

  f = fopen(wadfilename, "rb");
  if (!f)
  {
    if (verbose)
      HandleError("Q_ReadCache", "Can't open '%s'!", wadfilename);
    return 0;
  }

  fread(&head, sizeof(wadhead_t), 1, f);

  if ((head.magic[0] != 'W') ||
      (head.magic[1] != 'A') ||
      (head.magic[2] != 'D') ||
      (head.magic[3] != '2'))
  {
    fclose(f);
    return 0;
  }

  fseek(f, head.diroffset, SEEK_SET);

  entry = (wadentry_t*)Q_malloc(sizeof(wadentry_t) * head.numentries);
  if (!entry)
  {
    if (verbose)
      HandleError("ReadCache", "Out of memory!");
    fclose(f);
    return 0;
  }
  fread(entry, sizeof(wadentry_t) * head.numentries, 1, f);

  M.num_textures = 0;

  found_pal = 0;
  for (i = 0; i < head.numentries; i++)
  {
    switch (entry[i].type)
    {
      case 0x40:
        fseek(f, entry[i].offset, SEEK_SET);
        fread(tex_pal, sizeof(tex_pal), 1, f);
        {
          int j;
          for (j = 0; j < sizeof(tex_pal); j++)
            tex_pal[j] >>= 2;
        }
        SetTexturePal(tex_pal);
        found_pal = 1;
        break;
      case 0x44:
        M.num_textures++;
        break;
    }
  }

  if (!found_pal)
  {
    FILE* f2;
    char buf[512];

    FindFile(buf, "quake.pal");

    f2 = fopen(buf, "rb");
    if (!f2)
    {
      HandleError("ReadCache", "No palette in .wad file, and couldn't open quake.pal");
    }
    else
    {
      fread(tex_pal, sizeof(tex_pal), 1, f2);
      for (i = 0; i < sizeof(tex_pal); i++)
        tex_pal[i] >>= 2;
      SetTexturePal(tex_pal);
      fclose(f2);
    }
  }

  M.Cache = (texture_t*)Q_malloc(sizeof(texture_t) * M.num_textures);
  if (!M.Cache)
  {
    if (verbose)
      HandleError("Q_ReadCache", "Out of memory!");
    fclose(f);
    return 0;
  }

  M.num_textures = 0;
  for (i = 0; i < head.numentries; i++)
  {
    if (entry[i].type == 0x44)
    {
      fseek(f, entry[i].offset, SEEK_SET);
      fread(&miptex, sizeof(miptex_t), 1, f);

      M.Cache[M.num_textures].data = (char*)Q_malloc(miptex.width * miptex.height);
      M.Cache[M.num_textures].color = -1;
      if (M.Cache[M.num_textures].data == NULL)
      {
        for (i = 0; i < M.num_textures; i++)
        {
          Q_free(M.Cache[i].data);
        }
        Q_free(M.Cache);

        Q_free(entry);
        fclose(f);
        return 0;
      }
      M.Cache[M.num_textures].dsx = M.Cache[M.num_textures].rsx = miptex.width;
      M.Cache[M.num_textures].dsy = M.Cache[M.num_textures].rsy = miptex.height;
      strcpy(M.Cache[M.num_textures].name, entry[i].name);
      fseek(f, entry[i].offset + miptex.offset1, SEEK_SET);
      fread(M.Cache[M.num_textures].data, miptex.width * miptex.height, 1, f);

      M.num_textures++;
    }
  }

  fclose(f);
  Q_free(entry);

  do
  {
    flag = 1;
    for (i = 0; i < M.num_textures - 1; i++)
    {
      if (Q_TexSort(M.Cache[i].name, M.Cache[i + 1].name) > 0)
      {
        temp = M.Cache[i];
        M.Cache[i] = M.Cache[i + 1];
        M.Cache[i + 1] = temp;
        flag = 0;
      }
    }
  } while (!flag);

  return 1;
}

static void
Q_FlagsDefault(texdef_t* t)
{
  t->g.q.detail = 0;
}

static int
Q_TexBSPFlags(texdef_t* t)
{
  if (t->name[0] == '*')
    return TEX_FULLBRIGHT | TEX_NONSOLID;
  if (!strnicmp(t->name, "SKY", 3))
    return TEX_FULLBRIGHT;
  if (!stricmp(t->name, "TRIGGER"))
    return TEX_NODRAW;
  if (!stricmp(t->name, "CLIP"))
    return TEX_NODRAW | TEX_NONSOLID;
  return 0;
}

static game_t Game_Quake =
  {
    "Quake",
    {
      0,
      0,

      Q_TexSort,
      Q_ReadCache,
      NULL,
      NULL,
      Q_FlagsDefault,
      Q_ModifyFlags,
      Q_TexBSPFlags,
      NULL /* No texture description necessary */
    },
    {QMap_Load,
     QMap_Save,
     QMap_SaveVisible,
     QMap_LoadGroup,
     QMap_SaveGroup,
     QMap_Profile},
    {L_QUAKE,
     0,
     1},
    {1},
    ".pts"};

static int
Q_LoadTexInfo(texdef_t* t)
{
  int i;

  i = QMap_LoadTexInfo(t);
  if (i)
    return i;

  t->g.q.detail = 0;

  while (TokenAvailable(0))
  {
    GETTOKEN(0, T_NAME);
    if (!strcmp(token, "detail"))
      t->g.q.detail = 1;
    else
      return ERROR_PARSE;
  }

  return ERROR_NO;
}

static void
Q_SaveTexInfo(texdef_t* t, FILE* fp)
{
  QMap_SaveTexInfo(t, fp);
  if (t->g.q.detail)
    fprintf(fp, " detail");
}

void
Q_Init(void)
{
  Game = Game_Quake;
  qmap_loadtexinfo = Q_LoadTexInfo;
  qmap_savetexinfo = Q_SaveTexInfo;

  /* Set it to video.pal here so it isn't blank, Q_ReadCache will fill
  it in properly. */
  SetTexturePal(video.pal);
}

/*
 Quake-I

Like Quake, but load textures from individual .mip files. Cutting here
should remove everything that has to do with it.

TODO : think real hard about how this should work

*/
#if 1

#include "pakdir.h"

static void
QI_AddTName(char* name, int nl, int ofs)
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
    Abort("Q_I_Init", "Out of memory!");
  t = &tnames[n_tnames];
  memset(t, 0, sizeof(tex_name_t));
  n_tnames++;

  strcpy(t->filename, name);

  t->location = nl;
  t->ofs = ofs;
}

static int
Q_I_LoadTexture(tex_name_t* tn)
{
  FILE* f;
  int baseofs;
  int closefile;

  miptex_t h;
  texture_t* t;

  if (tn->tex)
    return 1;

  f = PD_Load(tn, &baseofs, &closefile);
  if (!f)
    return 0;

  t = Q_malloc(sizeof(texture_t));
  if (!t)
  {
    if (closefile)
      fclose(f);
    return 0;
  }
  memset(t, 0, sizeof(texture_t));

  fseek(f, baseofs, SEEK_SET);
  fread(&h, 1, sizeof(h), f);

  strcpy(t->name, tn->name);

  t->rsx = t->dsx = h.width;
  t->rsy = t->dsy = h.height;

  t->data = Q_malloc(t->dsx * t->dsy);
  if (!t->data)
  {
    if (closefile)
      fclose(f);
    return 0;
  }
  fseek(f, h.offset1 + baseofs, SEEK_SET);
  fread(t->data, 1, t->dsx * t->dsy, f);
  t->color = -1;

  if (closefile)
    fclose(f);

  tn->tex = t;

  return 1;
}

static game_t Game_Quake_I =
  {
    "Quake-I",
    {
      1,
      0,

      Q_TexSort,
      NULL,
      Q_I_LoadTexture,
      NULL,
      Q_FlagsDefault,
      Q_ModifyFlags,
      Q_TexBSPFlags,
      NULL /* No texture description necessary */
    },
    {QMap_Load,
     QMap_Save,
     QMap_SaveVisible,
     QMap_LoadGroup,
     QMap_SaveGroup,
     QMap_Profile},
    {L_QUAKE,
     0,
     1},
    {1},
    ".pts"};

void
Q_I_Init(void)
{
  Game = Game_Quake_I;
  qmap_loadtexinfo = Q_LoadTexInfo;
  qmap_savetexinfo = Q_SaveTexInfo;

  /* Set it to video.pal here so it isn't blank, Q_ReadCache will fill
  it in properly. */
  SetTexturePal(video.pal);

  {
    FILE* f;
    unsigned char buf[768];
    int i;

    FindFile(buf, "quake.pal");
    f = fopen(buf, "rb");
    if (!f)
      Abort("Q_I_Init", "Can't load quake.pal!");
    fread(buf, 1, 768, f);
    fclose(f);
    for (i = 0; i < 768; i++)
      buf[i] >>= 2;
    SetTexturePal(buf);
  }

  if (!PD_Init())
  {
    PD_Search("", ".mip", QI_AddTName);
    PD_Write();
  }

  {
    int i;
    tex_name_t* t;
    FILE* f;
    int baseofs, closefile;

    for (t = tnames, i = n_tnames; i; i--, t++)
    {
      f = PD_Load(t, &baseofs, &closefile);
      if (!f)
        continue;

      fseek(f, baseofs, SEEK_SET);
      fread(t->name, 1, 16, f);
      t->name[16] = 0;

      if (closefile)
        fclose(f);
    }
  }
}
#endif
