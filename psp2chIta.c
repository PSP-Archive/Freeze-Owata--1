/*
* $Id: psp2chIta.c 152 2008-09-10 05:53:53Z bird_may_nike $
*/

#include "psp2ch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psp2chNet.h"
#include "psp2chIta.h"
#include "psp2chThread.h"
#include "psp2chFavorite.h"
#include "psp2chMenu.h"
#include "utf8.h"
#include "pg.h"

//#define _GNU_SOURCE
//#define FONT_SCALE 0.667f // for intrafont
#define BOARD_FILE "2channel.brd"
#define BOARD_FILE_EXT "my.brd"									// psp2chRes.cにも同じ指定あり
	
extern S_2CH s2ch; // psp2ch.c
extern char keyWords[128]; // psp2chThread.c
extern Window_Layer *winParam;

static S_2CH_MENU_STR menuCate[2];
static S_2CH_MENU_STR menuIta[2];
static int itaEnd = 0;

extern const char favBoard[];

// prototype
static void psp2chDrawCategory(int start, int select, S_2CH_ITA_COLOR color);
static void psp2chDrawIta(int start, int select, S_2CH_ITA_COLOR color);
static int psp2chReplaceMenu(char* oldhost, char* oldita, char* newhost, char* newita);
static int psp2chReplaceFav(char* oldhost, char* oldita, char* newhost, char* newita);

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

void psp2chItaSetMenuString(const char **sBtnH, const char **sBtnV)
{
    int index1, index2, index3, index4, index5;
    int i, tmp;

    getIndex(s2ch.itaB[0].ok, index1);
    getIndex(s2ch.itaB[0].esc, index2);
    getIndex(s2ch.itaB[0].move, index3);
    getIndex(s2ch.itaB[0].reload, index4);
    getIndex(s2ch.itaB[0].shift, index5);
    sprintf(menuCate[0].main, "　%s : 決定　　%s : 終了　　%s : お気に入り　　%s : 更新　　%s : メニュー切替",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5]);
    sprintf(menuIta[0].main, "　%s : 決定　　%s : 戻る　　%s : お気に入り　　%s : 更新　　%s : メニュー切替",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5]);

    getIndex(s2ch.listB[0].top, index1);
    getIndex(s2ch.listB[0].end, index2);
    getIndex(s2ch.itaB[0].search2ch, index3);
    getIndex(s2ch.itaB[0].addFav, index4);
    sprintf(menuCate[0].sub, "　%s : 先頭　　　%s : 最後　　　%s : 全板検索",
            sBtnH[index1], sBtnH[index2], sBtnH[index3]);
    sprintf(menuIta[0].sub, "　%s : 先頭　　　%s : 最後　　　%s : 全板検索　　　%s : お気に入りに追加",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4]);

    getIndex(s2ch.itaB[1].ok, index1);
    getIndex(s2ch.itaB[1].esc, index2);
    getIndex(s2ch.itaB[1].move, index3);
    getIndex(s2ch.itaB[1].reload, index4);
    getIndex(s2ch.itaB[1].shift, index5);
    sprintf(menuCate[1].main, "　%s : 決定　　　　%s : 終了　　　%s : お気に入り\n　%s : 更新　　　%s : メニュー切替",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5]);
    sprintf(menuIta[1].main, "　%s : 決定　　　　%s : 戻る　　　%s : お気に入り\n　%s : 更新　　　%s : メニュー切替",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5]);

    getIndex(s2ch.listB[1].top, index1);
    getIndex(s2ch.listB[1].end, index2);
    getIndex(s2ch.itaB[1].search2ch, index3);
    getIndex(s2ch.itaB[1].addFav, index4);
    sprintf(menuCate[1].sub, "　%s : 先頭　　　%s : 最後　　　%s : 全板検索",
            sBtnV[index1], sBtnV[index2], sBtnV[index3]);
    sprintf(menuIta[1].sub, "　%s : 先頭　　　%s : 最後　　　%s : 全板検索\n　%s : お気に入りに追加",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4]);
}

/**********************
 カテゴリーと板一覧表示
**********************/
int psp2chIta(void)
{
    static int focus = 0,redraw = 0;
    int ret, rMenu, change = 0;
    char* menuStr;

	if (s2ch.itaList == NULL)
	{
		ret = psp2chItaList();
		if (ret < 0)
		{
			s2ch.sel = 0;
			return ret;
		}
		winParam->viewX = 0;
		winParam->viewY = 0;
		s2ch.oldPad.Buttons = PSP_CTRL_START;
	}
    if (focus)
    {
        s2ch.ita.count = s2ch.categoryList[s2ch.category.select].itaId + s2ch.categoryList[s2ch.category.select].num;
        rMenu = psp2chCursorSet(&s2ch.ita, winParam->lineEnd, s2ch.itaB[winParam->tateFlag].shift, 0, &change);
        menuStr = rMenu ? menuIta[winParam->tateFlag].sub : menuIta[winParam->tateFlag].main;
        if (s2ch.ita.start < s2ch.categoryList[s2ch.category.select].itaId)
        {
            s2ch.ita.start = s2ch.categoryList[s2ch.category.select].itaId;
        }
        if (s2ch.ita.select < s2ch.ita.start || s2ch.ita.count < s2ch.ita.select)
        {
        	s2ch.ita.select = s2ch.ita.start;
        }
    }
    else
    {
    	rMenu = psp2chCursorSet(&s2ch.category, winParam->lineEnd, s2ch.itaB[winParam->tateFlag].shift, 0, &change);
    	menuStr = rMenu ? menuCate[winParam->tateFlag].sub : menuCate[winParam->tateFlag].main;
        // アナログだと超えてしまう場合がある
        if (s2ch.category.select >= s2ch.category.count)
        {
        	s2ch.category.select = s2ch.category.count - 1;
        }
        s2ch.ita.start = s2ch.categoryList[s2ch.category.select].itaId;
        s2ch.ita.select = s2ch.categoryList[s2ch.category.select].itaId;
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
            if (s2ch.pad.Buttons & s2ch.itaB[winParam->tateFlag].addFav)
            {
                if (focus)
                {
                    psp2chAddFavoriteIta(s2ch.categoryList[s2ch.category.select].name, s2ch.itaList[s2ch.ita.select].title);
                }
            }
            else if (s2ch.pad.Buttons & s2ch.itaB[winParam->tateFlag].search2ch)
            {
            	s2ch.ita.count = itaEnd;
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
        }
        else
        {
            // 決定
            if (s2ch.pad.Buttons & s2ch.itaB[winParam->tateFlag].ok)
            {
                if (focus)
                {
                	s2ch.ita.count = itaEnd;
                    s2ch.sel = 3;
					redraw = 1;									// 一覧に戻ってきた時に素早く画面を更新させる
//                    return 0;
                }
                else
                {
                    focus = 1;
                }
            }
            // 戻る、終了
            else if (s2ch.pad.Buttons & s2ch.itaB[winParam->tateFlag].esc)
            {
                if (focus)
                {
                	s2ch.ita.count = itaEnd;
                    focus = 0;
                }
                else
                {
                    if (psp2chOwata())
                    {
                        return 0;
                    }
                }
            }
            // 更新
            else if (s2ch.pad.Buttons & s2ch.itaB[winParam->tateFlag].reload)
            {
                psp2chGetMenu();
                psp2chItaList();
                s2ch.category.start = 0;
                s2ch.category.select = 0;
                s2ch.ita.start = 0;
                s2ch.ita.select = 0;
                focus = 0;
            }
            // お気に入りに移動
            else if (s2ch.pad.Buttons & s2ch.itaB[winParam->tateFlag].move)
            {
                s2ch.sel = 1;
                s2ch.ita.count = itaEnd;
            }
        }
		change = 1;
    }
    // 描画
    if (change || redraw)
    {
		if (s2ch.category.select < s2ch.category.start) s2ch.category.start = s2ch.category.select;
		if (s2ch.category.select >= s2ch.category.start + winParam->lineEnd) s2ch.category.start = s2ch.category.select - winParam->lineEnd +1;
		if (s2ch.ita.select < s2ch.ita.start) s2ch.ita.start = s2ch.ita.select;
		if (s2ch.ita.select >= s2ch.ita.start + winParam->lineEnd) s2ch.ita.start = s2ch.ita.select - winParam->lineEnd +1;
    	if (focus)
		{
			psp2chDrawCategory(s2ch.category.start, s2ch.category.select, s2ch.cateOffColor);
			psp2chDrawIta(s2ch.ita.start, s2ch.ita.select, s2ch.cateOffColor);
		}
		else
		{
			psp2chDrawCategory(s2ch.category.start, s2ch.category.select, s2ch.cateOnColor);
			psp2chDrawIta(s2ch.ita.start, s2ch.ita.select, s2ch.cateOnColor);
		}
		if (s2ch.sel != 3){										// スレに移動する際は実行しない
			redraw = 0;
		}
	}
	pgPrintMenuBar(menuStr);
	pgCopy(0, 0);
	pgCopyMenuBar();
    return 0;
}

/**********************
初回アクセス時等で2channel.brdがない場合psp2chGetMenu()で2channel.brdを作成
2channel.brdを読み込んで s2ch.categoryListとs2ch.itaList構造体のメモリ確保とデータ作成を行う
s2ch.category.countとs2ch.ita.countに総数
**********************/
int psp2chItaList(void)
{
    SceUID fd;
    SceIoStat st, st2;
    char file[FILE_PATH];
    char file2[FILE_PATH];
    char *buf, *p, *q, *r;
    int i, cateOn, err;

    sprintf(file, "%s/%s", s2ch.cfg.logDir, BOARD_FILE);
    i = sceIoGetstat(file, &st);
    if (i < 0)
    {
        psp2chGetMenu();
        i = sceIoGetstat(file, &st);
        if (i< 0)
        {
            psp2chNormalError(FILE_STAT_ERR, file);
            return -1;
        }
    }
    sprintf(file2, "%s/%s", s2ch.cfg.logDir, BOARD_FILE_EXT);
    i = sceIoGetstat(file2, &st2);
    if (i < 0)
    {
        st2.st_size = 0;
    }
    buf = (char*)malloc(st.st_size + st2.st_size + 1);
    if (buf == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, "itaList");
        return -1;
    }
    fd = sceIoOpen(file, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
        free(buf);
        psp2chNormalError(FILE_OPEN_ERR, file);
        return -1;
    }
    psp2chFileRead(fd, buf, st.st_size);
    sceIoClose(fd);
    if (st2.st_size)
    {
        fd = sceIoOpen(file2, PSP_O_RDONLY, FILE_PARMISSION);
        if (fd < 0)
        {
            st2.st_size = 0;
        }
        else
        {
            psp2chFileRead(fd, buf + st.st_size, st2.st_size);
            sceIoClose(fd);
        }
    }
    buf[st.st_size + st2.st_size] = '\0';
    s2ch.category.count = 0;
    s2ch.ita.count = 0;
    p = buf;
    while (*p)
    {
        if (*p == '\t')
        {
            s2ch.ita.count++;
        }
        else
        {
            s2ch.category.count++;
        }
        while (*p++ != '\n')
        {
            ;
        }
    }
    s2ch.categoryList = (S_2CH_CATEGORY*)realloc(s2ch.categoryList, sizeof(S_2CH_CATEGORY) * s2ch.category.count);
    if (s2ch.categoryList == NULL)
    {
        free(buf);
        psp2chNormalError(MEM_ALLC_ERR, "categoryList");
        return -1;
    }
    s2ch.itaList = (S_2CH_ITA*)realloc(s2ch.itaList, sizeof(S_2CH_ITA) * s2ch.ita.count);
    if (s2ch.itaList == NULL)
    {
        free(buf);
        psp2chNormalError(MEM_ALLC_ERR, "itaList");
        return -1;
    }
    s2ch.category.count = 0;
    s2ch.ita.count = 0;
    cateOn = 0;
	p = buf;
	q = buf;
	for (i=0; i<st.st_size + st2.st_size; i++){					// 余計なCRを削除
		if (*p!='\r') *q++ = *p;
		p++;
	}
	*q = '\0';
    i = 0;
    p = buf;
    q = buf;
    err = 0;
    while(*q)
    {
        if (*q == '\t')
        {
            q++;
            p = q;
            q = strchr(p, '\t');
			if (!q){
				err = 1;
				break;
			}
            *q = '\0';
            strcpy(s2ch.itaList[s2ch.ita.count].host, p);
            q++;
            p = q;
            q = strchr(p, '\t');
			if (!q){
				err = 1;
				break;
			}
            *q = '\0';
            strcpy(s2ch.itaList[s2ch.ita.count].dir, p);
            q++;
            p = q;
            q = strchr(p, '\n');
			if (q) *q = '\0';
            r = strchr(p, '\r');
            if (r)
            {
                *r = '\0';
            }
            strcpy(s2ch.itaList[s2ch.ita.count].title, p);
            if (cateOn)
            {
                s2ch.categoryList[s2ch.category.count - 1].itaId = s2ch.ita.count;
                cateOn = 0;
            }
            s2ch.ita.count++;
            s2ch.categoryList[s2ch.category.count - 1].num++;
			if (q){
				q++;
			} else {
				break;
			}
        }
        else
        {
            p = q;
            q = strchr(p, '\t');
			if (q) *q = '\0';
            strcpy(s2ch.categoryList[s2ch.category.count].name, p);
            s2ch.categoryList[s2ch.category.count].num = 0;
            s2ch.category.count++;
            cateOn = 1;
			if (q){
				q++;
				p = q;
			}
            q = strchr(p, '\n');
			if (!q){
				err = 1;
				break;
			}
            q++;
        }
    }
    free(buf);
    itaEnd = s2ch.ita.count;
	if (err){
		psp2chErrorDialog(0, "2channel.brdかMY.BRDに異常が見つかりました");
	}
    return 0;
}

/**********************
2channel.brdを作成
**********************/
int psp2chGetMenu(void)
{
    char url[FILE_PATH], path[NET_PATH_LENGTH], tmpbuf[TMP_BUF_SIZE];
    const char* menustart = "<BR><B>";
    const char* menuend = "</B><BR>";
    const char* itastart = "<A HREF=";
    const char* hostend = "/";
    const char* dirend = "/>";
    const char* titleend = "</A>";
    S_NET net;
    SceUID fd;
    int ret, menuOn, tmplen;
    char buf[BUF_LENGTH];
    char menu[64];
    char itahost[32];
    char itadir[32];
    char *p, *q, *line, *r;

	memset(&net, 0, sizeof(S_NET));
	sprintf(url, "http://%s", s2ch.cfg.bbsmenuAddr);
	//接続開始    
    ret = psp2chGet(url, NULL, NULL, &net, 0);
    if (ret < 0)
    {
        return ret;
    }
    switch(net.status)
    {
        case 200:
            break;
        default:
        	sprintf(err_msg, "%d", net.status);
            psp2chNormalError(NET_GET_ERR, err_msg);
            free(net.body);
            return -1;
    }
    sprintf(path, "%s/%s", s2ch.cfg.logDir, BOARD_FILE);
    fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
    if (fd < 0)
    {
        psp2chNormalError(FILE_OPEN_ERR, path);
        return fd;
    }
    menuOn = 0;
    q = net.body;
    tmplen = 0;
    while(*q)
    {
        p = strchr(q, '\n');
        if (p == NULL)
        {
            break;
        }
        *p = '\0';
        strcpy(buf, q);
        q = p + 1;
        if ((r = strstr(buf, menustart)) != NULL && (p = strstr(buf, menuend)) != NULL)
        {
			//カテゴリの先頭
            *p = '\0';
            sprintf(menu, "%s\t0\n", r + strlen(menustart));
            menuOn = 1;
        }
        else if (menuOn)
        {
            if (strstr(buf, itastart) != buf)
            {
//                menuOn = 0;									// 板のリスト中に空行があるとそれ以後が読めないので
                continue;
            }
            //change
            if((p = strstr(buf, "http://")) == NULL)
            {
            	continue;
            }
            line = p + 7;
            if ((p = strstr(line, hostend)) == NULL)
            {
                continue;
            }
            *p = '\0';
            strcpy(itahost, line);
            line = p + 1;
            //add hrefのオプションを無視
            /*if((p = strstr(line, "/ ")) != NULL)
            {
            	r = strstr(line, ">") + 1;
            }
            else if ((p = strstr(line, dirend)) != NULL)
            {
                r = p + 2;
            }
            else
            {
            	continue;
            }*/
            if ((p = strstr(line, dirend)) == NULL)
            {
                continue;
            }
            *p = '\0';
            strcpy(itadir, line);
            //line = r;
            line = p + 2;
            if ((p = strstr(line, titleend)) == NULL)
            {
                continue;
            }
            *p = '\0';
            if (menuOn == 1)
            {
            	// バッファ動作に変更 by KMA
                if(tmplen + strlen(menu) > TMP_BUF_SIZE)
                {
                	psp2chFileWrite(fd, tmpbuf, tmplen);
                	tmplen = 0;
                	tmpbuf[0] = '\0';
                }
                memcpy(tmpbuf + tmplen, menu, strlen(menu));
                tmplen += strlen(menu);
                menuOn = 2;
            }
            sprintf(buf, "\t%s\t%s\t%s\n", itahost, itadir, line);
            if(tmplen + strlen(buf) > TMP_BUF_SIZE)
            {
            	psp2chFileWrite(fd, tmpbuf, tmplen);
            	tmplen = 0;
            	tmpbuf[0] = '\0';
            }
            memcpy(tmpbuf + tmplen, buf, strlen(buf));
            tmplen += strlen(buf);
        }
    }
    if(tmplen != 0)
    {
    	psp2chFileWrite(fd, tmpbuf, tmplen);
    }
    sceIoClose(fd);
    free(net.body);
    return 0;
}

/**********************
カテゴリー一覧を表示
start: 表示開始位置
select: 選択位置
**********************/
#define CATEGORY_W (100)
#define ITA_W (110)
static void psp2chDrawCategory(int start, int select, S_2CH_ITA_COLOR c)
{
    int i;
    
    if (start + winParam->lineEnd > s2ch.category.count)
    {
        start = s2ch.category.count - winParam->lineEnd;
    }
    pgFillvram(c.cate.bg, 0, 0, CATEGORY_W, winParam->height, 2);
    pgSetDrawStart(-1, 0, 0, 0);
    for (i = start; i < start + winParam->lineEnd; i++)
    {
        pgSetDrawStart(10, -1, 0, 0);
        if (i == select)
        {
        	pgFillvram(c.cate.s_bg, 0, winParam->pgCursorY, CATEGORY_W, LINE_PITCH, 2);
            pgPrint(s2ch.categoryList[i].name, c.cate.s_text, c.cate.s_bg, winParam->width);
        }
        else
        {
            pgPrint(s2ch.categoryList[i].name, c.cate.text, c.cate.bg, winParam->width);
        }
        pgSetDrawStart(-1, -1, 0, LINE_PITCH);
    }
}

/**********************
板一覧を表示
**********************/
static void psp2chDrawIta(int start, int select, S_2CH_ITA_COLOR c)
{
    int i, end;

    end = s2ch.categoryList[s2ch.category.select].itaId + s2ch.categoryList[s2ch.category.select].num;
    if (start + winParam->lineEnd < end)
    {
        end = start + winParam->lineEnd;
    }
    pgFillvram(c.ita.bg, CATEGORY_W, 0, ITA_W, winParam->height, 2);
    pgFillvram(c.base, CATEGORY_W + ITA_W, 0, winParam->width - CATEGORY_W - ITA_W, winParam->height, 2);
    pgSetDrawStart(-1, 0, 0, 0);
    for (i = start; i < end; i++)
    {
        pgSetDrawStart(CATEGORY_W + 10, -1, 0, 0);
        if (i == select)
        {
        	pgFillvram(c.ita.s_bg, CATEGORY_W, winParam->pgCursorY, ITA_W, LINE_PITCH, 2);
            pgPrint(s2ch.itaList[i].title, c.ita.s_text, c.ita.s_bg, CATEGORY_W + ITA_W);
        }
        else
        {
            pgPrint(s2ch.itaList[i].title, c.ita.text, c.ita.bg, CATEGORY_W + ITA_W);
        }
        pgSetDrawStart(-1, -1, 0, LINE_PITCH);
    }
}

/**********************
移転書き換え
**********************/
// 2channel.brdの書き換え
static int psp2chReplaceMenu(char* oldhost, char* oldita, char* newhost, char* newita)
{
    SceUID fd;
	char file[FILE_PATH];
	char match[BUF_LENGTH];
    SceIoStat st;
    char *buf, *start, *end, *p;
	int ret;

    sprintf(file, "%s/%s", s2ch.cfg.logDir, BOARD_FILE);
    ret = sceIoGetstat(file, &st);
    if (ret < 0)
    {
        return ret;
    }
    buf = (char*)malloc(st.st_size + 1);
    if (buf == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }
	fd = sceIoOpen(file, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
		free(buf);
        psp2chNormalError(FILE_OPEN_ERR, file);
        return fd;
    }
	psp2chFileRead(fd, buf, st.st_size);
	sceIoClose(fd);
	sprintf(match, "%s\t%s\t", oldhost, oldita);
	start = buf;
	if ((p = strstr(start, match)))
	{
		fd = sceIoOpen(file, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
		if (fd < 0)
		{
			free(buf);
			psp2chNormalError(FILE_OPEN_ERR, file);
			return fd;
		}
		while (*start && (end = strchr(start, '\n')))
		{
			*end = '\0';
			if ((p = strstr(start, match)))
			{
				start = p + strlen(match);
				sprintf(match, "\t%s\t%s\t", newhost, newita);
				psp2chFileWrite(fd, match, strlen(match));
			}
			psp2chFileWrite(fd, start, strlen(start));
			psp2chFileWrite(fd, "\n", strlen("\n"));
			start = end + 1;
		}
		sceIoClose(fd);
	}
	free(buf);
	return 0;
}

// favorite.brdの書き換え
static int psp2chReplaceFav(char* oldhost, char* oldita, char* newhost, char* newita)
{
    SceUID fd;
	char file[FILE_PATH];
	char match[BUF_LENGTH];
    SceIoStat st;
    char *buf, *start, *end, *p;
	int ret;

    sprintf(file, "%s/%s", s2ch.cfg.logDir, favBoard);
    ret = sceIoGetstat(file, &st);
    if (ret < 0)
    {
        return ret;
    }
    buf = (char*)malloc(st.st_size + 1);
    if (buf == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }
	fd = sceIoOpen(file, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
		free(buf);
        psp2chNormalError(FILE_OPEN_ERR, file);
        return fd;
    }
	psp2chFileRead(fd, buf, st.st_size);
	buf[st.st_size] = '\0';
	sceIoClose(fd);
	sprintf(match, "%s\t%s\t", oldhost, oldita);
	start = buf;
	if (strstr(start, match) != NULL)
	{
		fd = sceIoOpen(file, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
		if (fd < 0)
		{
			free(buf);
			psp2chNormalError(FILE_OPEN_ERR, file);
			return fd;
		}
		while (*start && (end = strchr(start, '\n')))
		{
			*end = '\0';
			if ((p = strstr(start, match)))
			{
				start = p + strlen(match);
				sprintf(match, "%s\t%s\t", newhost, newita);
				psp2chFileWrite(fd, match, strlen(match));
			}
			psp2chFileWrite(fd, start, strlen(start));
			psp2chFileWrite(fd, "\n", strlen("\n"));
			start = end + 1;
		}
		sceIoClose(fd);
	}
	free(buf);
	return 0;
}

/**********************
移転チェック
index.htmlを読み込んでJavascriptのlocationがあるかチェックする
locationがあればそのURLからhostと板ディレクトリを取得して書き換え関数へ送る
戻り値　0：移転あり書き換え成功
**********************/
int psp2chItenCheck(char* host, char* ita)
{
	const char* needle = "\nwindow.location.href=\"http://";
    S_NET net;
	int ret;
    char *newhost, *newita, *p;
	char buf[BUF_LENGTH];
	
	sprintf(buf, "http://%s/%s/index.html", host, ita);
    ret = psp2chGet(buf, NULL, NULL, &net, 0);
    if (ret < 0)
    {
        return ret;
    }
    switch(net.status)
    {
        case 200:
            break;
        default:
        	free(net.body);
            return -1;
    }
	newhost = strstr(net.body, needle);
	if (newhost == NULL)
	{
		return -1;
	}
	newhost += strlen(needle);
	p = strchr(newhost, '/');
	if (p == NULL)
	{
		return -1;
	}
	*p = '\0';
	newita = p + 1;
	p = strchr(newita, '/');
	if (p == NULL)
	{
		return -1;
	}
	*p = '\0';
    pgPrintMenuBar("移転先をチェック中");
	pgCopyMenuBar();
    flipScreen(0);
	psp2chReplaceMenu(host, ita, newhost, newita);
	psp2chReplaceFav(host, ita, newhost, newita);
	free(net.body);
	// リストの解放
	free(s2ch.itaList);
	s2ch.itaList = NULL;
	free(s2ch.favList);
	s2ch.favList = NULL;
	free(s2ch.threadList);
	s2ch.threadList = NULL;
	free(s2ch.findList);
	s2ch.findList = NULL;
	return 0;
}
