/*
status.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef STATUS_H
#define STATUS_H

typedef struct
{
   char    game_str[512];

   char    tex_str[512];

   char    class_dir[512];

	int     vid_mode;
   int     use_vbe2;

	int     snap_size;
	int     angle_snap_size;

	int     pan_speed;
   float   zoom_speed;

	int     turn_frames;
	int     depth_clip;

	int     flip_mouse;

	int     pop_menu;
	int     menu_limit;

   int     snap_to_int;

   char    mdlpaks[512];
   int     draw_models;
   int     load_models;

	int     edit_mode;

   int     texlock;

   int     draw_pts;
   int     debug;
	int     kbd_hand;
	int     exit_flag;

   int     map2;

	int     rotate;
	int     scale;
	int     move;
	lvec3_t move_amt;
} status_t;

extern status_t status;

#endif

