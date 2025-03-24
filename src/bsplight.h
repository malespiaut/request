/*
bsplight.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef BSPLIGHT_H
#define BSPLIGHT_H

float GetLight(vec3_t pos);

void GetLightCol(vec3_t pos, float* r, float* g, float* b);

void FaceLight(vec3_t norm, float dist);

void InitLights(int shadows);

void DoneLights(void);

#endif
