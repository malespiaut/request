#ifndef CLIP2_H
#define CLIP2_H

int AddBrushes(struct brush_s* b1, struct brush_s* b2);

void MakeHollow(void);
void MakeRoom(void);

void PlaneSplit(void);

int JoinBrushes(struct brush_s* b1, struct brush_s* b2);

#endif
