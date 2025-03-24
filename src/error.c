/*
error.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "types.h"

#include "error.h"

#include "game.h"
#include "qui.h"
#include "status.h"
#include "video.h"

void
HandleError(const char* proc, const char* format, ...)
{
  char error[1024];
  va_list args;

  va_start(args, format);
  vsprintf(error, format, args);
  va_end(args);

  if (Q.Active == TRUE)
  {
    QUI_Dialog(proc, error);
  }
  else
  {
    if (status.vid_mode != RES_TEXT)
    {
      SetMode(RES_TEXT);
      status.vid_mode = RES_TEXT;
    }
    fprintf(stderr, "-- %s:  %s\n", proc, error);
  }
}

static int aborted = 0;

void
Abort(const char* proc, const char* format, ...)
{
  char error[1024];
  va_list args;
  int was_aborted;

  was_aborted = aborted;
  aborted = 1;

  va_start(args, format);
  vsprintf(error, format, args);
  va_end(args);

  if (status.vid_mode != RES_TEXT)
  {
    SetMode(RES_TEXT);
    status.vid_mode = RES_TEXT;
  }
  fprintf(stderr, "-- %s:  %s\n", proc, error);
  fprintf(stderr, "Unrecoverable error! Program terminated!\n");

  if (was_aborted)
  {
    fprintf(stderr, "Abort called twice! Error saving map?\n");
  }
  else
  {
    if (Q.Active)
    {
      Game.map.savemap("q_backup.map");
      fprintf(stderr, "An attempt has been made to save your work to 'q_backup.map'.\n");
    }
  }

  //   *((int *)-1)=0;

  exit(1);
}
