/*
edit.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef EDIT_H
#define EDIT_H

void SelectWindow(void);

void ForEachSelTexdef(void (*func)(texdef_t *p));


void Solid_SelectBrushes(struct brush_s ***list,int *num);
void Solid_SelectFaces(fsel_t **list,int *num);

#endif

