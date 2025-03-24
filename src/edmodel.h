/*
edmodel.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef EDMODEL_H
#define EDMODEL_H

void HandleLeftClickModel(void);

void CreateModel(void);

int CreateLink(void);

int EditModel(void);

int DeleteModel(void);

void DrawAllModels(int vport);

void DrawAllLinks(int vport);

void AddToModel(void);

#endif
