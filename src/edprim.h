#ifndef EDPRIM_H
#define EDPRIM_H

#define CUBE 1
#define TETRA 2
#define CYLINDER 3
#define SPHERE 4
#define TEXRECT 5
#define NPRISM 6
#define PYRAMID 7
#define DODEC 8
#define ICOS 9
#define BUCKY 10
#define TORUS 11

/*
Note: Be careful when calling this from outside edprim.c .
*/
int AddBrush(int type, int x, int y, int z, float info1, float info2, float info3, float info4);

void AddRoom(void);

void CreateBrush(int type);

#endif
