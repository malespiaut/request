/*
edvert.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef EDVERT_H
#define EDVERT_H

void ClearSelVerts(void);

vsel_t *FindSelectVert(vec3_t *vec);

void AddSelectVert(struct brush_s *b,int num);

void AddSelectVec(vec3_t *v);

vec3_t *FindVertex(int vport,int mx,int my);

void SelectWindowVert(int vport,int x0,int y0,int x1,int y1);

void MoveSelVert(int dir,int recalc);

#endif

