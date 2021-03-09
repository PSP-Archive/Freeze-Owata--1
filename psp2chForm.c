/*
* $Id: psp2chForm.c 159 2008-09-30 22:41:46Z bird_may_nike $
*/

#include "psp2ch.h"
#include <psputility.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oniguruma.h>
#include "pg.h"
#include "psp2chNet.h"
#include "psp2chMenu.h"
#include "charConv.h"
#include "psphtmlviewer.h"
#include "psp2chReg.h"
#include "utf8.h"

#define FORM_BG_COLOR		115
#define BOX_BG_COLOR		116
#define BOX_TEXT_BG_COLOR	117
#define NINJA_FILE			"ninja.dat"
#define BEMAIL				"メールアドレス="
#define BEPASS				"パスワード="

extern S_2CH s2ch;				// psp2ch.c
extern char keyWords[128];		// psp2ch.c
extern Window_Layer *winParam;

static char cookie_2ch[NET_COOKIE_LENGTH] = {0},
            BeCode[512] = {0},
            ninja[1024] = {0},
            hidName[128] = {0}, hidValue[128] = {0};

// prototype
static int psp2chFormResPost(char* host, char* dir, unsigned long dat, char* subject, char* name, char* mail, char* message, int beFlag);
int psp2chInputKana(char* buf, unsigned int max);
static int psp2chFormResPreview(char *host, char *dir, char *title, unsigned long dat, char *subject, char *name, char *mail, char *message);

int	SMEMOtext(char *message,int maxsize,int maxline);	// PSPメモ帳


//==============================================================
// Beへログイン
//--------------------------------------------------------------
// flag   0以外:Be.txtを再構成する
// 戻り値  0:正常終了
//        -1:異常発生
//--------------------------------------------------------------
// Beにログインし認証コードを取得する。
// 認証コードはBeCode[]に格納。
//
// Be.txtのフォーマットは、
//   メールアドレス=ここにメールアドレス[CR][LF]
//   パスワード=ここにパスワード[CR][LF]
//
// 改行は[LF]と[CR][LF]に対応、最後の行の改行は無くても良い
// 行の順番は関係ない、余計な行があっても良い
//
// Be.txtが見つからないか書式が違うときはメールアドレスとパスワードの入力を求め
// Be.txtを再構成する。
//--------------------------------------------------------------

int psp2chFormBeLogin(int flag)
{
	char buf[TMP_BUF_SIZE], buf2[BUF_LENGTH], mail[BUF_LENGTH], pass[BUF_LENGTH], *pos, *pos2, *encode;
	int size, ret, len;
	SceUID fd;
	S_NET net;

	encode = (char*)malloc(512);
	if (encode == NULL) {
		psp2chNormalError(MEM_ALLC_ERR, NULL);
		return (-1);
	}

	mail[0] = '\0';
	pass[0] = '\0';
	buf2[0] = '\0';
	sprintf(buf, "%s/%s", s2ch.cfg.logDir, "Be.txt");
	fd = sceIoOpen(buf, PSP_O_RDONLY, FILE_PARMISSION);			// Be.txt読み込み
	if (fd > 0) {
		size = sceIoRead(fd, buf, BUF_LENGTH);
		sceIoClose(fd);
		buf[size] = '\0';
		pos = strstr(buf, BEMAIL);
		len = strlen(BEMAIL);
		if (pos) {
			pos2 = strchr(pos, '\n');
			if (pos2) {											// 改行で終わってる場合
				if (pos2[-1] == '\r') pos2--;					// CR,LFだった場合
				strncpy(mail, &pos[len], pos2 - (pos + len));
				mail[pos2-(pos+len)] = '\0';
			} else {											// 改行が無い場合
				strcpy(mail, &pos[len]);
			}
		}
		pos = strstr(buf, BEPASS);
		len = strlen(BEPASS);
		if (pos) {
			pos2 = strchr(pos, '\n');
			if (pos2) {											// 改行で終わってる場合
				if (pos2[-1] == '\r') pos2--;					// CR,LFだった場合
				strncpy(buf2, &pos[len], pos2 - (pos + len));
				buf2[pos2-(pos+len)] = '\0';
			} else {											// 改行が無い場合
				strcpy(buf2, &pos[len]);
			}
			psp2chUrlEncode(pass, buf2);
		}
	}

	if (flag || strlen(mail)==0 || strlen(pass)==0) {			// Be.txtの再構成
		if (!flag) {
			ret = psp2chErrorDialog(1, "Beログイン情報の取得に失敗しました。\n\nBe.txt を再構\成しますか？");
			if (ret != 1) {
				free(encode);
				return (-1);
			}
		}
		if (psp2chInputDialog("Beに登録してあるメールアドレスを入力", "メールアドレス", mail)) {
			free(encode);
			return (-1);
		}
		if (strlen(keyWords)==0) {
			free(encode);
			return (-1);
		}
		strcpy(mail, keyWords);
		pgRewrite();											// 画面を再描写
		if (psp2chInputDialog("Beに登録してあるパスワードを入力", "パスワード", buf2)) {
			free(encode);
			return (-1);
		}
		if (strlen(keyWords)==0) {
			free(encode);
			return (-1);
		}
		strcpy(buf, BEMAIL);
		strcat(buf, mail);
		strcat(buf, "\r\n" BEPASS);
		strcat(buf, keyWords);
		strcat(buf, "\r\n");
		sprintf(buf2, "%s/%s", s2ch.cfg.logDir, "Be.txt");
		fd = sceIoOpen(buf2, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
		if (fd >= 0) {
			psp2chFileWrite(fd, buf, strlen(buf));
			sceIoClose(fd);
		}
		psp2chUrlEncode(pass, keyWords);
	}

	sprintf(encode, "m=%s&p=%s", mail, pass);
	strcat(encode, "&submit=%C5%D0%CF%BF");						// ログインするためのbodyデータ
	memset(&net, 0, sizeof(S_NET));
	BeCode[0] = '\0';
	ret = psp2chPost("http://be.2ch.net/test/login.php", NULL, BeCode, encode, &net);
	if (ret < 0)
	{
		psp2chNormalError(NET_POST_ERR, NULL);
		free(net.body);
		free(encode);
		return (-1);
	}
	switch(net.status)
	{
		case 200: // OK
		case 302: // まちBBS
			break;
		default:
			sprintf(err_msg, "%d", net.status);
			psp2chNormalError(NET_GET_ERR, err_msg);
			free(net.body);
			free(encode);
			return (-1);
	}

	ret = 0;
	if (!strstr(BeCode, "DMDM=") && !strstr(BeCode, "MDMD=")) {	// Beログインに失敗した
		strcpy(buf, net.body);
		if(strstr(net.head.Content_Type, "EUC-JP") || strstr(net.head.Content_Type, "euc-jp"))	// コード変換
		{
			psp2chEucToSjis(buf, buf);
		}
		psp2chErrorDialog(1, buf);
		ret = -1;
		BeCode[0] = '\0';
	}

	free(net.body);
	free(encode);
	return (ret);
}


//==============================================================
// Beからログアウト
//--------------------------------------------------------------
// 戻り値 必ず成功
//--------------------------------------------------------------
// Beからログアウトする。
// 実際は認証コード（BeCode[]）をクリアしてるだけ。
//--------------------------------------------------------------

void psp2chFormBeLogout(void)
{
	BeCode[0] = '\0';
}


//==============================================================
// Beのログイン状況
//--------------------------------------------------------------
// 戻り値 0:ログインしていない
//        1:ログインしている
//--------------------------------------------------------------
// Be認証コードを取得しているならログイン中。
//--------------------------------------------------------------

int psp2chFormBeLoginCheck(void)
{
	return (strlen(BeCode)==0 ? 0 : 1);
}


/*********************
レス書き込み
*********************/
static int psp2chFormResPost(char* host, char* dir, unsigned long dat, char* subject, char* name, char* mail, char* message, int beFlag)
{
	S_2CH_RES_COLOR c;
    S_NET net;
	SceUID fd;
    int ret,count;
    char *encode, *str, *str2, buf[FILE_PATH];
    char src_url[FILE_PATH], dst_url[FILE_PATH], referer[FILE_PATH];
    char cookie[NET_COOKIE_LENGTH] = {0};

	// 送信しますかダイアログで画面消えてるので再描画
	pgDrawTexture(-1);
	pgCopyMenuBar();
	flipScreen(0);

	encode = (char*)malloc(RES_MESSAGE_LENGTH * 8);
    if (encode == NULL)
    {
        psp2chNormalError(MEM_ALLC_ERR, NULL);
        return -1;
    }

	if (dat) {													// レス書き
		sprintf(src_url, "http://%s/test/read.cgi/%s/%d/", host, dir, dat);
		ret = psp2chFormGetPattern(dst_url, "form.dat", referer, encode, src_url, name, mail, message, NULL);
	}
	else {														// スレ立て
		sprintf(src_url, "http://%s/test/read.cgi/%s/", host, dir);
		ret = psp2chFormGetPattern(dst_url, "formThread.dat", referer, encode, src_url, name, mail, message, subject);
	}
	if (ret < 0) {
		free(encode);
		return -1;
	}
	
    memset(&net, 0, sizeof(S_NET));
	if ((strstr(host, "2ch.net") || strstr(host, "bbspink.com")))
	{
		if (beFlag == 1) {										// Beにログイン
			if (strlen(BeCode)==0){								// Be認証コードを取得していないならログイン
				ret = psp2chErrorDialog(4, "Beにログインします");
				if (ret != 1 && ret != 4) {
					free(encode);
					return (-1);
				}
				pgPrintMenuBar("");
				pgRewrite();									// 画面を再描写
				if (psp2chFormBeLogin(ret==1 ? 0 : 1)) {		// エラーが発生したなら
					free(encode);
					return (-1);
				}
				pgPrintMenuBar("Beにログインしました");
				pgCopyMenuBar();
				flipScreen(0);
				pgWaitVn(60);
			}
		}
		if (!strlen(cookie_2ch))									// 起動後の初回
		{
			int		size;
			sprintf(buf, "%s/%s", s2ch.cfg.logDir, NINJA_FILE);		// 保存してある忍法帖コードを取得
			fd = sceIoOpen(buf, PSP_O_RDONLY, FILE_PARMISSION);
			if (fd > 0) {
				size = sceIoRead(fd, ninja, FILE_PATH);
				sceIoClose(fd);
				ninja[size] = '\0';
			} else {
				ninja[0] = '\0';
			}
			// 仮送信して2ちゃんからSet-CookieのPON HAP 取得
			memset(&net, 0, sizeof(S_NET));
			ret = psp2chPost(dst_url, referer, cookie, encode, &net);
			if (ret < 0)
			{
				free(net.body);
				free(encode);
				psp2chNormalError(NET_POST_ERR, "1st");
				return ret;
			}
			switch(net.status)
			{
			case 200: // OK
				break;
			default:
				free(net.body);
				free(encode);
				sprintf(err_msg, "%d", net.status);
				psp2chNormalError(NET_GET_ERR, err_msg);
				return -1;
			}
			// 応答確認
			str = strstr(cookie, "PON=");
			if (!str) {											// PONが返されなかった
				free(net.body);
				free(encode);
				psp2chErrorDialog(1, "書き込みに失敗したようです");
				return -1;
			}
			strcpy(cookie_2ch, str);
			// 書き込み確認メッセージを画面に表示
			c.text = WHITE;
			c.bg = FORM_INDEX + 3;
			c.link = BLUE;
			pgFillvram(FORM_INDEX + 3, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
			pgSetDrawStart(0, 0, 0, 0);
			if(strstr(net.head.Content_Type, "EUC-JP") || strstr(net.head.Content_Type, "euc-jp"))	// コード変換
			{
				psp2chEucToSjis(net.body, net.body);
			}
			str = net.body;
			while ((str = pgPrintHtml(str, &c, 0, SCR_WIDTH, 0)))
			{
				pgSetDrawStart(0, -1, 0, LINE_PITCH);
				if (winParam->pgCursorY >= 260)
				{
					break;
				}
			}
			pgRewrite();										// 画面更新
			// 隠し項目取得
			hidName[0] = '\0';
			hidValue[0] = '\0';
			str = strstr(net.body, "type=hidden name=\"");
			if (str) {
				str += strlen("type=hidden name=\"");
				str2 = strchr(str, '"');
				if (str2) {
					memcpy(hidName, str, str2 - str);
					hidName[str2 - str] = '\0';
				}
				str = strstr(str, "value=\"");
				if (str) {
					str += strlen("value=\"");
					str2 = strchr(str, '"');
					if (str2) {
						memcpy(hidValue, str, str2 - str);
						hidValue[str2 - str] = '\0';
					}
				}
			}
			// 隠し項目を追加
			if (strlen(hidName) == 0) {
				sprintf(buf, "; %s", s2ch.cfg.addCookie);
			} else {
				sprintf(buf, "; %s=%s", hidName, hidValue);
			}
			strcat(cookie_2ch, buf);
			free(net.body);
			sceKernelDelayThread(ACCESS_WAIT);
		}
		strcpy(cookie, cookie_2ch);
		if (strlen(ninja)) {									// 忍法帖を取得しているならクッキーに追加
			strcat(cookie, "; ");
			strcat(cookie, ninja);
		}
		if (beFlag == 1) {										// Be書き込みモード
			strcat(cookie, "; ");
			strcat(cookie, BeCode);								// Be認証コードをcookieに追加
		}
		/*else if (beFlag == 2) {								// ●書き込みモード
			static char sid[BUF_LENGTH * 2] = "";
			int size, ret;
			char *p;
			
			if (strlen(sid) == 0) {
				sprintf(buf, "%s/%s", s2ch.cfg.logDir,"maru.txt");
				fd = sceIoOpen(buf, PSP_O_RDONLY, FILE_PARMISSION);
				if (fd > 0) {
					size = sceIoRead(fd, buf, BUF_LENGTH);
					sceIoClose(fd);
					buf[size] = '\0';
					ret = psp2chNetMaruLogin(buf, "https://2chv.tora3.net/futen.cgi", buf);
					if (ret == 0) {
						if (strstr(buf, "ERROR"))
							sid[0] = '\0';
						else {
							p = strchr(buf, ':');
							psp2chUrlEncode(sid, p);
						}
					}
				}
			}
			strcat(encode, "&sid=");
			strcat(encode, sid);
		}*/
	}

    // Cookieをセットして送信
	memset(&net, 0, sizeof(S_NET));
    ret = psp2chPost(dst_url, referer, cookie, encode, &net);
	if (ret < 0)
    {
        psp2chNormalError(NET_POST_ERR, "書き込み処理で問題発生");
		free(encode);
        return -1;
    }

	count = 0;
	while (s2ch.wait[count].host[0]!='\0'){
		if (strcmp(s2ch.wait[count].host,host)==0){
			sceKernelLibcTime(&s2ch.wait[count].resTime);		//書き込みを行った時間を記録
			break;
		}
		count++;
	}

    switch(net.status)
    {
        case 200: // OK
        case 302: // まちBBS
            break;
        default:
        	free(net.body);
        	free(encode);
        	sprintf(err_msg, "%d", net.status);
        	psp2chNormalError(NET_GET_ERR, err_msg);
            return -1;
    }
	{
		char	*msg;
		int		waitTime;
		c.text = WHITE;
		c.bg = FORM_INDEX + 3;
		c.link = BLUE;
		pgFillvram(FORM_INDEX + 3, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
		pgSetDrawStart(0, 0, 0, 0);
		str = strstr(net.body, "</html");
		if (str) *str = 0;
		str = strstr(net.body, "<html");
		if(strstr(net.head.Content_Type, "euc-jp")) // コード変換
		{
			psp2chEucToSjis(str, str);
		}
		// 画面に表示しきれる分だけ表示（スクロールは面倒なのでなし）
		msg = str;
		while ((str = pgPrintHtml(str, &c, 0, SCR_WIDTH, 0)))
		{
			pgSetDrawStart(0, -1, 0, LINE_PITCH);
			if (winParam->pgCursorY >= 260)
			{
				break;
			}
		}
		str = strstr(msg, "ブラウザを立ち上げ");
		if (str!=NULL) {
			psp2chErrorDialog(1, "書き込みに失敗しました。\n\nPSPの時刻設定がずれていませんか？");
		}
		str = strstr(msg, "Beユーザー情報エラー");
		if (str!=NULL) {
			psp2chFormBeLogout();								// Be認証コード削除
		}
		// 忍法帖の取得
		str = strstr(cookie, "HAP=");
		if (str!=NULL) {										// cookieよりHAPエントリを抽出しファイルへ保存
			char	path[FILE_PATH];
			str2 = strchr(str, ';');
			if (str2==NULL) {
				str2 = str + strlen(str);
			}
			sprintf(path, "%s/%s", s2ch.cfg.logDir, NINJA_FILE);
			fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
			if (fd >= 0) {
				psp2chFileWrite(fd, str, str2 - str);
				sceIoClose(fd);
			}
			*str2 = '\0';
			strcpy(ninja, str);
		}
		// samba24規定時間の取得
		str = strstr(msg, "ＥＲＲＯＲ - 593");					// samba24エラーメッセージを探す
		if (str!=NULL){
			str += 17;
			waitTime = 0;
			sscanf(str, "%d", &waitTime);						// 規定時間を取得
			if (waitTime!=0){									// 正常に取得できたみたいなら
				int		flag;
				count = 0;
				flag = 0;
				while (s2ch.wait[count].host[0]!='\0'){
					if (strcmp(s2ch.wait[count].host,host)==0){
						if (s2ch.wait[count].waitTime!=waitTime){
							s2ch.wait[count].waitTime = waitTime;
							flag = 1;
						}
						break;
					}
					count++;
				}
				if (s2ch.wait[count].host[0]=='\0' && count<RES_WRITETIME_MAX-1){	// サーバの新規登録
					strcpy(s2ch.wait[count].host,host);
					s2ch.wait[count].waitTime = waitTime;
					sceKernelLibcTime(&s2ch.wait[count].resTime);	//書き込みを行った時間を記録
					s2ch.wait[count+1].host[0] = '\0';
					flag = 1;
				}
				if (flag){										// samba24設定値の保存
					char	path[FILE_PATH];
					pgPrintMenuBar("samba24の規定時間を記録中");
					pgRewrite();								// 画面更新
					sprintf(path, "%s/writetime.ini", s2ch.cfg.logDir);
					if ((fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION)) > 0){
						count = 0;
						while (s2ch.wait[count].host[0]!='\0'){
							sprintf(buf, "%s = %d\n", s2ch.wait[count].host, s2ch.wait[count].waitTime);
							sceIoWrite(fd, buf, strlen(buf));
							count++;
						}
						sceIoClose(fd);
					}
				}
			}
		}
		free(net.body);
		free(encode);
	}
	ret = 0;
    while (s2ch.running)
    {
		strcpy(buf, "画面は切り替わりません    ○ : 入力画面へ    × : レス表\示へ    ");
		if (strlen(ninja)) {
			ret = -1;
			strcat(buf, "□ : 忍法帖削除");
		}
		pgPrintMenuBar(buf);
        sceCtrlPeekBufferPositive(&s2ch.pad, 1);
        if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
        {
            s2ch.oldPad = s2ch.pad;
            if(s2ch.pad.Buttons & PSP_CTRL_CIRCLE)
            {
                break;
            }
            if(s2ch.pad.Buttons & PSP_CTRL_CROSS)
            {
                return 2;
            }
			if(s2ch.pad.Buttons & PSP_CTRL_SQUARE && ret) {		// 忍法帖削除
				if (psp2chErrorDialog(1, "忍法帖を削除します") == 1) {
					sprintf(buf, "%s/%s", s2ch.cfg.logDir, NINJA_FILE);
					sceIoRemove(buf);
					ninja[0] = '\0';
					ret = 0;
				}
			}
        }
		pgRewrite();											// 画面更新
    }
    return 1;
}

/*********************
入力画面表示
OSKで入力
*********************/
int psp2chForm(char* host, char* dir, char* title, unsigned long dat, char* subject, char* message, S_SCROLLBAR *bar)
{
    static char name[RES_NAME_LENGTH] = {0};
    static char mail[RES_MAIL_LENGTH] = {0};
    SceUID fd;
    int focus = 0, prefocus = -1, ret = 0, sage;
    char buf[BUF_LENGTH];
    char *str, *p;
	char *sagestr = "sage", menuStr[128];
    int changeFlag = 0,count,waitTime;
    time_t resTime,OldResTime;

	if (s2ch.cfg.formOSK){
		strcpy( menuStr, "　○ : 入力　　Ｒ : メモ帳で入力　× : 戻る　　△ : 送信　　□ : クリア　　start : メニュー" );
	} else {
		strcpy( menuStr, "　○ : 入力　　Ｒ : OSKで入力　　× : 戻る　　△ : 送信　　□ : クリア　　start : メニュー" );
	}
	count = 0;
	OldResTime = 0;
	waitTime = 0;
	while (s2ch.wait[count].host[0]!='\0'){
		if (strcmp(s2ch.wait[count].host,host)==0){
			OldResTime = s2ch.wait[count].resTime;				// そのサーバーで最後に書き込みを行った時間
			waitTime = s2ch.wait[count].waitTime;				// そのサーバーでのsamba24設定値
			break;
		}
		count++;
	}

	pgCreateTexture();
    focus = 0;
	prefocus = -1; // focusが移動したときだけ描画するための変数
    if (mail[0] == '\0' && name[0] == '\0')
    {
        sprintf(buf, "%s/form.ini", s2ch.cfg.logDir);
        fd = sceIoOpen(buf, PSP_O_RDONLY, FILE_PARMISSION);
        if (fd >= 0)
        {
            if (psp2chFileRead(fd, buf, BUF_LENGTH) > 0)
            {
            	str = strchr(buf, '\n');
	            *str = '\0';
            	strcpy(name, buf);
            	str++;
	            p = strchr(str, '\n');
   	    		*p = '\0';
	            strcpy(mail, str);
	        }
            sceIoClose(fd);
        }
    }
    if (strstr(mail, sagestr)) {
        sage = 1;
    }
    else {
        sage = 0;
    }
    s2ch.oldPad = s2ch.pad;
	pgFillvram(FORM_INDEX + 3, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
    while (s2ch.running)
    {
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if(s2ch.pad.Buttons & PSP_CTRL_UP)
                {
                    focus--;
                    if (focus < 0) {
                        focus = 0;
                    }
                }
                if(s2ch.pad.Buttons & PSP_CTRL_DOWN)
                {
                    focus++;
                    if (focus > 2) {
                        focus = 2;
                    }
                }
                if(s2ch.pad.Buttons & PSP_CTRL_LEFT)
                {
                    if (focus == 1) {
                        sage = sage ? 0 : 1;
						prefocus = -1;
                    }
                }
                if(s2ch.pad.Buttons & PSP_CTRL_RIGHT)
                {
                    if (focus == 1) {
                        sage = sage ? 0 : 1;
						prefocus = -1;
                    }
                }
                if((s2ch.pad.Buttons & PSP_CTRL_CIRCLE) || (s2ch.pad.Buttons & PSP_CTRL_RTRIGGER))
                {
                    switch (focus)
                    {
                    case 0:														// 名前
						if (s2ch.pad.Buttons & PSP_CTRL_RTRIGGER){				// Rボタンを押しているなら
							if (psp2chInputDialog("名前を入力", "名前", name) == 0){
								strcpy(name, keyWords);
							}
						} else {
							psp2chGets("名前", name, RES_NAME_LENGTH, 1);		// OSK
						}
                        changeFlag = 1;
                        break;
                    case 1:														// mail
						if (s2ch.pad.Buttons & PSP_CTRL_RTRIGGER){				// Rボタンを押しているなら
							if (psp2chInputDialog("メールアドレスを入力", "mail", mail) == 0){
								strcpy(mail, keyWords);
							}
						} else {
							psp2chGets("mail", mail, RES_MAIL_LENGTH, 1);		// OSK
						}
                        changeFlag = 1;
                        break;
                    case 2:														// 本文
						if (s2ch.pad.Buttons & PSP_CTRL_RTRIGGER){				// Rボタンを押しているなら
                        	if (s2ch.cfg.formOSK){
								SMEMOtext(message, RES_MESSAGE_LENGTH, 32);		// PSPメモ帳
								pgSetupGu();									// PSPメモ帳
								pgCursorColorSet();								// PSPメモ帳（カーソルイメージ再設定）
							} else {
                        		psp2chGets(NULL, message, RES_MESSAGE_LENGTH, 32);
                        	}
                        } else {												// ○ボタンなら
							if (s2ch.cfg.formOSK){
								psp2chGets(NULL, message, RES_MESSAGE_LENGTH, 32);
							} else {
								SMEMOtext(message, RES_MESSAGE_LENGTH, 32);		// PSPメモ帳
								pgSetupGu();									// PSPメモ帳
								pgCursorColorSet();								// PSPメモ帳（カーソルイメージ再設定）
							}
                        }
                        break;
                    }
					prefocus = -1;
                }
                else if(s2ch.pad.Buttons & PSP_CTRL_CROSS)
                {
                    break;
                }
                else if(s2ch.pad.Buttons & PSP_CTRL_TRIANGLE)
                {
                	ret = psp2chFormResPreview(host, dir, title, dat, subject, name, mail, message);
                    if (ret == 2)
                        break;
					count = 0;
					OldResTime = 0;
					waitTime = 0;
					while (s2ch.wait[count].host[0]!='\0'){
						if (strcmp(s2ch.wait[count].host,host)==0){
							OldResTime = s2ch.wait[count].resTime;				// そのサーバーで最後に書き込みを行った時間
							waitTime = s2ch.wait[count].waitTime;				// そのサーバーでのsamba24設定値
							break;
						}
						count++;
					}
					pgFillvram(FORM_INDEX + 3, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
					prefocus = -1;
                }
                else if(s2ch.pad.Buttons & PSP_CTRL_SQUARE)
                {
                    switch (focus)
                    {
                    case 0:
                        memset(name, 0, RES_NAME_LENGTH);
                        changeFlag = 1;
                        break;
                    case 1:
                    	memset(mail, 0, RES_MAIL_LENGTH);
                        changeFlag = 1;
                        break;
                    case 2:
                    	memset(message, 0, RES_MESSAGE_LENGTH);
                        break;
                    }
					prefocus = -1;
                }
                else if(s2ch.pad.Buttons & PSP_CTRL_START)
                {
                	// ファイルから読み込む
                	if (focus == 0)
                	{
                		ret = psp2chMenuForm(bar, name, RES_NAME_LENGTH - strlen(name), "name");
                	}
                	else if (focus == 1)
                	{
                		ret = psp2chMenuForm(bar, mail, RES_MAIL_LENGTH - strlen(mail), "mail");
                	}
                	else if (focus == 2)
                	{
                		ret = psp2chMenuForm(bar, message, RES_MESSAGE_LENGTH - strlen(message), "message");
                	} 
                	if (ret)
                	{
                		char buf[NET_HOST_LENGTH + NET_PATH_LENGTH];
                		sprintf(buf, "http://p2.2ch.net/p2/post_form.php?host=%s&bbs=%s&key=%d", host, dir, dat);
                		pspShowBrowser(buf, NULL);
                	}
                	pgFillvram(FORM_INDEX + 3, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
					prefocus = -1;
                }
                else if(s2ch.pad.Buttons & PSP_CTRL_SELECT)
                {
                	switch (focus)
                	{
                		case 0:
                			psp2chInputKana(name, RES_NAME_LENGTH);
                			break;
                		case 1:
                			psp2chInputKana(mail, RES_MAIL_LENGTH);
                			break;
                		case 2:
                			psp2chInputKana(message, RES_MESSAGE_LENGTH);
                			break;
                	}
                	prefocus = -1;
                }
				if (sage)
				{
					if (!strstr(mail, sagestr) && strlen(mail) < 60)
					{
						strcat(mail, sagestr);
						changeFlag = 1;
					}
				}
				else
				{
					p = strstr(mail, sagestr);
					if (p)
					{
						*p = '\0';
						changeFlag = 1;
					}
				}
				if (focus != prefocus)
				{
					if (focus<2){
						strcpy( menuStr, " ○ : OSKで入力　　Ｒ : ライン入力　　× : 戻る　　△ : 送信　　□ : クリア　　start : ﾒﾆｭｰ" );
					} else if (s2ch.cfg.formOSK){
						strcpy( menuStr, " ○ : OSKで入力　　Ｒ : ﾒﾓ帳で入力　　× : 戻る　　△ : 送信　　□ : クリア　　start : ﾒﾆｭｰ" );
					} else {
						strcpy( menuStr, " ○ : ﾒﾓ帳で入力　　Ｒ : OSKで入力　　× : 戻る　　△ : 送信　　□ : クリア　　start : ﾒﾆｭｰ" );
					}
					prefocus = focus;
					pgSetDrawStart(10, 30, 0, 0);
					pgPrint("　名前：", GRAY, FORM_BG_COLOR, 58);
					pgSetDrawStart(60, -1, 0, 0);
					pgEditBox(BOX_BG_COLOR, 58, 28, 400, 44);
					pgPrint(name, WHITE, BOX_BG_COLOR, 400);
					pgSetDrawStart(10, 60, 0, 0);
					pgPrint("メール：", GRAY, FORM_BG_COLOR, 58);
					pgSetDrawStart(60, -1, 0, 0);
					pgEditBox(BOX_BG_COLOR, 58, 58, 300, 74);
					pgPrint(mail, WHITE, BOX_BG_COLOR, 400);
					pgFillvram(FORM_INDEX + 3, 310, 60, 12, 12, 2);
					pgSetDrawStart(310, 60, 0, 0);
					if (sage)
					{
						pgPrint("●", GRAY, FORM_BG_COLOR, SCR_WIDTH);
					}
					else
					{
						pgPrint("○", GRAY, FORM_BG_COLOR, SCR_WIDTH);
					}
					pgPrint("sage (← →キーで切替)", GRAY, FORM_BG_COLOR, SCR_WIDTH);
					pgSetDrawStart(10, 90, 0, 0);
					pgEditBox(BOX_BG_COLOR, 8, 88, 470, 250);
					str = message;
					while ((str = pgPrint(str, WHITE, BOX_BG_COLOR, 470)))
					{
						pgSetDrawStart(10, -1, 0, LINE_PITCH);
						if (winParam->pgCursorY >= 250)
						{
							break;
						}
					}
					switch (focus)
					{
					case 0:
						pgSetDrawStart(10, 30, 0, 0);
						pgPrint("　名前：", WHITE, FORM_BG_COLOR, 58);
						pgSetDrawStart(60, -1, 0, 0);
						pgEditBox(BOX_TEXT_BG_COLOR, 58, 28, 400, 44);
						pgPrint(name, BLACK, BOX_TEXT_BG_COLOR, 400);
						break;
					case 1:
						pgSetDrawStart(10, 60, 0, 0);
						pgPrint("メール：", WHITE, FORM_BG_COLOR, 58);
						pgSetDrawStart(60, -1, 0, 0);
						pgEditBox(BOX_TEXT_BG_COLOR, 58, 58, 300, 74);
						pgPrint(mail, BLACK, BOX_TEXT_BG_COLOR, 300);
						pgFillvram(FORM_INDEX + 3, 310, 60, 12, 12, 2);
						pgSetDrawStart(310, 60, 0, 0);
						if (sage)
						{
							pgPrint("●", WHITE, FORM_BG_COLOR, SCR_WIDTH);
						}
						else
						{
							pgPrint("○", WHITE, FORM_BG_COLOR, SCR_WIDTH);
						}
						pgPrint("sage (← →キーで切替)", WHITE, FORM_BG_COLOR, SCR_WIDTH);
						break;
					case 2:
						pgSetDrawStart(10, 90, 0, 0);
						pgEditBox(BOX_TEXT_BG_COLOR, 8, 88, 470, 250);
						str = message;
						while ((str = pgPrint(str, BLACK, BOX_TEXT_BG_COLOR, 470)))
						{
							pgSetDrawStart(10, -1, 0, LINE_PITCH);
							if (winParam->pgCursorY >= 250)
							{
								break;
							}
						}
						break;
					}
				}
            }
        }
		pgFillvram(s2ch.formColor.title_bg, 0, 0, SCR_WIDTH, 15, 2);
		pgSetDrawStart(10, 1, 0, 0);
		pgPrint(subject, s2ch.formColor.title, s2ch.formColor.title_bg, SCR_WIDTH);
		sceKernelLibcTime(&resTime);
		if (resTime < OldResTime) OldResTime = 0;
		if (s2ch.cfg.timecount && resTime - OldResTime < waitTime){	// samba24設定時間内なら
			sprintf( buf, " (%3d/%3d秒)", (int)(resTime - OldResTime), (int)waitTime );
			winParam->pgCursorX = SCR_WIDTH - FONT_HEIGHT * 6;
			pgFillvram(s2ch.formColor.title_bg, winParam->pgCursorX, 0, SCR_WIDTH - winParam->pgCursorX, FONT_HEIGHT+1, 2);
			pgPrint(buf, s2ch.formColor.title, s2ch.formColor.title_bg, SCR_WIDTH);
		}
        pgPrintMenuBar(menuStr);
		pgDrawTexture(-1);
		pgCopyMenuBar();
    	flipScreen(0);
    }
    if (changeFlag)
    {
        sprintf(buf, "%s/form.ini", s2ch.cfg.logDir);
        fd = sceIoOpen(buf, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
        if (fd >= 0)
        {
            sprintf(buf, "%s\n%s\n", name, mail);
            psp2chFileWrite(fd, buf, strlen(buf));
            sceIoClose(fd);
        }
    }
	pgDeleteTexture();
    return ret;
}

void psp2chClearCookieFor2ch(void)
{
	memset(cookie_2ch, 0, NET_COOKIE_LENGTH);
	return;
}

int psp2chInputKana(char* buf, unsigned int max)
{
	const char kana_table[] = 
	{
		'ｱ', 'ｲ', 'ｳ', 'ｴ', 'ｵ',
		'ｶ', 'ｷ', 'ｸ', 'ｹ', 'ｺ',
		'ｻ', 'ｼ', 'ｽ', 'ｾ', 'ｿ',
		'ﾀ', 'ﾁ', 'ﾂ', 'ﾃ', 'ﾄ',
		'ﾅ', 'ﾆ', 'ﾇ', 'ﾈ', 'ﾉ',
		'ﾊ', 'ﾋ', 'ﾌ', 'ﾍ', 'ﾎ',
		'ﾏ', 'ﾐ', 'ﾑ', 'ﾒ', 'ﾓ',
		'ﾔ', ' ', 'ﾕ', ' ', 'ﾖ',
		'ﾗ', 'ﾘ', 'ﾙ', 'ﾚ', 'ﾛ',
		'ﾜ', 'ｦ', 'ﾝ', '､', '｡',
		'ｧ', 'ｨ', 'ｩ', 'ｪ', 'ｫ',
		'ｬ', 'ｭ', 'ｮ', 'ｯ', 'ﾞ',
		'ﾟ', '｢', '｣', '･', 'ｰ'
	};
    const char* menuStr = "  ○ : 入力      × : 戻る      □ : 消す      △ : 改行";
    char inputBar[(5 + 1)* sizeof(char)];
    char *str;
    int change = 1, select = 0;
    unsigned int size;
    
    size = strlen(buf);
    pgPrintMenuBar((char*)menuStr);
    while (s2ch.running)
    {
		if (change)
		{
			change = 0;
			pgSetDrawStart(10, 90, 0, 0);
			pgEditBox(BOX_TEXT_BG_COLOR, 8, 88, 470, 250);
			str = buf;
			while ((str = pgPrint(str, BLACK, BOX_TEXT_BG_COLOR, 470)))
			{
				pgSetDrawStart(10, -1, 0, LINE_PITCH);
				if (winParam->pgCursorY >= 250)
				{
					break;
				}
			}
			pgSetDrawStart(200, 160, 0, 0);
			memcpy(inputBar, &kana_table[select], 5);
			inputBar[5] = '\0';
			pgFillvram(BLACK, 200, 160, 40, LINE_PITCH, 2);
			pgPrint(inputBar, WHITE, BOX_BG_COLOR, 400);
		}
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
        	if(s2ch.pad.Buttons != s2ch.oldPad.Buttons)
        	{
        		s2ch.oldPad = s2ch.pad;
        		switch(s2ch.pad.Buttons)
        		{
        			case PSP_CTRL_UP:
        				select -= 5;
        				break;
        			case PSP_CTRL_DOWN:
        				select += 5;
        				break;
        			case PSP_CTRL_LEFT:
        				select--;
        				break;
        			case PSP_CTRL_RIGHT:
        				select++;
        				break;
        			case PSP_CTRL_CIRCLE:
        				if (size < max)
        				{
        					*(buf + size) = kana_table[select];
        					size++;
        				}
        				break;
        			case PSP_CTRL_SQUARE:
        				if (size > 0)
        				{
        					size--;
        				}
        				break;
        			case PSP_CTRL_TRIANGLE:
        				if (size < max)
        				{
        					*(buf + size) = '\n';
        					size++;
        				}
        				break;
        			case PSP_CTRL_CROSS:
        				return 0;
        		}
        		if (select < 0)
        		{
        			select += sizeof(kana_table);
        		}
        		else if ((int)sizeof(kana_table) <= select)
        		{
        			select -= sizeof(kana_table);
        		}
        		*(buf + size) = '\0';
	            change = 1;
	        }
        }
        pgCopyWindow(0, 8, 88, 470 - 8, 250 - 88);
		pgCopyMenuBar();
		flipScreen(0);
    }
    return 0;
}

static int psp2chFormResPreview(char *host, char *dir, char *title, unsigned long dat, char *subject, char *name, char *mail, char *message)
{
	char header[512];
	char *str = message;
	int ret, be, i, count = 0, max = 0;
	
	sprintf(header, "000 %s [%s] 1970/01/01(木) 09:00:00 ID:xxxxxxxx Be:", name , mail);
	if (psp2chFormBeLoginCheck()) {
		pgPrintMenuBar("○で送信　□でBe送信(Beログイン中)　×で戻る");
	} else {
		pgPrintMenuBar("○で送信　□でBe送信(Beログアウト中)　×で戻る");
	}

	while ((str = strchr(str + 1, '\n')) != NULL)
		max++;

	while (s2ch.running)
    {
		pgFillvram(FORM_INDEX + 3, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
		pgSetDrawStart(0, 0, 0, 0);
		pgPrint(header, WHITE, 0, SCR_WIDTH);
		pgSetDrawStart(0, -1, 0, LINE_PITCH);
		
		str = message;
		for (i = 0; i < count && str != NULL; i++)
			str = strchr(str, '\n') + 1;
		if (str == NULL)
			str = message;
		
		while ((str = pgPrint(str, WHITE, 0, SCR_WIDTH)))
		{
			pgSetDrawStart(0, -1, 0, LINE_PITCH);
			if (winParam->pgCursorY >= 260)
			{
				break;
			}
		}
        sceCtrlPeekBufferPositive(&s2ch.pad, 1);
        if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
        {
            s2ch.oldPad = s2ch.pad;
            if(s2ch.pad.Buttons & PSP_CTRL_CIRCLE) {
            	be = 0;
            	break;
            }
            else if (s2ch.pad.Buttons & PSP_CTRL_SQUARE) {
            	be = 1;
            	break;
            }
            else if (s2ch.pad.Buttons & PSP_CTRL_TRIANGLE) {
            	be = 2;
            	break;
            }
            else if (s2ch.pad.Buttons & PSP_CTRL_DOWN) {
            	count = (count < max) ? count + 1 : max;
            }
            else if (s2ch.pad.Buttons & PSP_CTRL_UP) {
            	count = (count > 0) ? count - 1 : 0;
            }
            else if(s2ch.pad.Buttons & PSP_CTRL_CROSS)
                return 0;
        }
        pgCopy(0, 0);
        pgCopyMenuBar();
        flipScreen(0);
    }
    ret = psp2chFormResPost(host, dir, dat, subject, name, mail, message, be);
    return ret;
}
