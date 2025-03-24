/*
quest.c file of the Quest Source Code

Copyright 1997, 1998, 1999, 2000 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "version.h"

#include "defines.h"
#include "types.h"

#include "quest.h"

#include "3d.h"
#include "button.h"
#include "camera.h"
#include "cfgs.h"
#include "color.h"
#include "config.h"
#include "display.h"
#include "dvport.h"
#include "entclass.h"
#include "entity.h"
#include "error.h"
#include "game.h"
#include "keyboard.h"
#include "keycfg.h"
#include "keyhit.h"
#include "leak.h"
#include "map.h"
#include "memory.h"
#include "menu.h"
#include "message.h"
#include "mouse.h"
#include "newgroup.h"
#include "qui.h"
#include "signals.h"
#include "status.h"
#include "tex.h"
#include "texcat.h"
#include "times.h"
#include "tool.h"
#include "video.h"

#ifdef _UNIX
#include <sys/stat.h>

extern void X_SetWindowTitle(char* filename);
#endif

// set to zero to see timings
#if (1)

#define Time(x) x
#define TimeR(x) x

#else

#define TimeR(x)                                                                             \
  ({                                                                                         \
    float start;                                                                             \
    float end;                                                                               \
    typeof(x) r;                                                                             \
                                                                                             \
    start = GetTime();                                                                       \
    r = x;                                                                                   \
    end = GetTime();                                                                         \
    printf("--- start=%-5.3f end=%-5.3f elap=%-5.3f %s ---\n", start, end, end - start, #x); \
    r;                                                                                       \
  })

#define Time(x)                                                                              \
  ({                                                                                         \
    float start;                                                                             \
    float end;                                                                               \
                                                                                             \
    start = GetTime();                                                                       \
    x;                                                                                       \
    end = GetTime();                                                                         \
    printf("--- start=%-5.3f end=%-5.3f elap=%-5.3f %s ---\n", start, end, end - start, #x); \
  })

#endif

// only print if status.debug is 1
void
dprint(const char* format, ...)
{
  va_list args;

  if (!status.debug)
    return;

  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

float mouse_sens_x = 1; /* mouse sensitivity, default value */
float mouse_sens_y = 1;

clipboard_t Clipboard; /* used for cutting pasting, obviously */

char texturename[256]; /* holds texture name */

char profile[256]; // The current profile, if any.

static char aprofile[256]; // Profile on command line, if any.

// Stuff to allow multiple maps.
map_t maps[MAX_MAPS];
map_t M;     // Current map. Not a pointer for speed reasons.
int cur_map; // number of current map, as it needs to be copied back to
             // maps[?] when switching

void
SwitchMap(int newmap, int verbose)
{
  if (newmap == cur_map)
    return;

  if (newmap >= MAX_MAPS)
    return;

  maps[cur_map] = M;
  M = maps[newmap];
  cur_map = newmap;
  if (verbose)
    NewMessage("Switched to map %i.", newmap + 1);
#ifdef _UNIX
  X_SetWindowTitle(M.mapfilename);
#endif
}

options_t options; /* Global options that may be set*/

int take_screenshot; /* set it to true and a screenshot will */
                     /* be taken at the next mouse update */

float gammaval; /* used for gamma control stuff */

int MenuShowing; /* used in hiding / showing the menu */

char** argv; /* to allow access anywhere */
int argc;

// argument parsing

typedef struct
{
  const char* name; // long name (--help)
  const char* sh;   // one letter (-h) or NULL
  const char* desc; // description
  int pass;         // what pass to call this in

  int action; // what to do
  void* data; // parameter to action
} arg_t;

// actions
#define ACT_CALL 0   // call the function void data(void)
#define ACT_SET1 1   // *data=1
#define ACT_SETSTR 2 // strcpy(data,next_param)

// passes
#define P_0 0      // the first pass, before anything else
#define P_LAST 100 // the last pass, just before main loop starts

static void ARG_Help(void);

static arg_t args[] =
  {
    {"help", "h", "Get help on arguments", P_0, ACT_CALL, ARG_Help},

    {"key-help", NULL, "Generate keys.hlp file", P_LAST, ACT_CALL, WriteKeyHelp},
    {"key-def", NULL, "Generate keys.def file", P_LAST, ACT_CALL, WriteKeyDef},

    {"menu-help", NULL, "Generate menu.hlp file", P_LAST, ACT_CALL, WriteMenuHelp},

    {"debug", "D", "Enable debugging mode", P_0, ACT_SET1, &status.debug},

    {"profile", "P", "Set current profile", P_0, ACT_SETSTR, aprofile},

    {0, 0, 0, 0, 0, 0}};
#define NUM_ARGS (sizeof(args) / sizeof(args[0]) - 1)

static void
ARG_Help(void)
{
  int i;
  int l;

  printf("Syntax:\n");
  printf("  %s", argv[0]);
  for (i = 0; i < NUM_ARGS; i++)
  {
    printf(" [--%s]", args[i].name);
  }
  printf("\n\n"
         "Name:          Description:\n");
  for (i = 0; i < NUM_ARGS; i++)
  {
    l = 0;
    l += printf("--%s", args[i].name);
    if (args[i].sh)
      l += printf(" (-%s)", args[i].sh);
    for (; l < 15; l++)
      printf(" ");
    printf("%s\n", args[i].desc);
  }
  exit(0);
}

static int curarg;

static void
ExecArg(arg_t* a)
{
  switch (a->action)
  {
    case ACT_CALL:
      ((void (*)(void))a->data)();
      break;
    case ACT_SET1:
      *((int*)(a->data)) = 1;
      break;
    case ACT_SETSTR:
      curarg++;
      if (curarg >= argc)
      {
        printf("Error in argument '%s'! Try '%s --help' for help.\n",
               argv[curarg],
               argv[0]);
        exit(1);
      }
      strcpy(a->data, argv[curarg]);
      break;
  }
}

static void
ParseArgs(int pass)
{
  int j;
  char* a;
  int cmap;

  cmap = 0;
  for (curarg = 1; curarg < argc; curarg++)
  {
    a = argv[curarg];
    if ((a[0] == '-') && (a[1] == '-'))
    {
      a++;
      a++;
      for (j = 0; j < NUM_ARGS; j++)
      {
        if (!strcmp(a, args[j].name))
        {
          if (args[j].pass == pass)
            ExecArg(&args[j]);
          break;
        }
      }
      if (j == NUM_ARGS)
      {
        printf("Unknown argument '%s'! Try '%s --help' for help.\n",
               argv[curarg],
               argv[0]);
        exit(1);
      }
    }
    else if ((a[0] == '-'))
    {
      a++;
      for (j = 0; j < NUM_ARGS; j++)
      {
        if (!strcmp(a, args[j].sh))
        {
          if (args[j].pass == pass)
            ExecArg(&args[j]);
          break;
        }
      }
      if (j == NUM_ARGS)
      {
        printf("Unknown argument '%s'! Try '%s --help' for help.\n",
               argv[curarg],
               argv[0]);
        exit(1);
      }
    }
    else
    {
      if (pass == P_0)
      {
        SwitchMap(cmap++, 0);
        strcpy(M.mapfilename, argv[curarg]);
      }
    }
  }
}

static void
InitQuest(int pargc, char** pargv)
{
  int i;

  printf("\n"
         "\n"
         "The \"Official\" new DOS/Win95 Quest " QUEST_VER " (" __DATE__ ")\n"
         "-----------------------------------------------------------------\n"
         " (c) Copyright 1997, 1998, 1999, 2000\n"
         "     Alexander Malmberg <alexander@malmberg.org>\n"
         "\n"
         " (c) Copyright 1996,\n"
         "     Chris Carollo\n"
         "     Trey Harrison\n"
         "\n"
         "Please read all of the documents contained in the Quest zipfile. They\n"
         "contain useful information.\n"
         "\n"
         "This program is distributed under the terms of the GNU General Public\n"
         "License as published by the Free Software Foundation. A copy of it is in\n"
         "the file legal.txt. This program comes with ABSOLUTELY NO WARRANTY.\n"
         "\n");
#ifdef BETA
  printf("This is a beta version of Quest. If you are not an official Quest beta\n"
         "tester, you are encouraged to use the latest stable version instead.\n"
         "\n");
#endif

  argv = pargv;
  argc = pargc;

  /* -------------- */
  /* INITIALIZATION */
  /* -------------- */
  memset(maps, 0, sizeof(maps)); // this should zero out ALL map stuff
  M = maps[0];                   // default map is map 0
  cur_map = 0;

  Clipboard.brushes = NULL;
  Clipboard.entities = NULL;
  Clipboard.ent_br = NULL;

  status.vid_mode = RES_TEXT;
  status.kbd_hand = FALSE;
  status.exit_flag = FALSE;
  status.edit_mode = BRUSH;
  status.depth_clip = 600;
  status.rotate = -1;
  status.scale = -1;
  status.move = FALSE;

  texturename[0] = 0;

  profile[0] = 0;

  ParseArgs(P_0);

  // read keyboard config from key.cfg
  Time(LoadKeyConfig("key.cfg"));

  /* map 0 must be the current map at this point, for LoadConfig and
     InitTextures */
  SwitchMap(0, 0);

  // read from the quest.cfg file
  Time(LoadConfig("quest.cfg", 1));

  { // check if we should load another .cfg file
    int loadcfg;
    char temp[256];

    loadcfg = 0;
    /*      if (M.mapfilename[0])
          {
             Game.map.mapprofile(M.mapfilename,temp);
             if (temp[0])
             {
                strcpy(profile,temp);
                loadcfg=1;
             }
          }*/

    if (aprofile[0])
    {
      strcpy(profile, aprofile);
      loadcfg = 1;
    }

    if (loadcfg)
    {
      strcpy(temp, profile);
      strcat(temp, ".cfg");
      Time(LoadConfig(temp, 0));
    }
  }

  // find which game we're editing and set the Game structure
  Time(InitGame());

  Time(InitCategories());

  // parse .qc files
  Time(InitEntClasses());

  for (i = 0; i < MAX_MAPS; i++)
  {
    SwitchMap(i, 0);

    InitMapDisplay();

    if (M.mapfilename[0])
    {
      char* d;
      int ext;

      ext = 1;
      for (d = M.mapfilename; *d; d++)
      {
        if (*d == '.')
          ext = 0;
        if ((*d == '/') || (*d == '\\'))
          ext = 1;
      }

      if (ext)
        strcat(M.mapfilename, ".map");

      if (!TimeR(Game.map.loadmap(M.mapfilename)))
      {
        Abort("main", "Unable to load %s.", M.mapfilename);
      }
    }
    else
    {
      M.mapfilename[0] = 0;

      /* This is (basically) what must be done when creating a new
         map. Clear out all the lists (vertices,entities,brushes,
         edges,groups,etc). Create a worldspawn entity, Init it,
         create a world group, init the camera */

      CreateWorldGroup();
      M.EntityHead = (entity_t*)Q_malloc(sizeof(entity_t));
      if (M.EntityHead == NULL)
      {
        Abort("InitQuest", "Unable to allocate new map.");
      }
      InitEntity(M.EntityHead);
      SetKeyValue(M.EntityHead, "classname", "worldspawn");
      M.WorldSpawn = M.EntityHead;
      M.num_entities = 1;
      M.BrushHead = NULL;
      InitCamera();
    }
  }

#ifdef _UNIX
  Time(InitSignals());
#endif

  if (!TimeR(LoadMenu("menu.txt", &Menus, &NumMenus, &List, &ListSize)))
  {
    Abort("InitQuest", "Unable to initialize menus.");
  }

  if (!TimeR(InitVideo(options.vid_mode)))
  {
    Abort("InitQuest", "Unable to initialize video.");
  }

#ifdef _UNIX
  X_SetWindowTitle(M.mapfilename);
#endif

  if (!TimeR(InitMouse()))
  {
    Abort("InitQuest", "Unable to initialize mouse.");
  }

  Time(InitButton());

  if (!TimeR(InitDisplay()))
  {
    Abort("InitQuest", "Unable to initialize display.");
  }

  for (i = 0; i < MAX_MAPS; i++)
  {
    SwitchMap(i, 0);
    UpdateViewportPositions();
  }

  Time(InitColor());

  Time(InitTool());

  Time(KBD_InstallHandler());

  MenuShowing = FALSE;

  /* Start off at map 0 */
  SwitchMap(0, 0);

  if (!status.pop_menu)
    Time(PopUpMenuWin());

  /* Update all viewports */
  Time(UpdateAllViewports());

  Time(RefreshScreen());

  DrawMouse(mouse.x, mouse.y);

  Time(FixColors());

  ParseArgs(P_LAST);
}

int
main(int argc, char** argv)
{
  int CurWindow;

  /* this will work with either scheme
     basically just inits memused if not using the custom scheme
     the custom scheme would normally create a heap of 2 megs but
     as it is now, the memused variable is just set to 0 */
  InitMemory(1024 * 1024 * 4);

  Q.Active = FALSE;
  Time(InitQuest(argc, argv));
  Q.Active = TRUE;

  NewMessage("Quest " QUEST_VER " initialized.");

#ifdef DEBUG_GUI
  {
    char buf[256];
    GUI_SelectFile("Test", "*.txt", "OK", buf);
    Abort("main", "!!!");
  }
#endif

  /* --------- *
   * MAIN LOOP *
   * --------- */

  /*
  status.exit_flag is accessible anywhere. set it to true and
  quest should dump you out.
  */

  while (!status.exit_flag)
  {
    CheckLeak();

    /*  Update mouse (which part of the screen is it in?)
       FindMouse(TRUE) used to box in the area were the mouse
       was, dunno if it still will. */
    CurWindow = FindMouse(FALSE);

    switch (CurWindow)
    {
      case MAP_WINDOW:
        UpdateMap();
        break;

      case MENU_WINDOW:
        UpdateMenu(&Menus, &List, &ListSize);
        break;

      case MESG_WINDOW:
        UpdateMsg();
        break;

      case TOOL_WINDOW:
        UpdateTool();
        break;

      case STATUS_WINDOW:
        break;
    }

    if (status.pop_menu == 2)
    {
      if (((CurWindow != MAP_WINDOW) && (mouse.button == 2)) ||
          (mouse.button == 4))
      {
        while (mouse.button)
          UpdateMouse();
        MenuPopUp();
      }
    }

    UpdateMouse();

    /* And Keyboard */
    CheckKeyHit();
  }

  SaveConfig();

  KBD_RemoveHandler();

  /* -------- */
  /* CLEAN-UP */
  /* -------- */

  SetMode(RES_TEXT);

  /* deallocate everything */
  DisposeDisplay();
  DisposeVideo();
  CloseMemory();

  return 0;
}

typedef struct
{
  char id;
  char version;
  char encoding;
  char bpp;
  short xmin;
  short ymin;
  short xmax;
  short ymax;
  short hres;
  short vres;
  char pal[48];
  char reserved;
  char nplanes;
  short bpl;
  short palinfo;
  char filler[58];
} PCXHeader /*__attribute__ ((packed))*/;

void
TakeScreenShot(void)
{
  static int LastFileName = 0;
  FILE* fp;
  char filename[80];
  char pal[768];
  int i, j;
  PCXHeader PCXHead;

#ifdef _UNIX
  struct stat buf;
#endif

  for (i = LastFileName; i < 1000; i++)
  {
    sprintf(filename, "quest%03i.pcx", i);
#ifdef DJGPP
    if (!__file_exists(filename))
#endif
#ifdef _UNIX
      if (stat(filename, &buf))
#endif
        break;
  }
  LastFileName = i;

  fp = fopen(filename, "wb");

  if (!fp)
  {
    HandleError("TakeScreenShot", "Unable to create %s", filename);
    return;
  }

  memset(&PCXHead, 0, sizeof(PCXHead));

  PCXHead.id = 10;
  PCXHead.version = 5;
  PCXHead.encoding = 1;
  PCXHead.bpp = 8;

  PCXHead.xmin = PCXHead.ymin = 0;
  PCXHead.xmax = video.ScreenWidth - 1;
  PCXHead.ymax = video.ScreenHeight - 1;

  PCXHead.hres = 0x9600;
  PCXHead.vres = 0x9600;

  PCXHead.nplanes = 1;
  PCXHead.bpl = video.ScreenWidth;
  PCXHead.palinfo = 2;

  if (fwrite(&PCXHead, 1, sizeof(PCXHead), fp) != sizeof(PCXHead))
  {
    fclose(fp);
    HandleError("TakeScreenShot", "Unable to write to %s", filename);
    return;
  }

  for (i = 0; i < video.ScreenHeight; i++)
  {
    unsigned char* buf;
    int last, num;

    buf = &video.ScreenBuffer[i * video.ScreenWidth];
    last = *buf++;
    num = 1;
    for (j = 1; j < video.ScreenWidth; j++, buf++)
    {
      if (last == *buf)
      {
        num++;
        if (num == 63)
        {
          fputc(0xC0 | num, fp);
          fputc(last, fp);
          num = 0;
        }
      }
      else
      {
        if (num)
        {
          if ((num == 1) && ((last & 0xC0) != 0xC0))
          {
            fputc(last, fp);
          }
          else
          {
            fputc(0xC0 | num, fp);
            fputc(last, fp);
          }
          num = 0;
        }
        last = *buf;
        num = 1;
      }
    }
    if (num)
    {
      if ((num == 1) && ((last & 0xc0) != 0xC0))
      {
        fputc(last, fp);
      }
      else
      {
        fputc(0xC0 | num, fp);
        fputc(last, fp);
      }
      num = 0;
    }
  }

  fputc(12, fp);

  /* GETS THE CURRENT PAL FROM VIDEO CARD
     (using quest.pal wouldn't hold correct gamma corrected values) */
  GetPal(pal);

  fwrite(pal, 1, 768, fp);

  fclose(fp);
}
