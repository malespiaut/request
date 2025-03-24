// KEYBOARD.C module of Quest v1.1 Source Code
// Info on the Quest source code can be found in qsrcinfo.txt

// Copyright 1996, Trey Harrison and Chris Carollo
// For non-commercial use only. More rules apply, read legal.txt
// for a full description of usage restrictions.

/*
WARNING - full of temp. hacks
*/

#include <stdio.h>
#include <stdlib.h>
#include <vga.h>
#include <vgakeyboard.h>

#include "defines.h"
#include "types.h"

#include "keyboard.h"

#include "mouse.h"

static u_char keys[NUM_UNIQUE_KEYS];
// static u_char dummykeys[NUM_UNIQUE_KEYS]; // used in the demo playback

int k_ascii[128] =
  {
    ['A'] KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,

    ['a'] KEY_A,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,

    ['0'] KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
};

#if OLD
int
TestYN()
{
  if (TestKey(KEY_Y) || TestKey(KEY_Z))
    return 'y';
  if (TestKey(KEY_N) || TestKey(KEY_ESCAPE))
    return 'n';

  return 0;
}

int
q_getch()
{
  int i;

  while (1)
  {
    i = GetKey();
    if (i & evsChar)
      return i & 0xff;
    if (i == KEY_BACKSPC)
      return 0x7f;
    if (i == KEY_ENTER)
      return '\n';
    if (i == KEY_ESCAPE)
      return 27;
  }
  //  return vga_getch();
}
#endif

static int last_ks = -1;
static unsigned char key_handled[NUM_UNIQUE_KEYS];

static void
KBD_Handler(int key, int press)
{
  key &= 127;
  if (press)
  {
    last_ks = key;
    keys[key] = 1;
    key_handled[key] = 0;
  }
  else
    keys[key] = 0;
}

void
KBD_InstallHandler()
{
  keyboard_init();
  keyboard_translatekeys(TRANSLATE_CURSORKEYS | TRANSLATE_KEYPADENTER |
                         TRANSLATE_DIAGONAL | DONT_CATCH_CTRLC);

  keyboard_seteventhandler(KBD_Handler);
}

void
KBD_RemoveHandler()
{
  keyboard_setdefaulteventhandler();

  keyboard_close();
}

int
TestKey(int key_to_test)
{
  keyboard_update();
  /*        if (keyboard_keypressed(key_to_test)) return TRUE;
    else return FALSE;*/
  return keys[key_to_test];
}

void
ClearKeys(void)
{
}

static const char tables[2][256] =
  {
    {
      // unshifted
      0,
      0,
      '1',
      '2',
      '3',
      '4',
      '5',
      '6',
      '7',
      '8',
      '9',
      '0',
      '-',
      '=',
      0,
      0, // 00-0f
      'q',
      'w',
      'e',
      'r',
      't',
      'y',
      'u',
      'i',
      'o',
      'p',
      '[',
      ']',
      0,
      0,
      'a',
      's', // 10-1f
      'd',
      'f',
      'g',
      'h',
      'j',
      'k',
      'l',
      ';',
      '\'',
      '\\',
      0,
      '<',
      'z',
      'x',
      'c',
      'v', // 20-2f
      'b',
      'n',
      'm',
      ',',
      '.',
      '/',
      0,
      0,
      0,
      ' ',
      0,
      0,
      0,
      0,
      0,
      0, // 30-3f
      0,
      0,
      0,
      0,
      0,
      0,
      0,
    },
    {
      // with shift held
      0,
      0,
      '!',
      '@',
      '#',
      '$',
      '%',
      '^',
      '&',
      '*',
      '(',
      ')',
      '_',
      '+',
      0,
      0, // 00-0f
      'Q',
      'W',
      'E',
      'R',
      'T',
      'Y',
      'U',
      'I',
      'O',
      'P',
      '{',
      '}',
      0,
      0,
      'A',
      'S', // 10-1f
      'D',
      'F',
      'G',
      'H',
      'J',
      'K',
      'L',
      ':',
      '"',
      '|',
      0,
      '>',
      'Z',
      'X',
      'C',
      'V', // 20-2f
      'B',
      'N',
      'M',
      '<',
      '>',
      '?',
      0,
      0,
      0,
      ' ',
      0,
      0,
      0,
      0,
      0,
      0, // 30-3f
      0,
      0,
      0,
      0,
      0,
      0,
      0,
    }};

int
GetKey(void)
{
  int sym, ctable;

  keyboard_update();

  if (!keys[last_ks])
    return 0;

  if (key_handled[last_ks])
    return 0;
  key_handled[last_ks] = 1;

  if (!keys[KEY_ALT] && !keys[KEY_CONTROL])
  {
    if (keys[KEY_LF_SHIFT] || keys[KEY_RT_SHIFT])
      ctable = 1;
    else
      ctable = 0;

    if (last_ks >= 1 && last_ks < 256 && tables[ctable][last_ks])
      return tables[ctable][last_ks] + KS_CHAR;
  }

  sym = last_ks;
  if (keys[KEY_ALT])
    sym |= KS_ALT;
  if (keys[KEY_CONTROL])
    sym |= KS_CTRL;
  if (keys[KEY_LF_SHIFT] || keys[KEY_RT_SHIFT])
    sym |= KS_SHIFT;

  return sym;
}

/* This is a bugfix for older versions of SVGALib. Problems are known
   with version <= 1.2.10. vga_waitevent in 1.2.11 seems to be ok. */
fd_set dummyset;

int
GetKeyB(void)
{
  int i;
  struct timeval timeout;

  while (1)
  {
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO(&dummyset);
    vga_waitevent(VGA_KEYEVENT, &dummyset, NULL, NULL, &timeout);

    i = GetKey();
    if (i)
      return i;
  }
}
