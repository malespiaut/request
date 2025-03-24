/*
bspleak.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef BSPLEAK_H
#define BSPLEAK_H

int TraceBSPInit(void);
void TraceBSPDone(void);

int Trace(vec3_t from, vec3_t to);

void FindLeak(void);

#endif
