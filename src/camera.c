/*
camera.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "camera.h"

#include "3d.h"
#include "display.h"
#include "entity.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "video.h"


#define MoveGen() \
{ \
   GenerateIRotMatrix(m,-rot_x,-rot_y,-rot_z); \
\
   d[0]=d[1]=d[2]=0; \
   switch (dir) \
   { \
      case MOVE_UP: \
         d[1]=1; \
         break; \
      case MOVE_DOWN: \
         d[1]=-1; \
         break; \
\
      case MOVE_LEFT: \
         d[0]=-1; \
         break; \
      case MOVE_RIGHT: \
         d[0]=1; \
         break; \
\
      case MOVE_FORWARD: \
         d[2]=1; \
         break; \
      case MOVE_BACKWARD: \
         d[2]=-1; \
         break; \
   } \
\
   for (i=0;i<3;i++) \
   { \
      e[i]=d[0]*m[i][0]+ \
           d[1]*m[i][1]+ \
           d[2]*m[i][2]; \
   } \
\
   *dx=e[0]*amt; \
   *dy=e[1]*amt; \
   *dz=e[2]*amt; \
}

void Move(int vport, int dir, int *dx, int *dy, int *dz, int amt)
{
   matrix_t m;
   scalar_t d[3],e[3];
   int i;
   int rot_x,rot_y,rot_z;

   GetRotValues(vport,&rot_x,&rot_y,&rot_z);

   MoveGen();
}

void Move90(int vport, int dir, int *dx, int *dy, int *dz, int amt)
{
   matrix_t m;
   scalar_t d[3],e[3];
   int i;
   int rot_x,rot_y,rot_z;

   GetRotValues(vport,&rot_x,&rot_y,&rot_z);

#define Snap(x) \
   if (x>0) \
      x=floor((x+45)/90)*90; \
   else \
   if (x<0) \
      x=floor((x-45)/90)*90;

   Snap(rot_x);
   Snap(rot_y);
   Snap(rot_z);

#undef Snap

   MoveGen();
}

void MoveCamera(int vport, int dir)
{
	int dx, dy, dz;

	Move(vport, dir, &dx, &dy, &dz, status.pan_speed);
	M.display.vport[vport].camera_pos.x += dx;
	M.display.vport[vport].camera_pos.y += dy;
	M.display.vport[vport].camera_pos.z += dz;
}

void LookDown(int vport)
{
	int pre_dir;

   if (M.display.vport[vport].axis_aligned)
   {
   	pre_dir = M.display.vport[vport].camera_dir;
   	if (M.display.vport[vport].camera_dir & LOOK_UP)
   		M.display.vport[vport].camera_dir ^= LOOK_UP;
   	else
   	if ((M.display.vport[vport].camera_dir & 0xF8) == 0)
   		M.display.vport[vport].camera_dir ^= LOOK_DOWN;
   	RotateDisplay(pre_dir, M.display.vport[vport].camera_dir);
   }
   else
   {
      M.display.vport[vport].rot_x+=5;
   }
}

void LookUp(int vport)
{
	int pre_dir;

   if (M.display.vport[vport].axis_aligned)
   {
   	pre_dir = M.display.vport[vport].camera_dir;
   	if (M.display.vport[vport].camera_dir & LOOK_DOWN)
   		M.display.vport[vport].camera_dir ^= LOOK_DOWN;
   	else
   	if ((M.display.vport[vport].camera_dir & 0xF8) == 0)
   		M.display.vport[vport].camera_dir ^= LOOK_UP;
   	RotateDisplay(pre_dir, M.display.vport[vport].camera_dir);
   }
   else
   {
      M.display.vport[vport].rot_x-=5;
   }
}

void TurnLeft(int vport)
{
	int pre_dir;

   if (M.display.vport[vport].axis_aligned)
   {
   	pre_dir = M.display.vport[vport].camera_dir;
   	if ((M.display.vport[vport].camera_dir & 0x03) == LOOK_POS_X)
   	{
   		M.display.vport[vport].camera_dir &= 0xF8;
   		M.display.vport[vport].camera_dir |= LOOK_POS_Y;
   	}
   	else
   	if ((M.display.vport[vport].camera_dir & 0x03) == LOOK_POS_Y)
   	{
   		M.display.vport[vport].camera_dir &= 0xF8;
   		M.display.vport[vport].camera_dir |= LOOK_NEG_X;
   	}
   	else
   	if ((M.display.vport[vport].camera_dir & 0x03) == LOOK_NEG_X)
   	{
   		M.display.vport[vport].camera_dir &= 0xF8;
   		M.display.vport[vport].camera_dir |= LOOK_NEG_Y;
   	}
   	else
   	if ((M.display.vport[vport].camera_dir & 0x03) == LOOK_NEG_Y)
   	{
   		M.display.vport[vport].camera_dir &= 0xF8;
   		M.display.vport[vport].camera_dir |= LOOK_POS_X;
   	}
   	RotateDisplay(pre_dir, M.display.vport[vport].camera_dir);
   }
   else
   {
      M.display.vport[vport].rot_z-=5;
   }
}

void TurnRight(int vport)
{
	int pre_dir;

   if (M.display.vport[vport].axis_aligned)
   {
   	pre_dir = M.display.vport[vport].camera_dir;
   	if ((M.display.vport[vport].camera_dir & 0x03) == LOOK_POS_X)
   	{
   		M.display.vport[vport].camera_dir &= 0xF8;
   		M.display.vport[vport].camera_dir |= LOOK_NEG_Y;
   	}
   	else
   	if ((M.display.vport[vport].camera_dir & 0x03) == LOOK_NEG_Y)
   	{
   		M.display.vport[vport].camera_dir &= 0xF8;
   		M.display.vport[vport].camera_dir |= LOOK_NEG_X;
   	}
   	else
   	if ((M.display.vport[vport].camera_dir & 0x03) == LOOK_NEG_X)
   	{
   		M.display.vport[vport].camera_dir &= 0xF8;
   		M.display.vport[vport].camera_dir |= LOOK_POS_Y;
   	}
   	else
   	if ((M.display.vport[vport].camera_dir & 0x03) == LOOK_POS_Y)
   	{
   		M.display.vport[vport].camera_dir &= 0xF8;
   		M.display.vport[vport].camera_dir |= LOOK_POS_X;
   	}
   	RotateDisplay(pre_dir, M.display.vport[vport].camera_dir);
   }
   else
   {
      M.display.vport[vport].rot_z+=5;
   }
}

void RollLeft(int vport)
{
   if (!M.display.vport[vport].axis_aligned)
   {
      M.display.vport[vport].rot_y-=5;
   }
}

void RollRight(int vport)
{
   if (!M.display.vport[vport].axis_aligned)
   {
      M.display.vport[vport].rot_y+=5;
   }
}

void InitCamera(void)
{
	int      i, j;
	int      found = FALSE;
	int      dir=0;
	lvec3_t  origin;
	int      angle;
	entity_t *e;

	for (e=M.EntityHead; e && (!found); e=e->Next)
	{
      if (!GetKeyValue(e,"classname"))
         continue;

		if (strcmp(GetKeyValue(e, "classname"), "info_player_start"))
         continue;

      if (!GetKeyValue(e,"origin"))
         continue;

      found = TRUE;
      sscanf(GetKeyValue(e, "origin"), "%d %d %d",
         &origin.x, &origin.y, &origin.z);

      for (j=0; j<MAX_NUM_VIEWPORTS; j++)
      {
      	M.display.vport[j].camera_pos.x = origin.x;
      	M.display.vport[j].camera_pos.y = origin.y;
      	M.display.vport[j].camera_pos.z = origin.z;
      }

      if (!GetKeyValue(e,"angle"))
         continue;

      angle = atoi(GetKeyValue(e, "angle"));
      if ((angle <= 45) || (angle >= 315))
      	dir = LOOK_POS_X;
      else
      if ((angle >= 45) && (angle <= 135))
      	dir = LOOK_NEG_Y;
      else
      if ((angle >= 135) && (angle <= 225))
      	dir = LOOK_NEG_X;
      else
      if ((angle >= 225) && (angle >= 315))
      	dir = LOOK_POS_Y;
	}

	if (found)
	{
		for (i=0; i<MAX_NUM_VIEWPORTS; i++)
		{
			M.display.vport[i].camera_dir = dir;
			M.display.vport[i].mode = WIREFRAME;
         M.display.vport[i].axis_aligned=1;
			M.display.vport[i].zoom_amt = 1.0;
		}
	}
	else
	{
		for (i=0; i<MAX_NUM_VIEWPORTS; i++)
		{
			M.display.vport[i].camera_dir = LOOK_POS_X;
			M.display.vport[i].camera_pos.x = 0;
			M.display.vport[i].camera_pos.y = 0;
			M.display.vport[i].camera_pos.z = 0;
         M.display.vport[i].axis_aligned=1;
			M.display.vport[i].mode = WIREFRAME;
			M.display.vport[i].zoom_amt = 1.0;
		}
	}

   M.display.vp_full=0;
}

