/*
* $Id: psp2chThread.h 77 2008-03-31 07:19:46Z bird_may_nike $
*/

#ifndef __PSP2CH_THREAD_H__
#define __PSP2CH_THREAD_H__

void psp2chThreadSetMenuString(const char **sBtnH, const char **sBtnV);
int psp2chThread(int retSel);
void psp2chSort(int sort);
int psp2chThreadSearch(void);

#endif
