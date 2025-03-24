/*
token.c file of the Quest Source Code

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

#include "token.h"

#include "error.h"
#include "memory.h"

static char* filebuf = NULL;
char* token_curpos;

static int global_flags;
static int flags;

static void (*PComment)(char* buf) = NULL;

char token[16384];
int token_linenum;

static void
TokenInit(int fileflags, void (*parsecomment)(char* buf))
{
  PComment = parsecomment;
  global_flags = fileflags;
  token_curpos = filebuf;
  token_linenum = 1;
}

int
TokenFile(const char* filename, int fileflags, void (*parsecomment)(char* buf))
{
  FILE* f;
  int size;

  TokenDone();

  f = fopen(filename, "rb");
  if (!f)
    return 0;

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  fseek(f, 0, SEEK_SET);

  filebuf = Q_malloc(size + 1);
  if (!filebuf)
    return 0;

  fread(filebuf, 1, size, f);

  filebuf[size] = 0;

  fclose(f);

  TokenInit(fileflags, parsecomment);

  return 1;
}

int
TokenBuf(char* buf, int fileflags, void (*parsecomment)(char* buf))
{
  int size;

  size = strlen(buf);
  filebuf = Q_malloc(size + 1);
  if (!filebuf)
    return 0;
  memcpy(filebuf, buf, size + 1);
  TokenInit(fileflags, parsecomment);
  return 1;
}

void
TokenDone(void)
{
  if (filebuf)
  {
    Q_free(filebuf);
    filebuf = NULL;
  }
}

static int
SkipWhitespace(void)
{
  while ((*token_curpos <= 32) && *token_curpos)
  {
    if (*token_curpos == '\n')
      token_linenum++;
    token_curpos++;
  }

  if (!*token_curpos)
    return 0;

  return 1;
}

static const char* punctation[] =
  {
    "+",
    "-",

    "=",

    "[",
    "]",

    "{",
    "}",

    "(",
    ")",

    ":",

    ";",
    ",",

    "@",

    "*"};
#define NUM_PUNCTATION (sizeof(punctation) / sizeof(punctation[0]))

static int
SkipToToken(int comments)
{
  char* t;

  do
  {
    if (!*token_curpos)
      return 0;

    if (!SkipWhitespace())
      return 0;

    if (flags & T_C)
    {
      if ((token_curpos[0] == '/') && (token_curpos[1] == '/'))
      {
        t = token;
        while (*token_curpos && (*token_curpos != '\n'))
          *t++ = *token_curpos++;
        *t = 0;

        if (!*token_curpos)
          return 0;

        if (PComment && comments)
        {
          PComment(token);
        }

        token_curpos++;
        token_linenum++;
        continue;
      }

      if ((token_curpos[0] == '/') && (token_curpos[1] == '*'))
      {
        token_curpos += 2;
        while (*token_curpos)
        {
          if (*token_curpos == '\n')
            token_linenum++;
          if ((token_curpos[0] == '*') && (token_curpos[1] == '/'))
            break;
          token_curpos++;
        }
        token_curpos += 2;
        if (!token_curpos)
          return 0;
        continue;
      }
    }

    if (flags & T_CFG)
    {
      if (*token_curpos == '#')
      {
        while (*token_curpos && (*token_curpos != '\n'))
          token_curpos++;

        if (!*token_curpos)
          return 0;

        token_curpos++;
        token_linenum++;
        continue;
      }
    }

    break;
  } while (*token_curpos);

  if (!*token_curpos)
    return 0;

  return 1;
}

static int
IsName(int ch)
{
  if (isalnum(ch))
    return 1;
  switch (ch)
  {
    case '_':
      return 1;
    default:
      return 0;
  }
}

int
TokenGet(int crossline, int tflags)
{
  int oldline;
  char* t;
  int i;

  if (!filebuf)
    Abort("TokenGet", "Internal error! TokenGet() before TokenFile()!");

  if (tflags == -1)
    flags = global_flags;
  else
  {
    flags = tflags;
    if (!(flags & T_ALLNAME))
      flags |= global_flags & (T_C | T_CFG);
  }

  oldline = token_linenum;

  if (!SkipToToken(1))
    return 0;

  if ((token_linenum != oldline) && !crossline)
    return 0;

  if (!*token_curpos)
    return 0;

  t = token;

  if ((*token_curpos == '\"') && (flags & T_STRING))
  {
    *t++ = *token_curpos++;
    while ((*token_curpos != '\"') && (*token_curpos))
    {
      if (*token_curpos == '\n')
        return 0;

      *t++ = *token_curpos++;
    }

    if (!*token_curpos)
      return 0;

    *t++ = *token_curpos++;
    *t = 0;

    return 1;
  }

  if (flags & T_ALLNAME)
  {
    while (*token_curpos)
    {
      if (*token_curpos <= 32)
        break;

      *t++ = *token_curpos++;
    }

    *t = 0;

    return 1;
  }

  if (flags & T_NUMBER)
  {
    if (((*token_curpos >= '0') && (*token_curpos <= '9')) || (*token_curpos == '-'))
    {
      *t++ = *token_curpos++;
      while (((*token_curpos >= '0') && (*token_curpos <= '9')) || *token_curpos == '.')
        *t++ = *token_curpos++;

      *t = 0;

      return 1;
    }
  }

  if (flags & T_MISC)
  {
    for (i = 0; i < NUM_PUNCTATION; i++)
    {
      if (!strncmp(token_curpos, punctation[i], strlen(punctation[i])))
      {
        strcpy(token, punctation[i]);
        token_curpos += strlen(punctation[i]);

        return 1;
      }
    }
  }

  if ((flags & T_NAME) && IsName(*token_curpos))
  {
    while (IsName(*token_curpos))
      *t++ = *token_curpos++;

    *t = 0;

    return 1;
  }

  return 0;
}

int
TokenAvailable(int crossline)
{
  int oldline, newline;
  char* oldpos;

  int i;

  oldline = token_linenum;
  oldpos = token_curpos;

  i = SkipToToken(0);

  newline = token_linenum;

  token_linenum = oldline;
  token_curpos = oldpos;

  if (!i)
    return 0;

  if ((newline != oldline) && !crossline)
    return 0;

  return 1;
}
