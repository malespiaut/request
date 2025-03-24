/*
tex.c file of the Quest Source Code

Copyright 1997, 1998, 1999 Alexander Malmberg
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

#include "texcat.h"

#include "brush.h"
#include "color.h"
#include "entclass.h"
#include "error.h"
#include "game.h"
#include "memory.h"
#include "quest.h"
#include "video.h"

location_t* locs;
int n_locs;

tex_name_t* tnames;
int n_tnames;

category_t* tcategories;
int n_tcategories;

/*** General support stuff ***/

void
DrawTexture(texture_t* t, int x, int y)
{
  int i;
  unsigned char* alias;

  alias = &video.ScreenBuffer[x + y * video.ScreenWidth];

  for (i = 0; i < t->dsy; i++)
  {
    memcpy(alias, &t->data[i * t->dsx], t->dsx);
    alias += video.ScreenWidth;
  }
}

int
GetTexColor(texture_t* tex)
{
  int i, j;
  float r, g, b;

  if (tex->color != -1)
    return 0;

  r = g = b = 0;
  for (i = tex->dsx * tex->dsy - 1; i >= 0; i--)
  {
    j = (unsigned char)(tex->data[i]) * 3;
    r += (float)texture_pal[j + 0] / 64;
    g += (float)texture_pal[j + 1] / 64;
    b += (float)texture_pal[j + 2] / 64;
  }
  i = tex->dsx * tex->dsy;
  r /= i;
  g /= i;
  b /= i;
  tex->color = AddColor(r, g, b, 0);

  tex->colv[0] = r;
  tex->colv[1] = g;
  tex->colv[2] = b;

  return 1;
}

/*** Interface with tname_t stuff ***/

static tex_name_t*
LoadTN(char* name)
{
  int i;
  tex_name_t* tn;

  for (i = 0; i < n_tnames; i++)
  {
    if (!stricmp(name, tnames[i].name))
      break;
  }
  if (i == n_tnames)
    return NULL; // should never happen

  tn = &tnames[i];

  if (tn->tex)
    return tn;

  if (Game.tex.loadtexture(tn))
    return tn;
  else
    return NULL;
}

int
LoadTexture(char* name, texture_t* res)
{
  tex_name_t* tn;

  tn = LoadTN(name);
  if (tn)
  {
    *res = *tn->tex;
    return 1;
  }
  return 0;
}

void
GetTNames(int* num, char*** names)
{
  int i;

  *num = 0;
  if (!Game.tex.cache)
  {
    *names = NULL;
    return;
  }

  *names = Q_malloc(sizeof(char*) * n_tnames);
  if (!*names)
    return;

  for (i = 0; i < n_tnames; i++)
  {
    // TODO: this can probably be removed now
    if (tnames[i].name[strlen(tnames[i].name) - 1] == '/')
      continue;

    (*names)[(*num)++] = tnames[i].name;
  }
}

/*** Add/remove from cache ***/

static int
AddTex(char* tname, int verbose)
{
  int i, j;
  texture_t* t;
  texture_t temp;
  texture_t* nc;

  if (FindTexture(tname) != -1)
    return 1;
  t = &temp;
  if (!LoadTexture(tname, t))
    return 0;

  nc = Q_realloc(M.Cache, (M.num_textures + 1) * sizeof(texture_t));
  if (!nc)
    return 0;
  M.Cache = nc;

  if (M.num_textures)
  {
    for (i = 0; i < M.num_textures; i++) // insertion sort to keep everything
    {                                    // sorted while loading dynamically
      if (Game.tex.sort(M.Cache[i].name, t->name) > 0)
      {
        for (j = M.num_textures; j > i; j--)
        {
          M.Cache[j] = M.Cache[j - 1];
        }
        M.Cache[i] = temp;

        break;
      }
    }
    if (i == M.num_textures)
    {
      M.Cache[M.num_textures] = temp;
    }
  }
  else
  {
    M.Cache[0] = temp;
  }

  M.num_textures++;

  return 1;
}

void
RemoveTex(char* name)
{
  int i;
  int j;
  texture_t* NCache;

  if (!Game.tex.cache)
    return;

  if (!CheckCache(0))
    return;

  i = FindTexture(name);
  if (i == -1)
    return;

  for (j = i; j < M.num_textures - 1; j++)
    memcpy(&M.Cache[j], &M.Cache[j + 1], sizeof(texture_t));

  M.num_textures--;
  NCache = Q_realloc(M.Cache, M.num_textures * sizeof(texture_t));
  if (NCache)
    M.Cache = NCache;
}

/*** Texture loading ***/
static int
FindTextureSlow(char* name)
{
  int i, j;
  char* n;
  texture_t* t;

  n = name;

  for (i = M.num_textures, j = 0, t = M.Cache; i; i--, j++, t++)
  {
    if (!stricmp(t->name, n))
      return j;
  }
  return -1;
}

int
FindTexture(char* name)
{
  int i, j;
  char* n;
  int n1;
  texture_t* t;

  if (!name[0] || !name[1] || !name[2])
    return FindTextureSlow(name);

  n = name;

  n1 = *((int*)(n));

  for (i = M.num_textures, j = 0, t = M.Cache; i; i--, j++, t++)
  {
    if (n1 == *((unsigned int*)(t->name)))
    {
      if (!stricmp(t->name, n))
        return j;
    }
  }
  return FindTextureSlow(name);
}

texture_t*
ReadMIPTex(char* mipname, int verbose)
{
  int i;

  if (!CheckCache(verbose))
    return NULL;

  i = FindTexture(mipname);
  if ((i == -1) && (Game.tex.cache))
  {
    if (!AddTex(mipname, verbose))
      return NULL;

    i = FindTexture(mipname);
  }
  if (i == -1)
    return NULL;

  return &M.Cache[i];
}

/*** Cache management ***/

int
ReadCache(int verbose)
{
  if (!Game.tex.cache)
    return Game.tex.readcache(verbose);

  {
    brush_t* b;
    int i;
    int all;
    /* Keep track of last failed texture to try to speed up case of no
    valid texture in the map */
    char fail[128];

    ClearCache();

    all = 1;
    fail[0] = 0;
    for (b = M.BrushHead; b; b = b->Next)
    {
      if (b->bt->flags & BR_F_BTEXDEF)
      {
        if (stricmp(b->tex.name, fail) &&
            !AddTex(b->tex.name, verbose))
        {
          all = 0;
          strcpy(fail, b->tex.name);
        }
      }
      else
      {
        for (i = 0; i < b->num_planes; i++)
          if (stricmp(b->plane[i].tex.name, fail) &&
              !AddTex(b->plane[i].tex.name, verbose))
          {
            all = 0;
            strcpy(fail, b->tex.name);
          }
      }
    }
    if (!all)
      if (verbose)
        HandleError("ReadCache", "All textures couldn't be loaded!");
  }
  return 1;
}

void
ClearCache(void)
{
  int i;

  if (M.Cache)
  {
    if (!Game.tex.cache)
      for (i = 0; i < M.num_textures; i++)
        Q_free(M.Cache[i].data);
    Q_free(M.Cache);
  }

  M.Cache = NULL;
  M.num_textures = 0;
}

int
CheckCache(int verbose)
{
  if (!M.Cache)
  {
    return ReadCache(verbose);
  }
  else
  {
    return 1;
  }
}

int
GetTCat(char* name)
{
  if (!Game.tex.gettexturecategory)
    return -1;
  return Game.tex.gettexturecategory(name);
}

void
GetTexDesc(char* str, const texture_t* t)
{
  if (Game.tex.gettexdesc)
    Game.tex.gettexdesc(str, t);
  else
    *str = 0;
}
