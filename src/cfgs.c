/*
cfgs.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

/*
 General definition system and configuration handler.

 If you want to add a key, add the define to the list in keyboard.h
and an AddDef in the keys array.
 If you want to add a command, add the define and AddDef to the cmds array
and insert the code to handle the command in the gigantic switch statement
in ExecCmd.
 Adding new flags and modes is complicated.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "version.h"

#include "defines.h"
#include "types.h"

#include "cfgs.h"

#include "brush.h"
#include "error.h"
#include "game.h"
#include "keyboard.h"
#include "qui.h"
#include "status.h"
#include "times.h"

#ifdef _UNIX
extern void X_SetWindowTitle(const char*);
#endif

// For all the commands.
#include "3d.h"
#include "bsp.h"
#include "bspleak.h"
#include "camera.h"
#include "check.h"
#include "clickbr.h"
#include "clip.h"
#include "clip2.h"
#include "color.h"
#include "config.h"
#include "display.h"
#include "dvport.h"
#include "edbrush.h"
#include "edbrush2.h"
#include "edcurve.h"
#include "edent.h"
#include "edentity.h"
#include "edface.h"
#include "editface.h"
#include "edmodel.h"
#include "edprim.h"
#include "edvert.h"
#include "entclass.h"
#include "entity.h"
#include "keyhit.h"
#include "map.h"
#include "memory.h"
#include "menu.h"
#include "message.h"
#include "mouse.h"
#include "newgroup.h"
#include "popupwin.h"
#include "portals.h"
#include "quest.h"
#include "split.h"
#include "tex.h"
#include "texdef.h"
#include "texpick.h"
#include "texreplc.h"
#include "undo.h"
#include "video.h"
#include "weld.h"

#include "games/qengine/q2_tex.h"
#include "games/qengine/quake3.h"

// Just for the profiling.
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

#define AddDef(x, y) {x, #x, y},

def_t key_defs[] =
  {
    AddDef(KEY_ESCAPE, "Escape")
      AddDef(KEY_ENTER, "Enter")
        AddDef(KEY_SPACE, "Space")

          AddDef(KEY_UP, "Up")
            AddDef(KEY_DOWN, "Down")
              AddDef(KEY_LEFT, "Left")
                AddDef(KEY_RIGHT, "Right")

                  AddDef(KEY_INSERT, "Insert")
                    AddDef(KEY_HOME, "Home")
                      AddDef(KEY_PAGEUP, "Page up")
                        AddDef(KEY_DELETE, "Delete")
                          AddDef(KEY_END, "End")
                            AddDef(KEY_PAGEDOWN, "Page down")
                              AddDef(KEY_CAPS_LCK, "Caps lock")
                                AddDef(KEY_TAB, "Tab")
                                  AddDef(KEY_BACKSPC, "Baskspace")

                                    AddDef(KEY_LF_BRACK, "[")
                                      AddDef(KEY_RT_BRACK, "]")
                                        AddDef(KEY_COLON, ":")
                                          AddDef(KEY_QUOTE, "\'")
                                            AddDef(KEY_TILDE, "~")
                                              AddDef(KEY_PIPE, "|")
                                                AddDef(KEY_COMMA, ",")
                                                  AddDef(KEY_PERIOD, ".")
                                                    AddDef(KEY_Q_MARK, "?")

                                                      AddDef(KEY_PRINT_SC, "Print screen")
                                                        AddDef(KEY_SCR_LOCK, "Scroll lock")
                                                          AddDef(KEY_NUM_LOCK, "Num lock")

                                                            AddDef(KEY_MINUS, "+")
                                                              AddDef(KEY_PLUS, "-")
                                                                AddDef(KEY_E_MINUS, "Extended +")
                                                                  AddDef(KEY_E_PLUS, "Extended -")
                                                                    AddDef(KEY_CENTER, "Center")

                                                                      AddDef(KEY_1, "1")
                                                                        AddDef(KEY_2, "2")
                                                                          AddDef(KEY_3, "3")
                                                                            AddDef(KEY_4, "4")
                                                                              AddDef(KEY_5, "5")
                                                                                AddDef(KEY_6, "6")
                                                                                  AddDef(KEY_7, "7")
                                                                                    AddDef(KEY_8, "8")
                                                                                      AddDef(KEY_9, "9")
                                                                                        AddDef(KEY_0, "0")

                                                                                          AddDef(KEY_A, "A")
                                                                                            AddDef(KEY_B, "B")
                                                                                              AddDef(KEY_C, "C")
                                                                                                AddDef(KEY_D, "D")
                                                                                                  AddDef(KEY_E, "E")
                                                                                                    AddDef(KEY_F, "F")
                                                                                                      AddDef(KEY_G, "G")
                                                                                                        AddDef(KEY_H, "H")
                                                                                                          AddDef(KEY_I, "I")
                                                                                                            AddDef(KEY_J, "J")
                                                                                                              AddDef(KEY_K, "K")
                                                                                                                AddDef(KEY_L, "L")
                                                                                                                  AddDef(KEY_M, "M")
                                                                                                                    AddDef(KEY_N, "N")
                                                                                                                      AddDef(KEY_O, "O")
                                                                                                                        AddDef(KEY_P, "P")
                                                                                                                          AddDef(KEY_Q, "Q")
                                                                                                                            AddDef(KEY_R, "R")
                                                                                                                              AddDef(KEY_S, "S")
                                                                                                                                AddDef(KEY_T, "T")
                                                                                                                                  AddDef(KEY_U, "U")
                                                                                                                                    AddDef(KEY_V, "V")
                                                                                                                                      AddDef(KEY_W, "W")
                                                                                                                                        AddDef(KEY_X, "X")
                                                                                                                                          AddDef(KEY_Y, "Y")
                                                                                                                                            AddDef(KEY_Z, "Z")

                                                                                                                                              AddDef(KEY_F1, "F1")
                                                                                                                                                AddDef(KEY_F2, "F2")
                                                                                                                                                  AddDef(KEY_F3, "F3")
                                                                                                                                                    AddDef(KEY_F4, "F4")
                                                                                                                                                      AddDef(KEY_F5, "F5")
                                                                                                                                                        AddDef(KEY_F6, "F6")
                                                                                                                                                          AddDef(KEY_F7, "F7")
                                                                                                                                                            AddDef(KEY_F8, "F8")
                                                                                                                                                              AddDef(KEY_F9, "F9")
                                                                                                                                                                AddDef(KEY_F10, "F10")

                                                                                                                                                                  {0, 0, 0}};
#define NUM_KEYS (sizeof(key_defs) / sizeof(key_defs[0]))

def_t k_flags[] =
  {
#define KF_CTRL 0x0100
#define KF_ALT 0x0200
#define KF_SHIFT 0x0400

    {KF_CTRL, "KEY_CTRL", "Ctrl"},
    {KF_ALT, "KEY_ALT", "Alt"},
    {KF_SHIFT, "KEY_SHIFT", "Shift"},

#define KF_REPEAT 0x0800
    {KF_REPEAT, "KEY_REPEAT", "Repeat if held down"},

    {0, 0, 0}};
#define NUM_K_FLAGS (sizeof(k_flags) / sizeof(k_flags[0]))
#define FLAG_SHIFT (8)

def_t modes[NUM_MODES] =
  {
    {MODE_MOVE, "MOVE", "Movement keys"},
    {MODE_COMMON, "COMMON", "Common keys"},

    {MODE_BRUSH, "BRUSH", "Brush mode keys"},
    {MODE_FACE, "FACE", "Face mode keys"},
    {MODE_ENTITY, "ENTITY", "Entity mode keys"},
    {MODE_MODEL, "MODEL", "Model mode keys"}};

int
FindDef(def_t* defs, char* name)
{
  int i;

  for (i = 0; defs[i].name; i++)
  {
    if (!strcmp(defs[i].name, name))
      return defs[i].num;
  }
  Abort("FindDef", "Unknown identifier %s!", name);
}

const char*
GetDefName(def_t* defs, int v)
{
  int i;

  for (i = 0; defs[i].name; i++)
  {
    if (defs[i].num == v)
      return defs[i].desc;
  }
  return "Unknown definition!";
}

typedef struct
{
  int n_defs;
  int* keys;
  int* cmds;
} cfg_t;

cfg_t cfgs[NUM_MODES];

void
AddCfg(int mode, int key, int cmd)
{
  cfg_t* c;

  c = &cfgs[mode];

  c->keys = Q_realloc(c->keys, (c->n_defs + 1) * sizeof(int));
  c->cmds = Q_realloc(c->cmds, (c->n_defs + 1) * sizeof(int));

  c->keys[c->n_defs] = key;
  c->cmds[c->n_defs] = cmd;

  c->n_defs++;
}

static int flag_mask;

// need to do this some better way someday

static int rep_cmds[NUM_MODES];
static int rep_cmd;
static int lasttick = -1;

static int
CheckKey(int key, int wait)
{
  if (((key >> FLAG_SHIFT) & ~(KF_REPEAT >> FLAG_SHIFT)) != flag_mask)
    return 0;

  if (!TestKey(key & 0xff))
    return 0;

  if (key & KF_REPEAT)
  {
    rep_cmd = 1;
    if (lasttick == -1)
    {
      lasttick = GetTick();
      return 1;
    }
    else
    {
      if (GetTick() < lasttick + 5)
        return 0;
      else
        return 1;
    }
  }
  else
  {
    if (wait)
      while (TestKey(key & 0xff))
        UpdateMouse();
  }

  return 1;
}

void
CheckCfg(int mode)
{
  cfg_t* c;
  int i;
  int redraw, r;

  flag_mask = 0;

  if (TestKey(KEY_CONTROL))
    flag_mask |= KF_CTRL >> FLAG_SHIFT;

  if (TestKey(KEY_ALT))
    flag_mask |= KF_ALT >> FLAG_SHIFT;

  if (TestKey(KEY_RT_SHIFT))
    flag_mask |= KF_SHIFT >> FLAG_SHIFT;

  if (TestKey(KEY_LF_SHIFT))
    flag_mask |= KF_SHIFT >> FLAG_SHIFT;

  rep_cmd = 0;

  redraw = 0;
  c = &cfgs[mode];
  for (i = 0; i < c->n_defs; i++)
  {
    if (CheckKey(c->keys[i], mode != MODE_MOVE))
    {
      r = ExecCmd(c->cmds[i]);

      if (r > redraw)
        redraw = r;

      // the ExecCmd might have taken a long time, so recheck the flags
      flag_mask = 0;

      if (TestKey(KEY_CONTROL))
        flag_mask |= KF_CTRL >> FLAG_SHIFT;

      if (TestKey(KEY_ALT))
        flag_mask |= KF_ALT >> FLAG_SHIFT;

      if (TestKey(KEY_RT_SHIFT))
        flag_mask |= KF_SHIFT >> FLAG_SHIFT;

      if (TestKey(KEY_LF_SHIFT))
        flag_mask |= KF_SHIFT >> FLAG_SHIFT;
    }
  }

  if (rep_cmds[mode] && !rep_cmd)
    lasttick = -1;
  rep_cmds[mode] = rep_cmd;

  if (redraw == 1)
    UpdateViewport(M.display.active_vport, TRUE);
  if (redraw == 2)
    UpdateAllViewports();

  if (redraw)
  {
    DrawMouse(mouse.x, mouse.y);
  }
}

def_t cmds[] =
  {
#define CMD_EXIT 1
    AddDef(CMD_EXIT, "Exit")

#define CMD_MODE_NEXT 100
      AddDef(CMD_MODE_NEXT, "Next mode")
#define CMD_MODE_LAST 101
        AddDef(CMD_MODE_LAST, "Prev mode")

#define CMD_MODE_BRUSH 102
          AddDef(CMD_MODE_BRUSH, "Switch to brush mode")
#define CMD_MODE_FACE 103
            AddDef(CMD_MODE_FACE, "Switch to face mode")
#define CMD_MODE_ENTITY 104
              AddDef(CMD_MODE_ENTITY, "Switch to entity mode")
#define CMD_MODE_MODEL 105
                AddDef(CMD_MODE_MODEL, "Switch to model mode")

#define CMD_MESSAGE_DUMP 200
                  AddDef(CMD_MESSAGE_DUMP, "Dump messages")

#define CMD_CAMERA_REINIT 300
                    AddDef(CMD_CAMERA_REINIT, "Reinit camera")

#define CMD_EDIT_DESELECT 350
                      AddDef(CMD_EDIT_DESELECT, "Deselect everything")

// Brush commands
#define CMD_BRUSH_SELECT_ALL 400
                        AddDef(CMD_BRUSH_SELECT_ALL, "Select all vertices on selected brushes")
#define CMD_BRUSH_COPY 401
                          AddDef(CMD_BRUSH_COPY, "Copy brush")
#define CMD_BRUSH_PASTE 402
                            AddDef(CMD_BRUSH_PASTE, "Paste brush")
#define CMD_BRUSH_ROTATE 403
                              AddDef(CMD_BRUSH_ROTATE, "Rotate brush")
#define CMD_BRUSH_MIRROR 404
                                AddDef(CMD_BRUSH_MIRROR, "Mirror brush")
#define CMD_BRUSH_SCALE 405
                                  AddDef(CMD_BRUSH_SCALE, "Scale brush")
#define CMD_BRUSH_SCALE_VER 406
                                    AddDef(CMD_BRUSH_SCALE_VER, "Scale selected vertices")
#define CMD_BRUSH_DELETE 407
                                      AddDef(CMD_BRUSH_DELETE, "Delete brush")
#define CMD_BRUSH_GRIDSNAP 408
                                        AddDef(CMD_BRUSH_GRIDSNAP, "Snap brush to grid")
#define CMD_BRUSH_VERTSNAP 409
                                          AddDef(CMD_BRUSH_VERTSNAP, "Snap vertices to grid")
#define CMD_BRUSH_VERTSNAPTO 410
                                            AddDef(CMD_BRUSH_VERTSNAPTO, "Snap vertices to vertex")
#define CMD_BRUSH_SWITCH_SUBTRACT 411
                                              AddDef(CMD_BRUSH_SWITCH_SUBTRACT, "Switch subtractive")

#define CMD_BRUSH_CREATE_CUBE 430
                                                AddDef(CMD_BRUSH_CREATE_CUBE, "Create cube")
#define CMD_BRUSH_CREATE_PRISM 431
                                                  AddDef(CMD_BRUSH_CREATE_PRISM, "Create prism")
#define CMD_BRUSH_CREATE_ROOM 432
                                                    AddDef(CMD_BRUSH_CREATE_ROOM, "Create room")
#define CMD_BRUSH_CREATE_PYRAMID 433
                                                      AddDef(CMD_BRUSH_CREATE_PYRAMID, "Create pyramid")
#define CMD_BRUSH_CREATE_DODEC 434
                                                        AddDef(CMD_BRUSH_CREATE_DODEC, "Create dodecahedron")
#define CMD_BRUSH_CREATE_ICOS 435
                                                          AddDef(CMD_BRUSH_CREATE_ICOS, "Create icosahedron")
#define CMD_BRUSH_CREATE_BUCKY 436
                                                            AddDef(CMD_BRUSH_CREATE_BUCKY, "Create buckyball")
#define CMD_BRUSH_CREATE_TORUS 437
                                                              AddDef(CMD_BRUSH_CREATE_TORUS, "Create torus")

#define CMD_VERT_LEFT 450
                                                                AddDef(CMD_VERT_LEFT, "Move selected vertices left")
#define CMD_VERT_RIGHT 451
                                                                  AddDef(CMD_VERT_RIGHT, "Move selected vertices right")
#define CMD_VERT_UP 452
                                                                    AddDef(CMD_VERT_UP, "Move selected vertices up")
#define CMD_VERT_DOWN 453
                                                                      AddDef(CMD_VERT_DOWN, "Move selected vertices down")
#define CMD_VERT_FORWARD 454
                                                                        AddDef(CMD_VERT_FORWARD, "Move selected vertices forward")
#define CMD_VERT_BACKWARD 455
                                                                          AddDef(CMD_VERT_BACKWARD, "Move selected vertices backward")

#define CMD_TVERT_LEFT 460
                                                                            AddDef(CMD_TVERT_LEFT, "Move texture left")
#define CMD_TVERT_RIGHT 461
                                                                              AddDef(CMD_TVERT_RIGHT, "Move texture right")
#define CMD_TVERT_UP 462
                                                                                AddDef(CMD_TVERT_UP, "Move texture up")
#define CMD_TVERT_DOWN 463
                                                                                  AddDef(CMD_TVERT_DOWN, "Move texture down")

#define CMD_TVERT_S_LEFT 464
                                                                                    AddDef(CMD_TVERT_S_LEFT, "Scale texture left")
#define CMD_TVERT_S_RIGHT 465
                                                                                      AddDef(CMD_TVERT_S_RIGHT, "Scale texture right")
#define CMD_TVERT_S_UP 466
                                                                                        AddDef(CMD_TVERT_S_UP, "Scale texture up")
#define CMD_TVERT_S_DOWN 467
                                                                                          AddDef(CMD_TVERT_S_DOWN, "Scale texture down")

// Face commands
#define CMD_FACE_SELECT_ALL 500
                                                                                            AddDef(CMD_FACE_SELECT_ALL, "Select all vertices on selected faces")
#define CMD_FACE_SPLIT 501
                                                                                              AddDef(CMD_FACE_SPLIT, "Split a face")
#define CMD_FACE_SPLIT_EDGE 502
                                                                                                AddDef(CMD_FACE_SPLIT_EDGE, "Split an edge")
#define CMD_FACE_SPLIT_BOTH 503
                                                                                                  AddDef(CMD_FACE_SPLIT_BOTH, "Split two edges and a face")
#define CMD_FACE_EDIT 504
                                                                                                    AddDef(CMD_FACE_EDIT, "Edit texture properties")
#define CMD_FACE_FLAGS 505
                                                                                                      AddDef(CMD_FACE_FLAGS, "Edit surface flags")

#define CMD_FACE_NEXT 506
                                                                                                        AddDef(CMD_FACE_NEXT, "Next face on brush")
#define CMD_FACE_PREV 507
                                                                                                          AddDef(CMD_FACE_PREV, "Prev face on brush")

// Entity commands
#define CMD_ENTITY_COPY 600
                                                                                                            AddDef(CMD_ENTITY_COPY, "Copy entity")
#define CMD_ENTITY_PASTE 601
                                                                                                              AddDef(CMD_ENTITY_PASTE, "Paste entity")
#define CMD_ENTITY_ROTATE 602
                                                                                                                AddDef(CMD_ENTITY_ROTATE, "Rotate entity")
#define CMD_ENTITY_EDIT 603
                                                                                                                  AddDef(CMD_ENTITY_EDIT, "Edit entity")
#define CMD_ENTITY_DELETE 604
                                                                                                                    AddDef(CMD_ENTITY_DELETE, "Delete entity")
#define CMD_ENTITY_CREATE 605
                                                                                                                      AddDef(CMD_ENTITY_CREATE, "Create entity")

#define CMD_ENTITY_LEFT 650
                                                                                                                        AddDef(CMD_ENTITY_LEFT, "Move selected entities left")
#define CMD_ENTITY_RIGHT 651
                                                                                                                          AddDef(CMD_ENTITY_RIGHT, "Move selected entities right")
#define CMD_ENTITY_UP 652
                                                                                                                            AddDef(CMD_ENTITY_UP, "Move selected entities up")
#define CMD_ENTITY_DOWN 653
                                                                                                                              AddDef(CMD_ENTITY_DOWN, "Move selected entities down")
#define CMD_ENTITY_FORWARD 654
                                                                                                                                AddDef(CMD_ENTITY_FORWARD, "Move selected entities forward")
#define CMD_ENTITY_BACKWARD 655
                                                                                                                                  AddDef(CMD_ENTITY_BACKWARD, "Move selected entities backward")

// Model commands
#define CMD_MODEL_LIST 700
                                                                                                                                    AddDef(CMD_MODEL_LIST, "List all models")
#define CMD_MODEL_CREATE 701
                                                                                                                                      AddDef(CMD_MODEL_CREATE, "Create model")
#define CMD_MODEL_ADD 702
                                                                                                                                        AddDef(CMD_MODEL_ADD, "Add brush to model")
#define CMD_LINK_CREATE 703
                                                                                                                                          AddDef(CMD_LINK_CREATE, "Create a link")

// Curve commands
#define CMD_CURVE_NEW 800
                                                                                                                                            AddDef(CMD_CURVE_NEW, "Create a bezier patch array")

// File commands
#define CMD_NEW 1000
                                                                                                                                              AddDef(CMD_NEW, "New map")
#define CMD_LOAD 1001
                                                                                                                                                AddDef(CMD_LOAD, "Load map")
#define CMD_SAVE 1002
                                                                                                                                                  AddDef(CMD_SAVE, "Save map")
#define CMD_SAVE_AS 1003
                                                                                                                                                    AddDef(CMD_SAVE_AS, "Save map as")

#define CMD_GRP_LOAD 1004
                                                                                                                                                      AddDef(CMD_GRP_LOAD, "Load group")
#define CMD_GRP_SAVE 1005
                                                                                                                                                        AddDef(CMD_GRP_SAVE, "Save group")

#define CMD_PTS_LOAD 1006
                                                                                                                                                          AddDef(CMD_PTS_LOAD, "Load pts/lin file")

#define CMD_VIDEO_REINIT 1100
                                                                                                                                                            AddDef(CMD_VIDEO_REINIT, "Reinit video system")

#define CMD_MAPINFO 1200
                                                                                                                                                              AddDef(CMD_MAPINFO, "Map information")
#define CMD_SETPREFS 1201
                                                                                                                                                                AddDef(CMD_SETPREFS, "Set preferences")
#define CMD_TEXTURE_PICK 1202
                                                                                                                                                                  AddDef(CMD_TEXTURE_PICK, "Pick texture")
#define CMD_GROUP_POPUP 1203
                                                                                                                                                                    AddDef(CMD_GROUP_POPUP, "Group popup")
#define CMD_BUILD_POPUP 1204
                                                                                                                                                                      AddDef(CMD_BUILD_POPUP, "Build popup")
#define CMD_CHECK_MAP 1205
                                                                                                                                                                        AddDef(CMD_CHECK_MAP, "Check map for errors")
#define CMD_SCREEN_SHOT 1206
                                                                                                                                                                          AddDef(CMD_SCREEN_SHOT, "Take a screenshot")
#define CMD_SELECT_WAD 1207
                                                                                                                                                                            AddDef(CMD_SELECT_WAD, "Select a WAD or textures")
#define CMD_REREAD_CACHE 1208
                                                                                                                                                                              AddDef(CMD_REREAD_CACHE, "Flush and reread the texture cache")
#define CMD_FIX_DUPLICATES 1209
                                                                                                                                                                                AddDef(CMD_FIX_DUPLICATES, "Fix texture duplicates")

// Camera commands
#define CMD_TURN_RIGHT 2000
                                                                                                                                                                                  AddDef(CMD_TURN_RIGHT, "Look right")
#define CMD_TURN_LEFT 2001
                                                                                                                                                                                    AddDef(CMD_TURN_LEFT, "Look left")
#define CMD_LOOK_UP 2002
                                                                                                                                                                                      AddDef(CMD_LOOK_UP, "Look up")
#define CMD_LOOK_DOWN 2003
                                                                                                                                                                                        AddDef(CMD_LOOK_DOWN, "Look down")
#define CMD_ROLL_RIGHT 2004
                                                                                                                                                                                          AddDef(CMD_ROLL_RIGHT, "Roll right")
#define CMD_ROLL_LEFT 2005
                                                                                                                                                                                            AddDef(CMD_ROLL_LEFT, "Roll left")

#define CMD_MOVE_FORWARD 2100
                                                                                                                                                                                              AddDef(CMD_MOVE_FORWARD, "Move forward")
#define CMD_MOVE_BACKWARD 2101
                                                                                                                                                                                                AddDef(CMD_MOVE_BACKWARD, "Move backward")
#define CMD_MOVE_LEFT 2102
                                                                                                                                                                                                  AddDef(CMD_MOVE_LEFT, "Move left")
#define CMD_MOVE_RIGHT 2103
                                                                                                                                                                                                    AddDef(CMD_MOVE_RIGHT, "Move right")
#define CMD_MOVE_UP 2104
                                                                                                                                                                                                      AddDef(CMD_MOVE_UP, "Move up")
#define CMD_MOVE_DOWN 2105
                                                                                                                                                                                                        AddDef(CMD_MOVE_DOWN, "Move down")
#define CMD_ZOOM_IN 2106
                                                                                                                                                                                                          AddDef(CMD_ZOOM_IN, "Zoom in")
#define CMD_ZOOM_OUT 2107
                                                                                                                                                                                                            AddDef(CMD_ZOOM_OUT, "Zoom out")

// Viewport commands
#define CMD_TOGGLE_AXISALIGNED 2200
                                                                                                                                                                                                              AddDef(CMD_TOGGLE_AXISALIGNED, "Toggle axis aligned view")
#define CMD_TOGGLE_FULLBRIGHT 2201
                                                                                                                                                                                                                AddDef(CMD_TOGGLE_FULLBRIGHT, "Toggle fullbright")
#define CMD_TOGGLE_FULL_VIEWPORT 2202
                                                                                                                                                                                                                  AddDef(CMD_TOGGLE_FULL_VIEWPORT, "Toggle full viewport")
#define CMD_AUTOZOOM 2203
                                                                                                                                                                                                                    AddDef(CMD_AUTOZOOM, "Autozoom to selected brushes")
#define CMD_DEPTHCLIP_INC 2204
                                                                                                                                                                                                                      AddDef(CMD_DEPTHCLIP_INC, "Increase depthclip")
#define CMD_DEPTHCLIP_DEC 2205
                                                                                                                                                                                                                        AddDef(CMD_DEPTHCLIP_DEC, "Decrease depthclip")
#define CMD_TOGGLE_PTS 2206
                                                                                                                                                                                                                          AddDef(CMD_TOGGLE_PTS, "Toggle display of pts/lin file")
#define CMD_HIDEALLBUTSEL 2207
                                                                                                                                                                                                                            AddDef(CMD_HIDEALLBUTSEL, "Hide everything that isn't selected")

#define CMD_TOGGLE_PERSP 2208
                                                                                                                                                                                                                              AddDef(CMD_TOGGLE_PERSP, "Toggle perspective (3d) view")
#define CMD_TOGGLE_POLY 2209
                                                                                                                                                                                                                                AddDef(CMD_TOGGLE_POLY, "Toggle polygon view")

#define CMD_TOGGLE_GRID 2210
                                                                                                                                                                                                                                  AddDef(CMD_TOGGLE_GRID, "Toggle grid")
#define CMD_GRID_INC 2211
                                                                                                                                                                                                                                    AddDef(CMD_GRID_INC, "Increase grid size")
#define CMD_GRID_DEC 2212
                                                                                                                                                                                                                                      AddDef(CMD_GRID_DEC, "Decrease grid size")

#define CMD_PAL_GAME 2213
                                                                                                                                                                                                                                        AddDef(CMD_PAL_GAME, "Switch to Quake's palette")
#define CMD_PAL_QUEST 2214
                                                                                                                                                                                                                                          AddDef(CMD_PAL_QUEST, "Switch to Quest's palette")

#define CMD_TOGGLE_BSP 2215
                                                                                                                                                                                                                                            AddDef(CMD_TOGGLE_BSP, "Toggle BSP view")
#define CMD_BSP_REBUILD 2216
                                                                                                                                                                                                                                              AddDef(CMD_BSP_REBUILD, "Rebuild BSP tree")
#define CMD_TOGGLE_TEXTURED 2217
                                                                                                                                                                                                                                                AddDef(CMD_TOGGLE_TEXTURED, "Toggle textured preview")
#define CMD_TOGGLE_TEXC 2218
                                                                                                                                                                                                                                                  AddDef(CMD_TOGGLE_TEXC, "Toggle correct textured preview")
#define CMD_TOGGLE_SHADED 2219
                                                                                                                                                                                                                                                    AddDef(CMD_TOGGLE_SHADED, "Toggle shaded view")
#define CMD_TOGGLE_LIGHT 2220
                                                                                                                                                                                                                                                      AddDef(CMD_TOGGLE_LIGHT, "Toggle lighted preview")
#define CMD_TOGGLE_LIGHTS 2221
                                                                                                                                                                                                                                                        AddDef(CMD_TOGGLE_LIGHTS, "Toggle lighted preview with shadows")

#define CMD_TOGGLE_TEXLOCK 2222
                                                                                                                                                                                                                                                          AddDef(CMD_TOGGLE_TEXLOCK, "Toggle texture locking")

#define CMD_TOGGLE_NUM_VPORTS 2223
                                                                                                                                                                                                                                                            AddDef(CMD_TOGGLE_NUM_VPORTS, "Toggle number of viewports")

// CSG commands
#define CMD_CSG_INTERSECT 2300
                                                                                                                                                                                                                                                              AddDef(CMD_CSG_INTERSECT, "CSG intersection")
#define CMD_CSG_SUBTRACT 2301
                                                                                                                                                                                                                                                                AddDef(CMD_CSG_SUBTRACT, "CSG subtraction")
#define CMD_CSG_MAKEHOLLOW 2302
                                                                                                                                                                                                                                                                  AddDef(CMD_CSG_MAKEHOLLOW, "CSG make hollow")
#define CMD_CSG_MAKEROOM 2303
                                                                                                                                                                                                                                                                    AddDef(CMD_CSG_MAKEROOM, "CSG make room")
#define CMD_WELD 2304
                                                                                                                                                                                                                                                                      AddDef(CMD_WELD, "Weld selected brushes")
#define CMD_JOIN 2305
                                                                                                                                                                                                                                                                        AddDef(CMD_JOIN, "Join two brushes")

#define CMD_PLANE_NEW 2350
                                                                                                                                                                                                                                                                          AddDef(CMD_PLANE_NEW, "Create a new clipping plane")
#define CMD_PLANE_SPLIT 2351
                                                                                                                                                                                                                                                                            AddDef(CMD_PLANE_SPLIT, "Split brushes with plane")

// Undo commands
#define CMD_UNDO 2400
                                                                                                                                                                                                                                                                              AddDef(CMD_UNDO, "Undo last change")

// Map switching commands
#define CMD_MAP0 2500
                                                                                                                                                                                                                                                                                AddDef(CMD_MAP0, "Switch to map 0")
#define CMD_MAP1 2501
                                                                                                                                                                                                                                                                                  AddDef(CMD_MAP1, "Switch to map 1")
#define CMD_MAP2 2502
                                                                                                                                                                                                                                                                                    AddDef(CMD_MAP2, "Switch to map 2")
#define CMD_MAP3 2503
                                                                                                                                                                                                                                                                                      AddDef(CMD_MAP3, "Switch to map 3")
#define CMD_MAP4 2504
                                                                                                                                                                                                                                                                                        AddDef(CMD_MAP4, "Switch to map 4")

#define CMD_FINDLEAK 2600
                                                                                                                                                                                                                                                                                          AddDef(CMD_FINDLEAK, "Find a leak")

#define CMD_GET_DISTANCE 2601
                                                                                                                                                                                                                                                                                            AddDef(CMD_GET_DISTANCE, "Get the distance between two points")

#define CMD_DEBUG_1 10000
                                                                                                                                                                                                                                                                                              AddDef(CMD_DEBUG_1, "Internal debug thing")
#define CMD_DEBUG_2 10001
                                                                                                                                                                                                                                                                                                AddDef(CMD_DEBUG_2, "Internal debug thing")
#define CMD_DEBUG_3 10002
                                                                                                                                                                                                                                                                                                  AddDef(CMD_DEBUG_3, "Internal debug thing")

                                                                                                                                                                                                                                                                                                    {0, 0, 0}};
#define NUM_CMDS (sizeof(cmds) / sizeof(cmds[0]) - 1)

void
WriteKeyHelp(void)
{
  FILE* f;
  int i, j, k, l;
  cfg_t* c;

  f = fopen("keys.hlp", "wt");

  fprintf(f, "keys.hlp generated by Quest " QUEST_VER "\n");

  for (i = 0; i < NUM_MODES; i++)
  {
    c = &cfgs[i];
    l = fprintf(f, "\n%s:\n", modes[i].desc);
    for (j = 2; j < l; j++)
      fprintf(f, "-");
    fprintf(f, "\n");

    for (j = 0; j < c->n_defs; j++)
    {
      l = 0;
      for (k = 0; k < NUM_K_FLAGS - 2; k++) // Don't print the repeat
      {
        if (c->keys[j] & k_flags[k].num)
        {
          l += fprintf(f, "%s-", k_flags[k].desc);
        }
      }
      for (k = 0; k < NUM_KEYS; k++)
      {
        if (key_defs[k].num == (c->keys[j] & 0xff))
        {
          break;
        }
      }
      if (k == NUM_KEYS)
      {
        printf("can't find key %04x\n", c->keys[j]);
        l += fprintf(f, "Unknown key %04x", c->keys[j]);
      }
      else
      {
        l += fprintf(f, "%s", key_defs[k].desc);
      }
      for (; l < 30; l++)
        fprintf(f, " ");

      for (k = 0; k < NUM_CMDS; k++)
      {
        if (cmds[k].num == c->cmds[j])
          break;
      }
      if (k == NUM_CMDS)
      {
        fprintf(f, "Unknown command");
      }
      else
      {
        fprintf(f, "%s\n", cmds[k].desc);
      }
    }
  }

  fclose(f);
}

void
WriteKeyDef(void)
{
  FILE* f;
  int i;

  f = fopen("keys.def", "wt");

  fprintf(f, "Key flags:\n"
             "----------\n");
  for (i = 0; i < NUM_K_FLAGS; i++)
    if (k_flags[i].name && k_flags[i].desc)
      fprintf(f, "%-20s %s\n", k_flags[i].name, k_flags[i].desc);

  fprintf(f, "\n"
             "Keys:\n"
             "-----\n");
  for (i = 0; i < NUM_KEYS; i++)
    if (key_defs[i].name && key_defs[i].desc)
      fprintf(f, "%-20s %s\n", key_defs[i].name, key_defs[i].desc);

  fprintf(f, "\n"
             "Commands:\n"
             "---------\n");
  for (i = 0; i < NUM_CMDS; i++)
    if (cmds[i].name && cmds[i].desc)
      fprintf(f, "%-30s %s\n", cmds[i].name, cmds[i].desc);

  fclose(f);
}

static void
CheckCfgErr(int mode)
{
  int i, j;
  cfg_t* c;

  c = &cfgs[mode];
  for (i = 0; i < c->n_defs; i++)
  {
    for (j = i + 1; j < c->n_defs; j++)
    {
      if (c->keys[i] == c->keys[j])
        HandleError("CheckCfgs", "Duplicate key in %s mode!", modes[mode].name);
    }
  }
}

void
CheckCfgs(void)
{
  int i, j;

  for (i = 0; i < NUM_MODES; i++)
  {
    CheckCfgErr(i);
  }

  for (i = 0; i < NUM_CMDS; i++)
  {
    for (j = i + 1; j < NUM_CMDS; j++)
    {
      if (i == j)
        continue;
      if (cmds[i].num == cmds[j].num)
        HandleError("CheckCfgs/Internal", "Duplicate cmd num %i!", cmds[i].num);
      if (!strcmp(cmds[i].name, cmds[j].name))
        HandleError("CheckCfgs/Internal", "Duplicate cmd name %s!", cmds[i].name);
    }
  }
}

int
ExecCmd(int cmd)
{
  char temp_str[256];

  brushref_t* b;
  fsel_t* f;

  int dx, dy, dz;

  int redraw;

  Move(M.display.active_vport, MOVE_FORWARD, &dx, &dy, &dz, 100);
  dx += M.display.vport[M.display.active_vport].camera_pos.x;
  dy += M.display.vport[M.display.active_vport].camera_pos.y;
  dz += M.display.vport[M.display.active_vport].camera_pos.z;

  redraw = 0;
  switch (cmd)
  {
    case CMD_EXIT:
      if (QUI_YesNo("Exit", "Are you sure?", "Yes", "No"))
      {
        status.exit_flag = TRUE;
      }
      break;

    case CMD_MODE_NEXT:
      if (status.edit_mode == BRUSH)
        status.edit_mode = FACE;
      else if (status.edit_mode == FACE)
        status.edit_mode = ENTITY;
      else if (status.edit_mode == ENTITY)
        status.edit_mode = MODEL;
      else if (status.edit_mode == MODEL)
        status.edit_mode = BRUSH;

      ClearSelBrushes();
      ClearSelVerts();
      ClearSelEnts();
      ClearSelFaces();
      QUI_RedrawWindow(STATUS_WINDOW);
      redraw = 2;
      break;
    case CMD_MODE_LAST:
      if (status.edit_mode == BRUSH)
        status.edit_mode = MODEL;
      else if (status.edit_mode == FACE)
        status.edit_mode = BRUSH;
      else if (status.edit_mode == ENTITY)
        status.edit_mode = FACE;
      else if (status.edit_mode == MODEL)
        status.edit_mode = ENTITY;

      ClearSelBrushes();
      ClearSelVerts();
      ClearSelEnts();
      ClearSelFaces();
      QUI_RedrawWindow(STATUS_WINDOW);
      redraw = 2;
      break;

    case CMD_MESSAGE_DUMP:
      DumpMessages();
      NewMessage("Messages dumped to message.out.");
      break;

    case CMD_CAMERA_REINIT:
      InitCamera();
      NewMessage("Camera Re-Initialized");
      redraw = 2;
      break;

    case CMD_EDIT_DESELECT:
      ClearSelVerts();
      ClearSelFaces();
      ClearSelBrushes();
      ClearSelEnts();
      redraw = 2;
      break;

    case CMD_BRUSH_SELECT_ALL:
      HighlightAllBrushes();
      break;
    case CMD_BRUSH_COPY:
      CopyBrush();
      break;
    case CMD_BRUSH_PASTE:
      PasteBrush();
      redraw = 2;
      break;
    case CMD_BRUSH_ROTATE:
      RotateBrush(M.display.active_vport);
      break;
    case CMD_BRUSH_MIRROR:
      MirrorBrush(M.display.active_vport);
      break;
    case CMD_BRUSH_SCALE:
      ScaleBrush(TRUE);
      break;
    case CMD_BRUSH_SCALE_VER:
      ScaleBrush(FALSE);
      break;
    case CMD_BRUSH_DELETE:
      SUndo(UNDO_NONE, UNDO_DELETE);
      DeleteBrush();
      redraw = 2;
      break;

    case CMD_BRUSH_CREATE_CUBE:
      SUndo(UNDO_NONE, UNDO_NONE);
      Move(M.display.active_vport, MOVE_FORWARD, &dx, &dy, &dz, 100);
      AddBrush(TEXRECT,
               M.display.vport[M.display.active_vport].camera_pos.x + dx,
               M.display.vport[M.display.active_vport].camera_pos.y + dy,
               M.display.vport[M.display.active_vport].camera_pos.z + dz,
               32,
               32,
               32,
               0);
      status.edit_mode = BRUSH;
      redraw = 2;
      break;

    case CMD_BRUSH_CREATE_PRISM:
      CreateBrush(NPRISM);
      redraw = 2;
      break;

    case CMD_BRUSH_CREATE_ROOM:
      AddRoom();
      status.edit_mode = BRUSH;
      redraw = 2;
      break;

    case CMD_BRUSH_CREATE_PYRAMID:
      CreateBrush(PYRAMID);
      redraw = 2;
      break;

    case CMD_BRUSH_CREATE_DODEC:
      CreateBrush(DODEC);
      redraw = 2;
      break;

    case CMD_BRUSH_CREATE_ICOS:
      CreateBrush(ICOS);
      redraw = 2;
      break;

    case CMD_BRUSH_CREATE_BUCKY:
      CreateBrush(BUCKY);
      redraw = 2;
      break;

    case CMD_BRUSH_CREATE_TORUS:
      CreateBrush(TORUS);
      redraw = 2;
      break;

    case CMD_BRUSH_GRIDSNAP:
      SnapSelectedBrushes();
      redraw = 2;
      break;

    case CMD_BRUSH_VERTSNAP:
      SnapSelectedVerts();
      redraw = 2;
      break;

    case CMD_BRUSH_VERTSNAPTO:
      if (M.display.num_vselected)
      {
        SnapToVertex();
      }
      else
      {
        HandleError("Snap to Vertex", "Must have at least one vertex selected!");
      }
      redraw = 2;
      break;

    case CMD_BRUSH_SWITCH_SUBTRACT:
      if (M.display.num_bselected)
      {
        brushref_t* bs;
        int s;

        for (bs = M.display.bsel; bs; bs = bs->Next)
        {
          if ((bs->Brush->bt->type != BR_NORMAL) && (bs->Brush->bt->type != BR_SUBTRACT))
          {
            HandleError("Switch subtractive", "Can't have non-normal brushes selected!");
            break;
          }
        }
        if (bs)
          break;

        SUndo(UNDO_NONE, UNDO_CHANGE);

        if (M.display.bsel->Brush->bt->type == BR_NORMAL)
        {
          NewMessage("Selected brushes change to %ssubtractive.", "");
          s = BR_SUBTRACT;
        }
        else
        {
          NewMessage("Selected brushes change to %ssubtractive.", "non-");
          s = BR_NORMAL;
        }

        for (bs = M.display.bsel; bs; bs = bs->Next)
          B_ChangeType(bs->Brush, s);

        redraw = 2;
      }
      else
        HandleError("Switch subtractive", "Must have at least one brush selected!");
      break;

    case CMD_FACE_SELECT_ALL:
      HighlightAllFaces();
      break;

    case CMD_FACE_SPLIT:
      SUndo(UNDO_NONE, UNDO_CHANGE);
      SplitFace();
      break;
    case CMD_FACE_SPLIT_EDGE:
      SUndo(UNDO_NONE, UNDO_CHANGE);
      SplitLine();
      break;
    case CMD_FACE_SPLIT_BOTH:
      SUndo(UNDO_NONE, UNDO_CHANGE);
      SplitLineFace();
      break;

    case CMD_FACE_EDIT:
      SUndo(UNDO_NONE, UNDO_CHANGE);
      EditFace();
      redraw = 2;
      break;
    case CMD_FACE_FLAGS:
      SUndo(UNDO_NONE, UNDO_CHANGE);
      Game.tex.settexdef();
      break;

    case CMD_VERT_LEFT:
      MoveSelVert(MOVE_LEFT, 1);
      redraw = 2;
      break;
    case CMD_VERT_RIGHT:
      MoveSelVert(MOVE_RIGHT, 1);
      redraw = 2;
      break;
    case CMD_VERT_UP:
      MoveSelVert(MOVE_UP, 1);
      redraw = 2;
      break;
    case CMD_VERT_DOWN:
      MoveSelVert(MOVE_DOWN, 1);
      redraw = 2;
      break;
    case CMD_VERT_FORWARD:
      MoveSelVert(MOVE_FORWARD, 1);
      redraw = 2;
      break;
    case CMD_VERT_BACKWARD:
      MoveSelVert(MOVE_BACKWARD, 1);
      redraw = 2;
      break;

    case CMD_TVERT_LEFT:
      MoveSelTVert(MOVE_LEFT);
      redraw = 2;
      break;
    case CMD_TVERT_RIGHT:
      MoveSelTVert(MOVE_RIGHT);
      redraw = 2;
      break;
    case CMD_TVERT_UP:
      MoveSelTVert(MOVE_UP);
      redraw = 2;
      break;
    case CMD_TVERT_DOWN:
      MoveSelTVert(MOVE_DOWN);
      redraw = 2;
      break;

    case CMD_TVERT_S_LEFT:
      ScaleSelTVert(MOVE_LEFT);
      redraw = 2;
      break;
    case CMD_TVERT_S_RIGHT:
      ScaleSelTVert(MOVE_RIGHT);
      redraw = 2;
      break;
    case CMD_TVERT_S_UP:
      ScaleSelTVert(MOVE_UP);
      redraw = 2;
      break;
    case CMD_TVERT_S_DOWN:
      ScaleSelTVert(MOVE_DOWN);
      redraw = 2;
      break;

    case CMD_FACE_NEXT:
      if (M.display.num_fselected != 1)
        break;
      M.display.fsel->facenum++;
      if (M.display.fsel->facenum >= M.display.fsel->Brush->num_planes)
        M.display.fsel->facenum = 0;
      redraw = 2;
      break;
    case CMD_FACE_PREV:
      if (M.display.num_fselected != 1)
        break;
      M.display.fsel->facenum--;
      if (M.display.fsel->facenum < 0)
        M.display.fsel->facenum = M.display.fsel->Brush->num_planes - 1;
      redraw = 2;
      break;

    case CMD_ENTITY_COPY:
      CopyEntity();
      break;
    case CMD_ENTITY_PASTE:
      PasteEntity();
      redraw = 2;
      break;
    case CMD_ENTITY_ROTATE:
      SUndo(UNDO_CHANGE, UNDO_NONE);
      RotateEntity();
      break;
    case CMD_ENTITY_EDIT:
      SUndo(UNDO_CHANGE, UNDO_NONE);
      EditEntity();
      redraw = 2;
      break;
    case CMD_ENTITY_DELETE:
      SUndo(UNDO_DELETE, UNDO_CHANGE);
      DeleteEntities();
      redraw = 2;
      break;
    case CMD_ENTITY_CREATE:
      EntityPicker(temp_str, CLASS_POINT);
      if (temp_str[0] != 0)
      {
        SUndo(UNDO_NONE, UNDO_NONE);
        AddEntity(temp_str, dx, dy, dz);

        status.edit_mode = ENTITY;
        ClearSelBrushes();
        ClearSelVerts();
        ClearSelEnts();
        ClearSelFaces();
        QUI_RedrawWindow(STATUS_WINDOW);
      }
      redraw = 2;
      break;

    case CMD_ENTITY_LEFT:
      MoveSelEnt(MOVE_LEFT, 1);
      redraw = 2;
      break;
    case CMD_ENTITY_RIGHT:
      MoveSelEnt(MOVE_RIGHT, 1);
      redraw = 2;
      break;
    case CMD_ENTITY_UP:
      MoveSelEnt(MOVE_UP, 1);
      redraw = 2;
      break;
    case CMD_ENTITY_DOWN:
      MoveSelEnt(MOVE_DOWN, 1);
      redraw = 2;
      break;
    case CMD_ENTITY_FORWARD:
      MoveSelEnt(MOVE_FORWARD, 1);
      redraw = 2;
      break;
    case CMD_ENTITY_BACKWARD:
      MoveSelEnt(MOVE_BACKWARD, 1);
      redraw = 2;
      break;

    case CMD_MODEL_LIST:
      ModelList();
      break;
    case CMD_MODEL_CREATE:
      CreateModel();
      break;
    case CMD_MODEL_ADD:
      SUndo(UNDO_NONE, UNDO_CHANGE);
      AddToModel();
      break;
    case CMD_LINK_CREATE:
      CreateLink();
      break;

    case CMD_CURVE_NEW:
      Curve_NewBrush();
      redraw = 2;
      break;

    case CMD_NEW:
      if (QUI_YesNo("New Map", "Are you sure?", "Yes", "No"))
      {
        UnloadMap();

        /* Start a new map */
        M.mapfilename[0] = 0;
        M.EntityHead = Q_malloc(sizeof(entity_t));
        InitEntity(M.EntityHead);
        SetKeyValue(M.EntityHead, "classname", "worldspawn");
        M.num_entities = 1;
        M.WorldSpawn = M.EntityHead;
        CreateWorldGroup();
        InitViewports();
        InitCamera();
        redraw = 2;

        ClearUndo();
      }
      M.modified = 0;
      break;

    case CMD_LOAD:
      if (!FileCmd(LOAD_IT))
      {
        /* Error loading, just start a new map */
        UnloadMap();
        M.mapfilename[0] = 0;
        M.EntityHead = Q_malloc(sizeof(entity_t));
        InitEntity(M.EntityHead);
        SetKeyValue(M.EntityHead, "classname", "worldspawn");
        M.WorldSpawn = M.EntityHead;
        CreateWorldGroup();
        InitCamera();
        M.num_entities = 1;
#ifdef _UNIX
        X_SetWindowTitle(M.mapfilename);
#endif

        ClearUndo();
      }
      M.modified = 0;
      QUI_RedrawWindow(STATUS_WINDOW);
      redraw = 2;
      break;

    case CMD_SAVE:
      Game.map.savemap(M.mapfilename);
      NewMessage("MAP saved as %s", M.mapfilename);
      break;

    case CMD_SAVE_AS:
      FileCmd(SAVE_IT);
      break;

    case CMD_GRP_LOAD:
      FileCmd(GROUP_LOAD);
      redraw = 2;
      break;

    case CMD_GRP_SAVE:
      M.CurGroup = GroupPicker();
      if (M.CurGroup != NULL)
        FileCmd(GROUP_SAVE);
      M.CurGroup = M.WorldGroup;
      break;

    case CMD_PTS_LOAD:
      FileCmd(PTS_LOAD);
      status.draw_pts = 1;
      redraw = 2;
      break;

    case CMD_MAPINFO:
      MapInfo();
      break;

    case CMD_SETPREFS:
      SetPrefs();
      SetMouseSensitivity(mouse_sens_x, mouse_sens_y);
      SetPal(PAL_QUEST);
      redraw = 2;
      break;

    case CMD_CHECK_MAP:
      ConsistencyCheck();
      redraw = 2;
      break;

    case CMD_SCREEN_SHOT:
      take_screenshot = TRUE;
      break;

    case CMD_TEXTURE_PICK:
      TexturePicker(texturename, temp_str);
      if (temp_str[0] != 0)
      {
        strcpy(texturename, temp_str);

        if (M.display.num_bselected || M.display.num_fselected)
          SUndo(UNDO_NONE, UNDO_CHANGE);

        for (b = M.display.bsel; b; b = b->Next)
          ApplyTexture(b->Brush, texturename);
        for (f = M.display.fsel; f; f = f->Next)
          ApplyFaceTexture(f, texturename);
        if (M.display.num_bselected || M.display.num_fselected)
          NewMessage("%s applied to all selected brushes.", texturename);
        else
          NewMessage("%s selected as active texture.", texturename);
        QUI_RedrawWindow(STATUS_WINDOW);
      }
      redraw = 2;
      break;

    case CMD_SELECT_WAD:
      if (Game.tex.cache)
      {
        HandleError("SelectWad", "Not valid in this configuration!");
      }
      else
      {
        FileCmd(WAD_IT);
      }
      break;

    case CMD_REREAD_CACHE:
      ReadCache(1);
      NewMessage("Cache reread.");
      break;

    case CMD_FIX_DUPLICATES:
      FixDups(); // TODO
      break;

    case CMD_GROUP_POPUP:
      GroupPopup();
      break;

    case CMD_BUILD_POPUP:
      AutoBuildPopup();
      break;

    case CMD_VIDEO_REINIT:
      SetMode(options.vid_mode);
      memset(video.ScreenBuffer, 0, video.ScreenBufferSize);
      InitMouse();
      SetPal(PAL_QUEST);
      QUI_RedrawWindow(MENU_WINDOW);
      QUI_RedrawWindow(MESG_WINDOW);
      QUI_RedrawWindow(TOOL_WINDOW);
      QUI_RedrawWindow(STATUS_WINDOW);
      redraw = 2;
      if (!status.pop_menu)
        PopUpMenuWin();
      RefreshScreen();
      break;

    case CMD_LOOK_UP:
      LookUp(M.display.active_vport);
      redraw = 1;
      break;
    case CMD_LOOK_DOWN:
      LookDown(M.display.active_vport);
      redraw = 1;
      break;
    case CMD_TURN_LEFT:
      TurnLeft(M.display.active_vport);
      redraw = 1;
      break;
    case CMD_TURN_RIGHT:
      TurnRight(M.display.active_vport);
      redraw = 1;
      break;
    case CMD_ROLL_LEFT:
      RollLeft(M.display.active_vport);
      redraw = 1;
      break;
    case CMD_ROLL_RIGHT:
      RollRight(M.display.active_vport);
      redraw = 1;
      break;

    case CMD_MOVE_FORWARD:
      MoveCamera(M.display.active_vport, MOVE_FORWARD);
      redraw = 1;
      break;
    case CMD_MOVE_BACKWARD:
      MoveCamera(M.display.active_vport, MOVE_BACKWARD);
      redraw = 1;
      break;
    case CMD_MOVE_LEFT:
      MoveCamera(M.display.active_vport, MOVE_LEFT);
      redraw = 1;
      break;
    case CMD_MOVE_RIGHT:
      MoveCamera(M.display.active_vport, MOVE_RIGHT);
      redraw = 1;
      break;
    case CMD_MOVE_DOWN:
      MoveCamera(M.display.active_vport, MOVE_DOWN);
      redraw = 1;
      break;
    case CMD_MOVE_UP:
      MoveCamera(M.display.active_vport, MOVE_UP);
      redraw = 1;
      break;

    case CMD_ZOOM_IN:
      M.display.vport[M.display.active_vport].zoom_amt *= status.zoom_speed;

      if (M.display.vport[M.display.active_vport].zoom_amt > 12)
        M.display.vport[M.display.active_vport].zoom_amt = 12;
      if (M.display.vport[M.display.active_vport].zoom_amt < 0.125 / 20)
        M.display.vport[M.display.active_vport].zoom_amt = 0.125 / 20;

      redraw = 1;
      break;
    case CMD_ZOOM_OUT:
      M.display.vport[M.display.active_vport].zoom_amt /= status.zoom_speed;

      if (M.display.vport[M.display.active_vport].zoom_amt > 12)
        M.display.vport[M.display.active_vport].zoom_amt = 12;
      if (M.display.vport[M.display.active_vport].zoom_amt < 0.125 / 20)
        M.display.vport[M.display.active_vport].zoom_amt = 0.125 / 20;

      redraw = 1;
      break;

    case CMD_TOGGLE_PERSP:
      if (M.display.vport[M.display.active_vport].mode != NOPERSP)
      {
        M.display.vport[M.display.active_vport].mode = NOPERSP;
        M.display.vport[M.display.active_vport].axis_aligned = 1;
      }
      else
      {
        M.display.vport[M.display.active_vport].mode = WIREFRAME;
      }
      redraw = 1;
      break;

    case CMD_TOGGLE_FULLBRIGHT:
      if (!M.display.vport[M.display.active_vport].fullbright)
        M.display.vport[M.display.active_vport].fullbright = TRUE;
      else
        M.display.vport[M.display.active_vport].fullbright = FALSE;
      NewMessage("Fullbright mode in viewport %d turned %s.",
                 M.display.active_vport,
                 M.display.vport[M.display.active_vport].fullbright ? "on" : "off");
      redraw = 1;
      break;

    case CMD_TOGGLE_AXISALIGNED:
      if (M.display.vport[M.display.active_vport].mode != NOPERSP)
      {
        if (M.display.vport[M.display.active_vport].axis_aligned)
          M.display.vport[M.display.active_vport].axis_aligned = 0;
        else
          M.display.vport[M.display.active_vport].axis_aligned = 1;
        NewMessage("Axis aligned mode in viewport %i turned %s.",
                   M.display.active_vport,
                   M.display.vport[M.display.active_vport].axis_aligned ? "on" : "off");
        redraw = 1;
      }
      break;

    case CMD_TOGGLE_POLY:
      if (M.display.vport[M.display.active_vport].mode != SOLID)
      {
        SetPal(PAL_TEXTURE);
        M.display.vport[M.display.active_vport].mode = SOLID;
      }
      else
      {
        SetPal(PAL_QUEST);
        M.display.vport[M.display.active_vport].mode = WIREFRAME;
      }
      redraw = 1;
      break;

    case CMD_AUTOZOOM:
      AutoZoom();
      redraw = 2;
      break;

    case CMD_TOGGLE_FULL_VIEWPORT:
      if (M.display.vp_full)
        M.display.vp_full = 0;
      else
        M.display.vp_full = 1;
      UpdateViewportPositions();
      redraw = 2;
      break;

    case CMD_TOGGLE_NUM_VPORTS:
      if (M.display.num_vports == 4)
        InitViewports();
      else
      {
        M.display.num_vports = 4;
        M.display.vport[0].f_xmin = 0;
        M.display.vport[0].f_ymin = 0;
        M.display.vport[0].f_xmax = 0.5;
        M.display.vport[0].f_ymax = 0.5;

        M.display.vport[1].f_xmin = 0;
        M.display.vport[1].f_ymin = 0.5;
        M.display.vport[1].f_xmax = 0.5;
        M.display.vport[1].f_ymax = 1;

        M.display.vport[2].f_xmin = 0.5;
        M.display.vport[2].f_ymin = 0;
        M.display.vport[2].f_xmax = 1;
        M.display.vport[2].f_ymax = 0.5;

        M.display.vport[3].f_xmin = 0.5;
        M.display.vport[3].f_ymin = 0.5;
        M.display.vport[3].f_xmax = 1;
        M.display.vport[3].f_ymax = 1;
        UpdateViewportPositions();
      }
      redraw = 2;
      break;

    case CMD_TOGGLE_GRID:
      if (M.display.vport[M.display.active_vport].grid_type == NOGRID)
        M.display.vport[M.display.active_vport].grid_type = GRID;
      else if (M.display.vport[M.display.active_vport].grid_type == GRID)
        M.display.vport[M.display.active_vport].grid_type = ALIGN;
      else
        //		if (M.display.vport[M.display.active_vport].grid_type == ALIGN)
        M.display.vport[M.display.active_vport].grid_type = NOGRID;
      redraw = 2;
      break;

    case CMD_DEPTHCLIP_INC:
      status.depth_clip *= 1.2;
      redraw = 2;
      break;
    case CMD_DEPTHCLIP_DEC:
      status.depth_clip /= 1.2;
      if (status.depth_clip < 10)
        status.depth_clip = 10;
      redraw = 2;
      break;

    case CMD_GRID_INC:
      status.snap_size *= 2;
      NewMessage("New Snap Size: %d", status.snap_size);
      redraw = 2;
      break;
    case CMD_GRID_DEC:
      status.snap_size /= 2;
      if (status.snap_size < 1)
        status.snap_size = 1;
      NewMessage("New Snap Size: %d", status.snap_size);
      redraw = 2;
      break;

    case CMD_PAL_GAME:
      SetPal(PAL_TEXTURE);
      redraw = 2;
      break;
    case CMD_PAL_QUEST:
      SetPal(PAL_QUEST);
      redraw = 2;
      break;

    case CMD_BSP_REBUILD:
      RebuildBSP();
      M.display.vport[M.display.active_vport].mode = BSPVIEW;
      redraw = 2;
      break;

    case CMD_TOGGLE_BSP:
      if ((M.display.vport[M.display.active_vport].mode != BSPVIEW) ||
          M.display.vport[M.display.active_vport].textured != BSP_COL)
      {
        M.display.vport[M.display.active_vport].mode = BSPVIEW;
        M.display.vport[M.display.active_vport].textured = BSP_COL;
      }
      else
      {
        M.display.vport[M.display.active_vport].mode = WIREFRAME;
      }
      SetPal(PAL_QUEST);
      redraw = 1;
      break;

    case CMD_TOGGLE_TEXTURED:
      if ((M.display.vport[M.display.active_vport].textured == BSP_TEX) &&
          (M.display.vport[M.display.active_vport].mode == BSPVIEW))
      {
        M.display.vport[M.display.active_vport].textured = 0;
        M.display.vport[M.display.active_vport].mode = WIREFRAME;
        SetPal(PAL_QUEST);
      }
      else
      {
        SetPal(PAL_TEXTURE);

        M.display.vport[M.display.active_vport].mode = BSPVIEW;
        M.display.vport[M.display.active_vport].textured = BSP_TEX;
      }
      redraw = 1;
      break;

    case CMD_TOGGLE_TEXC:
      if ((M.display.vport[M.display.active_vport].textured == BSP_TEXC) &&
          (M.display.vport[M.display.active_vport].mode == BSPVIEW))
      {
        M.display.vport[M.display.active_vport].textured = 0;
        M.display.vport[M.display.active_vport].mode = WIREFRAME;
        SetPal(PAL_QUEST);
      }
      else
      {
        SetPal(PAL_TEXTURE);

        M.display.vport[M.display.active_vport].mode = BSPVIEW;
        M.display.vport[M.display.active_vport].textured = BSP_TEXC;
      }
      redraw = 1;
      break;

    case CMD_TOGGLE_SHADED:
      if ((M.display.vport[M.display.active_vport].textured == BSP_GREY) &&
          (M.display.vport[M.display.active_vport].mode == BSPVIEW))
      {
        M.display.vport[M.display.active_vport].textured = 0;
        M.display.vport[M.display.active_vport].mode = WIREFRAME;
      }
      else
      {
        M.display.vport[M.display.active_vport].mode = BSPVIEW;
        M.display.vport[M.display.active_vport].textured = BSP_GREY;
      }
      SetPal(PAL_QUEST);
      redraw = 1;
      break;

    case CMD_TOGGLE_LIGHT:
      if ((M.display.vport[M.display.active_vport].textured == BSP_LIGHT) &&
          (M.display.vport[M.display.active_vport].mode == BSPVIEW))
      {
        M.display.vport[M.display.active_vport].textured = 0;
        M.display.vport[M.display.active_vport].mode = WIREFRAME;
        SetPal(PAL_QUEST);
      }
      else
      {
        SetPal(PAL_TEXTURE);

        M.display.vport[M.display.active_vport].mode = BSPVIEW;
        M.display.vport[M.display.active_vport].textured = BSP_LIGHT;
      }
      redraw = 1;
      break;

    case CMD_TOGGLE_LIGHTS:
      if ((M.display.vport[M.display.active_vport].textured == BSP_LIGHTS) &&
          (M.display.vport[M.display.active_vport].mode == BSPVIEW))
      {
        M.display.vport[M.display.active_vport].textured = 0;
        M.display.vport[M.display.active_vport].mode = WIREFRAME;
        SetPal(PAL_QUEST);
      }
      else
      {
        SetPal(PAL_TEXTURE);

        M.display.vport[M.display.active_vport].mode = BSPVIEW;
        M.display.vport[M.display.active_vport].textured = BSP_LIGHTS;
      }
      redraw = 1;
      break;

    case CMD_TOGGLE_PTS:
      if (status.draw_pts)
        status.draw_pts = 0;
      else
        status.draw_pts = 1;

      NewMessage("Pts/lin drawing in turned %s",
                 status.draw_pts ? "on" : "off");
      redraw = 2;
      break;

    case CMD_TOGGLE_TEXLOCK:
      if (status.texlock)
        status.texlock = 0;
      else
        status.texlock = 1;

      NewMessage("Texture locking turned %s",
                 status.texlock ? "on" : "off");
      break;

    case CMD_HIDEALLBUTSEL:
      if (M.showsel)
      {
        M.showsel = 0;
      }
      else
      {
        brush_t* b;
        entity_t* e;
        brushref_t* br;
        entityref_t* er;
        fsel_t* f;

        if (M.display.num_bselected ||
            M.display.num_eselected ||
            M.display.num_fselected)
        {
          M.showsel = 1;

          for (b = M.BrushHead; b; b = b->Next)
            b->hidden = 1;
          for (e = M.EntityHead; e; e = e->Next)
            e->hidden = 1;

          for (br = M.display.bsel; br; br = br->Next)
            br->Brush->hidden = 0;
          for (er = M.display.esel; er; er = er->Next)
            er->Entity->hidden = 0;
          for (f = M.display.fsel; f; f = f->Next)
            f->Brush->hidden = 0;
        }
        else
        {
          NewMessage("Nothing selected!");
        }
      }
      redraw = 2;
      break;

    case CMD_CSG_INTERSECT:
      if (M.display.num_bselected == 2)
      {
        if (!AddBrushes(M.display.bsel->Brush, M.display.bsel->Next->Brush))
        {
          HandleError("Brush Intersection", "Couldn't intersect brushes!");
        }
      }
      else
      {
        HandleError("Brush Intersection", "Must have exactly 2 brushes selected!");
      }
      redraw = 2;
      break;
    case CMD_CSG_SUBTRACT:
      BooleanSubtraction();
      redraw = 2;
      break;
    case CMD_CSG_MAKEHOLLOW:
      MakeHollow();
      redraw = 2;
      break;
    case CMD_CSG_MAKEROOM:
      MakeRoom();
      redraw = 2;
      break;

    case CMD_WELD:
      Weld();
      break;

    case CMD_JOIN:
      if (M.display.num_bselected == 2)
      {
        if (!JoinBrushes(M.display.bsel->Brush, M.display.bsel->Next->Brush))
        {
          HandleError("Brush Join", "Couldn't join brushes!");
        }
        redraw = 2;
      }
      else
      {
        HandleError("Brush Join", "Must have exactly 2 brushes selected!");
      }
      break;

    case CMD_PLANE_NEW:
      NewClipPlane();
      redraw = 2;
      break;
    case CMD_PLANE_SPLIT:
      PlaneSplit();
      redraw = 2;
      break;

    case CMD_UNDO:
      Undo();
      redraw = 2;
      break;

    case CMD_MAP0:
      SwitchMap(0, 1);
      redraw = 2;
      break;
    case CMD_MAP1:
      SwitchMap(1, 1);
      redraw = 2;
      break;
    case CMD_MAP2:
      SwitchMap(2, 1);
      redraw = 2;
      break;
    case CMD_MAP3:
      SwitchMap(3, 1);
      redraw = 2;
      break;
    case CMD_MAP4:
      SwitchMap(4, 1);
      redraw = 2;
      break;

    case CMD_FINDLEAK:
      FindLeak();
      redraw = 2;
      break;

    case CMD_GET_DISTANCE:
      CheckDistance();
      break;

    case CMD_DEBUG_1:
      Time(Profile(0));
      Time(Profile(1));
      Time(Profile(2));
      break;

    case CMD_DEBUG_2:
      /* TODO see portals.c
            {
               int i;
               char buf[256];

               i=SelectFile("Load portals",".prt","Load",buf);
               if (i)
                  LoadPortals(buf);
            }*/
      Quake3_JumpPad(); /* TODO */
      redraw = 2;
      break;

    case CMD_DEBUG_3: /* TODO */
      ReplaceTexture();
      redraw = 2;
      break;

    default:
      HandleError("ExecCmd", "Unknown command %i!", cmd);
      break;
  }

  return redraw;
}
