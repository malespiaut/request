/*
edface.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef EDFACE_H
#define EDFACE_H

fsel_t *FindFace(int vport, int mx, int my);
void HandleLeftClickFace(void);

void ClearSelFaces(void);
void HighlightAllFaces(void);

void ApplyFaceTexture(fsel_t *f, char *texname);

#endif

