/*
keyboard.c file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#include <stdio.h>
#include <dpmi.h>
#include <conio.h>
#include <go32.h>
#include <pc.h>
#include <sys/exceptn.h>

#include "defines.h"
#include "types.h"

#include "keyboard.h"

#include "status.h"
#include "times.h"


int k_ascii[128]=
{
['A'] KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,KEY_H,KEY_I,KEY_J,KEY_K,
      KEY_L,KEY_M,KEY_N,KEY_O,KEY_P,KEY_Q,KEY_R,KEY_S,KEY_T,KEY_U,KEY_V,
      KEY_W,KEY_X,KEY_Y,KEY_Z,

['a'] KEY_A,KEY_B,KEY_C,KEY_D,KEY_E,KEY_F,KEY_G,KEY_H,KEY_I,KEY_J,KEY_K,
      KEY_L,KEY_M,KEY_N,KEY_O,KEY_P,KEY_Q,KEY_R,KEY_S,KEY_T,KEY_U,KEY_V,
      KEY_W,KEY_X,KEY_Y,KEY_Z,

['0'] KEY_0,KEY_1,KEY_2,KEY_3,KEY_4,KEY_5,KEY_6,KEY_7,KEY_8,KEY_9,
};


static volatile char keys[NUM_UNIQUE_KEYS];

static volatile char keys_handled[NUM_UNIQUE_KEYS];
static volatile int last_key;

static volatile int e0;

static void my_kbd_hand(void)
{
   int key,keycode;

   key=inportb(0x60);
   if (key==0xe0)
   {
      e0=1;
      return;
   }
   keycode=key & 127;
/*   if (e0)
   {
      keycode+=0x80; // ignoring for now, would only mess up key definitions
   }*/
   if (!e0 || (keycode!=42)) // ignore l-shift on extended keys
   {
      if (key & 128)
      {
         keys[keycode]=0;
         keys_handled[keycode]=0;
      }
      else
      {
         keys[keycode]=1;
         last_key=keycode;
      }
   }
   e0=0;
}

static _go32_dpmi_seginfo old_handler, new_handler;

void KBD_InstallHandler(void)
{
   __dpmi_meminfo Info;

   __djgpp_set_ctrl_c(0);
	status.kbd_hand = TRUE;
   memset((void *)keys,0,sizeof(keys));
   memset((void *)keys_handled,0,sizeof(keys_handled));
   e0=0;
   last_key=0;

	ClearKeys();
	fflush(stdin);

   _go32_dpmi_get_protected_mode_interrupt_vector(9,&old_handler);

   Info.address=(int)my_kbd_hand;
   Info.size=(unsigned int)(KBD_InstallHandler - my_kbd_hand);
   __dpmi_lock_linear_region(&Info);

   Info.address=(int)&keys;
   Info.size=sizeof(keys);
   __dpmi_lock_linear_region(&Info);

   Info.address=(int)&e0;
   Info.size=sizeof(e0);
   __dpmi_lock_linear_region(&Info);

   new_handler.pm_offset = (int)my_kbd_hand;
   new_handler.pm_selector = _go32_my_cs();
   _go32_dpmi_chain_protected_mode_interrupt_vector(9, &new_handler);
}

void KBD_RemoveHandler(void)
{
   _go32_dpmi_set_protected_mode_interrupt_vector(9, &old_handler);
	status.kbd_hand = FALSE;
	ClearKeys();
}

int TestKey(int key_to_test)
{
	int r;

	r=keys[key_to_test];
   if (keys_handled[key_to_test])
      r=0;
	ClearKeys();
	return r;
}

void ClearKeys(void)
{
	while (kbhit())
	{
		if (!getch()) getch();
	}
}



#if 0
static int key2ascii[128]=
{
0,27,
'1','2','3','4','5','6','7','8','9','0',0,0,'\b',
'\t','q','w','e','r','t','y','u','i','o','p',0,0,'\r',
0,'a','s','d','f','g','h','j','k','l',0,0,0,0,
0,'z','x','c','v','b','n','m',',','.',
// TODO: finish
};
#endif


int GetKey(void)
{
   int shift;
static float next_time=0;
static int time_key=0;

   shift=0;
   if (keys[KEY_CONTROL])
      shift|=KS_CTRL;
   if (keys[KEY_ALT])
      shift|=KS_ALT;
   if (keys[KEY_LF_SHIFT] || keys[KEY_RT_SHIFT])
      shift|=KS_SHIFT;

   if (kbhit())
   {
      int i=getch();

//      printf("BIOS got key %i %c (%i)\n",i,i,last_key);

      switch (i)
      {
      case 32 ... 255:
//         printf("getch() %i '%c' last_key=%i\n",i,i,last_key);
//         printf("%i handled to %i\n",last_key,i);
         keys_handled[last_key]=2; // mark key as handled (assume it's the right one)
         return i+KS_CHAR;
      }
      if (!i)
      {
         getch();
      }
      if (keys_handled[last_key]==2) keys_handled[last_key]=0;
   }

   if (!keys[last_key])
   {
      time_key=0;
      return 0;
   }

//   printf("last_key=%i %02x\n",last_key,shift);

   if (keys_handled[last_key]==2)
   {
//      printf("BIOS handled\n");
      return 0; // it's been handled by the BIOS key code
   }

   switch (last_key)
   {
   case KEY_LF_SHIFT:
   case KEY_RT_SHIFT:
   case KEY_CONTROL:
   case KEY_ALT:
   case KEY_CAPS_LCK:
   case KEY_SCR_LOCK:
   case KEY_NUM_LOCK:
      return 0;
   }

//   printf("key %i, time_key=%i, handled=%i next_time=%g GetTime()=%g\n",
//      last_key,time_key,keys_handled[last_key],next_time,GetTime());

   if (last_key!=time_key && !keys_handled[last_key])
   {
      time_key=last_key;
      keys_handled[last_key]=1;
      next_time=GetTime()+0.5;
      return last_key+shift;
   }

   if (GetTime()<next_time)
      return 0;
   next_time=GetTime()+0.1;
   return last_key+shift;
}

int GetKeyB(void)
{
   int i;
   do
   {
      i=GetKey();
   } while (!i);
   return i;
}

