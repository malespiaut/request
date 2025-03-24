/*
mdl.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef MDL_H
#define MDL_H

typedef struct mdl_s
{
  vec3_t* vertlist;
  vec3_t* tvertlist; // this is only used in the DrawModel function
  int numvertices;

  edge_t* edgelist;
  int numedges;
} mdl_t;

char** LoadModelNames(int types, int* numnames);

void DrawModel(int vport, entity_t* ent, matrix_t m, int selected);

void InitMDLPak(void);

void DoneMDLPak(void);

mdl_t* LoadModel(char* name);

#endif
