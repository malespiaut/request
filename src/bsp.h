/*
bsp.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef BSP_H
#define BSP_H

/*
The numerical order of these is important, so don't change it unless
you know what you're doing.

 From 'bspspan.c':
   >=BSP_LIGHT   should be lighted and correctly textured.
   >=BSP_TEX     should be textured.
   <=BSP_COL     single color.

*/
// The BSP modes:
#define BSP_GREY 0   // Grey shaded.
#define BSP_COL 1    // Colored according to average texture color.
#define BSP_TEX 2    // Linear textured.
#define BSP_TEXC 3   // Correct textured.
#define BSP_LIGHT 4  // Lighted preview without shadows.
#define BSP_LIGHTS 5 // Lighted preview with shadows.

int BSPAddBrush(struct brush_s* b);

void BSPDeleteBrush(struct brush_s* b);

void DeleteBSPTree(void);

void RebuildBSP(void);

#endif
