/*
display.c file of the Quest Source Code

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

#include "display.h"

#include "3d.h"
#include "brush.h"
#include "dvport.h"
#include "edface.h"
#include "edbrush.h"
#include "edent.h"
#include "edmodel.h"
#include "error.h"
#include "camera.h"
#include "file.h"
#include "map.h"
#include "memory.h"
#include "menu.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "video.h"


int      num_bitmaps;
bitmap_t bitmap[MAX_NUM_BITMAPS];


bitmap_t *FindBitmap(const char *name)
{
   int i;

   for (i=0; i<MAX_NUM_BITMAPS; i++)
   {
      if (bitmap[i].used)
         if (stricmp(name, bitmap[i].name) == 0)
         {
            bitmap[i].used=2;
            return &bitmap[i];
         }
   }

   return NULL;
}

int DrawBitmapP(bitmap_t *bm, int x, int y)
{
   int       i;
   unsigned char *alias;

   alias = &video.ScreenBuffer[x+y*video.ScreenWidth];

   for (i=0; i<bm->size.y; i++)
   {
      memcpy(alias,&bm->data[i*bm->size.x],bm->size.x);
      alias+=video.ScreenWidth;
   }

   return TRUE;
}

int DrawBitmap(const char *name, int x, int y)
{
   bitmap_t *bm;

   bm = FindBitmap(name);
   if (bm == NULL)
   {
      HandleError("DrawBitmap", "Unable to find bitmap '%s'!",name);
      return FALSE;
   }
   return DrawBitmapP(bm,x,y);
}


#if 0   // not used
static void FreeBitmap(char *name)
{
   int i;

   for (i=0;i<MAX_NUM_BITMAPS;i++)
   {
      if (!stricmp(name,bitmap[i].name))
      {
         bitmap[i].used=0;
         num_bitmaps--;
         return;
      }
   }
}
#endif


static void FindDimPCX(FILE *fp, int *xres, int *yres)
{
   fseek(fp, 8, SEEK_SET);

   *xres=*yres=0;
   fread(xres,1,2,fp);
   fread(yres,1,2,fp);

   (*xres)++;
   (*yres)++;

   if (((*xres) & 0x01) != 0)
      (*xres)++;
}

static int LoadPCX(FILE *fp,unsigned char *buffer,int xres,int yres)
{
   int i;
   int cur_byte;
   int run_len;
   int size;


   size = (int)(xres) * (int)(yres);

   fseek(fp, 128, SEEK_SET);

   while (size > 0)
   {
      cur_byte = fgetc(fp);
      if ((cur_byte & 0xC0) == 0xC0)
      {
         run_len = cur_byte & 0x3F;
         cur_byte = fgetc(fp);
         for (i=0; (i<run_len) && (size>0); i++, size--, buffer++)
         {
            *buffer = cur_byte;
         }
      }
      else
      {
         *buffer = cur_byte;
         buffer++;
         size--;
      }
   }

   return TRUE;
}


static int GetNewBitmapNum(void)
{
   int i;

   if (num_bitmaps >= MAX_NUM_BITMAPS)
   {
      HandleError("GetNewBitmapNum", "The maximum number of bitmaps has been exceeded!");
      return -1;
   }

   for (i=0;i<MAX_NUM_BITMAPS;i++)
   {
      if (!bitmap[i].used)
      {
         bitmap[i].used=1;
         num_bitmaps++;
         return i;
      }
   }

   HandleError("GetNewBitmapNum", "The maximum number of bitmaps has been exceeded!");
   return -1;
}

static int RegisterBitmap(const char *name,const char *file)
{
   FILE     *fp;
   bitmap_t *bm;
   int      size;
   int      i;
   char     filename[256];

   i=GetNewBitmapNum();
   if (i==-1)
      return FALSE;

   bm = &bitmap[i];

   FindFile(filename,file);
   if ((fp = fopen(filename, "rb")) == NULL)
   {
      HandleError("RegisterBitmap", "Unable to open file.");
      return FALSE;
   }

   FindDimPCX(fp, &bm->size.x, &bm->size.y);

   size = (int)(bm->size.x) * (int)(bm->size.y);

   bm->data = Q_malloc(sizeof(unsigned char) * size);

   if (bm->data == NULL)
   {
      HandleError("RegisterBitmap", "Unable to allocate bitmap");
      return FALSE;
   }

   if (!LoadPCX(fp, bm->data, bm->size.x, bm->size.y))
   {
      HandleError("RegisterBitmap", "Unable to load PCX.");
      return FALSE;
   }
   strcpy(bm->name, name);
   fclose(fp);

   return TRUE;
}

static int LoadBitmaps(void)
{
#define RegBMP(bitmapname,filename) \
   if (!RegisterBitmap(bitmapname,filename)) \
   { \
      HandleError("LoadBitmaps","Unable to register '%s'!",filename); \
      return FALSE; \
   }

   /* Always register mouse pointer first, because the mouse drawing
      routines don't want to call FindBitmap every time the mouse is
      moved, so they just assume it's bitmap zero. */
   RegBMP("mouse_ptr","graphics/pointer.pcx");

/*   RegBMP("button_up","graphics/b_up.pcx");
   RegBMP("button_down","graphics/b_down.pcx");
   RegBMP("button_left","graphics/b_left.pcx");
   RegBMP("button_right","graphics/b_right.pcx");*/

   RegBMP("button_small_up"    , "graphics/b_sm_up.pcx");
   RegBMP("button_small_down"  , "graphics/b_sm_dn.pcx");
   RegBMP("button_small_scroll", "graphics/b_sm_sc.pcx");

   RegBMP("button_turn_left" , "graphics/b_t_left.pcx");
   RegBMP("button_turn_right", "graphics/b_t_rt.pcx");

/* RegBMP("button_forward","graphics/b_fwd.pcx");
   RegBMP("button_backward", "graphics/b_bwd.pcx");*/

   RegBMP("button_look_up"  , "graphics/b_t_up.pcx");
   RegBMP("button_look_down", "graphics/b_t_dn.pcx");

   RegBMP("button_brush"  , "graphics/b_brush.pcx");
   RegBMP("button_move"   , "graphics/b_move.pcx");
   RegBMP("button_split"  , "graphics/b_split.pcx");
   RegBMP("button_texture", "graphics/b_tex.pcx");
   RegBMP("button_copy"   , "graphics/b_copy.pcx");

   RegBMP("status_bar_title", "graphics/sb_title.pcx");

   RegBMP("button_tiny_up"    , "graphics/b_ty_up.pcx");
   RegBMP("button_tiny_down"  , "graphics/b_ty_dn.pcx");
   RegBMP("button_tiny_scroll", "graphics/b_ty_sc.pcx");

   RegBMP("button_hidden_toggle", "graphics/b_hidden.pcx");

   return TRUE;
#undef RegBMP
}


int InitDisplay(void)
{
   /* Load bitmaps */
   num_bitmaps = 0;

   if (!LoadBitmaps())
   {
      HandleError("InitDisplay", "Unable to load bitmaps.");
      HandleError("InitDisplay", "Make sure you unzipped with the -d option.");
      return FALSE;
   }

   /* Init QUI */
   if (!QUI_Init())
   {
      HandleError("InitDisplay", "Unable to initialize QUI.");
      return FALSE;
   }

   return TRUE;
}

void DisposeDisplay(void)
{
/*   int i;
   bitmap_t *b;

   for (i=0;i<MAX_NUM_BITMAPS;i++)
   {
      b=&(bitmap[i]);
      switch (b->used)
      {
      case 1:
         printf("Allocated but unused '%s'!\n",b->name);
         break;
      case 2:
         printf("Allocated and used '%s'!\n",b->name);
         break;
      }
   }*/
}


void InitMapDisplay(void)
{
   /* VPORTS */
   InitViewports();
}

void ClearViewport(int vport_num)
{
   int    width;
   int    i;
   unsigned char *alias;
   viewport_t *v;

   v=&M.display.vport[vport_num];
   width = v->xmax - v->xmin;
   alias = &video.ScreenBuffer[v->ymin*video.ScreenWidth + v->xmin];

   for (i=v->ymax - v->ymin;i;i--)
   {
      memset(alias, 0, width);
      alias+=video.ScreenWidth;
   }
}

void UpdateMap(void)
{
   int       i;
   int       new_vport = FALSE;
   brush_t  *b;
   entity_t *e;
   fsel_t   *f;
   int dx1,dy1,dz1,dx2,dy2,dz2;
   int origx,origy;
// int  ox,oy,oz;

   /* Click detected in a viewport */
   if (mouse.button&3)
   {
      /* see if it is a new viewport*/
      for (i=0; i<M.display.num_vports; i++)
      {
         if (!M.display.vport[i].used)
            continue;

         if ((mouse.x > M.display.vport[i].xmin) &&
             (mouse.x < M.display.vport[i].xmax) &&
             (mouse.y > M.display.vport[i].ymin) &&
             (mouse.y < M.display.vport[i].ymax))
         {
            if (M.display.active_vport != i)
               new_vport = TRUE;
            M.display.active_vport = i;
            break;
         }
      }

      if (new_vport)
         UpdateAllViewports();


      /* Left mouse click in active vport */
      if ((mouse.button==1)&&(!new_vport))
      {
         switch (status.edit_mode)
         {
         case BRUSH:
            HandleLeftClickBrush();
            break;
         case FACE:
            HandleLeftClickFace();
            break;
         case ENTITY:
            HandleLeftClickEntity();
            break;
         case MODEL:
            HandleLeftClickModel();
            break;
         }
      }
      else
      if (mouse.button==2)
      {
      /*
       TODO: ctrl-right click + drag -> look around?
      */
         origx=mouse.x;
         origy=mouse.y;
         i=0;
         while (mouse.button!=0)
         {
            GetMousePos();
            if (mouse.moved)
            {
               i=1;

               dx1=dy1=dz1=0;
               dx2=dy2=dz2=0;

               if (origx>mouse.x)
               {
                  Move(M.display.active_vport, MOVE_LEFT, &dx1, &dy1, &dz1, origx-mouse.x);
               }
               else
               if (origx<mouse.x)
               {
                  Move(M.display.active_vport, MOVE_RIGHT, &dx1, &dy1, &dz1, mouse.x-origx);
               }

               if (mouse.button==2)
               {
                  if (origy>mouse.y)
                  {
                     Move(M.display.active_vport, MOVE_UP, &dx2, &dy2, &dz2, origy-mouse.y);
                  }
                  else
                  if (origy<mouse.y)
                  {
                     Move(M.display.active_vport, MOVE_DOWN, &dx2, &dy2, &dz2, mouse.y-origy);
                  }
               }

               if (mouse.button==3)
               {
                  if (origy>mouse.y)
                  {
                     Move(M.display.active_vport, MOVE_FORWARD, &dx2, &dy2, &dz2, origy-mouse.y);
                  }
                  else
                  if (origy<mouse.y)
                  {
                     Move(M.display.active_vport, MOVE_BACKWARD, &dx2, &dy2, &dz2, mouse.y-origy);
                  }
               }
               if (status.flip_mouse)
               {
                  M.display.vport[M.display.active_vport].camera_pos.x -= (dx1 + dx2);
                  M.display.vport[M.display.active_vport].camera_pos.y -= (dy1 + dy2);
                  M.display.vport[M.display.active_vport].camera_pos.z -= (dz1 + dz2);
               }
               else
               {
                  M.display.vport[M.display.active_vport].camera_pos.x += (dx1 + dx2);
                  M.display.vport[M.display.active_vport].camera_pos.y += (dy1 + dy2);
                  M.display.vport[M.display.active_vport].camera_pos.z += (dz1 + dz2);
               }
               UpdateViewport(M.display.active_vport,TRUE);
               SetMousePos(mouse.prev_x,mouse.prev_y);
            }
         }
         if (!i && (status.pop_menu==2))
         {
            MenuPopUp();
         }
      }
      while (mouse.button!=0)
         UpdateMouse();
   }

   switch (status.edit_mode)
   {
      case BRUSH:
         b = FindBrush(mouse.x, mouse.y);
         if (b != NULL)
         {
            M.cur_brush = b;

            if (b->bt->flags&BR_F_BTEXDEF)
               strcpy(M.cur_texname, b->tex.name);
            else
               strcpy(M.cur_texname, b->plane[0].tex.name);

            QUI_RedrawWindow(STATUS_WINDOW);
            RefreshPart(Q.window[STATUS_WINDOW].pos.x,
                        Q.window[STATUS_WINDOW].pos.y,
                        Q.window[STATUS_WINDOW].pos.x + Q.window[STATUS_WINDOW].size.x,
                        Q.window[STATUS_WINDOW].pos.y + Q.window[STATUS_WINDOW].size.y);
         }
         else
         {
            if (M.cur_texname[0] != '\0')
            {
               M.cur_texname[0] = '\0';
               QUI_RedrawWindow(STATUS_WINDOW);
               RefreshPart(Q.window[STATUS_WINDOW].pos.x,
                           Q.window[STATUS_WINDOW].pos.y,
                           Q.window[STATUS_WINDOW].pos.x + Q.window[STATUS_WINDOW].size.x,
                           Q.window[STATUS_WINDOW].pos.y + Q.window[STATUS_WINDOW].size.y);
            }
         }

         break;

      case FACE:
         f = FindFace(M.display.active_vport, mouse.x, mouse.y);
         if (f != NULL)
         {
            if (f->Brush->bt->flags&BR_F_BTEXDEF)
               strcpy(M.cur_texname, f->Brush->tex.name);
            else
               strcpy(M.cur_texname, f->Brush->plane[f->facenum].tex.name);

            M.cur_face.Brush = f->Brush;
            M.cur_face.facenum = f->facenum;

            QUI_RedrawWindow(STATUS_WINDOW);
            RefreshPart(Q.window[STATUS_WINDOW].pos.x,
                        Q.window[STATUS_WINDOW].pos.y,
                        Q.window[STATUS_WINDOW].pos.x + Q.window[STATUS_WINDOW].size.x,
                        Q.window[STATUS_WINDOW].pos.y + Q.window[STATUS_WINDOW].size.y);
            Q_free(f);
         }
         else
         {
            if (M.cur_texname[0] != '\0')
            {
               M.cur_texname[0] = '\0';

               M.cur_face.Brush = NULL;

               QUI_RedrawWindow(STATUS_WINDOW);
               RefreshPart(Q.window[STATUS_WINDOW].pos.x,
                           Q.window[STATUS_WINDOW].pos.y,
                           Q.window[STATUS_WINDOW].pos.x + Q.window[STATUS_WINDOW].size.x,
                           Q.window[STATUS_WINDOW].pos.y + Q.window[STATUS_WINDOW].size.y);
            }
         }

         break;


      case ENTITY:
         e = FindEntity(mouse.x, mouse.y);
         if (e != NULL)
         {
            M.cur_entity=e;

            QUI_RedrawWindow(STATUS_WINDOW);
            RefreshPart(Q.window[STATUS_WINDOW].pos.x,
                        Q.window[STATUS_WINDOW].pos.y,
                        Q.window[STATUS_WINDOW].pos.x + Q.window[STATUS_WINDOW].size.x,
                        Q.window[STATUS_WINDOW].pos.y + Q.window[STATUS_WINDOW].size.y);
         }

         break;
   }
}

