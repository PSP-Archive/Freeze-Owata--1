/*
* $Id: psp2ch.h 141 2008-08-20 04:28:32Z bird_may_nike $
*/

#ifndef __PSP2CH_H__
#define __PSP2CH_H__

#include <pspkernel.h>
#include <pspctrl.h>
#include "psp2chError.h"

//#define DEBUG
#define RES_BAR_WIDTH (8)
#define RES_SCR_WIDTH (SCR_WIDTH - RES_BAR_WIDTH)
#define RES_BAR_WIDTH_V (6)
#define RES_SCR_WIDTH_V (SCR_HEIGHT - RES_BAR_WIDTH_V)
#define RES_A_X 45
#define RES_A_Y 30
#define RES_A_WIDTH 390
#define RES_A_HEIGHT 195
#define RES_A_LINE (RES_A_HEIGHT / LINE_PITCH)
#define RES_A_X_V 30
#define RES_A_Y_V 45
#define RES_A_WIDTH_V 195
#define RES_A_HEIGHT_V 390
#define RES_A_LINE_V (RES_A_HEIGHT_V / LINE_PITCH)

// 基本設定
#define FILE_PATH (512)
#define BUF_LENGTH (256) // ショート
#define TMP_BUF_SIZE (4096)
#define ACCESS_WAIT (1000) // ウェイト0.1秒
#define DISPLAY_WAIT (1 * 1000 * 1000)
#define TRUNC (PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC)
#define APPEND (PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND)
#define FILE_PARMISSION (0777)
#define STREAM_END '\0'
#define LINE_END '\n'

// ネットワーク設定
#define NET_HOST_LENGTH (64)
#define NET_PATH_LENGTH (256)
#define NET_REQUEST_HEADER (2048)
#define NET_UESER_AGENT (128)
#define NET_COOKIE_LENGTH (1024)

// 板
#define ITA_CATEGORY_LENGTH (32)
#define ITA_DIRECTORY_LENGTH (32)
#define ITA_NAME_LENGTH (32)
// スレッド一覧
#define SUBJECT_LENGTH (256)

// スレッド書き込み用
#define RES_NAME_LENGTH (64)
#define RES_MAIL_LENGTH (64)
#define RES_MESSAGE_LENGTH (2048)
#define RES_MESSAGE_MAX (4096)
#define RES_ETC_LENGTH (512)
#define RES_WRITETIME_MAX (50)									// samba24で管理するサーバーの数

#define FONT_DIR "font"
#define COLOR_DIR "color"

typedef struct {
    char name[ITA_CATEGORY_LENGTH];
    int itaId;
    char num; // add
} S_2CH_CATEGORY;

typedef struct {
    char host[NET_HOST_LENGTH];
    char dir[NET_PATH_LENGTH];
    char title[ITA_NAME_LENGTH];
} S_2CH_ITA;

typedef struct {
    char host[NET_HOST_LENGTH];
    char dir[NET_PATH_LENGTH];
    char title[ITA_NAME_LENGTH];
    int dat;
    char subject[SUBJECT_LENGTH];
    int res;
    int update;
} S_2CH_FAVORITE;

typedef struct {
    char cate[ITA_CATEGORY_LENGTH];
    char title[ITA_DIRECTORY_LENGTH];
} S_2CH_FAV_ITA;

typedef struct {
    int id;
    unsigned long dat;
    char title[SUBJECT_LENGTH];
    int res;
    int old;
    int ikioi;
} S_2CH_THREAD;

typedef struct {
    int num;
    char* name;
    char* mail;
    char* date;
    char* id;
    char* text;
    char* title;
    char* be;
    int line;
    int ng;
	int res;													// レスアンカーで示された数
	int resFlag;												// 重複カウント回避用
} S_2CH_RES;

typedef struct {
    int count;
    int start;
    int select;
} S_2CH_SCREEN;

typedef struct {
    unsigned char num;
	unsigned char num2;											// アンカー１回
	unsigned char num3;											// アンカー２回以上
    unsigned char name1;
    unsigned char name2;
    unsigned char mail;
    unsigned char date;
    unsigned char id0;											// 単発ID色
    unsigned char id1;
    unsigned char id2;
    unsigned char id3;
} S_2CH_HEADER_COLOR;

typedef struct {
    unsigned char text;
    unsigned char bg;
    unsigned char link;
    unsigned char alink;
} S_2CH_RES_COLOR;

typedef struct {
    unsigned char slider;
    unsigned char bg;
} S_2CH_BAR_COLOR;

typedef struct {
    unsigned char text;
    unsigned char bg;
    unsigned char bat1;
    unsigned char bat2;
    unsigned char bat3;
} S_2CH_MENU_COLOR;

typedef struct {
    unsigned char num[2];
    unsigned char category[2];
    unsigned char text1[2];
    unsigned char text2[2];
    unsigned char bg[2];
    unsigned char count1[2];
    unsigned char count2[2];
} S_2CH_THREAD_COLOR;

typedef struct {
    unsigned char text;
    unsigned char bg;
    unsigned char s_text;
    unsigned char s_bg;
    unsigned char dir;
} S_2CH_TXT_COLOR;

typedef struct {
    S_2CH_TXT_COLOR cate;
    S_2CH_TXT_COLOR ita;
    unsigned char base;
} S_2CH_ITA_COLOR;

typedef struct {
    unsigned char ita;
    unsigned char title;
    unsigned char title_bg;
} S_2CH_FORM_COLOR;

typedef struct {
    unsigned char arrow1;
    unsigned char arrow2;
} S_2CH_CURSOR_COLOR;

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int total;
    int view;
    int start;
} S_SCROLLBAR;

typedef struct {
    int x1;
    int x2;
    int line;
    int res[1001];
    int resCount;
} S_2CH_RES_ANCHOR;

typedef struct {
    int x1;
    int x2;
    int line;
	int http;
    char host[NET_HOST_LENGTH];
    char path[NET_PATH_LENGTH];
} S_2CH_URL_ANCHOR;

typedef struct {
    int x1;
    int x2;
    int line;
    char id[17];
} S_2CH_ID_ANCHOR;

typedef struct {
    int x1;
    int x2;
    int line;
    int num;
} S_2CH_NUM_ANCHOR;

typedef struct {
    int x1;
    int x2;
    int line;
    char be[15];
} S_2CH_BE_ANCHOR;

typedef struct {
    int x1;
    int x2;
    int line;
    int type;
    int id;
} S_2CH_ANCHOR_LIST;

typedef struct {
    int up, pUp, down, pDown, top, end;
} S_2CH_SCROLL_BUTTONS;

typedef struct {
    int ok, esc, move, reload, shift;
    int addFav, search2ch;
} S_2CH_ITA_BUTTONS;

typedef struct {
    int ok, move, change, del, shift;
    int sort, search2ch, update, trans;
} S_2CH_FAV_BUTTONS;

typedef struct {
    int ok, esc, move, reload, shift;
    int sort, search, search2ch;
} S_2CH_THREAD_BUTTONS;

typedef struct {
    int ok, esc, ita, fav, shift;
    int search2ch;
} S_2CH_SEARCH_BUTTONS;

typedef struct {
    int ok, esc;
    char main[112];
} S_2CH_MENU_WIN;

typedef struct {
    int del, esc;
    char main[112];
} S_2CH_MENU_NG;

typedef struct {
    int osk, esc, ok;
    char main[112];
} S_2CH_INPUT_DIALOG;

typedef struct {
    int form, back, reload, datDel, change, addFav, delFav, cursor, wide, search;
    int resForm, resFBack, resFRes, idView, idNG, idBack, resView, resMove, resBack, url, urlHtml, urlBack, urlDown;
    S_2CH_SCROLL_BUTTONS s;
} S_2CH_RES_BUTTONS;

typedef struct {
    char main[112];
    char sub[112];
} S_2CH_MENU_STR;

typedef struct {
    char main[112];
    char sub1[112];
    char sub2[112];
    char aNum[112];
    char aRes[112];
    char aId[112];
	char aBe[112];
    char aUrl[112];
} S_2CH_RES_MENU_STR;

typedef struct {
    int padReverse;
    int padAccel;
    int padCutoff;
    int favSelect;
    int imageView;
	int browserDisconnect;
	int logDisplay;
	int timecount;					// add
	int ikioi;						// add
	int formOSK;					// add
	int browserTabs;				// add
	int anchorRange;				// add
	int threadSearch;				// add
	int threadRoll;					// add
	int resType;					// add
	int fileSort;					// add
	int hbl;						// add
	int idCount;					// add
	int abon;						// add
	int colorSet;					// add
	int inputType;					// add
	int updateTime;					// add
	int rollTime;					// add
	int bgmLoop;					// add
	int picMenu;					// add
	int thumb;						// add
	int sleep;						// add
	int findMax;					// add
	int padSpeed;					// add
	char imageDir[FILE_PATH];		// add
    char logDir[FILE_PATH];			// add
    char bbsmenuAddr[FILE_PATH];	// add
    char templateDir[FILE_PATH];	// add
    char addCookie[256];			// add
	char google[256];				// add
	char unzipDir[FILE_PATH];		// add
	char unrarDir[FILE_PATH];		// add
	char bgmDir[FILE_PATH];			// add
} S_2CH_CONFIG;

typedef struct {
    int height;
    int pitch;
    int lineH;
    int lineV;
    char fileA[32];
    char fileJ[32];
    char name[32];
    char** set;
    int count;
    int select;
} S_2CH_FONT;

typedef struct {												// samba24管理用構造体
	char	host[128];
	int		waitTime;
	time_t	resTime;
} S_2CH_WAIT;

typedef struct {
    S_2CH_CATEGORY* categoryList;
    S_2CH_ITA* itaList;
    S_2CH_FAVORITE* favList;
    S_2CH_FAVORITE* findList;
    S_2CH_FAV_ITA* favItaList;
    S_2CH_THREAD* threadList;
    S_2CH_RES* resList;
    S_2CH_SCREEN category;
    S_2CH_SCREEN ita;
    S_2CH_SCREEN fav;
    S_2CH_SCREEN find;
    S_2CH_SCREEN favIta;
    S_2CH_SCREEN thread;
    S_2CH_SCREEN res;
    S_2CH_URL_ANCHOR urlAnchor[50];
    S_2CH_RES_ANCHOR resAnchor[50];
    S_2CH_ID_ANCHOR idAnchor[40];
    S_2CH_NUM_ANCHOR numAnchor[40];
	S_2CH_BE_ANCHOR beAnchor[40];
    S_2CH_ANCHOR_LIST anchorList[100];
    int urlAnchorCount;
    int resAnchorCount;
    int idAnchorCount;
    int numAnchorCount;
	int beAnchorCount;
    S_2CH_HEADER_COLOR resHeaderColor;
    S_2CH_RES_COLOR resColor;
    S_2CH_BAR_COLOR resBarColor;
    S_2CH_HEADER_COLOR resAHeaderColor;
    S_2CH_RES_COLOR resAColor;
    S_2CH_BAR_COLOR resABarColor;
    S_2CH_MENU_COLOR menuColor;
    S_2CH_THREAD_COLOR threadColor;
    S_2CH_ITA_COLOR cateOnColor;
    S_2CH_ITA_COLOR cateOffColor;
    S_2CH_FORM_COLOR formColor;
    S_2CH_TXT_COLOR menuWinColor;
    S_2CH_CURSOR_COLOR cursorColor;
    S_2CH_RES_BUTTONS btnRes[2];
    S_2CH_SCROLL_BUTTONS listB[2];
    S_2CH_ITA_BUTTONS itaB[2];
    S_2CH_THREAD_BUTTONS thB[2];
    S_2CH_FAV_BUTTONS favB[2];
    S_2CH_SEARCH_BUTTONS findB[2];
    S_2CH_MENU_WIN menuWin[2];
    S_2CH_MENU_NG menuNG[2];
    S_2CH_CONFIG cfg;
    S_2CH_FONT font;
    int running;
    int sel;
    char cwDir[FILE_PATH];
    SceCtrlData pad;
    SceCtrlData oldPad;
	S_2CH_WAIT wait[RES_WRITETIME_MAX];							// samba24管理用（面倒なので固定サイズで確保しちゃいます）
} S_2CH;

int psp2ch(void);
int psp2chOwata(void);
int psp2chCursorSet(S_2CH_SCREEN* line,  int lineEnd, int shift, int pad, int* change);
int psp2chPadSet(int scrollX);
int psp2chInit(void);
int psp2chTerm(void);
void psp2chGets(char* title, char* text, int num, int lines);
int psp2chInputDialog(const unsigned char* text1, char* text2, const char* pretext);
int psp2chErrorDialog(const int buttonType, const char* fmt, ...);
// add
int psp2chFileRead(SceUID fd, void *data, SceSize size);
int psp2chFileWrite(SceUID fd, const void *data, SceSize size);

#endif
