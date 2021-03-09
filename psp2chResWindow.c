/*
* $Id: psp2chResWindow.c 145 2008-08-21 08:26:11Z bird_may_nike $
*/

#include "psp2ch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pg.h"
#include "psp2chNet.h"
#include "psp2chRes.h"
#include "psp2chResWindow.h"
#include "psp2chImageView.h"
#include "psphtmlviewer.h"
#include "oniguruma.h"
#include "psp2chReg.h"
#include "utf8.h"		// add

extern S_2CH s2ch;		// psp2ch.c
extern int preLine;		// psp2chRes.c
extern int cursorMode;	// psp2chRes.c
extern int gCPos;		// psp2chRes.c
extern S_2CH_RES_MENU_STR menuRes[2];
extern Window_Layer *winParam;

enum IMG_TYPE
{
	NOT_IMG,
	IMG_JPG,
	IMG_PNG,
	IMG_BMP,
	IMG_GIF,
	IMG_IMEPITA
};

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

// prototype
static void psp2chDrawResAnchor(S_2CH_RES_ANCHOR a, S_2CH_SCREEN anchor, int lineFlag);
void psp2chAnchorSearch(int res);

/*****************************
レスのウィンドウ表示
lineFlag:0=変更なし; 1=下に1行追加; 2=上に1行追加; その他:全画面再描画
psp2chResAnchor()とpsp2chIdAnchor()とpsp2chAnchorSearch()で使用
*****************************/
static void psp2chDrawResAnchor(S_2CH_RES_ANCHOR a, S_2CH_SCREEN anchor, int lineFlag)
{
	int re;
	int skip;
	int line = 0, i, drawLine;
	int startX, startY, scrW, lineEnd;

	if (winParam->tateFlag)
	{
		startX = 1;
		startY = 2;
		scrW = RES_A_WIDTH_V;
		lineEnd = RES_A_LINE_V;
	}
	else
	{
		startX = 1;
		startY = 2;
		scrW = RES_A_WIDTH;
		lineEnd = RES_A_LINE;
	}
	drawLine = anchor.start;
	if (lineFlag == 0 && drawLine)
	{
		return;
	}
	else if (lineFlag == 1)
	{
		drawLine += lineEnd-1;
		skip = drawLine;
		i = 0;
		while (skip >= 0)
		{
			if (s2ch.resList[a.res[i]].ng == 0)
			{
				skip -= psp2chCountRes(a.res[i], scrW);
				skip--;
			} else if (s2ch.cfg.abon){
				skip--;											// NGの場合でも１行は表示する
				skip--;
			}
			i++;
			if (i >= a.resCount)
			{
				break;
			}
		}
		re = --i;
		skip++;
		if (s2ch.resList[a.res[i]].ng && s2ch.cfg.abon){
			skip += 1;
		} else {
			skip += psp2chCountRes(a.res[i], scrW);
		}
		pgSetDrawStart(startX, (LINE_PITCH * drawLine + startY) & 0x01FF, 0, 0);
		line = psp2chDrawResHeader(a.res[re], &skip, lineEnd, lineEnd, startX, scrW+startX, s2ch.resAColor, s2ch.resAHeaderColor, &drawLine);
		if (line > lineEnd)
		{
			return;
		}
		line = psp2chDrawResText(a.res[re], &skip, lineEnd, lineEnd, startX, scrW+startX, s2ch.resAColor, &drawLine);
	}
	else if (lineFlag == 2)
	{
		skip = drawLine;
		i = 0;
		while (skip >= 0)
		{
			if (s2ch.resList[a.res[i]].ng == 0)
			{
				skip -= psp2chCountRes(a.res[i], scrW);
				skip--;
			} else if (s2ch.cfg.abon){
				skip--;											// NGの場合でも１行は表示する
				skip--;
			}
			i++;
			if (i >= a.resCount)
			{
				break;
			}
		}
		re = --i;
		skip++;
		if (s2ch.resList[a.res[i]].ng && s2ch.cfg.abon){
			skip += 1;
		} else {
			skip += psp2chCountRes(a.res[i], scrW);
		}
		pgSetDrawStart(startX, (LINE_PITCH*drawLine+startY)&0x01FF, 0, 0);
		line = psp2chDrawResHeader(a.res[re], &skip, lineEnd, lineEnd, startX, scrW+startX, s2ch.resAColor, s2ch.resAHeaderColor, &drawLine);
		if (line > lineEnd)
		{
			return;
		}
		line = psp2chDrawResText(a.res[re], &skip, lineEnd, lineEnd, startX, scrW+startX, s2ch.resAColor, &drawLine);
	}
	else
	{
		skip = drawLine;
		i = 0;
		while (skip >= 0)
		{
			if (s2ch.resList[a.res[i]].ng == 0)
			{
				skip -= psp2chCountRes(a.res[i], scrW);
				skip--;
			} else if (s2ch.cfg.abon){
				skip--;											// NGの場合でも１行は表示する
				skip--;
			}
			i++;
			if (i >= a.resCount)
			{
				break;
			}
		}
		re = --i;
		skip++;
		if (s2ch.resList[a.res[i]].ng && s2ch.cfg.abon){
			skip += 1;
		} else {
			skip += psp2chCountRes(a.res[i], scrW);
		}
//		pgSetDrawStart(startX, (LINE_PITCH*drawLine+startY)&0x01FF, 0, 0);
		pgSetDrawStart(startX, startY, 0, 0);
		s2ch.resAnchorCount = 0;
		s2ch.resAnchor[0].x1 = 0;
		line = 0;
//		if (s2ch.resList[a.res[re]].ng)
//		{
//			pgFillvram(s2ch.resAColor.bg, startX-2, winParam->pgCursorY, scrW+2, (lineEnd - line + 1)*LINE_PITCH, 2);
//			return;
//		}
		while (line <= lineEnd)
		{
			line = psp2chDrawResHeader(a.res[re], &skip, line, lineEnd, startX, scrW+startX, s2ch.resAColor, s2ch.resAHeaderColor, &drawLine);
			if (line > lineEnd)
			{
				break;
			}
			line = psp2chDrawResText(a.res[re], &skip, line, lineEnd, startX, scrW+startX, s2ch.resAColor, &drawLine);
			if (s2ch.cfg.abon){
				re++;
			} else {
				while (++re < a.resCount && a.res[re] < s2ch.res.count && s2ch.resList[a.res[re]].ng)
				{
				}
			}
			if (re >= a.resCount || a.res[re] >= s2ch.res.count)
			{
				pgFillvram(s2ch.resAColor.bg, startX-2, winParam->pgCursorY, scrW+2, (lineEnd - line + 1)*LINE_PITCH, 2);
				break;
			}
		}
	}
}

/**************
レスアンカー表示
***************/
void psp2chResAnchor(S_2CH_RES_ANCHOR *anc)
{
	const char *sBtnH[] = {"Sel", "", "", "St", "↑", "→", "↓", "←", "L", "R", "", "", "△", "○", "×", "□", ""};
	static int	getTex = 0;
	int i, j, lineFlag, drawLine, totalLine = 0, rMenu;
	int index1, tmp;
	S_SCROLLBAR bar;
	S_2CH_SCREEN anchor;
	S_2CH_RES_ANCHOR a = *anc;
	int startX, startY, scrX, scrY, lineEnd, barW, cursorX, cursorY;
	char menuStr[64],menuRes[64];
	int numMenu, idMenu, beMenu, resMenu, urlMenu;

	if (a.resCount <= 0) {
		return;
	}
	if (!getTex){
		pgCreateTexture();										// 新しい作画画面を生成
	}
	getTex++;													// 再帰処理の回数をカウント
	if (winParam->tateFlag) {
		startX = 0;
		startY = 1;
		scrX = RES_A_WIDTH_V;
		scrY = RES_A_HEIGHT_V;
		lineEnd = RES_A_LINE_V;
		barW = RES_BAR_WIDTH_V;
		bar.view = RES_A_HEIGHT_V;
		bar.x = startX + scrX;
		bar.y = 0;
		bar.w = RES_BAR_WIDTH_V;
		bar.h = RES_A_HEIGHT_V;
	}
	else {
		startX = 0;
		startY = 0;
		scrX = RES_A_WIDTH;
		scrY = RES_A_HEIGHT;
		lineEnd = RES_A_LINE;
		barW = RES_BAR_WIDTH;
		bar.view = RES_A_HEIGHT;
		bar.x = startX + scrX;
		bar.y = 0;
		bar.w = RES_BAR_WIDTH;
		bar.h = RES_A_HEIGHT;
	}
	for (i = 0; i < a.resCount; i++) {
		totalLine += psp2chCountRes(a.res[i], scrX);
		totalLine++;
	}
	if (totalLine < lineEnd) {
		totalLine = lineEnd;
	}
	anchor.count = totalLine;
	bar.total = totalLine * LINE_PITCH;
	bar.start = 0;
	anchor.start = anchor.select = 0;
	cursorX = startX + scrX;
	cursorY = startY + scrY;
	drawLine = anchor.start;
	lineFlag = 3;
	getIndex(s2ch.btnRes[winParam->tateFlag].cursor, index1);

	while (s2ch.running) {
		psp2chResResetAnchors();
//		psp2chDrawResAnchor(a, anchor, lineFlag);
		psp2chDrawResAnchor(a, anchor, 3);						// こっちの方がカーソル挙動が自然だと思う
		// ポインタ位置のチェック
		i = j = 0;
		numMenu = idMenu = beMenu = resMenu = urlMenu = -1;
		sprintf(menuStr, "　× : 戻る　　%s : カーソ\ル(%d)", sBtnH[index1], cursorMode);
		while (s2ch.anchorList[i].line >= 0)
		{
			if (cursorY / LINE_PITCH + anchor.start == s2ch.anchorList[i].line &&
				cursorX > s2ch.anchorList[i].x1 && cursorX < s2ch.anchorList[i].x2)
			{
				switch(s2ch.anchorList[i].type)
				{
				case 0:
					// レス番メニュー
					numMenu = s2ch.anchorList[i].id;
					sprintf(menuStr, "　○ : レス元抽出　　× : 戻る　　%s : カーソ\ル(%d)", sBtnH[index1], cursorMode);
					break;
				case 1:
					// IDメニュー
					idMenu = s2ch.anchorList[i].id;
					sprintf(menuStr, "　○ : ID抽出　　× : 戻る　　%s : カーソ\ル(%d)", sBtnH[index1], cursorMode);
					break;
				case 2:
					// レスアンカーメニュー
					resMenu = s2ch.anchorList[i].id;
					sprintf(menuStr, "　○ : レス抽出　　× : 戻る　　%s : カーソ\ル(%d)", sBtnH[index1], cursorMode);
					break;
				case 3:
					// URLアンカーメニュー
					urlMenu = s2ch.anchorList[i].id;
					sprintf(menuStr, "　○ : リンク表\示　　× : 戻る　　%s : カーソ\ル(%d)", sBtnH[index1], cursorMode);
					break;
				case 4:
					// BEアンカーメニュー
					beMenu = s2ch.anchorList[i].id;
					sprintf(menuStr, "　○ : Be抽出　　× : 戻る　　%s : カーソ\ル(%d)", sBtnH[index1], cursorMode);
					break;
				}
				j = 1;
				break;
			}
			i++;
		}
		pgPrintMenuBar(menuStr);
		
		if(sceCtrlPeekBufferPositive(&s2ch.pad, 1)) {
			rMenu = psp2chResCursorMove(&anchor, totalLine, lineEnd, &cursorX, &cursorY, scrX, scrY);
			psp2chResPadMove(&cursorX, &cursorY, bar.x, bar.view);
			if(anchor.start == drawLine) {
				lineFlag = 0;
			}
			else if(anchor.start == drawLine + 1) {
				lineFlag = 1;
			}
			else if(anchor.start == drawLine - 1) {
				lineFlag = 2;
			}
			else {
				lineFlag = 3;
			}
			drawLine = anchor.start;
			
			if (s2ch.pad.Buttons != s2ch.oldPad.Buttons) {
				s2ch.oldPad = s2ch.pad;
				if (idMenu >= 0) {
					if (s2ch.pad.Buttons & PSP_CTRL_CIRCLE) {
						psp2chIdAnchor(idMenu);
					}
				}
				else if (beMenu >= 0) {
					if (s2ch.pad.Buttons & PSP_CTRL_CIRCLE) {
						psp2chBeAnchor(beMenu);
					}
				}
				else if (resMenu >= 0) {
					if (s2ch.pad.Buttons & PSP_CTRL_CIRCLE) {
						psp2chResAnchor(&s2ch.resAnchor[resMenu]);
					}
				}
				else if (urlMenu >= 0) {
					if (s2ch.pad.Buttons & PSP_CTRL_CIRCLE) {
						psp2chUrlAnchor(urlMenu, anchor.start * LINE_PITCH, 1, 0);
					}
				}
				else if (numMenu >= 0) {
					if (s2ch.pad.Buttons & PSP_CTRL_CIRCLE) {
						psp2chAnchorSearch(s2ch.numAnchor[numMenu].num+1);
					}
				}
				if (s2ch.pad.Buttons & PSP_CTRL_CROSS) {
					break;
				}
				else if (s2ch.pad.Buttons & s2ch.btnRes[winParam->tateFlag].cursor)
				{
					cursorMode = (cursorMode) ? 0 : 1;
				}
			}
		}
		pgDrawTexture(1);										// 背景を更新
//		pgCopyWindow(anchor.start * LINE_PITCH, startX, startY, scrX, scrY);
		pgCopyWindow(0, startX, startY, scrX, scrY);			// 抽出ウィンドウ内容を画面へ転送
		pgWindowFrame(startX, startY, startX + scrX + barW, startY + scrY);
		bar.start = anchor.start * LINE_PITCH;
		pgCopyMenuBar();
		pgScrollbar(&bar, s2ch.resABarColor);
		if (cursorMode && rMenu) {
			pgPadCursor(cursorX, cursorY, gCPos+1);
		} else {
			pgPadCursor(cursorX, cursorY, 0);
		}
		flipScreen(0);
	}
	getTex--;
	if (!getTex){												// 再帰処理の最後の場合なら
		pgDeleteTexture();										// 最新の作画画面を削除
	}
	preLine = -2;
	return;
}

/**************
ID抽出
***************/
void psp2chIdAnchor(int anc)
{
	int i;
	S_2CH_RES_ANCHOR a;

	a.resCount = 0;
	for (i = 0; i < s2ch.res.count; i++) {
		if (s2ch.resList[i].id && strcmp(s2ch.resList[i].id, s2ch.idAnchor[anc].id) == 0) {
			a.res[a.resCount] = i;
			a.resCount++;
		}
	}
	psp2chResAnchor(&a);

	return;
}

//==============================================================
// Be抽出
//--------------------------------------------------------------

void psp2chBeAnchor(int anc)
{
	int i;
	S_2CH_RES_ANCHOR a;

	a.resCount = 0;
	for (i = 0; i < s2ch.res.count; i++) {
		if (s2ch.resList[i].id && strstr(s2ch.resList[i].be, s2ch.beAnchor[anc].be)) {
			a.res[a.resCount] = i;
			a.resCount++;
		}
	}
	psp2chResAnchor(&a);

	return;
}

/**************
URL
***************/
int psp2chUrlAnchor(int anchor, int offset, int mode, const unsigned long dat)
{
	SceUID fd;
	int i, ret, view, type = NOT_IMG, redirect = 1;
	int picMenu = 1,thumbFlag = -1;
	S_NET net;
	char ref[FILE_PATH], fpath[FILE_PATH], buf[BUF_LENGTH];
	char ext[8], tmp[4], url[FILE_PATH], url2[FILE_PATH];
	unsigned char digest[16];
	char *p, *p2, *p3;
	const char *err_msg[] = {"ヘッダ不正", "ライブラリエラー", "メモリ容量不足", "形式不明"};

	picMenu = s2ch.cfg.picMenu;
	thumbFlag = s2ch.cfg.thumb;
	memset(&net, 0, sizeof(S_NET));
	if (s2ch.urlAnchor[anchor].http){
		sprintf(url, "https://%s/%s", s2ch.urlAnchor[anchor].host, s2ch.urlAnchor[anchor].path);
	} else {
		sprintf(url, "http://%s/%s", s2ch.urlAnchor[anchor].host, s2ch.urlAnchor[anchor].path);
	}
	psp2chUrlPatGet(url2, ref, &view, url);
	if (view == 2 || view == 3) {								// imepic/imefix対応処理
		ret = psp2chGet(url, ref, NULL, &net, redirect);		// htmlを取得
		if (ret < 0) {
			psp2chErrorDialog(0, "ネットアクセスで問題発生");
			return ret;
		}
		switch(net.status) {
		case 200: // OK
		case 201:
			break;
		default:
			free(net.body);
			pgCopy(0, offset);
			sprintf(buf, "Status code %d", net.status);
			pgPrintMenuBar(buf);
			pgCopyMenuBar();
			flipScreen(0);
			pgWaitVn(30);
			return -1;
		}
		ret = 0;
		if (view == 2) {										// imepic用
			p = strstr(net.body, url2);							// 画像パスの位置を探す
			if (p) {
				p2 = strchr(p, '"');
				if (p2) {										// 画像パスの終端位置を探す
					strncpy(url2, p, p2-p);						// 正式な画像ファイル名を取得
					url2[p2-p] = '\0';
					ret = 1;
				}
			}
		} else if (view == 3) {									// imefix用
			p = strstr(net.body, url2);							// 画像パスの位置を探す
			if (p) {
				p2 = strchr(p, '"');
				if (p2) {										// 画像パスの終端位置を探す
					strcpy(url2, url);
					p3 = strchr(url2, '.');
					if (p3) {
						p3 = strchr(p3, '/');
						if (p3) {
							strncpy(p3, p, p2-p);
							p3[p2-p] = '\0';
							ret = 1;
						}
					}
				}
			}
		}
		free(net.body);
		memset(&net, 0, sizeof(S_NET));
		view = 1;
		if (!ret) {
			psp2chErrorDialog(0, "画像ファイルへのパスの取得に失敗しました");
			return -1;
		}
	}
	if (mode == 1)
		mode = view;
	
	p = strrchr(url2, '#');
	if (p)
		*p = '\0';
	
	p = strrchr(url2, '.');
	if (p && strlen(p) < 8)
		strcpy(ext, p);
	else
		ext[0] = '\0';
	if(stricmp(ext, ".jpg") == 0 || stricmp(ext, ".jpeg") == 0)
		type = IMG_JPG;
	else if(stricmp(ext, ".png") == 0)
		type = IMG_PNG;
	else if(stricmp(ext, ".bmp") == 0)
		type = IMG_BMP;
	else if(stricmp(ext, ".gif") == 0)
		type = IMG_GIF;
	
	if (!s2ch.cfg.imageView && mode > 0)
	{
		sprintf(buf, "%s/%s", s2ch.urlAnchor[anchor].host, s2ch.urlAnchor[anchor].path);
		sceKernelUtilsMd5Digest((u8*)buf, strlen(buf), digest);
		sprintf(fpath, "%s/%d-", s2ch.cfg.imageDir, dat);
		for (i = 0; i < 16; i++) {
			sprintf(tmp, "%02x", digest[i]);
			strcat(fpath, tmp);
		}
		strcat(fpath, ext);
		fd = sceIoOpen(fpath, PSP_O_RDONLY, FILE_PARMISSION);
		if (fd > 0) {//キャッシュ有り
			net.status = 0;
			net.Header = NULL;
			net.length = sceIoLseek(fd, 0, SEEK_END);
			sceIoLseek(fd, 0, SEEK_SET);
			net.body = (char*)malloc(sizeof(char) * (net.length + 1));
			if(net.body == NULL){
				sprintf(buf, "%dbyte 確保できませんでした", net.length);
				psp2chNormalError(MEM_ALLC_ERR, buf);			// エラーメッセージを追加
				return -1;
			}
			if(psp2chFileRead(fd, net.body, net.length) != net.length) {
				free(net.body);
				psp2chErrorDialog(0, "キャッシュの取得に失敗");
				return -1;
			}
			sceIoClose(fd);
			net.body[net.length] = '\0';
		}
		else {//キャッシュ無し
			ret = psp2chGet(url2, ref, NULL, &net, redirect);
			if (ret < 0) {
				psp2chErrorDialog(0, "ネットアクセスで問題発生");
				return ret;
			}
			switch(net.status) {
			case 200: // OK
			case 201:
				break;
			default:
				free(net.body);
				pgCopy(0, offset);
				sprintf(buf, "Status code %d", net.status);
				pgPrintMenuBar(buf);
				pgCopyMenuBar();
				flipScreen(0);
				pgWaitVn(30);
				return -1;
			}
			if(mode == 2) {
				saveImage(fpath, net.body, net.length);
				free(net.body);
				return 0;
			}
			if(!strstr(net.head.Content_Type, "image") && type == NOT_IMG) {
				psp2chErrorDialog(0, "not image");
				free(net.body);
				return -1;
			}
		}
		pgCopy(0, offset);
		pgPrintMenuBar("表\示します");
		pgCopyMenuBar();
		flipScreen(0);
		// 拡張子の存在しない場合
		if(!ext[0]) {
			if(strstr(net.head.Content_Type, "jpg") || strstr(net.head.Content_Type, "jpeg")) {
				type = IMG_JPG;
				strcat(fpath, ".jpg");
			}
			else if(strstr(net.head.Content_Type, "png")) {
				type = IMG_PNG;
				strcat(fpath, ".png");
			}
			else if(strstr(net.head.Content_Type, "bmp")) {
				type = IMG_BMP;
				strcat(fpath, ".bmp");
			}
			else if(strstr(net.head.Content_Type, "gif")) {
				type = IMG_GIF;
				strcat(fpath, ".gif");
			}
		}
		//拡張子毎に処理
		switch(type) {
		case IMG_JPG: ret = psp2chImageViewJpeg(net.body, net.length, &picMenu, &thumbFlag, 0, NULL); break;
		case IMG_PNG: ret = psp2chImageViewPng(net.body, net.length, &picMenu, &thumbFlag, 0, NULL); break;
		case IMG_BMP: ret = psp2chImageViewBmp(net.body, net.length, &picMenu, &thumbFlag, 0, NULL); break;
		case IMG_GIF: ret = psp2chImageViewGif(net.body, net.length, &picMenu, &thumbFlag, 0, NULL); break;
		default: ret = -1;
		}
		//エラー表示
		switch (ret) {
		case COMPLETE:
		case SAVE_ERR: break;
		case HEAD_ERR: strcpy(buf, err_msg[0]); break;
		case LIB_ERR: strcpy(buf, err_msg[1]); break;
		case MEM_ERR: strcpy(buf, err_msg[2]); break;
		case SUP_ERR: strcpy(buf, err_msg[3]); break;
		}
		if (ret < 0) {
			char buff[32];
			sprintf(buff, "%s error: %s", ext + 1, buf);
			pgPrintMenuBar(buff);
			pgCopyMenuBar();
			flipScreen(0);
			pgWaitVn(30);
		}
		if (ret == SAVE_ERR && net.status)
			saveImage(fpath, net.body, net.length);
		free(net.body);
	}
	else
		pspShowBrowser(url, NULL);
	
	sceCtrlPeekBufferPositive(&s2ch.oldPad, 1);
	return 0;
}

/**************
レス元表示
指定されたレス番号にアンカーを打っている書き込みを集計して表示
***************/
void psp2chAnchorSearch(int res)
{
	S_2CH_RES_ANCHOR	a;
	char	*text;
	int		i;

	a.resCount = 0;
	for (i = 0; i < s2ch.res.count; i++) {
		text = s2ch.resList[i].text;
		while ((text = strstr(text,"&gt;&gt;"))){				// ">>"を探してアンカー先にマークを打つ
			int		num1,num2;
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
					if (ed-st<s2ch.cfg.anchorRange && res>=st && res<=ed){	// このレスからアンカーが打たれている
						a.res[a.resCount] = i;					// レスを登録
						a.resCount++;
						text = NULL;
						break;
					}
				} else {										// 単独指定
					if (num1==res){								// このレスからアンカーが打たれている
						a.res[a.resCount] = i;					// レスを登録
						a.resCount++;
						text = NULL;
						break;
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
	}

	psp2chResAnchor(&a);
	return;
}

