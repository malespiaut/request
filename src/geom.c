/*
geom.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "defines.h"
#include "types.h"

#include "geom.h"

void Normalize(vec3_t *v)
{
//	vec3_t vec;
	float  len;

	len = v->x*v->x + v->y*v->y + v->z*v->z;

	if (len != 0)
	{
		len = sqrt(len);
      len = 1/len;

		v->x *= len;
		v->y *= len;
		v->z *= len;
	}

}

void _CrossProd(vec3_t v1,vec3_t v2,vec3_t *normal)
{
	normal->x = (v1.y * v2.z) - (v2.y * v1.z);
	normal->y = (v1.z * v2.x) - (v2.z * v1.x);
	normal->z = (v1.x * v2.y) - (v2.x * v1.y);
}

float _DotProd(vec3_t v1,vec3_t v2)
{
	return ((v1.x*v2.x) + (v1.y*v2.y) + (v1.z*v2.z));
}

float Sign(float num)
{
	if (num > 0)
		return 1.0;
	else if (num < 0)
		return -1.0;
	else
		return 0;
}

