/*
types.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef TYPES_H
#define TYPES_H

// Base types

typedef float scalar_t;

typedef struct vec3_s
{
	scalar_t  x;
	scalar_t  y;
	scalar_t  z;
} vec3_t;

typedef struct svec_s
{
   int x,y;
   int onscreen;
} svec_t;

typedef struct lvec3_s
{
	int x;
	int y;
	int z;
} lvec3_t;

typedef struct box_s
{
	int x;
	int y;
} box_t;

typedef struct boundbox_s
{
	vec3_t min;            /* Minimum value of (x, y, z) */
	vec3_t max;            /* Maximum value of (x, y, z) */
} boundbox_t;

typedef float matrix_t[4][4];

typedef struct edge_s
{
	int startvertex;            /* ID of the start vertex */
	int endvertex;              /* ID of the end vertex */
} edge_t;


// texdef_t structures for different games

typedef struct
{
   int detail;
} q_texdef_t;

typedef struct
{
   unsigned int contents;  /* Contents of this plane */
   unsigned int flags;     /* Flags */
   unsigned int value;     /* Strength of light */
} q2_texdef_t;

typedef struct
{
   char animname[64];
   unsigned int flags;
   unsigned int contents;
   unsigned int value;
   unsigned int direct;
   float animtime;
   float nonlit;
   unsigned int directangle;
   unsigned int trans_angle;
   char  directstyle[64];
   float translucence;
   float friction;
   float restitution;
   float trans_mag;
   float color[3];
} sin_texdef_t;


typedef struct texdef_s
{
	char    name[64];  /* Name of texture for plane */
	float   shift[2];
	float   rotate;
	float   scale[2];

/* Game specific fields, keep in union to save space */
   union
   {
      q_texdef_t q;
      q2_texdef_t q2;
      sin_texdef_t sin;
   } g;
} texdef_t;


// Map structures (brush related stuff is in brush.h)

/*
   Group flags (8 bits, therefore 8 flags)
	0x1 - The WorldSpawn Group
	0x2 - DONT display
	0x4 - Smart Selecting (selecting 1 of the group selects all)
*/
typedef struct group_s
{
	char groupname[40];
	char color;
	char flags;
	struct group_s *Next;
	struct group_s *Last;
} group_t;


typedef struct entity_s
{
	char   **key;
	char   **value;
	int      numkeys;
	vec3_t   trans;
   svec_t   strans;
   vec3_t   min,max;
   vec3_t   center;

	int      drawn;
   int      hidden;
   int      nbrushes;

   unsigned int uid;         /* Identification number, MUST be unique */
	group_t *Group;
	struct entity_s *Last;
	struct entity_s *Next;
} entity_t;


// Structures used for selecting stuff

typedef struct entityref_s
{
	entity_t *Entity;

	struct entityref_s *Next;
	struct entityref_s *Last;
} entityref_t;

typedef struct vsel_s
{
	vec3_t *vert;
	vec3_t *tvert;
	svec_t *svert;
	struct vsel_s *Next;
	struct vsel_s *Last;
} vsel_t;

typedef struct facesel_s
{
	struct brush_s *Brush;
	int facenum;

	struct facesel_s *Next;
	struct facesel_s *Last;
} fsel_t;

typedef struct brushref_s
{
	struct brush_s *Brush;

	struct brushref_s *Next;
	struct brushref_s *Last;
} brushref_t;

#endif

