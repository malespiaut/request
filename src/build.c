/*
build.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef DJGPP
#include <bios.h>
#endif

#include "defines.h"
#include "types.h"

#include "build.h"

#include "3d.h"
#include "button.h"
#include "color.h"
#include "error.h"
#include "file.h"
#include "game.h"
#include "keyboard.h"
#include "map.h"
#include "memory.h"
#include "mouse.h"
#include "quest.h"
#include "video.h"
#include "token.h"
#include "qui.h"


/*
Autobuild popup stuff.

In build.cfg,

this     is replaced with this (examples assume c:/quest/test.map):
 %%       %
 %n       Just the name of the map ('test').
 %N       The name with the directory, normal slashes ('c:/quest/test').
 %M       The name with the directory, backslashes ('c:\quest\test').

*/


typedef struct
{
   char *desc;
   char **cmds;
   int ncmds;
} build_cfg_t;


static build_cfg_t game_cfg;  // special config to run the game

static build_cfg_t *bcfg;
static int n_bcfg;


static int LoadBCfg(void)
{
   build_cfg_t *b;
   char line[256];
   int first;


   FindFile(line,"build.cfg");

   if (!TokenFile(line,T_C|T_MISC|T_STRING,NULL))
   {
      HandleError("LoadBCfg","Unable to load '%s'!","build.cfg");
      return 0;
   }

   bcfg=NULL;
   n_bcfg=0;
   first=1;

   while (TokenGet(1,-1))
   {
      if (first)
      {
         first=0;
         b=&game_cfg;
      }
      else
      {
         bcfg=Q_realloc(bcfg,sizeof(build_cfg_t)*(n_bcfg+1));
         b=&bcfg[n_bcfg++];
         memset(b,0,sizeof(build_cfg_t));

         if (token[0]=='"')
         {
            strcpy(token,&token[1]);
            token[strlen(token)-1]=0;
         }
         b->desc=Q_strdup(token);
         TokenGet(1,-1);
      }

      if (strcmp(token,"{"))
         Abort("LoadBCfg","{");

      TokenGet(1,T_ALLNAME);
      while (strcmp(token,"}"))
      {
         strcpy(line,token);
         while (TokenAvailable(0))
         {
            TokenGet(1,T_ALLNAME);
            strcat(line," ");
            strcat(line,token);
         }

         b->cmds=Q_realloc(b->cmds,sizeof(char *)*(b->ncmds+1));
         b->cmds[b->ncmds++]=Q_strdup(line);

         TokenGet(1,T_ALLNAME);
      }
   }

   TokenDone();

/*   {
      int i,j;

      for (i=0;i<n_bcfg;i++)
      {
         b=&bcfg[i];
         printf("Dest: '%s'  Target: '%s' ncmds=%i\n",
            b->desc,b->target,b->ncmds);
         for (j=0;j<b->ncmds;j++)
         {
            printf("  '%s'\n",b->cmds[j]);
         }
      }
   }*/

   return 1;
}


#include <unistd.h>

static char name_n[256];  // %n
static char name_N[256];  // %N
static char name_M[256];  // %M

static void SetName(char *mapname)
{
   char *c;

   strcpy(name_n,mapname);

   for (c=name_n;*c;c++)
      if (*c=='\\') *c='/';

   if (strrchr(name_n,'.'))
      *strrchr(name_n,'.')=0;

   if ((name_n[1]==':') || (name_n[0]=='/'))
   {
      strcpy(name_N,name_n);
   }
   else
   {
      getcwd(name_N,sizeof(name_N) - strlen(name_n) - 1);
      strcat(name_N,"/");
      strcat(name_N,name_n);
   }

   strcpy(name_M,name_N);
   for (c=name_M;*c;c++)
      if (*c=='/') *c='\\';

   if (strrchr(name_n,'/'))
      strcpy(name_n,strrchr(name_n,'/')+1);

//   printf("'%s' '%s' '%s'\n",name_n,name_N,name_M);
}


static void ExpandStr(char *dest,char *src)
{
   char *c,*d;
   const char *e;

   c=dest;
   for (d=src;*d;d++)
   {
      if (*d=='%')
      {
         d++;
         if (!*d)
            continue;

         switch (*d)
         {
         case 'n':
            e=name_n;
            break;
         case 'N':
            e=name_N;
            break;
         case 'M':
            e=name_M;
            break;
         case '%':
            e="%";
            break;
         default:
            e=NULL;
            break;
         }
         if (e)
         {
            strcpy(c,e);
            c+=strlen(e);
         }
      }
      else
         *c++=*d;
   }
   *c=0;
}

#ifdef _UNIX
#define SCRIPT_NAME "./questtmp.sh"
#else
#define SCRIPT_NAME "questtmp.bat"
#endif

static void RunCfg(build_cfg_t *b)
{
   char temp[256];
   int i;
   char olddir[256];
   FILE *f;

   if (!b->ncmds)
      return;

   f=fopen(SCRIPT_NAME,"wt");
   if (f == NULL) {
     fprintf(stderr, "Can't write " SCRIPT_NAME ": %s\n", strerror(errno));
     return;
   }
#ifdef _UNIX
   fprintf(f, "#!/bin/sh\n\n");
#endif
   for (i=0;i<b->ncmds;i++)
   {
      ExpandStr(temp,b->cmds[i]);
      fprintf(f,"%s\n",temp);
   }
   fclose(f);
   getcwd(olddir,sizeof(olddir));
   system(SCRIPT_NAME);
   chdir(olddir);
}


// Remember these when the dialog closes.
static int run_game=1;       // run the game after compiling
static int just_visible=0;   // only compile the visible part of the map
static int add_shell=0;      // add a shell around the map to prevent leaks

void AutoBuildPopup(void)
{
	QUI_window_t *w;
	unsigned char *temp_buf;

	int *op;
	int b_cancel;
   int b_game,b_vis,b_shell;
   int bp;

	int i;
   int select;
   int done;

   char mapname[128];


   if (!n_bcfg)
   {
      if (!LoadBCfg()) return;
   }

   if (M.modified)
   {
      if (QUI_YesNo(
             "Modified map",
             "The map has been modified since the last save. Save now?",
             "Save",
             "No"))
      {
         Game.map.savemap(M.mapfilename);
      }
   }

	/* Set up window Position */
	w = &Q.window[POP_WINDOW_1 + Q.num_popups];
	w->size.x = 500;
	w->size.y = 88+(n_bcfg*22);
	w->pos.x = (video.ScreenWidth - w->size.x) / 2;
	w->pos.y = (video.ScreenHeight - w->size.y) / 2;

	/* Make the option buttons */
   op=Q_malloc(sizeof(int)*n_bcfg);
   if (!op)
   {
      HandleError("AutoBuildPopup","Out of memory!");
      return;
   }

   PushButtons();

   for (i=0;i<n_bcfg;i++)
      op[i]=AddButtonText(0,0,0," ");

	b_cancel = AddButtonText(0,0,B_ESCAPE,"Cancel");

   b_game = AddButtonText(0,0,B_TOGGLE,"Run game");
   b_vis  = AddButtonText(0,0,B_TOGGLE,"Only visible");
   b_shell= AddButtonText(0,0,B_TOGGLE,"Add shell");

   for (i=0;i<n_bcfg;i++)
   	MoveButton(op[i], w->pos.x + 20, w->pos.y + 35 + 22*i);

	MoveButton(b_cancel,w->pos.x+4,w->pos.y+w->size.y-30);

   button[b_game].y=button[b_vis].y=button[b_shell].y=button[b_cancel].y;

   button[b_game ].x=button[b_cancel].x+button[b_cancel].sx+4;
   button[b_vis  ].x=button[b_game  ].x+button[b_game  ].sx+4;
   button[b_shell].x=button[b_vis   ].x+button[b_vis   ].sx+4;

	/* Actually draw the window */
	QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Select Build", &temp_buf);
	Q.num_popups++;

   ToggleButton(b_game ,run_game);
   ToggleButton(b_vis  ,just_visible);
   ToggleButton(b_shell,add_shell);

	/* Draw the buttons */
   DrawButtons();

   for (i=0;i<n_bcfg;i++)
	   QUI_DrawStrM(w->pos.x+45,w->pos.y+40+22*i,w->pos.x+w->size.x-4,
	      BG_COLOR,0,0,FALSE,
	      "%s",bcfg[i].desc);

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

   done=0;
   select=-1;
	while (!done)
	{
      UpdateMouse();
      bp=UpdateButtons();

      if (bp!=-1)
      {
         for (i=0;i<n_bcfg;i++)
         {
            if (bp==op[i])
            {
               select=i;
               done=1;
            }
         }
         if (bp==b_cancel)
            done=1;

         if (bp==b_game)
         {
            run_game=!run_game;
            ToggleButton(b_game,run_game);
            RefreshButton(b_game);
         }
         if (bp==b_vis)
         {
            just_visible=!just_visible;
            ToggleButton(b_vis,just_visible);
            RefreshButton(b_vis);

            if (!just_visible)
            {
               add_shell=0;
               ToggleButton(b_shell,add_shell);
               RefreshButton(b_shell);
            }
         }
         if ((bp==b_shell) && just_visible)
         {
            add_shell=!add_shell;
            ToggleButton(b_shell,add_shell);
            RefreshButton(b_shell);
         }
      }
	}

   for (i=0;i<n_bcfg;i++)
      RemoveButton(op[i]);
   Q_free(op);
	RemoveButton(b_cancel);
   RemoveButton(b_game);
   RemoveButton(b_vis);
   RemoveButton(b_shell);
   PopButtons();

	/* Pop down the window */
	Q.num_popups--;
	QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &temp_buf);

	RefreshPart(w->pos.x, w->pos.y, w->pos.x+w->size.x, w->pos.y+w->size.y);

	if (select!=-1)
   {
		if (just_visible)
      {
         strcpy(mapname,"questtmp.map");
			Game.map.savevisiblemap(mapname,add_shell);
		}
      else
      {
         if (!M.mapfilename[0])
         {
            HandleError("AutoBuildPopup","Map has no name!");
            return;
         }
         strcpy(mapname,M.mapfilename);
      }

      SetName(mapname);

		SetMode(RES_TEXT);

      KBD_RemoveHandler();

      RunCfg(&bcfg[select]);

      if (run_game)
      {
         printf("Press any key to run the game or ESC to return to Quest...");
         fflush(stdout);
#ifdef DJGPP
         if (bioskey(0)!=0x011b)   // check for escape
#endif
#ifdef _UNIX
//#error Unimplemented!
#endif
            RunCfg(&game_cfg);
      }
      else
      {
         printf("Press any key to continue...");
         fflush(stdout);
#ifdef DJGPP
         bioskey(0);
#endif
#ifdef _UNIX
//#error Unimplemented!
#endif
      }

      KBD_InstallHandler();

		SetMode(options.vid_mode);

		InitMouse();
		SetPal(PAL_QUEST);
		QUI_RedrawWindow(MESG_WINDOW);
		QUI_RedrawWindow(TOOL_WINDOW);
		QUI_RedrawWindow(STATUS_WINDOW);
		UpdateAllViewports();
	}
}

