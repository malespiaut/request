/*
undo.h file of the Quest Source Code

Copyright 1997, 1998 Alexander Malmberg
Copyright 1996, Trey Harrison and Chris Carollo

This program is distributed under the GNU General Public License.
See legal.txt for more information.
*/

#ifndef UNDO_H
#define UNDO_H

#define UNDO_NONE   0   // no change, don't save undo information
#define UNDO_CHANGE 1   // changing, moving, whatever
#define UNDO_DELETE 2   // deleting

void SUndo(int etype,int btype);

void AddDBrush(struct brush_s *b);

void AddDEntity(entity_t *e);

void Undo(void);

void ClearUndo(void);

void UndoDone(void); // Takes care of texture locking and updating the BSP
                     // tree, both of which require access to both the new
                     // and old brushes.


int MapSize(void);
int UndoSize(void);

#endif

