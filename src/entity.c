/*
entity.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "entity.h"

#include "memory.h"
#include "newgroup.h"
#include "quest.h"
#include "uid.h"

void InitEntity(entity_t *ent)
{
	ent->key = NULL;
	ent->value = NULL;

	ent->numkeys = 0;

	ent->drawn    = FALSE;

	ent->Group = FindVisGroup(M.WorldGroup);

	ent->Next = NULL;
	ent->Last = NULL;

   ent->uid=GetEntityUID();
}

const char *GetKeyValue(entity_t *ent,const char *key)
{
	int   i;
	int   found;

	/* Find key/value */
	found = FALSE;
	for (i=0; i<ent->numkeys; i++)
	{
      if (strcmp(ent->key[i], key) == 0)
      {
         found = TRUE;
			break;
		}
	}

	if (!found)
	{
		return NULL;
	}

	/* Return pointer to value */
	return ent->value[i];
}

void SetKeyValue(entity_t *ent,const char *key,const char *value)
{
	int   i;
	int   found;

	/* If setting to null, remove key */
	if (value[0] == '\0')
		return;

	/* Find key/value */
	found = FALSE;
	for (i=0; i<ent->numkeys; i++)
	{
		if (strcmp(ent->key[i], key) == 0)
		{
			found = TRUE;
			break;
		}
	}

	/* Create a new entry if it wasn't found */
	if (!found)
	{
		CreateKeyValue(ent, key, value);
		return;
	}

	/* Otherwise, resize and copy the new value in */
	ent->value[i] = Q_realloc(ent->value[i], sizeof(char) * (strlen(value) + 1));

	strcpy(ent->value[i], value);
}

void CreateKeyValue(entity_t *ent,const char *key,const char *value)
{
	/* Allocate new key/value pair */
	ent->key = (char **)Q_realloc(ent->key, sizeof(char *) * (ent->numkeys + 1));
	ent->value = (char **)Q_realloc(ent->value, sizeof(char *) * (ent->numkeys + 1));

	/* Allocate new key and value strings */
	ent->key[ent->numkeys] = (char *)Q_malloc(sizeof(char) * (strlen(key) + 1));
	ent->value[ent->numkeys] = (char *)Q_malloc(sizeof(char) * (strlen(value) + 1));

	/* Copy key/value into new structure */
	strcpy(ent->key[ent->numkeys], key);
	strcpy(ent->value[ent->numkeys], value);

	/* And increment the counter */
	ent->numkeys++;
}

void RemoveKeyValue(entity_t *ent,const char *key)
{
	int   i, j;
	int   found;

	/* Can't remove if entity is empty */
	if (ent->numkeys == 0)
	{
		return;
	}

	/* Find key/value */
	found = FALSE;
	for (i=0; i<ent->numkeys; i++)
	{
		if (strcmp(ent->key[i], key) == 0)
		{
			found = TRUE;
			break;
		}
	}

	if (!found)
	{
		return;
	}

	for (j=i; j<(ent->numkeys-1); j++)
	{
		/* Resize key/value pair */
		ent->key[j] = Q_realloc(ent->key[j], sizeof(char) * (strlen(ent->key[j+1]) + 1));
		ent->value[j] = Q_realloc(ent->value[j], sizeof(char) * (strlen(ent->value[j+1]) + 1));

		/* Copy in data */
		strcpy(ent->key[j], ent->key[j+1]);
		strcpy(ent->value[j], ent->value[j+1]);
	}

	/* Free the last entry */
	Q_free(ent->key[ent->numkeys-1]);
	Q_free(ent->value[ent->numkeys-1]);

	/* Either size down the key/value list or free it completely */
	if (ent->numkeys > 1)
	{
		ent->key = (char **)Q_realloc(ent->key, sizeof(char *) * (ent->numkeys - 1));
		ent->value = (char **)Q_realloc(ent->value, sizeof(char *) * (ent->numkeys - 1));
	}
	else
	{
		Q_free(ent->key);
		Q_free(ent->value);
      ent->key=ent->value=NULL;
	}

	ent->numkeys--;
}

int GetTargetNumStr(const char *v)
{
   char *bp;
   int i;

   if (v[0]!='t')
      return -1;

   i=strtol(&v[1],&bp,0);

   if (*bp)
      return -1;

   return i;
}

int GetTargetNum(entity_t *e,const char *key)
{
   const char *v;

   v=GetKeyValue(e,key);
   if (!v)
      return -1;
   return GetTargetNumStr(v);
}

int GetLastTarget(void)
{
   int last;
   entity_t *e;
   int num;

   last=-1;
   for (e=M.EntityHead;e;e=e->Next)
   {
      num=GetTargetNum(e,"target");
      if (num>last) last=num;
      num=GetTargetNum(e,"targetname");
      if (num>last) last=num;
   }

   return last;
}

int GetFreeTarget(void)
{
   int cur,next;
   int num;
   entity_t *e;

   cur=0;
start_search:
   next=cur+1;
   for (e=M.EntityHead;e;e=e->Next)
   {
      num=GetTargetNum(e,"target");
      if (num==cur)
      {
         cur=next;
         goto start_search;
      }
      if (num==next)
         next++;

      num=GetTargetNum(e,"targetname");
      if (num==cur)
      {
         cur=next;
         goto start_search;
      }
      if (num==next)
         next++;
   }

   return cur;
}

