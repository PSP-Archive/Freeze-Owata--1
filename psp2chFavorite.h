/*
* $Id: psp2chFavorite.h 77 2008-03-31 07:19:46Z bird_may_nike $
*/

#ifndef __PSP2CH_FAVORITE_H__
#define __PSP2CH_FAVORITE_H__

void psp2chFavSetMenuString(const char **sBtnH, const char **sBtnV);
int psp2chFavorite(void);
int psp2chAddFavorite(char* host, char* dir, char* title, unsigned long dat);
int psp2chAddFavoriteIta(char* cate, char* title);
int psp2chDelFavorite(char* title, unsigned long dat);
int psp2chDelFavoriteIta(int index);

#endif
