/*
menu.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef MENU_H
#define MENU_H

typedef struct
{
   int   color;
   char  title[24];
   int   parent;
   int   numkids;
   int   next;
   int   flag;
   int   showkids;
   int   command;
} menu_t;

typedef struct
{
   char title[24];
   int   menu_num;
   int   depth;
} list_t;

extern menu_t *Menus;
extern int   NumMenus;

extern list_t *List;
extern int   ListSize;

void PopUpMenuWin(void);

void PopDownMenuWin(void);

void MenuToList(menu_t *M, list_t **L, int location, int indent, int *ListSize);

void DepthSort(list_t **L, int ListSize);

void DisplayList(menu_t *M, list_t *L, int ListSize);

void EraseList(menu_t *M, list_t *L, int ListSize);

int UpdateMenu(menu_t **M, list_t **L, int *ListSize);

int LoadMenu(const char *filename, menu_t **M, int *NumMenus, list_t **L, int *ListSize);

void Menu_Exe(int command);

void WriteMenuHelp(void);

void MenuPopUp(void);

#endif

