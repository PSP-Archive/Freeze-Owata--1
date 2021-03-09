/*
* $Id: psp2chResWindow.h 120 2008-06-22 12:29:41Z bird_may_nike $
*/

#ifndef __PSP2CH_RES_WINDOW_H__
#define __PSP2CH_RES_WINDOW_H__

void psp2chResAnchor(S_2CH_RES_ANCHOR *anc);
void psp2chIdAnchor(int anchor);
void psp2chBeAnchor(int anchor);
int psp2chUrlAnchor(int anchor, int offset, int mode, const unsigned long dat);
void psp2chAnchorSearch(int res);

#endif
