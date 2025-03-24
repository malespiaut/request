#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "sin_texi.h"

#include "sin.h"

#include "tex.h"

void
Sin_FlagsDefault(texdef_t* tex)
{
  texture_t* t;

  t = ReadMIPTex(tex->name, 0);
  if (t)
  {
    tex->g.sin = t->g.sin;
  }
  else
  {
    memset(&tex->g.sin, 0, sizeof(tex->g.sin));
  }
}
