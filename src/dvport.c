#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "dvport.h"

#include "quest.h"
#include "qui.h"


void UpdateViewportPositions(void)
{
   int i;
   viewport_t *vp;
   int px,py,sx,sy;


   px=Q.window[MAP_WINDOW].pos.x;
   py=Q.window[MAP_WINDOW].pos.y;
   sx=Q.window[MAP_WINDOW].size.x-1;
   sy=Q.window[MAP_WINDOW].size.y-1;


   if (M.display.active_vport>=M.display.num_vports)
      M.display.active_vport=0;


   if (M.display.vp_full)
   {
      for (i=0,vp=M.display.vport;i<M.display.num_vports;i++,vp++)
         vp->used=0;

      vp=&M.display.vport[M.display.active_vport];
      vp->used=1;
      vp->xmin=px;
      vp->xmax=px+sx;
      vp->ymin=py;
      vp->ymax=py+sy;
   }
   else
   {
      for (i=0,vp=M.display.vport;i<M.display.num_vports;i++,vp++)
      {
         vp->used=1;

         vp->xmin=vp->f_xmin*sx+px;
         vp->xmax=vp->f_xmax*sx+px-1;
         vp->ymin=vp->f_ymin*sy+py;
         vp->ymax=vp->f_ymax*sy+py-1;
      }
   }
}


void InitViewports(void)
{
   int i;

   for (i=0;i<MAX_NUM_VIEWPORTS;i++)
   {
      M.display.vport[i].grid_type = GRID;
      M.display.vport[i].used=1;
   }

   M.display.num_vports=3;

   M.display.vport[0].f_xmin=0.5;
   M.display.vport[0].f_xmax=1;
   M.display.vport[0].f_ymin=0;
   M.display.vport[0].f_ymax=1;

   M.display.vport[1].f_xmin=0;
   M.display.vport[1].f_xmax=0.5;
   M.display.vport[1].f_ymin=0;
   M.display.vport[1].f_ymax=0.5;

   M.display.vport[2].f_xmin=0;
   M.display.vport[2].f_xmax=0.5;
   M.display.vport[2].f_ymin=0.5;
   M.display.vport[2].f_ymax=1;

   UpdateViewportPositions();
}

