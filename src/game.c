/*
game.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "defines.h"
#include "types.h"

#include "game.h"

#include "version.h"

#include "games/qengine/halflife.h"
#include "games/qengine/heretic2.h"
#include "games/qengine/quake.h"
#include "games/qengine/quake2.h"
#include "games/qengine/quake3.h"
#include "games/qengine/sin.h"

#include "error.h"
#include "status.h"
#include "token.h"

typedef struct
{
  const char* name;
  void (*init)(void);
} lookup_game_t;

extern void Q_I_Init(void);

static lookup_game_t games[] =
  {
    {"Quake", Q_Init},

    /* TODO */
    {"Quake-I", Q_I_Init},

    {"Quake2", Q2_Init},
    {"Quake3", Q3_Init},
    {"Sin", Sin_Init},
    {"Halflife", HL_Init},
    {"Heretic2", Her2_Init}};
#define NUM_GAMES (sizeof(games) / sizeof(games[0]))

game_t Game;

void
InitGame(void)
{
  int i;

  for (i = 0; i < NUM_GAMES; i++)
  {
    if (!stricmp(status.game_str, games[i].name))
      break;
  }
  if (i == NUM_GAMES)
    Abort("InitGame", "Invalid game in 'quest.cfg'!");

  games[i].init();
}
