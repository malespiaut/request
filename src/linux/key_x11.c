//KEYBOARD.C module of Quest v1.1 Source Code
//Info on the Quest source code can be found in qsrcinfo.txt

//Copyright 1996, Trey Harrison and Chris Carollo
//For non-commercial use only. More rules apply, read legal.txt
//for a full description of usage restrictions.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#include "defines.h"
#include "types.h"

#include "keyboard.h"
#include "hw_x11.h"

#include "mouse.h"
#include "memory.h"
#include "times.h"
#include "video.h"


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


static struct q_xconv {
  KeySym Xsym;
  u_char dosCode;
} xconvtable[] = {
  { XK_A, KEY_A }, { XK_a, KEY_A },
  { XK_B, KEY_B }, { XK_b, KEY_B },
  { XK_C, KEY_C }, { XK_c, KEY_C },
  { XK_D, KEY_D }, { XK_d, KEY_D },
  { XK_E, KEY_E }, { XK_e, KEY_E },
  { XK_F, KEY_F }, { XK_f, KEY_F },
  { XK_G, KEY_G }, { XK_g, KEY_G },
  { XK_H, KEY_H }, { XK_h, KEY_H },
  { XK_I, KEY_I }, { XK_i, KEY_I },
  { XK_J, KEY_J }, { XK_j, KEY_J },
  { XK_K, KEY_K }, { XK_k, KEY_K },
  { XK_L, KEY_L }, { XK_l, KEY_L },
  { XK_M, KEY_M }, { XK_m, KEY_M },
  { XK_N, KEY_N }, { XK_n, KEY_N },
  { XK_O, KEY_O }, { XK_o, KEY_O },
  { XK_P, KEY_P }, { XK_p, KEY_P },
  { XK_Q, KEY_Q }, { XK_q, KEY_Q },
  { XK_R, KEY_R }, { XK_r, KEY_R },
  { XK_S, KEY_S }, { XK_s, KEY_S },
  { XK_T, KEY_T }, { XK_t, KEY_T },
  { XK_U, KEY_U }, { XK_u, KEY_U },
  { XK_V, KEY_V }, { XK_v, KEY_V },
  { XK_W, KEY_W }, { XK_w, KEY_W },
  { XK_X, KEY_X }, { XK_x, KEY_X },
  { XK_Y, KEY_Y }, { XK_y, KEY_Y },
  { XK_Z, KEY_Z }, { XK_z, KEY_Z },

  { XK_BackSpace, KEY_BACKSPC  }, 
  { XK_Caps_Lock, KEY_CAPS_LCK },
  { XK_Escape,    KEY_ESCAPE   },
  { XK_F1,        KEY_F1       },
  { XK_F10,       KEY_F10      },
  { XK_F2,        KEY_F2       },
  { XK_F3,        KEY_F3       },
  { XK_F4,        KEY_F4       },
  { XK_F5,        KEY_F5       },
  { XK_F6,        KEY_F6       },
  { XK_F7,        KEY_F7       },
  { XK_F8,        KEY_F8       },
  { XK_F9,        KEY_F9       },

  { XK_0,         KEY_0        }, { XK_KP_0,         KEY_0        },
  { XK_1,         KEY_1        }, { XK_KP_1,         KEY_1        },
  { XK_2,         KEY_2        }, { XK_KP_2,         KEY_2        },
  { XK_3,         KEY_3        }, { XK_KP_3,         KEY_3        },
  { XK_4,         KEY_4        }, { XK_KP_4,         KEY_4        },
  { XK_5,         KEY_5        }, { XK_KP_5,         KEY_5        },
  { XK_6,         KEY_6        }, { XK_KP_6,         KEY_6        },
  { XK_7,         KEY_7        }, { XK_KP_7,         KEY_7        },
  { XK_8,         KEY_8        }, { XK_KP_8,         KEY_8        },
  { XK_9,         KEY_9        }, { XK_KP_9,         KEY_9        },
  { XK_Delete,    KEY_DELETE   }, { XK_KP_Delete,    KEY_DELETE   },
  { XK_Down,      KEY_DOWN     }, { XK_KP_Down,      KEY_DOWN     },
  { XK_End,       KEY_END      }, { XK_KP_End,       KEY_END      },
  { XK_Home,      KEY_HOME     }, { XK_KP_Home,      KEY_HOME     },
  { XK_Insert,    KEY_INSERT   }, { XK_KP_Insert,    KEY_INSERT   },
  { XK_Left,      KEY_LEFT     }, { XK_KP_Left,      KEY_LEFT     },
  { XK_Page_Down, KEY_PAGEDOWN }, { XK_KP_Page_Down, KEY_PAGEDOWN },
  { XK_Page_Up,   KEY_PAGEUP   }, { XK_KP_Page_Up,   KEY_PAGEUP   },
  { XK_Return,    KEY_ENTER    }, { XK_KP_Enter,     KEY_ENTER    },
  { XK_Right,     KEY_RIGHT    }, { XK_KP_Right,     KEY_RIGHT    },
  { XK_Tab,       KEY_TAB      }, { XK_KP_Tab,       KEY_TAB      },
  { XK_Up,        KEY_UP       }, { XK_KP_Up,        KEY_UP       },
  { XK_minus,     KEY_MINUS    }, { XK_KP_Subtract,  KEY_E_MINUS  },
  { XK_plus,      KEY_PLUS     }, { XK_KP_Add,       KEY_E_PLUS   },
  { XK_space,     KEY_SPACE    }, { XK_KP_Space,     KEY_SPACE    },

  { XK_Num_Lock,     KEY_NUM_LOCK },
  { XK_Scroll_Lock,  KEY_SCR_LOCK },
  { XK_Sys_Req,      KEY_PRINT_SC }, 
  { XK_asciitilde,   KEY_TILDE    },
  { XK_bar,          KEY_PIPE     },
  { XK_bracketleft,  KEY_LF_BRACK }, 
  { XK_bracketright, KEY_RT_BRACK }, 
  { XK_colon,        KEY_COLON    },
  { XK_comma,        KEY_COMMA    },
  { XK_period,       KEY_PERIOD   },
  { XK_quotedbl,     KEY_Q_MARK   },
  { XK_quoteright,   KEY_QUOTE    },

  { XK_Alt_L,     KEY_ALT      }, { XK_Alt_R,     KEY_ALT      },
  { XK_Control_L, KEY_CONTROL  }, { XK_Control_R, KEY_CONTROL  },
  { XK_Shift_L,   KEY_LF_SHIFT }, { XK_Shift_R,   KEY_RT_SHIFT },

  { NoSymbol, 0 }
};

static int dosCode(KeySym keysym)
{
  struct q_xconv *p;

  if (keysym == NoSymbol) return 0;

  for (p = xconvtable; p->Xsym != NoSymbol; ++p)
    if (p->Xsym == keysym) return p->dosCode;

  return 0;
}


int keys[NUM_UNIQUE_KEYS];
static int key_handled[NUM_UNIQUE_KEYS];
static KeySym last_ks;
static char ks_buf[32];


static float xk_time_of_last_event=-1;

void X_HandleEvents(void)
{
   XEvent xe;
   int num;

   num=0;
   while (XCheckMaskEvent(xdisplay,-1,&xe))
   {
      num=1;
      switch (xe.type)
      {
      case KeyPress:
      case KeyRelease:
         {
            KeySym ks;

            ks=XLookupKeysym(&xe.xkey,0);
            keys[dosCode(ks)]=(xe.type==KeyPress);
            
            if (xe.type==KeyPress)
            {
               XLookupString(&xe.xkey,ks_buf,sizeof(ks_buf),&last_ks,NULL);
               last_ks=ks;
            }
            else
               key_handled[dosCode(ks)]=0;
         }
         break;

      case ButtonPress:
      case ButtonRelease:
         {
            int i;
            if (xe.xbutton.button==Button1)
               i=1;
            else
            if (xe.xbutton.button==Button3)
               i=2;
            else
               i=4;
   
            if (xe.type==ButtonPress)
               mouse.button|=i;
            else
               mouse.button&=~i;
         }
         mouse.x=xe.xbutton.x;
         mouse.y=xe.xbutton.y;
         break;
      case MotionNotify:
         mouse.x=xe.xmotion.x;
         mouse.y=xe.xmotion.y;
         break;
       
      case Expose:
         RefreshScreen();
         break;
      }
   }
   if (!num && (xk_time_of_last_event!=-1))
   {
      if (GetTime()-xk_time_of_last_event>1)
      {
         ClearKeys();
         xk_time_of_last_event=-1;
      }
   }
   else
      xk_time_of_last_event=GetTime();
}


void KBD_InstallHandler(void)
{
   ClearKeys();
}

void KBD_RemoveHandler()
{
   ClearKeys();
}


int TestKey(int key_to_test)
{
   X_HandleEvents();
   return keys[key_to_test];
}

void ClearKeys(void)
{
   int k;
   for (k = 0; k < NUM_UNIQUE_KEYS; k++)
      key_handled[k]=keys[k] = 0;
}


int GetKey(void)
{
   int sym;

   X_HandleEvents();
    
   if (!keys[dosCode(last_ks)])
      return 0;
   
   if (key_handled[dosCode(last_ks)])
      return 0;
   key_handled[dosCode(last_ks)]=1;
    
   if (last_ks>=0x20 && last_ks<256 && !ks_buf[1])
      sym=ks_buf[0]+KS_CHAR;
   else
   {
      sym=dosCode(last_ks);
      if (keys[KEY_ALT]) sym|=KS_ALT;
      if (keys[KEY_CONTROL]) sym|=KS_CTRL;
      if (keys[KEY_LF_SHIFT] || keys[KEY_RT_SHIFT]) sym|=KS_SHIFT;
   }
   
   return sym;
}

int GetKeyB(void)
{
   int i;
   XEvent xe;

   do
   {
      XNextEvent(xdisplay,&xe);
      XPutBackEvent(xdisplay,&xe);
      i=GetKey();
   } while (!i);
   return i;
}

