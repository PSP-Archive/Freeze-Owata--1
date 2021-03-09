/*
* $Id: psp2chSearch.c 148 2008-08-27 06:31:27Z bird_may_nike $
*/

#include "psp2ch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pg.h"
#include "psp2chNet.h"
#include "psp2chSearch.h"
#include "psp2chIta.h"
#include "psp2chThread.h"
#include "psp2chFavorite.h"
#include "psp2chRes.h"
#include "psp2chForm.h"
#include "charConv.h"

extern S_2CH s2ch; // psp2ch.c
extern char keyWords[128]; // psp2chThread.c
extern int preLine; // psp2chRes.c
extern Window_Layer *winParam;

static S_2CH_MENU_STR menuSearch[2];

// prototype
static int psp2chSearchList(void);
static void psp2chDrawSearch(void);
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

void psp2chSearchSetMenuString(const char **sBtnH, const char **sBtnV)
{
    int index1, index2, index3, index4, index5;
    int i, tmp;

    getIndex(s2ch.findB[0].ok, index1);
    getIndex(s2ch.findB[0].ita, index2);
    getIndex(s2ch.findB[0].fav, index3);
    getIndex(s2ch.findB[0].esc, index4);
    getIndex(s2ch.findB[0].shift, index5);
    sprintf(menuSearch[0].main, "　%s : 決定　　　%s : 板一覧　　　%s : お気に入り　　　%s : 戻る　　　%s : メニュー切替",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5]);

    getIndex(s2ch.listB[0].top, index1);
    getIndex(s2ch.listB[0].end, index2);
    getIndex(s2ch.findB[0].search2ch, index3);
    sprintf(menuSearch[0].sub, "　%s : 先頭　　　%s : 最後　　　　%s : 全板検索",
            sBtnH[index1], sBtnH[index2], sBtnH[index3]);

    getIndex(s2ch.findB[1].ok, index1);
    getIndex(s2ch.findB[1].ita, index2);
    getIndex(s2ch.findB[1].fav, index3);
    getIndex(s2ch.findB[1].esc, index4);
    getIndex(s2ch.findB[1].shift, index5);
    sprintf(menuSearch[1].main, "　%s : 決定　　　　　%s : 板一覧　　　　%s : お気に入り\n　%s : 戻る　　　%s : メニュー切替",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5]);

    getIndex(s2ch.listB[1].top, index1);
    getIndex(s2ch.listB[1].end, index2);
    getIndex(s2ch.findB[1].search2ch, index3);
    sprintf(menuSearch[1].sub, "　%s : 先頭　　　%s : 最後　　　　%s : 全板検索",
            sBtnV[index1], sBtnV[index2], sBtnV[index3]);
}

/*********************
2ちゃんねる検索
**********************/
int psp2chSearch(int retSel)
{
	static int ret = 0;
    char* menuStr;
    int rMenu, change;

	if (s2ch.findList == NULL)
	{
		if (psp2chSearchList() < 0)
		{
			s2ch.sel = retSel;
			return 0;
		}
		ret = retSel;
		s2ch.find.start = 0;
		s2ch.find.select = 0;
	}
    rMenu = psp2chCursorSet(&s2ch.find, winParam->lineEnd, s2ch.findB[winParam->tateFlag].shift, 1, &change);
    menuStr = (rMenu) ? menuSearch[winParam->tateFlag].sub : menuSearch[winParam->tateFlag].main;
    if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
    {
        if (s2ch.pad.Buttons & PSP_CTRL_SELECT)
        {
            psp2chSetScreenParam(1);
        }
        if (rMenu)
        {
            if(s2ch.pad.Buttons & s2ch.findB[winParam->tateFlag].search2ch)
            {
                if (psp2chThreadSearch() == 0 && keyWords[0])
                {
                    if (s2ch.findList)
                    {
                        free(s2ch.findList);
                        s2ch.findList = NULL;
                    }
                    return 0;
                }
            }
        }
        else
        {
            if(s2ch.pad.Buttons & s2ch.findB[winParam->tateFlag].ok)
            {
                free(s2ch.resList);
                s2ch.resList = NULL;
                preLine = -2;
                s2ch.sel = 8;
                return 0;
            }
            else if(s2ch.pad.Buttons & s2ch.findB[winParam->tateFlag].esc)
            {
                s2ch.sel = ret;
            }
            else if(s2ch.pad.Buttons & s2ch.findB[winParam->tateFlag].fav)
            {
                s2ch.sel = 1;
            }
            else if(s2ch.pad.Buttons & s2ch.findB[winParam->tateFlag].ita)
            {
                s2ch.sel = 2;
            }
        }
        change = 1;
    }
    if (change)
    {
		psp2chDrawSearch();
		pgPrintMenuBar(menuStr);
	}
	pgCopy(winParam->viewX, 0);
	pgCopyMenuBar();
    return 0;
}

/**********************
検索キーワードをURLエンコードしてQUERY STRINGとしてGET送信
受信したhtmlを解析してリスト(s2ch.findList)に登録
**********************/
static int psp2chSearchList(void)
{
    char euc[128];
    char buf[128*3];
    char query[512];
    int count, offset;
    int ret, i;
    S_NET net;
    char in;
    char *p, *q, *r;

    count = s2ch.cfg.findMax;
    offset = 0;
    keyWords[127] = '\0';
    psp2chSjisToEuc(euc, keyWords);
    psp2chUrlEncode(buf, euc);
    sprintf(query, "http://find.2ch.net/?STR=%s&COUNT=%d&OFFSET=%d", buf, count, offset);
    ret = psp2chGet(query, NULL, NULL, &net, 0);
    if (ret < 0)
    {
        return ret;
    }
    switch(net.status)
    {
        case 200: // OK
            break;
        default:
        	sprintf(err_msg, "%d", net.status);
            psp2chNormalError(NET_GET_ERR, err_msg);
            free(net.body);
            return -1;
    }
    s2ch.findList = (S_2CH_FAVORITE*)realloc(s2ch.findList, sizeof(S_2CH_FAVORITE) * s2ch.cfg.findMax);
    if (s2ch.findList == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }
    s2ch.find.count = 0;
    r = net.body;
    while((in = *r++))
    {
        if (in != '<') continue;
        if (!(in = *r++)) break;
        if (in != 'd') continue;
        if (!(in = *r++)) break;
        if (in != 't') continue;
        if (!(in = *r++)) break;
        if (in != '>') continue;
        if (!(in = *r++)) break;
        if (in != '<') continue;
        if (!(in = *r++)) break;
        if (in != 'a') continue;
        // parse html
        i = 0;
        memset(query, 0, sizeof(query));
        while((in = *r++))
        {
            query[i] = in;
            i++;
            if (strstr(query, "</dt>") || i >= 512)
            {
                break;
            }
        }
        query[i] = '\0';
        // host
        p = strstr(query, "http://");
        if (p == NULL) break;
        p += 7;
        q = strchr(p, '/');
        if (q == NULL) break;
        *q = '\0';
        strcpy(s2ch.findList[s2ch.find.count].host, p);
        // dir
        q++;
        p = strstr(q, "read.cgi/");
        if (p == NULL) break;
        p += 9;
        q = strchr(p, '/');
        if (q == NULL) break;
        *q = '\0';
        strcpy(s2ch.findList[s2ch.find.count].dir, p);
        // dat
        q++;
        s2ch.findList[s2ch.find.count].dat = strtol(q, NULL, 10);
        // subject
        p = strchr(q, '>');
        if (p == NULL) break;
        p++;
        q = strchr(p, '<');
        if (q == NULL) break;
        *q = '\0';
        psp2chEucToSjis(s2ch.findList[s2ch.find.count].subject, p);
        // ita title
        q++;
        p = strstr(q, "<a");
        if (p == NULL) break;
        p = strchr(p, '>');
        if (p == NULL) break;
        p++;
        q = strchr(p, '<');
        if (q == NULL) break;
        q -= 2; // 板を削除
        *q = '\0';
        psp2chEucToSjis(s2ch.findList[s2ch.find.count].title, p);
        s2ch.find.count++;
        if (s2ch.find.count >= s2ch.cfg.findMax)
        {
            break;
        }
    }
    free(net.body);
    return 0;
}

/**********************
検索結果の一覧を表示
**********************/
static void psp2chDrawSearch(void)
{
    int start;
    int i, color_sel;
    char buf[32];
    int scrW, scrH;

    scrW = winParam->width + winParam->viewX;
    scrH = winParam->height;
    start = s2ch.find.start;
    if (start + winParam->lineEnd > s2ch.find.count) {
        start = s2ch.find.count - winParam->lineEnd;
    }
    if (start < 0) {
        start = 0;
    }
    pgFillvram(s2ch.threadColor.bg[0], winParam->viewX, 0, scrW, winParam->height, 2);
    pgSetDrawStart(-1, 0, 0, 0);
    for (i = start; i < start + winParam->lineEnd; i++)
    {
        if (i >= s2ch.find.count) {
            return;
        }
        color_sel = 0;
        
        pgSetDrawStart(0, -1, 0, 0);
        sprintf(buf, "%4d", i + 1);
        if (i == s2ch.find.select) {
        	color_sel = 1;
            pgFillvram(s2ch.threadColor.bg[1], winParam->viewX, winParam->pgCursorY, scrW, LINE_PITCH, 2);
        }
        pgPrintNumber(i + 1, s2ch.threadColor.num[color_sel], s2ch.threadColor.bg[color_sel]);
        pgSetDrawStart(FONT_HEIGHT * 2, -1, 0, 0);
        pgPrint(s2ch.findList[i].title, s2ch.threadColor.category[color_sel], s2ch.threadColor.bg[color_sel], scrW);
        pgSetDrawStart(-1, -1, 8, 0);
        pgPrint(s2ch.findList[i].subject, s2ch.threadColor.text1[color_sel], s2ch.threadColor.bg[color_sel], scrW);
        pgSetDrawStart(-1, -1, 0, LINE_PITCH);
    }
}
