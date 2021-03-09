
#ifndef __PSP2CH_REG_H__
#define __PSP2CH_REG_H__

void psp2chRegGetStr(char *dst, const char *orig, OnigRegion *region, int num);
void psp2chRegReplace(char *dst, const char *base, const char *orig, OnigRegion *region);
int psp2chRegPatGet(const char *host, int *code, char *pattern1, char *pattern2);
int psp2chUrlPatGet(char *dst, char *ref, int *mode, const char *src);
int psp2chFormGetPattern(char *target_url, char *filename, char *target_ref, char *encode, char *src_url, char *name, char *mail, char *message, char *title);
int psp2chBBSGetPattern(char *dst_url, char *referer, char *src_url, int endRes, char *filename);

#endif