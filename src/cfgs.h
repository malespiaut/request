/*
cfgs.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef CFGS_H
#define CFGS_H

typedef struct
{
  int num;
  const char* name;
  const char* desc;
} def_t;

extern def_t key_defs[];

extern def_t k_flags[];

extern def_t cmds[];

#define NUM_MODES 6

#define MODE_MOVE 0
#define MODE_COMMON 1
#define MODE_BRUSH 2
#define MODE_FACE 3
#define MODE_ENTITY 4
#define MODE_MODEL 5

extern def_t modes[NUM_MODES];

int FindDef(def_t* defs, char* name);

const char* GetDefName(def_t* defs, int v);

void AddCfg(int mode, int key, int cmd);

void WriteKeyHelp(void);

void WriteKeyDef(void);

void CheckCfg(int mode);

void CheckCfgs(void);

int ExecCmd(int cmd);

#endif
