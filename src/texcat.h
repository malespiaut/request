/*
texcat.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef TEXCAT_H
#define TEXCAT_H

typedef struct category_s
{
   char name[256];
} category_t;

extern category_t *categories;
extern int n_categories;

void InitCategories(void);

void SaveCategories(void);

int GetCategory(char *name);

void SetCategory(char *name,int cat);

void CreateCategory(char *name);

void RemoveCategory(int num);

#endif

