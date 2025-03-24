/*
tex.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef TEX_H
#define TEX_H

/* texture cache handling */

typedef struct texture_s
{
  char name[64];
  int rsx, rsy; /* The real size of the texture */

  int dsx, dsy; /* The size of the texture data */
  unsigned char* data;

  int color;
  float colv[3];

  union
  {
    q2_texdef_t q2;
    sin_texdef_t sin;
  } g;
} texture_t;

void DrawTexture(texture_t* t, int x, int y);
int GetTexColor(texture_t* tex);

int FindTexture(char* name);
texture_t* ReadMIPTex(char* mipname, int verbose);

int LoadTexture(char* name, texture_t* res);
void RemoveTex(char* name);
void GetTNames(int* num, char*** names);

int GetTCat(char* name);

void GetTexDesc(char* str, const texture_t* t);

int ReadCache(int verbose);
void ClearCache(void);
int CheckCache(int verbose);

void TexFlagsDefault(texdef_t* tex);

#endif
