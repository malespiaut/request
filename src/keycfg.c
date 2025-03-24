/*
keycfg.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "keycfg.h"

#include "cfgs.h"
#include "error.h"
#include "file.h"
#include "keyboard.h"
#include "token.h"

static void PError(const char* format, ...)
  __attribute__((noreturn, format(printf, 1, 2)));

static void
PError(const char* format, ...)
{
  char buf[256];
  va_list args;

  va_start(args, format);
  vsprintf(buf, format, args);
  va_end(args);

  Abort("LoadKeyConfig", "Line %i: %s", token_linenum, buf);
}

static void
Expect(const char* what)
{
  if (!TokenGet(0, -1))
    PError("Expected '%s'!", what);

  if (strcmp(token, what))
    PError("Expected '%s'!", what);
}

void
LoadKeyConfig(const char* name)
{
  int cur_mode;
  int i;
  int key, cmd;
  char filename[256];

  FindFile(filename, name);

  if (!TokenFile(filename, T_C | T_NAME | T_MISC, NULL))
  {
    Abort("LoadKeyConfig", "Unable to load '%s'!", name);
  }

  cur_mode = -1;

  while (TokenGet(1, -1))
  {
    if (!strcmp(token, "["))
    {
      if (!TokenGet(0, -1))
        PError("Parse error!");

      for (i = 0; i < NUM_MODES; i++)
        if (!strcmp(modes[i].name, token))
          break;
      if (i == NUM_MODES)
        PError("Unknown mode '%s'!", token);

      cur_mode = i;

      Expect("]");
    }
    else
    {
      if (cur_mode == -1)
        PError("Undefined mode!");

      key = FindDef(key_defs, token);
      if (!TokenGet(0, -1))
        PError("Parse error!");
      while (!strcmp(token, "+"))
      {
        if (!TokenGet(0, -1))
          PError("Parse error!");
        key |= FindDef(k_flags, token);
        if (!TokenGet(0, -1))
          PError("Parse error!");
      }
      if (strcmp(token, "="))
        PError("Expected '%s'!", "=");

      if (!TokenGet(0, -1))
        PError("Parse error!");

      cmd = FindDef(cmds, token);

      AddCfg(cur_mode, key, cmd);
    }
  }

  TokenDone();

  CheckCfgs();
}
