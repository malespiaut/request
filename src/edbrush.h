/*
edbrush.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef EDBRUSH_H
#define EDBRUSH_H

int AddSelBrush(struct brush_s* sel_brush, int addgroup);
void RemoveSelBrush(brushref_t* b);

struct brush_s* FindBrush(int mx, int my);
void SelectWindowBrush(int x0, int y0, int x1, int y1);

void HandleLeftClickBrush(void);

int CopyBrush(void);
int PasteBrush(void);

void DeleteABrush(struct brush_s* b);
int DeleteBrush(void);

void ApplyTexture(struct brush_s* b, char* texname);

void HighlightAllBrushes(void);

void ClearSelBrushes(void);

void AutoZoom(void);

#endif
