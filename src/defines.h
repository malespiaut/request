/*
defines.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef DEFINES_H
#define DEFINES_H

#define TRUE  1
#define FALSE 0

#define  MAX_NUM_VIEWPORTS  5

#define  APERTURE    6

/* View modes */
#define  WIREFRAME   1
#define  NOPERSP     2
#define  SOLID       3
#define  BSPVIEW     4

/* Grid types */
#define  NOGRID  1
#define  GRID    2
#define  ALIGN   3

/* Edit modes */
#define  BRUSH   1
#define  FACE    2
#define  ENTITY  3
#define  MODEL   4

#ifdef _UNIX

#ifndef PI
#define PI 3.14159265358979323846
#endif  /* PI */
#define stricmp(a, b) strcasecmp((a), (b))
#define strnicmp(a, b, c) strncasecmp((a), (b), (c))
char *strlwr(char *string);

#endif  /* _UNIX */

#endif

