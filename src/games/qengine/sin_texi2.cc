#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include "defines.h"
#include "types.h"

#include "sin_texi.h"

#include "sin.h"

#include "edit.h"
#include "message.h"
#include "quest.h"
}

#include "gui/gui.h"

static const char* cont_names[32] =
  {
    {"solid"},
    {"window"},
    {"fence"},
    {"lava"},
    {"slime"},
    {"water"},
    {"mist"},
    {"0x80"},
    {"0x100"},
    {"0x200"},
    {"0x400"},
    {"0x800"},
    {"0x1000"},
    {"0x2000"},
    {"0x4000"},
    {"0x8000"},
    {"playerclip"},
    {"monsterclip"},
    {"current_0"},
    {"current_90"},
    {"current_180"},
    {"current_270"},
    {"current_up"},
    {"current_dn"},
    {"origin"},
    {"monster"},
    {"corpse"},
    {"detail"},
    {"translucent"},
    {"ladder"},
    {"0x40000000"},
    {"0x80000000"}};

static const char* surf_names[32] =
  {
    {"light"},
    {"masked"},
    {"sky"},
    {"warping"},
    {"nonlit"},
    {"nofilter"},
    {"conveyor"},
    {"nodraw"},
    {"hint"},
    {"skip"},
    {"wavy"},
    {"ricochet"},
    {"prelit"},
    {"mirror"},
    {"console"},
    {"usecolor"},
    {"hardwareonly"},
    {"damage"},
    {"weak"},
    {"normal"},
    {"add-blend"},
    {"envmapped"},
    {"random"},
    {"animate"},
    {"rndtime"},
    {"translate"},
    {"nomerge"},
    {"surfbit0"},
    {"surfbit1"},
    {"surfbit2"},
    {"surfbit3"},
    {"0x80000000"}};

static int s_and, s_or;
static int c_and, c_or;

static struct
{ // 0 if invalid, !0 if valid
  char animname;
  char value;
  char direct;
  char animtime;
  char nonlit;
  char directangle;
  char trans_angle;
  char directstyle;
  char translucence;
  char friction;
  char restitution;
  char trans_mag;
  char color;
} valid;
static int first_valid; // 1 if valid structure has been set at least once
static sin_texdef_t values;

static void
GetValues(texdef_t* t)
{
  int flags, contents, value;
  sin_texdef_t* s;

  s = &t->g.sin;

  flags = s->flags;
  contents = s->contents;
  value = s->value;

  s_and &= flags;
  s_or |= flags;

  c_and &= contents;
  c_or |= contents;

  if (first_valid)
  {
    first_valid = 0;
    memset(&valid, 0xff, sizeof(valid));
    values = *s;
    return;
  }

#define CHECK_VAL(x)      \
  if (valid.x)            \
    if (s->x != values.x) \
      valid.x = 0;

  CHECK_VAL(value);
  CHECK_VAL(direct);
  CHECK_VAL(animtime);
  CHECK_VAL(nonlit);
  CHECK_VAL(directangle);
  CHECK_VAL(trans_angle);
  CHECK_VAL(translucence);
  CHECK_VAL(friction);
  CHECK_VAL(restitution);
  CHECK_VAL(trans_mag);

#undef CHECK_VAL

  if (valid.color)
  {
    if ((s->color[0] != values.color[0]) ||
        (s->color[1] != values.color[1]) ||
        (s->color[2] != values.color[2]))
      valid.color = 0;
  }

  if (valid.animname)
    if (strcmp(s->animname, values.animname))
      valid.animname = 0;

  if (valid.directstyle)
    if (strcmp(s->directstyle, values.directstyle))
      valid.directstyle = 0;
}

static void
SetValues(texdef_t* t)
{
  sin_texdef_t* s;

  s = &t->g.sin;

  s->flags &= s_and;
  s->flags |= s_or;

  s->contents &= c_and;
  s->contents |= c_or;

#define SETFIELD(field)        \
  {                            \
    if (valid.field)           \
      s->field = values.field; \
  }

  SETFIELD(value);
  SETFIELD(direct);
  SETFIELD(animtime);
  SETFIELD(nonlit);
  SETFIELD(directangle);
  SETFIELD(trans_angle);
  SETFIELD(translucence);
  SETFIELD(friction);
  SETFIELD(restitution);
  SETFIELD(trans_mag);

#undef SETFIELD

  if (valid.color)
  {
    s->color[0] = values.color[0];
    s->color[1] = values.color[1];
    s->color[2] = values.color[2];
  }

  if (valid.animname)
    strcpy(s->animname, values.animname);

  if (valid.directstyle)
    strcpy(s->directstyle, values.directstyle);
}

void
Sin_ModifyFlags(void)
{
  gui_win* win;

  gui_list *lcont, *lsurf;
  gui_checkbox* cont[32];
  gui_checkbox* surf[32];

  gui_textbox *tb_value, *tb_direct;
  gui_textbox *tb_animtime, *tb_nonlit;
  gui_textbox *tb_directangle, *tb_trans_angle;
  gui_textbox* tb_translucence;
  gui_textbox* tb_friction;
  gui_textbox* tb_restitution;
  gui_textbox* tb_trans_mag;
  gui_textbox *tb_animname, *tb_directstyle;
  gui_textbox* tb_color;

  char buf[128];

  gui_button *b_ok, *b_cancel;

  gui_label* lbl;

  int i, j;

  event_t ev;

  if (!M.display.num_bselected && !M.display.num_fselected)
  {
    NewMessage("Nothing selected!");
    return;
  }

  s_and = c_and = -1;
  s_or = c_or = 0;
  first_valid = 1;
  ForEachSelTexdef(GetValues);

  win = new gui_win(-1, -1, 600, 400);
  win->Init("Modify flags");

  lcont = new gui_list(4, 48, -1, -1, win);
  for (i = 0; i < 32; i++)
  {
    if (c_and & (1 << i))
      j = cbvOn;
    else if (!(c_or & (1 << i)))
      j = cbvOff;
    else
      j = cbvUndef;

    cont[i] = new gui_checkbox(-1, -1, -1, -1, lcont);
    cont[i]->Init(j, cbfCanUndef, cont_names[i]);
  }
  lcont->Init(2);

  lbl = new gui_label(-1, -1, -1, -1, win);
  lbl->Init(txtLeft, "~Contents:", lcont);

  lsurf = new gui_list(lcont->x2 + 4, lcont->y1, -1, -1, win);
  for (i = 0; i < 32; i++)
  {
    if (s_and & (1 << i))
      j = cbvOn;
    else if (!(s_or & (1 << i)))
      j = cbvOff;
    else
      j = cbvUndef;

    surf[i] = new gui_checkbox(-1, -1, -1, -1, lsurf);
    surf[i]->Init(j, cbfCanUndef, surf_names[i]);
  }
  lsurf->Init(2);

  lbl = new gui_label(-1, -1, -1, -1, win);
  lbl->Init(txtLeft, "~Surface flags:", lsurf);

#define ADDBOX(x, y, sx, sy, field, name)            \
  {                                                  \
    tb_##field = new gui_textbox(x, y, sx, sy, win); \
    tb_##field->Init(64, buf);                       \
    lbl = new gui_label(-1, -1, -1, -1, win);        \
    lbl->Init(txtLeft, name ":", tb_##field);        \
  }

#define ADDBOXI(x, y, sx, sy, field, name) \
  {                                        \
    if (valid.field)                       \
      sprintf(buf, "%i", values.field);    \
    else                                   \
      strcpy(buf, "?");                    \
    ADDBOX(x, y, sx, sy, field, name);     \
  }

#define ADDBOXF(x, y, sx, sy, field, name) \
  {                                        \
    if (valid.field)                       \
      sprintf(buf, "%g", values.field);    \
    else                                   \
      strcpy(buf, "?");                    \
    ADDBOX(x, y, sx, sy, field, name);     \
  }

  ADDBOXI(lsurf->x2 + 4, lsurf->y1, 80, -1, value, "~value");
  ADDBOXI(tb_value->x2 + 4, tb_value->y1, 80, -1, direct, "~direct");

  ADDBOXF(tb_value->x1, tb_value->y2 + 16 + 8, 80, -1, animtime, "~animtime");
  ADDBOXF(tb_direct->x1, tb_animtime->y1, 80, -1, nonlit, "~nonlit");

  ADDBOXI(tb_value->x1, tb_animtime->y2 + 16 + 8, 164, -1, directangle, "d~irectangle");
  ADDBOXI(tb_value->x1, tb_directangle->y2 + 16 + 8, 164, -1, trans_angle, "~trans_angle");
  ADDBOXF(tb_value->x1, tb_trans_angle->y2 + 16 + 8, 164, -1, translucence, "t~ranslucence");
  ADDBOXF(tb_value->x1, tb_translucence->y2 + 16 + 8, 164, -1, friction, "~friction");
  ADDBOXF(tb_value->x1, tb_friction->y2 + 16 + 8, 164, -1, restitution, "r~estitution");
  ADDBOXF(tb_value->x1, tb_restitution->y2 + 16 + 8, 164, -1, trans_mag, "trans_ma~g");

  if (valid.animname)
    strcpy(buf, values.animname);
  else
    strcpy(buf, "?");
  ADDBOX(4, lsurf->y2 + 16 + 8, 128, -1, animname, "ani~mname");

  if (valid.directstyle)
    strcpy(buf, values.directstyle);
  else
    strcpy(buf, "?");
  ADDBOX(tb_animname->x2 + 4, tb_animname->y1, 128, -1, directstyle, "directst~yle");

  if (valid.color)
    sprintf(buf, "%g %g %g", values.color[0], values.color[1], values.color[2]);
  else
    strcpy(buf, "?");
  ADDBOX(tb_directstyle->x2 + 4, tb_animname->y1, 128, -1, color, "c~olor");

  b_ok = new gui_button(4, win->sy - 28, -1, -1, win);
  b_ok->Init(btnDefault, "OK");

  b_cancel = new gui_button(b_ok->x2 + 4, b_ok->y1, -1, -1, win);
  b_cancel->Init(btnCancel, "Cancel");

  win->InitPost();
  win->ReDraw();

  while (1)
  {
    win->Run(&ev);

    if ((ev.what == evCommand) && (ev.cmd == cmdbPressed))
      break;
  }

  win->UnDraw();
  win->Refresh();

  if (ev.control == b_ok)
  {
    char* c;

    memset(&valid, 0, sizeof(valid));

#define SETFIELD_I(field)                      \
  {                                            \
    c = tb_##field->GetText();                 \
    if (strcmp(c, "?"))                        \
      valid.field = 1, values.field = atoi(c); \
  }

#define SETFIELD_F(field)                      \
  {                                            \
    c = tb_##field->GetText();                 \
    if (strcmp(c, "?"))                        \
      valid.field = 1, values.field = atof(c); \
  }

    SETFIELD_I(value);
    SETFIELD_I(direct);
    SETFIELD_F(animtime);
    SETFIELD_F(nonlit);
    SETFIELD_I(directangle);
    SETFIELD_I(trans_angle);
    SETFIELD_F(translucence);
    SETFIELD_F(friction);
    SETFIELD_F(restitution);
    SETFIELD_F(trans_mag);

    c = tb_animname->GetText();
    if (strcmp(c, "?"))
    {
      valid.animname = 1;
      strcpy(values.animname, c);
    }

    c = tb_directstyle->GetText();
    if (strcmp(c, "?"))
    {
      valid.directstyle = 1;
      strcpy(values.directstyle, c);
    }

    c = tb_color->GetText();
    if (strcmp(c, "?"))
    {
      valid.color = 1;
      values.color[0] = values.color[1] = values.color[2] = 1;
      sscanf(c, "%f %f %f", &values.color[0], &values.color[1], &values.color[2]);
    }

    s_and = c_and = -1;
    s_or = c_or = 0;
    for (i = 0; i < 32; i++)
    {
      j = 1 << i;

      if (cont[i]->GetValue() == cbvOn)
        c_or |= j;
      if (cont[i]->GetValue() == cbvOff)
        c_and &= ~j;

      if (surf[i]->GetValue() == cbvOn)
        s_or |= j;
      if (surf[i]->GetValue() == cbvOff)
        s_and &= ~j;
    }

    ForEachSelTexdef(SetValues);
  }

  delete win;
}
