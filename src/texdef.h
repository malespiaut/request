/*
texdef.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef TEXDEF_H
#define TEXDEF_H

void SetTexture(texdef_t *t,char	*name);

void InitTexdef(texdef_t *t);


void MoveSelTVert(int dir);
void ScaleSelTVert(int dir);

#endif

