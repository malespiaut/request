/*
clip.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef CLIP_H
#define CLIP_H

#define MAX_FACE_POINTS 64

typedef struct
{
	char    misc;
	int     numpts;
	vec3_t  planepts[3];
	vec3_t  pts[MAX_FACE_POINTS];  /* points on face / plane (the polygon pts) */
	vec3_t  normal;                /* Normal to plane */
	float   dist;

   texdef_t tex;
} face_my_t;

typedef face_my_t plane_my_t;

typedef struct
{
	int numfaces;
	face_my_t *faces;
} brush_my_t;


int DivideFaceByPlane(face_my_t F,plane_my_t P,face_my_t *Front,face_my_t *Back,int KeepBack);
void MakeBoxOnPlane(face_my_t *P);

int CreateBrushFaces(brush_my_t *B);

/* frees B->faces */
int BuildBrush(brush_my_t *B, struct brush_s *b);

struct brush_s *MyBrushIntoMap(brush_my_t *B, entity_t *EntityRef, group_t *Group);

int BrushesIntersect(brush_my_t *b1, brush_my_t *b2);

void BooleanSubtraction(void);

#endif

