/*
newgroup.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef NEWGROUP_H
#define NEWGROUP_H

void CreateWorldGroup(void);

group_t* CreateGroup(char* groupname);

void RemoveGroup(char* groupname);

group_t* FindVisGroup(group_t* first);

void GroupPopup(void);

void GetGroupName(char* name);

group_t* GroupPicker(void);

#endif
