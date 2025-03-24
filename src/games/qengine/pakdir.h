#ifndef PAKDIR_H
#define PAKDIR_H

int PD_Init(void);

void PD_Search(const char* p_dir, const char* p_ext, void (*p_addtname)(char* name, int nl, int ofs));

void PD_Write(void);

struct tex_name_s;
FILE* PD_Load(struct tex_name_s* tn, int* baseofs, int* closefile);

#endif
