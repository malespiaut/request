/*
filedir.c file of the Quest Source Code

Copyright 1998, 1998 Alexander Malmberg

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "filedir.h"

#include "memory.h"


#ifdef DJGPP

#include <dir.h>
#include <errno.h>
#include <ctype.h>

typedef struct directory_s
{
   int first;
   int type;
   struct ffblk ff;
} directory_t;


struct directory_s *DirOpen(const char *name,int type)
{
   directory_t *d;
   int attr;
   int i;
   char temp[256];

   d=Q_malloc(sizeof(directory_t));
   if (!d)
      return NULL;

   if (type&FILE_NORMAL)
      attr=FA_RDONLY|FA_ARCH;
   else
      attr=0;
   if (type&FILE_DIREC)
      attr|=FA_DIREC;

   d->first=1;
   d->type=type;

   strcpy(temp,name);
   i=strlen(name);
   if ((name[i-1]=='*') &&
       ((i<2) || (name[i-2]=='/') || (name[i-2]=='\\')))
      strcat(temp,".*"); // change * to *.* for DOS

   i=findfirst(temp,&d->ff,attr);

   if (i)
      d->first=2;  // assume the error is that there are no files

   return d;
}

int DirRead(struct directory_s *d,filedir_t *f)
{
   char *c;

   if (d->first==2)
      return 0;
   while (1)
   {
      if (d->first)
         d->first=0;
      else
      {
         if (findnext(&d->ff))
         {
            d->first=2;
            return 0;
         }
      }

      if ( (d->ff.ff_attrib&FA_DIREC) && !(d->type&FILE_DIREC))
         continue;
      if (!(d->ff.ff_attrib&FA_DIREC) && !(d->type&FILE_NORMAL))
         continue;

      strcpy(f->name,d->ff.ff_name);
      if (d->ff.ff_attrib&FA_DIREC)
         f->type=FILE_DIREC;
      else
         f->type=FILE_NORMAL;

      for (c=f->name;*c;c++)
         if (toupper(*c)!=*c)
            break;
      if (!*c)
      {
         for (c=f->name;*c;c++)
            *c=tolower(*c);
      }

      return 1;
   }
}

void DirClose(struct directory_s *d)
{
   if (d->first!=2)  // clean up if there are files remaining
   {
      while (!findnext(&d->ff)) { }
   }
   Q_free(d);
}


#endif


#ifdef _UNIX

#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>

typedef struct directory_s
{
   DIR *d;
   int type;
   char wildcard[128];
   char base[1024];
} directory_t;

struct directory_s *DirOpen(const char *aname,int type)
{
   directory_t *d;
   char name[1024];

   d=Q_malloc(sizeof(directory_t));
   if (!d)
      return NULL;

   strcpy(name,aname);
   if (strrchr(name,'/'))
   {
      strcpy(d->wildcard,strrchr(name,'/')+1);
      *strrchr(name,'/')=0;
      d->d=opendir(name);
      strcpy(d->base,name);
      strcat(d->base,"/");
   }
   else
   {
      strcpy(d->wildcard,name);
      d->d=opendir(".");
      strcpy(d->base,"./");
   }
   if (!d->d)
   {
      Q_free(d);
      return NULL;
   }
   d->type=type;
   return d;
}

int DirRead(struct directory_s *d,filedir_t *f)
{
   struct dirent *de;
   struct stat s;
   char name[1024];

   while (1)
   {
      de=readdir(d->d);
      if (!de)
	      return 0;

      if (fnmatch(d->wildcard,de->d_name,0))
         continue;

      strcpy(name,d->base);
      strcat(name,de->d_name);
      stat(name,&s);

      if (S_ISDIR(s.st_mode))
	      f->type=FILE_DIREC;
      else
      if (S_ISREG(s.st_mode))
	      f->type=FILE_NORMAL;
      else
	      continue;

      if (!(f->type&d->type))
	      continue;

      strcpy(f->name,de->d_name);
      return 1;
   }
}

void DirClose(struct directory_s *d)
{
   closedir(d->d);
   Q_free(d);
}

#endif

