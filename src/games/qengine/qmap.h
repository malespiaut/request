/*
qmap.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef QMAP_H
#define QMAP_H

#define ERROR_NO 0    // no error occured, assumed to be 0
#define ERROR_NOMEM 1 // out of memory
#define ERROR_PARSE 2 // error parsing file

#define GETTOKEN(x, y) \
  if (!TokenGet(x, y)) \
    return ERROR_PARSE;

#define EXPECT(x, y, z)   \
  {                       \
    GETTOKEN(x, y)        \
    if (strcmp(token, z)) \
      return ERROR_PARSE; \
  }

extern int (*qmap_loadtexinfo)(texdef_t* tex);
extern void (*qmap_savetexinfo)(texdef_t* tex, FILE* fp);

int QMap_LoadTexInfo(texdef_t* tex);
void QMap_SaveTexInfo(texdef_t* tex, FILE* fp);

void QMap_Profile(const char* name, char* profile);

int QMap_Load(const char* filename);

int QMap_Save(const char* filename);
int QMap_SaveVisible(const char* filename, int add_shell);

int QMap_LoadGroup(const char* filename);
int QMap_SaveGroup(const char* filename, group_t* g);

#endif
