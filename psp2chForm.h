/*
* $Id: psp2chForm.h 23 2008-02-27 07:58:08Z bird_may_nike $
*/

#ifndef __PSP2CH_FORM_H__
#define __PSP2CH_FORM_H__

int psp2chFormBeLogin(int flag);
void psp2chFormBeLogout(void);
int psp2chFormBeLoginCheck(void);
int psp2chForm(char* host, char* dir, char* title, int dat, char* subject, char* message, S_SCROLLBAR *bar);
void psp2chClearCookieFor2ch(void);

#endif
