/*
entity.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef ENTITY_H
#define ENTITY_H

void InitEntity(entity_t* ent);

const char* GetKeyValue(entity_t* ent, const char* key);

void SetKeyValue(entity_t* ent, const char* key, const char* value);

void CreateKeyValue(entity_t* ent, const char* key, const char* value);

void RemoveKeyValue(entity_t* ent, const char* key);

int GetTargetNumStr(const char* v);

int GetTargetNum(entity_t* e, const char* key);

int GetLastTarget(void);

int GetFreeTarget(void);

#endif
