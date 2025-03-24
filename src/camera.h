/*
camera.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef CAMERA_H
#define CAMERA_H

#define LOOK_POS_X 0x00
#define LOOK_POS_Y 0x01
#define LOOK_NEG_X 0x02
#define LOOK_NEG_Y 0x03
#define LOOK_UP 0x10
#define LOOK_DOWN 0x08

#define MOVE_UP 1
#define MOVE_DOWN 2
#define MOVE_LEFT 3
#define MOVE_RIGHT 4
#define MOVE_FORWARD 5
#define MOVE_BACKWARD 6

void Move(int vport, int dir, int* dx, int* dy, int* dz, int amt);

void Move90(int vport, int dir, int* dx, int* dy, int* dz, int amt);

void MoveCamera(int vport, int dir);

void LookDown(int vport);

void LookUp(int vport);

void TurnLeft(int vport);

void TurnRight(int vport);

void RollLeft(int vport);

void RollRight(int vport);

void InitCamera(void);

#endif
