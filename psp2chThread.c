/*
* $Id: psp2chThread.c 152 2008-09-10 05:53:53Z bird_may_nike $
*/

#include "psp2ch.h"
#include <pspgu.h>
#include <psputility.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psp2chNet.h"
#include "psp2chIta.h"
#include "psp2chThread.h"
#include "psp2chFavorite.h"
#include "psp2chRes.h"
#include "psp2chMenu.h"
#include "utf8.h"
#include "pg.h"
#include "psp2chForm.h"
#include "charConv.h"
#include "oniguruma.h"
#include "psp2chReg.h"

#define SUBJECT_FILE "subject.txt"

typedef struct {
	unsigned char *str;
	OnigRegion *region;
} reg_data;

extern S_2CH s2ch; // psp2ch.c
extern int preLine; // psp2chRes.c
extern char keyWords[]; //psp2ch.c
extern const char* ngThreadFile; // psp2chMenu.c
extern Window_Layer *winParam;

int* threadSort = NULL;
int threadSortType = 0;

static S_2CH_MENU_STR menuThread[2];

// prototype
static void psp2chThreadSend(char* host, char* dir, char* title);
static int psp2chThreadList(int ita);
static int psp2chGetSubject(int ita);
static void psp2chThreadSort(void);
static void psp2chDrawThread(void);
static int psp2chSubjectParser(unsigned char *str, const unsigned char *pattern);

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

void psp2chThreadSetMenuString(const char **sBtnH, const char **sBtnV)
{
    int index1, index2, index3, index4, index5;
    int i, tmp;

    getIndex(s2ch.thB[0].ok, index1);
    getIndex(s2ch.thB[0].esc, index2);
    getIndex(s2ch.thB[0].reload, index3);
    getIndex(s2ch.thB[0].move, index4);
    getIndex(s2ch.thB[0].shift, index5);
    sprintf(menuThread[0].main, "　%s : 決定　　%s : 戻る　　%s : 更新　　%s : お気に入り　　%s : メニュー切替",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5]);

    getIndex(s2ch.listB[0].top, index1);
    getIndex(s2ch.listB[0].end, index2);
    getIndex(s2ch.thB[0].sort, index3);
    getIndex(s2ch.thB[0].search, index4);
    getIndex(s2ch.thB[0].search2ch, index5);
    sprintf(menuThread[0].sub, "　%s : 先頭　　%s : 最後　　%s : ソ\ート　　× : ログ削除　　%s : 検索　　%s : 全板検索",
            sBtnH[index1], sBtnH[index2], sBtnH[index3], sBtnH[index4], sBtnH[index5]);

    getIndex(s2ch.thB[1].ok, index1);
    getIndex(s2ch.thB[1].esc, index2);
    getIndex(s2ch.thB[1].reload, index3);
    getIndex(s2ch.thB[1].move, index4);
    getIndex(s2ch.thB[1].shift, index5);
    sprintf(menuThread[1].main, "　%s : 決定　　　%s : 戻る　　　%s : 更新\n　%s : お気に入り　　　%s : メニュー切替",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5]);

    getIndex(s2ch.listB[1].top, index1);
    getIndex(s2ch.listB[1].end, index2);
    getIndex(s2ch.thB[1].sort, index3);
    getIndex(s2ch.thB[1].search, index4);
    getIndex(s2ch.thB[1].search2ch, index5);
    sprintf(menuThread[1].sub, "　%s : 先頭　　　 %s : 最後　　　%s : ソ\ート\n　× : ログ削除　%s : 検索　　　%s : 全板検索",
            sBtnV[index1], sBtnV[index2], sBtnV[index3], sBtnV[index4], sBtnV[index5]);
}

//==============================================================
// ログのディレクトリパスを生成
//--------------------------------------------------------------
// s2ch.cfg.hblの指定に基づきディレクトリパスを生成する。
// したらば掲示板の場合、ディレクトリ名称に'/'が含まれてしまうためこれを'-'に置き換える
//--------------------------------------------------------------

static void psp2chThreadLogPath(char *path, int ita)
{
	int		i,pos;
	char	buf[2] = {0,0};

	strcpy( path, s2ch.cfg.logDir );
	strcat( path, "/" );
	if (s2ch.cfg.hbl){
		for (i=0; i<strlen(s2ch.itaList[ita].dir) ;i++){
			buf[0] = s2ch.itaList[ita].dir[i];
			if (buf[0] == '/') buf[0] = '-';
			strcat( path, buf );
		}
	} else {
		strcat( path, s2ch.itaList[ita].title );
	}
}


/****************
 スレッド一覧表示
*****************/
int psp2chThread(int retSel)
{
    char* menuStr;
    static int ret = 0,redraw = 1;
    int i, rMenu, change;

	if (!ret || s2ch.threadList==NULL) {
		if (psp2chThreadList(s2ch.ita.select) < 0) {
			s2ch.sel = retSel;
			return -1;
		}
		ret = retSel;
		s2ch.thread.start = 0;
		s2ch.thread.select = 0;
		preLine = -2;
	}
	
	rMenu = psp2chCursorSet(&s2ch.thread, winParam->lineEnd, s2ch.thB[winParam->tateFlag].shift, 1, &change);
	menuStr = rMenu ? menuThread[winParam->tateFlag].sub : menuThread[winParam->tateFlag].main;
	
    if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
    {
        if (s2ch.pad.Buttons & PSP_CTRL_SELECT)
        {
            psp2chSetScreenParam(1);
        }
        // STARTボタン
        else if(s2ch.pad.Buttons & PSP_CTRL_START)
        {
            if (psp2chMenu(NULL)){								// スレタイNGワードに変化があったなら
				psp2chThreadList(s2ch.ita.select);				// スレ一覧を更新
				s2ch.thread.start = 0;
				s2ch.thread.select = 0;
			}
        }
        else if (rMenu)
        {
            // ソート
            if (s2ch.pad.Buttons & s2ch.thB[winParam->tateFlag].sort)
            {
                psp2chThreadSort();
            }
            // 検索
            else if (s2ch.pad.Buttons & s2ch.thB[winParam->tateFlag].search)
            {
                if (psp2chThreadSearch() == 0 && keyWords[0])
                {
                    psp2chSort(10);
                }
            }
            // 2ちゃんねる検索
            else if (s2ch.pad.Buttons & s2ch.thB[winParam->tateFlag].search2ch)
            {
                if (psp2chThreadSearch() == 0 && keyWords[0])
                {
                	ret = 0;
                    if (s2ch.findList)
                    {
                        free(s2ch.findList);
                        s2ch.findList = NULL;
                    }
                    s2ch.sel = 7;
                }
            }
            // スレ立て
            else if (s2ch.pad.Buttons & PSP_CTRL_LTRIGGER)
            {
            	psp2chThreadSend(s2ch.itaList[s2ch.ita.select].host, s2ch.itaList[s2ch.ita.select].dir, s2ch.itaList[s2ch.ita.select].title);
				if (s2ch.cfg.threadSearch){						// 今立てたスレを検索する
					if (psp2chGetSubject(s2ch.ita.select) == -2)	// スレ一覧を更新
					{
						// 移転
						s2ch.sel = -1;
						ret = 0;
						return 0;
					}
					psp2chThreadList(s2ch.ita.select);				// スレ一覧を再構築
					s2ch.thread.start = 0;
					s2ch.thread.select = 0;
					for (i=0; i<s2ch.thread.count ;i++){			// スレ一覧の中からスレ立てしたタイトルと同じモノを検索
						int len;
						len = strspn(s2ch.threadList[i].title,keyWords);
						if (strlen(s2ch.threadList[i].title)==len){
							s2ch.thread.start = i;					// これだと一覧の最後の方で見つかると少々問題になるけど、まぁ大丈夫だろ
							s2ch.thread.select = i;
							break;
						}
					}
				}
            }
            // ログ削除
            else if (s2ch.pad.Buttons & PSP_CTRL_CROSS)
            {
            	if (psp2chErrorDialog(2, TEXT_5) == PSP_UTILITY_MSGDIALOG_RESULT_YES)
				{
					char buf[20],path[FILE_PATH],path2[FILE_PATH];
					psp2chThreadLogPath( path, s2ch.ita.select );
					sprintf( buf, "/%d", s2ch.threadList[threadSort[s2ch.thread.select]].dat );
					strcat( path, buf );
					strcpy( path2, path );
					strcat( path2, ".dat" );
					sceIoRemove(path2);
					strcpy( path2, path );
					strcat( path2, ".idx" );
					sceIoRemove(path2);
					s2ch.threadList[threadSort[s2ch.thread.select]].old = 0;
				}
            }
        }
        else
        {
            // レス表示
            if (s2ch.pad.Buttons & s2ch.thB[winParam->tateFlag].ok)
            {
				psp2chDrawThread();
				pgPrintMenuBar(menuStr);
				pgCopy(winParam->viewX, 0);
				pgCopyMenuBar();								// 画面を更新
                free(s2ch.resList);
                s2ch.resList = NULL;
                preLine = -2;
                s2ch.sel = 5;
				redraw = 1;										// 一覧へ戻ってきた時に素早く画面を更新させる
                return 0;
            }
            // 戻る
            else if (s2ch.pad.Buttons & s2ch.thB[winParam->tateFlag].esc)
            {
                s2ch.sel = ret;
                ret = 0;
				redraw = 1;										// 一覧へ戻ってきた時に素早く画面を更新させる
                return 0;
            }
            // スレ一覧の更新
            else if (s2ch.pad.Buttons & s2ch.thB[winParam->tateFlag].reload)
            {
                if (psp2chGetSubject(s2ch.ita.select) == -2)
				{
					// 移転
					s2ch.sel = -1;
					ret = 0;
					return 0;
				}
                psp2chThreadList(s2ch.ita.select);
                s2ch.thread.start = 0;
                s2ch.thread.select = 0;
            }
            // お気に入りへ移動
            else if (s2ch.pad.Buttons & s2ch.thB[winParam->tateFlag].move)
            {
                s2ch.sel = 1;
                ret = 0;
                return 0;
            }
        }
        change = 1;
    }
    // 描画
    if (s2ch.cfg.threadRoll || (change || redraw))				// カーソル行をスクロールさせるなら毎回更新
    {
		psp2chDrawThread();
		pgPrintMenuBar(menuStr);
		redraw = 0;
	}
	pgCopy(winParam->viewX, 0);
	pgCopyMenuBar();
    return 0;
}

static void psp2chThreadSend(char* host, char* dir, char* title)
{
	char* message;
	char subject[BUF_LENGTH];
	const unsigned char text1[] = "新しいスレのタイトルを入力してください";
	char* text2 = "タイトル";

	message = (char*)malloc(sizeof(char) * RES_MESSAGE_LENGTH);
	if (message == NULL)
	{
		psp2chNormalError(MEM_ALLC_ERR, "Form Message");
		return;
	}
	memset(message, '\0', RES_MESSAGE_LENGTH);
	if (psp2chInputDialog(text1, text2, NULL) < 0)
		return;
	strcpy(subject, keyWords);
	psp2chForm(host, dir, title, 0, subject, message, NULL);
	preLine = -2;
}

/****************
メモステに保存されたsubject.txtを読み込んで
threadList構造体を作成
番号順にソート
*****************/
static int psp2chThreadList(int ita)
{
    SceUID fd, dfd;
    SceIoStat st;
    SceIoDirent dir;
    char file[FILE_PATH], subject[BUF_LENGTH], thread[BUF_LENGTH];
    S_2CH_THREAD* temp;
    char *buf, *p, *q;
    int i, j, tmpCount, setFlag, code, logMode = s2ch.cfg.logDisplay;
	unsigned long dat = 0;

	if (logMode == 2)
		s2ch.cfg.logDisplay = 0;
	
	psp2chThreadLogPath( file, ita );
	strcat( file, "/" );
	strcat( file, SUBJECT_FILE );
//    sprintf(file, "%s/%s/%s", s2ch.cfg.logDir, s2ch.itaList[ita].title, SUBJECT_FILE);
    i = sceIoGetstat(file, &st);
    if (i < 0)
    {
        if (psp2chGetSubject(ita) < 0)
        {
            return -1;
        }
        i = sceIoGetstat(file, &st);
        if (i< 0)
        {
            psp2chNormalError(FILE_STAT_ERR, file);
            return -1;
        }
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
        return -1;
    }
    psp2chFileRead(fd, buf, st.st_size);
    sceIoClose(fd);
    buf[st.st_size] = '\0';
    s2ch.thread.count = 0;
    p = buf;
    while (*p)
    {
        if (*p++ == '\n')
        {
            s2ch.thread.count++;
        }
    }
    s2ch.thread.count -= 2;										// subject.txtに記載されているスレッドの総数
	psp2chThreadLogPath( file, ita );
//    sprintf(file, "%s/%s", s2ch.cfg.logDir, s2ch.itaList[ita].title);
    dfd = sceIoDopen(file);
    if (dfd >= 0)
    {
        memset(&dir, 0, sizeof(dir)); // 初期化しないとreadに失敗する
        while (sceIoDread(dfd, &dir) > 0)
        {
            s2ch.thread.count++;
        }
        sceIoDclose(dfd);
    }
    temp = (S_2CH_THREAD*)realloc(s2ch.threadList, sizeof(S_2CH_THREAD) * s2ch.thread.count);
    if (temp == NULL )
    {
        free(buf);
        psp2chNormalError(MEM_ALLC_ERR, "threadList");
        return -1;
    }
    s2ch.threadList = temp;										// subject.txt記載分と板フォルダ内にあるスレッドを読み込むのに十分なサイズ
    s2ch.thread.count = 0;
    q = buf;
    while (*q++ != '\n');
    while (*q++ != '\n');										// subject.txt最初の２行は無関係
    //add subjectの形式に合わせてパーサを切り替え
    psp2chRegPatGet(s2ch.itaList[ita].host, &code, subject, thread);
	if (code)
		psp2chEucToSjis(q, q);
	psp2chSubjectParser((unsigned char*)q, (unsigned char*)subject);	// s2ch.thread.countのカウントを行いつつs2ch.threadList[]にスレッド情報入力
    free(buf);
    tmpCount = s2ch.thread.count;
    pgPrintMenuBar("取得済みスレッドの検索中");
    pgCopyMenuBar();
    flipScreen(0);
    dfd = sceIoDopen(file);
    if (dfd >= 0)
    {
        memset(&dir, 0, sizeof(dir));
        while (sceIoDread(dfd, &dir) > 0)
        {
            if (strstr(dir.d_name, ".dat"))						// 板フォルダ内にある.datファイルを検索
            {
            	if (logMode == 2) {
	            	pgPrintMenuBar(dir.d_name);
    	        	pgCopyMenuBar();
        	    	flipScreen(0);
            	}
                dat = strtol(dir.d_name, NULL, 10);
                setFlag = 0;
                for (i = 0; i < tmpCount; i++)
                {
                    if (s2ch.threadList[i].dat == dat)			// subject.txtに記載のあるファイルなら（現行スレの場合）
                    {
						if (s2ch.cfg.hbl){
							char	c,title[128];
							int		j;
							for (j=0; j<strlen(s2ch.itaList[ita].dir) ;j++){
								c = s2ch.itaList[ita].dir[j];
								if (c == '/') c = '-';
								title[j] = c;
							}
							title[j] = '\0';
							psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, &(s2ch.threadList[i].old), NULL, NULL,
								s2ch.itaList[ita].host, title, dat);
						} else {
	                        psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, &(s2ch.threadList[i].old), NULL, NULL,
	                			s2ch.itaList[ita].host, s2ch.itaList[ita].title, dat);
	                	}
                        setFlag = 1;
                        break;
                    }
                }
                // DAT落ちスレの追加
                if (setFlag == 0 && logMode)
                {
                    s2ch.threadList[s2ch.thread.count].id = -1;
                    s2ch.threadList[s2ch.thread.count].dat = dat;
                    s2ch.threadList[s2ch.thread.count].res = 0;
                    s2ch.threadList[s2ch.thread.count].ikioi = 0;
                    s2ch.threadList[s2ch.thread.count].title[0] = '\0';
                    s2ch.threadList[s2ch.thread.count].old = 0;
					if (s2ch.cfg.hbl){
						char	c,title[128];
						int		j;
						for (j=0; j<strlen(s2ch.itaList[ita].dir) ;j++){
							c = s2ch.itaList[ita].dir[j];
							if (c == '/') c = '-';
							title[j] = c;
						}
						title[j] = '\0';
						if (psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, 
									&(s2ch.threadList[s2ch.thread.count].old), 
									NULL, s2ch.threadList[s2ch.thread.count].title,
									s2ch.itaList[ita].host, title, dat) != 8) {
							psp2chWriteIdx(s2ch.itaList[ita].host, title, dat, 1);
							psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
									s2ch.threadList[s2ch.thread.count].title,
	                				s2ch.itaList[ita].host, title, dat);
	                	}
					} else {
	                    if (psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, 
	                    			&(s2ch.threadList[s2ch.thread.count].old), 
	                    			NULL, s2ch.threadList[s2ch.thread.count].title,
	                				s2ch.itaList[ita].host, s2ch.itaList[ita].title, dat) != 8) {
	                		psp2chWriteIdx(s2ch.itaList[ita].host, s2ch.itaList[ita].title, dat, 1);
							psp2chReadIdx(NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
									s2ch.threadList[s2ch.thread.count].title,
	                				s2ch.itaList[ita].host, s2ch.itaList[ita].title, dat);
	                	}
					}
                    s2ch.thread.count++;
                }
            }
        }
        sceIoDclose(dfd);
    }
	// スレタイNG 指定単語を含むスレを一覧から削除
	buf = NULL;
	buf = psp2chGetNGBuf(ngThreadFile, buf);					// ngThreadFileはpsp2chMenu.cで定義
	if (buf){
		p = buf;
		while (*p){
			q = strchr( p,'\n' );
			if (q == NULL) break;
			*q = '\0';
			for (i=0; i<s2ch.thread.count ;i++){
				if (strstr(s2ch.threadList[i].title,p)){
					for (j=i+1; j<s2ch.thread.count ;j++){
						s2ch.threadList[j-1] = s2ch.threadList[j];
					}
					s2ch.thread.count--;
				}
			}
			p = q + 1;
		}
		free(buf);
	}
    // s2ch.thread.counがDAT落ちでない分少なくなるので縮める
    s2ch.threadList = (S_2CH_THREAD*)realloc(s2ch.threadList, sizeof(S_2CH_THREAD) * s2ch.thread.count);
    if (s2ch.threadList == NULL )
    {
        psp2chNormalError(MEM_ALLC_ERR, "threadList");
        return -1;
    }
    threadSort = (int*)realloc(threadSort, sizeof(int) * s2ch.thread.count);
    if (threadSort == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, "treadSort");
        return -1;
    }
    psp2chSort(-1);
    return 0;
}

/****************
板のディレクトリがなければ作成
板のディレクトリにsubject.txtがあれば読み込む
subject.txtの取得日データをつけて2chにアクセス
更新されていれば新しいデータを取得して保存
なければ普通に取得して保存
移転時は-2を返す
*****************/
static int psp2chGetSubject(int ita)
{
    int ret;
    S_NET net;
    SceUID fd;
    char dst_url[FILE_PATH], src_url[FILE_PATH], referer[FILE_PATH], path[FILE_PATH], buf[BUF_LENGTH];
    char *p, *q;

	memset(&net, 0, sizeof(S_NET));
    // Make ita directory
	psp2chThreadLogPath( buf, ita );
    strcpy(path, buf);
    // ダメ文字対策
    if (path[strlen(path) - 1] == '\\')
    	strcat(path, "\\");
    if ((fd = sceIoDopen(path)) < 0) {
        if (sceIoMkdir(buf, FILE_PARMISSION) < 0) {
            psp2chNormalError(DIR_MAKE_ERR, path);
            return -1;
        }
    }
    else {
        sceIoDclose(fd);
	}
	
    // check subject.txt
    sprintf(path, "%s/%s", path, SUBJECT_FILE);
    fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
        buf[0] = '\0';
    }
    else
    {
        psp2chFileRead(fd, buf, BUF_LENGTH);
        sceIoClose(fd);
        p = strchr(buf, '\n');
        *p = '\0';
        strcpy(net.head.Last_Modified, buf);
        p++;
        q =  strchr(p, '\n');
        *q = 0;
        strcpy(net.head.ETag, p);
    }
    // subject.txtの読み込み先
    sprintf(src_url, "http://%s/%s/", s2ch.itaList[ita].host, s2ch.itaList[ita].dir);
    ret = psp2chBBSGetPattern(dst_url, referer, src_url, 0, "thread.dat");
	if (ret < 0)
		return ret;
    ret = psp2chGet(dst_url, referer, NULL, &net, 0);
    if (ret < 0)
    {
        return ret;
    }
    switch(net.status)
    {
        case 200: // OK
			if (strlen(net.body) == 0)
			{
				if (psp2chItenCheck(s2ch.itaList[ita].host, s2ch.itaList[ita].dir) == 0)
				{
					psp2chErrorDialog(0, TEXT_10);
					sceCtrlPeekBufferPositive(&s2ch.oldPad, 1);
					return -4;
				}
			}
            break;
        case 302: // Found
        	free(net.body);
            if (psp2chErrorDialog(1, TEXT_7) == PSP_UTILITY_MSGDIALOG_RESULT_YES)
            {
                psp2chGetMenu();
            }
            return -1;
        case 304: // Not modified
        	free(net.body);
            return 0;
        default:
        	free(net.body);
        	sprintf(err_msg, "%d", net.status);
            psp2chNormalError(NET_GET_ERR, err_msg);
            return -1;
    }
    // Receive and Save subject
	psp2chThreadLogPath( path, ita );
	strcat( path, "/" );
	strcat( path, SUBJECT_FILE );
//    sprintf(path, "%s/%s/%s", s2ch.cfg.logDir, s2ch.itaList[ita].title, SUBJECT_FILE);
    fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
    if (fd < 0)
    {
        psp2chNormalError(FILE_OPEN_ERR, path);
        return fd;
    }
    sprintf(buf, "%s\n%s\n", net.head.Last_Modified, net.head.ETag);
    psp2chFileWrite(fd, buf, strlen(buf));
    psp2chFileWrite(fd, net.body, net.length);
    sceIoClose(fd);
	free(net.body);
    return 0;
}

/****************
スレッドをソートする
threadSort配列にソートデータが入る
sort:
0=既読スレ順
1=勢い
2=番号順
3=作成日(降順)
4=作成日(昇順)
5=新着レス数(降順)
6=新着レス数(昇順)
7=取得済みレス数(降順)
8=取得済みレス数(昇順)
10=検索単語順
*****************/
void psp2chSort(int sort)
{
    int i, j, tmp;
    char haystack[BUF_LENGTH];
    char *p;

    if (sort >= 0)
    {
        threadSortType = sort;
    }
    switch (threadSortType)
    {
    case 0:
        for (i = 0, j = 0; i < s2ch.thread.count; i++)
        {
            if (s2ch.threadList[i].old > 0)
            {
                threadSort[j] = i;
                j++;
            }
        }
        for (i = 0; i < s2ch.thread.count; i++)
        {
            if (s2ch.threadList[i].old == 0)
            {
                threadSort[j] = i;
                j++;
            }
        }
        break;
    case 1:
        for (i = 0; i < s2ch.thread.count; i++)
        {
            threadSort[i] = i;
        }
        for (i = 0; i < s2ch.thread.count-1; i++)
        {
            for (j = i; j < s2ch.thread.count; j++)
            {
                if (s2ch.threadList[threadSort[j]].ikioi > s2ch.threadList[threadSort[i]].ikioi)
                {
                    tmp = threadSort[j];
                    threadSort[j] = threadSort[i];
                    threadSort[i] = tmp;
                }
            }
        }
        break;
    case 2:
        for (i = 0; i < s2ch.thread.count; i++)
        {
            threadSort[i] = i;
        }
        break;
    case 3:
        for (i = 0; i < s2ch.thread.count; i++)
        {
            threadSort[i] = i;
        }
        for (i = 0; i < s2ch.thread.count-1; i++)
        {
            for (j = i; j < s2ch.thread.count; j++)
            {
                if (s2ch.threadList[threadSort[j]].dat > s2ch.threadList[threadSort[i]].dat)
                {
                    tmp = threadSort[j];
                    threadSort[j] = threadSort[i];
                    threadSort[i] = tmp;
                }
            }
        }
        break;
    case 4:
        for (i = 0; i < s2ch.thread.count; i++)
        {
            threadSort[i] = i;
        }
        for (i = 0; i < s2ch.thread.count-1; i++)
        {
            for (j = i; j < s2ch.thread.count; j++)
            {
                if (s2ch.threadList[threadSort[j]].dat < s2ch.threadList[threadSort[i]].dat)
                {
                    tmp = threadSort[j];
                    threadSort[j] = threadSort[i];
                    threadSort[i] = tmp;
                }
            }
        }
        break;
	case 5:														// 新着レス数(降順))
		for (i = 0; i < s2ch.thread.count; i++){
			threadSort[i] = i;
		}
		for (i = 0; i < s2ch.thread.count-1; i++){
			for (j = i; j < s2ch.thread.count; j++){
				if (s2ch.threadList[threadSort[j]].res > s2ch.threadList[threadSort[i]].res){
					tmp = threadSort[j];
					threadSort[j] = threadSort[i];
					threadSort[i] = tmp;
				}
			}
		}
		break;
	case 6:														// 新着レス数(昇順))
		for (i = 0; i < s2ch.thread.count; i++){
			threadSort[i] = i;
		}
		for (i = 0; i < s2ch.thread.count-1; i++){
			for (j = i; j < s2ch.thread.count; j++){
				if (s2ch.threadList[threadSort[j]].res < s2ch.threadList[threadSort[i]].res){
					tmp = threadSort[j];
					threadSort[j] = threadSort[i];
					threadSort[i] = tmp;
				}
			}
		}
		break;
	case 7:														// 取得済みレス数(降順))
		for (i = 0; i < s2ch.thread.count; i++){
			threadSort[i] = i;
		}
		for (i = 0; i < s2ch.thread.count-1; i++){
			for (j = i; j < s2ch.thread.count; j++){
				if (s2ch.threadList[threadSort[j]].old > s2ch.threadList[threadSort[i]].old){
					tmp = threadSort[j];
					threadSort[j] = threadSort[i];
					threadSort[i] = tmp;
				}
			}
		}
		break;
	case 8:														// 取得済みレス数(昇順))
		for (i = 0; i < s2ch.thread.count; i++){
			threadSort[i] = i;
		}
		for (i = 0; i < s2ch.thread.count-1; i++){
			for (j = i; j < s2ch.thread.count; j++){
				if (s2ch.threadList[threadSort[j]].old < s2ch.threadList[threadSort[i]].old){
					tmp = threadSort[j];
					threadSort[j] = threadSort[i];
					threadSort[i] = tmp;
				}
			}
		}
		break;
    case 10:
        p = keyWords;
        if (*p >= 'a' && *p <= 'z')
        {
            *p -= ('a' - 'A');
        }
        p++;
        while (*p)
        {
            if (*(p-1) > 0 && *p >= 'a' && *p <= 'z')
            {
                *p -= ('a' - 'A');
            }
            p++;
        }
        for (i = 0, j = 0; i < s2ch.thread.count; i++)
        {
            strcpy(haystack, s2ch.threadList[i].title);
            p = haystack;
            if (*p >= 'a' && *p <= 'z')
            {
                *p -= ('a' - 'A');
            }
            p++;
            while (*p)
            {
                if (*(p-1) > 0 && *p >= 'a' && *p <= 'z')
                {
                    *p -= ('a' - 'A');
                }
                p++;
            }
            if (strstr(haystack, keyWords))
            {
                threadSort[j] = i;
                j++;
            }
        }
        for (i = 0; i < s2ch.thread.count; i++)
        {
            strcpy(haystack, s2ch.threadList[i].title);
            p = haystack;
            if (*p >= 'a' && *p <= 'z')
            {
                *p -= ('a' - 'A');
            }
            p++;
            while (*p)
            {
                if (*(p-1) > 0 && *p >= 'a' && *p <= 'z')
                {
                    *p -= ('a' - 'A');
                }
                p++;
            }
            if (strstr(haystack, keyWords) == NULL)
            {
                threadSort[j] = i;
                j++;
            }
        }
        break;
    }
}

/****************
ソート用ダイアログ表示
*****************/
#define MAX_SORT_COUNT (9)
static void psp2chThreadSort(void)
{
    const unsigned char title[] = "どの項目でソ\ートしますか";
    const unsigned char text1[] = "既読スレ";					// 既読スレ
    const unsigned char text2[] = "勢い";						// 勢い
    const unsigned char text3[] = "番号順";						// 番号順
    const unsigned char text4[] = "作成日(降順)";				// 作成日(降順)
    const unsigned char text5[] = "作成日(昇順)";				// 作成日(昇順)
    const unsigned char text6[] = "新着レス数(降順)";			// 新着レス数(降順)
    const unsigned char text7[] = "新着レス数(昇順)";			// 新着レス数(昇順)
    const unsigned char text8[] = "取得済みレス数(降順)";		// 取得済みレス数(降順)
    const unsigned char text9[] = "取得済みレス数(昇順)";		// 取得済みレス数(昇順)
    
    const unsigned char* text[MAX_SORT_COUNT] = {text1, text2, text3, text4, text5, text6, text7, text8, text9};
    int i, select = 0;

	pgCreateTexture();
	pgFillvram(THREAD_INDEX + 14, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
	
	s2ch.oldPad = s2ch.pad;
    while (s2ch.running)
    {
    	pgSetDrawStart(180, 34, 0, 0);
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
            }
        }
        pgDrawTexture(-1);
        flipScreen(0);
    }
    pgDeleteTexture();
    return psp2chSort(select);
}

/****************
検索ダイアログ表示
*****************/
int psp2chThreadSearch(void)
{
    const unsigned char text1[] = "検索文字列を入力してください";
    char* text2 = "検索文字列";

    return psp2chInputDialog(text1, text2, NULL);
}

/****************
スレ一覧の描画ルーチン
*****************/
#define THREAD_TITLE_BOUND (8)
static void psp2chDrawThread(void)
{
	static int oldSelect,rollTime,rollPos;
    int start = s2ch.thread.start, resCount, ikioiCount;
    int i, color_sel, pos;

	if (s2ch.cfg.ikioi){										// 勢い値を表示するなら
		ikioiCount = FONT_HEIGHT * 2.5;
	} else {
		ikioiCount = 0;
	}
	resCount = winParam->width - FONT_HEIGHT * 4 + winParam->viewX - ikioiCount;
    if (start + winParam->lineEnd > s2ch.thread.count) {
        start = s2ch.thread.count - winParam->lineEnd;
    }
    if (start < 0) {
        start = 0;
    }
    // 全体の背景
    pgFillvram(s2ch.threadColor.bg[0], winParam->viewX, 0, winParam->viewX + winParam->width, winParam->height, 2);
    pgSetDrawStart(-1, 0, 0, 0);
    for (i = start; i < start + winParam->lineEnd; i++)
    {
        if (i >= s2ch.thread.count) {
            return;
        }
        color_sel = 0;
        
        pgSetDrawStart(0, -1, 0, 0);
        // 選択状態の背景
        if (i == s2ch.thread.select) {
        	color_sel = 1;
            pgFillvram(s2ch.threadColor.bg[color_sel], winParam->viewX, winParam->pgCursorY, winParam->viewX + winParam->width, LINE_PITCH, 2);
        }
        // id
		if (s2ch.threadList[threadSort[i]].id >= 0) {
		    pgPrintNumber(s2ch.threadList[threadSort[i]].id + 1, s2ch.threadColor.num[color_sel], s2ch.threadColor.bg[color_sel]);
		}
		// タイトル
        pgSetDrawStart(FONT_HEIGHT * 2, -1, 0, 0);
		if (s2ch.cfg.threadRoll && i == s2ch.thread.select){	// カーソル行をスクロールさせる
			int time1,time2;
			if (winParam->tateFlag){							// 縦画面は作画が遅いのでウェイトを少なめに
				time1 = 25;
				time2 = 3;
			} else {											// 横画面
				time1 = 60;
				time2 = 10;
			}
			if (s2ch.thread.select!=oldSelect){					// カーソル行が変化した
				oldSelect = s2ch.thread.select;
				rollTime = 0;
				rollPos = 0;
			} else if ((rollPos==0 && rollTime>time1) || (rollPos!=0 && rollTime>time2)){
				unsigned char c;
				c = (unsigned char)s2ch.threadList[threadSort[i]].title[rollPos];
				if (((c>=0x80) && (c<0xa0)) || (c>=0xe0)) {		// 全角文字
					rollPos++;
				} else if (c=='&') {							// 特殊文字
					while (c!=';' || c!='\0') {
						c = (unsigned char)s2ch.threadList[threadSort[i]].title[++rollPos];
					}
				}
				rollPos++;
				rollTime = 0;
			}
			rollTime++;
			if (strlen(s2ch.threadList[threadSort[i]].title)<rollPos) rollPos = 0;
			pos = rollPos;
		} else {
			pos = 0;
		}
        if (s2ch.threadList[threadSort[i]].old > 0) {
            pgPrint(&s2ch.threadList[threadSort[i]].title[pos], s2ch.threadColor.text2[color_sel], s2ch.threadColor.bg[color_sel], resCount+THREAD_TITLE_BOUND);
        }
        else {
            pgPrint(&s2ch.threadList[threadSort[i]].title[pos], s2ch.threadColor.text1[color_sel], s2ch.threadColor.bg[color_sel], resCount+THREAD_TITLE_BOUND);
        }
        // 勢い値
		if (s2ch.cfg.ikioi){
			pgSetDrawStart(resCount, -1, 0, 0);
			pgPrintNumber(s2ch.threadList[threadSort[i]].ikioi, s2ch.threadColor.count1[color_sel], s2ch.threadColor.bg[color_sel]);
		}
        // レス数
        pgSetDrawStart(resCount + ikioiCount, -1, 0, 0);
        pgPrintNumber(s2ch.threadList[threadSort[i]].res, s2ch.threadColor.count1[color_sel], s2ch.threadColor.bg[color_sel]);
        if (s2ch.threadList[threadSort[i]].old > 0) {
            pgPrintNumber(s2ch.threadList[threadSort[i]].old, s2ch.threadColor.count2[color_sel], s2ch.threadColor.bg[color_sel]);
        }
        // 次の行へ
        pgSetDrawStart(-1, -1, 0, LINE_PITCH);
    }
}
#undef THREAD_TITLE_BOUND

// 名前付き捕獲式集合で値をセットする
static int thread_name_callback(const UChar* name, const UChar* name_end, int ngroup_num, int* group_nums, regex_t* reg, void* arg)
{
	int i, gn;
	reg_data* data = (reg_data*)arg;
	unsigned char *str = data->str;
	OnigRegion *region = data->region;
	char buf[512];

	for (i = 0; i < ngroup_num; i++) {
		gn = group_nums[i];
		memcpy(buf, str + region->beg[gn], region->end[gn] - region->beg[gn]);
		buf[region->end[gn] - region->beg[gn]] = '\0';
		if (memcmp((char*)name, "thread_id", name_end - name) == 0) {
			s2ch.threadList[s2ch.thread.count].id = strtol(buf, NULL, 10) - 1;
		}
		else if (memcmp((char*)name, "dat_num", name_end - name) == 0) {
			s2ch.threadList[s2ch.thread.count].dat = strtol(buf, NULL, 10);
		}
		else if (memcmp((char*)name, "subject", name_end - name) == 0) {
			strlcpy(s2ch.threadList[s2ch.thread.count].title, buf, SUBJECT_LENGTH);
		}
		else if (memcmp((char*)name, "res_num", name_end - name) == 0) {
			s2ch.threadList[s2ch.thread.count].res = strtol(buf, NULL, 10);
		}
	}
	return 0;
}

// 正規表現でsubject.txtを分解する
static int psp2chSubjectParser(unsigned char *str, const unsigned char *pattern)
{
	time_t tm;
	int denom;
	int r;
	unsigned char *start, *end;
	regex_t* reg;
	OnigErrorInfo einfo;
	OnigRegion *region;
	
    sceKernelLibcTime (&tm);
    
	r = onig_new(&reg, pattern, pattern + strlen((char* )pattern),
		ONIG_OPTION_DEFAULT, ONIG_ENCODING_SJIS, ONIG_SYNTAX_DEFAULT, &einfo);
	if (r != ONIG_NORMAL) {
		char s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str(s, r, &einfo);
		psp2chErrorDialog(0, "ERROR: %s\n", s);
		return -1;
	}
	
	region = onig_region_new();
	
	end   = str + strlen((char* )str);
	start = str;
	while ((r = onig_search(reg, str, end, start, end, region, ONIG_OPTION_NONE)) >= 0) {
		s2ch.threadList[s2ch.thread.count].id = s2ch.thread.count;
		s2ch.threadList[s2ch.thread.count].old = 0;
		
		reg_data data = {str, region};
		r = onig_foreach_name(reg, thread_name_callback, (void* )&data);
		
		denom = (tm - s2ch.threadList[s2ch.thread.count].dat) ? tm - s2ch.threadList[s2ch.thread.count].dat : 1;
        s2ch.threadList[s2ch.thread.count].ikioi = s2ch.threadList[s2ch.thread.count].res * 60 * 60 * 24 / denom;
        
		s2ch.thread.count++;
		start = str + region->end[0] + 1;
		onig_region_clear(region);
	}

	onig_region_free(region, 1);
	onig_free(reg);
	onig_end();

    return 0;
}
