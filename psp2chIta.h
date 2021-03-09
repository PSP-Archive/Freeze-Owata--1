/*
* $Id: psp2chIta.h 151 2008-09-09 08:39:16Z bird_may_nike $
*/

#ifndef __PSP2CH_ITA_H__
#define __PSP2CH_ITA_H__

#include "psp2ch.h"

void psp2chItaSetMenuString(const char **sBtnH, const char **sBtnV);
int psp2chIta(void);
int psp2chItaList(void);
int psp2chGetMenu(void);
int psp2chItenCheck(char* host, char* ita);

#endif
