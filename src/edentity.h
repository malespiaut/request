/*
edentity.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef EDENTITY_H
#define EDENTITY_H

int PopAnyEntity(char *title, char **string, char **value, int number);

int GetString(char *Title, char *String);

void DumpEntity(entity_t *e);

void EditAnyEntity(int nents, entity_t **ents);

void ModelList(void);

#endif

