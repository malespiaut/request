/*
memory.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef MEMORY_H
#define MEMORY_H

extern int memused;

extern int maxused;

extern int Q_TotalMem;

void InitMemory(unsigned int heapsize);

void CloseMemory(void);

void *Q_malloc(int size);

void *Q_realloc(void *temp, int size);

int Q_free(void *temp);


char *Q_strdup(const char *src);

#endif

