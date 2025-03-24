/*
game.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef GAME_H
#define GAME_H

// To make sure their scope is outside the function definitions'.
struct texture_s;
struct tex_name_s;

typedef struct game_s
{
  const char* name;

  struct
  {
    int cache;
    int fixdups;

    //      int (*sort)(const struct texture_s *t1,const struct texture_s *t2);
    int (*sort)(const char* t1, const char* t2);
    // only when !Game.tex.cache, the textures are loaded by loadtexture otherwise
    int (*readcache)(int verbose);
    // only when Game.tex.cache, the textures are loaded by readcache otherwise
    int (*loadtexture)(struct tex_name_s* tn);
    int (*gettexturecategory)(char* name);
    void (*settexdefdefault)(texdef_t* tex);
    void (*settexdef)(void);

    int (*gettexbspflags)(texdef_t* t);
    void (*gettexdesc)(char* dest, const struct texture_s* t);
  } tex;

  struct
  {
    int (*loadmap)(const char* filename);

    int (*savemap)(const char* filename);
    int (*savevisiblemap)(const char* filename, int add_shell);

    int (*loadgroup)(const char* filename);
    int (*savegroup)(const char* filename, group_t* g);

    void (*mapprofile)(const char* name, char* profile);
  } map;

  struct
  {
    int model;
    int lfaces;
    int fullbright; // color>=240 are fullbright
  } light;

  struct
  {
    int qdiff; // automatically add !easy, !medium, etc.
  } entity;

  const char* pts_ext;
} game_t;

/*** Values returned by gettexbspflags() ***/
#define TEX_FULLBRIGHT 1  // in lighted modes, face is fullbright
#define TEX_NODRAW 2      // shouldn't be drawn at all
#define TEX_DOUBLESIDED 4 // shouldn't be backface culled
#define TEX_NONSOLID 8    // not solid, doesn't prevent leaks, for bspleak.c

/*** Light models ***/
#define L_QUAKE 0
#define L_QUAKE2 1

// Everything should look at this structure to decide how to act.
extern game_t Game;

void InitGame(void);

#endif
