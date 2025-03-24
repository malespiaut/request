/*
entclass.c file of the Quest Source Code

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

#include "entclass.h"

#include "button.h"
#include "color.h"
#include "entity.h"
#include "error.h"
#include "filedir.h"
#include "game.h"
#include "keyboard.h"
#include "mdl.h"
#include "memory.h"
#include "mouse.h"
#include "qui.h"
#include "status.h"
#include "token.h"
#include "video.h"

class_t** classes;
int num_classes;

static class_t** base_classes;
static int num_base_classes;

static FILE* f;
static char l[256];

static void
FixStr(char* str)
{
  if ((str[0] == '"') && (str[strlen(str) - 1] == '"'))
  {
    strcpy(str, &str[1]);
    str[strlen(str) - 1] = 0;
  }
}

static void
AddKeyChoice(Q_key_t* key, key_choice_t* c)
{
  key->x.ch.choices =
    Q_realloc(key->x.ch.choices,
              (key->x.ch.num_choices + 1) * sizeof(key_choice_t));
  if (!key->x.ch.choices)
    Abort("ParseSpecial", "Out of memory!");
  key->x.ch.choices[key->x.ch.num_choices++] = *c;
}

static void
AddKey(class_t* c, Q_key_t* key)
{
  int i;

  for (i = 0; i < c->num_keys; i++)
    if (!strcmp(c->keys[i].name, key->name))
    {
      c->keys[i] = *key;
      return;
    }

  c->keys = Q_realloc(c->keys, (c->num_keys + 1) * sizeof(Q_key_t));
  if (!c->keys)
    Abort("ParseSpecial", "Out of memory!");
  c->keys[c->num_keys++] = *key;
}

static char* extra_comment;

static void
AddBase(class_t* c, char* basename)
{
  int i;
  class_t* b;

  for (i = 0; i < num_base_classes; i++)
  {
    if (!strcmp(base_classes[i]->name, basename))
      break;
  }
  if (i == num_base_classes)
  {
    for (i = 0; i < num_classes; i++)
    {
      if (!strcmp(classes[i]->name, basename))
        break;
    }

    if (i == num_classes)
      Abort("ParseSpecial", "Unknown base class '%s' in class '%s'!", basename, c->name);

    b = classes[i];
  }
  else
  {
    b = base_classes[i];
  }
  for (i = 0; i < b->num_keys; i++)
    AddKey(c, &b->keys[i]);

  if (b->comment)
  {
    if (extra_comment)
    {
      extra_comment =
        Q_realloc(extra_comment,
                  strlen(extra_comment) + strlen(b->comment) + 1);
      if (!extra_comment)
        Abort("InitEntClasses", "Out of memory!");
      strcat(extra_comment, b->comment);
    }
    else
    {
      extra_comment = Q_strdup(b->comment);
      if (!extra_comment)
        Abort("InitEntClasses", "Out of memory!");
    }
  }
}

#define GETTOKEN()      \
  if (!TokenGet(1, -1)) \
  Abort("ParseSpecial", "Parse error in class '%s'!", c->name)

#define EXPECT(x)       \
  GETTOKEN();           \
  if (strcmp(token, x)) \
  Abort("ParseSpecial", "Parse error in class '%s' (expected '%s')!", c->name, x)

static void
GetStr(char** str, class_t* c)
{
  GETTOKEN();
  FixStr(token);
  *str = Q_strdup(token);
  if (!*str)
    Abort("ParseSpecial", "Out of memory!");
}

static void
ParseSpecial(class_t* c)
{
  Q_key_t temp;
  key_choice_t temp_c;

  TokenBuf(c->comment, T_C | T_NAME | T_MISC | T_STRING, NULL);

  EXPECT("{");
  TokenGet(1, -1);
  while (strcmp(token, "}"))
  {
    memset(&temp, 0, sizeof(Q_key_t));

    // key types
    if (!strcmp(token, "choice"))
    {
      temp.type = KEY_CHOICE;
      GetStr(&temp.name, c);
      EXPECT("(");
      GETTOKEN();
      while (strcmp(token, ")"))
      {
        if (strcmp(token, "("))
          Abort("ParseSpecial", "Parse error in class '%s'!", c->name);

        GetStr(&temp_c.val, c);
        EXPECT(",");
        GetStr(&temp_c.name, c);
        AddKeyChoice(&temp, &temp_c);
        EXPECT(")");
        GETTOKEN();
      }
      EXPECT(";");
      AddKey(c, &temp);
    }
    else if (!strcmp(token, "list"))
    {
      temp.type = KEY_LIST;
      GetStr(&temp.name, c);
      EXPECT("(");
      GetStr(&temp.x.list.name, c);
      EXPECT(")");
      EXPECT(";");
      AddKey(c, &temp);
    }
    else if (!strcmp(token, "texture"))
    {
      temp.type = KEY_TEXTURE;
      GetStr(&temp.name, c);
      AddKey(c, &temp);
      EXPECT(";");
    }
    // Default values
    else if (!strcmp(token, "default"))
    {
      c->defs = Q_realloc(c->defs, sizeof(Q_default_t) * (c->num_defs + 1));

      EXPECT("(");
      GetStr(&c->defs[c->num_defs].key, c);
      EXPECT(",");
      GetStr(&c->defs[c->num_defs++].value, c);
      EXPECT(")");
      EXPECT(";");
    }

    // Game specific stuff
    else if (!strcmp(token, "q2_color"))
    {
      temp.type = KEY_Q2_COLOR;
      GetStr(&temp.name, c);
      AddKey(c, &temp);
      EXPECT(";");
    }

    else if (!strcmp(token, "hl_color"))
    {
      temp.type = KEY_HL_COLOR;
      GetStr(&temp.name, c);
      AddKey(c, &temp);
      EXPECT(";");
    }
    else if (!strcmp(token, "hl_light"))
    {
      temp.type = KEY_HL_LIGHT;
      GetStr(&temp.name, c);
      AddKey(c, &temp);
      EXPECT(";");
    }

    else // commands
      if (!strcmp(token, "base"))
      {
        EXPECT("(");
        GETTOKEN();
        FixStr(token);
        AddBase(c, token);
        EXPECT(")");
        EXPECT(";");
      }
      else if (!strcmp(token, "model"))
      {
        EXPECT("(");
        GETTOKEN();
        FixStr(token);
        if (status.load_models)
          c->mdl = LoadModel(token);
        EXPECT(")");
        EXPECT(";");
      }
      else
      {
        Abort("ParseSpecial", "Unknown command '%s' in class '%s'!", token, c->name);
      }
    TokenGet(1, -1);
  }
  Q_free(c->comment);

  {
    char* d;

    for (d = token_curpos; *d && *d <= 32; d++)
    {
    }
    if (*d)
    {
      c->comment = Q_strdup(d);
      if (!c->comment)
        Abort("ParseSpecial", "Out of memory!");
    }
    else
      c->comment = NULL;
  }

  TokenDone();
#undef GETTOKEN()
#undef EXPECT()

  /*   {
        int i,j;
        Q_key_t *k;
        key_choice_t *ch;

        printf("-- '%s'\n",c->name);
        for (i=0;i<c->num_keys;i++)
        {
           k=&c->keys[i];
           printf("  %i '%s'\n",k->type,k->name);
           switch (k->type)
           {
           case KEY_CHOICE:
              for (j=0,ch=k->x.ch.choices;j<k->x.ch.num_choices;j++,ch++)
                 printf("    '%s' '%s'\n",ch->val,ch->name);
              break;
           }
        }
     }*/
}

static void
ParseClass(void)
{
  class_t* temp;
  char* tok;
  char tempstr[256];
#define PGetToken() tok = strtok(0, " \n()\t")

  tok = strtok(l, " "); // /*QUAKED

  PGetToken(); // item_shells
  if (FindClass(tok))
    return;

  temp = Q_malloc(sizeof(class_t));
  if (!temp)
  {
    Abort("InitEntClasses", "Out of memory!");
  }
  memset(temp, 0, sizeof(class_t));

  temp->name = Q_strdup(tok);

  PGetToken(); // (0 .5 .8)
  if (tok)
  {
    temp->col[0] = atof(tok);
    PGetToken();
    temp->col[1] = atof(tok);
    PGetToken();
    temp->col[2] = atof(tok);

    PGetToken(); // (0 0 0)
    if (!strcmp(tok, "?"))
    {
      temp->type = CLASS_MODEL;
    }
    else
    {
      temp->type = CLASS_POINT;

      temp->bound[0][0] = atof(tok);
      PGetToken();
      temp->bound[0][1] = atof(tok);
      PGetToken();
      temp->bound[0][2] = atof(tok);

      PGetToken(); // (32 32 32)
      temp->bound[1][0] = atof(tok);
      PGetToken();
      temp->bound[1][1] = atof(tok);
      PGetToken();
      temp->bound[1][2] = atof(tok);
    }

    temp->num_flags = 0;
    temp->flags = NULL;
    for (PGetToken(); tok; PGetToken()) // big
    {
      if (*tok == '"')
      {
        strcpy(tempstr, &tok[1]);
        while (tok[strlen(tok) - 1] != '"')
        {
          PGetToken();
          strcat(tempstr, " ");
          strcat(tempstr, tok);
        }
        tempstr[strlen(tempstr) - 1] = 0;
      }
      else
      {
        strcpy(tempstr, tok);
      }

      temp->num_flags++;
      temp->flags = Q_realloc(temp->flags, temp->num_flags * sizeof(char*));
      if (!temp->flags)
      {
        Abort("InitEntClasses", "Out of memory!");
      }
      if (strcmp(tempstr, "x"))
        temp->flags[temp->num_flags - 1] = Q_strdup(tempstr);
      else // handle "x" specially, as it occurs frequently
        temp->flags[temp->num_flags - 1] = "x";
    }
    if (Game.entity.qdiff)
    {
      temp->flags = Q_realloc(temp->flags, sizeof(char*) * 12);
      while (temp->num_flags < 12)
      {
        if (temp->num_flags < 8)
          temp->flags[temp->num_flags] = "x";
        else if (temp->num_flags == 8)
          temp->flags[temp->num_flags] = "!Easy";
        else if (temp->num_flags == 9)
          temp->flags[temp->num_flags] = "!Medium";
        else if (temp->num_flags == 10)
          temp->flags[temp->num_flags] = "!Hard";
        else if (temp->num_flags == 11)
          temp->flags[temp->num_flags] = "!Deathmatch";
        temp->num_flags++;
      }
    }
  }
  else
    temp->type = CLASS_BASE;

  temp->comment = NULL;
  fgets(l, 256, f);
  while (l[0] && l[strlen(l) - 1] <= 32)
    l[strlen(l) - 1] = 0;
  while (strncmp(&l[(strlen(l) > 2) ? strlen(l) - 2 : 0], "*/", 2))
  //   while (/*printf("%s\n",&l[((strlen(l)>3)?strlen(l):3)-3]),*/strncmp(&l[((strlen(l)>3)?strlen(l):3)-3],"*/",2))
  {
// Check for new, special fields
#define F_MDL ":model=" // specifies which mdl/md2 file this classname uses
    if (!strncmp(l, F_MDL, strlen(F_MDL)))
    {
      if (status.load_models)
      {
        strcpy(l, &l[strlen(F_MDL)]);
        while (l[strlen(l) - 1] <= 32)
          l[strlen(l) - 1] = 0;
        temp->mdl = LoadModel(l);
      }
      goto done;
    }

    strcat(l, "\n"); // add newline again
    if (temp->comment)
    {
      temp->comment = Q_realloc(temp->comment, strlen(temp->comment) + strlen(l) + 1);
      if (!temp->comment)
        Abort("InitEntClasses", "Out of memory!");
      strcat(temp->comment, l);
    }
    else
    {
      temp->comment = Q_strdup(l);
      if (!temp->comment)
        Abort("InitEntClasses", "Out of memory!");
    }
done:
    if (!fgets(l, 256, f))
      Abort("InitEntClasses", "Unexpected end of file!");
    while (l[0] && l[strlen(l) - 1] <= 32)
      l[strlen(l) - 1] = 0;
  }

  if (temp->comment)
  {
    extra_comment = NULL;
    if (temp->comment[0] == '{')
    {
      ParseSpecial(temp);
    }
    if (extra_comment)
    {
      if (!temp->comment)
        temp->comment = Q_strdup("");
      temp->comment = Q_realloc(temp->comment, strlen(extra_comment) + strlen(temp->comment) + 1);
      if (!temp->comment)
        Abort("InitEntClasses", "Out of memory!");
      strcat(temp->comment, extra_comment);
      Q_free(extra_comment);
    }
  }

  if (temp->type == CLASS_BASE)
  {
    num_base_classes++;
    base_classes = Q_realloc(base_classes, num_base_classes * sizeof(class_t*));
    if (!base_classes)
      Abort("InitEntClasses", "Out of memory!");
    base_classes[num_base_classes - 1] = temp;
  }
  else
  {
    num_classes++;
    classes = Q_realloc(classes, num_classes * sizeof(class_t*));
    if (!classes)
      Abort("InitEntClasses", "Out of memory!");
    classes[num_classes - 1] = temp;
  }
#undef PGetToken
}

static void
ParseFile(char* name)
{
  f = fopen(name, "rt");
  if (!f)
    return;
  while (!feof(f))
  {
    if (!fgets(l, 256, f))
      break;
    while (l[0] && l[strlen(l) - 1] <= 32)
      l[strlen(l) - 1] = 0;

    if (!strncmp(l, "/*QUAKED", 8))
    {
      ParseClass();
    }
  }
  fclose(f);
}

static int
ClassSort(const void* e1, const void* e2)
{
  const class_t *c1, *c2;

  c1 = *(class_t* const*)e1;
  c2 = *(class_t* const*)e2;
  return strcmp(c1->name, c2->name);
}

void
InitEntClasses(void)
{
  struct directory_s* d;
  filedir_t f;
  class_t* c;

  char file[1024];
  char filebase[1024];

  num_classes = 1;
  classes = Q_malloc(sizeof(class_t*));
  if (!classes)
  {
    Abort("InitEntClasses", "Out of memory!");
  }

  c = classes[0] = Q_malloc(sizeof(class_t));
  if (!(classes[0]))
  {
    Abort("InitEntClasses", "Out of memory!");
  }

  memset(c, 0, sizeof(class_t));
  c->name = "???";
  c->col[0] = c->col[1] = c->col[2] = 1;
  c->type = CLASS_POINT;
  c->bound[0][0] = c->bound[0][1] = c->bound[0][2] = -8;
  c->bound[1][0] = c->bound[1][1] = c->bound[1][2] = 8;

  strcpy(filebase, status.class_dir);

  if (strrchr(filebase, '/'))
  {
    *(strrchr(filebase, '/') + 1) = 0;
  }
  else
  {
    filebase[0] = 0;
  }

  if (status.load_models)
    InitMDLPak();

  d = DirOpen(status.class_dir, FILE_NORMAL);
  if (!d)
    Abort("InitEntClasses", "Invalid class_dir in quest.cfg!");
  while (DirRead(d, &f))
  {
    strcpy(file, filebase);
    strcat(file, f.name);

    ParseFile(file);
  }
  DirClose(d);

  if (status.load_models)
    DoneMDLPak();

  qsort(classes, num_classes, sizeof(class_t*), ClassSort);
}

class_t*
FindClass(const char* classname)
{
  int i;

  if (!classname)
    return NULL;
  for (i = 0; i < num_classes; i++)
  {
    if (!strcmp(classname, classes[i]->name))
    {
      return classes[i];
    }
  }
  return NULL;
}

#if (0)
void
DrawClassInfo(int bx, int by, int sizex, int sizey, char* name)
{
  class_t* c;
  int col;

  char cbuf[128];
  char* cb;
  char* d;

  int x, y;
  int cx;
  int sx;
  int maxy;

  int i;

  if (!name)
  {
    HandleError("DrawClassInfo", "No classname!");
    return;
  }
  c = FindClass(name);
  if (!c)
  {
    QUI_DrawStr(bx, by + 0, BG_COLOR, 15, 0, 0, "%s", name);
    QUI_DrawStr(bx, by + 16, BG_COLOR, 15, 0, 0, "Unknown classname!");

    return;
  }
  col = AddColor(c->col[0], c->col[1], c->col[2], 1);

  QUI_DrawStr(bx, by + 0, BG_COLOR, col, 0, 0, "%s", name);

  if (c->size)
  {
    QUI_DrawStr(bx, by + 16, BG_COLOR, 0, 0, 0, "Size: (%0.0f %0.0f %0.0f)-(%0.0f %0.0f %0.0f)", c->bound[0][0], c->bound[0][1], c->bound[0][2], c->bound[1][0], c->bound[1][1], c->bound[1][2]);
  }
  else
  {
    QUI_DrawStr(bx, by + 16, BG_COLOR, 0, 0, 0, "Size: ? (brush model)");
  }

  QUI_DrawStr(bx, by + 32, BG_COLOR, 15, 0, 0, "Flags:");
  for (i = 0; i < c->num_flags; i++)
    QUI_DrawStr(bx + 70, by + 32 + i * 16, BG_COLOR, 0, 0, 0, "%3i %s", 1 << i, c->flags[i]);

  if (c->comment)
  {
    QUI_DrawStr(bx, by + 32 + i * 16, BG_COLOR, 15, 0, 0, "Comment:");

    d = c->comment;
    x = 0;
    y = 0;
    cb = cbuf;
    cx = 0;
    sx = 0;
    maxy = sizey - 10 - 48 - i * 16;

    while (*d)
    {
      if ((*d == ' ') || (*d == 9) || (*d == '\n'))
      {
        *cb = 0;
        QUI_DrawStr(x + bx, y + by + 48 + i * 16, BG_COLOR, 0, 0, 0, cbuf);
        cb = cbuf;
        x += cx;
        cx = 0;
      }
      switch (*d)
      {
        case ' ':
          x += 8;
          break;
        case 9:
          x = (x / 64 + 1) * 64;
          sx = x;
          break;
        case '\n':
          y += ROM_CHAR_HEIGHT;
          x = 0;
          sx = 0;
          break;
        default:
          *cb++ = *d;
          cx += Q.font[0].width[Q.font[0].map[(unsigned char)*d]];
          break;
      }
      if (x + cx > sizex - 10)
      {
        x = sx;
        y += ROM_CHAR_HEIGHT;
      }
      if (y > maxy - ROM_CHAR_HEIGHT)
      {
        break;
      }
      d++;
    }
    if (*d)
    {
      QUI_DrawStr(x + bx, y + by + 48 + i * 16, BG_COLOR, 15, 0, 0, "More...");
    }
    else
    {
      *cb = 0;
      QUI_DrawStr(x + bx, y + by + 48 + i * 16, BG_COLOR, 0, 0, 0, cbuf);
    }
  }
}
#endif

int
DrawClassInfo(int bx, int by, int sizex, int sizey, const char* name, int s_y)
{
  class_t* c;

  char cbuf[128];
  char* cb;
  char* d;

  int x, y;
  int cx;
  int sx;
  int maxy;

  if (!name)
  {
    HandleError("DrawClassInfo", "No classname!");
    return 0;
  }
  c = FindClass(name);
  if (!c)
  {
    return 0;
  }
  if (!c->comment)
    return 0;

  d = c->comment;
  x = 0;
  y = 0;
  cb = cbuf;
  cx = 0;
  sx = 0;
  maxy = sizey - 4;

  while (*d)
  {
    if ((*d == ' ') || (*d == 9) || (*d == '\n'))
    {
      *cb = 0;
      if (!s_y)
        QUI_DrawStr(x + bx, y + by, BG_COLOR, 0, 0, 0, cbuf);
      cb = cbuf;
      x += cx;
      cx = 0;
    }
    switch (*d)
    {
      case ' ':
        x += 8;
        break;
      case 9:
        x = (x / 64 + 1) * 64;
        sx = x;
        break;
      case '\n':
        if (s_y)
        {
          s_y--;
        }
        else
        {
          y += ROM_CHAR_HEIGHT;
        }
        x = 0;
        sx = 0;
        break;
      default:
        *cb++ = *d;
        cx += Q.font[0].width[Q.font[0].map[(unsigned char)*d]];
        break;
    }
    if (x + cx > sizex - 10)
    {
      x = sx;
      if (s_y)
      {
        s_y--;
      }
      else
      {
        y += ROM_CHAR_HEIGHT;
      }
    }
    if (y > maxy)
    {
      break;
    }
    d++;
  }
  if (!(*d))
  {
    *cb = 0;
    QUI_DrawStr(x + bx, y + by, BG_COLOR, 0, 0, 0, cbuf);
    return 0;
  }
  return 1;
}

#if (0)
void
ClassInfo(char* classname)
{
  QUI_window_t* w;
  unsigned char* TempBuf;

  int b_ok;
  int b;

  w = &Q.window[POP_WINDOW_1 + Q.num_popups];

  w->size.x = video.ScreenWidth - 40;
  w->pos.x = (video.ScreenWidth / 2) - (w->size.x / 2);
  w->size.y = video.ScreenHeight - 20;
  w->pos.y = (video.ScreenHeight / 2) - (w->size.y / 2);

  QUI_PopUpWindow(POP_WINDOW_1 + Q.num_popups, "Class info", &TempBuf);
  Q.num_popups++;

  //   DrawClassInfo(w->pos.x+10,w->pos.y+40,w->size.x-10,w->size.y-80,classname);

  PushButtons();
  b_ok = AddButtonText(0, 0, 0, "OK");
  MoveButton(b_ok, w->pos.x + 10, w->pos.y + w->size.y - 30);

  DrawButtons();

  RefreshScreen();

  do
  {
    UpdateMouse();
    b = UpdateButtons();
  } while (!TestKey(KEY_ENTER) && (b != b_ok));

  Q.num_popups--;
  QUI_PopDownWindow(POP_WINDOW_1 + Q.num_popups, &TempBuf);

  RemoveButton(b_ok);
  PopButtons();

  RefreshScreen();
}
#endif

void
FixColors(void)
{
  class_t* c;
  int i;

  for (i = 0; i < num_classes; i++)
  {
    c = classes[i];
    c->color = AddColor(c->col[0], c->col[1], c->col[2], 0);
  }
  SetGammaPal(video.pal);
}

void
SetEntityDefaults(entity_t* e)
{
  class_t* cl;
  const char* c;
  int i;

  if (!(c = GetKeyValue(e, "classname")))
    return;

  cl = FindClass(c);
  if (!cl)
    return;

  for (i = 0; i < cl->num_defs; i++)
    SetKeyValue(e, cl->defs[i].key, cl->defs[i].value);
}
