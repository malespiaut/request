/*
mouse.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef MOUSE_H
#define MOUSE_H

typedef struct
{
	int   x;
	int   y;
	int   prev_x;
	int   prev_y;
	int   moved;
	int   button;
} mouse_t;

extern mouse_t mouse;

int InitMouse(void);

void DrawMouse(int x, int y);

void UndrawMouse(int x, int y);

void SetMouseSensitivity(float valuex,float valuey);

void SetMousePos(int x, int y);

void GetMousePos(void);

void SetMouseLimits(int x0, int y0, int x1, int y1);

void UpdateMouse(void);

int FindMouse(int DrawBox);

int InBox(int x1, int y1, int x2, int y2);

#endif

