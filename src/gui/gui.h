#ifndef GUI_H
#define GUI_H

#define ABORT(x, args...) Abort(__PRETTY_FUNCTION__, x, ##args)

// #define FUNC printf("--- %s\n",__PRETTY_FUNCTION__);
#define FUNC

// gui_evt.cc

typedef struct event_s
{
  int what;
#define evNothing 0

#define evMouseMove 1
#define evMouse1Down 2
#define evMouse1Up 4
#define evMouse1Pressed 8
#define evMouse2Down 16
#define evMouse2Up 32
#define evMouse2Pressed 64

#define evKey 128
#define evCommand 256

#define evMouse 0x7f

  int key;

  int mx, my, mb;

  class gui_c* control;

  int cmd;
#define cmdNothing 0

// general stuff
#define cmdChanged 1

// button stuff
#define cmdbPressed 100
#define cmdbDefault 101
#define cmdbCancel 102

// custom control stuff
#define cmdRedraw 200
} event_t;

void GetEvent(event_t* e);

int GUI_InBox(int x1, int y1, int x2, int y2);

// gui_c.cc

class gui_c
{
public:
  int x1, y1, x2, y2;     // coordinates in parent
  int rx1, ry1, rx2, ry2; // coordinates on screen (real coordinates)
  int sx, sy;             // size

  gui_c *Next, *Prev;
  class gui_group* Parent;

  int Flags;
#define flCanFocus 1
#define flPreEvent 2
#define flPostEvent 4

public:
  gui_c(int x1, int y1, int sx, int sy, gui_group* parent);
  virtual ~gui_c(void);

  void UpdatePos(void);

  virtual void Draw(void);
  virtual void UnDraw(void);
  virtual void Refresh(void);
  virtual void ReDraw(void);

  virtual void HandleEvent(event_t* e);
  virtual void SendEvent(event_t* e);

  virtual void FocusOn(void);
  virtual void FocusOff(void);

  void SendEvent_Cmd(int cmd);
};

// gui_text.cc

class gui_text : public gui_c
{
protected:
  char* text;

public:
  gui_text(int x1, int y1, int sx, int sy, gui_group* parent);
  virtual ~gui_text(void);

  void Init(int align, char* stext);
#define txtLeft 0
#define txtRight 1
#define txtCenter 2

  virtual void Draw(void);
};

class gui_label : public gui_text
{
protected:
  gui_c* link;
  int hotkey;

public:
  gui_label(int x1, int y1, int sx, int sy, gui_group* parent);

  void Init(int align, char* stext, gui_c* link);

  virtual void Draw(void);

  virtual void HandleEvent(event_t* e);
};

// gui_btn.cc

class gui_button : public gui_c
{
protected:
  int bFlags;
#define btnDefault 1 // activate when enter is pressed in dialog
#define btnCancel 2  // activate when escape is pressed in dialog
#define btnFast 4    // don't wait until user lets go of button to send event

  int bType;
#define btntText 0
#define btntPic 1

  void* bData;

  int bState;
#define btsNormal 0
#define btsDisabled 1
#define btsFocused 2

  int bPressed;

public:
  gui_button(int x1, int y1, int sx, int sy, gui_group* parent);
  virtual ~gui_button(void);

  void Init(int flags, char* text);
  void InitP(int flags, char* pic);

  virtual void Draw(void);

  virtual void HandleEvent(event_t* e);

  virtual void FocusOn(void);
  virtual void FocusOff(void);

  void SetState(int newstate);
  int GetState(void);
};

// gui_tbox.cc

class gui_textbox : public gui_c
{
protected:
  char* text;
  unsigned int maxlen;

  int focused;

  int sel_start, sel_end;
  int cur_pos, cur_inc;

  void InsertStr(int len, char* str);
  void DeleteSel(void);

  void MoveTo(int pos);
  void MoveSel(int npos);
  int SkipWord(int start, int dir);

public:
  gui_textbox(int x1, int y1, int sx, int sy, gui_group* parent);
  virtual ~gui_textbox(void);

  void Init(unsigned int maxlen, char* default_text);

  virtual void Draw(void);

  virtual void HandleEvent(event_t* e);

  virtual void FocusOn(void);
  virtual void FocusOff(void);

  void SetText(char* ntext);
  char* GetText(void);
};

class gui_textbox_int : public gui_textbox
{
protected:
  int value;

public:
  gui_textbox_int(int x1, int y1, int sx, int sy, gui_group* parent);

  void Init(int startvalue);

  void SetValue(int value);
  int GetValue(void);

  virtual void FocusOff(void);
};

class gui_textbox_float : public gui_textbox
{
protected:
  float value;

public:
  gui_textbox_float(int x1, int y1, int sx, int sy, gui_group* parent);

  void Init(float startvalue);

  void SetValue(float value);
  float GetValue(void);

  virtual void FocusOff(void);
};

// gui_cbox.cc

class gui_checkbox : public gui_c
{
protected:
  int value;
#define cbvOff 0
#define cbvOn 1
#define cbvUndef 2

  int cbFlags;
#define cbfCanUndef 1
#define cbfFocused 2

  char* text;

  void NextValue(void);

public:
  gui_checkbox(int x1, int y1, int sx, int sy, gui_group* parent);
  virtual ~gui_checkbox(void);

  void Init(int avalue, int flags, const char* text);

  virtual void Draw(void);
  virtual void HandleEvent(event_t* e);

  virtual void FocusOn(void);
  virtual void FocusOff(void);

  int GetValue(void);
  void SetValue(int avalue);
};

// gui_grp.cc

class gui_group : public gui_c
{
protected:
  gui_c* Children;

  gui_c* Focus;

  gui_c* last_child;

#define MAX_EVENTQUE_EVENTS 4
  event_t EventQue[MAX_EVENTQUE_EVENTS];
  int numevents;

public:
  gui_group(int x1, int y1, int sx, int sy, gui_group* parent);
  virtual ~gui_group(void);

  virtual void AddChild(gui_c* child);
  gui_c* FindChild(int x, int y);

  virtual void Draw(void);

  virtual void FocusSet(gui_c* c);
  virtual void FocusUnset(gui_c* c);
  virtual void FocusNext(void);
  virtual void FocusPrev(void);

  virtual void HandleEvent(event_t* e);
  virtual void SendEvent(event_t* e);
  void Run(event_t* ev);
};

// gui_win.cc

class gui_win : public gui_group
{
protected:
  unsigned char* scrbuf;
  char* title;

public:
  gui_win(int x1, int y1, int sx, int sy);
  virtual ~gui_win(void);

  void Init(char* title, ...) __attribute__((format(printf, 2, 3)));
  void InitPost(void);

  virtual void Draw(void);
  virtual void UnDraw(void);

  virtual void HandleEvent(event_t* e);
};

// gui_list.cc

class gui_list : public gui_group
{
protected:
  int num_children;
  int num_columns;

  int* col_x;

public:
  gui_list(int x1, int y1, int sx, int sy, gui_group* parent);
  virtual ~gui_list(void);

  void Init(int columns);

  virtual void AddChild(gui_c* child);

  virtual void Draw(void);

  virtual void HandleEvent(event_t* e);

  virtual void FocusOn(void);
  virtual void FocusOff(void);
};

// gui_slst.cc

class gui_scroll_list : public gui_group
{
protected:
  int num_data;
  char** data;

  int pos, scroll;
  int num_lines;

  int tx2;

  void MoveTo(int npos);

  gui_button *b_up, *b_dn;

public:
  gui_scroll_list(int x1, int y1, int sx, int sy, gui_group* parent);
  virtual ~gui_scroll_list(void);

  void Init(void);

  virtual void Draw(void);

  virtual void HandleEvent(event_t* e);

  void Data_New(void);
  void Data_Add(char* txt);
  void Data_Done(void);
  void Data_Free(void);

  char* Data_Get(int num);
  int GetPos(void);
};

#endif
