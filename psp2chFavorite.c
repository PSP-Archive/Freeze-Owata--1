/*
* $Id: psp2chFavorite.c 152 2008-09-10 05:53:53Z bird_may_nike $
*/

#include "psp2ch.h"
#include <psputility.h>
#include <pspgu.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psp2chIta.h"
#include "psp2chThread.h"
#include "psp2chRes.h"
#include "psp2chFavorite.h"
#include "psp2chMenu.h"
#include "psp2chNet.h"
#include "utf8.h"
#include "pg.h"

#define favItaBoard "favoriteita.brd"

extern S_2CH s2ch; // psp2ch.c
extern int preLine; // psp2chRes.c
extern char keyWords[128]; // psp2chThread.c
extern Window_Layer *winParam;

int* favSort = NULL;
int favSortType = 0;
char favBoard[32] = "favorite.brd";

static S_2CH_MENU_STR menuFav[2];
static S_2CH_MENU_STR menuFavIta[2];

// prototype
static int psp2chLoadFavorite(void);
static int psp2chLoadFavoriteIta(void);
static void psp2chFavSort(int sort);
static void psp2chFavSortDialog(void);
static void psp2chDrawFavorite(void);
static void psp2chDrawFavoriteIta(void);

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

void psp2chFavSetMenuString(const char **sBtnH, const char **sBtnV)
{
    int index1, index2, index3, index4, index5, index6;
    int i, tmp;

    getIndex(s2ch.favB[0].ok, index1);
    getIndex(s2ch.favB[0].move, index2);
    getIndex(s2ch.favB[0].change, index3);
    getIndex(s2ch.favB[0].del, index4);
    getIndex(s2ch.favB[0].shift, index5);
    sprintf(menuFav[0].main, "　%s : 決定　　%s : 板一覧　　%s : お気に板　　%s : 削除　　%s : メニュー切替",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5]);
    sprintf(menuFavIta[0].main, "　%s : 決定　　%s : 板一覧　　%s : お気にスレ　　%s : 削除　　%s : メニュー切替",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5]);

    getIndex(s2ch.listB[0].top, index1);
    getIndex(s2ch.listB[0].end, index2);
    getIndex(s2ch.favB[0].sort, index3);
    getIndex(s2ch.favB[0].search2ch, index4);
    getIndex(s2ch.favB[0].update, index5);
    getIndex(s2ch.favB[0].trans, index6);
    sprintf(menuFav[0].sub, "　%s : 先頭　%s : 最後　%s : ソ\ート　%s : 全板検索　%s : 一括取得　%s : 切り替え",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5], sBtnH[index6]);
    sprintf(menuFavIta[0].sub, "　%s : 先頭　　　%s : 最後　　　%s : 全板検索",
            sBtnH[index1], sBtnH[index2], sBtnH[index4]);

    getIndex(s2ch.favB[1].ok, index1);
    getIndex(s2ch.favB[1].move, index2);
    getIndex(s2ch.favB[1].change, index3);
    getIndex(s2ch.favB[1].del, index4);
    getIndex(s2ch.favB[1].shift, index5);
    sprintf(menuFav[1].main, "　%s : 決定　　%s : 板一覧　　%s : お気に板\n　%s : 削除　　%s : メニュー切替",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5]);
    sprintf(menuFavIta[1].main,"　%s : 決定　　%s : 板一覧　　%s : お気にスレ\n　%s : 削除　　%s : メニュー切替",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5]);

    getIndex(s2ch.listB[1].top, index1);
    getIndex(s2ch.listB[1].end, index2);
    getIndex(s2ch.favB[1].sort, index3);
    getIndex(s2ch.favB[1].search2ch, index4);
    getIndex(s2ch.favB[1].update, index5);
    getIndex(s2ch.favB[1].trans, index6);
    sprintf(menuFav[1].sub, "　%s : 先頭　%s : 最後　%s : ソ\ート\n　%s : 全板検索　%s : 一括取得　%s : 切り替え",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5], sBtnV[index6]);
    sprintf(menuFavIta[1].sub, "　%s : 先頭　　　%s : 最後　　　%s : 全板検索",
            sBtnV[index1], sBtnV[index2], sBtnV[index4]);
}

/**********************
 Favorite
**********************/
int psp2chFavorite(void)
{
	static int focus = -1, update = -1;
	int rMenu, i, res, change = 0;
    char* menuStr;

	if (s2ch.favList == NULL)
	{
		psp2chLoadFavorite();
	}
	if (s2ch.favItaList == NULL)
	{
		psp2chLoadFavoriteIta();
	}
	if (s2ch.favList == NULL && s2ch.favItaList == NULL)
	{
		s2ch.sel = 2;
		return -1;
	}
	if (focus < 0)
	{
		if (s2ch.favList && s2ch.cfg.favSelect == 0)
		{
			focus = s2ch.cfg.favSelect;
		}
		else if (s2ch.favItaList && s2ch.cfg.favSelect == 1)
		{
			focus = s2ch.cfg.favSelect;
		}
		else
		{
			focus = 1;
		}
	}
	if (focus)
	{
		rMenu = psp2chCursorSet(&s2ch.favIta, winParam->lineEnd, s2ch.favB[winParam->tateFlag].shift, 1, &change);
		menuStr = (rMenu) ? menuFavIta[winParam->tateFlag].sub : menuFavIta[winParam->tateFlag].main;
	}
	else
	{
		rMenu = psp2chCursorSet(&s2ch.fav, winParam->lineEnd, s2ch.favB[winParam->tateFlag].shift, 1, &change);
		menuStr = (rMenu) ? menuFav[winParam->tateFlag].sub : menuFav[winParam->tateFlag].main;
	}
    // 一括取得処理(1スレごとに描画)
    if (update >= 0)
    {
    	// 一括更新停止用
		if ((s2ch.pad.Buttons & PSP_CTRL_CROSS) || update == s2ch.fav.count)
	    {
	    		update = -1;
	    }
	    else
	    {
	    	sceKernelDelayThread(DISPLAY_WAIT * 2); /* add for wait update */
	    	s2ch.fav.select = update;
        	if (s2ch.favList[favSort[update]].update != -1)
        	{
	            if (s2ch.fav.select >= s2ch.fav.start + winParam->lineEnd)
	            {
	                s2ch.fav.start = s2ch.fav.select - winParam->lineEnd + 1;
	            }
	            res = psp2chGetDat(
	            	s2ch.favList[favSort[update]].host, 
	            	s2ch.favList[favSort[update]].dir, 
	            	s2ch.favList[favSort[update]].title, 
	            	s2ch.favList[favSort[update]].dat);
	            switch (res)
	            {
            	case 0:
            		if (s2ch.cfg.hbl){
						psp2chReadIdx(NULL, NULL, NULL, NULL, NULL,
								&(s2ch.favList[favSort[update]].res), &(s2ch.favList[favSort[update]].update), NULL,
								s2ch.favList[favSort[update]].host, s2ch.favList[favSort[update]].dir, s2ch.favList[favSort[update]].dat);
					} else {
	                	psp2chReadIdx(NULL, NULL, NULL, NULL, NULL,
	                			&(s2ch.favList[favSort[update]].res), &(s2ch.favList[favSort[update]].update), NULL,
	                			s2ch.favList[favSort[update]].host, s2ch.favList[favSort[update]].title, s2ch.favList[favSort[update]].dat);
	                }
                	break;
                case 1:
	            	break;
            	case -2:
            		return 0;
            	case -3:
            		s2ch.favList[favSort[update]].update = -1;
            		break;
            	default:
                	s2ch.favList[favSort[update]].res = 0;
                	s2ch.favList[favSort[update]].update = 1;
                	break;
	            }
	        }
            update++;
        }
        change = 1;
    }
    if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
    {
        if (s2ch.pad.Buttons & PSP_CTRL_SELECT)
        {
            psp2chSetScreenParam(1);
        }
        // STARTボタン
        else if(s2ch.pad.Buttons & PSP_CTRL_START)
        {
            psp2chMenu(NULL);
        }
        else if (rMenu)
        {
            // 全板検索
            if (s2ch.pad.Buttons & s2ch.favB[winParam->tateFlag].search2ch)
            {
                if (psp2chThreadSearch() == 0 && keyWords[0])
                {
                    if (s2ch.findList)
                    {
                        free(s2ch.findList);
                        s2ch.findList = NULL;
                    }
                    s2ch.sel = 7;
                }
            }
            if (!focus)
            {
                // ソート
                if (s2ch.pad.Buttons & s2ch.favB[winParam->tateFlag].sort)
                {
                    psp2chFavSortDialog();
                }
                // 一括取得開始
                else if (s2ch.pad.Buttons & s2ch.favB[winParam->tateFlag].update)
                {
                    update = 0;
                }
                else if (s2ch.pad.Buttons & s2ch.favB[winParam->tateFlag].trans)
                {
                	if (memcmp(favBoard, "favorite.brd", strlen("favorite.brd")) == 0)
                		strcpy(favBoard, "favorite1.brd");
                	else
                		strcpy(favBoard, "favorite.brd");
                	psp2chLoadFavorite();
                }
            }
        }
        else
        {
            // 決定
            if (s2ch.pad.Buttons & s2ch.favB[winParam->tateFlag].ok)
            {
                if (focus)
                {
                    if (s2ch.itaList == NULL)
                    {
                        if (psp2chItaList() < 0)
                        {
                            return 0;
                        }
                    }
                    for (i = 0; i < s2ch.ita.count; i++)
                    {
                        if (strcmp(s2ch.itaList[i].title, s2ch.favItaList[s2ch.favIta.select].title) == 0)
                        {
                            s2ch.ita.select = i;
                            s2ch.sel = 3;
                            return 0;
                        }
                    }
                }
                else
                {
                    free(s2ch.resList);
                    s2ch.resList = NULL;
                    preLine = -2;
                    s2ch.sel = 4;
                    return 0;
                }
            }
            // 板一覧に移動
            else if (s2ch.pad.Buttons & s2ch.favB[winParam->tateFlag].move)
            {
                s2ch.sel = 2;
            }
            // お気に入り切り替え
            else if (s2ch.pad.Buttons & s2ch.favB[winParam->tateFlag].change)
            {
                focus = 1 - focus;
                if (focus && s2ch.favItaList == NULL)
                    focus = 0;
                else if (focus == 0 && s2ch.favList == NULL)
                    focus = 1;
            }
            // 削除
            else if (s2ch.pad.Buttons & s2ch.favB[winParam->tateFlag].del) {
                if (focus)
                    psp2chDelFavoriteIta(s2ch.favIta.select);
                else
                    psp2chDelFavorite(s2ch.favList[favSort[s2ch.fav.select]].title, s2ch.favList[favSort[s2ch.fav.select]].dat);
            }
        }
		change = 1;
    }
    if (change) {
		if (focus)
			psp2chDrawFavoriteIta();
		else
			psp2chDrawFavorite();
		pgPrintMenuBar(menuStr);
	}
	pgCopy(winParam->viewX, 0);
	pgCopyMenuBar();
    return 0;
}

/**********************
favorite.brdがあれば読み込んで
s2ch.favListのメモリ再確保とデータ作成
**********************/
static int psp2chLoadFavorite(void)
{
    SceUID fd;
    SceIoStat st;
    char path[FILE_PATH];
    char *buf, *p, *r;
    int i;

    sprintf(path, "%s/%s", s2ch.cfg.logDir, favBoard);
    i = sceIoGetstat(path, &st);
    if (i < 0)
    {
        return -1;
    }
    buf = (char*)malloc(st.st_size + 1);
    if (buf == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }
    fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
        free(buf);
        return -1;
    }
    psp2chFileRead(fd, buf, st.st_size);
    sceIoClose(fd);
    buf[st.st_size] = '\0';
    p = buf;
    s2ch.fav.count = 0;
    while (*p)
    {
        if (*p++ == '\n')
        {
            s2ch.fav.count++;
        }
    }
    if (s2ch.fav.count <= 0)
    {
        free(buf);
        return -1;
    }
    s2ch.favList = (S_2CH_FAVORITE*)realloc(s2ch.favList, sizeof(S_2CH_FAVORITE) * s2ch.fav.count);
    if (s2ch.favList == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, "favList");
        return -1;
    }
    favSort = (int*)realloc(favSort, sizeof(int) * s2ch.fav.count);
    if (favSort == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, "favSort");
        return -1;
    }
    r = buf;
    i = 0;
    while (*r)
    {
    	sscanf(r, "%[^\t] %[^\t] %[^\t] %d %[^\n]",
    		s2ch.favList[i].host,
    		s2ch.favList[i].dir,
    		s2ch.favList[i].title,
    		&(s2ch.favList[i].dat),
    		s2ch.favList[i].subject);
    	r = strchr(r, '\n') + 1;
        s2ch.favList[i].update = 0;
		if (s2ch.cfg.hbl){
			psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, &(s2ch.favList[i].res), &(s2ch.favList[i].update), NULL, 
					s2ch.favList[i].host, s2ch.favList[i].dir, s2ch.favList[i].dat);
		} else {
	        psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, &(s2ch.favList[i].res), &(s2ch.favList[i].update), NULL, 
	        		s2ch.favList[i].host, s2ch.favList[i].title, s2ch.favList[i].dat);
	    }
        i++;
    }
    free(buf);
    psp2chFavSort(-1);
    return 0;
}

/**********************
favoriteita.brdがあれば読み込んで
s2ch.favItaListのメモリ再確保とデータ作成
**********************/
static int psp2chLoadFavoriteIta(void)
{
    SceUID fd;
    SceIoStat st;
    char path[FILE_PATH];
    char *buf, *p, *r;
    int i;

    sprintf(path, "%s/%s", s2ch.cfg.logDir, favItaBoard);
    i = sceIoGetstat(path, &st);
    if (i < 0)
    {
        return -1;
    }
    buf = (char*)malloc(st.st_size + 1);
    if (buf == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }
    fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
        free(buf);
        return -1;
    }
    psp2chFileRead(fd, buf, st.st_size);
    sceIoClose(fd);
    buf[st.st_size] = '\0';
    p = buf;
    s2ch.favIta.count = 0;
    while (*p)
    {
        if (*p++ == '\n')
        {
            s2ch.favIta.count++;
        }
    }
    if (s2ch.favIta.count <= 0)
    {
        free(buf);
        return -1;
    }
    s2ch.favItaList = (S_2CH_FAV_ITA*)realloc(s2ch.favItaList, sizeof(S_2CH_FAV_ITA) * s2ch.favIta.count);
    if (s2ch.favItaList == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }
    r = buf;
    i = 0;
    while (*r)
    {
        p = strchr(r, '\t');
        *p= '\0';
        strcpy(s2ch.favItaList[i].cate, r);
        r = ++p;
        p = strchr(r, '\n');
        *p= '\0';
        strcpy(s2ch.favItaList[i].title, r);
        r = ++p;
        i++;
    }
    free(buf);
    return 0;
}

/**********************
表示中のスレッドをfavorite.brdの最後に追加
psp2chLoadFavorite()でリストを作成しなおす
**********************/
int psp2chAddFavorite(char* host, char* dir, char* title, unsigned long dat)
{
    SceUID fd;
    char path[FILE_PATH];
    S_2CH_FAVORITE *temp;
    int i;

    if (s2ch.fav.count == 0)
    {
        psp2chLoadFavorite();
    }
    for (i = 0; i < s2ch.fav.count; i++)
    {
        if (s2ch.favList[i].dat == dat && strcmp(s2ch.favList[i].title, title) == 0)
        {
            psp2chErrorDialog(0, TEXT_8);
            return -1;
        }
    }
    sprintf(path, "%s/%s", s2ch.cfg.logDir, favBoard);
    fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, FILE_PARMISSION);
    if (fd < 0)
    {
        return -1;
    }
    sprintf(path, "%s\t%s\t%s\t%d\t%s\n", host, dir, title, dat, s2ch.resList[0].title);
    psp2chFileWrite(fd, path, strlen(path));
    sceIoClose(fd);
    
    
	temp = realloc(s2ch.favList, sizeof(S_2CH_FAVORITE) * (s2ch.fav.count + 1));
	if (temp == NULL) {
		psp2chNormalError(MEM_ALLC_ERR, "favList");
		return -1;
	}
	s2ch.favList = temp;
	favSort = (int*)realloc(favSort, sizeof(int) * (s2ch.fav.count + 1));
    if (favSort == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, "favSort");
        return -1;
    }
	
	strcpy(s2ch.favList[s2ch.fav.count].host, host);
	strcpy(s2ch.favList[s2ch.fav.count].dir, dir);
	strcpy(s2ch.favList[s2ch.fav.count].title, title);
	s2ch.favList[s2ch.fav.count].dat = dat;
	strcpy(s2ch.favList[s2ch.fav.count].subject, s2ch.resList[0].title);
	s2ch.favList[s2ch.fav.count].res = s2ch.res.count;
	s2ch.favList[s2ch.fav.count].update = 1;
	s2ch.fav.count++;
	psp2chFavSort(-1);
    return 0;
}

/**********************
favoriteita.brdの最後に追加
psp2chLoadFavoriteIta()でリストを作成しなおす
**********************/
int psp2chAddFavoriteIta(char* cate, char* title)
{
    SceUID fd;
    char path[FILE_PATH];
    int i;

    if (s2ch.favIta.count == 0)
    {
        psp2chLoadFavoriteIta();
    }
    for (i = 0; i < s2ch.favIta.count; i++)
    {
        if (strcmp(s2ch.favItaList[i].cate, cate) == 0 && strcmp(s2ch.favItaList[i].title, title) == 0)
        {
            psp2chErrorDialog(0, TEXT_8);
            return -1;
        }
    }
    sprintf(path, "%s/%s", s2ch.cfg.logDir, favItaBoard);
    fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_APPEND, FILE_PARMISSION);
    if (fd < 0)
    {
        return -1;
    }
    sprintf(path, "%s\t%s\n", cate, title);
    psp2chFileWrite(fd, path, strlen(path));
    sceIoClose(fd);
    return psp2chLoadFavoriteIta();
}

/**********************
s2ch.favListからtitleとdatの一致する項目以外のリストのみをfavorite.brdに書き出す
リストを作成しなおす
**********************/
int psp2chDelFavorite(char* title, unsigned long dat)
{
    SceUID fd;
    char path[FILE_PATH];
    int i, del;

    if (s2ch.favList == NULL || s2ch.fav.count <= 0)
    {
        return -1;
    }
    if (psp2chErrorDialog(1, TEXT_9) == PSP_UTILITY_MSGDIALOG_RESULT_YES)
    {
        sprintf(path, "%s/%s", s2ch.cfg.logDir, favBoard);
        fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
        if (fd < 0)
        {
            return -1;
        }
        for (i = 0, del = -1; i < s2ch.fav.count; i++)
        {
            if (s2ch.favList[i].dat == dat && strcmp(s2ch.favList[i].title, title) == 0)
            {
                del = i;
                continue;
            }
            sprintf(path, "%s\t%s\t%s\t%d\t%s\n", s2ch.favList[i].host, s2ch.favList[i].dir, s2ch.favList[i].title, s2ch.favList[i].dat, s2ch.favList[i].subject);
            psp2chFileWrite(fd, path, strlen(path));
        }
        sceIoClose(fd);
        if (s2ch.fav.start > 0)
        	s2ch.fav.start--;
        if (s2ch.fav.select > 0)
	        s2ch.fav.select--;
        if (del >= 0)
        {
            s2ch.fav.count--;
            for (i = del; i < s2ch.fav.count; i++)
            {
                s2ch.favList[i] = s2ch.favList[i+1];
            }
        }
    }
    psp2chFavSort(-1);
    return 0;
}

/**********************
s2ch.favItaListからindex以外のリストのみをfavoriteita.brdに書き出す
psp2chLoadFavoriteIta()でリストを作成しなおす
**********************/
int psp2chDelFavoriteIta(int index)
{
    SceUID fd;
    char path[FILE_PATH];
    int i;

    if (s2ch.favItaList == NULL || s2ch.favIta.count <= 0)
    {
        return -1;
    }
    if (psp2chErrorDialog(1, TEXT_12) == PSP_UTILITY_MSGDIALOG_RESULT_YES)
    {
        sprintf(path, "%s/%s", s2ch.cfg.logDir, favItaBoard);
        fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
        if (fd < 0)
        {
            return -1;
        }
        for (i = 0; i < s2ch.favIta.count; i++)
        {
            if (i == index)
            {
                continue;
            }
            sprintf(path, "%s\t%s\n", s2ch.favItaList[i].cate, s2ch.favItaList[i].title);
            psp2chFileWrite(fd, path, strlen(path));
        }
        sceIoClose(fd);
        s2ch.favIta.start = 0;
        s2ch.favIta.select = 0;
        return psp2chLoadFavoriteIta();
    }
    return 0;
}

/****************
お気に入りスレッドをソートする
favSort配列にソートデータが入る
sort:
0=板名
1=スレタイ
2=作成日(降順)
3=作成日(昇順)
4=レス数(降順)
5=レス数(昇順)
*****************/
static void psp2chFavSort(int sort)
{
    int i, j, tmp;

    if (sort >= 0)
    {
        favSortType = sort;
    }
    switch (favSortType)
    {
    case 0:
        for (i = 0; i < s2ch.fav.count; i++)
        {
            favSort[i] = i;
        }
        for (i = 0; i < s2ch.fav.count-1; i++)
        {
            for (j = i; j < s2ch.fav.count; j++)
            {
                if (strcmp(s2ch.favList[favSort[j]].title, s2ch.favList[favSort[i]].title) < 0)
                {
                    tmp = favSort[j];
                    favSort[j] = favSort[i];
                    favSort[i] = tmp;
                }
                else if (strcmp(s2ch.favList[favSort[j]].title, s2ch.favList[favSort[i]].title) == 0)
                {
                    if (strcmp(s2ch.favList[favSort[j]].subject, s2ch.favList[favSort[i]].subject) < 0)
                    {
                        tmp = favSort[j];
                        favSort[j] = favSort[i];
                        favSort[i] = tmp;
                    }
                }
            }
        }
        break;
    case 1:
        for (i = 0; i < s2ch.fav.count; i++)
        {
            favSort[i] = i;
        }
        for (i = 0; i < s2ch.fav.count-1; i++)
        {
            for (j = i; j < s2ch.fav.count; j++)
            {
                if (strcmp(s2ch.favList[favSort[j]].subject, s2ch.favList[favSort[i]].subject) < 0)
                {
                    tmp = favSort[j];
                    favSort[j] = favSort[i];
                    favSort[i] = tmp;
                }
            }
        }
        break;
    case 2:
        for (i = 0; i < s2ch.fav.count; i++)
        {
            favSort[i] = i;
        }
        for (i = 0; i < s2ch.fav.count-1; i++)
        {
            for (j = i; j < s2ch.fav.count; j++)
            {
                if (s2ch.favList[favSort[j]].dat > s2ch.favList[favSort[i]].dat)
                {
                    tmp = favSort[j];
                    favSort[j] = favSort[i];
                    favSort[i] = tmp;
                }
            }
        }
        break;
    case 3:
        for (i = 0; i < s2ch.fav.count; i++)
        {
            favSort[i] = i;
        }
        for (i = 0; i < s2ch.fav.count-1; i++)
        {
            for (j = i; j < s2ch.fav.count; j++)
            {
                if (s2ch.favList[favSort[j]].dat < s2ch.favList[favSort[i]].dat)
                {
                    tmp = favSort[j];
                    favSort[j] = favSort[i];
                    favSort[i] = tmp;
                }
            }
        }
        break;
	case 4:														// レス数(降順)
		for (i = 0; i < s2ch.fav.count; i++){
			favSort[i] = i;
		}
		for (i = 0; i < s2ch.fav.count-1; i++){
			for (j = i; j < s2ch.fav.count; j++){
				if (s2ch.favList[favSort[j]].res < s2ch.favList[favSort[i]].res){
					tmp = favSort[j];
					favSort[j] = favSort[i];
					favSort[i] = tmp;
				}
			}
		}
		break;
	case 5:														// レス数(昇順)
		for (i = 0; i < s2ch.fav.count; i++){
			favSort[i] = i;
		}
		for (i = 0; i < s2ch.fav.count-1; i++){
			for (j = i; j < s2ch.fav.count; j++){
				if (s2ch.favList[favSort[j]].res > s2ch.favList[favSort[i]].res){
					tmp = favSort[j];
					favSort[j] = favSort[i];
					favSort[i] = tmp;
				}
			}
		}
		break;
    }
}

/****************
ソート用ダイアログ表示
*****************/
#define MAX_SORT_COUNT (6)
static void psp2chFavSortDialog(void)
{
    const unsigned char title[] = "どの項目でソ\ートしますか";
    const unsigned char text1[] = "板名";						// 板名
    const unsigned char text2[] = "スレタイ";					// スレタイ
    const unsigned char text3[] = "作成日(降順)";				// 作成日(降順)
    const unsigned char text4[] = "作成日(昇順)";				// 作成日(昇順)
    const unsigned char text5[] = "レス数(降順)";				// レス数(降順)
    const unsigned char text6[] = "レス数(昇順)";				// レス数(昇順)
    const unsigned char* text[MAX_SORT_COUNT] = {text1, text2, text3, text4, text5, text6};
    int i, select = 0, change = 1;

	pgCreateTexture();
	pgFillvram(THREAD_INDEX + 14, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
    while (s2ch.running)
    {
    	if (change)
    	{
    		change = 0;
	        pgSetDrawStart(180, 53, 0, 0);
	        pgPrint((char*)title, YELLOW, BLUE, SCR_WIDTH);
	        pgSetDrawStart(200, -1, 0, 25);
	        for (i = 0; i < MAX_SORT_COUNT; i++)
	        {
	            if (select == i)
	            {
	            	pgPrint((char*)text[i], WHITE, BLACK, SCR_WIDTH);
	            }
	            else
	            {
	            	pgPrint((char*)text[i], GRAY, 0, SCR_WIDTH);
	            }
	            pgSetDrawStart(200, -1, 0, 20);
	        }
	    }
	    s2ch.oldPad = s2ch.pad;
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if(s2ch.pad.Buttons & PSP_CTRL_UP)
                {
                    if (select)
                    {
                        select--;
                    } else {									// ループ処理
						select = MAX_SORT_COUNT - 1;
					}
                }
                if(s2ch.pad.Buttons & PSP_CTRL_DOWN)
                {
                    if (select < MAX_SORT_COUNT - 1)
                    {
                        select++;
                    } else {									// ループ処理
						select = 0;
					}
                }
                if(s2ch.pad.Buttons & PSP_CTRL_CIRCLE)
                {
                    break;
                }
                if(s2ch.pad.Buttons & PSP_CTRL_CROSS)
                {
                	select = -1;
                    break;
                }
                change = 1;
            }
        }
        pgDrawTexture(-1);
        flipScreen(0);
    }
    pgDeleteTexture();
    return psp2chFavSort(select);
}

/**********************
お気に入りスレの描画
**********************/
static void psp2chDrawFavorite(void)
{
    int start;
    int i, color_sel;
    int scrW, scrH;

    scrW = winParam->width + winParam->viewX;
    scrH = winParam->height;
    start = s2ch.fav.start;
    if (start + winParam->lineEnd > s2ch.fav.count)
    {
        start = s2ch.fav.count - winParam->lineEnd;
    }
    if (start < 0)
    {
        start = 0;
    }
    pgFillvram(s2ch.threadColor.bg[0], winParam->viewX, 0, scrW, winParam->height, 2);
    pgSetDrawStart(-1, 0, 0, 0);
    for (i = start; i < start + winParam->lineEnd; i++)
    {
        if (i >= s2ch.fav.count) {
            return;
        }
        color_sel = 0;
        
        pgSetDrawStart(0, -1, 0, 0);
        if (i == s2ch.fav.select) {
        	color_sel = 1;
            pgFillvram(s2ch.threadColor.bg[color_sel], winParam->viewX, winParam->pgCursorY, scrW, LINE_PITCH, 2);
        }
        pgPrintNumber(i + 1, s2ch.threadColor.num[color_sel], s2ch.threadColor.bg[color_sel]);
        pgSetDrawStart(FONT_HEIGHT * 2, -1, 0, 0);
        pgPrint(s2ch.favList[favSort[i]].title, s2ch.threadColor.category[color_sel], s2ch.threadColor.bg[color_sel], scrW);
        pgSetDrawStart(-1, -1, 8, 0);
        pgPrint(s2ch.favList[favSort[i]].subject, s2ch.threadColor.text1[color_sel], s2ch.threadColor.bg[color_sel], scrW);
        pgSetDrawStart(scrW - FONT_HEIGHT * 2, -1, 0, 0);
        if (s2ch.favList[favSort[i]].update == 1) {
            pgPrintNumber(s2ch.favList[favSort[i]].res, s2ch.threadColor.count2[color_sel], s2ch.threadColor.bg[color_sel]);
        }
        else {
            pgPrintNumber(s2ch.favList[favSort[i]].res, s2ch.threadColor.count1[color_sel], s2ch.threadColor.bg[color_sel]);
        }
        pgSetDrawStart(-1, -1, 0, LINE_PITCH);
    }
}

#define ITA_W (110)
/**********************
お気に入り板の描画
**********************/
static void psp2chDrawFavoriteIta(void)
{
    int start;
    int i, color_sel;
    
    start = s2ch.favIta.start;
    if (start + winParam->lineEnd > s2ch.favIta.count)
    {
        start = s2ch.favIta.count - winParam->lineEnd;
    }
    if (start < 0)
    {
        start = 0;
    }
    pgFillvram(s2ch.threadColor.bg[0], 0, 0, BUF_WIDTH * 2, winParam->height, 2);
    pgSetDrawStart(-1, 0, 0, 0);
    for (i = start; i < start + winParam->lineEnd; i++)
    {
        if (i >= s2ch.favIta.count) {
            return;
        }
        color_sel = 0;
        
        pgSetDrawStart(0, -1, 0, 0);
        if (i == s2ch.favIta.select) {
        	color_sel = 1;
            pgFillvram(s2ch.threadColor.bg[color_sel], 0, winParam->pgCursorY, BUF_WIDTH * 2, LINE_PITCH, 2);
        }
        pgPrintNumber(i + 1, s2ch.threadColor.num[color_sel], s2ch.threadColor.bg[color_sel]);
        pgSetDrawStart(30, -1, 0, 0);
        pgPrint(s2ch.favItaList[i].cate, s2ch.threadColor.category[color_sel], s2ch.threadColor.bg[color_sel], ITA_W);
        pgSetDrawStart(100, -1, 0, 0);
        pgPrint(s2ch.favItaList[i].title, s2ch.threadColor.text1[color_sel], s2ch.threadColor.bg[color_sel], winParam->width);
        pgSetDrawStart(-1, -1, 0, LINE_PITCH);
    }
}
