/*
entclass.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef ENTCLASS_H
#define ENTCLASS_H

typedef struct
{
  char* name;
  char* val;
} key_choice_t;

typedef struct
{
  char* name;
  int type;

  union
  {
    struct
    {
      key_choice_t* choices;
      int num_choices;
    } ch;
    struct
    {
      char* name;
    } list;
  } x;
} Q_key_t;

#define KEY_CHOICE 0
#define KEY_LIST 1
#define KEY_TEXTURE 2

#define KEY_Q2_COLOR 3

#define KEY_HL_COLOR 4
#define KEY_HL_LIGHT 5

typedef struct
{
  char *key, *value;
} Q_default_t;

typedef struct
{
  const char* name;
  int type;

  float col[3];
  float bound[2][3];

  const char** flags;
  int num_flags;

  char* comment;

  Q_key_t* keys;
  int num_keys;

  Q_default_t* defs;
  int num_defs;

  int color;
  struct mdl_s* mdl;
} class_t;

#define CLASS_MODEL 1
#define CLASS_POINT 2
#define CLASS_BASE 4

extern class_t** classes;
extern int num_classes;

void InitEntClasses(void);
class_t* FindClass(const char* classname);

int DrawClassInfo(int bx, int by, int sizex, int sizey, const char* name, int s_y);

void FixColors(void);
int GetCol(float rf, float gf, float bf, int update);

void SetEntityDefaults(entity_t* e);

#endif
