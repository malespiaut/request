/*
geom.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef GEOM_H
#define GEOM_H

#define CrossProd(v1, v2, normal)                        \
  ({                                                     \
    (normal)->x = ((v1).y * (v2).z) - ((v2).y * (v1).z); \
    (normal)->y = ((v1).z * (v2).x) - ((v2).z * (v1).x); \
    (normal)->z = ((v1).x * (v2).y) - ((v2).x * (v1).y); \
  })

#define DotProd(v1, v2) ((v1).x * (v2).x + (v1).y * (v2).y + (v1).z * (v2).z)

void Normalize(vec3_t* v);

void _CrossProd(vec3_t v1, vec3_t v2, vec3_t* normal);

float _DotProd(vec3_t v1, vec3_t v2);

float Sign(float num);

#endif
