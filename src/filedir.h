/*
filedir.h file of the Quest Source Code

Copyright 1998, 1998 Alexander Malmberg

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef FILEDIR_H
#define FILEDIR_H


typedef struct
{
   char name[256];
   int type;
} filedir_t;

#define FILE_NORMAL 1    // a normal file
#define FILE_DIREC  2    // a directory


struct directory_s *DirOpen(const char *name,int type);
// Returns NULL on error.
//  name   wildcard w/w/o directory (eg. '*.c' or 'maps/*.map')
//  type   type(s) of files you want to find (eg. FILE_NORMAL|FILE_DIREC)

int DirRead(struct directory_s *d,filedir_t *f);
/*
 Returns 0 if no more files.
*/

void DirClose(struct directory_s *d);

#endif

