/*
tex_all.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef TEX_ALL_H
#define TEX_ALL_H


/* tnames and locs are managed by texture system specific code, the other
code only looks at tex_name_t.name and tex_name_t.tex, everything else can
be used for anything by the texture system code. */

typedef struct
{
   char name[128];
   int type;
} location_t;

typedef struct tex_name_s
{
   char name[64];
   char filename[64];
   int location;
   int ofs;
   texture_t *tex;

   /* Field for game-specific information (needed for Quake 3) */
   void *x;
} tex_name_t;

extern location_t *locs;
extern int n_locs;

extern tex_name_t *tnames;
extern int n_tnames;

extern struct category_s *tcategories;
extern int n_tcategories;


#endif

