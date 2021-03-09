/*
* $Id: psp2chRes.c 152 2008-09-10 05:53:53Z bird_may_nike $
*/

#include "psp2ch.h"
#include <psputility.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pg.h"
#include "psp2chNet.h"
#include "psp2chIta.h"
#include "psp2chThread.h"
#include "psp2chRes.h"
#include "psp2chResWindow.h"
#include "psp2chFavorite.h"
#include "psp2chForm.h"
#include "psp2chMenu.h"
#include "psphtmlviewer.h"
#include "utf8.h"
#include "charConv.h" // EUCtoSJIS
#include "oniguruma.h"
#include "psp2chReg.h"

enum RES_FORM {TITLE, TEXT, DATE, MAIL, NAME, NUM, TOTAL};
//#define ID " ID:"
#define BE " BE:"
#define BOARD_FILE_EXT "my.brd"									// psp2chIta.cにも同じ指定あり
static const char *broken = "broken";

typedef struct {
	void *target;
	unsigned char *str;
	OnigRegion *region;
} reg_data;

extern S_2CH s2ch; // psp2ch.c
extern int* favSort; // psp2chFavorite.c
extern int* threadSort; // psp2chThread.c
extern Window_Layer *winParam;

extern const char* ngNameFile; // psp2chMenu.c
extern const char* ngIDFile; // psp2chMenu.c
extern const char* ngWordFile; // psp2chMenu.c
extern const char* ngMailFile; // psp2chMenu.c

extern char keyWords[128]; //psp2ch.c

int cursorMode = 1; // change
int wide = 0; // change
int preLine = -2;
int autoUpdate = 0;
int autoScroll = 0;
int gCPos = 3;													// カーソルジャンプモード

S_2CH_RES_MENU_STR menuRes[2];
static char* resBuffer = NULL;
static int gResSize = 0;
static char jmpHost[NET_HOST_LENGTH], jmpDir[NET_PATH_LENGTH], jmpTitle[ITA_NAME_LENGTH];
static unsigned long jmpDat;

// prototype
static int psp2chRes(char* host, char* dir, char* title, unsigned long dat, int ret);
static void psp2chResSend(char* host, char* dir, char* title, unsigned long dat, int *totalLine, S_SCROLLBAR *bar, int numMenu);
void psp2chBeInfo(int anc, char* host, char* dir, unsigned long dat);
static int psp2chResJump(int urlMenu);
static int psp2chResSetLine(S_SCROLLBAR* bar);
static void psp2chResSetAnchorList(S_2CH_SCREEN *screen, int lineEnd, int cursorPosition);
static void psp2chResLineSet(int* startRes, int* startLine);
static void psp2chResLineGet(int startRes, int startLine);
static int psp2chDrawResStr(char* str, S_2CH_RES_COLOR c, int line, int lineEnd, int startX, int endX, int *drawLine);
static void psp2chDrawRes(int line);
static int res_name_callback(const UChar* name, const UChar* name_end, int ngroup_num, int* group_nums, regex_t* reg, void* arg);
static int psp2chResParser(unsigned char *str, const unsigned char *pattern);
static int psp2cGetThreadTitle(char *title, const char *host, char *buf);
static int psp2chMakeIdx(const char *host, const char *title, const unsigned long dat);

int createDir (char *newdir);									// psp2chMenu.c

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

void psp2chResSetMenuString(const char **sBtnH, const char **sBtnV)
{
	int index1, index2, index3, index4, index5, index6;
	int i, tmp;

	getIndex(s2ch.btnRes[0].form, index1);
	getIndex(s2ch.btnRes[0].back, index2);
	getIndex(s2ch.btnRes[0].reload, index3);
	getIndex(s2ch.btnRes[0].datDel, index4);
	getIndex(s2ch.btnRes[0].change, index5);
	sprintf(menuRes[0].main, "　%s : 書き込み　　%s : 戻る　　%s : 更新　　%s : 削除　　%s : メニュー切替　　Ｌ : 検索次へ",
			sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5]);

	getIndex(s2ch.btnRes[0].s.top, index1);
	getIndex(s2ch.btnRes[0].s.end, index2);
	getIndex(s2ch.btnRes[0].addFav, index3);
	getIndex(s2ch.btnRes[0].delFav, index4);
	getIndex(s2ch.btnRes[0].cursor, index5);
	getIndex(s2ch.btnRes[0].wide, index6);
	sprintf(menuRes[0].sub1, " %s : 先頭　%s : 最後　%s : お気に入り登録　%s : カーソ\ル(%%d)　%s : ワイド(%%d)　Ｌ : 検索",
			sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index5], sBtnH[index6]);
	sprintf(menuRes[0].sub2, " %s : 先頭　%s : 最後　%s : お気に入り削除　%s : カーソ\ル(%%d)　%s : ワイド(%%d)　Ｌ : 検索",
			sBtnH[index1], sBtnH[index2], sBtnH[index4], sBtnH[index5], sBtnH[index6]);

	getIndex(s2ch.btnRes[0].resForm, index1);
	getIndex(s2ch.btnRes[0].resFBack, index2);
	getIndex(s2ch.btnRes[0].resFRes, index3);
	sprintf(menuRes[0].aNum, "　%s : レスをする　　　%s : 戻る　　　△ : コピーメニュー　　　%s : レス元抽出",
			sBtnH[index1], sBtnH[index2], sBtnH[index3]);

	getIndex(s2ch.btnRes[0].idView, index1);
	getIndex(s2ch.btnRes[0].idNG, index2);
	getIndex(s2ch.btnRes[0].idBack, index3);
	sprintf(menuRes[0].aId, "　%s : ID抽出　　　%s : NGID登録　　　%s : 戻る",
			sBtnH[index1], sBtnH[index2], sBtnH[index3]);

	getIndex(s2ch.btnRes[0].idBack, index1);
	sprintf(menuRes[0].aBe, "　○ : Be抽出　　　□ : Beプロフィール(%%s)　　　%s : 戻る",
			sBtnH[index1]);

	getIndex(s2ch.btnRes[0].resView, index1);
	getIndex(s2ch.btnRes[0].resMove, index2);
	getIndex(s2ch.btnRes[0].resBack, index3);
	sprintf(menuRes[0].aRes, "　%s : レス表\示　　　%s : レスに移動　　　%s : 戻る",
			sBtnH[index1], sBtnH[index2], sBtnH[index3]);

	getIndex(s2ch.btnRes[0].url, index1);
	getIndex(s2ch.btnRes[0].urlHtml, index2);
	getIndex(s2ch.btnRes[0].urlBack, index3);
	getIndex(s2ch.btnRes[0].urlDown, index4);
	sprintf(menuRes[0].aUrl, "　%s : リンク表\示　　　%s : ブラウザ表\示　　　%s : 戻る　　　%s : ダウンロード",
			sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4]);

	getIndex(s2ch.btnRes[1].form, index1);
	getIndex(s2ch.btnRes[1].back, index2);
	getIndex(s2ch.btnRes[1].reload, index3);
	getIndex(s2ch.btnRes[1].datDel, index4);
	getIndex(s2ch.btnRes[1].change, index5);
	sprintf(menuRes[1].main, "　%s : 書き込み　　　%s : 戻る　　　　%s : 更新\n　%s : 削除　　%s : メニュー切替　　Ｌ : 検索次へ",
			sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5]);

	getIndex(s2ch.btnRes[1].s.top, index1);
	getIndex(s2ch.btnRes[1].s.end, index2);
	getIndex(s2ch.btnRes[1].addFav, index3);
	getIndex(s2ch.btnRes[1].delFav, index4);
	getIndex(s2ch.btnRes[1].cursor, index5);
	getIndex(s2ch.btnRes[1].wide, index6);
	sprintf(menuRes[1].sub1, " %s : 先頭　%s : 最後　%s : お気に入り登録\n %s : カーソ\ル(%%d)　%s : ワイド(%%d)　Ｌ : 検索",
			sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index5], sBtnV[index6]);
	sprintf(menuRes[1].sub2, " %s : 先頭　%s : 最後　%s : お気に入り削除\n %s : カーソ\ル(%%d)　%s : ワイド(%%d)　Ｌ : 検索",
			sBtnV[index1], sBtnV[index2], sBtnV[index4], sBtnV[index5], sBtnV[index6]);

	getIndex(s2ch.btnRes[1].resForm, index1);
	getIndex(s2ch.btnRes[1].resFBack, index2);
	getIndex(s2ch.btnRes[1].resFRes, index3);
	sprintf(menuRes[1].aNum, "　%s : レスをする　　　%s : 戻る　　　△ : コピーメニュー　%s : レス元抽出",
			sBtnV[index1], sBtnV[index2], sBtnV[index2]);

	getIndex(s2ch.btnRes[1].idView, index1);
	getIndex(s2ch.btnRes[1].idNG, index2);
	getIndex(s2ch.btnRes[1].idBack, index3);
	sprintf(menuRes[1].aId, "　%s : ID抽出　　　%s : NGID登録　　　%s : 戻る",
			sBtnV[index1], sBtnV[index2], sBtnV[index3]);

	getIndex(s2ch.btnRes[1].idBack, index1);
	sprintf(menuRes[1].aBe, "　○ : Be抽出　　　□ : Beプロフィール(%%s)　　　%s : 戻る",
			sBtnV[index1]);

	getIndex(s2ch.btnRes[1].resView, index1);
	getIndex(s2ch.btnRes[1].resMove, index2);
	getIndex(s2ch.btnRes[1].resBack, index3);
	sprintf(menuRes[1].aRes, "　%s : レス表\示　　　%s : レスに移動　　　%s : 戻る",
			sBtnV[index1], sBtnV[index2], sBtnV[index3]);

	getIndex(s2ch.btnRes[1].url, index1);
	getIndex(s2ch.btnRes[1].urlHtml, index2);
	getIndex(s2ch.btnRes[1].urlBack, index3);
	getIndex(s2ch.btnRes[1].urlDown, index4);
	sprintf(menuRes[1].aUrl, "　%s : リンク表\示　　　%s : ブラウザ表\示　　　%s : 戻る　　　%s : ダウンロード",
			sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4]);
}

//==============================================================
// シフトJISの第１文字チェック
//--------------------------------------------------------------
// 戻り値  0:第１文字ではない
//        -1:第１文字である
//--------------------------------------------------------------

static int checkSJIS(unsigned char cr)
{
	if (cr<0x80U || (cr>=0xA0U && cr<0xE0U) || cr>=0xFDU){
		return (0);
	} else {
		return (-1);
	}
}


//==============================================================
// ログのディレクトリパスを生成
//--------------------------------------------------------------
// s2ch.cfg.hblの指定に基づきディレクトリパスを生成する。
// したらば掲示板の場合、ディレクトリ名称に'/'が含まれてしまうためこれを'-'に置き換える。
// ファイル名として使えない文字があったら'_'に置き換える。
//--------------------------------------------------------------

static void psp2chResLogPath(char *path, char *dir, char *title)
{
	int		i, pos, zenn;
	char	buf[2] = {0,0};

	strcpy( path, s2ch.cfg.logDir );
	strcat( path, "/" );
	if (s2ch.cfg.hbl){
		for (i=0; i<strlen(dir) ;i++) {
			*buf = dir[i];
			if (*buf == '/') *buf = '-';
			strcat( path, buf );
		}
	} else {
		zenn = 0;
		for (i=0; i<strlen(title); i++) {
			*buf = title[i];
			if (zenn) {										// 全角文字２文字目
				zenn = 0;
			} else {
				if (checkSJIS((unsigned)*buf)) {			// 全角文字１文字目なら
					zenn = 1;
				} else {
					if (strchr("/:*?\"<>|", *buf)) *buf = '_';
				}
			}
			strcat( path, buf );
		}
	}
}

/**************
  レス表示
***************/
#define setMenuStr(S) (menuStr = menuRes[winParam->tateFlag].S)

int psp2chFavoriteRes(int ret)
{
	return psp2chRes(s2ch.favList[favSort[s2ch.fav.select]].host, s2ch.favList[favSort[s2ch.fav.select]].dir, s2ch.favList[favSort[s2ch.fav.select]].title, s2ch.favList[favSort[s2ch.fav.select]].dat, ret);
}

int psp2chThreadRes(int ret)
{
	return psp2chRes(s2ch.itaList[s2ch.ita.select].host, s2ch.itaList[s2ch.ita.select].dir,s2ch.itaList[s2ch.ita.select].title,s2ch.threadList[threadSort[s2ch.thread.select]].dat,ret);
}

int psp2chJumpRes(int ret)
{
	return psp2chRes(jmpHost, jmpDir, jmpTitle, jmpDat, ret);
}

int psp2chSearchRes(int ret)
{
	return psp2chRes(s2ch.findList[s2ch.find.select].host, s2ch.findList[s2ch.find.select].dir, s2ch.findList[s2ch.find.select].title, s2ch.findList[s2ch.find.select].dat, ret);
}

static int psp2chRes(char* host, char* dir, char* title, unsigned long dat, int ret)
{
	static int cursorX = 240, cursorY = 130, totalLine = 0;
	static S_SCROLLBAR bar;
	static int resMenu = -1, urlMenu = -1, idMenu = -1, beMenu = -1, numMenu = -1;
	char* menuStr = menuRes[winParam->tateFlag].main;
	char path[FILE_PATH];
	int i, j, rMenu, ret2;

	if (s2ch.resList == NULL)
	{
		if ((ret2 = psp2chResList(host, dir, title, dat)) < 0)
		{
			winParam->viewX = winParam->viewY = 0;
			if (ret2 == -2){
				s2ch.sel = -1;									// 板が移転していた場合
			} else {
				s2ch.sel = ret;
			}
			return -1;
		}
		totalLine = psp2chResSetLine(&bar);
		winParam->viewX = winParam->viewY = 0;
		preLine = -2;
		s2ch.oldPad = s2ch.pad;
		cursorX = 480;
		cursorY = 480;											// スレを開いた時のカーソル初期位置は右下（実際の補正はpsp2chResCursorMove()で）
	}
	rMenu = psp2chResCursorMove(&s2ch.res, totalLine, winParam->lineEnd, &cursorX, &cursorY, bar.x, bar.view);
	psp2chResPadMove(&cursorX, &cursorY, bar.x, bar.view);
	// ポインタチェック
	i = j = 0;
	numMenu = idMenu = beMenu = resMenu = urlMenu = -1;
	while (s2ch.anchorList[i].line >= 0)
	{
		if (cursorY / LINE_PITCH + s2ch.res.start == s2ch.anchorList[i].line &&
			cursorX > s2ch.anchorList[i].x1 && cursorX < s2ch.anchorList[i].x2)
		{
			switch(s2ch.anchorList[i].type)
			{
			case 0:
				// レス番メニュー
				numMenu = s2ch.anchorList[i].id;
				setMenuStr(aNum);
				break;
			case 1:
				// IDメニュー
				idMenu = s2ch.anchorList[i].id;
				setMenuStr(aId);
				break;
			case 2:
				// レスアンカーメニュー
				resMenu = s2ch.anchorList[i].id;
				setMenuStr(aRes);
				break;
			case 3:
				// URLアンカーメニュー
				urlMenu = s2ch.anchorList[i].id;
				setMenuStr(aUrl);
				break;
			case 4:
				// BEアンカーメニュー
				beMenu = s2ch.anchorList[i].id;
				sprintf(path, menuRes[winParam->tateFlag].aBe, s2ch.beAnchor[beMenu].be);
				menuStr = path;
				break;
			}
			j = 1;
			break;
		}
		i++;
	}
	if (autoUpdate)
	{
		static int upflag = 0;
		time_t old;
		sceKernelLibcTime(&old);
		old %= s2ch.cfg.updateTime;								// 自動更新時間（秒）
		psp2chSetPalette("%d", old);
		if (old == 0) // 3分単位
		{
			if (upflag)
			{
				int rt;
				psp2chResLineSet(&i, &j);
				rt = psp2chGetDat(host, dir, title, dat);
				if (rt == 0)
				{
					psp2chResList(host, dir, title, dat);
					totalLine = psp2chResSetLine(&bar);
					psp2chResLineGet(i, j);
					s2ch.res.start++;
				}
				else if (rt == -3)
					autoUpdate = 0;
				upflag = 0;
			}
		}
		else
			upflag = 1;
	}
	if (autoScroll)
	{
		static int scrollflag = 0;
		time_t old2;
		sceKernelLibcTime(&old2);
		old2 %= s2ch.cfg.rollTime;								// 自動スクロール時間（秒）
		psp2chSetPalette("%d", old2);
		if (old2 == 0)
		{
			if (scrollflag)
			{
				s2ch.res.start += winParam->lineEnd;
				scrollflag = 0;
			}
		}
		else
			scrollflag = 1;
	}
	// ポインタメニューでないとき
	if (j == 0)
	{
		// シフトメニュー
		if (rMenu)
		{
			// お気に入りリストにあるかチェック
			j = 0;
			if (s2ch.fav.count)
			{
				for (i = 0; i < s2ch.fav.count; i++)
				{
					if (s2ch.favList[i].dat == dat && strcmp(s2ch.favList[i].title, title) == 0)
					{
						j = 1;
						break;
					}
				}
			}
			// お気に入りにある
			if (j)
			{
				sprintf(path, menuRes[winParam->tateFlag].sub2, cursorMode, wide);
			}
			// お気に入りにない
			else
			{
				sprintf(path, menuRes[winParam->tateFlag].sub1, cursorMode, wide);
			}
			menuStr = path;
		}
		// 通常メニュー
		else
		{
			setMenuStr(main);
		}
	}
	if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
	{
		// SELECTボタン 縦横切り替え
		if (s2ch.pad.Buttons & PSP_CTRL_SELECT)
		{
			psp2chResLineSet(&i, &j);
			psp2chSetScreenParam(1);
			totalLine = psp2chResSetLine(&bar);
			psp2chResLineGet(i, j);
			psp2chResResetAnchors();
			
			preLine = -2;
		}
		// STARTボタン メニューウィンドー表示
		else if(s2ch.pad.Buttons & PSP_CTRL_START)
		{
			pgScrollbar(&bar, s2ch.resBarColor);
			psp2chResLineSet(&i, &j);
			psp2chMenu(&bar);
			totalLine = psp2chResSetLine(&bar);
			psp2chResLineGet(i, j);
			preLine = -2;
		}
		// レスアンカーメニュー
		else if (resMenu >= 0)
		{
			// アンカーレス表示
			if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].resView)
			{
				psp2chResAnchor(&s2ch.resAnchor[resMenu]);
			}
			// アンカーレス番に移動
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].resMove)
			{
				if (s2ch.resList[s2ch.resAnchor[resMenu].res[0]].ng == 0 || s2ch.cfg.abon)
				{
					for (i = 0, j = 0; i < s2ch.resAnchor[resMenu].res[0]; i++)
					{
						if (s2ch.resList[i].ng == 0)
						{
							j += s2ch.resList[i].line;
							j++;
						} else if (s2ch.cfg.abon){
							j += 2;								// NGの場合でも２行は表示する
						}
					}
					s2ch.res.start = j;
				}
			}
			// 戻る
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].resBack)
			{
				cursorX = bar.x;
				cursorY = bar.view;
			}
		}
		// URLアンカーメニュー
		else if (urlMenu >= 0)
		{
			if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].urlHtml)
			{
				// 強制ブラウザ表示
				psp2chUrlAnchor(urlMenu, s2ch.res.start*LINE_PITCH, 0, dat);
			}
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].url)
			{
				ret2 = psp2chResJump(urlMenu);
				if (ret2 >= 1){									// urlで指定されたスレッドに移動
					if (s2ch.cfg.hbl){							// 現在の表示位置をidxファイルに保存
						psp2chWriteIdx(host, dir, dat, 0);
					} else {
						psp2chWriteIdx(host, title, dat, 0);
					}
					free(s2ch.resList);							// スレッドを変更するので
					s2ch.resList = NULL;
					s2ch.res.start = 0;
					if (ret2 == 2){								// スレッド一覧へ移行する場合
						return 2;								// 「2」は板一覧へ戻る、という意味
					} else {
						return ret;
					}
				} else if (ret2 != -1){							// urlをブラウザで開く
					psp2chUrlAnchor(urlMenu, s2ch.res.start*LINE_PITCH, 1, dat);
				}
			}
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].urlDown)
			{
				// 強制ダウンロード
				psp2chUrlAnchor(urlMenu, s2ch.res.start*LINE_PITCH, 2, dat);
			}
			// 戻る
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].urlBack)
			{
				cursorX = bar.x;
				cursorY = bar.view;
			}
			else if (s2ch.pad.Buttons & PSP_CTRL_LTRIGGER) {
				int i, res, line, nowLine, startLine, limitLine, total = 0;
				i = urlMenu;
				nowLine = s2ch.res.start;
				s2ch.res.start += cursorY / LINE_PITCH;
				psp2chResLineSet(&res, &line);
				psp2chResLineGet(res, 0);
				startLine = s2ch.res.start;
				limitLine = s2ch.res.start + s2ch.resList[res].line;
				while (startLine <= s2ch.urlAnchor[i].line && s2ch.urlAnchor[i].line <= limitLine && s2ch.urlAnchor[i].line != -1) {
					psp2chUrlAnchor(i, s2ch.res.start*LINE_PITCH, 1, dat);
					
					total++;
					if (total == 50) {
						break;
					}
					i++;
					if (i >= 50) {
						i = 0;
					}
				}
				s2ch.res.start = nowLine;
				preLine = -2;
			}
		}
		// IDメニュー
		else if (idMenu >= 0)
		{
			// ID 抽出
			if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].idView) {
				psp2chIdAnchor(idMenu);
			}
			// NGID 登録
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].idNG)
			{
				psp2chNGAdd(ngIDFile, s2ch.idAnchor[idMenu].id);
				psp2chResCheckNG();
				totalLine = psp2chResSetLine(&bar);
				preLine = -2;
			}
			// 戻る
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].idBack)
			{
				cursorX = bar.x;
				cursorY = bar.view;
			}
		}
		// BEメニュー
		else if (beMenu >= 0)
		{
			// BE抽出
			if (s2ch.pad.Buttons & PSP_CTRL_CIRCLE) {
				psp2chBeAnchor(beMenu);
			}
			// BE詳細
			else if (s2ch.pad.Buttons & PSP_CTRL_SQUARE) {
				psp2chBeInfo(beMenu, host, dir, dat);
			}
			// 戻る
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].idBack)
			{
				cursorX = bar.x;
				cursorY = bar.view;
			}
		}
		// レス番号メニュー
		else if (numMenu >= 0)
		{
			// 自動アンカーで書き込む
			if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].resForm)
			{
				psp2chResSend(host, dir, title, dat, &totalLine, &bar, numMenu);
			}
			// 戻る
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].resFBack)
			{
				cursorX = bar.x;
				cursorY = bar.view;
			}
			// コピーメニュー
			else if((s2ch.pad.Buttons & PSP_CTRL_TRIANGLE))
			{
				int mode;
				psp2chResLineSet(&i, &j);
				mode = psp2chSubMenu(&bar, numMenu, host, dir, dat, title);
				if (mode == -1)
				{
					winParam->viewX = winParam->viewY = 0;
					s2ch.sel = ret;
					return 0;
				}
				else if (mode == -2)
				{
					i += 100;
					if (i > s2ch.res.count)
						i = s2ch.res.count - 1;
				}
				else if (mode == -3)
				{
					i -= 100;
					if (i < 0)
						i = 0;
				}
				totalLine = psp2chResSetLine(&bar);
				psp2chResLineGet(i, j);
				preLine = -2;
			}
			// レス元抽出
			else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].resFRes)
			{
				psp2chAnchorSearch(s2ch.numAnchor[numMenu].num+1);
			}
		}
		// ノーマルメニュー
		else
		{
			if (rMenu)
			{
				// お気に入り追加
				if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].addFav)
				{
					psp2chAddFavorite(host, dir, title, dat);
				}
				// お気に入り削除
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].delFav)
				{
					psp2chDelFavorite(title, dat);
				}
				// カーソルモード切替
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].cursor)
				{
					cursorMode = (cursorMode) ? 0 : 1;
				}
				// 画面モード切替
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].wide)
				{
					psp2chResLineSet(&i, &j);
					winParam->viewX = 0;						// 横方向の画面位置をリセット
					wide = (wide) ? 0 : 1;
					totalLine = psp2chResSetLine(&bar);
					psp2chResLineGet(i, j);
					preLine = -2;
				}
				// add スレ内検索 検索ワードを設定
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].search)
				{
					const unsigned char text1[] = "検索文字列を入力してください";
					char* text2 = "検索文字列";
					psp2chInputDialog(text1, text2, NULL);
					preLine = -2;
				}
			}
			else
			{
				// 戻る
				if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].back)
				{
					autoUpdate = 0;
					if (s2ch.cfg.hbl){
						psp2chWriteIdx(host, dir, dat, 0);
					} else {
						psp2chWriteIdx(host, title, dat, 0);
					}
					if (s2ch.threadList)
					{
						psp2chSort(-1); // 前回のソート順で再ソート
					}
					winParam->viewX = winParam->viewY = 0;
					s2ch.sel = ret;
					return ret;
				}
				// 書き込み
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].form)
				{
					psp2chResSend(host, dir, title, dat, &totalLine, &bar, -1);
				}
				// 更新
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].reload)
				{
					int rt;
					autoUpdate = 0;
					psp2chResLineSet(&i, &j);
					rt = psp2chGetDat(host, dir, title, dat);
					if (rt == 0)
					{
						psp2chResList(host, dir, title, dat);
						totalLine = psp2chResSetLine(&bar);
						psp2chResLineGet(i, j);
					}
					// 移転
					else if (rt == -4)
					{
						winParam->viewX = winParam->viewY = 0;
						s2ch.sel = -1;
						return 0;
					}
					preLine = -2;
				}
				// DAT削除
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].datDel)
				{
					autoUpdate = 0;
					if (psp2chErrorDialog(2, TEXT_5) == PSP_UTILITY_MSGDIALOG_RESULT_YES)
					{
						char buf[20],path2[FILE_PATH];
						psp2chResLogPath( path, dir, title );
						sprintf( buf, "/%d", dat );
						strcat( path, buf );
						strcpy( path2, path );
						strcat( path2, ".dat" );
						sceIoRemove(path2);
						strcpy( path2, path );
						strcat( path2, ".idx" );
						sceIoRemove(path2);
						if (s2ch.threadList) {
							if (s2ch.threadList[threadSort[s2ch.thread.select]].dat == dat) {
								s2ch.threadList[threadSort[s2ch.thread.select]].old = 0;
								psp2chSort(-1);
							}
						}
						winParam->viewX = winParam->viewY = 0;
						s2ch.sel = ret;
						return 0;
					}
				}
				// add 検索
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].search)
				{
					if(strlen(keyWords) != 0)
					{
						int st_num, st_ln;
						psp2chResLineSet(&st_num, &st_ln);
						for (i = st_num + 1; i < s2ch.res.count; i++)
						{
							if (strstr(s2ch.resList[i].name, keyWords) != NULL)
								break;
							else if (strstr(s2ch.resList[i].mail, keyWords) != NULL)
								break;
							else if (strstr(s2ch.resList[i].text, keyWords) != NULL)
								break;
						}
						// 終端までに見つかれば移動
						if (i != s2ch.res.count)
							psp2chResLineGet(i, 0);
					}
				}
			}
		}
	}
	// 描画
	psp2chDrawRes(s2ch.res.start);
	pgPrintMenuBar(menuStr);
	// 横スクロール
	if (wide)
	{
		// cursorMode時は常に左右スクロール　それ以外はカーソルが画面端にあるときだけスクロール
		if (cursorMode || cursorX == 0 || cursorX == bar.x)
		{
			winParam->viewX = psp2chPadSet(winParam->viewX);
		}
	}
	winParam->viewY = s2ch.res.start * LINE_PITCH;
	pgCopy(winParam->viewX, winParam->viewY);
	bar.start = winParam->viewY;
	pgScrollbar(&bar, s2ch.resBarColor);
	pgCopyMenuBar();
	if (rMenu)
	{
		pgPrintTitleBar(title, s2ch.resList[0].title, gResSize);
		pgCopyTitleBar();
	}
	if (cursorMode && rMenu) {
		pgPadCursor(cursorX, cursorY, gCPos+1);
	} else {
		pgPadCursor(cursorX, cursorY, 0);
	}
	return ret;
}

/*****************************
書き込みボタン処理
*****************************/
static void psp2chResSend(char* host, char* dir, char* title, unsigned long dat, int *totalLine, S_SCROLLBAR *bar, int numMenu)
{
	static char* message = NULL;
	char path[BUF_LENGTH];

	if (message == NULL)
	{
		message = (char*)malloc(sizeof(char) * RES_MESSAGE_LENGTH);
		if (message == NULL)
		{
			psp2chNormalError(MEM_ALLC_ERR, "Form Message");
			return;
		}
		memset(message, '\0', RES_MESSAGE_LENGTH);
	}
	if (numMenu >= 0)
	{
		sprintf(path, ">>%d\n", s2ch.numAnchor[numMenu].num + 1);
		strcat(message, path);
	}
	// 書き込みがあったときのみ更新
	if (psp2chForm(host, dir, title, dat, s2ch.resList[0].title, message, bar) > 0)
	{
		if (s2ch.cfg.hbl){
			psp2chWriteIdx(host, dir, dat, 0);
		} else {
			psp2chWriteIdx(host, title, dat, 0);
		}
		sceKernelDelayThread(DISPLAY_WAIT);
		psp2chGetDat(host, dir, title, dat);
		psp2chResList(host, dir, title, dat);
		s2ch.res.start++;
		*totalLine = psp2chResSetLine(bar);
	}
	preLine = -2;
}

//==============================================================
// MY.BRDへ掲示板を登録する
//--------------------------------------------------------------
// jmpHost, jmpDir, jmpDat, jmpTitleで指定されたBBSを MY.BRD に登録する。
// BBS名称等は dat/bbs.dat より取得し、板名称は各サーバーの bbsmenu.html（bbs.datに記載）
// より取得する。
// bbs.datフォーマット例
//   .nicovideo.jp[tab]ニコニコ動画[tab]bbsmenu.html
//   yy12.kakiko.com[tab]2ch緊急避難所
//--------------------------------------------------------------

static int psp2chResBBS(void)
{
	const char	mati1[15][10] = {"tawara","hokkaidou","touhoku","kousinetu","kanto","tokyo","tama","kana",
								 "toukai","kinki","osaka","cyugoku","sikoku","kyusyu","okinawa"},
				mati2[15][19] = {"会議室","北海道掲示板","東北掲示板","北陸甲信越掲示板","関東掲示板","東京23区掲示板","東京多摩地区掲示板",
								 "神奈川掲示板","東海掲示板","近畿掲示板","大阪掲示板","中国掲示板","四国掲示板","九州掲示板","沖縄掲示板"};
	char		url[FILE_PATH],path[FILE_PATH],bbsName[128],itaName[128],jmpDirSub[128],tmp[128],*p,*p1,*p2,*data;
	int			i,ret,dataSize;
	SceIoStat	st;
	SceUID		fd;
	S_NET		net;

	if (strstr(jmpHost, "jbbs")){								// したらば掲示板の場合
		strcpy(bbsName, "したらば掲示板");
		strcpy(itaName, jmpDir);
		p = strchr(itaName, '/');
		if (p) *p = '-';
	} else if (strstr(jmpHost, "machi.to")){					// まちBBSの場合
		strcpy(bbsName, "まちBBS");
		strcpy(itaName, jmpDir);								// 登録無しの場合
		for (i=0; i<15 ;i++){									// リストから探す
			if (strstr(jmpDir, mati1[i])){
				strcpy(itaName, mati2[i]);
				break;
			}
		}
	} else {													// その他のBBS
		url[0] = '\0';
		strcpy(bbsName, jmpHost);
		sprintf(path, "%s/dat/bbs.dat", s2ch.cwDir);
		ret = sceIoGetstat(path, &st);
		if (ret>=0){
			data = (char*)malloc(st.st_size + 1);
			if (data){
				fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
				if (fd>=0){
					psp2chFileRead(fd, data, st.st_size);
					sceIoClose(fd);
					data[st.st_size] = '\0';
					p = data;
					while(1){									// bbs.dat内より対象サーバーを探しBBS名称と正しいbbsmenu.html名称を得る
						p2 = strchr(p, '\n');
						if (p2){
							*p2 = '\0';
							if (*(p2-1)=='\r') *(p2-1) = '\0';
						}
						p1 = strchr(p, '\t');
						if (p1){
							*p1 = '\0';
							if (strstr(jmpHost, p)){			// ホスト名があった
								p = p1 + 1;
								p1 = strchr(p, '\t');
								if (p1) *p1 = '\0';
								strcpy(bbsName, p);				// BBS名称
								if (!p1) break;					// bbsmenu.htmlの指定が無い
								p = p1 + 1;
								sprintf(url, "http://%s/%s", jmpHost, p);	// 正しいbbsmenu.html
								break;
							}
						}
						if (!p2) break;							// 次の行が無い
						p = p2 + 1;
					}
				}
				free(data);
			}
		}
		strcpy(itaName, jmpTitle);								// 板名称のデフォルト
		if (strlen(url)!=0){
			memset(&net, 0, sizeof(S_NET));
			ret = psp2chGet(url, NULL, NULL, &net, 0);			// BBSメニューの取得
			if (ret<0) return 0;
			switch(net.status){
			case 200:
				break;
			default:
				sprintf(err_msg, "%d\n%s\n板メニューの取得に失敗しました", net.status, url);
				psp2chNormalError(NET_GET_ERR, err_msg);
				net.body[0] = '\0';
			}
			strcpy(jmpDirSub, "/");
			strcat(jmpDirSub, jmpDir);
			strcat(jmpDirSub, "/");								// "aa"と"aaorz"の様に部分一致してしまうのを防ぐため
			p = strstr(net.body, jmpDirSub);					// 板名称
			if (p){
				p = strchr(p, '>');
				if (p){
					p++;
					p1 = strchr(p, '<');
					if (p1){
						strncpy(itaName, p, p1-p);
						itaName[p1-p] = '\0';
					}
				}
			}
			free(net.body);
		}
	}

	do{
		ret = psp2chInputDialog("BBS名称を入力してください", "BBS名称", bbsName);
		if (ret) return 0;
		if (strlen(keyWords)==0){
			if (psp2chErrorDialog(2, "何か入力してください")==3){
				return 0;
			}
		}
	}while (strlen(keyWords)==0);
	strcpy(bbsName, keyWords);
	do{
		ret = psp2chInputDialog("板名称を入力してください", "板名称", itaName);
		if (ret) return 0;
		if (strlen(keyWords)==0){
			if (psp2chErrorDialog(2, "何か入力してください")==3){
				return 0;
			}
		}
	}while (strlen(keyWords)==0);
	strcpy(itaName, keyWords);
	strcpy(jmpTitle, itaName);

	sprintf(path, "%s/%s", s2ch.cfg.logDir, BOARD_FILE_EXT);
	ret = sceIoGetstat(path, &st);
	if (ret>=0){
		dataSize = st.st_size + 1024;
	} else {
		dataSize = 1024;
	}
	data = (char*)malloc(dataSize);								// 行を追加するので大きめに確保
	if (data==NULL){
		psp2chNormalError(MEM_ALLC_ERR, "read MY.BRD");
		return 0;
	}
	fd = sceIoOpen(path, PSP_O_RDWR | PSP_O_CREAT, FILE_PARMISSION);
	if (fd>=0){
		dataSize = psp2chFileRead(fd, data, dataSize);			// MY.BRDを読み込む
	} else {
		dataSize = 0;
	}
	data[dataSize] = '\0';
	p = data;
	while(p){													// 同じBBS名称が登録されているか
		if (*p!='\t'){
			p1 = strchr(p, '\t');
			if (p1){
				if (strncmp(p, bbsName, p1-p)==0){				// 同じBBS名称があった場合
					p = strchr(p, '\n');
					if (p) p++;
					break;
				}
			}
		}
		p = strchr(p, '\n');
		if (p) p++;
	}
	tmp[0] = '\0';
	if (p==NULL){												// BBS名称が見つからなかった時は最後尾にBBS名称を追加
		p = &data[dataSize];
		strcpy(tmp, bbsName);
		strcat(tmp, "\t0\n");
	}
	sprintf(tmp, "%s\t%s\t%s\t", tmp, jmpHost, jmpDir);
	strcat(tmp, itaName);
	strcat(tmp, "\n");
	if (&data[dataSize]>p){
		memmove(p+strlen(tmp), p, &data[dataSize] - p +1);		// 追加する場所を空けて
	}
	strncpy(p, tmp, strlen(tmp));								// 行をはめ込む（strcpy()だと\0が付くのでダメ）
	dataSize += strlen(tmp);
	sceIoLseek(fd, 0, SEEK_SET);								// ファイルポインタを先頭に戻して
	psp2chFileWrite(fd, data, dataSize);						// BY.BRDを上書き
	sceIoClose(fd);
	free(data);
	psp2chItaList();											// 板リストを更新
	return 1;
}

//==============================================================
// Beプロフィール
//--------------------------------------------------------------
// http://be.2ch.netから指定されたBE基礎番号のプロフィール画面を表示する。
// 表示にはPSP内蔵インターネットブラウザを使用。
// 多少起動に時間が掛かるけど汎用性があるから…
//--------------------------------------------------------------

void psp2chBeInfo(int anc, char* host, char* dir, unsigned long dat)
{
	char url[1024];

	sprintf(url, "http://be.2ch.net/test/p.php?i=%s&u=d:http://%s/test/read.cgi/%s/%d/l50", s2ch.beAnchor[anc].be, host, dir, dat);
	pspShowBrowser(url, NULL);
}

/*****************************
リンク先のスレに移動
*****************************/
static int psp2chResJump(int urlMenu)
{
	char path[NET_HOST_LENGTH + NET_PATH_LENGTH], tmp[BUF_LENGTH];
	unsigned char *pattern[3] = {
		(unsigned char*)"http://(jbbs\\.livedoor\\.jp)/.+?/read\\.cgi/(.+?/\\d+)/(\\d+).*?",	// したらば掲示板
		(unsigned char*)"http://(.+?)/.+?/read\\.cgi/(.+?)/(\\d+).*?",							// 2ch系
		(unsigned char*)"http://(.+?)/(.+?)/"};													// 板
	char *p;
	int i, r, ret, ita;
	unsigned char *start, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	
	sprintf(path, "http://%s/%s", s2ch.urlAnchor[urlMenu].host, s2ch.urlAnchor[urlMenu].path);
	
	region = onig_region_new();
	end   = (unsigned char*)path + strlen((char*)path);
	start = (unsigned char*)path;
	for (i = 0; i < 3; i++) {
		r = onig_new(&reg, pattern[i], pattern[i] + strlen((char*)pattern[i]),
			ONIG_OPTION_DEFAULT, ONIG_ENCODING_SJIS, ONIG_SYNTAX_DEFAULT, &einfo);
		if (r != ONIG_NORMAL) {
			char s[ONIG_MAX_ERROR_MESSAGE_LEN];
			onig_error_code_to_str(s, r, &einfo);
			psp2chErrorDialog(0, "ERROR: %s\n", s);
			return -1;
		}
		
		if (onig_match(reg, (unsigned char*)path, end, start, region, ONIG_OPTION_NONE) >= 0) {
			break;
		}
		onig_region_clear(region);
	}
	if (i == 3) {
		onig_region_free(region, 1);
		onig_free(reg);
		onig_end();
		return 0;
	}
	psp2chRegGetStr(jmpHost, path, region, 1);
	psp2chRegGetStr(jmpDir, path, region, 2);
	psp2chRegGetStr(tmp, path, region, 3);
	jmpDat = strtol(tmp, NULL, 10);
	onig_region_free(region, 1);
	onig_free(reg);
	onig_end();

	if (s2ch.itaList == NULL) {
		if (psp2chItaList() < 0) {
			return 0;
		}
	}
	//一致するホストを検索
	for (i = 0; i < s2ch.ita.count; i++) {
		if (strcmp(jmpHost, s2ch.itaList[i].host) == 0) {
			break;
		}
	}
	if (i == s2ch.ita.count) {//見つからない
		if (jmpDat <= 0) return 0;								// 板が登録されておらず、DATも指定されていない
		if (psp2chErrorDialog(2, TEXT_13) != 1) {
			return 0;
		}
	}
	
	for(i = 0; i < s2ch.ita.count; i++) {
		if (strcmp(jmpDir, s2ch.itaList[i].dir) == 0) {
			strcpy(jmpTitle, s2ch.itaList[i].title);
			break;
		}
	}
	ita = i;
	if (i == s2ch.ita.count) {
		if(strstr(jmpHost, "jbbs")) {							// したらば掲示板
			p = strchr(jmpDir, '/');
			if (p){
				strcpy(jmpTitle, p + 1);
			} else {
				return 0;										// dat番号が無かった
			}
		}
		else{
			strcpy(jmpTitle, "unknown");
		}
	}

	if (jmpDat == 0){
		sprintf(path, "host: %s\ndir: %s\ntitle: %s\n\nこのスレッド一覧に移動しますか", jmpHost, jmpDir, jmpTitle);
	} else {
		sprintf(path, "host: %s\ndir: %s\ndat: %d\ntitle: %s\n\n%s", jmpHost, jmpDir, jmpDat, jmpTitle, TEXT_11);
	}
	ret = psp2chErrorDialog(3, path);
	if (ret == 3) return -1;									// 移動キャンセル
	if (ret == 4){												// 板リストに登録
		if (!psp2chResBBS()) return -1;							// BBS登録がキャンセルされた
	}
	if (jmpDat == 0){											// dat番号が指定されてないならスレッド一覧へ
		free(s2ch.threadList);
		s2ch.threadList = NULL;
		s2ch.ita.select = ita;
		for (i=0; i<s2ch.category.count; i++){
			if (s2ch.categoryList[i].itaId >= ita){
				s2ch.category.select = i - 1;
				break;
			}
		}
		s2ch.sel = 3;
		return 2;
	} else {													// dat番号が指定されているなら指定スレへ
		s2ch.sel = 6;
		return 1;
	}
}

/*****************************
アンカー情報をリセット
*****************************/
void psp2chResResetAnchors(void)
{
	int i;

	for (i = 0; i < 50; i++)
	{
		s2ch.resAnchor[i].line = -1;
		s2ch.resAnchor[i].x1 = 0;
		s2ch.resAnchor[i].x2 = 0;
		s2ch.urlAnchor[i].line = -1;
		s2ch.urlAnchor[i].x1 = 0;
		s2ch.urlAnchor[i].x2 = 0;
	}
	for (i = 0; i < 40; i++)
	{
		s2ch.idAnchor[i].line = -1;
		s2ch.idAnchor[i].x1 = 0;
		s2ch.idAnchor[i].x2 = 0;
		s2ch.beAnchor[i].line = -1;
		s2ch.beAnchor[i].x1 = 0;
		s2ch.beAnchor[i].x2 = 0;
		s2ch.numAnchor[i].line = -1;
		s2ch.numAnchor[i].x1 = 0;
		s2ch.numAnchor[i].x2 = 0;
	}
}

/*****************************
上下左右キーでの移動
アナログパッドの移動も追加
*****************************/
int psp2chResCursorMove(S_2CH_SCREEN *screen, int totalLine, int lineEnd, int* cursorX, int* cursorY, int limitX, int limitY)
{
	static int keyStart = 0, keyRepeat = 0, rMenu = 0, cursorPosition = 3;
	static clock_t keyTime = 0;
	int padUp = 0, padDown = 0;
	int i, line, positionFlag = 0;

	rMenu = (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].change);
	if (cursorMode && !rMenu)
	{
		if (winParam->tateFlag)
		{
			if ((*cursorY == 0 && (s2ch.pad.Buttons & s2ch.btnRes[1].s.up)) || s2ch.pad.Lx > 127 + s2ch.cfg.padCutoff)
			{
				padUp = (s2ch.pad.Lx == 255) ? s2ch.cfg.padSpeed : 1;
			}
			else if ((*cursorY == limitY && (s2ch.pad.Buttons & s2ch.btnRes[1].s.down)) || s2ch.pad.Lx < 127 - s2ch.cfg.padCutoff)
			{
				padDown = (s2ch.pad.Lx == 0) ? s2ch.cfg.padSpeed : 1;
			}
		}
		else
		{
			if ((*cursorY == 0 && (s2ch.pad.Buttons & s2ch.btnRes[0].s.up)) || s2ch.pad.Ly < 127 - s2ch.cfg.padCutoff)
			{
				padUp = (s2ch.pad.Ly == 0) ? s2ch.cfg.padSpeed : 1;
			}
			else if ((*cursorY == limitY && (s2ch.pad.Buttons & s2ch.btnRes[0].s.down)) || s2ch.pad.Ly > 127 + s2ch.cfg.padCutoff)
			{
				padDown = (s2ch.pad.Ly == 255) ? s2ch.cfg.padSpeed : 1;
			}
		}
	}
	else if (!cursorMode && !rMenu)
	{
		if (winParam->tateFlag)
		{
			if ((*cursorY == 0 && s2ch.pad.Lx > 127 + s2ch.cfg.padCutoff) || (s2ch.pad.Buttons & s2ch.btnRes[1].s.up))
			{
				padUp = (s2ch.pad.Lx == 255) ? s2ch.cfg.padSpeed : 1;
			}
			else if ((*cursorY == limitY && s2ch.pad.Lx < 127 - s2ch.cfg.padCutoff) || (s2ch.pad.Buttons & s2ch.btnRes[1].s.down))
			{
				padDown = (s2ch.pad.Lx == 0) ? s2ch.cfg.padSpeed : 1;
			}
		}
		else
		{
			if ((*cursorY == 0 && s2ch.pad.Ly < 127 - s2ch.cfg.padCutoff) || (s2ch.pad.Buttons & s2ch.btnRes[0].s.up))
			{
				padUp = (s2ch.pad.Ly == 0) ? s2ch.cfg.padSpeed : 1;
			}
			else if ((*cursorY == limitY && s2ch.pad.Ly > 127 + s2ch.cfg.padCutoff) || (s2ch.pad.Buttons & s2ch.btnRes[0].s.down))
			{
				padDown = (s2ch.pad.Ly == 255) ? s2ch.cfg.padSpeed : 1;
			}
		}
	}
	if (s2ch.pad.Buttons != s2ch.oldPad.Buttons || keyRepeat)
	{
		if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
		{
			keyStart = 1;
		}
		else
		{
			keyStart = 0;
		}
		keyTime = sceKernelLibcClock();
		keyRepeat = 0;
		if(padUp)
		{
			screen->start -= padUp;
		}
		else if(padDown)
		{
			screen->start += padDown;
		}
		if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].s.pUp)
		{
			if (rMenu && cursorMode)
			{
				cursorPosition--;
				if (cursorPosition < 0)
				{
					cursorPosition = 3;
				}
				positionFlag = 1;
			}
			else
			{
				screen->start -= (lineEnd - 2);
			}
		}
		else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].s.pDown)
		{
			if (rMenu && cursorMode)
			{
				cursorPosition++;
				if (cursorPosition > 3)
				{
					cursorPosition = 0;
				}
				positionFlag = 2;
			}
			else
			{
				screen->start += (lineEnd - 2);
			}
		}
		if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].s.top)
		{
			if (rMenu && !padUp)
			{
				screen->start = 0;
			}
		}
		else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].s.end)
		{
			if (rMenu && !padDown)
			{
				screen->start = totalLine - lineEnd;
			}
		}
		// 範囲外に出た場合に修正
		if (screen->start > totalLine - lineEnd) {
			screen->start = totalLine - lineEnd;
		}
		if (screen->start < 0) { // totalLine < lineEnd でもマイナスになるので後ろで補正
			screen->start = 0;
		}
		// カーソルモードに対応したアンカー情報をセット
		psp2chResSetAnchorList(screen, lineEnd, cursorPosition);
		// 上下キーでアンカー移動
		if (cursorMode && (!rMenu || positionFlag))
		{
			line = *cursorY / LINE_PITCH + screen->start;
			if((s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].s.up) || positionFlag == 1)
			{
				i = 0;
				while (s2ch.anchorList[i].line >= 0)
				{
					if (s2ch.anchorList[i].line == line && s2ch.anchorList[i].x2 > *cursorX)
					{
						break;
					}
					else if (s2ch.anchorList[i].line > line)
					{
						break;
					}
					i++;
				}
				if (i == 0)
				{
					*cursorY = 0;
				}
				else
				{
					i--;
					while (i>0){								// 右側画面外のアンカーを無視する
						if (s2ch.anchorList[i].x1 < limitX) break;
						i--;
					}
					*cursorX = s2ch.anchorList[i].x1 + 10; //change
					*cursorY = (s2ch.anchorList[i].line - screen->start) * LINE_PITCH + 5;
				}
			}
			else if((s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].s.down) || positionFlag == 2)
			{
				i = 0;
				while (s2ch.anchorList[i].line >= 0)
				{
					if (s2ch.anchorList[i].line == line && s2ch.anchorList[i].x1 > *cursorX && s2ch.anchorList[i].x1 < limitX)
					{
						*cursorX = s2ch.anchorList[i].x1 + 10; //change
						*cursorY = (line - screen->start) * LINE_PITCH + 5;
						break;
					}
					else if (s2ch.anchorList[i].line > line)
					{
						*cursorX = s2ch.anchorList[i].x1 + 10; //change
						*cursorY = (s2ch.anchorList[i].line - screen->start) * LINE_PITCH + 5;
						break;
					}
					i++;
				}
				if (s2ch.anchorList[i].line < 0)
				{
					*cursorY = limitY;
				}
			}
		}
	}
	else
	{
		if (keyStart)
		{
			if (sceKernelLibcClock() - keyTime > 300000)
				keyRepeat = 1;
		}
		else
		{
			if (sceKernelLibcClock() - keyTime > 1000 *0)
				keyRepeat = 1;
		}
	}
	gCPos = cursorPosition;
	return rMenu;
}

/*****************************
レスの行数をセット
スクロールバーの構造体をセット
総行数を返す
*****************************/
static int psp2chResSetLine(S_SCROLLBAR* bar)
{
	int i, j, scrWidth;;

	if (winParam->tateFlag)
	{
		scrWidth = wide ? BUF_WIDTH * 2 : RES_SCR_WIDTH_V;
		if (bar)
		{
			bar->view = SCR_WIDTH - s2ch.font.height - s2ch.font.pitch;
			bar->x = RES_SCR_WIDTH_V;
			bar->y = 0;
			bar->w = RES_BAR_WIDTH_V;
			bar->h = bar->view;
		}
	}
	else
	{
		scrWidth = wide ? BUF_WIDTH * 2 : RES_SCR_WIDTH;
		if (bar)
		{
			bar->view = SCR_HEIGHT - s2ch.font.height;
			bar->x = RES_SCR_WIDTH;
			bar->y = 0;
			bar->w = RES_BAR_WIDTH;
			bar->h = bar->view;
		}
	}
	for (i = 0, j = 0; i < s2ch.res.count; i++)
	{
		s2ch.resList[i].line = psp2chCountRes(i, scrWidth);
		if (s2ch.resList[i].ng == 0)
		{
			j += s2ch.resList[i].line;
			j++;
		} else if (s2ch.cfg.abon){
			j++;
			j++;												// NGの場合でも１行は表示する
		}
	}
	if (bar)
	{
		bar->total = j * LINE_PITCH;
	}
	return j;
}

/*****************************
アナログパッドを読み取ってカーソル座標を更新
*****************************/
void psp2chResPadMove(int* cursorX, int* cursorY, int limitX, int limitY)
{
	int padX, padY;
	int dL, dS;
	int cufoff = 128 - s2ch.cfg.padCutoff, direction = (winParam->tateFlag) ? -1 : 1;

	padX = ((winParam->tateFlag) ? s2ch.pad.Ly : s2ch.pad.Lx) - 127;
	padY = ((winParam->tateFlag) ? s2ch.pad.Lx : s2ch.pad.Ly) - 127;
	if (cursorMode == 0)
	{
		if (s2ch.cfg.padAccel)
		{
			if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].change)
				dL = 5;
			else
				dL = 3;
			if (!winParam->tateFlag)
				dL++;
			if (padX < -s2ch.cfg.padCutoff)
			{
				padX = (1 - padX - s2ch.cfg.padCutoff) * 128 / cufoff;
				*cursorX -= padX >> dL;
			}
			else if (padX > s2ch.cfg.padCutoff)
			{
				padX = (padX - s2ch.cfg.padCutoff) * 128 / cufoff;
				*cursorX += padX >> dL;
			}
			if (padY < -s2ch.cfg.padCutoff)
			{
				padY = (1 - padY - s2ch.cfg.padCutoff) * 128 / cufoff;
				*cursorY -= (padY >> dL) * direction;
			}
			else if (padY > s2ch.cfg.padCutoff)
			{
				padY = (padY - s2ch.cfg.padCutoff) * 128 / cufoff;
				*cursorY += (padY >> dL) * direction;
			}
		}
		else
		{
			if(s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].change)
			{
				dL = 4;
				dS = 2;
			}
			else
			{
				dL = 16;
				dS = 2;
			}
			if (!winParam->tateFlag)
			{
				dL >>= 1;
				dS >>= 1;
			}
			if (padX < -s2ch.cfg.padCutoff)
			{
				if (padX == -127)
					*cursorX -= dL;
				else
					*cursorX -= dS;
			}
			else if (padX > s2ch.cfg.padCutoff)
			{
				if (padX == 128)
					*cursorX += dL;
				else
					*cursorX += dS;
			}
			if (padY < -s2ch.cfg.padCutoff)
			{
				if (padY == -127)
					*cursorY -= dL * direction;
				else
					*cursorY -= dS * direction;
			}
			else if (padY > s2ch.cfg.padCutoff)
			{
				if (padY == 128)
					*cursorY += dL * direction;
				else
					*cursorY += dS * direction;
			}
		}
	}
	if (*cursorX < 0)
		*cursorX = 0;
	else if (*cursorX > limitX)
		*cursorX = limitX;
	if (*cursorY < 0)
		*cursorY = 0;
	else if (*cursorY > limitY)
		*cursorY = limitY;
}

/*****************************
アンカーリスト作成
*****************************/
static void psp2chResSetAnchorList(S_2CH_SCREEN *screen, int lineEnd, int cursorPosition)
{
	int i, j, k, end;
	S_2CH_ANCHOR_LIST list[180], tmp;

	end = screen->start + lineEnd;
	k = 0;
	switch(cursorPosition)
	{
	// レス番
	case 0:
		for (i = screen->start; i < end; i++)
		{
			for (j = 0; j < 40; j++)
			{
				if (s2ch.numAnchor[j].line == i && s2ch.numAnchor[j].x2 > winParam->viewX)
				{
					s2ch.anchorList[k].line = i;
					s2ch.anchorList[k].x1 = s2ch.numAnchor[j].x1 - winParam->viewX;
					s2ch.anchorList[k].x2 = s2ch.numAnchor[j].x2 - winParam->viewX;
					s2ch.anchorList[k].type = 0;
					s2ch.anchorList[k].id = j;
					k++;
					break;
				}
			}
		}
		s2ch.anchorList[k].line = -1;
		break;
	// アンカー
	case 1:
		for (i = screen->start; i < end; i++)
		{
			for (j = 0; j < 50; j++)
			{
				if (s2ch.resAnchor[j].line == i && s2ch.resAnchor[j].x2 > winParam->viewX)
				{
					list[k].line = i;
					list[k].x1 = s2ch.resAnchor[j].x1 - winParam->viewX;
					list[k].x2 = s2ch.resAnchor[j].x2 - winParam->viewX;
					list[k].type = 2;
					list[k].id = j;
					k++;
				}
			}
			for (j = 0; j < 50; j++)
			{
				if (s2ch.urlAnchor[j].line == i && s2ch.urlAnchor[j].x2 > winParam->viewX)
				{
					list[k].line = i;
					list[k].x1 = s2ch.urlAnchor[j].x1 - winParam->viewX;
					list[k].x2 = s2ch.urlAnchor[j].x2 - winParam->viewX;
					list[k].type = 3;
					list[k].id = j;
					k++;
				}
			}
			if (k >= 99)
			{
				break;
			}
		}
		list[k].line = -1;
		k++;
		// sort
		for (i = 0; i < k - 1; i++)
		{
			for (j = i; j < k; j++)
			{
				if ((list[i].line == list[j].line) && (list[i].x1 > list[j].x1))
				{
					tmp = list[i];
					list[i] = list[j];
					list[j] = tmp;
				}
			}
		}
		// 重複削除
		j = 0;
		for (i = 0; i < k - 1; i++)
		{
			if ((list[i].line == list[i + 1].line) && (list[i].x1 == list[i + 1].x1))
			{
				continue;
			}
			s2ch.anchorList[j] = list[i];
			j++;
		}
		s2ch.anchorList[j].line = -1;
		break;
	// ID
	case 2:
		for (i = screen->start; i < end; i++)
		{
			for (j = 0; j < 40; j++)
			{
				if (s2ch.idAnchor[j].line == i && s2ch.idAnchor[j].x2 > winParam->viewX)
				{
					s2ch.anchorList[k].line = i;
					s2ch.anchorList[k].x1 = s2ch.idAnchor[j].x1 - winParam->viewX;
					s2ch.anchorList[k].x2 = s2ch.idAnchor[j].x2 - winParam->viewX;
					s2ch.anchorList[k].type = 1;
					s2ch.anchorList[k].id = j;
					k++;
					break;
				}
			}
			for (j = 0; j < 40; j++)
			{
				if (s2ch.beAnchor[j].line == i && s2ch.beAnchor[j].x2 > winParam->viewX)
				{
					s2ch.anchorList[k].line = i;
					s2ch.anchorList[k].x1 = s2ch.beAnchor[j].x1 - winParam->viewX;
					s2ch.anchorList[k].x2 = s2ch.beAnchor[j].x2 - winParam->viewX;
					s2ch.anchorList[k].type = 4;
					s2ch.anchorList[k].id = j;
					k++;
					break;
				}
			}
		}
		// 重複削除
		j = 0;
		for (i = 0; i < k - 1; i++)
		{
			if ((s2ch.anchorList[i].line == s2ch.anchorList[i + 1].line) &&
			    (s2ch.anchorList[i].x1 == s2ch.anchorList[i + 1].x1))
			{
				continue;
			}
			s2ch.anchorList[j] = s2ch.anchorList[i];
			j++;
		}
		s2ch.anchorList[j].line = -1;
		break;
	// 全部
	default:
		for (i = screen->start; i < end; i++)
		{
			for (j = 0; j < 40; j++)
			{
				if (s2ch.numAnchor[j].line == i && s2ch.numAnchor[j].x2 > winParam->viewX)
				{
					list[k].line = i;
					list[k].x1 = s2ch.numAnchor[j].x1 - winParam->viewX;
					list[k].x2 = s2ch.numAnchor[j].x2 - winParam->viewX;
					list[k].type = 0;
					list[k].id = j;
					k++;
				}
			}
			for (j = 0; j < 40; j++)
			{
				if (s2ch.idAnchor[j].line == i && s2ch.idAnchor[j].x2 > winParam->viewX)
				{
					list[k].line = i;
					list[k].x1 = s2ch.idAnchor[j].x1 - winParam->viewX;
					list[k].x2 = s2ch.idAnchor[j].x2 - winParam->viewX;
					list[k].type = 1;
					list[k].id = j;
					k++;
				}
			}
			for (j = 0; j < 40; j++)
			{
				if (s2ch.beAnchor[j].line == i && s2ch.beAnchor[j].x2 > winParam->viewX)
				{
					list[k].line = i;
					list[k].x1 = s2ch.beAnchor[j].x1 - winParam->viewX;
					list[k].x2 = s2ch.beAnchor[j].x2 - winParam->viewX;
					list[k].type = 4;
					list[k].id = j;
					k++;
				}
			}
			for (j = 0; j < 50; j++)
			{
				if (s2ch.resAnchor[j].line == i && s2ch.resAnchor[j].x2 > winParam->viewX)
				{
					list[k].line = i;
					list[k].x1 = s2ch.resAnchor[j].x1 - winParam->viewX;
					list[k].x2 = s2ch.resAnchor[j].x2 - winParam->viewX;
					list[k].type = 2;
					list[k].id = j;
					k++;
				}
			}
			for (j = 0; j < 50; j++)
			{
				if (s2ch.urlAnchor[j].line == i && s2ch.urlAnchor[j].x2 > winParam->viewX)
				{
					list[k].line = i;
					list[k].x1 = s2ch.urlAnchor[j].x1 - winParam->viewX;
					list[k].x2 = s2ch.urlAnchor[j].x2 - winParam->viewX;
					list[k].type = 3;
					list[k].id = j;
					k++;
				}
			}
			if (k >= 179)
			{
				break;
			}
		}
		list[k].line = -1;
		k++;
		// sort
		for (i = 0; i < k - 1; i++)
		{
			for (j = i; j < k; j++)
			{
				if ((list[i].line == list[j].line) && (list[i].x1 > list[j].x1))
				{
					tmp = list[i];
					list[i] = list[j];
					list[j] = tmp;
				}
			}
		}
		// 重複削除
		j = 0;
		for (i = 0; i < k - 1; i++)
		{
			if ((list[i].line == list[i + 1].line) && (list[i].x1 == list[i + 1].x1))
			{
				continue;
			}
			s2ch.anchorList[j] = list[i];
			j++;
			if (j>=99) break;
		}
		s2ch.anchorList[j].line = -1;
	}
}

/*****************************
NGチェック
*****************************/
void psp2chResCheckNG(void)
{
	char* buf, *p, *q;
	int i;

	if (s2ch.resList == NULL)
	{
		return;
	}
	for (i = 0; i < s2ch.res.count; i++)
	{
		s2ch.resList[i].ng = 0;
	}
	buf = NULL;
	buf = psp2chGetNGBuf(ngNameFile, buf);
	if (buf)
	{
		p= buf;
		while (*p)
		{
			q = strchr(p, '\n');
			if (q == NULL)
			{
				break;
			}
			*q = '\0';
			for (i = 0; i < s2ch.res.count; i++)
			{
				if (strstr(s2ch.resList[i].name, p))
				{
					s2ch.resList[i].ng = 1;
				}
			}
			p = q + 1;
		}
		free(buf);
	}
	buf = NULL;
	buf = psp2chGetNGBuf(ngIDFile, buf);
	if (buf)
	{
		p= buf;
		while (*p)
		{
			q = strchr(p, '\n');
			if (q == NULL)
			{
				break;
			}
			*q = '\0';
			for (i = 0; i < s2ch.res.count; i++)
			{
				if (s2ch.resList[i].id == NULL)
				{
					continue;
				}
				if (strcmp(s2ch.resList[i].id, p) == 0)
				{
					s2ch.resList[i].ng = 2;
				}
			}
			p = q + 1;
		}
		free(buf);
	}
	buf = NULL;
	buf = psp2chGetNGBuf(ngWordFile, buf);
	if (buf)
	{
		p= buf;
		while (*p)
		{
			q = strchr(p, '\n');
			if (q == NULL)
			{
				break;
			}
			*q = '\0';
			for (i = 0; i < s2ch.res.count; i++)
			{
				if (strstr(s2ch.resList[i].text, p))
				{
					s2ch.resList[i].ng = 3;
				}
			}
			p = q + 1;
		}
		free(buf);
	}
	buf = NULL;
	buf = psp2chGetNGBuf(ngMailFile, buf);
	if (buf)
	{
		p= buf;
		while (*p)
		{
			q = strchr(p, '\n');
			if (q == NULL)
			{
				break;
			}
			*q = '\0';
			for (i = 0; i < s2ch.res.count; i++)
			{
				if (strstr(s2ch.resList[i].mail, p))
				{
					s2ch.resList[i].ng = 4;
				}
			}
			p = q + 1;
		}
		free(buf);
	}
	// 1は表示するようにする
	s2ch.resList[0].ng = 0;
}

/*****************************
datファイルをメモリに読み込みデータの区切りを'\0'（文字列終端）に書き換える
s2ch.resList構造体のポインタに各データのアドレスを代入
*****************************/
int psp2chResList(char* host, char* dir, char* title, unsigned long dat)
{
	SceUID fd;
	SceIoStat st;
	char path[FILE_PATH], subject[BUF_LENGTH], thread[BUF_LENGTH], buf[20];
	char *p,*lastRes;
	int ret, startRes, startLine, len, code, LFcount;
	//clock_t time = sceKernelLibcClock();
	
	if (s2ch.cfg.hbl){
		psp2chReadIdx(NULL, NULL, NULL, &startRes, &startLine, NULL, NULL, NULL, host, dir, dat);
	} else {
		psp2chReadIdx(NULL, NULL, NULL, &startRes, &startLine, NULL, NULL, NULL, host, title, dat);
	}
	
	psp2chResLogPath( path, dir, title );
	sprintf( buf, "/%d.dat", dat );
	strcat( path, buf );
//	sprintf(path, "%s/%s/%d.dat", s2ch.cfg.logDir, title, dat);
	ret = sceIoGetstat(path, &st);
	if (ret < 0) {
		startRes = startLine = 0;
		ret = psp2chGetDat(host, dir, title, dat);
		if (ret < 0) {
			return ret;
		}
		ret = sceIoGetstat(path, &st);
		if (ret < 0) {
			psp2chNormalError(FILE_STAT_ERR, path);
			return -1;
		}
	}
	free(resBuffer);
	gResSize = 0;
	resBuffer = (char*)malloc(st.st_size + 1);
	if (resBuffer == NULL)
	{
		psp2chNormalError(MEM_ALLC_ERR, "resBuffer");
		return -1;
	}
	fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
	if (fd < 0)
	{
		psp2chNormalError(FILE_OPEN_ERR, path);
		return -1;
	}
	ret = psp2chFileRead(fd, resBuffer, st.st_size);
	sceIoClose(fd);
	if (ret != st.st_size)
	{
		free(resBuffer);
		resBuffer = NULL;
		pgPrintMenuBar("DATの読み込みに失敗しました");
		pgCopyMenuBar();
		flipScreen(0);
		pgWaitVn(30);
		return -1;
	}
	resBuffer[st.st_size] = '\0';
	s2ch.res.count = 0;
	p = resBuffer;
	len = st.st_size;
	lastRes = NULL;
	LFcount = 0;
	while (len--)
	{
		if (*(p + len) == '\n')
		{
			s2ch.res.count++;
			LFcount++;
			if (LFcount == 2) lastRes = p + len + 1;			// 最後尾から２番目のLFの次の位置が最新レス番号
		}
		if (*(p + len) == '\0')
		{
			*(p + len) = ' ';
		}
	}
	if (lastRes){												// 最新レス番号を数値に変換
		int num;
		num = strtol( lastRes, &p, 10 );
		if (num <= 1000 && s2ch.res.count < num){				// 外部板であぼーんがあった場合
			if (*p=='<' && *(p+1)=='>'){						// レス番号なら
				s2ch.res.count = num;
			}
		}
	}
	free(s2ch.resList);
	s2ch.resList = (S_2CH_RES*)malloc(sizeof(S_2CH_RES) * s2ch.res.count);
	if (s2ch.resList == NULL)
	{
		free(resBuffer);
		resBuffer = NULL;
		psp2chNormalError(MEM_ALLC_ERR, "resList");
		return -1;
	}
	psp2chRegPatGet(host, &code, subject, thread);
	if (code)
		psp2chEucToSjis(resBuffer, resBuffer);
	ret = psp2chResParser((unsigned char*)resBuffer, (unsigned char*)thread);
	if(ret < 0)
	{
		free(s2ch.resList);
		s2ch.resList = NULL;
		free(resBuffer);
		resBuffer = NULL;
		sprintf(err_msg, "%d/%d", ret, s2ch.res.count);
		psp2chNormalError(DATA_PARSE_ERR, err_msg);
		return -1;
	}
	pgPrintMenuBar("NGレスマーキング中");
	pgCopyMenuBar();
	flipScreen(0);
	gResSize = st.st_size;
	psp2chResCheckNG();
	
	pgPrintMenuBar("全行数カウント");
	pgCopyMenuBar();
	flipScreen(0);
	psp2chResSetLine(NULL);
	if (startRes > s2ch.res.count || startRes < 0)
	{
		startRes = 0;
		startLine = 0;
	}
	pgPrintMenuBar("前回位置頭出し");
	pgCopyMenuBar();
	flipScreen(0);
	psp2chResLineGet(startRes, startLine);
	//psp2chSetPalette("%d", (sceKernelLibcClock() - time));
	return 0;
}

/*****************************
2ちゃんねるサーバのdatファイルにアクセスして保存
戻り値 0:データ取得, 1:更新なし, <0:error -1:その他のエラー -2:無線LANOFF -3:dat落ち -4:移転
*****************************/
int psp2chGetDat(char* host, char* dir, char* title, unsigned long dat)
{
	int ret, range, len, startRes, startLine, endRes;
	S_NET net;
	SceUID fd;
	char dst_url[FILE_PATH], src_url[FILE_PATH], path[FILE_PATH], referer[FILE_PATH], buf[BUF_LENGTH], threadTitle[SUBJECT_LENGTH] = "";
	char *p;

	memset(&net, 0, sizeof(S_NET));
	// Make ita directory
	psp2chResLogPath( path, dir, title );
//	sprintf(path, "%s/%s", s2ch.cfg.logDir, title);
	if (path[strlen(path) - 1] == '\\' || path[strlen(path) - 1] == '|')
		strcat(path, "\\");
	if ((fd = sceIoDopen(path)) < 0) {
		if (sceIoMkdir(path, FILE_PARMISSION) < 0) {
			psp2chNormalError(DIR_MAKE_ERR, path);
			return -1;
		}
	}
	else {
		sceIoDclose(fd);
	}
	
	// check idx
	range = startRes = startLine = endRes = 0;
	if (s2ch.cfg.hbl){
		psp2chReadIdx(net.head.Last_Modified, net.head.ETag, &range, &startRes, &startLine, &endRes, NULL, threadTitle, host, dir, dat);
	} else {
		psp2chReadIdx(net.head.Last_Modified, net.head.ETag, &range, &startRes, &startLine, &endRes, NULL, threadTitle, host, title, dat);
	}
	if (range)
		net.head.Range = range - 1;
	
	sprintf(src_url, "http://%s/test/read.cgi/%s/%d/", host, dir, dat);
	ret = psp2chBBSGetPattern(dst_url, referer, src_url, endRes + 1, "res.dat");
	if (ret < 0)
		return ret;
	ret = psp2chGet(dst_url, referer, NULL, &net, 0);
	if (ret < 0)
		return ret;
	switch(net.status)
	{
	case 200: // OK
	case 206: // Partial content
		p = net.body;
		len = net.length;
		break;
	case 302: // Found
	case 404:
		free(net.body);
		if (psp2chItenCheck(host, dir) == 0)
		{
			psp2chErrorDialog(0, TEXT_10);
			sceCtrlPeekBufferPositive(&s2ch.oldPad, 1);
			return -4;
		}
		pgPrintMenuBar("このスレはDAT落ちしたようです");
		pgCopyMenuBar();
		flipScreen(0);
		sceKernelDelayThread(DISPLAY_WAIT);
		return -3;
	case 304: // Not modified
		free(net.body);
		return 1;
	case 416: // abone
		psp2chErrorDialog(0, TEXT_4);
		free(net.body);
		return -1;
	default:
		sprintf(err_msg, "%d", net.status);
		psp2chNormalError(NET_GET_ERR, err_msg);
		free(net.body);
		return -1;
	}
	// abone check
	if (range) {
		ret = 0;
		if(net.head.Content_Length != -1 && net.head.Content_Length < 2) // 未更新で200を返す場合はねる
		{
			free(net.body);
			return 1;
		}
		if(net.head.Range) // Range Check
		{
			if(net.body[0] != '\n') {
				ret = 1;
			}
			p++;
			len--;
		}
		if (ret) {
			free(net.body);
			psp2chErrorDialog(0, TEXT_4);
			return -1;
		}
	}
	if (len == 0) {
		free(net.body);
		psp2chErrorDialog(0, TEXT_16);
		return -1;
	}
	// save dat.dat
	psp2chResLogPath( path, dir, title );
	sprintf( buf, "/%d.dat", dat );
	strcat( path, buf );
//	sprintf(path, "%s/%s/%d.dat", s2ch.cfg.logDir, title, dat);
	fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, FILE_PARMISSION);
	if (fd < 0)
	{
		psp2chNormalError(FILE_OPEN_ERR, path);
		return fd;
	}
	psp2chFileWrite(fd, p, len);
	range += len;
	sceIoClose(fd);
	while (len--)
	{
		if (*(p + len) == '\n')
		{
			endRes++;
		}
	}
	free(net.body);
	// save dat.idx
	if (strlen(threadTitle) == 0) {
		if (s2ch.cfg.hbl){
			psp2chMakeIdx(host, dir, dat);
			psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, NULL, NULL, threadTitle, host, dir, dat);
		} else {
			psp2chMakeIdx(host, title, dat);
			psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, NULL, NULL, threadTitle, host, title, dat);
		}
	}
	psp2chResLogPath( path, dir, title );
	sprintf( buf, "/%d.idx", dat );
	strcat( path, buf );
//	sprintf(path, "%s/%s/%d.idx", s2ch.cfg.logDir, title, dat);
	fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
	if (fd < 0)	{
		psp2chNormalError(FILE_OPEN_ERR, path);
		return fd;
	}
	sprintf(buf, "%s\n%s\n%d\n%d\n%d\n%d\n1\n%s\n", net.head.Last_Modified, net.head.ETag, range, startRes, startLine, endRes, threadTitle);
	psp2chFileWrite(fd, buf, strlen(buf));
	sceIoClose(fd);
	return 0;
}

/*****************************
現在の描画位置（s2ch.res.start）をレス番とレス行に変換
*****************************/
static void psp2chResLineSet(int* startRes, int* startLine)
{
	*startLine = s2ch.res.start;
	for (*startRes = 0; *startRes < s2ch.res.count; (*startRes)++)
	{
		if (s2ch.resList[*startRes].ng == 0)
		{
			*startLine -= s2ch.resList[*startRes].line;
			if (*startLine < 0)
			{
				break;
			}
			(*startLine)--;
		} else if (s2ch.cfg.abon){
			*startLine -= 1;									// NGの場合でも１行は表示する
			if (*startLine < 0)
			{
				break;
			}
			(*startLine)--;
		}
	}
	*startLine = s2ch.resList[*startRes].line + *startLine;
}

/*****************************
レス番とレス行を描画位置（s2ch.res.start）に変換
*****************************/
static void psp2chResLineGet(int startRes, int startLine)
{
	int i;

	s2ch.res.start = 0;
	for (i = 0; i < startRes; i++)
	{
		if (s2ch.resList[i].ng == 0)
		{
			s2ch.res.start += s2ch.resList[i].line;
			s2ch.res.start++;
		} else if (s2ch.cfg.abon){
			s2ch.res.start += 1;
			s2ch.res.start++;									// NGの場合でも１行は表示する
		}
	}
	s2ch.res.start += startLine;
}

/*****************************

*****************************/
static int psp2chDrawResStr(char* str, S_2CH_RES_COLOR c, int line, int lineEnd, int startX, int endX, int *drawLine)
{
	while ((str = pgPrintHtml(str, &c, startX, endX, *drawLine)))
	{
		pgSetDrawStart(startX, -1, 0, LINE_PITCH);
		(*drawLine)++;
		if (++line > lineEnd)
		{
			break;
		}
		pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
	}
	return line;
}

/*****************************
レスのヘッダ部分の表示（名前、日付、ID等）
*****************************/
// ID 出現回数で色を変える
#define ID_COUNT 5
int psp2chDrawResHeader(int re, int* skip, int line, int lineEnd, int startX, int endX,S_2CH_RES_COLOR c,S_2CH_HEADER_COLOR hc, int* drawLine)
{
	const char ngMsg[4][9] = {"NG名前","NGID","NGワード","NGメール"};
	char* str;
	char buf[BUF_LENGTH];
	int i, j;

	if (s2ch.cfg.resType && s2ch.resList[re].res!=0){			// レス番
		sprintf(buf, " %d[%d] ", s2ch.resList[re].num + 1, s2ch.resList[re].res);
	} else {
		sprintf(buf, " %d ", s2ch.resList[re].num + 1);
	}
	str = buf;
	switch (s2ch.resList[re].res){								// アンカーで示された数によりレス番の色を変える
	case 0:
		c.text = hc.num2;
		break;
	case 1:
		c.text = hc.num3;
		break;
	default:
		c.text = hc.num;
		break;
	}
	if (*skip)
	{
		while ((str = pgCountHtml(str, endX, 1)))
		{
			pgSetDrawStart(startX, -1, 0, 0);
			if (--(*skip) == 0)
			{
				s2ch.numAnchor[s2ch.numAnchorCount].x1 = winParam->pgCursorX;
				s2ch.numAnchor[s2ch.numAnchorCount].line = *drawLine;
				s2ch.numAnchor[s2ch.numAnchorCount].num = s2ch.resList[re].num;
				pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
				line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
				break;
			}
		}
	}
	else
	{
		s2ch.numAnchor[s2ch.numAnchorCount].x1 = winParam->pgCursorX;
		s2ch.numAnchor[s2ch.numAnchorCount].line = *drawLine;
		s2ch.numAnchor[s2ch.numAnchorCount].num = s2ch.resList[re].num;
		pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
		line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
	}
	s2ch.numAnchor[s2ch.numAnchorCount].x2 = winParam->pgCursorX;
	s2ch.numAnchorCount++;
	if (s2ch.numAnchorCount >= 40)
	{
		s2ch.numAnchorCount = 0;
	}
	if (s2ch.cfg.abon && s2ch.resList[re].ng){					// NGフラグが立っているなら
		strcpy( buf, ngMsg[s2ch.resList[re].ng-1] );
		strcat( buf, "であぼーん" );
		str = buf;
		c.text = hc.name1;
		if (*skip)
		{
			while ((str = pgCountHtml(str, endX, 1)))
			{
				pgSetDrawStart(startX, -1, 0, 0);
				if (--(*skip) == 0)
				{
					pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
					line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
					line++;
					(*drawLine)++;
					pgSetDrawStart(startX, -1, 0, LINE_PITCH);
					return line;
				}
			}
			pgSetDrawStart(startX, -1, 0, 0);
			(*skip)--;
		} else {
			line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
			line++;
			(*drawLine)++;
			pgSetDrawStart(startX, -1, 0, LINE_PITCH);
		}
		return line;
	}
/*	strcpy(buf, "");											// 名前: (未使用)
	str = buf;
	c.text = hc.name1;
	if (*skip)
	{
		while ((str = pgCountHtml(str, endX, 1)))
		{
			pgSetDrawStart(startX, -1, 0, 0);
			if (--(*skip) == 0)
			{
				pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX +6, LINE_PITCH, 2);
				line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
				break;
			}
		}
	}
	else
	{
		line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
	}
	if (line > lineEnd)
	{
		return line;
	}
*/	str = s2ch.resList[re].name;								// なまえ
	c.text = hc.name2;
	if (*skip)
	{
		while ((str = pgCountHtml(str, endX, 1)))
		{
			pgSetDrawStart(startX, -1, 0, 0);
			if (--(*skip) == 0)
			{
				pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
				line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
				break;
			}
		}
	}
	else
	{
		line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
	}
	if (line > lineEnd)
	{
		return line;
	}
	sprintf(buf, "[%s] ", s2ch.resList[re].mail);				// メール欄
	str = buf;
	c.text = hc.mail;
	if (*skip)
	{
		while ((str = pgCountHtml(str, endX, 1)))
		{
			pgSetDrawStart(startX, -1, 0, 0);
			if (--(*skip) == 0)
			{
				pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
				line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
				break;
			}
		}
	}
	else
	{
		line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
	}
	if (line > lineEnd)
	{
		return line;
	}
	str = s2ch.resList[re].date;								// 日時
	c.text = hc.date;
	if (*skip)
	{
		while ((str = pgCountHtml(str, endX, 1)))
		{
			pgSetDrawStart(startX, -1, 0, 0);
			if (--(*skip) == 0)
			{
				pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
				line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
				break;
			}
		}
	}
	else
	{
		line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
	}
	if (line > lineEnd)
	{
		return line;
	}
	if (s2ch.resList[re].id && strlen(s2ch.resList[re].id))		// ID
	{
		int pos;
		if (strncmp(&s2ch.resList[re].id[-7], "発信元", 6) != 0) {
			sprintf(buf, " ID:%s", s2ch.resList[re].id);
		} else {
			sprintf(buf, " 発信元:%s", s2ch.resList[re].id);
		}
		str = buf;
		c.text = hc.id0;
		pos = 0;
		for (i = 0, j = 0; i < s2ch.res.count; i++)
		{
			if (s2ch.resList[i].id && s2ch.resList[i].id[0] != '?' && (strcmp(s2ch.resList[i].id, s2ch.resList[re].id) == 0))
			{
				j++;
				if (j == 2){
					c.text = hc.id1;
				} else if (j == ID_COUNT)
				{
					c.text = hc.id2;
					if (!s2ch.cfg.idCount) break;
				}
				if (i==re) pos = j;
			}
		}
		if (s2ch.cfg.idCount && j>1){							// IDの出現数表示
			sprintf( str, "%s [%d/%d]", str, pos, j );
		}
		if (*skip)
		{
			while ((str = pgCountHtml(str, endX, 1)))
			{
				pgSetDrawStart(startX, -1, 0, 0);
				if (--(*skip) == 0)
				{
					pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
					line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
					break;
				}
			}
		}
		else
		{
			line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
		}
		if (line > lineEnd)
		{
			return line;
		}
	}
	if (s2ch.resList[re].be && strlen(s2ch.resList[re].be))		// Be
	{
		str = s2ch.resList[re].be;
/*		str = strchr(s2ch.resList[re].be, '-');
		if (str) {
			str++;
		} else {
			str = s2ch.resList[re].be;							// フォーマットがおかしい
		}
*/		sprintf(buf, " BE:%s", str);
		if (strstr(buf, "2BP")) {
			
		} else if (strstr(buf, "BRZ")) {
			
		} else if (strstr(buf, "PLT")) {
			
		} else if (strstr(buf, "DIA")) {
			
		} else if (strstr(buf, "S★")) {
			
		} else {
			
		}
		c.text = hc.num2;
		str = buf;
		if (*skip)
		{
			while ((str = pgCountHtml(str, endX, 1)))
			{
				pgSetDrawStart(startX, -1, 0, 0);
				if (--(*skip) == 0)
				{
					pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX, LINE_PITCH, 2);
					line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
					line++;
					(*drawLine)++;
					pgSetDrawStart(startX, -1, 0, LINE_PITCH);
					return line;
				}
			}
			pgSetDrawStart(startX, -1, 0, 0);
			(*skip)--;
		}
		else
		{
			line = psp2chDrawResStr(str, c, line, lineEnd, startX, endX, drawLine);
			line++;
			(*drawLine)++;
			pgSetDrawStart(startX, -1, 0, LINE_PITCH);
		}
		if (line > lineEnd)
		{
			return line;
		}
	}
	else
	{
		if (*skip)
		{
			pgSetDrawStart(startX, -1, 0, 0);
			(*skip)--;
		}
		else
		{
			line++;
			(*drawLine)++;
			pgSetDrawStart(startX, -1, 0, LINE_PITCH);
		}
	}
	return line;
}

/*****************************
レスの本文の表示
*****************************/
int psp2chDrawResText(int res, int* skip, int line, int lineEnd, int startX, int endX, S_2CH_RES_COLOR c, int* drawLine)
{
	char	*str;
	int		shift;

	if (s2ch.resList[res].ng){
		str = NULL;
	} else {
		str = s2ch.resList[res].text;
	}
	if (*skip)
	{
		while (str)
		{
			shift = (*str==' ') ? 0 : 3;						// 画面左に空白を開ける
			str = pgCountHtml(str, endX, 1);
			pgSetDrawStart(startX, -1, shift, 0);
			if (--(*skip) == 0)
			{
				while (str)
				{
					pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX +6, LINE_PITCH, 2);
					str = pgPrintHtml(str, &c, startX, endX, *drawLine);
					(*drawLine)++;
					if (++line > lineEnd)
					{
						return line;
					}
					if (str){
						shift = (*str==' ') ? 0 : 3;			// 画面左に空白を開ける
					}
					pgSetDrawStart(startX, -1, shift, LINE_PITCH);
				}
			}
		}
		pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX +6, LINE_PITCH, 2);
	}
	else
	{
		while (str)
		{
			shift = (*str==' ') ? 0 : 3;						// 画面左に空白を開ける
			pgSetDrawStart(startX, -1, shift, 0);
			pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX +6, LINE_PITCH, 2);
			str = pgPrintHtml(str, &c, startX, endX, *drawLine);
			(*drawLine)++;
			if (++line > lineEnd)
			{
				return line;
			}
			pgSetDrawStart(startX, -1, shift, LINE_PITCH);
		}
		pgFillvram(c.bg, startX, winParam->pgCursorY, endX-startX +6, LINE_PITCH, 2);
	}
	pgSetDrawStart(startX, -1, 0, LINE_PITCH);
	line++;
	(*drawLine)++;
	return line;
}

/*****************************
レスの表示
drawLine:画面一番上に表示する行
*****************************/
static void psp2chDrawRes(int drawLine)
{
	int re;
	int skip;
	int line = 0;
	int i;
	int endX;

	endX = (wide) ? BUF_WIDTH * 2 : ((winParam->tateFlag) ? RES_SCR_WIDTH_V : RES_SCR_WIDTH);
	// 表示行に変化なし
	if (drawLine == preLine)
	{
		return;
	}
	// 1行下に移動
	else if (drawLine == preLine+1)
	{
		preLine = drawLine;
		i = 0;
		drawLine += winParam->lineEnd-1;
		skip = drawLine;
		while (skip >= 0)
		{
			if (s2ch.resList[i].ng == 0)
			{
				skip -= s2ch.resList[i].line;
				skip--;
			} else if (s2ch.cfg.abon){
				skip--;											// NGの場合でも１行は表示する
				skip--;
			}
			i++;
			if (i >= s2ch.res.count)
			{
				break;
			}
		}
		re = --i;
		skip++;
		if (s2ch.resList[i].ng && s2ch.cfg.abon){
			skip += 1;
		} else {
			skip += s2ch.resList[i].line;
		}
		pgSetDrawStart(0, (LINE_PITCH * drawLine) & 0x01FF, 0, 0);
		line = psp2chDrawResHeader(re, &skip, winParam->lineEnd, winParam->lineEnd, 0, endX, s2ch.resColor, s2ch.resHeaderColor, &drawLine);
		if (line > winParam->lineEnd)
		{
			pgFillvram(s2ch.resColor.bg, 0, winParam->pgCursorY, endX, LINE_PITCH, 2);
			return;
		}
		line = psp2chDrawResText(re, &skip, winParam->lineEnd, winParam->lineEnd, 0, endX, s2ch.resColor, &drawLine);
		pgFillvram(s2ch.resColor.bg, 0, winParam->pgCursorY+LINE_PITCH, endX, LINE_PITCH, 2);
	}
	// 1行上に移動
	else if (drawLine == preLine-1)
	{
		preLine = drawLine;
		skip = drawLine;
		i = 0;
		while (skip >= 0)
		{
			if (s2ch.resList[i].ng == 0)
			{
				skip -= s2ch.resList[i].line;
				skip--;
			} else if (s2ch.cfg.abon){
				skip--;											// NGの場合でも１行は表示する
				skip--;
			}
			i++;
			if (i >= s2ch.res.count)
			{
				break;
			}
		}
		re = --i;
		skip++;
		if (s2ch.resList[i].ng && s2ch.cfg.abon){
			skip += 1;
		} else {
			skip += s2ch.resList[i].line;
		}
		pgSetDrawStart(0, (LINE_PITCH * drawLine) & 0x01FF, 0, 0);
		line = psp2chDrawResHeader(re, &skip, winParam->lineEnd, winParam->lineEnd, 0, endX, s2ch.resColor, s2ch.resHeaderColor, &drawLine);
		if (line > winParam->lineEnd)
		{
			return;
		}
		line = psp2chDrawResText(re, &skip, winParam->lineEnd, winParam->lineEnd, 0, endX, s2ch.resColor, &drawLine);
	}
	// 全画面書き直し
	else
	{
		preLine = drawLine;
		skip = drawLine;
		i = 0;
		while (skip >= 0)
		{
			if (s2ch.resList[i].ng == 0)
			{
				skip -= s2ch.resList[i].line;
				skip--;
			} else if (s2ch.cfg.abon){
				skip--;											// NGの場合でも１行は表示する
				skip--;
			}
			i++;
			if (i >= s2ch.res.count)
			{
				break;
			}
		}
		re = --i;
		skip++;
		if (s2ch.resList[i].ng && s2ch.cfg.abon){
			skip += 1;
		} else {
			skip += s2ch.resList[i].line;
		}
		pgSetDrawStart(0, (LINE_PITCH * drawLine) & 0x01FF, 0, 0);
		s2ch.resAnchorCount = 0;
		s2ch.resAnchor[0].x1 = 0;
		psp2chResResetAnchors();
		line = 0;
		while (line <= winParam->lineEnd)
		{
			line = psp2chDrawResHeader(re, &skip, line, winParam->lineEnd, 0, endX, s2ch.resColor, s2ch.resHeaderColor, &drawLine);
			if (line > winParam->lineEnd)
			{
				break;
			}
			line = psp2chDrawResText(re, &skip, line, winParam->lineEnd, 0, endX, s2ch.resColor, &drawLine);
			if (s2ch.cfg.abon){
				re++;
			} else {
				while (++re < s2ch.res.count && s2ch.resList[re].ng)
				{
					// NG レスをスキップ
				}
			}
			if (re >= s2ch.res.count)
			{
				pgFillvram(s2ch.resColor.bg, 0, winParam->pgCursorY, endX, (winParam->lineEnd + 1 - line)*LINE_PITCH, 2);
				break;
			}
		}
	}
}

/*****************************
res:レス番号
width:レス表示画面の幅
戻り値:レスを画面幅で表示したときの行数
*****************************/
int psp2chCountRes(int res, int width)
{
	char* str;
	char buf[BUF_LENGTH];
	int count = 0;

	// NG 対象スレは除外
	if (s2ch.resList[res].ng)
	{
		return 0;
	}
	pgSetDrawStart(0, 0, 0, 0);
	if (s2ch.cfg.resType && s2ch.resList[res].res!=0){
		sprintf(buf, " %d[%d] ", s2ch.resList[res].num + 1, s2ch.resList[res].res);
	} else {
		sprintf(buf, " %d ", res + 1);							// 本当は" %d "のハズなんだが幅が長いっぽい？ので
	}
	str = buf;
	while ((str = pgCountHtml(str, width, 1)))
	{
		pgSetDrawStart(0, -1, 0, 0);
		count++;
	}
	str = s2ch.resList[res].name;
	while ((str = pgCountHtml(str, width, 1)))
	{
		pgSetDrawStart(0, -1, 0, 0);
		count++;
	}
	sprintf(buf, "[%s] ", s2ch.resList[res].mail);
	str = buf;
	while ((str = pgCountHtml(str, width, 1)))
	{
		pgSetDrawStart(0, -1, 0, 0);
		count++;
	}
	str = s2ch.resList[res].date;
	while ((str = pgCountHtml(str, width, 1)))
	{
		pgSetDrawStart(0, -1, 0, 0);
		count++;
	}
	if (s2ch.resList[res].id && strlen(s2ch.resList[res].id))
	{
		int i,j,pos;
		if (strncmp(&s2ch.resList[res].id[-7], "発信元", 6) != 0) {
			sprintf(buf, " ID:%s", s2ch.resList[res].id);
		} else {
			sprintf(buf, " 発信元:%s", s2ch.resList[res].id);
		}
		str = buf;
		pos = 0;
		if (s2ch.cfg.idCount) {									// IDの出現数表示
			for (i = 0, j = 0; i < s2ch.res.count; i++) {
				if (s2ch.resList[i].id && s2ch.resList[i].id[0] != '?' && (strcmp(s2ch.resList[i].id, s2ch.resList[res].id) == 0))
				{
					j++;
					if (i==res) pos = j;
				}
			}
			if (j > 1) {										// IDが２つ以上見つかった
				sprintf( str, "%s [%d/%d]", str, pos, j );
			}
		}
		while ((str = pgCountHtml(str, width, 1)))
		{
			pgSetDrawStart(0, -1, 0, 0);
			count++;
		}
	}
	if (s2ch.resList[res].be)									// Be
	{
		sprintf(buf, " BE:%s", s2ch.resList[res].be);
		str = buf;
		while ((str = pgCountHtml(str, width, 1)))
		{
			pgSetDrawStart(0, -1, 0, 0);
			count++;
		}
	}
	pgSetDrawStart(0, -1, 0, 0);
	count++;
	str = s2ch.resList[res].text;
	while ((str = pgCountHtml(str, width, 1)))
	{
		pgSetDrawStart(0, -1, 0, 0);
		count++;
	}
	pgSetDrawStart(0, -1, 0, 0);
	count++;
	return count;
}

int psp2chResCopy(int numMenu, int ref, int header)
{
	SceUID	fd,dst;
	int		i,size;
	char	path[256], buf[256];

	if (createDir(s2ch.cfg.templateDir) < 0){					// テンプレートフォルダが無いなら作成する
		psp2chNormalError(DIR_MAKE_ERR, s2ch.cfg.templateDir);
		return -1;
	}

	for (i = 0; i < 1000; i++){
		sprintf(path, "%s/%03d-copy.txt", s2ch.cfg.templateDir, i);
		fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
		if (fd < 0)
			break;
		sceIoClose(fd);
	}

	sprintf(buf, "%s に書き出し中", path);
	pgPrintMenuBar(buf);
	pgCopyMenuBar();
	flipScreen(0);

	fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);

	if (fd > 0)
	{
		char buf[512], format[256], flag = 1, cont = 0;
		char *p, *q;

		if (header)
		{
			if (ref)
				strcpy(format, ">%d %s[%s] %s ID:%s \n");
			else
				strcpy(format, "%d %s[%s] %s ID:%s \n");
			sprintf(buf, format,
				s2ch.resList[s2ch.numAnchor[numMenu].num].num + 1,
				s2ch.resList[s2ch.numAnchor[numMenu].num].name,
				s2ch.resList[s2ch.numAnchor[numMenu].num].mail,
				s2ch.resList[s2ch.numAnchor[numMenu].num].date,
				s2ch.resList[s2ch.numAnchor[numMenu].num].id);
			if (s2ch.resList[s2ch.numAnchor[numMenu].num].be)
			{
				strcat(buf, "Be:");
				strcat(buf, s2ch.resList[s2ch.numAnchor[numMenu].num].be);
				strcat(buf, "\n");
			}
			sceIoWrite(fd, buf, strlen(buf));
		}
		p = s2ch.resList[s2ch.numAnchor[numMenu].num].text;
		while (flag)
		{
			if (!cont)
			{
				cont = 0;
				if (ref)
					sceIoWrite(fd, ">", strlen(">"));
				if (*p == ' ')
					p++;
			}
			q = strchr(p, '<');
			if (q)
			{
				if (q[1] == 'b' && q[2] == 'r' && q[3] == '>')
				{
					sceIoWrite(fd, p, q - p);
					sceIoWrite(fd, "\n", strlen("\n"));
					p = q + strlen("<br>");
					cont = 0;
				}
				else
				{
					sceIoWrite(fd, p, q - p);
					p = strchr(q, '>') + 1;
					cont = 1;
					continue;
				}
			}
			else
			{
				sceIoWrite(fd, p, strlen(p));
				sceIoWrite(fd, "\n", strlen("\n"));
				flag = 0;
			}
		}
		sceIoClose(fd);

	//----- メモ帳のクリップボードへコピー -----

		if((fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION)) < 0){
			return 0;
		}
		if((dst = sceIoOpen("ms0:/PSP/COMMON/CLIPBOARD.TXT", PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION)) < 0)
		{
			sceIoClose(fd);
			return 0;
		}
		while((size = psp2chFileRead(fd, buf, sizeof(buf))) > 0)
		{
			psp2chFileWrite(dst, buf, size);
		}
		sceIoClose(fd);
		sceIoClose(dst);

		psp2chErrorDialog(0, "%s\nに保存しました", path);

	} else {
		psp2chErrorDialog(0, "%s\nへの保存に失敗しました", path);
	}
	return 0;
}

// 名前付き捕獲式集合で値をセットするコールバック
static int res_name_callback(const UChar* name, const UChar* name_end, int ngroup_num, int* group_nums, regex_t* reg, void* arg)
{
	int gn;
	reg_data* data = (reg_data*)arg;
	S_2CH_RES *dst = (S_2CH_RES *)data->target;
	char *str = (char*)data->str;
	OnigRegion *region = data->region;

	gn = group_nums[0];
	switch(name[0]) {
	case 'B': dst->name = str + region->beg[gn]; break;
	case 'C': dst->mail = str + region->beg[gn]; break;
	case 'D': dst->date =  str + region->beg[gn]; break;
	case 'G': dst->text = str + region->beg[gn]; break;
	case 'H': dst->title = str + region->beg[gn]; break;
	case 'E': dst->id = str + region->beg[gn]; break;
	case 'A': dst->num = strtol(str + region->beg[gn], NULL, 10) - 1; break;
	case 'F': dst->be = str + region->beg[gn]; break;
	}
	if (region->beg[gn] != -1)
		str[region->end[gn]] = '\0';
	return 0;
}

static int psp2chResParser(unsigned char *str, const unsigned char *pattern)
{
	char *abon = "あぼーん";
	int i, r, ret;
	unsigned char *start, *end;
	char *text;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;

	r = onig_new(&reg, pattern, pattern + strlen((char*)pattern),
		ONIG_OPTION_DEFAULT, ONIG_ENCODING_SJIS, ONIG_SYNTAX_DEFAULT, &einfo);
	if (r != ONIG_NORMAL) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str(s, r, &einfo);
		psp2chErrorDialog(0, "ERROR: %s\n", s);
		return -1;
	}
	
	region = onig_region_new();
	
	end   = str + strlen((char*)str);
	start = str;
	for (ret = 0; ret < s2ch.res.count; ret++){					// まずレスアンカーで示された数をリセット
		s2ch.resList[ret].res = 0;
		s2ch.resList[ret].resFlag = 0;
	}
	for (ret = 0; ret < s2ch.res.count; ret++)
	{
		if (!*start)
			break;
		s2ch.resList[ret].num = ret;
		s2ch.resList[ret].name = s2ch.resList[ret].mail = s2ch.resList[ret].date = "";
		s2ch.resList[ret].id = s2ch.resList[ret].be = NULL;
		r = onig_search(reg, start, end, start, end, region, ONIG_OPTION_NONE);
		if (r >= 0)
		{
			reg_data data = {&(s2ch.resList[ret]), start, region};
			r = onig_foreach_name(reg, res_name_callback, (void*)&data);
			start += region->end[0] + 1;
		}
		else
		{
			s2ch.resList[ret].text = (char*)broken;
			start = (unsigned char*)strchr((char*)start, '\n') + 1;
		}
		onig_region_clear(region);

		if (s2ch.resList[ret].num != ret){						// レス番に欠番があった（あぼーんされてる）
			int i,next;
			next = s2ch.resList[ret].num;
			s2ch.resList[next] = s2ch.resList[ret];				// 本来の位置へコピー
			for (i=ret; i<next; i++){							// 欠番を「あぼーん」で埋める
				s2ch.resList[i].num = i;
				s2ch.resList[i].name = abon;
				s2ch.resList[i].mail = abon;
				s2ch.resList[i].date = abon;
				s2ch.resList[i].title =  NULL;
				s2ch.resList[i].id =  NULL;
				s2ch.resList[i].be =  NULL;
				s2ch.resList[i].text = abon;
			}
			ret = next;
		}

		text = s2ch.resList[ret].text;
		while ((text = strstr(text,"&gt;&gt;"))){				// ">>"を探してアンカー先にマークを打つ
			int		i,num1,num2;
			char	c;
			text += 8;
			do{
				c = 0;
				if (!sscanf( text, "%d%c", &num1, &c )){		// レス番を取得
					num1 = 0;
					c = '\0';
				}
				text = strchr( text, c );
				if (c=='-'){									// 範囲指定
					int		st,ed;
					text++;
					sscanf( text, "%d%c", &num2, &c );
					text = strchr( text, c );
					if (num1>num2){
						st = num2;
						ed = num1;
					} else {
						st = num1;
						ed = num2;
					}
					if (st>=s2ch.res.count) st = s2ch.res.count +1;
					if (ed>=s2ch.res.count) ed = s2ch.res.count +1;
					if (ed-st<s2ch.cfg.anchorRange && st>0){
						for ( i=st-1; i<ed ;i++ ){				// 重複カウントしないよう、今はマークするだけ
							if (i<s2ch.res.count) s2ch.resList[i].resFlag = 1;
						}
					}
				} else {										// 単独指定
					if (num1>0 && num1<=s2ch.res.count){		// 重複カウントしないよう、今はマークするだけ
						s2ch.resList[num1-1].resFlag = 1;
					}
				}
				if (c=='<'){									// レス番ジャンプ用タグを無視する
					text = strchr( text, '>' );
					if (text){
						text++;
						c = *text;
					}
				}
				if (c==',' && text!=NULL) text++;
			} while (c==',' && text!=NULL);
			if (text==NULL) break;
		}
		for (i = 0; i < s2ch.res.count; i++){					// マークされてるレスのみカウントする
			if (s2ch.resList[i].resFlag){
				s2ch.resList[i].res ++;
				s2ch.resList[i].resFlag = 0;
			}
		}
		
		if (ret % 50 == 0) {
			char buf[32];
			sprintf(buf, "parsing at %d", ret);
			pgPrintMenuBar(buf);
			pgCopyMenuBar();
			flipScreen(0);
		}
	}
	onig_region_free(region, 1);
	onig_free(reg);
	onig_end();

	if (s2ch.res.count != ret)
		s2ch.res.count = ret;
	return 0;
}

static int psp2cGetThreadTitle(char *title, const char *host, char *buf)
{
	int *index;
	int code, ret;
	char subject[BUF_LENGTH], thread[BUF_LENGTH];
	char s[ONIG_MAX_ERROR_MESSAGE_LEN];
	unsigned char *start, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	const char *titleName = "H";

	psp2chRegPatGet(host, &code, subject, thread);
	if (code)
		psp2chEucToSjis(buf, buf);
	ret = onig_new(&reg, (unsigned char*)thread, (unsigned char*)thread + strlen(thread),
		ONIG_OPTION_DEFAULT, ONIG_ENCODING_SJIS, ONIG_SYNTAX_DEFAULT, &einfo);
	if (ret != ONIG_NORMAL) {
		goto Error;
	}
	region = onig_region_new();
	end   = (unsigned char*)strchr(buf, '\n') + 1;
	start = (unsigned char*)buf;
	ret = onig_search(reg, (unsigned char*)buf, end, start, end, region, ONIG_OPTION_NONE);
	if (ret < 0) {
		goto Error;
	}
	onig_name_to_group_numbers(reg, (unsigned char*)titleName, (unsigned char*)titleName + strlen(titleName), &index);
	memcpy(title, buf + region->beg[index[0]], region->end[index[0]]- region->beg[index[0]]);
	title[region->end[index[0]] - region->beg[index[0]]] = '\0';
	onig_region_free(region, 1);
	onig_free(reg);
	onig_end();
	
	return 0;
	
Error:
	onig_error_code_to_str(s, ret, &einfo);
	psp2chErrorDialog(0, "ERROR: %s\n", s);
	return -1;
}

static int psp2chMakeIdx(const char *host, const char *title, const unsigned long dat)
{
	SceUID fd;
	char path[FILE_PATH], threadTitle[BUF_LENGTH];
	char *buf, *p;
	int ret, fileSize, endRes = 0;

	sprintf(path, "%s/%s/%d.dat", s2ch.cfg.logDir, title, dat);
	fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
	if (fd < 0) {
		return -1;
	}
	else {
		fileSize = sceIoLseek32(fd, 0, SEEK_END);
		sceIoLseek32(fd, 0, SEEK_SET);
		buf = malloc(sizeof(char) * (fileSize + 1));
		if (buf == NULL) {
			sceIoClose(fd);
			return -1;
		}
		ret = psp2chFileRead(fd, buf, fileSize);
		sceIoClose(fd);
		
		if (ret != fileSize) {
			free(buf);
			return -1;
		}
		buf[fileSize] = '\0';
		
		p = buf;
		while ((p = strchr(p, '\n'))) {
			endRes++;
			p++;
		}
		
		if (psp2cGetThreadTitle(threadTitle, host, buf) < 0) {
			free(buf);
			return -1;
		}
		
		sprintf(buf,
			"Thu, 1 Jan 1970 00:00:00 GMT\n\"\"\n%d\n0\n0\n%d\n1\n%s\n",
			fileSize, endRes, threadTitle);
		sprintf(path, "%s/%s/%d.idx", s2ch.cfg.logDir, title, dat);
		fd = sceIoOpen(path, TRUNC, FILE_PARMISSION);
		if (fd < 0) {
			free(buf);
			return -1;
		}
		psp2chFileWrite(fd, buf, strlen(buf));
		sceIoClose(fd);
		free(buf);
	}
	return 1;
}

int psp2chReadIdx(
	char *lastModified, char *eTag, int *range, int *startRes, int *startLine, int *endRes, int *update, char *threadTitle,
	const char *host, const char *title, const unsigned long dat)
{
	SceUID fd;
	char path[FILE_PATH];
	char *buf, *p, *q;
	int fileSize, progress = 0;
	
	sprintf(path, "%s/%s/%d.idx", s2ch.cfg.logDir, title, dat);
	fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
	if (fd < 0) {
		if (psp2chMakeIdx(host, title, dat) < 0) {
			if (lastModified) {
				strcpy(lastModified, "");
			}
			if (eTag) {
				strcpy(eTag, "");
			}
			if (range) {
				*range = 0;
			}
			if (startRes) {
				*startRes = 0;
			}
			if (startLine) {
				*startLine = 0;
			}
			if (endRes) {
				*endRes = 0;
			}
			if (update) {
				*update = 0;
			}
			if (threadTitle) {
				strcpy(threadTitle, "");
			}
			return 8;
		}
		psp2chReadIdx(lastModified, eTag, range, startRes, startLine, endRes, update, threadTitle, host, title, dat);
		return 8;												// psp2chMakeIdx()に成功している以上psp2chReadIdx()には失敗しないはず
	}
	else {
		fileSize = sceIoLseek32(fd, 0, SEEK_END);
		sceIoLseek32(fd, 0, SEEK_SET);
		buf = malloc(sizeof(char) * (fileSize + 1));
		if (buf == NULL) {
			sceIoClose(fd);
			return -1;
		}
		psp2chFileRead(fd, buf, fileSize);
		buf[fileSize] = '\0';
		sceIoClose(fd);
		
		p = buf;
		
		while (*p) {
			q = memchr(p, '\n', fileSize);
			if (q == NULL)
				break;
			*q = '\0';
			switch (progress) {
			case 0:
				if (lastModified) {
					strcpy(lastModified, p);
				}
				break;
			case 1:
				if (eTag) {
					strcpy(eTag, p);
				}
				break;
			case 2:
				if (range) {
					*range = strtol(p, NULL, 10);
				}
				break;
			case 3:
				if (startRes) {
					*startRes = strtol(p, NULL, 10);
				}
				break;
			case 4:
				if (startLine) {
					*startLine = strtol(p, NULL, 10);
				}
				break;
			case 5:
				if (endRes) {
					*endRes = strtol(p, NULL, 10);
				}
				break;
			case 6:
				if (update) {
					*update = strtol(p, NULL, 10);
				}
				break;
			case 7:
				if (threadTitle) {
					strcpy(threadTitle, p);
				}
				break;
			}
			p = q + 1;
			progress++;
		}
		free(buf);
	}
	return progress;
}

void psp2chWriteIdx(const char *host, const char *title, const unsigned long dat, int forcedUpdate)
{
	int startRes, startLine, range;
	int startRes_old, startLine_old, endRes;
	char lastModified[BUF_LENGTH] = "", eTag[BUF_LENGTH] = "", threadTitle[BUF_LENGTH] = "", path[FILE_PATH];
	SceUID fd;
	
	if (psp2chReadIdx(lastModified, eTag, &range, &startRes_old, &startLine_old, &endRes, NULL, threadTitle, host, title, dat) != 8) {
		psp2chMakeIdx(host, title, dat);
		psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, &endRes, NULL, threadTitle, host, title, dat);
		forcedUpdate = 1;
	}
	if (s2ch.threadList) {
		s2ch.threadList[threadSort[s2ch.thread.select]].old = s2ch.res.count;
	}
	if (s2ch.favList && s2ch.sel == 4) {
		s2ch.favList[favSort[s2ch.fav.select]].res = s2ch.res.count;
		s2ch.favList[favSort[s2ch.fav.select]].update = 0;
	}
	if (!forcedUpdate) {
		psp2chResLineSet(&startRes, &startLine);
		endRes = s2ch.res.count;
	}
	else {
		startRes = startRes_old;
		startLine = startLine_old;
	}
	if (startRes == startRes_old && startLine == startLine_old && !forcedUpdate) {
		return;
	}
	
	sprintf(path, "%s/%s/%d.idx", s2ch.cfg.logDir, title, dat);
	fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
	if (fd < 0) {
		return;
	}
	sprintf(path, "%s\n%s\n%d\n%d\n%d\n%d\n0\n%s\n", lastModified, eTag, range, startRes, startLine, endRes, threadTitle);
	psp2chFileWrite(fd, path, strlen(path));
	sceIoClose(fd);
}
