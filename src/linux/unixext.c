/*
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
*/  /* (Not necessary) */

#include <ctype.h>


char *strlwr(char *string)
{
      char *s;

      if (string)
      {
            for (s = string; *s; ++s)
                  *s = tolower(*s);
      }
      return string;
}
