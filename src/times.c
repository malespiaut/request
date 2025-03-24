/*
times.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

/*
System specific timing code. Should make porting easier. In all cases,
which time is 0 doesn't matter.
*/

#include "times.h"

#ifdef DJGPP

#include <go32.h>
#include <sys/farptr.h>

/*
Return current time as a float, with as much precision as possible. Returns
0 the first time, although that shouldn't be necessary.
*/

#include <time.h>

float
GetTime(void)
{
#if 1 // Safe way
  static float start = -1;
  int ct;
  float t;

  ct = _farpeekl(_dos_ds, 0x46c);
  t = (float)(ct) / 18.2;

  if (start == -1)
  {
    start = t;
    return 0;
  }
  return t - start;
#else
  return (float)uclock() / (float)UCLOCKS_PER_SEC;
#endif
}

/*
Return current tick. The DOS version is based on 18.2 ticks/second, but
anything from 15 to 20 should work fine.
*/

int
GetTick(void)
{
  return _farpeekl(_dos_ds, 0x46c);
}

#elif _UNIX

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

float
GetTime(void)
{
  static int base = -1;
  struct timeval tv;
  struct timezone tz;
  int i;

  i = gettimeofday(&tv, &tz);
  if (base == -1)
    base = tv.tv_sec;
  tv.tv_sec -= base;

  return tv.tv_sec + ((float)tv.tv_usec) / 1000000;
}

int
GetTick(void)
{
  return (int)GetTime() * 18.2;
}

#else

#error Unimplemented!

#endif
