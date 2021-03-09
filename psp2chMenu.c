/*
* $Id: psp2chMenu.c 148 2008-08-27 06:31:27Z bird_may_nike $
*
* 2010-05-31 STEAR BGMファイラー関連を修正
*/

#include "psp2ch.h"
#include <psputility.h>
#include <psppower.h> // scePowerSetClockFrequency
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include "pg.h"
#include "psp2chIni.h"
#include "psp2chMenu.h"
#include "psp2chRes.h"
#include "psp2chWlan.h"
#include "psp2chImageView.h"
#include "psp2chAudio.h"
#include "psphtmlviewer.h"
#include "psp2chForm.h"
#include "charConv.h"
#include "psp2chError.h"
#include "utf8.h"

#define SCE_ERROR_ERRNO_EEXIST	0x80010011

typedef struct {
	int dat;
	int match;
} searchResult;

extern S_2CH s2ch; // psp2ch.c
extern char keyWords[128]; //psp2ch.c
extern int autoUpdate; // psp2chRes.c
extern int autoScroll; // psp2chRes.c
extern int* threadSort; // psp2chThread.c
extern Window_Layer *winParam;

const char* ngNameFile = "ngname.txt";
const char* ngIDFile = "ngid.txt";
const char* ngWordFile = "ngword.txt";
const char* ngMailFile = "ngmail.txt";
const char* ngThreadFile = "ngthread.txt";
const char* colorFile = "color.ini";
extern const char searchWords[];

// display
int dispPalette = 0;
char palette[512] = "";

int gNGchange;													// NGワードに変化があったか

// prototype
static void psp2chMenuNG(S_SCROLLBAR* bar);
static int psp2chNGDel(const char* file, S_SCROLLBAR* bar,int type);
static void psp2chMenuFont(S_SCROLLBAR* bar);
//static int psp2chMenuColor(S_SCROLLBAR* bar);
static void psp2chDrawMenu(char** menuList, S_2CH_SCREEN menu, int x, int y, int width, int height);
static int psp2chMenuPlayer(S_SCROLLBAR* bar, char* base, char* message, int length);
static int psp2chMenuFileCtrl(S_SCROLLBAR* bar, char* path, char* name, int status);
static void psp2chMenuSystem(S_SCROLLBAR* bar);
static int myCompare(const void *p1, const void *p2);
static int psp2chMenuUnzip(char *file, char *destpath);

int	SMEMOfile(char *path);										// PSPメモ帳

int unzipToDir(const char *zippath, const char *destpath, const char *pass,char *info);	// miniunz.c
//int makedir (char *newdir);																// miniunz.c
int mainRAR(int argc, char *argv[]);													// rar.cpp


/*********************
メニュー文字列の作成
**********************/
#define getIndex(X, Y) \
    tmp = (X);\
    (Y) = 0;\
    for (i = 0; i < 16; i++)\
    {\
        if (tmp & 1)\
        {\
            break;\
        }\
        (Y)++;\
        tmp >>= 1;\
    }

void psp2chMenuSetMenuString(const char **sBtnH, const char **sBtnV)
{
    int index1, index2;
    int i, tmp;

    getIndex(s2ch.menuWin[0].ok, index1);
    getIndex(s2ch.menuWin[0].esc, index2);
    sprintf(s2ch.menuWin[0].main, "　%s : 決定　　%s : 戻る",
            sBtnH[index1], sBtnH[index2]);

    getIndex(s2ch.menuWin[1].ok, index1);
    getIndex(s2ch.menuWin[1].esc, index2);
    sprintf(s2ch.menuWin[1].main, "　%s : 決定　　%s : 戻る",
            sBtnV[index1], sBtnV[index2]);

    getIndex(s2ch.menuNG[0].del, index1);
    getIndex(s2ch.menuNG[0].esc, index2);
    sprintf(s2ch.menuNG[0].main, "　%s : 削除　　　%s : 戻る",
            sBtnH[index1], sBtnH[index2]);

    getIndex(s2ch.menuNG[1].del, index1);
    getIndex(s2ch.menuNG[1].esc, index2);
    sprintf(s2ch.menuNG[1].main, "　%s : 削除　　　%s : 戻る",
            sBtnV[index1], sBtnV[index2]);
}

/****************
メニュー選択ウィンドウ
****************/
#define MENU_WIDTH (126)
#define MENU_ITEM (10)
#define MENU_HEIGHT (MENU_ITEM * LINE_PITCH)
int psp2chMenu(S_SCROLLBAR* bar)
{
	static S_2CH_SCREEN menu;
    const char* menuList[] = {"NG 設定", "LAN 接続/切断", "フォント変更", "カラー変更", "ぐぐれ", "内蔵ブラウザ(ﾌﾞｯｸﾏｰｸ)", "BGMファイラ(BGM)", "BGMファイラ(IMAGE)", "Be Login/Logout", "システム"};
	SceIoStat	st;
    char* menuStr = s2ch.menuWin[winParam->tateFlag].main;
	char path[FILE_PATH];
    int lineEnd, temp = 0, change = 1, ret;
    int startX, startY, scrX, scrY;

	gNGchange = 0;												// スレタイNGワードに変更があったか
    startX = (winParam->width - MENU_WIDTH) / 2;
    startY = (winParam->height - MENU_HEIGHT) / 2;
    scrX = MENU_WIDTH;
    scrY = MENU_HEIGHT;
    lineEnd = MENU_ITEM;
    menu.count = MENU_ITEM;
    
    if (winParam->tateFlag)
    	temp = 1;
    pgCreateTexture();
    if (temp)
    	psp2chSetScreenParam(1);
    
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
				psp2chDrawMenu((char**)menuList, menu, startX, startY, scrX, scrY);
				pgPrintMenuBar(menuStr);  
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
                    switch (menu.select)
                    {
                    	case 0:
                        	psp2chMenuNG(bar);
                        	break;
	                    case 1:
	                    	if (psp2chIsConnect())
	                    		psp2chDisconnect();
	                    	else
		                        psp2chApConnect();
	                        psp2chClearCookieFor2ch();
	                        break;
	                    case 2:
	                        psp2chMenuFont(bar);
	                        scrY = MENU_HEIGHT;
	                        break;
	                    case 3:
	                        psp2chMenuColor(bar);
	                        break;
	                    case 4:
	                    	pspShowBrowser(s2ch.cfg.google, NULL);
	                    	break;
						case 5:
							st.st_size = 0;
							strcpy(path, "file://ef0:/PSP/SYSTEM/BROWSER/bookmarks.html");
							if (sceIoGetstat(&path[7], &st) < 0) {
								strcpy(&path[7], "ms0:/PSP/SYSTEM/BROWSER/bookmarks.html");
								sceIoGetstat(&path[7], &st);
							}
							if (st.st_size > 0) {				// ターゲットが見つかったなら
								pspShowBrowser(path, NULL);
							} else {
								psp2chErrorDialog(0, "内蔵ブラウザのブックマークファイルが見つかりません。");
							}
							break;
	                    case 6:
	                    	psp2chMenuPlayer(bar, s2ch.cfg.bgmDir, NULL, 0);
	                    	break;
						case 7:
							psp2chMenuPlayer(bar, s2ch.cfg.imageDir, NULL, 0);
							break;
						case 8:
							if (psp2chFormBeLoginCheck()) {
								if (psp2chErrorDialog(0, "Beからログアウトします\n（現在:ログイン中）")==1) {
									psp2chFormBeLogout();
								}
							} else {
								ret = psp2chErrorDialog(4, "Beにログインします\n（現在:ログアウト中）");
								if (ret==1 || ret==4) {
									psp2chFormBeLogin(ret==1 ? 0 : 1);
								}
							}
							break;
	                    case 9:
	                    	psp2chMenuSystem(bar);
	                    	break;
                    }
					change = 1;
                }
                else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
            }
            pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
    pgDeleteTexture();
    return gNGchange;
}

/****************
NG設定ウィンドウ
****************/
#define MENU_NG_WIDTH (92)
#define MENU_NG_ITEM (10)
#define MENU_NG_HEIGHT (MENU_NG_ITEM * LINE_PITCH)
static void psp2chMenuNG(S_SCROLLBAR* bar)
{
	static S_2CH_SCREEN menu;
	const char* menuList[] = {"NG名前登録", "NG名前削除", "NGID削除", "NGワード登録", "NGワード削除", "NGメール登録", "NGメール削除", "NGスレタイ登録", "NGスレタイ削除", "検索語削除"};
    const unsigned char title1[] = "NG登録する名前を入力してください";
    const unsigned char title2[] = "NG登録する単語を入力してください";
    const unsigned char title3[] = "NG登録するアドレスを入力してください";
    char* text = "NGネーム";
    char* menuStr = s2ch.menuWin[winParam->tateFlag].main;;
    int lineEnd, change = 1;
    int startX, startY, scrX, scrY;

    startX = (winParam->width - MENU_NG_WIDTH) / 2;
    startY = (winParam->height - MENU_NG_HEIGHT) / 2;
    scrX = MENU_NG_WIDTH;
    scrY = MENU_NG_HEIGHT;
    lineEnd = MENU_NG_ITEM;
    menu.count = MENU_NG_ITEM;
    
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
	            psp2chDrawMenu((char**)menuList, menu, startX, startY, scrX, scrY);
	            pgPrintMenuBar(menuStr);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
                    switch (menu.select)
                    {
                    case 0: // NG name add
                        if (psp2chInputDialog(title1, text, NULL) == 0 && keyWords[0])
                        {
                            psp2chNGAdd(ngNameFile, keyWords);
                        }
                        break;
                    case 1: // NG name del
                        psp2chNGDel(ngNameFile, bar, 0);
                        break;
                    case 2: // NG ID del
                        psp2chNGDel(ngIDFile, bar, 0);
                        break;
                    case 3: // NG word add
                        if (psp2chInputDialog(title2, text, NULL) == 0 && keyWords[0])
                        {
                            psp2chNGAdd(ngWordFile, keyWords);
                        }
                        break;
                    case 4: // NG word del
                        psp2chNGDel(ngWordFile, bar, 0);
                        break;
                    case 5: // NG mail add
                        if (psp2chInputDialog(title3, text, NULL) == 0 && keyWords[0])
                        {
                            psp2chNGAdd(ngMailFile, keyWords);
                        }
                        break;
                    case 6: // NG mail del
                        psp2chNGDel(ngMailFile, bar, 0);
                        break;
					case 7: // NG スレッドタイトル add
						if (psp2chInputDialog(title2, text, NULL) == 0 && keyWords[0]){
							psp2chNGAdd(ngThreadFile, keyWords);
							gNGchange = 1;
						}
						break;
					case 8: // NG スレッドタイトル del
						psp2chNGDel(ngThreadFile, bar, 0);
						gNGchange = 1;
						break;
                    case 9: // 検索語削除
                    	psp2chNGDel(searchWords, bar, 1);
                    	break;
                    }
					change = 1;
                }
                else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
}

/****************
NGファイルがあればバッファを確保し
ファイル内容を読み込んで返す
****************/
char* psp2chGetNGBuf(const char* file, char* buf)
{
    SceUID fd;
    SceIoStat st;
    char path[FILE_PATH];
    int ret;

    sprintf(path, "%s/%s", s2ch.cfg.logDir, file);
    ret = sceIoGetstat(path, &st);
    if (ret < 0)
    {
        return NULL;
    }
    buf = (char*)malloc(st.st_size + 2);
    if (buf == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return NULL;
    }
    fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
        free(buf);
        psp2chNormalError(FILE_OPEN_ERR, path);
        return NULL;
    }
    psp2chFileRead(fd, buf, st.st_size);
    sceIoClose(fd);
    buf[st.st_size] = '\0';
	buf[st.st_size+1] = '\0';
    return buf;
}

/****************
NG削除
****************/
#define MENU_NGLIST_WIDTH (200)
static int psp2chNGDel(const char* file, S_SCROLLBAR* bar, int type)
{
	static S_2CH_SCREEN menu;
    SceUID fd;
    char path[FILE_PATH];
    char *buf, *p, *q, *menuStr = s2ch.menuNG[winParam->tateFlag].main;
    char** list;
    int i, lineEnd, change = 1;
    int startX, startY, scrX, scrY;

    if (winParam->tateFlag)
    {
        startX = (winParam->width - MENU_NGLIST_WIDTH) / 2;
        startY = (winParam->height - 400) / 2;
        scrY = 390;
        lineEnd = 30;
    }
    else
    {
        startX = (winParam->width - MENU_NGLIST_WIDTH) / 2;
        startY = (winParam->height - 200) / 2;
        scrY = 195;
        lineEnd = 15;
    }
    scrX = MENU_NGLIST_WIDTH;
    buf = NULL;
    buf = psp2chGetNGBuf(file, buf);
    if (buf == NULL)
    {
        return -1;
    }
    p = buf;
    menu.count = 0;
	if (type){
		while (1){												// 入力履歴用
			if (*p++ == '\0'){
				menu.count++;
				if (*p == '\0') break;							// '\0'が連続していたら終端
			}
		}
	} else {
		while (*p)
		{
			if (*p++ == '\n')
			{
				menu.count++;
			}
		}
	}
    list = (char**)malloc(sizeof(char*) * menu.count);
    if (list == NULL)
    {
        free(buf);
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }
    p = buf;
	if (type){
		for (i = 0; i < menu.count; i++){						// 入力履歴用
			list[i] = p;
			p += strlen(p) + 1;
		}
	} else {
	    for (i = 0; i < menu.count; i++)
	    {
	        q = strchr(p, '\n');
	        if (q == NULL)
	        {
	            break;
	        }
	        *q = '\0';
	        list[i] = p;
	        p = q + 1;
	    }
	}
    sprintf(path, "%s/%s", s2ch.cfg.logDir, file);
    
    while (s2ch.running)
    {
        if (sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
				psp2chDrawMenu(list, menu, startX, startY, scrX, scrY);
				pgPrintMenuBar(menuStr);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuNG[winParam->tateFlag].esc)
                {
                    fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
                    if (fd < 0)
                    {
                        free(list);
                        free(buf);
                        sceCtrlPeekBufferPositive(&s2ch.oldPad, 1);
                        psp2chNormalError(FILE_OPEN_ERR, path);
                        return -1;
                    }
                    for (i = 0; i < menu.count; i++)
                    {
                        psp2chFileWrite(fd, list[i], strlen(list[i]));
						if (type){
							psp2chFileWrite(fd, "\0", 1);		// 入力履歴用
						} else {
	                        psp2chFileWrite(fd, "\n", strlen("\n"));
						}
                    }
                    sceIoClose(fd);
                    psp2chResCheckNG();
                    break;
                }
                else if (s2ch.pad.Buttons & s2ch.menuNG[winParam->tateFlag].del)
                {
                    menu.count--;
                    for (i = menu.select; i < menu.count; i++)
                    {
                        list[i] = list[i + 1];
                    }
                }
				change = 1;
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
    free(list);
    free(buf);
    return 0;
}

/****************
NG登録
****************/
int psp2chNGAdd(const char* file, char* val)
{
    SceUID fd;
    char path[FILE_PATH];

    sprintf(path, "%s/%s", s2ch.cfg.logDir, file);
    fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, FILE_PARMISSION);
    if (fd < 0)
    {
        psp2chNormalError(FILE_OPEN_ERR, path);
        return -1;
    }
    psp2chFileWrite(fd, val, strlen(val));
    psp2chFileWrite(fd, "\n", strlen("\n"));
    sceIoClose(fd);
    pgPrintMenuBar("登録しました");
	pgCopyMenuBar();
    flipScreen(0);
    sceKernelDelayThreadCB(DISPLAY_WAIT);
    psp2chResCheckNG();
    return 0;
}

void psp2chMenuFontSet(int select)
{
    if (s2ch.font.set == NULL)
    {
        return;
    }
    if (select >= s2ch.font.count)
    {
        return;
    }
    sscanf(s2ch.font.set[select], "%s %s %s %d %d",
                    s2ch.font.name,
                    s2ch.font.fileA,
                    s2ch.font.fileJ,
                    &s2ch.font.height,
                    &s2ch.font.pitch
    );
    s2ch.font.select = select;
}

/****************
フォント設定ウィンドウ
****************/
#define MENU_FONT_WIDTH (160)
#define MENU_FONT_ITEM (5)
static void psp2chMenuFont(S_SCROLLBAR* bar)
{
	static S_2CH_SCREEN menu;
    char **menuList;
    char *menuStr = s2ch.menuWin[winParam->tateFlag].main;
    int lineEnd = MENU_FONT_ITEM, change = 1;
    int i, startX, startY, scrX, scrY;

    if (s2ch.font.set == NULL)
    {
        return;
    }
    menuList = (char**)malloc(sizeof(char*) * s2ch.font.count);
    for (i = 0; i < s2ch.font.count; i++)
    {
        menuList[i] = (char*)malloc(sizeof(char) * 32);
        sscanf(s2ch.font.set[i], "%s", menuList[i]);
    }
    startX = (winParam->width - MENU_FONT_WIDTH) / 2;
    startY = (winParam->height - LINE_PITCH * lineEnd) / 2;
    scrX = MENU_FONT_WIDTH;
    scrY = LINE_PITCH * lineEnd;
    menu.start = 0;
    menu.count = s2ch.font.count;
    menu.select = s2ch.font.select;
    if (menu.select >= MENU_FONT_ITEM)
    {
        menu.start = menu.select - MENU_FONT_ITEM + 1;
    }
    
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
	            psp2chDrawMenu((char**)menuList, menu, startX, startY, scrX, scrY);
	            pgPrintMenuBar(menuStr);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
                    psp2chMenuFontSet(menu.select);
                    pgExtraFontInit();
					psp2chSetFontParam();
					psp2chSetBarParam();
					psp2chSetScreenParam(0);
                    scrY = LINE_PITCH * lineEnd;
					change = 1;
                }
                else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
    for (i = 0; i < s2ch.font.count; i++)
    {
        free(menuList[i]);
    }
    free(menuList);
}

/****************
カラー設定ウィンドウ
****************/
#define MENU_COLOR_WIDTH (100)
#define MENU_COLOR_ITEM (5)
//static int psp2chMenuColor(S_SCROLLBAR* bar)
int psp2chMenuColor(S_SCROLLBAR* bar)
{
	static S_2CH_SCREEN menu;
    SceUID fd;
    SceIoDirent dir;
    char path[FILE_PATH], strbuf[256];
    char **menuList;
    char *menuStr = s2ch.menuWin[winParam->tateFlag].main;
    int lineEnd = MENU_COLOR_ITEM, change = 1;
    int i, startX, startY, scrX, scrY;

    sprintf(path, "%s/%s", s2ch.cwDir, COLOR_DIR);
    if ((fd = sceIoDopen(path)) < 0)
    {
        psp2chNormalError(DIR_MAKE_ERR, path);
        return -1;
    }
    menu.count = 0;
    memset(&dir, 0, sizeof(dir)); // 初期化しないとreadに失敗する
    while (sceIoDread(fd, &dir) > 0)
    {
        if (dir.d_stat.st_attr & FIO_SO_IFDIR)
        {
            continue;
        }
        menu.count++;
    }
    sceIoDclose(fd);
    menuList = (char**)malloc(sizeof(char*) * menu.count);
    if (menuList == NULL )
    {
        psp2chNormalError(MEM_ALLC_ERR, "menuColorList");
        return -1;
    }
    if ((fd = sceIoDopen(path)) < 0)
    {
        free(menuList);
        psp2chNormalError(DIR_MAKE_ERR, path);
        return -1;
    }
    menuList[0] = "デフォルト";
    i = 1;
    memset(&dir, 0, sizeof(dir));
    while (sceIoDread(fd, &dir) > 0)
    {
        if ((dir.d_stat.st_attr & FIO_SO_IFDIR) || stricmp(dir.d_name, colorFile) == 0)
        {
            continue;
        }
		if (s2ch.cfg.hbl){
			psp2chUTF82Sjis(strbuf, dir.d_name);
		} else {
			strcpy(strbuf, dir.d_name);
		}
        menuList[i] = (char*)malloc(strlen(strbuf) + 1);
        strcpy(menuList[i], strbuf);
        i++;
        if (i >= menu.count)
        {
            break;
        }
    }
    sceIoDclose(fd);
    startX = (winParam->width - MENU_COLOR_WIDTH) / 2;
    startY = (winParam->height -LINE_PITCH * lineEnd) / 2;
    scrX = MENU_COLOR_WIDTH;
    scrY = LINE_PITCH * lineEnd;
    menu.start = 0;
    menu.select = 0;
    if (menu.select >= MENU_COLOR_ITEM)
    {
        menu.start = menu.select - MENU_COLOR_ITEM + 1;
    }
    
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
	            psp2chDrawMenu((char**)menuList, menu, startX, startY, scrX, scrY);
	            pgPrintMenuBar(menuStr);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
					unsigned short	ucs[256];
					unsigned char	utf8[256];
					if (s2ch.cfg.hbl){
						psp2chSJIS2UCS(ucs, menuList[menu.select], 256);
						psp2chUCS2UTF8(utf8, ucs);
					} else {
						strcpy(utf8, menuList[menu.select]);
					}
					psp2chIniSetColor(menu.select == 0 ? NULL : utf8);
					change = 1;
                }
                else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
    for (i = 1; i < menu.count; i++)
    {
        free(menuList[i]);
    }
    free(menuList);
	pgCursorColorSet();
    return 0;
}

/****************
メニューウィンドウ描画
****************/
static void psp2chDrawMenu(char** menuList, S_2CH_SCREEN menu, int x, int y, int width, int height)
{
    int i, start, lineEnd;
	
    pgSetDrawStart(x, y, 0, 0);
    lineEnd = height / LINE_PITCH;
    start = menu.start;
    if (start + lineEnd > menu.count)
    {
        start = menu.count - lineEnd;
    }
    if (start < 0)
    {
        start = 0;
    }
    pgFillvram(0, 0, 0, BUF_WIDTH, BUF_HEIGHT, 2);
    pgFillvram(s2ch.menuWinColor.bg, x, y, width, height, 2);
    for (i = start; i < start + lineEnd; i++)
    {
        if (i >= menu.count)
        {
            return;
        }
        if (i == menu.select)
        {
            pgFillvram(s2ch.menuWinColor.s_bg, x, winParam->pgCursorY, width, LINE_PITCH, 2);
            pgPrint(menuList[i], s2ch.menuWinColor.s_text, s2ch.menuWinColor.s_bg, x + width);
        }
        else
        {
            pgPrint(menuList[i], s2ch.menuWinColor.text, s2ch.menuWinColor.bg, x + width);
        }
        pgSetDrawStart(x, -1, 0, LINE_PITCH);
    }
}

/****************
ファイルリストウィンドウ描画
****************/
static void psp2chDrawMenu2(char** menuList, int* menuCor, S_2CH_SCREEN menu, int x, int y, int width, int height)
{
	char	strbuf[256];
	int		i, start, lineEnd;

	pgSetDrawStart(x, y, 0, 0);
	lineEnd = height / LINE_PITCH;
	start = menu.start;
	if (start + lineEnd > menu.count){
		start = menu.count - lineEnd;
	}
	if (start < 0){
		start = 0;
	}
	pgFillvram(0, 0, 0, BUF_WIDTH, BUF_HEIGHT, 2);
	pgFillvram(s2ch.menuWinColor.bg, x, y, width, height, 2);
	for (i = start; i < start + lineEnd; i++){
		if (i >= menu.count){
			return;
		}
		if (s2ch.cfg.hbl){
			psp2chUTF82Sjis(strbuf, menuList[i]);
		} else {
			strcpy(strbuf, menuList[i]);
		}
		if (i == menu.select){
			pgFillvram(s2ch.menuWinColor.s_bg, x, winParam->pgCursorY, width, LINE_PITCH, 2);
			if (menuCor && menuCor[i]){							// ディレクトリ用
				pgPrint(strbuf, s2ch.menuWinColor.dir, s2ch.menuWinColor.s_bg, x + width);
			} else {											// ファイル用
				pgPrint(strbuf, s2ch.menuWinColor.s_text, s2ch.menuWinColor.s_bg, x + width);
			}
		} else {
			if (menuCor && menuCor[i]){							// ディレクトリ用
				pgPrint(strbuf, s2ch.menuWinColor.dir, s2ch.menuWinColor.bg, x + width);
			} else {											// ファイル用
				pgPrint(strbuf, s2ch.menuWinColor.text, s2ch.menuWinColor.bg, x + width);
			}
		}
		pgSetDrawStart(x, -1, 0, LINE_PITCH);
	}
}

#define MENU_MP3FILE_WIDTH (440)
#define MENU_MP3FILE_ITEM (18)
static int psp2chMenuPlayer(S_SCROLLBAR* bar, char* base, char* message, int length)
{
	static S_2CH_SCREEN	menu;
    SceIoStat			stats;
    char				path[FILE_PATH], dPath[2][FILE_PATH], file[FILE_PATH];
    char				**menuList = NULL;
    int					**menuCor = NULL;
//    char* menuStr = s2ch.menuWin[winParam->tateFlag].main, *p;
	char				*p,menuStr[128];
    int					lineEnd = MENU_MP3FILE_ITEM, change = 1;
    int					i, startX, startY, scrX, scrY;
	int					picMenu, thumbFlag, drive;
    
	strcpy(menuStr, s2ch.menuWin[winParam->tateFlag].main);
	strcat(menuStr, "    □ : BGM停止    △ : メニュー    St : ブラウザで表\示");	// キーガイダンス追加
    if (base)
    	strcpy(path, base);
    else
    	strcpy(path, s2ch.cwDir);
	strcpy(dPath[0], path);
	strcpy(dPath[1], "ef0:");
	drive = 0;													// 0:ms0 1:ef0
    menu.count = psp2chMenuGetDir(path, &menuList, &menuCor);
    strcpy(file, path);
	picMenu = s2ch.cfg.picMenu;									// メニュー表示
	thumbFlag = s2ch.cfg.thumb;									// 倍率

    if(menu.count < 0)
    	return -1;
    startX = (winParam->width - MENU_MP3FILE_WIDTH) / 2;
    startY = (winParam->height - LINE_PITCH * lineEnd) / 2;
    scrX = MENU_MP3FILE_WIDTH;
    scrY = LINE_PITCH * lineEnd;
    menu.start = 0;
    menu.select = 0;
    if (menu.select >= MENU_MP3FILE_ITEM)
    {
        menu.start = menu.select - MENU_MP3FILE_ITEM + 1;
    }
    
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
			if (change == 2){									// ファイル配置が変化したので再取得
				for (i = 0; i < menu.count; i++)
				{
					free(menuList[i]);
				}
				free(menuList);
				free(menuCor);
				menu.count = psp2chMenuGetDir(path, &menuList, &menuCor);
				if(menu.count < 0)
					return -1;
				if (menu.select>=menu.count) menu.select = menu.count - 1;
				if (menu.start>menu.select) menu.start = menu.select;
			}
        	if (change)
			{
				char sts[128],drv[5];
	            psp2chDrawMenu2((char**)menuList, menuCor, menu, startX, startY, scrX, scrY);
				for (i=0; i<4; i++) {
					drv[i] = path[i];
				}
				drv[4] = '\0';
				sprintf(sts, "%s    [%s]", menuStr, drv);
				pgPrintMenuBar(sts);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
                	strcpy( file, path );
                	strcat( file, "/" );
                	strcat( file, menuList[menu.select] );
//                	sprintf(file, "%s/%s", path, menuList[menu.select]);
                	sceIoGetstat(file, &stats);
                	if ((stats.st_mode & FIO_S_IFDIR) || strcmp(menuList[menu.select], "..") == 0)
                	{
                		// 上へ
                		if (strcmp(menuList[menu.select], "..") == 0)
                		{
                			p = strrchr(path, '/');
                			*p = '\0';
                		}
                		else
                			strcpy(path, file);
                		for (i = 0; i < menu.count; i++)
    					{
        					free(menuList[i]);
    					}
    					free(menuList);
    					free(menuCor);
    					menu.count = psp2chMenuGetDir(path, &menuList, &menuCor);
    					if(menu.count < 0)
    						return -1;
    					menu.start = 0;
    					menu.select = 0;
    					if (menu.select >= MENU_MP3FILE_ITEM)
    					{
        					menu.start = menu.select - MENU_MP3FILE_ITEM + 1;
    					}
                	}
                	else
                	{
						while (s2ch.running){					// 画像送り用ループ
							int type,next;
	                		p = strrchr(menuList[menu.select], '.');
							type = 0;								// 0:その他 1:画像ファイル
	                		if(p)
	                		{
		                		if (stricmp(p, ".mp3") == 0)
		                		{
				               		psp2chAudioPlay(path, menuList[menu.select]);
								}
								if (stricmp(p, ".jpg") == 0 || stricmp(p, ".jpeg") == 0)
								{
									psp2chMenuImageView(file, stats.st_size, &picMenu, &thumbFlag, 0);
									type = 1;
								}
								if (stricmp(p, ".png") == 0)
								{
									psp2chMenuImageView(file, stats.st_size, &picMenu, &thumbFlag, 1);
									type = 1;
								}
								if (stricmp(p, ".gif") == 0)
								{
									psp2chMenuImageView(file, stats.st_size, &picMenu, &thumbFlag, 2);
									type = 1;
								}
								if (stricmp(p, ".bmp") == 0)
								{
									psp2chMenuImageView(file, stats.st_size, &picMenu, &thumbFlag, 3);
									type = 1;
								}
								if (stricmp(p, ".txt") == 0)
								{
									if(message != NULL)				// 「読み込み」時の処理
									{
										int		datasize;
										SceUID fd = sceIoOpen(file, PSP_O_RDONLY, FILE_PARMISSION);
										if(fd < 0)
										{
											psp2chNormalError(FILE_OPEN_ERR, file);
										}
										p = (char*)malloc(RES_MESSAGE_LENGTH * sizeof(char));
										memset(p, STREAM_END, RES_MESSAGE_LENGTH);
										psp2chFileRead(fd, p, length);
										sceIoClose(fd);
										datasize = strlen(message);	// 読み込み前の文章サイズ
										memcpy(message + datasize, p, length - 1);
										free(p);
										message[datasize + length - 1] = '\0';
										p = NULL;
										sprintf(file, "set message %d bytes", strlen(message));
										pgPrintMenuBar(file);
										pgCopyMenuBar();
										flipScreen(0);
										sceKernelDelayThread(DISPLAY_WAIT);
										break;
									} else {
										SMEMOfile( file );			// 指定ファイルをPSPメモ帳で開く
										pgSetupGu();
										pgCursorColorSet();			// カーソルイメージ再設定
									}
								}
								if (stricmp(p, ".ini") == 0 || stricmp(p, ".cfg") == 0){
									SMEMOfile( file );				// 指定ファイルをPSPメモ帳で開く
									pgSetupGu();
									pgCursorColorSet();				// カーソルイメージ再設定
								}
								if (stricmp(p, ".zip") == 0 || stricmp(p, ".rar") == 0){
									char	tmp[FILE_PATH];
									int		ret;
									if (stricmp(p, ".rar") == 0){	// アーカイブ種類選択
										strcpy(tmp, s2ch.cfg.unrarDir);
										strcat(tmp, "/");
										ret = psp2chMenuUnRAR(file, tmp);
									} else {
										strcpy(tmp, s2ch.cfg.unzipDir);
										strcat(tmp, "/");
										strcat(tmp, menuList[menu.select]);
										p = strrchr(tmp, '.');
										if (p) *p = '\0';			// 拡張子は削除
										strcat(tmp, "/");
										ret = psp2chMenuUnzip(file, tmp);
									}
									if (ret!=1){					// アーカイブファイルの解凍
										for (i = 0; i < menu.count; i++){
											free(menuList[i]);
										}
										free(menuList);
										free(menuCor);
										tmp[strlen(tmp)-1] = '\0';
										strcpy(path, tmp);
										menu.count = psp2chMenuGetDir(path, &menuList, &menuCor);
										if(menu.count < 0){
											return -1;
										}
										menu.start = 0;
										menu.select = 0;
										if (menu.select >= MENU_MP3FILE_ITEM){
											menu.start = menu.select - MENU_MP3FILE_ITEM + 1;
										}
										change = 2;
									}
								}
							}
							if (type && (s2ch.pad.Buttons & PSP_CTRL_RTRIGGER ||
							             s2ch.pad.Buttons & PSP_CTRL_LTRIGGER)){	// 画像送り
								if (s2ch.pad.Buttons & PSP_CTRL_RTRIGGER){			// 次の画像へ
									next = -1;
									menu.select++;
									if (menu.select>=menu.count) menu.select = 0;
									while (next==-1){								// 次の画像ファイルを探す
										for (i=menu.select; i<menu.count ;i++){
											p = strrchr(menuList[i], '.');
											if(p){
												if (stricmp(p, ".jpg") == 0 || stricmp(p, ".jpeg") == 0 ||
												    stricmp(p, ".png") == 0 ||
												    stricmp(p, ".gif") == 0 ||
												    stricmp(p, ".bmp") == 0){
													next = i;
													break;
												}
											}
										}
										if (next==-1) menu.select = 0;				// 画像が見つからなければ先頭に戻ってやり直し
									}
								} else {											// 前の画像へ
									next = -1;
									menu.select--;
									if (menu.select<0) menu.select = menu.count -1;
									while (next==-1){								// 前の画像ファイルを探す
										for (i=menu.select; i>=0 ;i--){
											p = strrchr(menuList[i], '.');
											if(p){
												if (stricmp(p, ".jpg") == 0 || stricmp(p, ".jpeg") == 0 ||
												    stricmp(p, ".png") == 0 ||
												    stricmp(p, ".gif") == 0 ||
												    stricmp(p, ".bmp") == 0){
													next = i;
													break;
												}
											}
										}
										if (next==-1) menu.select = menu.select = menu.count -1;	// 画像が見つからなければ最後尾に戻ってやり直し
									}
								}
								menu.select = next;				// 次に表示する画像
								sprintf(file, "%s/%s", path, menuList[menu.select]);
								sceIoGetstat(file, &stats);
							} else {
								if (menu.select<menu.start) menu.start = menu.select;
								if (menu.select>menu.start+MENU_MP3FILE_ITEM-1) menu.start = menu.select - MENU_MP3FILE_ITEM + 1;
								break;							// 画像送りとは関係ないのでループ終了
							}
						}
					}
					change = 1;
                }
                else if (s2ch.pad.Buttons & PSP_CTRL_START)
                {
                	sprintf(file, "file://%s/%s", path, menuList[menu.select]);
                	pspShowBrowser(file, NULL);
                }
                else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
                else if (s2ch.pad.Buttons & PSP_CTRL_TRIANGLE)
                {
                	if (psp2chMenuFileCtrl(bar, path, menuList[menu.select], menuCor[menu.select])){
						change = 2;								// ファイル配置が変化したので再取得
					} else {
						change = 1;
					}
                }
                else if(s2ch.pad.Buttons & PSP_CTRL_SQUARE)
                {
                	psp2chAudioStop();
                	strcpy(file, path);
                }
                else if(s2ch.pad.Buttons & (PSP_CTRL_RTRIGGER | PSP_CTRL_LTRIGGER))
                {
					SceUID fd;
					strcpy(dPath[drive], path);
					drive = 1 - drive;							// ドライブパスを変更
					if ((fd = sceIoDopen(dPath[drive])) >= 0) {	// 指定ディレクトリにアクセスできるなら
						sceIoDclose(fd);
						strcpy(path, dPath[drive]);
						change = 2;								// ディレクトリ再取得
					} else {
						drive = 1 - drive;
					}
				}
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
    for (i = 0; i < menu.count; i++)
    {
        free(menuList[i]);
    }
    free(menuList);
    free(menuCor);
	return 0;
}

void psp2chSetPalette(const char* fmt, ...)
{
    va_list list;

	memset(palette,0,sizeof(palette));
    va_start(list, fmt);
    vsprintf(palette, fmt, list);
    va_end(list);
}

/*************************************
ディレクトリ情報取得
*************************************/
int psp2chMenuGetDir(const char* file, char ***list,int **cor)
{
    SceUID fd;
    SceIoDirent dir;
    int i, j, count = 0;
    char **menuList,**buf,path[1024];
	int *menuCor,buf2;
    
    strcpy( path, file );
    if (path[strlen(path)-1]=='\\') strcat(path,"\\");
    if ((fd = sceIoDopen(path)) < 0)							// pathの最後に'\'が来るとおかしな動作をする…?
    {
    	psp2chNormalError(DIR_MAKE_ERR, file);
    	return -1;
    }
    memset(&dir, 0, sizeof(dir)); // 初期化しないとreadに失敗する
    while (sceIoDread(fd, &dir) > 0)
    {
    	if(strcmp(dir.d_name, ".") == 0)
    	{
    		continue;
    	}
        count++;
    }
    sceIoDclose(fd);
    menuList = (char**)malloc(sizeof(char*) * count);
    if (menuList == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, "menuList");
        return -1;
    }
    menuCor = (int*)malloc(sizeof(int) * count);
    if (menuCor == NULL){
		free(menuList);
		psp2chNormalError(MEM_ALLC_ERR, "menuList");
		return -1;
	}
    if ((fd = sceIoDopen(path)) < 0)
    {
        free(menuList);
        free(menuCor);
        psp2chNormalError(DIR_MAKE_ERR, file);
        return -1;
    }
    i = 0;
    memset(&dir, 0, sizeof(dir));
    while (sceIoDread(fd, &dir) > 0)
    {
    	if(strcmp(dir.d_name, ".") == 0)
    	{
    		continue;
    	}
        buf = (char*)malloc(sizeof(char) * (strlen(dir.d_name) + 1));
		if (buf==NULL) break;									// メモリが無くなった！
		menuList[i] = buf;
		if (dir.d_stat.st_attr & FIO_SO_IFDIR){					// ディレクトリか？ファイルか？
			menuCor[i] = -1;									// ディレクトリ
		} else {
			menuCor[i] = 0;										// ファイル
		}
        strcpy(menuList[i], dir.d_name);
        i++;
        if (i >= count)
        {
            break;
        }
    }
    sceIoDclose(fd);
	if (s2ch.cfg.fileSort){										// ファイル配列ソート
		for (i=1; i<count-1 ;i++){
			for (j=i+1; j<count ;j++){
				if ((strcmp(menuList[i],menuList[j])>0 && menuCor[i]==menuCor[j]) || (!menuCor[i] && menuCor[j])){
					buf = menuList[i];
					menuList[i] = menuList[j];
					menuList[j] = buf;
					buf2 = menuCor[i];
					menuCor[i] = menuCor[j];
					menuCor[j] = buf2;
				}
			}
		}
	}
    *list = menuList;
	*cor = menuCor;
    return count;
}


//==============================================================
// ディレクトリ削除
//--------------------------------------------------------------
// 指定されたディレクトリを内部ファイルも含めて削除する。
// 内部にあるサブディレクトリも削除します。
//--------------------------------------------------------------

static void removeDir(char *path)
{
	SceIoDirent	dir;
	SceUID		fd;
	char		file[1024];

	if ((fd = sceIoDopen(path)) < 0) return;					// 指定ディレクトリを開けなかったので終了
	memset(&dir, 0, sizeof(dir));								// 初期化しないとreadに失敗する
	while (sceIoDread(fd, &dir) > 0){
		if(strcmp(dir.d_name, ".") == 0) continue;
		if(strcmp(dir.d_name, "..") == 0) continue;
		strcpy( file, path );
		strcat( file, "/" );
		strcat( file, dir.d_name );
		if (dir.d_stat.st_attr & FIO_SO_IFDIR){					// ディレクトリか？ファイルか？
			removeDir(file);									// ディレクトリ（再帰処理でサブディレクトリを処理する）
		} else {
			sceIoRemove(file);									// ファイル
		}
	}
	sceIoDclose(fd);
	sceIoRmdir(path);
}

//==============================================================
// ディレクトリ作成
//--------------------------------------------------------------
// 戻り値：0  成功
//         -1 作成に失敗
//--------------------------------------------------------------
// 指定されたディレクトリを作成する。
// 多階層をまとめて作成できます。
// 指定ディレクトリが既に存在している場合は成功と見なします。
//--------------------------------------------------------------

int createDir (char *newdir)
{
	char	*buffer, *p;
	char	hold;
	int		len, ret;

	len = strlen(newdir);
	if (len <= 0)
		return 0;

	buffer = (char*)malloc(len+1);
	strcpy(buffer,newdir);

	if (buffer[len-1] == '/'){
		buffer[len-1] = '\0';
	}
	ret = sceIoMkdir(buffer,0777);								// 単階層作成
	if (ret>=0 || ret==SCE_ERROR_ERRNO_EEXIST){					// 作成に成功するか既に存在してるなら
		free(buffer);
		return 0;
	}

	p = buffer+1;
	while (1){													// 多階層作成
		while (*p && *p != '\\' && *p != '/'){
			p++;
		}
		hold = *p;
		*p = '\0';
		ret = sceIoMkdir(buffer,0777);
		if (ret<0 && ret!=SCE_ERROR_ERRNO_EEXIST){
			free(buffer);
			return -1;											// ディレクトリ作成に失敗
		}
		if (hold == '\0'){
			break;
		}
		*p++ = hold;
	}
	free(buffer);
	return 0;
}

/****************
ファイル操作
****************/
#define FILE_CTRL_WIDTH (80)
#define FILE_CTRL_ITEM (6)
static int psp2chMenuFileCtrl(S_SCROLLBAR* bar, char* path, char* name, int status)
{
	static S_2CH_SCREEN menu;
	static int mode = 0; // コピー，移動のモード
	static char mpath[FILE_PATH], mname[FILE_PATH]; // 元の場所
    const char* menuList[] = {"コピー", "移動", "ペースト", "リネーム", "削除","メモ帳で開く"};
    const unsigned char title1[] = "新しいファイル名を入力してください";
    const char *text = "新しいファイル名";
    SceUID fd, dst;
    char file[FILE_PATH], old[FILE_PATH], buf[BUF_LENGTH];
    char* menuStr = s2ch.menuWin[winParam->tateFlag].main;
    int lineEnd = FILE_CTRL_ITEM, change = 1, size;
    int startX, startY, scrX, scrY,CHflag;

	menu.count = FILE_CTRL_ITEM;
    startX = (winParam->width - FILE_CTRL_WIDTH) / 2;
    startY = (winParam->height - LINE_PITCH * lineEnd) / 2;
    scrX = FILE_CTRL_WIDTH;
    scrY = LINE_PITCH * lineEnd;
    menu.start = 0;
    menu.select = 0;
    if (menu.select >= FILE_CTRL_ITEM)
    {
        menu.start = menu.select - FILE_CTRL_ITEM + 1;
    }
    
    CHflag = 0;													// ファイル配列に変更があった？（0:No）
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
	            psp2chDrawMenu((char**)menuList, menu, startX, startY, scrX, scrY);
	            pgPrintMenuBar(menuStr);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
                    switch (menu.select)
                    {
                	case 0:
						if (status) break;						// 対象はファイルではない
                    	mode = 1;
                    	strcpy(mpath, path);
                    	strcpy(mname, name);
                    	break;
                    case 1:
						if (status) break;						// 対象はファイルではない
                    	mode = 2;
                    	strcpy(mpath, path);
                    	strcpy(mname, name);
                        break;
                    case 2:
                    	if(!mode)
                    		break;
                    	sprintf(old, "%s/%s", mpath, mname);
                    	sprintf(file, "%s/%s", path, mname);
                    	if((fd = sceIoOpen(old, PSP_O_RDONLY, FILE_PARMISSION)) < 0)
                    	{
                    		psp2chNormalError(FILE_OPEN_ERR, old);
                    		break;
                    	}
                    	if((dst = sceIoOpen(file, PSP_O_WRONLY | PSP_O_CREAT, FILE_PARMISSION)) < 0)
                    	{
                    		psp2chNormalError(FILE_OPEN_ERR, file);
                    		sceIoClose(fd);
                    		break;
                    	}
                    	while((size = psp2chFileRead(fd, buf, sizeof(buf))) > 0)
                    	{
	                    	psp2chFileWrite(dst, buf, size);
	                    }
	                    sceIoClose(fd);
	                    sceIoClose(dst);
						CHflag = -1;							// ファイル配列に変更があった
	                    if(mode == 2)
	                    {
	                    	sceIoRemove(old);
	                    	strcpy(mpath, "");
	                    	strcpy(mname, "");
	                    }
	                    mode = 0;
                        break;
                    case 3:
						if (s2ch.cfg.hbl){
							psp2chUTF82Sjis(buf, name);
						} else {
							strcpy(buf, name);
						}
						if(psp2chInputDialog(title1, (char*)text, buf) == 0 && keyWords[0])
						{
							unsigned short	ucs[256];
							unsigned char	utf8[256];
							sprintf(old, "%s/%s", path, name);
							if (s2ch.cfg.hbl){
								psp2chSJIS2UCS(ucs, keyWords, 256);
								psp2chUCS2UTF8(utf8, ucs);
								sprintf(file, "%s/%s", path, utf8);
							} else {
								sprintf(file, "%s/%s", path, keyWords);
							}
							sceIoRename(old, file);
							CHflag = -1;						// ファイル配列に変更があった
						}
                        break;
                    case 4:										// 削除
						sprintf(file, "%s/%s", path, name);
						if (s2ch.cfg.hbl){
							psp2chUTF82Sjis(buf, file);
						} else {
							strcpy(buf, file);
						}
						if (status){							// 対象がディレクトリだった
							if (psp2chErrorDialog(2, "%s\nこれはフォルダです\n内部ファイル（サブフォルダも含む）も含めて削除されますがよろしいですか", 
											buf) == PSP_UTILITY_MSGDIALOG_RESULT_YES){
								removeDir(file);
								CHflag = -1;					// ファイル配列に変更があった
							}
						} else {								// 対象がファイルだった
							if (psp2chErrorDialog(2, "%s\n%s", buf, FILE_TXT_1) == PSP_UTILITY_MSGDIALOG_RESULT_YES)
							{
								sceIoRemove(file);
								CHflag = -1;					// ファイル配列に変更があった
							}
						}
                    	break;
                    case 5:										// メモ帳で開く
						if (status) break;						// 対象はファイルではない
						sprintf(file, "%s/%s", path, name);
						SMEMOfile( file );						// 指定ファイルをPSPメモ帳で開く
						pgSetupGu();
						pgCursorColorSet();						// カーソルイメージ再設定
						break;
                    }
					change = 1;
                    break;
                }
                else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
    return CHflag;
}

//==============================================================
// PSP内蔵ブラウザのURL履歴取得
//--------------------------------------------------------------
// buf    最後尾に選択されたURLのタイトルとURLを追加する。
// length bufのサイズ
//--------------------------------------------------------------
// メモリースティック上にある内蔵ブラウザ履歴保存ファイルよりURLリストを
// 取得しURLを buf に貼り付ける。
//--------------------------------------------------------------

//----- 指定位置からlong長を取得 -----

static int GetLong(char *data)
{
	unsigned char	*pos;
	int				val;

	pos = data;
	val  = (*pos++);
	val += (*pos++) << 8;
	val += (*pos++) << 16;
	val += (*pos++) << 24;
	return (val);
}

//----- メイン -----

static void psp2chMenuBrowserURL(S_SCROLLBAR* bar, char *buf, int length)
{
	static S_2CH_SCREEN	menu;
	SceUID		fd;
	SceIoStat	st;
	char		*data, path[FILE_PATH], utf8[FILE_PATH], sjis[FILE_PATH], **title, **url;
	char		*menuStr;
	int			i, ret, size, pos, max, count, posNext, type, titleLen, urlLen;
	int			startX, startY, scrX, scrY;
	int			lineEnd = MENU_MP3FILE_ITEM, change = 1;

	//----- 履歴ファイル取得 -----

	size = 0;
	st.st_size = 0;
	strcpy(path, "ef0:/PSP/SYSTEM/BROWSER/historyv.dat");
	if (sceIoGetstat(path, &st) < 0) {
		strcpy(path, "ms0:/PSP/SYSTEM/BROWSER/historyv.dat");
		sceIoGetstat(path, &st);
	}
	if (st.st_size > 0){										// ターゲットが見つかったなら
		data = (char*)malloc(st.st_size + 1);
		if (!data) {
			psp2chNormalError(MEM_ALLC_ERR, "MenuBrowserURL");
			return;
		}
		fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
		if (fd >= 0) {
			size = psp2chFileRead(fd, data, st.st_size);
			sceIoClose(fd);
		}
	}
	if (size <= 0) {											// ファイル内容の取得に失敗した
		psp2chErrorDialog(0, "PSP内蔵ブラウザのURL履歴ファイルの取得に失敗しました。");
		free(data);
		return;
	}

	//----- 履歴ファイル内容の解析 -----

	pos = 0x46;
	count = 0;
	while (pos < size) {										// 項目数をカウント
		posNext = GetLong(&data[pos]);							// レコードの終端位置
		pos += posNext;
		pos += 16;
		count++;
	}
	title = (char**)malloc(sizeof(char*) * count);
	if (!title) {
		psp2chNormalError(MEM_ALLC_ERR, "MenuBrowserURL title");
		free(data);
		return;
	}
	url = (char**)malloc(sizeof(char*) * count);
	if (!url) {
		psp2chNormalError(MEM_ALLC_ERR, "MenuBrowserURL url");
		free(title);
		free(data);
		return;
	}
	for (i=0; i<count; i++) {									// 内容をクリアしておく
		title[i] = NULL;
		url[i] = NULL;
	}
	max = count;

	pos = 0x46;
	count = 0;
	ret = 0;
	while (pos < size) {										// 項目内容を取得
		pos += 12;
		type = GetLong(&data[pos]);								// レコードのタイプ(0:URLなし 1:URLあり)
		pos += 4;
		titleLen = GetLong(&data[pos]);							// タイトルの文字数
		if (titleLen >= FILE_PATH) {
			ret = -1;											// エラー発生
			break;
		}
		pos += 4;
		memcpy(utf8, &data[pos], titleLen);						// タイトル文字列
		utf8[titleLen] = '\0';
		psp2chUTF82Sjis(sjis, utf8);
		title[count] = (char*)malloc(strlen(sjis) +1);
		strcpy(title[count], sjis);
		pos += titleLen;
		if (type != 0) {
			urlLen = GetLong(&data[pos]);						// URLの文字数
			if (urlLen >= FILE_PATH) {
				ret = -1;										// エラー発生
				break;
			}
			pos += 4;
			url[count] = (char*)malloc(urlLen +1);
			memcpy(url[count], &data[pos], urlLen);				// URLの文字列
			url[count][urlLen] = '\0';
			pos += urlLen;
		}
		pos += 16;
		count++;
		if (count>max) {
			ret = -1;											// エラー発生
			break;
		}
	}
	free(data);

	if (ret) {
		psp2chErrorDialog(0, "PSP内蔵ブラウザのURL履歴ファイルの解析に失敗しました");
	} else {

	//----- リスト選択 -----
	// 基本的にBGMファイラーの処理そのまま

		menuStr = "  ○ : 選択　　　× : 戻る";
		menu.count = count;
		startX = (winParam->width - MENU_MP3FILE_WIDTH) / 2;
		startY = (winParam->height - LINE_PITCH * lineEnd) / 2;
		scrX = MENU_MP3FILE_WIDTH;
		scrY = LINE_PITCH * lineEnd;
		menu.start = 0;
		menu.select = 0;
		menu.count = count;
		if (menu.select >= MENU_MP3FILE_ITEM) {
			menu.start = menu.select - MENU_MP3FILE_ITEM + 1;
		}

		while (s2ch.running) {
			if(sceCtrlPeekBufferPositive(&s2ch.pad, 1)) {
				if (change) {
					psp2chDrawMenu2((char**)title, NULL, menu, startX, startY, scrX, scrY);
				}
				psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
				if (s2ch.pad.Buttons != s2ch.oldPad.Buttons) {
					s2ch.oldPad = s2ch.pad;
					if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok) {
						if (url[menu.select]) {
							ret = psp2chErrorDialog(1, "%s\n%s", title[menu.select], url[menu.select]);
						} else {
							ret = psp2chErrorDialog(1, "%s", title[menu.select]);
						}
						if (ret == 1) break;					// 選択したなら終了
					}
					else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
					{
						menu.select = -1;						// 中断
						break;
					}
				}
				pgDrawTexture(1);
				pgCopyWindow(0, startX, startY, scrX, scrY);
				pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
				pgScrollbar(bar, s2ch.resBarColor);				// スクロールバーを作画
				pgCopyMenuBar();
				flipScreen(0);
			}
		}

	//----- URL出力 -----

		if (menu.select >= 0) {
			if (url[menu.select]) {
				if (strlen(buf) + strlen(title[menu.select]) + strlen(url[menu.select]) + 1 < length) {
					strcat(buf, title[menu.select]);
					strcat(buf, "\n");
					strcat(buf, &url[menu.select][1]);			// http → ttp
					strcat(buf, "\n");
				} else {
					psp2chErrorDialog(0, "文字数制限を超えるので処理を中断しました。");
				}
			} else {
				if (strlen(buf) + strlen(title[menu.select]) + 1 < length) {
					strcat(buf, title[menu.select]);
					strcat(buf, "\n");
				} else {
					psp2chErrorDialog(0, "文字数制限を超えるので処理を中断しました。");
				}
			}
		}
	}

	//----- 終了処理 -----

	for (i=0; i<count; i++) {
		free(url[i]);
		free(title[i]);
	}
	free(url);
	free(title);
}

/****************************************
add フォームでのメニューセット
****************************************/
#define MENU_WIDTH (98)
#define MENU_FORM_ITEM (4)
int psp2chMenuForm(S_SCROLLBAR* bar, char* buf, int length, const char* filename)
{
	static S_2CH_SCREEN menu;
    const char* menuList[] = {"保存", "読み込み", "p2表\示", "ブラウザURL履歴"};
    char* menuStr = s2ch.menuWin[winParam->tateFlag].main;
    SceUID fd;
    char path[FILE_PATH], newPath[FILE_PATH], buf2[FILE_PATH];
    int lineEnd = MENU_FORM_ITEM, change = 1, i;
    int startX, startY, scrX, scrY;

    if (buf && strcmp(s2ch.cfg.templateDir, "") != 0)
    	sprintf(path, "%s/%s", s2ch.cfg.templateDir, filename);
    else
    	strcpy(path, s2ch.cwDir);
    startX = (SCR_WIDTH - MENU_WIDTH) / 2;
    startY = (SCR_HEIGHT - lineEnd * LINE_PITCH) / 2;
    scrX = MENU_WIDTH;
    scrY = lineEnd * LINE_PITCH;
    menu.count = MENU_FORM_ITEM;
    
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
				psp2chDrawMenu((char**)menuList, menu, startX, startY, scrX, scrY);
				pgPrintMenuBar(menuStr);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if(s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
                    switch (menu.select)
                    {
                    	case 0:
							if (createDir(path)<0){				// フォルダが無いなら作成する
								pgErrorDialog(0, path);
								return -1;
							}
                    		for (i = 0; i < 1000; i++)
                    		{
                    			sprintf(newPath, "%s/%03d-%s.txt", path, i, filename);
                    			fd = sceIoOpen(newPath, PSP_O_RDONLY, FILE_PARMISSION);
                    			if (fd < 0)
                    				break;
                    			sceIoClose(fd);
                    		}
							sprintf(buf2, "%s に書き出し中", newPath);
							pgPrintMenuBar(buf2);
							pgCopyMenuBar();
							flipScreen(0);
                        	fd = sceIoOpen(newPath, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
                        	if(fd < 0)
                        	{
                        		psp2chNormalError(FILE_OPEN_ERR, newPath);
                        		return 0;
                        	}
                        	psp2chFileWrite(fd, buf, strlen(buf));
                        	sceIoClose(fd);
							psp2chErrorDialog(0, "%s\nに保存しました", newPath);
                        	return 0;
	                    case 1:
							if (createDir(path)<0){				// フォルダが無いなら作成する
								pgErrorDialog(0, path);
								return -1;
							}
	                        psp2chMenuPlayer(bar, path, buf, length);
	                        return 0;
	                    case 2:
	                    	return 1;
						case 3:
							psp2chMenuBrowserURL(bar, buf, length);
							return 0;
                    }
					change = 1;
                }
                else if(s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
    return 0;
}

/****************************************
システム用メニュー
****************************************/
#define MENU_SYSTEM_WIDTH (120)
#define MENU_SYSTEM_ITEM (9)
#define MENU_SYSTEM_HEIGHT (MENU_SYSTEM_ITEM * LINE_PITCH)
static void psp2chMenuSystem(S_SCROLLBAR* bar)
{
	static S_2CH_SCREEN menu;
    const char* menuList[] = {"状態確認", "クロック変更", "パレット", "過去ログ表\示", "メモリテスト", "mp3再生モード", "自動更新", "自動ページ送り", "idx診断"};
	const char bgmList[][27] = {"１曲ループ", "フォルダ内ループ", "サブフォルダも含めてループ", "指定フォルダ以下ループ"};
    char* menuStr = s2ch.menuWin[winParam->tateFlag].main;
    int lineEnd, change = 1;
    int startX, startY, scrX, scrY;
    char *test[16], msg[256];
	int i, j, cpu, bus, bgm, size[16];

    startX = (winParam->width - MENU_SYSTEM_WIDTH) / 2;
    startY = (winParam->height - MENU_SYSTEM_HEIGHT) / 2;
    scrX = MENU_SYSTEM_WIDTH;
    scrY = MENU_SYSTEM_HEIGHT;
    lineEnd = MENU_SYSTEM_ITEM;
    menu.count = MENU_SYSTEM_ITEM;
    
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
	            psp2chDrawMenu((char**)menuList, menu, startX, startY, scrX, scrY);
	            pgPrintMenuBar(menuStr);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
					pgScrollbar(bar, s2ch.resBarColor);			// スクロールバーを作画
                    switch (menu.select)
                    {
                    case 0: // 状態確認
                    	psp2chErrorDialog(0, 
                    		"Free Memory: %d B\nMax Free Block: %d B\nFree Stack Size: %d B\nCpu Clock: %d\n Bus Clock: %d", 
                    		sceKernelTotalFreeMemSize(), 
                    		sceKernelMaxFreeMemSize(),
                    		sceKernelGetThreadStackFreeSize(0),
                    		scePowerGetCpuClockFrequency(),
                    		scePowerGetBusClockFrequency());
                        break;
                    case 1: // クロック
	                    	switch(scePowerGetCpuClockFrequency()) {
	                    		case 333:
	                    			cpu = 133; bus = 66;
	                    			break;
	                    		case 132:
	                    			cpu = 222; bus = 111;
	                    			break;
	                    		case 222:
	                    			cpu = 333; bus = 166;
	                    			break;
	                    	}
	                    	scePowerSetClockFrequency(cpu, cpu, bus);
                        break;
                    case 2: // パレット
                    	dispPalette = !dispPalette;
                        break;
                    case 3: // 過去ログ表示
                    	s2ch.cfg.logDisplay = !s2ch.cfg.logDisplay;
                    	psp2chErrorDialog(0, "log display mode %d (1 display)", s2ch.cfg.logDisplay);
                        break;
                    case 4:
                    	for (i = 0; i < 16; i++) {
							test[i] = NULL;
							size[i] = 0;
						}
						for (i = 0; i < 16; i++) {
							for (j = 50*1024; j > 0; j-=8) {
								test[i] = (char*)malloc(sizeof(char) * 1024 * j);
								if (test[i]) {
									size[i] = j;
									break;
								}
							}
						}
						strcpy( msg, "フリーメモリブロックサイズ\n\n");
						for (i = 0; i < 16; i++) {
							if (size[i] != 0){
								sprintf( msg, "%s%d KB\n", msg, size[i]);
							}
						}
						for (i = 15; i >= 0; i--) {
							free(test[i]);
						}
						psp2chErrorDialog(0, msg);
						break;
                    case 5:										// BGMループ設定
						bgm = (s2ch.cfg.bgmLoop +1) % 4;
						if (psp2chErrorDialog(0, "%s に設定します\n（現在:%s）", bgmList[bgm], bgmList[s2ch.cfg.bgmLoop]) == PSP_UTILITY_MSGDIALOG_RESULT_YES)
						{
							s2ch.cfg.bgmLoop = bgm;
							if (bgm == 0){
								psp2chAudioSetLoop(-1);
							} else {
								psp2chAudioSetLoop(0);
							}
						}
                    	break;
                    case 6:
                    	autoUpdate = !autoUpdate;
                    	psp2chErrorDialog(0, "set autoupdate %d", autoUpdate);
                    	break;
                    case 7:
                    	autoScroll = !autoScroll;
                    	psp2chErrorDialog(0, "set autoScroll %d", autoScroll);
                    	break;
                    case 8:
                    	if (s2ch.sel == 3)
                    		s2ch.cfg.logDisplay = 2;
                    	break;
                    }
					change = 1;
                }
                else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
}

/****************
サブメニュー選択ウィンドウ
****************/
#define SUBMENU_WIDTH (100)
#define SUBMENU_ITEM (9)
#define SUBMENU_HEIGHT (SUBMENU_ITEM * LINE_PITCH)
int psp2chSubMenu(S_SCROLLBAR* bar, int numMenu, char *host, char *dir, int dat, char *title)
{
	static S_2CH_SCREEN menu;
	char* menuStr = s2ch.menuWin[winParam->tateFlag].main;
    const char* menuList[] = {"コピー", "引用コピー", "内容コピー", "スレ情報をコピー", "パレットへコピー", "過去ログ検索", "類似スレ検索", "100下に移動", "100上に移動"};
    int lineEnd, temp = 0, change = 1, loop = 1, ret = 0;
    int startX, startY, scrX, scrY;
    SceUID fd;
    char path[FILE_PATH], buf[BUF_LENGTH*2], buf2[BUF_LENGTH], urlbuf[BUF_LENGTH * 3], *p;

    startX = (winParam->width - SUBMENU_WIDTH) / 2;
    startY = (winParam->height - SUBMENU_HEIGHT) / 2;
    scrX = SUBMENU_WIDTH;
    scrY = SUBMENU_HEIGHT;
    lineEnd = SUBMENU_ITEM;
    menu.count = SUBMENU_ITEM;
    
    if (winParam->tateFlag)
    	temp = 1;
    pgCreateTexture();
    if (temp)
    	psp2chSetScreenParam(1);
    
    while (loop)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if (change)
			{
				psp2chDrawMenu((char**)menuList, menu, startX, startY, scrX, scrY);
				pgPrintMenuBar(menuStr);
			}
            psp2chCursorSet(&menu, lineEnd, 0, 0, &change);
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].ok)
                {
                    switch (menu.select)
                    {
                	case 0:
                    	psp2chResCopy(numMenu, 0, 1);
                    	break;
                    case 1:
                        psp2chResCopy(numMenu, 1, 1);
                        break;
                    case 2:
                    	psp2chResCopy(numMenu, 0, 0);
                    	break;
                    case 3:
                    	strcpy(buf2,dir);
                    	p = strchr(buf2,'/');
                    	if (p){									// '/'が見つかったなら
							*p = '-';							// '/'を'-'へ変換（したらば掲示板への対応）
                    	}
                    	if (strstr(host, ".machi.to")) {		// まちBBSの場合
							sprintf(buf, "[%s]%s\nhttp://%s/bbs/read.cgi/%s/%d\n", title, s2ch.resList[0].title, host, dir, dat);
						} else {								// その他
                    		sprintf(buf, "[%s]%s\nhttp://%s/test/read.cgi/%s/%d\n", title, s2ch.resList[0].title, host, dir, dat);
                    	}
						sprintf(path, "%s/%s-%d.txt", s2ch.cfg.templateDir, buf2, dat);
						sprintf(buf2, "%s に書き出し中", path);
						pgPrintMenuBar(buf2);
						pgCopyMenuBar();
						flipScreen(0);
						if ((fd = sceIoOpen("ms0:/PSP/COMMON/CLIPBOARD.TXT", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION)) >= 0){
							sceIoWrite(fd, buf, strlen(buf));
							sceIoClose(fd);
						}
                        fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
                    	if (fd){
                    		sceIoWrite(fd, buf, strlen(buf));
                    		sceIoClose(fd);
							psp2chErrorDialog(0, "%s\nに保存しました", path);
                    	} else {
							psp2chErrorDialog(0, "%s\nへの保存に失敗しました", path);
						}
                        break;
                    case 4:
                        psp2chSetPalette("%d [%s] %s ID:%s", 
                        	dat,
							s2ch.resList[s2ch.numAnchor[numMenu].num].mail,
							s2ch.resList[s2ch.numAnchor[numMenu].num].date,
							s2ch.resList[s2ch.numAnchor[numMenu].num].id);
                        break;
                    case 5:
						sprintf(buf, "http://%s/test/read.cgi/%s/%d/", host, dir, dat);
						psp2chUrlEncode(urlbuf, buf);
						strcat(urlbuf, "&btnG=%95%CF%8A%B7");
						sprintf(path, "http://yellow.ribbon.to/~mirror/url.php?url=%s", urlbuf);
						pspShowBrowser(path, NULL);
						break;
					case 6:
						if (s2ch.threadList != NULL)
						{
							searchResult *threadComp;
							int i, j, len;
							
							threadComp = malloc(sizeof(searchResult) * s2ch.thread.count);
							if (!threadComp)
								break;
							memset(threadComp, 0, sizeof(searchResult) * s2ch.thread.count);
							len = strlen(s2ch.resList[0].title);
							for (j = 0; j < s2ch.thread.count; j++)
							{
								threadComp[j].dat = s2ch.threadList[j].id;
								for (i = 0; i < len; i++)
								{
									if (strchr(s2ch.threadList[j].title, s2ch.resList[0].title[i]))
										threadComp[j].match++;
								}
							}
							qsort(threadComp, s2ch.thread.count, sizeof(searchResult), myCompare);
							for (i = 0; i < s2ch.thread.count; i++)
							{
								threadSort[i] = threadComp[i].dat;
							}
							free(threadComp);
							ret = -1;
							loop = 0;
						}
						continue;
					case 7:
						ret = -2;
						loop = 0;
						continue;
					case 8:
						ret = -3;
						loop = 0;
						continue;
                    }
					change = 1;
                }
                else if (s2ch.pad.Buttons & s2ch.menuWin[winParam->tateFlag].esc)
                {
                    break;
                }
            }
			
			pgDrawTexture(1);
			pgCopyWindow(0, startX, startY, scrX, scrY);
			pgWindowFrame(startX, startY, startX + scrX, startY + scrY);
			pgScrollbar(bar, s2ch.resBarColor);					// スクロールバーを作画
			pgCopyMenuBar();
			flipScreen(0);
        }
    }
    pgDeleteTexture();
    return ret;
}

static int myCompare(const void *p1, const void *p2)
{
	const searchResult a = *((searchResult*)p1);
	const searchResult b = *((searchResult*)p2);
	if (a.match > b.match)
		return -1;
	else if (a.match == b.match)
		return 0;
	else
		return 1;
}


//==============================================================
// ZIPファイル解凍
//--------------------------------------------------------------
// 戻り値：1     展開されていない（destpathフォルダは生成されていない）
//         0     正常終了
//         0以下 エラー発生（destpathフォルダに一部展開されている）
//--------------------------------------------------------------
// fileで指定されたzipファイルをdestpathフォルダへ展開する。
// 展開先を変更した場合は変更後のパスがdestpathに返される。
//--------------------------------------------------------------

int psp2chMenuUnzip(char *file, char *destpath)
{
	char	*p,msg[256];
	int		ret;

	if (psp2chInputDialog("展開先フォルダを入力してください", "展開先フォルダ", destpath)) return 1;
	if (strlen(keyWords)==0) return 1;
	strcpy(destpath, keyWords);
	p = strrchr(destpath, '.');
	if (p) *p = '\0';											// 拡張子は削除
	if (psp2chInputDialog("必要な場合はパスワードを入力してください", "パスワード", NULL)) return 1;

	strcpy(msg, file);
	strcat(msg, "を展開中");
	pgPrintMenuBar(msg);
	pgDrawTexture(-1);
	pgCopyMenuBar();
	flipScreen(0);

	ret = unzipToDir(file, destpath, keyWords, msg);			// アーカイブ展開

	if (ret){													// エラーが発生したら
		char	buf[256];
		sprintf(buf, "%sの展開で問題が起こりました\n\n", file);
		strcat(buf, msg);
		psp2chErrorDialog(0, buf);
	}
	return ret;
}

//==============================================================
// RARファイル解凍
//--------------------------------------------------------------
// 戻り値：1     展開されていない（destpathフォルダは生成されていない）
//         0     正常終了
//         0以下 エラー発生（destpathフォルダに一部展開されている）
//--------------------------------------------------------------
// fileで指定されたrarファイルをdestpathフォルダへ展開する。
// 展開先を変更した場合は変更後のパスがdestpathに返される。
//--------------------------------------------------------------

int psp2chMenuUnRAR(char *file, char *destpath)
{
	char	*p, msg[256], extDir[256];
	char	**argv, **currentString;
	int		i, ret, argc, count;
	unsigned short	ucs[256];

	if (psp2chInputDialog("展開先フォルダを入力してください", "展開先フォルダ", destpath)) return 1;
	if (strlen(keyWords)==0) return 1;
	strcpy(destpath, keyWords);
	p = strrchr(destpath, '.');
	if (p) *p = '\0';											// 拡張子は削除
	if (psp2chInputDialog("必要な場合はパスワードを入力してください", "パスワード", NULL)) return 1;

	if (s2ch.cfg.hbl){											// HBL環境下ではファイル名のシフトJIS→UTF8変換を行う
		psp2chSJIS2UCS(ucs, destpath, 256);
		psp2chUCS2UTF8(extDir, ucs);
	} else {
		strcpy(extDir, destpath);
	}
	argv = calloc( 8, sizeof(char *) );
	currentString = argv;
	if (strlen(keyWords)==0){									// パスワードが指定されてない
		argc = 5;
		count = argc+1;
		for ( i=0; i<count; i++ ){
			if (i==0) asprintf( currentString, "unrar");
			if (i==1) asprintf( currentString, "x");
			if (i==2) asprintf( currentString, "%s", file);
			if (i==3) asprintf( currentString, "%s", extDir);
			if (i==4) asprintf( currentString, "-y");
			if (i> 4) asprintf( currentString, " ");
			currentString++;
		}

	}else{														// パスワードが指定された
		argc = 6;
		count = argc+1;
		for ( i=0; i<count; i++ ){
			if (i==0) asprintf( currentString, "unrar");
			if (i==1) asprintf( currentString, "x");
			if (i==2) asprintf( currentString, "%s", file);
			if (i==3) asprintf( currentString, "%s", extDir);
			if (i==4) asprintf( currentString, "-y");
			if (i==5) asprintf( currentString, "-p%s", keyWords);
			if (i> 5) asprintf( currentString, " ");
			currentString++;
		}
	}

	if (createDir(extDir)<0){
		psp2chErrorDialog(0, "%s\nフォルダの作成に失敗しました", extDir);
		return 1;
	}
	ret = mainRAR(argc,argv);									// アーカイブ展開
	if (ret){													// エラーが発生したら
		char	buf[256];
		sprintf(buf, "%sの展開で問題が起こりました\n\n", file);
//		strcat(buf, msg);
		psp2chErrorDialog(0, buf);
	}

	currentString = argv;
	for ( i = 0; i < count; i++ ){
		free( *currentString );
		currentString++;
	}
	free(argv);
	return ret;
}
