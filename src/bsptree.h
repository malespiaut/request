/*
bsptree.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef BSPTREE_H
#define BSPTREE_H

/*
This file has the typedef:s for the BSP tree stuff.
*/


#define HASH_SIZE 64      // size of the hash will be HASH_SIZE**2


typedef struct vec3_vs
{
   float x,y,z;
   int   sx,sy;
   int   outcode;
} vec3_vt;

typedef struct
{
   float s,t;
} vtex_t;


typedef struct
{
   vec3_t normal;
   float  dist;
} bsp_plane_t;


typedef struct bsp_map_face_s
{
   char texname[64];
   int flags;         // flags for this face, defined in game.h

//   struct brush_s *b;
} bsp_map_face_t;     // One for every face in the map, each referenced
                      // from every bsp_face_t created from that map face.

typedef struct
{
   vec3_t  *pts;        // these get freed.. dont reference after tree is built
   int     *i_pts;      // indices in the main BSP vertex list
   vtex_t  *t_pts;      /* texture coordinates at each vertex */

   int     numpts;

   bsp_map_face_t *mf;

   int     samenormal;  // used so that we dont have to calc 2 dot products
} bsp_face_t;

typedef struct bsp_node_s
{
   struct bsp_node_s *FrontNode;
   struct bsp_node_s *BackNode;

   bsp_face_t **faces;
   int        numfaces;

   bsp_plane_t plane;
} bsp_node_t;


#define MY_DotProd(v1,v2,v3)                                              \
{                                                                         \
  (v3) = (((v1).x)*((v2).x)) + (((v1).y)*((v2).y)) + (((v1).z)*((v2).z)); \
}

#endif

