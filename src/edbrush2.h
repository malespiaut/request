#ifndef EDBRUSH2_H
#define EDBRUSH2_H

void SnapSelectedBrushes(void);
void SnapSelectedVerts(void);

int RotateBrush(int vport);
int ScaleBrush(int brush_flag);
int MirrorBrush(int vport);

void NewClipPlane(void);

void SnapToVertex(void);

#endif

