/*
config.c file of the Quest Source Code

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

#include "config.h"

#include "color.h"
#include "display.h"
#include "error.h"
#include "file.h"
#include "memory.h"
#include "menu.h"
#include "mouse.h"
#include "quest.h"
#include "qui.h"
#include "status.h"
#include "token.h"
#include "video.h"

#define DEFAULT_VIDEO_MODE 1
#define DEFAULT_SNAP_SIZE 4
#define DEFAULT_PAN_SPEED 8

typedef struct
{
  const char* name;
  int type;
  int flags;
  void* data;
  const char* nname;
} cfgvar_t;

#define INT 0 // int
#define FLT 1 // float
#define STR 2 // char *

#define SAVE 1      // variable will be save to quest.cfg upon exit
#define EDIT 2      // variable can be edited in the preferences dialog
#define SAVED 0x100 // internal, has the variable been saved yet

cfgvar_t cfgvars[] =
  {
    {"vid_mode", INT, 0, &options.vid_mode, NULL},
    {"use_vbe2", INT, 0, &status.use_vbe2, NULL},
    {"gamma", FLT, SAVE | EDIT, &gammaval, "~Gamma"},

    {"angle_snap_size", INT, SAVE | EDIT, &status.angle_snap_size, "~Angle snap size"},
    {"snap_size", INT, SAVE | EDIT, &status.snap_size, "~Snap size"},

    {"pan_speed", INT, SAVE | EDIT, &status.pan_speed, "~Pan speed"},
    {"zoom_speed", FLT, SAVE | EDIT, &status.zoom_speed, "~Zoom speed"},

    {"turn_frames", INT, SAVE | EDIT, &status.turn_frames, "~Turn frames"},
    {"depth_clip", INT, SAVE | EDIT, &status.depth_clip, "~Depth clip"},

    {"texture_path", STR, 0, status.tex_str, NULL},
    {"texturename", STR, 0, texturename, NULL},

    {"mouse_sens_x", FLT, SAVE | EDIT, &mouse_sens_x, "Mouse sens ~x"},
    {"mouse_sens_y", FLT, SAVE | EDIT, &mouse_sens_y, "Mouse sens ~y"},
    {"flip_mouse", INT, SAVE | EDIT, &status.flip_mouse, "~Flip mouse"},

    {"pop_menu", INT, SAVE | EDIT, &status.pop_menu, "P~opup menu"},
    {"menu_limit", INT, SAVE | EDIT, &status.menu_limit, "~Menu popup limit"},

    {"game", STR, 0, status.game_str, NULL},
    {"class_dir", STR, 0, status.class_dir, NULL},

    {"snap_to_int", INT, 0, &status.snap_to_int, NULL},

    {"mdlpaks", STR, 0, status.mdlpaks, NULL},
    {"load_models", INT, 0, &status.load_models, NULL},
    {"draw_models", INT, SAVE | EDIT, &status.draw_models, "D~raw models"},

    {"map2", INT, SAVE | EDIT, &status.map2, "~New map format"},

    {"profile", STR, 0, profile, NULL},

    {0, 0, 0, NULL, NULL}};
#define NUM_VARS (sizeof(cfgvars) / sizeof(cfgvars[0]) - 1)

void
LoadConfig(const char* filename, int first)
{
  char key[128], value[512];
  int temp;
  float tempf;
  char name[256];
  int x;
  cfgvar_t* c;

  if (first)
  {
    status.tex_str[0] = 0;
    texturename[0] = 0;
    gammaval = 0.5;
    options.vid_mode = -1;
    status.snap_size = 16;
    status.angle_snap_size = 15;
    status.pan_speed = 20;
    status.zoom_speed = 1.125;
    status.flip_mouse = 0;
    status.use_vbe2 = 0;
    status.turn_frames = 5;
  }

  FindFile(name, filename);

  if (!TokenFile(name, T_CFG | T_ALLNAME | T_STRING, NULL))
  {
    if (first)
    {
      printf("-- Unable to open configuration file.  Using defaults.\n");
      printf("-- Press any key to continue...\n");
      options.vid_mode = DEFAULT_VIDEO_MODE;
      status.snap_size = DEFAULT_SNAP_SIZE;
      status.pan_speed = DEFAULT_PAN_SPEED;
      getchar();
    }
    return;
  }

  while (TokenGet(1, -1))
  {
    strcpy(key, token);
    if (!TokenGet(0, -1))
    {
      Abort("LoadConfig", "Error in cfg file, line %i!", token_linenum);
    }
    strcpy(value, token);

    if (value[0] == '"')
    {
      value[strlen(value) - 1] = 0;
      strcpy(value, &value[1]);
    }

    for (x = 0, c = cfgvars; x < NUM_VARS; x++, c++)
    {
      if (!stricmp(key, c->name))
        break;
    }

    if (x == NUM_VARS)
    {
      Abort("LoadConfig", "Error in cfg file, line %i!", token_linenum);
    }

    switch (c->type)
    {
      case INT:
        sscanf(value, "%i", &temp);
        *((int*)c->data) = temp;
        break;

      case FLT:
        sscanf(value, "%f", &tempf);
        *((float*)c->data) = tempf;
        break;

      case STR:
        strcpy(c->data, value);
        break;
    }
  }

  TokenDone();

  if ((status.angle_snap_size > 360) || (status.angle_snap_size < 1))
  {
    Abort("LoadConfig", "Illegal angle_snap_size specified in cfg file!");
  }

  if (status.snap_size < 1)
  {
    Abort("LoadConfig", "Illegal snap_size specified in cfg file!");
  }

  if ((status.pan_speed > 32) || (status.pan_speed < 2))
  {
    Abort("LoadConfig", "Illegal pan_speed specified in cfg file!");
  }

  if (options.vid_mode == -1)
    options.vid_mode = DEFAULT_VIDEO_MODE;

  status.vid_mode = options.vid_mode;

  if (status.snap_size == -1)
    status.snap_size = DEFAULT_SNAP_SIZE;

  if (status.pan_speed == -1)
    status.pan_speed = DEFAULT_PAN_SPEED;
}

void
SaveConfig(void)
{
  FILE *fpin, *fpout;
  char instring[256];
  char key[256];
  int i;
  char n_cfg[256], n_tmp[256];
  cfgvar_t* c;

  if (profile[0])
  {
    strcpy(n_tmp, profile);
    strcat(n_tmp, ".cfg");
  }
  else
  {
    strcpy(n_tmp, "quest.cfg");
  }

  FindFile(n_cfg, n_tmp);

  strcpy(n_tmp, n_cfg);
  strcpy(strrchr(n_tmp, '.'), ".tmp");

  rename(n_cfg, n_tmp);

  if ((fpout = fopen(n_cfg, "wt")) == NULL)
    return;

  if ((fpin = fopen(n_tmp, "rt")))
  {
    fgets(instring, 256, fpin);
    while (!feof(fpin))
    {
      sscanf(instring, "%s", key);

      key[0] = '\0';
      for (i = 0; (instring[i] != ' ') && (i < strlen(instring)); i++)
        key[i] = instring[i];
      key[i] = '\0';

      for (i = 0, c = cfgvars; i < NUM_VARS; i++, c++)
        if (!stricmp(key, c->name))
          break;

      if ((i == NUM_VARS) || !(c->flags & SAVE))
      {
        fprintf(fpout, "%s", instring);
        goto cont;
      }

#define WriteVar(c)                                           \
  switch (c->type)                                            \
  {                                                           \
    case INT:                                                 \
      fprintf(fpout, "%s %i\n", c->name, *((int*)c->data));   \
      break;                                                  \
                                                              \
    case FLT:                                                 \
      fprintf(fpout, "%s %f\n", c->name, *((float*)c->data)); \
      break;                                                  \
                                                              \
    case STR:                                                 \
      fprintf(fpout, "%s %s\n", c->name, (char*)c->data);     \
      break;                                                  \
  }

      WriteVar(c);

      c->flags |= SAVED;

cont:
      fgets(instring, 256, fpin);
    }

    fclose(fpin);
  }

  for (i = 0, c = cfgvars; i < NUM_VARS; i++, c++)
  {
    if ((c->flags & SAVE) && !(c->flags & SAVED))
    {
      WriteVar(c);
    }
  }

  fclose(fpout);

  remove(n_tmp);
}

void
SetPrefs(void)
{
  int i, j;
  char** keys;
  char** values;
  int n_keys;
  cfgvar_t* c;

  n_keys = 0;
  for (i = 0, c = cfgvars; i < NUM_VARS; i++, c++)
  {
    if (c->flags & EDIT)
      n_keys++;
  }

  keys = (char**)Q_malloc(sizeof(char*) * n_keys);
  values = (char**)Q_malloc(sizeof(char*) * n_keys);

  if (!keys || !values)
  {
    HandleError("SetPrefs", "Out of memory!");
    return;
  }

  j = 0;
  c = cfgvars;
  for (i = 0; i < n_keys; i++)
  {
    keys[i] = (char*)Q_malloc(sizeof(char) * 64);
    values[i] = (char*)Q_malloc(sizeof(char) * 64);

    if (!keys[i] || !values[i])
    {
      HandleError("SetPrefs", "Out of memory!");
      return;
    }

    for (; j < NUM_VARS; j++, c++)
    {
      if (c->flags & EDIT)
      {
        strcpy(keys[i], c->nname);

        switch (c->type)
        {
          case INT:
            sprintf(values[i], "%i", *(int*)c->data);
            break;
          case FLT:
            sprintf(values[i], "%2.2f", *(float*)c->data);
            break;
          case STR:
            sprintf(values[i], "%s", (char*)c->data);
            break;
        }
        c++;
        break;
      }
    }
  }

  if (QUI_PopEntity("Preferences", keys, values, n_keys))
  {
    j = 0;
    c = cfgvars;
    for (i = 0; i < n_keys; i++)
    {
      for (; j < NUM_VARS; j++, c++)
      {
        if (c->flags & EDIT)
        {
          switch (c->type)
          {
            case INT:
              sscanf(values[i], "%i", (int*)c->data);
              break;
            case FLT:
              sscanf(values[i], "%f", (float*)c->data);
              break;
            case STR:
              sscanf(values[i], "%s", (char*)c->data);
              break;
          }
          c++;
          break;
        }
      }
    }
  }

  for (i = 0; i < n_keys; i++)
  {
    Q_free(keys[i]);
    Q_free(values[i]);
  }
  Q_free(keys);
  Q_free(values);

  SetMouseSensitivity(mouse_sens_x, mouse_sens_y);
  SetPal(PAL_QUEST);
  if ((!status.pop_menu) && (!MenuShowing))
    PopUpMenuWin();
}
