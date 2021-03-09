/*
* $Id: psp2chRes.h 88 2008-04-04 08:38:05Z bird_may_nike $
*/

#ifndef __PSP2CH_RES_H__
#define __PSP2CH_RES_H__

void psp2chResSetMenuString(const char **sBtnH, const char **sBtnV);
int psp2chFavoriteRes(int ret);
int psp2chThreadRes(int ret);
int psp2chJumpRes(int ret);
void psp2chResResetAnchors(void);
int psp2chResCursorMove(S_2CH_SCREEN *screen, int totalLine, int lineEnd, int* cursorX, int* cursorY, int limitX, int limitY);
void psp2chResPadMove(int* cursorX, int* cursorY, int limitX, int limitY);
int psp2chSearchRes(int ret);
void psp2chResCheckNG(void);
int psp2chResList(char* host, char* dir, char* title, unsigned long dat);
int psp2chGetDat(char* host, char* dir, char* title, unsigned long dat);
int psp2chDrawResHeader(int re, int* skip, int line, int lineEnd, int startX, int endX, S_2CH_RES_COLOR c, S_2CH_HEADER_COLOR hc, int* drawLine);
int psp2chDrawResText(int res, int* skip, int line, int lineEnd, int startX, int endX, S_2CH_RES_COLOR c, int* drawLine);
int psp2chCountRes(int res, int width);
int psp2chResCopy(int numMenu, int ref, int header);
int psp2chReadIdx(char *lastModified, char *eTag, int *range, int *startRes, int *startLine, int *endRes, int *update, char *threadTitle,
	const char *host, const char *title, const unsigned long dat);
void psp2chWriteIdx(const char *host, const char *title, const unsigned long dat, int forcedUpdate);

#endif
