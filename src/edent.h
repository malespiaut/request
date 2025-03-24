/*
edent.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef EDENT_H
#define EDENT_H

void HandleLeftClickEntity(void);

void SelectWindowEntity(int x0, int y0, int x1, int y1);

void MoveSelEnt(int dir, int update);

entity_t* FindEntity(int mx, int my);

int AddEntity(char* name, int x, int y, int z);

int RotateEntity(void);

void EditEntity(void);

int CopyEntity(void);

int PasteEntity(void);

void DeleteEntity(entity_t* e);

void DeleteEntities(void);

void ClearSelEnts(void);

#endif
