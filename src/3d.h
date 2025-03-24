/*
3d.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef _3D_H
#define _3D_H

void InitMatrix(matrix_t matrix);
void MultMatrix(matrix_t one, matrix_t two, matrix_t three);

void GetRotValues(int vport, int* rx, int* ry, int* rz);

void GenerateRotMatrix(matrix_t m, int rx, int ry, int rz);
void GenerateIRotMatrix(matrix_t m, int rx, int ry, int rz);

void GenerateMatrix(matrix_t m, int vport);
void GenerateIMatrix(matrix_t m, int vport);

void ReadPts(char* name);
void FreePts(void);

void UpdateViewport(int vport, int refresh);
void UpdateAllViewports(void);

void UpdateViewportBorder(int vport);

void DrawBrush(int vport, struct brush_s* b, int base_color, int force_bright);

void RotateDisplay(int from, int to);

void Profile(int vport);

#endif
