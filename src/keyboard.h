/*
keyboard.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef KEYBOARD_H
#define KEYBOARD_H


#define  NUM_UNIQUE_KEYS   256

/* Alphabet */
#define  KEY_A    30
#define  KEY_B    48
#define  KEY_C    46
#define  KEY_D    32
#define  KEY_E    18
#define  KEY_F    33
#define  KEY_G    34
#define  KEY_H    35
#define  KEY_I    23
#define  KEY_J    36
#define  KEY_K    37
#define  KEY_L    38
#define  KEY_M    50
#define  KEY_N    49
#define  KEY_O    24
#define  KEY_P    25
#define  KEY_Q    16
#define  KEY_R    19
#define  KEY_S    31
#define  KEY_T    20
#define  KEY_U    22
#define  KEY_V    47
#define  KEY_W    17
#define  KEY_X    45
#define  KEY_Y    21
#define  KEY_Z    44

/* Most Useful Keys */
#define  KEY_ESCAPE   1
#define  KEY_ENTER    28
#define  KEY_SPACE    57
#define  KEY_UP       72
#define  KEY_DOWN     80
#define  KEY_LEFT     75
#define  KEY_RIGHT    77

/*Useful Keys*/
#define  KEY_INSERT   82
#define  KEY_HOME     71
#define  KEY_PAGEUP   73
#define  KEY_DELETE   83
#define  KEY_END      79
#define  KEY_PAGEDOWN 81
#define  KEY_LF_SHIFT 42
#define  KEY_RT_SHIFT 54
#define  KEY_CAPS_LCK 58
#define  KEY_ALT      56
#define  KEY_CONTROL  29
#define  KEY_TAB      15
#define  KEY_BACKSPC  14

/* Useless keys */
#define  KEY_LF_BRACK 26
#define  KEY_RT_BRACK 27
#define  KEY_COLON    39
#define  KEY_QUOTE    40
#define  KEY_TILDE    41
#define  KEY_PIPE     43
#define  KEY_COMMA    51
#define  KEY_PERIOD   52
#define  KEY_Q_MARK   53

/* Even more useless keys */
#define  KEY_PRINT_SC 55
#define  KEY_SCR_LOCK 70
//#define  KEY_PAUSE    37  // this doesn't seem correct
#define  KEY_NUM_LOCK 69

/* Keypad and number keys */
#define  KEY_MINUS    12
#define  KEY_PLUS     13
#define  KEY_E_MINUS  74
#define  KEY_E_PLUS   78
#define  KEY_CENTER   76
#define  KEY_1    2
#define  KEY_2    3
#define  KEY_3    4
#define  KEY_4    5
#define  KEY_5    6
#define  KEY_6    7
#define  KEY_7    8
#define  KEY_8    9
#define  KEY_9    10
#define  KEY_0    11

/* Function Keys. */
#define  KEY_F1       59
#define  KEY_F2       60
#define  KEY_F3       61
#define  KEY_F4       62
#define  KEY_F5       63
#define  KEY_F6       64
#define  KEY_F7       65
#define  KEY_F8       66
#define  KEY_F9       67
#define  KEY_F10      68
#define  KEY_F11      87
#define  KEY_F12      88


extern int k_ascii[128];

void KBD_InstallHandler(void);
void KBD_RemoveHandler(void);


int TestKey(int key_to_test);
void ClearKeys(void);


#define KS_SHIFT 0x100
#define KS_CTRL  0x200
#define KS_ALT   0x400
#define KS_CHAR  0x800

/*
 GetKey() should return one of the KEY_xxx macros above|any KS_xxx
 currently held down, or an ASCII character|KS_CHAR if the currently
 pressed key generates a valid printable character.
*/
int GetKey(void);

/* Like GetKey() but block until there is a key. */
int GetKeyB(void);


#endif

