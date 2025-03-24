/*
memory.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "memory.h"

// #define NEW_MEM

/* memory stuffs */
int memused;    /*mainly used for memory debugging*/
int Q_TotalMem; /* ""*/
int maxused;

#ifndef NEW_MEM
// REGULAR MEMORY SCHEME

#define TRACK_MEM_USED

void
InitMemory(unsigned int heapsize)
{
  memused = maxused = 0;
  Q_TotalMem = -1;
}

void
CloseMemory(void)
{
  /* do nothing */
}

void*
Q_malloc(int size)
{
  void* temp;

#ifdef TRACK_MEM_USED
  temp = (void*)malloc(size + 4);
#else
  temp = (void*)malloc(size);
#endif
  if (temp == NULL)
  {
    //      printf("Warning! malloc failed for %i bytes\n",size);
    return NULL;
  }
#ifdef TRACK_MEM_USED
  memused += size;
  if (memused > maxused)
    maxused = memused;
  *(int*)temp = size;
  return temp + 4;
#else
  return temp;
#endif
}

void*
Q_realloc(void* temp, int size)
{
#ifdef TRACK_MEM_USED
  int oldsize = 0;

  if (temp != NULL)
  {
    temp -= 4;
    oldsize = *(int*)temp;
  }
  temp = (void*)realloc(temp, size + 4);
#else
  temp = (void*)realloc(temp, size);
#endif

#ifdef TRACK_MEM_USED
  if (temp != NULL)
  {
    memused -= oldsize;
    memused += size;
    if (memused > maxused)
      maxused = memused;
    *(int*)temp = size;
    return temp + 4;
  }
#endif

  return temp;
}

int
Q_free(void* temp)
{
  if (temp == NULL)
    return FALSE;

#ifdef TRACK_MEM_USED
  temp -= 4;
  memused -= *(int*)temp;
#endif
  free(temp);
  return TRUE;
}

#else
// NEW MEMORY SCHEME, currently unused

struct memory_struct_t
{
  char type; /*0 for memory free, 1 for memory used*/
  char* offset;
  unsigned int size;
  struct memory_struct_t* next;
};
typedef struct memory_struct_t memory_t;

memory_t* MemoryStart;
char* EndOfMem;

void
InitMemory(unsigned int heapsize)
{
  unsigned int truesize;

  MemoryStart = (memory_t*)malloc(sizeof(memory_t));
  MemoryStart->type = 0;
  MemoryStart->offset = (char*)malloc(heapsize);
  //	truesize = _msize(MemoryStart->offset);
  truesize = heapsize;
  if (MemoryStart->offset == NULL)
  {
    Abort("InitMemory", "Attempt to allocate %d bytes for heap failed.\n", heapsize);
  }
  printf("Allocated %d bytes for heap\n", heapsize);

  MemoryStart->size = truesize;
  MemoryStart->next = NULL;

  EndOfMem = MemoryStart->offset + truesize;
  memused = 0;
  Q_TotalMem = truesize;
}

void*
Q_malloc(int size)
{
  void* temp;
  memory_t* M;
  memory_t* N;

  /*no weird blocks*/
  if (size <= 0)
  {
    printf("Possible bug: Request to allocate a block of %d bytes\n", size);
    return NULL;
  }

  /*find the first available free block large enough*/
  for (M = MemoryStart; M; M = M->next)
  {
    if ((M->type == 0) && (M->size >= size))
      break;
  }

  /*if we couldnt find one, then we cant allocate*/
  if (M == NULL)
  {
    /*              issue a warning?*/
    /*              printf("Memory Error: Out of memory, unable to allocate request block of %d bytes\n",size);*/
    return NULL;
  }

  memused += size;

  temp = M->offset;
  M->type = 1;

  /*Is this the last block in the heap?*/
  /*If so, put another free block after it (if possible)*/
  if (M->next == NULL)
  {

    /*if this block takes up exactly how much is left, then*/
    /*we are already done*/
    if (M->size == size)
      return temp;

    /*otherwise, we must insert another free block after it*/
    N = (memory_t*)malloc(sizeof(memory_t));

    M->size = size;
    M->next = N;

    N->next = NULL;
    N->type = 0;
    N->offset = M->offset + size;
    N->size = (unsigned int)(EndOfMem) - (unsigned int)(N->offset);

    return temp;
  }

  /*the heap is allocated and freed in such a way as to prevent */
  /*having consecutive free blocks. If we have proceeded this far,*/
  /*and the next block is not a used one, then we have an unexpected error*/

  if (((M->next)->type) != 1)
  {
    Abort("Q_malloc", "Heap Error: Expecting type 1 but found type %d\n", ((M->next)->type));
  }

  /*if this block fits perfectly into the heap block, then we're done*/
  if (M->size == size)
    return temp;

  /*it does not fit perfectly, we must insert a free block in*/
  /*between the 2 used ones*/

  N = (memory_t*)malloc(sizeof(memory_t));

  N->type = 0;
  N->offset = M->offset + size;
  N->size = (unsigned int)((M->next)->offset) - (unsigned int)(N->offset);

  M->size = size;
  N->next = M->next;
  M->next = N;

  return temp;
}

void*
Q_realloc(void* temp, int size)
{
  char* oldptr;
  memory_t* M;
  unsigned int size2;

  /*if the size is 0, might as well free it*/
  if (size == 0)
  {
    Q_free(temp);
    //		*newptr=NULL;
    return NULL;
  }

  /*the tests below used to check if temp was a NULL pointer*/
  /*(ie not yet used) however, some unused pointers might not*/
  /*be NULL, they might just have garbage values in there.*/
  /*if they do, we'll be assured this way that they need to be*/
  /*truely allocated*/

  /*Check to see if the block is already part of the heap*/
  for (M = MemoryStart; M; M = M->next)
  {
    if ((M->type == 1) && (M->offset == temp))
      break;
  }

  /*if the block isnt part of the heap then make it one*/
  if ((M == NULL) || (temp == NULL))
  {
    return Q_malloc(size);
  }

  /*If the block is part of the heap, then we are actually going*/
  /*to reallocate it to its new size (if the new size is different*/
  /*from the old one)*/

  /*if we arent changing the size, just return the pointer (duh)*/
  if (M->size == size)
  {
    /*                printf("Q_Realloc: Realloced a block of the same size, dumbass\n");*/
    return temp;
  }

  /*free the old block, allocate a new one, copy the old contents into */
  /*a new block*/

  size2 = M->size;

  oldptr = temp;
  Q_free(temp);
  temp = Q_malloc(size);
  if (temp != NULL)
  {
    if (temp != oldptr)
      memcpy(temp, oldptr, size2);
    return temp;
  }
  else
    return NULL;
}

int
Q_free(void* temp)
{
  memory_t* M;
  memory_t* N;
  unsigned int size;

  /*Find the pointer, as well as the pointer in front of it*/
  /* (N = M->last, if there was actually a ->last field)*/

  N = NULL;
  for (M = MemoryStart; M; M = M->next)
  {
    if ((M->type == 1) && (M->offset == temp))
      break;
    N = M;
  }

  /*if we couldnt find one, then we cant free it*/
  if (M == NULL)
  {
    printf("Q_free: Tried to free a block not previously allocated\n");
    return FALSE;
  }

  M->type = 0;
  size = M->size;
  memused -= size;

  /*Try to combine this block with the one preceding it*/
  if (N != NULL)
  {
    if (N->type == 0)
    {
      /*if the previous block is a free block*/
      /*combine the two*/
      N->size += M->size;
      N->next = M->next;
      free(M);
      M = N;
    }
  }

  /*A rare case, M is the LAST block in the heap, we are done*/
  if (M->next == NULL)
    return TRUE;

  /*If the next block is not a free one, we are done*/
  if ((M->next)->type == 1)
    return TRUE;

  /*Otherwise, we need to combine this block with the one after it*/
  N = M->next;

  /*If the next block is in fact free, combine*/
  if (N->type == 0)
  {
    M->size += N->size;
    M->next = N->next;
    free(N);
  }

  return TRUE;
}

void
WalkHeap(void)
{
  memory_t* M;
  int i = 0;
  unsigned int count1 = 0;
  unsigned int count2 = 0;
  unsigned int count3 = 0;

  for (M = MemoryStart; M; M = M->next)
  {
    /*                printf("Entry           #%d\n",i);*/
    /*                if (M->type==0) printf("Type:           FREE\n");*/
    /*                else if (M->type==1) printf("Type:           USED\n");*/
    /*                else printf("Type:           Unknown type, this is bad\n");*/
    /*                printf("Memory location %d\n",M->offset);*/
    /*                printf("Memory size     %d\n",M->size);*/
    /*                printf("\n");*/
    /*                i++;*/
    count1 += M->size;
    if (M->type == 1)
      count2 += M->size;
    else if (M->type == 0)
      count3 += M->size;
    else
    {
      Abort("WalkHeap", "Unexpected Heap Type, type %d\n", M->type);
    }
  }
  if (count1 != (Q_TotalMem))
  {
    Abort("WalkHeap", "Total Memory Mismatch, Corrupted Heap\n");
  }
  if (count2 != memused)
  {
    Abort("WalkHeap", "Used Memory Mismatch, Corrupted Heap\n");
  }
  if (count3 != (Q_TotalMem - memused))
  {
    Abort("WalkHeap", "Free Memory Mismatch, Corrupted Heap\n");
  }
  /*	if (_msize(MemoryStart->offset)!=(Q_TotalMem))
     {
       Abort("WalkHeap","Main Heap Allocation Corrupted\n");
    }*/
}

void
CloseMemory(void)
{
  memory_t* M;
  memory_t* N;

  free(MemoryStart->offset);

  for (M = MemoryStart; M; M = N)
  {
    N = M->next;
    free(M);
  }
}
#endif

char*
Q_strdup(const char* src)
{
  char* r;

  r = Q_malloc(strlen(src) + 1);
  if (!r)
    return NULL;
  strcpy(r, src);

  return r;
}
