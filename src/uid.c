/*
uid.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "uid.h"

#include "error.h"

/* 32 bits should be more than enough. */
static unsigned int uid_brush = 0;
static unsigned int uid_entity = 0;

int
GetBrushUID(void)
{
  unsigned int i;

  i = uid_brush++;
  //   printf("b_uid=%i\n",i);
  if (!uid_brush) // wraparound, duplicate values, not good
  {
    Abort("GetBrushUID", "UID overflow! Restart Quest! (how did this happen?)");
  }
  return i;
}

int
GetEntityUID(void)
{
  unsigned int i;

  i = uid_entity++;
  //   printf("e_uid=%i\n",i);
  //   if (i==4)
  //      *((int *)(-1))=0;
  if (!uid_entity) // wraparound, duplicate values, not good
  {
    Abort("GetEntityUID", "UID overflow! Restart Quest! (how did this happen?)");
  }
  return i;
}
