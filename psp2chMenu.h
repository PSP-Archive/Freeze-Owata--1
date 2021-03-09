/*
* $Id: psp2chMenu.h 125 2008-06-24 07:34:35Z bird_may_nike $
*/

#ifndef __PSP2CH_MENU_H__
#define __PSP2CH_MENU_H__

void psp2chMenuSetMenuString(const char **sBtnH, const char **sBtnV);
int psp2chMenu(S_SCROLLBAR* bar);
char* psp2chGetNGBuf(const char* file, char* buf);
int psp2chNGAdd(const char* file, char* val);
void psp2chMenuFontSet(int select);
int psp2chMenuColor(S_SCROLLBAR* bar);
void psp2chSetPalette(const char* fmt, ...);
int psp2chMenuGetDir(const char* file, char*** list, int** cor);
int psp2chMenuForm(S_SCROLLBAR* bar, char* buf, int length, const char* filename);
int psp2chSubMenu(S_SCROLLBAR* bar, int numMenu, char *host, char *dir, int dat, char *title);

#endif
