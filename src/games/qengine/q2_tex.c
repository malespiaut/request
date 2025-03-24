/*
tex_q2.c file of the Quest Source Code

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

#include "quake2.h"

#include "q2_tex.h"
#include "q2_texi.h"
#include "q2_texi2.h"
#include "qmap.h"

#include "brush.h"
#include "color.h"
#include "error.h"
#include "file.h"
#include "game.h"
#include "memory.h"
#include "message.h"
#include "pakdir.h"
#include "quest.h"
#include "token.h"

static const char*
NameOnly(const char* name)
{
  if (strrchr(name, '/'))
    return strrchr(name, '/') + 1;
  else
    return (const char*)name;
}

static int
Q2_TexSort(const char* t1, const char* t2)
{
  return strcmp(NameOnly(t1), NameOnly(t2));
}

typedef struct
{
  char name[32];
  unsigned int width, height;
  unsigned int offsets[4];
  char animname[32];
  int flags;
  int contents;
  int value;
} q2_miptex_t;

static int
Q2_LoadTexture(tex_name_t* tn)
{
  FILE* f;
  int baseofs;
  int closefile;

  q2_miptex_t h;
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

  //   strcpy(t->name,h.name);
  strcpy(t->name, tn->name);
  /* Use the name of the file instead of the name in .wal file. Fix for
     broken .wal file makers. */
  strlwr(t->name);

  t->rsx = t->dsx = h.width;
  t->rsy = t->dsy = h.height;
  t->g.q2.flags = h.flags;
  t->g.q2.contents = h.contents;
  t->g.q2.value = h.value;

  t->data = Q_malloc(t->dsx * t->dsy);
  if (!t->data)
  {
    if (closefile)
      fclose(f);
    return 0;
  }
  fseek(f, h.offsets[0] + baseofs, SEEK_SET);
  fread(t->data, 1, t->dsx * t->dsy, f);
  t->color = -1;

  if (closefile)
    fclose(f);

  tn->tex = t;

  return 1;
}

typedef struct
{
  int real; // which real texture to use instead
  char name[32];
} dup_t;

typedef struct
{
  char name[32];
} real_t;

static real_t* realtex;
static int nreal;

static dup_t* dups;
static int ndups;

static void
LoadDup(void)
{
  char filename[256];

  FindFile(filename, "textures.dup");

  realtex = NULL;
  dups = NULL;
  nreal = ndups = 0;

  if (!TokenFile(filename, T_ALLNAME, NULL))
    return;

  while (TokenGet(1, -1))
  {
    realtex = Q_realloc(realtex, sizeof(real_t) * (nreal + 1));
    if (!realtex)
      Abort("LoadDup", "Out of memory!");

    strcpy(realtex[nreal].name, token);

    TokenGet(0, -1);
    if (strcmp(token, "="))
      Abort("LoadDup", "'textures.dup', line %i: Parse error!", token_linenum);

    while (TokenAvailable(0))
    {
      TokenGet(0, -1);
      dups = Q_realloc(dups, sizeof(dup_t) * (ndups + 1));
      dups[ndups].real = nreal;
      strcpy(dups[ndups].name, token);
      ndups++;
    }

    nreal++;
  }

  TokenDone();
}

void
FixDups(void)
{
  int j, k;
  brush_t* b;
  plane_t* p;
  dup_t* d;
  int n;
  int r, c;
  char name[32];

  if (!Game.tex.fixdups)
    return;

  r = c = 0;
  for (b = M.BrushHead; b; b = b->Next)
  {
    if (b->bt->type != BR_NORMAL)
      continue;

    p = b->plane;
    for (j = b->num_planes; j; j--, p++)
    {
      strcpy(name, p->tex.name);
      strlwr(name);
      n = *((unsigned int*)name);

      d = dups;
      for (k = ndups; k; k--, d++)
      {
        c++;
        if (n == *((unsigned int*)d->name))
        {
          if (!strcmp(name, d->name))
          {
            r++;
            strcpy(p->tex.name, realtex[d->real].name);
            break;
          }
        }
      }
    }
  }

  NewMessage("%i (%i) textures changed.", r, c);
}

static void
AddTName(char* name, int nl, int ofs)
{
  tex_name_t* t;
  int i;
  char tname[64];

  for (i = 0; i < n_tnames; i++)
  {
    if (!stricmp(tnames[i].filename, name))
      return;
  }

  strcpy(tname, &name[strlen("textures/")]);
  tname[strlen(tname) - 4] = 0;

  for (i = 0; i < ndups; i++)
  {
    if (!stricmp(dups[i].name, tname))
    {
      return;
    }
  }

  tnames = Q_realloc(tnames, sizeof(tex_name_t) * (n_tnames + 1));
  if (!tnames)
    Abort("Q2_Init", "Out of memory!");
  t = &tnames[n_tnames];
  memset(t, 0, sizeof(tex_name_t));
  n_tnames++;

  strcpy(t->filename, name);
  strcpy(t->name, tname);
  t->location = nl;
  t->ofs = ofs;
}

int
Q2_LoadTexInfo(texdef_t* tex)
{
  int i;

  i = QMap_LoadTexInfo(tex);
  if (i)
    return i;

  if (TokenAvailable(0))
  {
    GETTOKEN(0, T_NUMBER);
    tex->g.q2.contents = atoi(token);

    GETTOKEN(0, T_NUMBER);
    tex->g.q2.flags = atoi(token);

    GETTOKEN(0, T_NUMBER);
    tex->g.q2.value = atoi(token);
  }
  else
  {
    texture_t* t;

    t = ReadMIPTex(tex->name, 0);
    if (!t)
    {
      tex->g.q2.contents = 0;
      tex->g.q2.flags = 0;
      tex->g.q2.value = 0;
    }
    else
    {
      tex->g.q2.contents = t->g.q2.contents;
      tex->g.q2.flags = t->g.q2.flags;
      tex->g.q2.value = t->g.q2.value;
    }
  }

  return ERROR_NO;
}

void
Q2_SaveTexInfo(texdef_t* tex, FILE* fp)
{
  texture_t* t;

  QMap_SaveTexInfo(tex, fp);

  t = ReadMIPTex(tex->name, 0);

  if (!t)
  {
    if (tex->g.q2.contents ||
        tex->g.q2.flags ||
        tex->g.q2.value)
    {
      fprintf(fp, " %i %i %i", tex->g.q2.contents, tex->g.q2.flags, tex->g.q2.value);
    }
  }
  else
  {
    if ((tex->g.q2.contents != t->g.q2.contents) ||
        (tex->g.q2.flags != t->g.q2.flags) ||
        (tex->g.q2.value != t->g.q2.value))
    {
      fprintf(fp, " %i %i %i", tex->g.q2.contents, tex->g.q2.flags, tex->g.q2.value);
    }
  }
}

static int
Q2_TexBSPFlags(texdef_t* t)
{
  if (t->g.q2.flags & SURF_SKY)
    return TEX_FULLBRIGHT;
  if (t->g.q2.flags & SURF_NODRAW)
    return TEX_NODRAW;
  if (t->g.q2.flags & SURF_LIGHT)
    return TEX_FULLBRIGHT;

  if (t->g.q2.flags & (SURF_WARP | SURF_TRANS33 | SURF_TRANS66))
    return TEX_FULLBRIGHT | TEX_NONSOLID;

  return 0;
}

static void
Q2_TexDesc(char* str, const texture_t* t)
{
  sprintf(str, "F=%08x C=%08x v=%i", t->g.q2.flags, t->g.q2.contents, t->g.q2.value);
}

static void
Q2_ModifyFlags(void)
{

  static const char* q2_contents[32] =
    {
      "Solid", "Window", "Aux", "Lava", // 00000001
      "Slime",
      "Water",
      "Mist",
      NULL, // 00000010
      NULL,
      NULL,
      NULL,
      NULL, // 00000100
      NULL,
      NULL,
      NULL,
      NULL, // 00001000
      "PlayerClip",
      "MonsterClip",
      "Current_0",
      "Current_90", // 00010000
      "Current_180",
      "Current_270",
      "Current_up",
      "Current_down", // 00100000
      "Origin",
      NULL,
      NULL,
      "Detail", // 01000000
      "Translucent",
      "Ladder",
      NULL,
      NULL // 10000000
    };

  static const char* q2_flags[32] =
    {
      "Light", "Slick", "Sky", "Warp", // 00000001
      "Trans33",
      "Trans66",
      "Flowing",
      "NoDraw", // 00000010
      "Hint",
      "Skip",
      NULL,
      NULL, // 00000100
      NULL,
      NULL,
      NULL,
      NULL, // 00001000
      NULL,
      NULL,
      NULL,
      NULL, // 00010000
      NULL,
      NULL,
      NULL,
      NULL, // 00100000
      NULL,
      NULL,
      NULL,
      NULL, // 01000000
      NULL,
      NULL,
      NULL,
      NULL // 10000000
    };

  Quake2Flags_ModifyFlags(q2_contents, q2_flags);
}

static game_t Game_Quake2 =
  {
    "Quake2",
    {1, 1,

     Q2_TexSort,
     NULL,
     Q2_LoadTexture,
     NULL,
     Q2_FlagsDefault,
     Q2_ModifyFlags,
     Q2_TexBSPFlags,
     Q2_TexDesc},
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
Q2_Init(void)
{
  char curloc[256];
  FILE* f;
  int ver;
  unsigned char tex_pal[768];

  Game = Game_Quake2;
  qmap_loadtexinfo = Q2_LoadTexInfo;
  qmap_savetexinfo = Q2_SaveTexInfo;

  FindFile(curloc, "q2.pal");
  f = fopen(curloc, "rb");
  if (!f)
    Abort("Q2_Init", "Can't load q2.pal!");
  fread(tex_pal, 1, 768, f);
  fclose(f);
  for (ver = 0; ver < 768; ver++)
    tex_pal[ver] >>= 2;
  SetTexturePal(tex_pal);

  LoadDup();

  if (!PD_Init())
  {
    PD_Search("textures/", ".wal", AddTName);
    PD_Write();
  }
}
