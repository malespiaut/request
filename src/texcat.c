/*
texcat.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "texcat.h"

#include "error.h"
#include "file.h"
#include "memory.h"
#include "token.h"

#ifdef _UNIX
#include "linux/unixext.h"
#endif


category_t *categories;
int n_categories;


typedef struct
{
   char name[64];
   int cat;
} ctex_t;

static ctex_t *ctex;
static int nctex;


typedef struct
{
   char name[32];
   int cat;
} file_ctex_t;
// format in the binary textures.cat file


static void LoadCategories(char *name)
{
   if (!TokenFile(name,T_C|T_MISC|T_STRING,NULL))
      Abort("LoadCategories","Couldn't load '%s'!",name);

   n_categories=0;
   nctex=0;

   while (TokenGet(1,-1))
   {
      categories=Q_realloc(categories,sizeof(category_t)*(n_categories+1));
      if (!categories)
         Abort("LoadCategories","Out of memory!");
      token[strlen(token)-1]=0;
      strcpy(categories[n_categories].name,&token[1]);

      if (!TokenGet(-1,T_MISC))
         Abort("LoadCategories","Unexpected end of file!");
      if (strcmp(token,"{"))
         Abort("LoadCategories","Parse error: Expected '{', got '%s'!",token);

      if (!TokenGet(-1,T_ALLNAME))
         Abort("LoadCategories","Unexpected end of file!");
      while (strcmp(token,"}"))
      {
         SetCategory(token,n_categories);
         if (!TokenGet(-1,T_ALLNAME))
            Abort("LoadCategories","Unexpected end of file!");
      }
      n_categories++;
   }

   TokenDone();
}


void InitCategories(void)
{
   FILE *f;
   char name[256];
   char buf[4];

   FindFile(name,"textures.cat");
   f=fopen(name,"rb");
   if (!f)
   {
      n_categories=0;
      categories=NULL;
      return;
   }


/*
If it's the old binary format, it starts with an int, so at least one of the
first four bytes should be 0 (unless the user has >=16843009 categories). If
none of the first four bytes are 0, assume it's the new text format and load
that.
*/
   fread(buf,1,sizeof(buf),f);
   if (!buf[0] || !buf[1] || !buf[2] || !buf[3])
   {
      file_ctex_t *fct;
      file_ctex_t *ft;
      ctex_t *ct;
      int i;

      fseek(f,0,SEEK_SET);
      fread(&n_categories,1,sizeof(int),f);
      fread(&nctex,1,sizeof(int),f);
   
      categories=Q_malloc(sizeof(category_t)*n_categories);
      if (!categories)
      {
         n_categories=nctex=0;
         HandleError("InitCategories","Out of memory!");
         fclose(f);
         return;
      }
   
      ctex=Q_malloc(sizeof(ctex_t)*nctex);
      fct=Q_malloc(sizeof(file_ctex_t)*nctex);
      if (!ctex || !fct)
      {
         n_categories=nctex=0;
         Q_free(categories);
         categories=NULL;
         Q_free(ctex);
         Q_free(fct);
         HandleError("InitCategories","Out of memory!");
         fclose(f);
         return;
      }
   
      fread(categories,1,sizeof(category_t)*n_categories,f);
      fread(fct,1,sizeof(file_ctex_t)*nctex,f);
      for (i=0,ct=ctex,ft=fct;i<nctex;i++,ft++,ct++)
      {
         strcpy(ct->name,ft->name);
         ct->cat=ft->cat;
      }
      Q_free(fct);
      fclose(f);
   }
   else
   {
      fclose(f);
      LoadCategories(name);
   }
}

void SaveCategories(void)
{
   FILE *f;
   char name[256];
   int i,j;
   ctex_t *ct;

   if (!n_categories)
      return;

   FindFile(name,"textures.cat");
   f=fopen(name,"wb");
   if (!f)
   {
      HandleError("SaveCategories","Unable to write 'textures.cat'!");
      return;
   }

   fprintf(f,"// Quest texture category file\n\n");
   for (i=0;i<n_categories;i++)
   {
      fprintf(f,"\"%s\"\n{\n",categories[i].name);
      for (j=nctex,ct=ctex;j;j--,ct++)
         if (ct->cat==i) fprintf(f,"%s\n",ct->name);
      fprintf(f,"}\n\n");
   }

/* Old binary format:
   fwrite(&n_categories,1,sizeof(int),f);
   fwrite(&nctex,1,sizeof(int),f);

   fwrite(categories,1,sizeof(category_t)*n_categories,f);
   fwrite(ctex,1,sizeof(ctex_t)*nctex,f);*/

   fclose(f);
}


static int nearest;

static int FindCTex(char *name)
{
   char tmp[64];
   int min,max,med;
   int i,j;

   if (!nctex)
      return -1;

   strcpy(tmp,name);
   strlwr(tmp);

   min=0;
   max=nctex-1;
   j=0;
   while (min<max)
   {
      med=(min+max)/2;

      j++;
      i=strcmp(tmp,ctex[med].name);
      if (!i)
      {
         return med;
      }

      if (i<0)
         max=med-1;
      else
         min=med+1;
   }

   if (!strcmp(tmp,ctex[min].name))
      return min;

   nearest=min;

   return -1;
}


int GetCategory(char *name)
{
   int i;

   i=FindCTex(name);
   if (i==-1)
      return -1;

   return ctex[i].cat;
}

void SetCategory(char *name,int cat)
{
   int i,j;
   ctex_t *nct;

   i=FindCTex(name);
   if (i!=-1)
   {
      ctex[i].cat=cat;
      return;
   }

   if (!nctex)
   {
      ctex=Q_malloc(sizeof(ctex_t));
      if (!ctex)
      {
         HandleError("SetCategory","Out of memory!");
         return;
      }
      memset(ctex,0,sizeof(ctex_t));
      nctex=1;
      ctex[0].cat=cat;
      strcpy(ctex[0].name,name);
      strlwr(ctex[0].name);
      return;
   }

   i=stricmp(name,ctex[nearest].name);
   if (i>0)
      j=nearest+1;
   else
      j=nearest;

   nct=Q_realloc(ctex,sizeof(ctex_t)*(nctex+1));
   if (!nct)
   {
      HandleError("SetCategory","Out of memory!");
      return;
   }
   memset(&nct[nctex],0,sizeof(ctex_t));
   ctex=nct;
   nctex++;

   for (i=nctex-1;i>j;i--)
      memcpy(&ctex[i],&ctex[i-1],sizeof(ctex_t));

   ctex[j].cat=cat;
   strcpy(ctex[j].name,name);
   strlwr(ctex[j].name);
}


void CreateCategory(char *name)
{
   category_t *nc;

   nc=Q_realloc(categories,sizeof(category_t)*(n_categories+1));
   if (!nc)
   {
      HandleError("CreateCategory","Out of memory!");
      return;
   }
   memset(&nc[n_categories],0,sizeof(category_t));
   categories=nc;
   strcpy(categories[n_categories++].name,name);
}

void RemoveCategory(int num)
{
   int i;
   category_t *nc;

   n_categories--;
   for (i=num;i<n_categories;i++)
      memcpy(&categories[i],&categories[i+1],sizeof(category_t));

   nc=Q_realloc(categories,sizeof(category_t)*n_categories);
   if (nc)
      categories=nc;

   for (i=0;i<nctex;i++)
   {
      if (ctex[i].cat==num)
         ctex[i].cat=-1;
      else
      if (ctex[i].cat>num)
         ctex[i].cat--;
   }
}

