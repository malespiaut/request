/*
texfull.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "texfull.h"

#include "button.h"
#include "color.h"
#include "error.h"
#include "file.h"
#include "game.h"
#include "keyboard.h"
#include "memory.h"
#include "mouse.h"
#include "qui.h"
#include "status.h"
#include "tex.h"
#include "video.h"


#undef BG_COLOR
#define BG_COLOR 3

#ifndef WINDOW_MODE
#define MOUSE_LIP 2048
#endif

static int LeftX;
static button_t *TexButtons;
static texture_t *Textures;
static int *TexCached;
static int NumTextures;


static int col_bg,col_15;



static void DrawTextures(void)
{
   int i,j;
   int x1,x2,y;
   int sizeadj;
   int leftadj;

   for (i=0;i<NumTextures;i++)
   {
      if (!Textures[i].data) continue;
      
      x1 = TexButtons[i].x;
      x1 -= LeftX;
      x2 = TexButtons[i].x+TexButtons[i].sx-1;
      x2 -= LeftX;
      if (!((x1<video.ScreenWidth)&&(x2>=0)))
         continue;
         
      y=TexButtons[i].y;
      leftadj = 0;
      sizeadj = TexButtons[i].sx;
      if (x1<0)
      {
         sizeadj+=x1;
         leftadj=-x1;
         x1=0;
      }
      else
      {
         if (TexCached[i] && (x1>0))
         {
            x1--;
            for (j=-1;j<=Textures[i].dsy;j++)
               video.ScreenBuffer[x1+(y+j)*video.ScreenWidth]=col_15;
            x1++;
         }
      }

      if (x2>=video.ScreenWidth)
      {
         sizeadj-=(x2-(video.ScreenWidth-1));
         x2=video.ScreenWidth-1;
      }
      else
      {
         if (TexCached[i] && (x2<video.ScreenWidth-1))
         {
            x2++;
            for (j=-1;j<=Textures[i].dsy;j++)
               video.ScreenBuffer[x2+(y+j)*video.ScreenWidth]=col_15;
            x2--;
         }
      }

      if (TexCached[i])
      {
         memset(&(video.ScreenBuffer[x1+(y-2)*(video.ScreenWidth)]),
                col_15,sizeadj);
         memset(&(video.ScreenBuffer[x1+(y+Textures[i].dsy+1)*(video.ScreenWidth)]),
                col_15,sizeadj);
      }

      if (Textures[i].data)
      {
         for (j=0;j<Textures[i].dsy;j++)
         {
            memcpy(&(video.ScreenBuffer[x1+(y+j)*(video.ScreenWidth)]),
                   &(Textures[i].data[leftadj + j*Textures[i].dsx]),sizeadj);
         }
      }
   }
}

static char bookmarks[64][10];

int TexPickFull(char *PickedName, int NumT,struct texture_s *Tex)
{
   int Redraw;
   char *OldScreen;
#ifndef WINDOW_MODE
   int OldMX,OldMY;
#endif
   int BUTTONHIT=-1,OldSelect=-1,CurSelect=-1;
   int i,j;
   int x,y;
   int FinalX,OldLeftX,MaxX,MaxY;
   int Textx1,Texty1,Textx2,Texty2;
   int HitIt;
   texture_t *t;

   char tdesc[128];


   col_15=GetColor2(15);
   col_bg=GetColor2(BG_COLOR);

   NumTextures = NumT;
#ifndef WINDOW_MODE
   GetMousePos();
   OldMX=mouse.x;
   OldMY=mouse.y;
#endif

   Textures = Tex;
   if (Textures==NULL)
   {
      HandleError("TexPickFull","Not enough memory to load textures");
      return FALSE;
   }

   TexCached=Q_malloc(sizeof(int)*NumTextures);
   if (!TexCached)
   {
      HandleError("Texture Picker","Out of memory!");
      return FALSE;
   }

   TexButtons = (button_t *)Q_malloc(sizeof(button_t)*NumTextures);
   if (TexButtons==NULL)
   {
      HandleError("Texture Picker","Not enough memory to hold texture buttons.");
      return FALSE;
   }
   
   x=2;
   y=Q.window[STATUS_WINDOW].size.y+2;
   MaxX=0;
   MaxY=0;
   for (i=0;i<NumTextures;i++)
   {
      if (!Textures[i].data) continue;
   
      if ((y+Textures[i].dsy)<(video.ScreenHeight-Q.window[STATUS_WINDOW].size.y))
      {
         if (Textures[i].dsx>MaxX) MaxX=Textures[i].dsx;
         y+=(Textures[i].dsy+16);
      }
      else
      {
         x+=(MaxX+16);
         y=Q.window[STATUS_WINDOW].size.y+2;
         MaxX=Textures[i].dsx;
         y+=(Textures[i].dsy+16);
      }
   }
   MaxX += x;
   FinalX = MaxX+3;
   
   x=2;
   y=Q.window[STATUS_WINDOW].size.y+2;
   MaxX=0;
   MaxY=0;
   for (i=0;i<NumTextures;i++)
   {
      if (!Textures[i].data) continue;

      if (!PickedName)
      {
         if (FindTexture(Textures[i].name)==-1)
            TexCached[i]=0;
         else
            TexCached[i]=1;
      }
      else
         TexCached[i]=0;

      if ((y+Textures[i].dsy)<(video.ScreenHeight-Q.window[STATUS_WINDOW].size.y))
      {
         TexButtons[i].x = x;
         TexButtons[i].y = y;
         TexButtons[i].sx = Textures[i].dsx;
         TexButtons[i].sy = Textures[i].dsy;
         TexButtons[i].data=Q_strdup(Textures[i].name);
         if (!TexButtons[i].data)
            Abort("TexPickFull","Out of memory!");
         if (Textures[i].dsx>MaxX) MaxX=Textures[i].dsx;
         y+=(Textures[i].dsy+16);
      }
      else
      {
         x+=(MaxX+16);
         y=Q.window[STATUS_WINDOW].size.y+2;
         TexButtons[i].x = x;
         TexButtons[i].y = y;
         TexButtons[i].sx = Textures[i].dsx;
         TexButtons[i].sy = Textures[i].dsy;
         TexButtons[i].data=Q_strdup(Textures[i].name);
         if (!TexButtons[i].data)
            Abort("TexPickFull","Out of memory!");
         MaxX=Textures[i].dsx;
         y+=(Textures[i].dsy+16);
      }
   }
   
   OldScreen = (char *)Q_malloc(video.ScreenWidth*video.ScreenHeight);
   if (OldScreen==NULL)
   {
      Q_free(TexButtons);
      HandleError("Texture Picker","Not enough memory to hold temp buffer");
      return FALSE;
   }

   memcpy(OldScreen,video.ScreenBuffer,video.ScreenWidth*video.ScreenHeight);
   for (i=Q.window[STATUS_WINDOW].size.y;i<video.ScreenHeight;i++)
   {
      memset(&(video.ScreenBuffer[i*video.ScreenWidth]),col_bg,video.ScreenWidth);
   }

   Textx1 = 0;
   Textx2 = video.ScreenWidth-1;
   Texty1 = video.ScreenHeight - ROM_CHAR_HEIGHT - 1;
   Texty2 = video.ScreenHeight-1;
   RefreshScreen();

#ifndef WINDOW_MODE
   DrawMouse(mouse.x,mouse.y);
   SetMouseLimits(0,0,video.ScreenWidth+2*MOUSE_LIP-1, video.ScreenHeight-1);
   SetMousePos(video.ScreenWidth/2+MOUSE_LIP,video.ScreenHeight/2);
#endif

   LeftX = 0;
   OldSelect=-1;
   DrawTextures();
   RefreshScreen();

   while (1)
   {
      if (mouse.button==1 && PickedName)
         break;

#ifndef WINDOW_MODE
      if (mouse.button==2)
         break;
#else
      if (TestKey(KEY_ESCAPE))
         break;
#endif

      ClearKeys();
#ifdef WINDOW_MODE
      UpdateMouse();
#else
      mouse.x+=MOUSE_LIP;
      GetMousePos();
      mouse.x-=MOUSE_LIP;
#endif

      Redraw=FALSE;
#ifdef WINDOW_MODE
      {
         int stx,sty;
         stx=mouse.x;
         sty=mouse.y;
         while (mouse.button==2)
         {
            if (mouse.moved)
            {
               OldLeftX=LeftX;
               LeftX+=mouse.prev_x-mouse.x;
               if (LeftX<0) LeftX=0;
               if (LeftX>FinalX-video.ScreenWidth-1)
                  LeftX=FinalX-video.ScreenWidth-1;
               for (i=Q.window[STATUS_WINDOW].size.y;i<video.ScreenHeight-ROM_CHAR_HEIGHT-1;i++)
               {
                 memset(&(video.ScreenBuffer[i*video.ScreenWidth]),col_bg,video.ScreenWidth);
               }       
               DrawTextures();
               WaitRetr();
               RefreshScreen();
/*               UndrawMouse(mouse.prev_x-OldLeftX, mouse.prev_y);
               DrawMouse(mouse.x-LeftX, mouse.y);*/
               SetMousePos(stx,sty);
            }
            UpdateMouse();
         }
      }
#else // WINDOW_MODE
      if (mouse.moved)
      {
         OldLeftX=LeftX;
         if (mouse.x>video.ScreenWidth-1)
         {
            LeftX+=(mouse.x - (video.ScreenWidth-1));
            if (LeftX+video.ScreenWidth>=FinalX)
               LeftX=FinalX-video.ScreenWidth-1;
            if (LeftX<0) LeftX=0;

            SetMousePos(MOUSE_LIP+video.ScreenWidth-1,mouse.y);
            mouse.x-=MOUSE_LIP;
            Redraw=TRUE;
         }
         else
         if (mouse.x<0)
         {
            LeftX+=mouse.x;
            if (LeftX<0) LeftX=0;

            SetMousePos(MOUSE_LIP,mouse.y);
            mouse.x-=MOUSE_LIP;
            Redraw=TRUE;
         }
         if ((Redraw==TRUE) && (OldLeftX!=LeftX))
         {
            for (i=Q.window[STATUS_WINDOW].size.y;i<video.ScreenHeight-ROM_CHAR_HEIGHT-1;i++)
            {
               memset(&(video.ScreenBuffer[i*video.ScreenWidth]),col_bg,video.ScreenWidth);
            }
            DrawTextures();
            WaitRetr();
            RefreshScreen();
         }
         UndrawMouse(mouse.prev_x-MOUSE_LIP, mouse.prev_y);
         DrawMouse(mouse.x, mouse.y);
      }
#endif

      HitIt = FALSE;
/*      if (X_texture_move_mode)
      {
         for (i=0;i<NumTextures;i++)
         {
            if (!Textures[i].data) continue;
            if (InBox(TexButtons[i].x-LeftX,TexButtons[i].y,
                      TexButtons[i].x-LeftX+TexButtons[i].sx,
                      TexButtons[i].y+TexButtons[i].sy))
            {
              BUTTONHIT=i;
              CurSelect=i;
              HitIt=TRUE;
            }
         }
      }
      else*/
      {
         for (i=0;i<NumTextures;i++)
         {
            if (!Textures[i].data) continue;
            if (InBox(TexButtons[i].x-LeftX,TexButtons[i].y,
                      TexButtons[i].x-LeftX+TexButtons[i].sx,
                      TexButtons[i].y+TexButtons[i].sy))
            {
               BUTTONHIT=i;
               CurSelect=i;
               HitIt=TRUE;
            }
         }
      }
         
      if (HitIt==TRUE)
      {
         if (CurSelect!=OldSelect)
         {
            if (OldSelect!=-1)
            {
               t=&Textures[OldSelect];

               QUI_DrawStr(Textx1,Texty1,BG_COLOR,BG_COLOR,0,TRUE,
                  "%32s %ix%i",
                  t->name,t->rsx,t->rsy);

               GetTexDesc(tdesc,t);
               QUI_DrawStr(Textx1+360,Texty1,BG_COLOR,BG_COLOR,0,TRUE,
                  "%s",tdesc);
            }

            t=&Textures[CurSelect];

            QUI_DrawStr(Textx1,Texty1,BG_COLOR,15,0,TRUE,
               "%32s %ix%i",
               t->name,t->rsx,t->rsy);

            GetTexDesc(tdesc,t);
            QUI_DrawStr(Textx1+360,Texty1,BG_COLOR,15,0,TRUE,
               "%s",tdesc);

            DrawMouse(mouse.x,mouse.y);
            OldSelect=CurSelect;
         }
      }
      else
      { /* (He isnt on anything) */
         if (OldSelect!=-1)
         {
            t=&Textures[OldSelect];

            QUI_DrawStr(Textx1,Texty1,BG_COLOR,BG_COLOR,0,TRUE,
               "%32s %ix%i",
               t->name,t->rsx,t->rsy);

            GetTexDesc(tdesc,t);
            QUI_DrawStr(Textx1+360,Texty1,BG_COLOR,BG_COLOR,0,TRUE,
               "%s",tdesc);
               
            DrawMouse(mouse.x,mouse.y);
            OldSelect=-1;
         }
      }

      for (i=0;i<10;i++)
      {
         if (TestKey(k_ascii[i+'0']))
         {
            if (TestKey(KEY_ALT))
            {
               for (j=0;j<NumTextures;j++)
               {
                  if (!Textures[j].data) continue;
                  
                  if (TexButtons[j].x>=LeftX)
                  {
                     strcpy(bookmarks[i],TexButtons[j].data);
                     break;
                  }
               }
            }
            else
            if (bookmarks[i])
            {
               for (j=0;j<NumTextures;j++)
               {
                  if (!Textures[j].data) continue;
                  if (!strcmp(bookmarks[i],TexButtons[j].data))
                  {
                     LeftX=TexButtons[j].x-2;
                     if (LeftX<0) LeftX=0;
                     if (LeftX+video.ScreenWidth-1>FinalX)
                        LeftX=FinalX-video.ScreenWidth+1;

                     for (i=Q.window[STATUS_WINDOW].size.y;i<video.ScreenHeight-ROM_CHAR_HEIGHT-1;i++)
                     {
                        memset(&(video.ScreenBuffer[i*video.ScreenWidth]),col_bg,video.ScreenWidth);
                     }
                     DrawTextures();
                     WaitRetr();
                     RefreshScreen();

/*                     SetMousePos(video.ScreenWidth/2,video.ScreenHeight/2);
                     DrawMouse(mouse.x, mouse.y);*/
                     break;
                  }
               }
            }
            while (TestKey(k_ascii[i+'0'])) ;
            break;
         }
      }

      if ((mouse.button==1) && !PickedName && (OldSelect!=-1))
      {
         if (TexCached[CurSelect])
            TexCached[CurSelect]=0;
         else
            TexCached[CurSelect]=1;

         for (i=Q.window[STATUS_WINDOW].size.y;i<video.ScreenHeight-ROM_CHAR_HEIGHT-1;i++)
         {
            memset(&(video.ScreenBuffer[i*video.ScreenWidth]),col_bg,video.ScreenWidth);
         }
         DrawTextures();
         WaitRetr();
         RefreshScreen();

         while (mouse.button)
         {
#ifdef WINDOW_MODE
            UpdateMouse();
#else
            GetMousePos();
            if (mouse.moved)
            {
               UndrawMouse(mouse.prev_x-MOUSE_LIP,mouse.prev_y);
               DrawMouse(mouse.x-MOUSE_LIP,mouse.y);
            }
#endif
         }
      }
   }

   while (mouse.button!=0) UpdateMouse();

   if ((OldSelect!=-1) && (PickedName))
      strcpy(PickedName,(TexButtons[OldSelect].data));
   memcpy(video.ScreenBuffer,OldScreen,video.ScreenWidth*video.ScreenHeight);

   RefreshScreen();
   Q_free(OldScreen);
   for (i=0;i<NumTextures;i++)
   {
      if (!Textures[i].data) continue;
      
      if (!PickedName)
      {
         if (TexCached[i])
            ReadMIPTex(TexButtons[i].data,0);
         else
            RemoveTex(TexButtons[i].data);
      }

      Q_free(TexButtons[i].data);
   }
   Q_free(TexButtons);

   Q_free(TexCached);

#ifndef WINDOW_MODE
   SetMouseLimits(0,0,video.ScreenWidth-1, video.ScreenHeight-1);
   SetMousePos(OldMX,OldMY);
   mouse.prev_x=mouse.x;
   mouse.prev_y=mouse.y;
   UpdateMouse();
#endif
   if (OldSelect!=-1)
      return TRUE;
   else
      return FALSE;
}

