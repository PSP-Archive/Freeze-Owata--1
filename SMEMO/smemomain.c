//==============================================================
// PSPメモ帳   (STPP.05)
//     for PSP CFW5.00 M33-6
// STEAR 2009-2010
//--------------------------------------------------------------
// Simple IMEによる日本語テキストエディタ。
// 普段見慣れている某エディタを手本にしてみました。
// 対応している改行コードはCRLFとLFです。CR単体には対応してません。
//
// このバージョンでは使用メモリ削減のためにintraFontを使用できなくしています。
//
// SMEMOtext() 指定されたテキストを編集する
// SMEMOfile() 指定されたファイル内容を編集する
//--------------------------------------------------------------

#include <pspuser.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <kubridge.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>

#include "graphics.h"
#include "zenkaku.h"
#include "draw.h"
#include "sime.h"
#include "strInput.h"
#include "filedialog.h"
#include "smemo.h"

#include "psp2ch.h"
#include "charConv.h"

//----- マクロ -----


//----- 定数設定 -----

#define	VERSION			"PSPメモ帳 Ver1.09α"					// アプリ名称
#define	INITFILE		"SMEMO.INI"								// 設定ファイル名
#define	TEMPFILE		"SMEMO_TEMP.TMP"						// 一時作業ファイル名（環境設定で使用）
#define	CLIPBOARD		"ms0:/PSP/COMMON/CLIPBOARD.TXT"			// クリップボードファイル
#define	HISFILEJUMP		"JUMP.HIS"
#define	HISFILESARH		"SARH.HIS"
#define	HISFILECHG		"CHANGE.HIS"
#define	HISFILEPASTE	"PASTE.HIS"
#define	HISFILEFONT		"FONT.HIS"
#define	HISFILEFILE		"FILE.HIS"
#define	TBY				(24)									// テキスト領域の開始Y座標
#define	LWIDTH			(475)									// 画面上一行に表示できるドット数
#define	WWIDTH			(900)									// 仮想画面上一行に表示できるドット数
#define	PAGEMOVE		(12)									// ページ送りの行数
#define	TEXTBLSIZE		(30)									// 行レコード取得時の初期本文領域サイズ
#define	UNDOSIZE		(2000)									// 「やり直し」記録文字数

//----- プロトタイプ宣言 -----


//- システム関連 -

//int		SetupCallbacks(void);
//int		callbackThread(SceSize args, void *argp);
//int		exitCallback(int arg1, int arg2, void *common);

//- その他 -

int		MenuDLGyn(char *msg,long corFrm,long corIn);

//------ グローバル変数 -----

extern S_2CH s2ch; // psp2ch.c

unsigned int __attribute__((aligned(16)))	gList[1000];
//volatile int								gExitRequest = 0;

static strInpHis	**gHisLJump,								// 入力履歴（行ジャンプ）
					**gHisSarh,									// 入力履歴（検索）
					**gHisChg,									// 入力履歴（置換え）
					**gHisFPaste,								// 入力履歴（ファイルからコピペ）
					**gHisFont,									// 入力履歴（フォント指定）
					**gHisFile;									// 入力履歴（文章を開く）

static char	gFolder[1024],										// 作業しているフォルダ
			gPFolder[1024],										// ファイルから貼り付け時の対象フォルダ
			gMonaFile[4][1024];									// monafontファイル名管理

long gSCor[] = {												// システム関連の配色
				0xFFFFFF,										// 文字色
				0x80FF80,										// 通知ダイアログ（枠）
				0x185018,										// 通知ダイアログ（内部）
				0x8080A0,										// 警告ダイアログ（枠）
				0x404090,										// 警告ダイアログ（内部）
				0x103010,										// 通知ダイアログ（背景）
				0x40A040,										// スクロールバー（枠）
				0x306030,										// スクロールバー（内部）
				0x90F090,										// クスロールバー（バー）
				0x40FFFF,										// ディレクトリの文字色
				0xC04020,										// カーソル（枠）
				0x602010,										// カーソル（内部）
				0x809080,										// メニューの選択不可項目
				0xFFC0C0,										// ダイアログのキーガイダンス
				0x40A040,										// プログレスバーの色
			};
static long	gCor[] = {
				0x804040,										// タイトル
				0xFFFFFF,										// タイトル部文字色
				0x000000,										// 背景色
				0xFFFFFF,										// 文字色
				0x40A040,										// スクロールバー（枠）
				0x306030,										// スクロールバー（内部）
				0xB0FFB0,										// クスロールバー（バー）
				0x707070,										// タブ文字
				0x80FF80,										// 改行文字
				0x80FF80,										// [EOF]文字
				0x80FFFF,										// その他のコントロールコード
				0x707070										// 全角空白文字
			};

static char	*gTextClip;											// テキストのコピペ用

static int	gSave,												// ファイルが変更されているか（0:No）
			gEnvSave,											// 環境設定が変更されているか（0:No）
			gTCChg,												// テキストクリップの内容に変更があったか
			gCRLF,												// 改行コード（0:LF 1:CRLF）
			gTab = 0,											// タブ幅（0:4文字 以外:8文字）
			gPSpc = 0,											// 全角空白を記号表示するか（0:No 以外:Yes）
			gPTab = 0,											// タブを記号表示するか（0:No 以外:Yes）
			gFont = 1,											// 表示に使うフォント（0:東雲フォント 1:monafont 2:intraFont 3:intraFontP）
			gIME = 0,											// IMEの種類（0:Simple IME 以外:OSK）
			gRoll = 0,											// 横方向を拡張表示するか（0:画面固定モード 1:仮想画面モード）
			gLineLen,											// 行レコードの本文のサイズ
			gFHeight,											// フォントの高さ
			gFWidth,											// フォントの最大幅
			gSLine,												// 画面上に表示出来る行数
			gReDraw,											// 最下行がはみ出すか（0:出ない 以外:出る）
			gUndoSize,											// 「やり直し」データ数
			gUndoPos;											// 「やり直し」データ先頭位置
static long	gLineMax,											// 全体の行数（=使用されている行レコードの総数）
			gLineListMax;										// 行レコードリストの個数

static float	gXDraw,											// 作画を行う部分のサイズ
				gXOffset,										// 文字表示開始位置（左右スクロール用）
				gXShift;										// 画面内左側の作画しない部分のサイズ

static struct strText{
	long			chainBack;									// リストチェイン（前のレコード№）
																//     0xFFFFFFFFなら未使用レコード、0xFFFFFFFEなら先頭行
	long			chainNext;									// リストチェイン（次のレコード№）
																//     0xFFFFFFFFなら最終行
	unsigned int	back : 1;									// 1 前のレコードから続いている
	unsigned int	next : 1;									// 1 次のレコードに続いている
	unsigned int	enter : 1;									// 1 改行コードあり
	int				textsize;									// 本文用に確保されている領域のサイズ
	int				len;										// 本文の長さ
	char			*text;										// 本文へのポインタ（実体は使うときに別に確保する）
} **gText;														// 行管理（画面上の一行が一つのレコードに対応する）

static struct {													// テキスト範囲指定位置管理用
	int		cxr;
	long	rno;
	long	line;
} gTSel[3];

struct strCsrSts{												// カーソル位置指定用
	int		cx;													// 画面上での物理カーソルX位置（文字単位）
	float	sx;													// 画面上での物理カーソルX位置（ドット単位）
	int		cxr;												// 論理カーソルX位置
	float	cxb;												// カーソルX位置指定時の到達目標ポイント（ドット単位）
	int		cy;													// 画面上での物理カーソルY位置
	long	pline;												// 画面上の表示開始行№
	long	rno;												// カーソル位置のレコード№
	long	rline;												// カーソル位置の論理行№
	int		adjust;												// 画面位置をカーソルに合わせる（0:そのまま 1:合わせる）
};

static struct {													// 「やり直し」管理用
	long	pline;												// 操作が行われた物理行レコード
	int		cxr;												// 操作が行われた行レコード内論理位置
	char	str[3];												// str[0]～[1] 入力/削除文字
																// str[2]が1なら文字入力、0なら文字削除
} gUndo[UNDOSIZE];


//------ 実行モード指定 -----

#ifndef	PSP_MODULE_KERNEL
#define	PSP_MODULE_KERNEL	0x1000
#endif
#ifndef	PSP_MODULE_USER
#define	PSP_MODULE_USER		0
#endif

//PSP_MODULE_INFO( "PSPメモ帳", PSP_MODULE_USER, 1, 1 );
//PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
//PSP_HEAP_SIZE_KB(-800);


//==============================================================
// ボタンが押されるまで待機
//--------------------------------------------------------------

void WaitBtn(void)
{
	SceCtrlData		pad;

	ScreenView();												// 画面更新
	while (!gExitRequest){										// 何か押されるまで待機
		sceCtrlReadBufferPositive( &pad, 1 );
		if (SIMEgetchar(pad)!=0) break;
		sceDisplayWaitVblankStart();
	}
}

//==============================================================
// ボタンが離されるまで待機
//--------------------------------------------------------------

void Unpush(void)
{
	SceCtrlData		pad;

	ScreenView();												// 画面更新
	while (1){
		sceCtrlReadBufferPositive(&pad, 1);
		if (!(pad.Buttons & 0x00F3F9)) break;					// 操作系ボタンが全て離された
		sceDisplayWaitVblankStart();
	}
}

//==============================================================
// タイトル部作画
//--------------------------------------------------------------

void DrawTitle(char *path)
{
	char	str[65],filename[256];
	int		pos,len;

	pos = 0;
	if (s2ch.cfg.hbl){											// HBL環境下ではファイル名のUTF8→シフトJIS変換を行う
		psp2chUTF82Sjis(filename, path);
	} else {
		strcpy(filename, path);
	}
	len = strlen(filename);
	if (len>61) pos = len - 61;
	strcpy( str, &filename[pos] );								// 画面からはみ出さないように文字数制限
	if (gSave)
		strcat( str, " (更新)" );
	CurveBox( 0, 0, 480, 21, 3, gCor[0], gCor[0] );
	mh_print( 6, 1, str, gCor[1] );
	Fill( LWIDTH, 14, 5, 8, gCor[0] );
	HLine( 0, 22, LWIDTH, gCor[3] );
}

//==============================================================
// ステータス表示
//--------------------------------------------------------------
// flag 1:カーソル位置とルーラーの強制作画
//      2:ルーラーの強制作画
//--------------------------------------------------------------

void DrawSts(struct strCsrSts *cs,int flag)
{
	static int	bx,lxBak = 0;
	static long	lineBak = 0;
	int			i,wy,lx,len;
	float		cw,cx;
	long		rno,pos,rline;

	if (flag==1 || lineBak!=cs->rline){							// 変化していない時は作画しない
		rline = (cs->rline) +1;
		if (rline>999999) rline = 999999;
		Fill( 414, 1, 36, 12, gCor[0] );
		DrawVal( 414, 1, (cs->rline)+1, 0, 1, gCor[1] );		// 論理行位置
		lineBak = cs->rline;
	}
	mh_print( 450, 1, ":", gCor[1] );
	lx = cs->cx;
	rno = cs->rno;
	while (gText[rno]->back){									// 複数行に折り返されている場合
		rno = gText[rno]->chainBack;
		pos = 0;
		len = 0;
		while (pos<gText[rno]->len){							// 行の物理文字数を調べる
			if (gText[rno]->text[pos++]=='\t'){
				if (gTab){
					len = (len + 8) & 0xFF8;					// TAB=8
				} else {
					len = (len + 4) & 0xFFC;					// TAB=4
				}
			} else {
				len++;
			}
		}
		lx += len;
		if (lx>998){
			lx = 998;
			break;
		}
	}
	if (flag==1 || lxBak!=lx){
		Fill( 456, 1, 18, 12, gCor[0] );
		DrawVal( 456, 1, lx+1, -1, 1, gCor[1] );				// 列位置
		lxBak = lx;
	}

	cw = GetStrWidth( gFont,"漢" )/2;
	if (flag){													// ルーラー部作画
		Fill( 0, 14, LWIDTH, 9, gCor[2] );
		HLine( 0, 22, LWIDTH, gCor[3] );
		i = 0;
		while (i*cw<LWIDTH-1+gXOffset){
			wy = 2;
			if (i%8==1) wy = 1;
			if (i%4==0) wy += 2;
			if (i%8==0){
				wy += 3;
				cx = i*cw - gXOffset;
				if (cx>=0 && cx<LWIDTH-20){
					DrawVal( cx+2, 15, i, -1, 0, gCor[3] );
				}
			}
			cx = i*cw - gXOffset;
			if (cx>=0){
				VLine( cx, 22-wy, wy, gCor[3] );
			}
			i++;
		}
	} else {
		cx = 1 + bx*cw - gXOffset;
		if (cx>=0 && cx<LWIDTH-cw-1)
			XFill( cx, 14, cw-1, 8, gCor[1] );					// カーソル位置（消去）
	}
	cx = 1 + (cs->cx)*cw - gXOffset;
	if (cx>=0 && cx<LWIDTH-cw-1)
		XFill( cx, 14, cw-1, 8, gCor[1] );						// カーソル位置（表示）
	bx = cs->cx;
	VRollBar( LWIDTH, 22, 272-22, cs->pline, gSLine, gLineMax, flag, gCor[4], gCor[5], gCor[6] );
}

//==============================================================
// 指定行レコードを削除
//--------------------------------------------------------------

void DelRec(long rno)
{
	if (!gText) return;

	if (gText[rno]){
		free(gText[rno]->text);									// 本文用領域を解放
		free(gText[rno]);										// 行レコードを解放
		gText[rno] = NULL;										// 対応するレコードリストを未使用に
		gLineMax--;
	}
}

//==============================================================
// 新しい行レコードを取得する
//--------------------------------------------------------------
// size   初期本文領域サイズ（0:領域は確保しない）
// 戻り値  -1:メモリが足りない
//        0～:取得した行レコードナンバー
//--------------------------------------------------------------
// 本文用の領域は最初最低限を確保し、後で足りなくなったらその都度一定数ずつ
// 拡張させます。
//--------------------------------------------------------------

long GetNewRec(int size)
{
	struct strText	*tmp,**text;
	char	*tmpc;
	long	i,rno;

	if (gLineMax+1>=gLineListMax){								// レコードリストが足りないなら追加
		text = (struct strText**) realloc( gText,sizeof(struct strText**)*(gLineListMax+1000) );
		if (text==NULL) return (-1);
		gText = text;
		for (i=gLineListMax; i<gLineListMax+1000 ;i++){
			gText[i] = NULL;
		}
		gLineListMax += 1000;
	}
	rno = -1;
	if (gText[gLineMax+1]==NULL){
		rno = gLineMax +1;
	} else {
		for (i=gLineMax; i<gLineListMax ;i++){					// 使用領域の最後の方から未使用領域を探してみる
			if (gText[i]==NULL){
				rno = i;
				break;
			}
		}
		if (rno==-1){
			for (i=0; i<gLineListMax ;i++){						// 未使用領域を探す
				if (gText[i]==NULL){
					rno = i;
					break;
				}
			}
		}
	}
	if (rno==-1) return (-1);

	tmp = (struct strText*) malloc(sizeof(struct strText));
	if (tmp==NULL) return (-1);
	gText[rno] = tmp;
	gText[rno]->chainBack = 0xFFFFFFFF;
	gText[rno]->chainNext = 0xFFFFFFFF;
	gText[rno]->back = 0;
	gText[rno]->next = 0;
	gText[rno]->enter = 0;
	if (size>gLineLen) size = gLineLen;
	gText[rno]->textsize = size;
	if (size){
		tmpc = (char*) malloc(gText[rno]->textsize);
		if (tmpc==NULL){										// 本文用領域を確保出来なかった
			free(gText[rno]);
			gText[rno] = NULL;
			return (-1);
		}
		gText[rno]->text = tmpc;
		memset( gText[rno]->text, 0, gText[rno]->textsize);
	} else {
		gText[rno]->text = NULL;
	}
	gText[rno]->len = 0;
	gLineMax++;
	return (rno);
}

//==============================================================
// 行レコードの本文用領域の拡張
//--------------------------------------------------------------
// rno 行レコード№（0xFFFFFFFFだと何もしない）
// len 本文領域がこれより小さい場合、拡張される
// 戻り値  0:正常終了
//        -1:メモリが足りない
//--------------------------------------------------------------
// 指定された行レコードの本文領域が指定文字数より小さい場合、本文領域を拡張
// させる。
// 拡張する場合は指定値より+50バイト多め（ただしgLineLenは超えない）に拡張します。
// ちなみに一度拡張された本文領域は行レコードを削除しない限り縮小されることは
// ありません。
//--------------------------------------------------------------

int AddRecText(long rno,int len)
{
	char	*buf;

	if (rno!=0xFFFFFFFF){
		if (gText[rno]->textsize<=len){
			len += 50;
			if (len>gLineLen) len = gLineLen;
			buf = (char*) realloc( gText[rno]->text,len );
			if (buf==NULL) return (-1);							// メモリが確保できない
			gText[rno]->textsize = len;
			gText[rno]->text = buf;
		}
	}
	return (0);
}

//==============================================================
// 行レコード構造体の全削除
//--------------------------------------------------------------
// 既に確保されている行レコード関連のメモリを全て解放する。
//--------------------------------------------------------------

void ClrRec(void)
{
	long	i;

	for (i=0; i<gLineListMax ;i++){								// 行レコードを全て解放
		DelRec(i);
	}
	free(gText);												// 行レコードリストを解放
	gText = NULL;
	gLineListMax = 0;
}

//==============================================================
// 行レコードの全初期化
//--------------------------------------------------------------
// 既に確保されている行レコード関連のメモリを全て解放した後、一行分の行レコード
// を確保します。
//--------------------------------------------------------------

void InitRec(void)
{
	long	i;

	ClrRec();													// 行レコードを全て解放
	gLineListMax = 1000;
	gText = (struct strText**) malloc( sizeof(struct strText*) * gLineListMax );
	for (i=0; i<gLineListMax ;i++){
		gText[i] = NULL;
	}

	gLineMax = -1;
	GetNewRec(TEXTBLSIZE);
	gText[0]->chainBack = 0xFFFFFFFE;
	gLineMax = 0;

	gUndoSize = 0;												// 「やり直し」初期化
	gUndoPos = 0;
	gSave = 0;
	gCRLF = 1;													// デフォルトの改行コードはCR,LF
	gTSel[0].rno = -1;											// 範囲選択を解除
	gTSel[1].rno = -1;											// 範囲選択を解除
	gXOffset = 0;												// 左右スクロール用
	gXShift = 0;
	gXDraw = LWIDTH;
}

//==============================================================
// ドット単位で次のタブ位置を求める
//--------------------------------------------------------------
// 全角文字の半分の幅を基準にしています。
//--------------------------------------------------------------

float GetTabPos(float x)
{
	float	px,wx;

	wx = GetStrWidth(gFont,"漢") * (gTab ? 8 : 4) /2;			// '漢'に特別な意味はない、全角文字幅が知りたいだけ
	px = 0;
	while (px<=x){
		px += wx;
	}
	return (px);
}

//==============================================================
// ファイル読み込み
//--------------------------------------------------------------
// 戻り値  2:ユーザーにより中断された
//         1:行レコードが取得出来ない（メモリが足りない）
//         0:成功
//        -1:失敗（ワークメモリが足りない）
//        -2:失敗（ファイルが開けない）
//--------------------------------------------------------------
// 行レコードの取得に失敗した場合、その時点で読み込みを終了します。
// テキストは4096バイトずつ読み込みながらレコード単位に分割しています。
// 行レコードの本文領域は最初最大値で確保した後、内容が確定した時点でサイズを
// 最適化し余計なメモリを使わないようにし…ようとしたが、効果なし。
// reallocで縮小させてもメモリは解放されないらしい。
// 仕方がないので本文が確定した時点で本文領域を確保するようにしています。
//--------------------------------------------------------------

//----- 行レコードの本文をセット -----

int fileloadText(long rno,char *text,int cnt)
{
	char	*buf;
	long	rno2;

	buf = (char*) malloc(cnt+1);								// 本文領域の確保
	if (buf){													// 確保できたら
		gText[rno]->text = buf;
		gText[rno]->textsize = cnt +1;
		memcpy( gText[rno]->text,text,cnt );
		gText[rno]->len = cnt;
		return (0);
	} else {													// メモリが足りない
		rno2 = gText[rno]->chainBack;
		DelRec(rno);
		gText[rno2]->chainNext = 0xFFFFFFFF;
		gText[rno2]->next = 0;
		return (-1);
	}
}

//----- 次の行レコードを作成 -----

int fileloadNextLine(long *rno,char *text,int *cnt,int *len,float *wx)
{
	long	rno2;

	if (fileloadText( *rno,text,*cnt )) return (-1);			// 本文をセット
	*cnt = 0;
	*len = 0;
	*wx = 0;
	rno2 = GetNewRec(0);
	if (rno2==-1) return (-1);									// 行レコードが取得出来ない
	gText[*rno]->chainNext = rno2;
	gText[*rno]->next = 1;										// 次の行に続いている
	gText[rno2]->chainBack = (*rno);
	gText[rno2]->back = 1;										// 前の行から続いている
	*rno = rno2;
	return (0);
}

//----- メイン -----

int fileload(char *fname)
{
	const int		ws[5] = {16384,4096,1024,128};
	SceCtrlData		pad;
	char	*data,text[WWIDTH/2+2],msg[128],c[3] = {0,0,0};
	int		i,fd,cnt,len,ret,worksize;
	long	filesize,pos,poscnt,rno,rno2;
	float	wc,wx;

	//----- プログレスバー -----

	Progressbar1( -1, 135, 0, "×:キャンセル", gSCor[7], gSCor[13], gSCor[1], gSCor[2] );
	ScreenView();												// 画面更新

	//----- データレコード初期化 -----

	InitRec();

	//----- ファイル読み込み -----

	msg[0] = '\0';
	filesize = 0;
	fd = sceIoOpen( fname, PSP_O_RDONLY, 0777 );
	data = NULL;
	if (fd>=0){
		filesize = sceIoLseek( fd, 0, SEEK_END );
		sceIoLseek( fd, 0, SEEK_SET );
		for (i=0; i<4 ;i++){									// 出来るだけ大きくバッファを確保する
			worksize = ws[i];
			data = (char*) malloc(worksize);
			if (data) break;									// バッファの確保に成功した
		}
		if (!data){												// メモリを確保できなかった
			sceIoClose(fd);
			strcpy( msg, "ワークメモリを確保できませんでした" );
			DiaBox1( -1, 120, 0, msg, gSCor[0], gSCor[3], gSCor[4] );	// 中央に表示
		} else {
			sceIoRead( fd, data, worksize );
		}
	} else {
		return (-2);											// ファイルが開けなかった
	}

	if (msg[0]){												// エラー発生
		WaitBtn();												// 何か押されるまで待機
		return (-1);
	}

	//----- レコード単位に分割 -----

	pos = 0;
	poscnt = 0;
	cnt = 0;
	len = 0;
	wx = 0;
	gCRLF = 0;
	ret = 0;
	rno = 0;
	while (filesize>poscnt+pos && !gExitRequest){
		c[0] = data[pos++];
		if (pos>=worksize){													// テキストをリロード
			poscnt += pos;
			pos = 0;
			sceIoRead( fd, data, worksize );
			Progressbar2( poscnt, filesize, gSCor[14] );				// プログレスバーの更新
			ScreenView();												// 画面更新
			sceCtrlReadBufferPositive( &pad, 1 );
			if (SIMEgetchar(pad)==SIME_KEY_CROSS){
				ret = 2;
				break;
			}
		}
		if (c[0]==0) c[0] = 1;											// バイナリデータ'\0'が含まれると不都合が発生するので
		if (c[0]=='\r'){												// CRは捨てる
			gCRLF = 1;
		} else if (c[0]=='\n'){											// LF発見
			text[cnt++] = '\n';
			gText[rno]->len = cnt;
			if (fileloadText( rno,text,cnt )){							// 本文をセット
				ret = 1;
				break;
			}
			cnt = 0;
			len = 0;
			wx = 0;
			rno2 = GetNewRec(0);
			if (rno2==-1){												// 行レコードが取得できない
				ret = 1;
				break;
			}
			gText[rno]->chainNext = rno2;
			gText[rno]->enter = 1;										// 改行あり
			gText[rno2]->chainBack = rno;
			rno = rno2;
		} else {
			if (chkSJIS(c[0])){											// 全角文字だった場合
				c[1] = data[pos++];
				if (pos>=worksize){											// テキストをリロード
					poscnt += pos;
					pos = 0;
					sceIoRead( fd, data, worksize );
					Progressbar2( poscnt, filesize, gSCor[14] );		// プログレスバーの更新
					ScreenView();										// 画面更新
					sceCtrlReadBufferPositive( &pad, 1 );
					if (SIMEgetchar(pad)==SIME_KEY_CROSS){
						ret = 2;
						break;
					}
				}
				wc = GetStrWidth( gFont,c );
				if (len>=gLineLen-1 || (wx+wc>=(gRoll ? WWIDTH : LWIDTH))){	// 行に収まらない
					if (fileloadNextLine( &rno, text, &cnt, &len, &wx )){	// 次の行を作成
						ret = 1;
						break;
					}
				}
				text[cnt++] = c[0];
				text[cnt++] = c[1];
				len += 2;
				wx += wc;
			} else {													// 半角文字だった場合
				c[1] = '\0';
				wc = GetStrWidth( gFont,c );
				if (wx+wc>=(gRoll ? WWIDTH : LWIDTH) && c[0]!='\t'){	// 行に収まらない
					if (fileloadNextLine( &rno, text, &cnt, &len, &wx )){	// 次の行を作成
						ret = 1;
						break;
					}
				}
				text[cnt++] = c[0];
				if (c[0]=='\t'){										// タブ
					if (gTab){
						len = (len + 8) & 0xFF8;						// TAB=8
					} else {
						len = (len + 4) & 0xFFC;						// TAB=4
					}
					wx = GetTabPos(wx);
					wc = 0;
				} else {
					len++;
					wx += wc;
				}
				if (len>=gLineLen || wx+wc>=(gRoll ? WWIDTH : LWIDTH)){
					if (fileloadNextLine( &rno, text,&cnt, &len, &wx )){	// 次の行を作成
						ret = 1;
						break;
					}
				}
			}
		}
	}
	fileloadText( rno,text,cnt+1 );										// 最終行が空行だった場合の対策
	gText[rno]->len = cnt;
	gText[rno]->chainNext = 0xFFFFFFFF;

	//----- 後始末 -----

	if (gExitRequest) ret = 2;
	sceIoClose(fd);
	free(data);
	return (ret);
}

//==============================================================
// ファイル書き込み
//--------------------------------------------------------------
// 戻り値  0:成功
//        -1:ワークメモリの取得に失敗
//        -2:保存先ファイルのオープンに失敗
//--------------------------------------------------------------
// ワーク領域上にプレーンテキスト化しながらファイルに書き出しています。
// ワーク領域はサイズごとに何パターンか確保にトライし、可能な限り大きい領域を
// 取得するようにしています。
// メモリ残量が少ない場合、処理に長時間かかる事もあるでしょう。
// 改行コードは読み込み元と同じになるように補正します。
//--------------------------------------------------------------

int filesave(char *fname)
{
	char	*data;
	int		i,fd;
	long	ws[5] = {65536,16384,4096,1024,WWIDTH/2};
	long	rno,pos,line,worksize;

	//----- ワーク領域の確保 -----

	for (i=0; i<5 ;i++){										// ワーク領域の大きさを決定
		worksize = ws[i];
		data = (char*) malloc(worksize);
		if (data) break;
	}
	if (data==NULL) return (-1);								// ワーク領域が確保できなかった

	//----- プログレスバー -----

	Progressbar1( -1, 135, 0, "キャンセル出来ません", gSCor[7], gSCor[13], gSCor[1], gSCor[2] );
	ScreenView();												// 画面更新

	//----- 本文をファイルへ書き出し -----

	fd = sceIoOpen( fname, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if (fd<0){
		free(data);
		return (-2);											// ファイルが開けない
	}
	rno = 0;
	pos = 0;
	line = 0;
	do{
		if (pos+gText[rno]->len > worksize){					// ワークに収まらないならファイルに出力
			Progressbar2( line, gLineMax, gSCor[14] );			// プログレスバーの更新
			ScreenView();										// 画面更新
			sceIoWrite( fd, data, pos );
			pos = 0;
		}
		memcpy( &data[pos], gText[rno]->text, gText[rno]->len );
		if (gText[rno]->enter && gCRLF){						// 改行コード修正
			data[pos+gText[rno]->len-1] = '\r';
			data[pos+gText[rno]->len] = '\n';
			pos++;
		}
		pos += gText[rno]->len;
		line++;
		rno = gText[rno]->chainNext;
	}while (rno!=0xFFFFFFFF && !gExitRequest);
	if (pos){													// ワークに残っているテキストをファイルへ
		sceIoWrite( fd, data, pos );
	}
	sceIoClose(fd);

	//----- 終了処理 -----

	gSave = 0;
	free(data);
	return (0);
}

//==============================================================
// 環境データの読み込み
//--------------------------------------------------------------

void LoadEnv(void)
{
	const char		it[12][11] = {"FONT","ROLL","IME","TABW","SPCP","TABP","FOLDER1","FOLDER2","FONT12A","FONT12W","FONT16A","FONT16W"};
	char	c,path[1024],buf[1030],*p;
	int		i,fd,no;
	FILE*	fp1;

	gEnvSave = 0;												// 環境設定が変更されているか
	if (getcwd( path,1024 )==NULL){								// カレントディレクトリの取得
		strcpy( path,"ms0:");
	}
	strcat( path,"/" );
	strcat( path,INITFILE );
	fp1 = fopen( path,"r" );
	if (fp1!=NULL){
		while (1){												// トークンを切り出し、各変数に値を設定する
			if (fgets( buf,1030,fp1 )==NULL) break;
			for (i=0; i<strlen(buf) ;i++){						// 改行コードを削除
				if (buf[i]<32 && buf[i]!='\t') buf[i] = '\0';
			}
			p = strtok( buf," =\t" );
			no = -1;
			for (i=0; i<12 ;i++){
				if (strcmp(p,it[i])==0) no = i;
			}
			p = strtok( NULL," =\t" );
			c = (unsigned char) p[0] - '0';
			switch (no){
			case 0:
				if (c>4) c = 0;
				gFont = c;
				break;
			case 1:
				if (c>1) c = 0;
				gRoll = c;
				break;
			case 2:
				if (c>1) c = 0;
				gIME = c;
				break;
			case 3:
				if (c>1) c = 0;
				gTab = c;
				break;
			case 4:
				if (c>1) c = 0;
				gPSpc = c;
				break;
			case 5:
				if (c>1) c = 0;
				gPTab = c;
				break;
			case 6:
				fd = sceIoDopen(p);
				sceIoDclose(fd);
				if (fd>=0){										// フォルダが存在するなら
					strncpy( gFolder,p,1023 );
					gFolder[1023] = '\0';
				}
				break;
			case 7:
				fd = sceIoDopen(p);
				sceIoDclose(fd);
				if (fd>=0){										// フォルダが存在するなら
					strncpy( gPFolder,p,1023 );
					gPFolder[1023] = '\0';
				}
				break;
			case 8:
				fd = sceIoOpen( p, PSP_O_RDONLY, 0777 );
				sceIoDclose(fd);
				if (fd>=0){										// ファイルが存在するなら
					strncpy( gMonaFile[0],p,1023 );
					gMonaFile[0][1023] = '\0';
				}
				break;
			case 9:
				fd = sceIoOpen( p, PSP_O_RDONLY, 0777 );
				sceIoDclose(fd);
				if (fd>=0){										// ファイルが存在するなら
					strncpy( gMonaFile[1],p,1023 );
					gMonaFile[1][1023] = '\0';
				}
				break;
			case 10:
				fd = sceIoOpen( p, PSP_O_RDONLY, 0777 );
				sceIoDclose(fd);
				if (fd>=0){										// ファイルが存在するなら
					strncpy( gMonaFile[2],p,1023 );
					gMonaFile[2][1023] = '\0';
				}
				break;
			case 11:
				fd = sceIoOpen( p, PSP_O_RDONLY, 0777 );
				sceIoDclose(fd);
				if (fd>=0){										// ファイルが存在するなら
					strncpy( gMonaFile[3],p,1023 );
					gMonaFile[3][1023] = '\0';
				}
				break;
			}
		}
		fclose (fp1);
	}
}

//==============================================================
// 環境データの保存
//--------------------------------------------------------------

//----- 数値系 -----

int SaveEnvSub1(char *buf,int pos,int item,int data)
{
	const char	it[6][5] = {"FONT","ROLL","IME","TABW","SPCP","TABP"};
	int	i;

	for (i=0; i<strlen(it[item]) ;i++){
		buf[pos++] = it[item][i];
	}
	buf[pos++] = ' ';
	buf[pos++] = '=';
	buf[pos++] = ' ';
	buf[pos++] = '0' + data;
	buf[pos++] = '\r';
	buf[pos++] = '\n';
	return (pos);
}

//----- 文字列系 -----

int SaveEnvSub2(char *buf,int pos,int item,char *data)
{
	const char	it[6][11] = {"FOLDER1 = ","FOLDER2 = ","FONT12A = ","FONT12W = ","FONT16A = ","FONT16W = "};
	int	i;

	for (i=0; i<strlen(it[item]) ;i++){
		buf[pos++] = it[item][i];
	}
	for (i=0; i<strlen(data) ;i++){
		buf[pos++] = data[i];
	}
	buf[pos++] = '\r';
	buf[pos++] = '\n';
	return (pos);
}

//----- メイン -----

void SaveEnv(void)
{
	char	path[1024],buf[3100];
	int		fd,pos;

	if (gEnvSave==0) return;									// 環境設定が変更されていないなら終了

	pos = 0;
	pos = SaveEnvSub1( buf, pos, 0, gFont );
	pos = SaveEnvSub1( buf, pos, 1, gRoll );
	pos = SaveEnvSub1( buf, pos, 2, gIME );
	pos = SaveEnvSub1( buf, pos, 3, gTab );
	pos = SaveEnvSub1( buf, pos, 4, gPSpc );
	pos = SaveEnvSub1( buf, pos, 5, gPTab );
	pos = SaveEnvSub2( buf, pos, 0, gFolder );
	pos = SaveEnvSub2( buf, pos, 1, gPFolder );
	pos = SaveEnvSub2( buf, pos, 2, gMonaFile[0] );
	pos = SaveEnvSub2( buf, pos, 3, gMonaFile[1] );
	pos = SaveEnvSub2( buf, pos, 4, gMonaFile[2] );
	pos = SaveEnvSub2( buf, pos, 5, gMonaFile[3] );

	if (getcwd( path,1024 )==NULL){								// カレントディレクトリの取得
		strcpy( path,"ms0:");
	}
	strcat( path,"/" );
	strcat( path,INITFILE );
	fd = sceIoOpen( path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if (fd>=0){
		sceIoWrite( fd, buf, pos );
		sceIoClose(fd);
	}
}

//==============================================================
// 環境変更時の各種画面関係のパラメータを設定
//--------------------------------------------------------------

void setScreen(void)
{
	if (gFont==2){												// 16ドットmonafont
		gSLine = 14;											// 画面行数
		gFHeight = 18;											// 行幅
		gFWidth = 16;											// 最大フォント幅
		gReDraw = 1;											// 最下行がはみ出す
	} else {													// その他のフォント
		gSLine = 19;											// 画面行数
		gFHeight = 13;											// 行幅
		gFWidth = 12;											// 最大フォント幅
		gReDraw = 0;											// 最下行がはみ出さない
	}

	if (gRoll){													// 横方向のサイズに合わせて行レコードの本文のサイズを変更
		gLineLen = WWIDTH / 2 +2;
	} else {
		gLineLen = LWIDTH / 2 +2;
	}
}

//==============================================================
// 指定物理行に対応する行レコード№を取得する
//--------------------------------------------------------------
// 行レコードは追加されたり削除されたりするためレコード№と物理行番号は一致
// しません。
// 物理行番号（画面に表示された状態での行番号）に対応しているレコード№を取得
// するために先頭行からリストチェインを辿りつつ指定行を探します。
// これは行数に比例して処理時間が掛かるので、安易に使うべきではありません。
// 一万行くらいから処理時間が無視できなくなります。
//--------------------------------------------------------------

long GetRNo(long line)
{
	long	no,rno;

	no = 0;
	rno = 0;
	while (no!=line){											// リストチェインを辿りながら行をカウント
		if (gText[rno]->chainNext==0xFFFFFFFF) break;
		rno = gText[rno]->chainNext;
		no++;
	}
	return (rno);
}

//==============================================================
// 現在の行レコード№から相対的に移動した行レコード№を取得する
//--------------------------------------------------------------
// 現在の行レコード№が分かっている場合に有効です。
// 先頭行より前には移動しません、先頭行で止まります。
// 最終行を超えると0xFFFFFFFFを返します。
//--------------------------------------------------------------

long ShiftRNo(long rno,int shift)
{
	int		i;

	if (shift<0){
		for (i=0; i<(-shift) ;i++){
			rno = gText[rno]->chainBack;
			if (rno==0xFFFFFFFE) break;
		}
		return (rno);
	} else {
		for (i=0; i<shift ;i++){
			rno = gText[rno]->chainNext;
			if (rno==0xFFFFFFFF) break;
		}
		return (rno);
	}
}

//==============================================================
// 指定行レコード№に対応する物理行を取得する
//--------------------------------------------------------------
// これは行数に比例して処理時間が掛かるので、安易に使うべきではありません。
// 一万行くらいから処理時間が無視できなくなります。
//--------------------------------------------------------------

long GetLine(long rno)
{
	long	line,rnoP;

	line = 0;
	rnoP = 0;
	while (rnoP!=rno){											// リストチェインを辿りながら行をカウント
		if (gText[rnoP]->chainNext==0xFFFFFFFF) break;
		rnoP = gText[rnoP]->chainNext;
		line++;
	}
	return (line);
}

//==============================================================
// レコード内容を画面に表示
//--------------------------------------------------------------
// y       表示する行位置
// rno     表示する行のレコード№
// line    表示する行の物理行№
//--------------------------------------------------------------
// 文字単位で背景を消した後、文字を書いてます。
// 行末以降は背景色で消す。
// gXOffset,gXShift,gXDrawの指定に従って表示を行います。
// gXOffsetは画面外左の表示されない範囲、gXShiftは画面内左側で作画を行わない範囲、
// gXDrawはこの位置以降は作画を行わない、という作画設定です。
// 描写範囲から外れて作画された場合（スクロールバーに文字が上書きされる）に備え、
// 描写範囲外イメージを未使用VRAM（ページ3）に待避させてます。
//--------------------------------------------------------------

void DrawRecLine(int y,long rno,long line)
{
	char	c[3] = {0,0,0},str[6];
	int		i,pos,cr,wy;
	float	sx,bx,xsx,xbx,wx;
	long	corStr[5],corBak[2],selPos1=0,selPos2=0,selPos3;

	if (gTSel[0].rno!=-1){
		selPos1 = gTSel[0].line * gLineLen + gTSel[0].cxr;
		selPos2 = gTSel[1].line * gLineLen + gTSel[1].cxr;
		if (selPos1>selPos2){											// selPos1からselPos2までが選択範囲
			selPos3 = selPos2;
			selPos2 = selPos1;
			selPos1 = selPos3;
		}
	}
	cr = 0;
	if (gFHeight>13) cr = 4;											// 16ドットフォント時の特殊記号
	wy = gFHeight;
	if (TBY+(y+1)*gFHeight>272) wy = 272 - (TBY+y*gFHeight);			// 画面下にはみ出るなら
	pos = 0;
	sx = 0;
	bx = 0;
	selPos3 = line * gLineLen;
	ImgMove( LWIDTH,TBY+y*gFHeight,480-LWIDTH,wy, 0,272*2.5 );		// 行末以降イメージを待避
	corBak[0] = gCor[2];												// 通常文字背景
	if (gXShift==0){													// 画面左端に残るゴミの削除
		wx = gFWidth;
		if (wx>gXDraw) wx = gXDraw;
		Fill( 0, TBY+y*gFHeight, wx, wy, corBak[0] );
	}
	while (gText[rno]->len){
		if (gTSel[0].rno==-1 || selPos3<selPos1 || selPos3>=selPos2){
			corStr[0] = gCor[3];										// 通常文字
			corStr[1] = gCor[11];										// 全角空白
			corStr[2] = gCor[10];										// コントロールコード
			corStr[3] = gCor[8];										// 改行文字
			corStr[4] = gCor[7];										// タブ
			corBak[0] = gCor[2];										// 通常文字背景
			corBak[1] = gCor[2];										// 改行背景
		} else {														// テキスト選択時の色指定
			corStr[0] = gCor[2];										// 通常文字
			corStr[1] = gCor[2];										// 全角空白
			corStr[2] = gCor[2];										// コントロールコード
			corStr[3] = gCor[2];										// 改行文字
			corStr[4] = gCor[2];										// タブ
			corBak[0] = gCor[3];										// 通常文字背景
			corBak[1] = gCor[8];										// 改行背景
		}
		c[0] = gText[rno]->text[pos++];
		selPos3++;
		xbx = bx - gXOffset;
		xsx = sx - gXOffset;
		if (c[0]=='\t'){												// タブ発見
			sx = GetTabPos(sx);											// 次のタブ位置
			xsx = sx - gXOffset;
			if (xbx<gXShift && xsx>=gXShift){
				Fill( gXShift, TBY+y*gFHeight, xsx-gXShift, wy, corBak[0] );
			}else {
				if (xbx>=0 && xsx>gXDraw){								// タブが画面からはみ出る場合
					Fill( xbx, TBY+y*gFHeight, gXDraw-xbx, wy, corBak[0] );
				} else if (xbx>=gXShift && xsx>=gXShift){
					Fill( xbx, TBY+y*gFHeight, xsx-xbx, wy, corBak[0] );
				}
			}
			if (gPTab){
				if (xbx>=gXShift || (gXShift>3 && xbx>=gXShift-3)){		// 3はタブマークの幅（左スクロールするときにマークがつぶされるので）
					DrawChar( xbx, TBY+y*gFHeight+2, 0+cr, corStr[4] );
				}
			}
		} else if (c[0]=='\n'){											// 改行発見
			if (xsx<gXDraw && xsx>=gXShift){
				Fill( xsx, TBY+y*gFHeight, 6+1, wy, corBak[1] );		// 文字背景
				DrawChar( xsx, TBY+y*gFHeight+2, 1+cr, corStr[3] );
			}
			sx += 6;
		} else if (c[0]==0x1A){											// EOF発見（何も表示しない）
		} else if ((unsigned char)c[0]<32){								// その他のコントロールコードを発見
			wx = GetStrWidth(gFont," ");
			if (xsx>=gXShift){
				Fill( xsx, TBY+y*gFHeight, wx, wy, corBak[0] );			// 文字背景
				Fill( xsx, TBY+y*gFHeight, wx-1, wy-1, corStr[2] );
			}
			sx += wx;
		} else if (chkSJIS(c[0])){										// 全角文字発見
			c[1] = gText[rno]->text[pos++];
			wx = GetStrWidth(gFont,c);
			if (xsx>=gXShift){
				Fill( xsx, TBY+y*gFHeight, wx+0.5, wy, corBak[0] );		// 文字背景
			}
			selPos3++;
			if (strcmp(c,"　")==0 && gPSpc){							// 全角空白
				if (xsx>=gXShift){
					DrawChar( xsx+0, TBY+y*gFHeight+2, 2+cr, corStr[1] );
					DrawChar( xsx+5, TBY+y*gFHeight+2, 3+cr, corStr[1] );
				}
			} else {													// その他の文字
				if (xsx>=gXShift){
					pf_print( xsx, TBY+y*gFHeight, c, corStr[0] );
				}
			}
			c[1] = '\0';
			sx += wx;
		} else {														// 半角文字発見
			wx = GetStrWidth(gFont,c);
			if (xsx>=gXShift){
				Fill( xsx, TBY+y*gFHeight, wx+0.5, wy, corBak[0] );		// 文字背景
				pf_print( xsx, TBY+y*gFHeight, c, corStr[0] );
			}
			sx += wx;
		}
		bx = sx;
		if (gText[rno]->len<=pos || sx-gXOffset>=gXDraw){				// 表示終了
			break;
		}
	}
	if (gText[rno]->chainNext==0xFFFFFFFF && gText[rno]->len<=pos){		// 最終行なら[EOF]と表示
		strcpy( str,"[EOF]" );
		for (i=0; i<5 ;i++){
			xsx = sx - gXOffset;
			c[0] = str[i];
			wx = GetStrWidth( gFont,c );
			if (xsx>=gXShift && xsx<LWIDTH){
				Fill( xsx, TBY+y*gFHeight, wx+1, wy, gCor[2] );			// 文字背景
				pf_print( xsx, TBY+y*gFHeight, c, gCor[9] );
			}
			sx += wx;
		}
	}

	xsx = sx - gXOffset;
	if (xsx<gXDraw){													// 行末文字以降の余白を作画
		if (xsx>=gXShift){
			Fill( xsx, TBY+y*gFHeight, (gXDraw-(xsx-0.9f)), wy, gCor[2] );
		} else {
			Fill( gXShift, TBY+y*gFHeight, gXDraw-gXShift, wy, gCor[2] );
		}
	} else if (xsx>=LWIDTH){											// 画面からはみ出した場合の修正
		ImgMove( 0,272*2.5,480-LWIDTH,wy, LWIDTH,TBY+y*gFHeight );		// 待避していた行末以降イメージを復元
	}
}

//==============================================================
// 指定位置のレコード内容を画面に表示
//--------------------------------------------------------------

void DrawRec(int y,long rno,long pline)
{
	int		wy;

	if (rno!=0xFFFFFFFF){
		DrawRecLine( y, rno, pline + y );
	} else {
		wy = gFHeight;
		if (TBY+(y+1)*gFHeight>272) wy = 272 - TBY+y*gFHeight;	// 画面下にはみ出るなら
		Fill( 0, TBY+y*gFHeight, LWIDTH, wy, gCor[2] );			// 最終行以降
	}
}

//==============================================================
// 画面全体のレコード内容を表示
//--------------------------------------------------------------

void DrawFullRec2(long rno,long pline)
{
	int		i,l;

	l = 0;
	for (i=0; i<gSLine ;i++){
		l++;
		DrawRecLine( i, rno, pline++ );
		rno = gText[rno]->chainNext;
		if (rno==0xFFFFFFFF) break;
	}
	if (l<gSLine || !gReDraw){									// 画面下部を消去してゴミを削除
		Fill( 0, TBY+l*gFHeight, LWIDTH, 272-(TBY+l*gFHeight), gCor[2] );
	}
}

void DrawFullRec(long pline)
{
	DrawFullRec2(GetRNo(pline),pline);
}

//==============================================================
// スクロールダウン
//--------------------------------------------------------------
// 指定行から下を一行分スクロールダウン。
// スクロールの結果開いた場所の更新はここでは行わない。
// GUに処理させてます。
// 一発で転送するには下側から転送させないといけないのだが、そんな指定はできない
// っぽいので、一度バッファに転送してから目的位置に描き戻しています。
//--------------------------------------------------------------

void RollDown(int y)
{
	int		wy;

	if (y==gSLine-1) return;

	wy = (gSLine-1-y) * gFHeight;
	if (TBY+(y+1)*gFHeight + wy>=272) wy = 272 - (TBY+(y+1)*gFHeight);	// 画面下からはみ出す場合
	sceGuStart( GU_DIRECT, gList );
	sceGuCopyImage( GU_PSM_8888, 0, TBY+y*gFHeight, LWIDTH, wy, 512, VRAM, 0,            272*2.5, 512, VRAM );
	sceGuCopyImage( GU_PSM_8888, 0,        272*2.5, LWIDTH, wy, 512, VRAM, 0, TBY+(y+1)*gFHeight, 512, VRAM );
	sceGuFinish();
	sceGuSync(0,0);
}

//==============================================================
// スクロールアップ
//--------------------------------------------------------------
// 指定行から下をスクロールアップ。
// スクロールの結果開いた場所の更新はここでは行わない。
// GUに処理させてます。
// こちらはそのまま素直に処理してます。
//--------------------------------------------------------------

void RollUp(int y)
{
	int		wy,yy;

	wy = (gSLine-1-y)*gFHeight;
	yy = 0;
	if (TBY+y*gFHeight+gFHeight + wy >= 272){					// 画面下からはみ出す場合
		yy = TBY+y*gFHeight+gFHeight + wy - 272;				// 画面外にはみ出すサイズ
		wy = 272 - (TBY+y*gFHeight+gFHeight);					// 画面内のサイズ
	}
	ImgMove(0, TBY+y*gFHeight+gFHeight, LWIDTH, wy, 0, TBY+y*gFHeight);
	if (yy){													// はみ出した分だけ修正が必要
		Fill( 0, TBY+y*gFHeight+gFHeight+wy, LWIDTH, yy, gCor[2] );
	}
}

//==============================================================
// スクロールライト
//--------------------------------------------------------------
// 画面を右にスクロールさせる。
// スクロールの結果開いた場所の更新はここでは行わない。
// GUに処理させてます。
// 一発では転送できないので、一度バッファに転送してから目的位置に描き戻しています。
// 一度CPUで処理させてみたけど、やっぱりGUの方が早い。
// CPU一画面作画よりGU一画面作画*2の方が早いのね。
//--------------------------------------------------------------

void RollRight(int wx)
{
	sceGuStart( GU_DIRECT, gList );
	sceGuCopyImage( GU_PSM_8888, 0, TBY, LWIDTH-wx, 272-TBY, 512, VRAM, 0, 272*2.5, 512, VRAM );
	sceGuCopyImage( GU_PSM_8888, 0, 272*2.5, LWIDTH-wx, 272-TBY, 512, VRAM, wx, TBY, 512, VRAM );
	sceGuFinish();
	sceGuSync(0,0);
}

//==============================================================
// スクロールレフト
//--------------------------------------------------------------
// 画面を左にスクロールさせる。
// スクロールの結果開いた場所の更新はここでは行わない。
// GUに処理させてます。
//--------------------------------------------------------------

void RollLeft(int wx)
{
	ImgMove( wx, TBY, LWIDTH-wx, 272-TBY, 0, TBY );
}

//==============================================================
// 指定物理位置を越えないでもっとも近いカーソル位置を検索
//--------------------------------------------------------------
// cx   物理カーソル位置
// sx   ドット単位でのカーソル位置
// cxr  論理カーソル位置
// cxb  目標カーソル位置
// rno  行レコード№
// flag 行末の扱い（0:行末境界検出 以外:表示上終端位置まで移動）
//--------------------------------------------------------------
// cxbに最も近い位置にcx,sx,cxrをセットする。
// シフトJISでは全角を正しく処理するためには行頭から一つ一つ全角半角をチェック
// していくしか方法がありません。
// flagは画面表示に伴うカーソル移動用（1）か、ClipRec()による行操作用（0）か
// の識別用です。
//--------------------------------------------------------------

void CMSetSub(int *cx,float *sx,int *cxr,float cxb,char *text,int len,long rno,int flag)
{
	char	c[3] = {0,0,0};
	int		stepp,stepr;
	float	stepw;

	*cx = 0;
	*sx = 0;
	*cxr = 0;
	if (len==0) return;

	while (1){
		if (chkSJIS(text[*cxr])){
			stepr = 2;
			stepp = 2;
			c[0] = text[(*cxr)+0];
			c[1] = text[(*cxr)+1];
			stepw = GetStrWidth( gFont,c );
		} else if (text[*cxr]=='\t'){
			stepr = 1;
			if (gTab){
				stepp = (((*cx) + 8) & 0xFF8) - (*cx);			// TAB=8
			} else {
				stepp = (((*cx) + 4) & 0xFFC) - (*cx);			// TAB=4
			}
			stepw = GetTabPos(*sx) - (*sx);
		} else if (flag && text[*cxr]=='\n'){
			break;
		} else {
			stepr = 1;
			stepp = 1;
			c[0] = text[(*cxr)+0];
			c[1] = '\0';
			stepw = GetStrWidth( gFont,c );
		}
		if (flag || text[*cxr]!='\t'){
			if ((*sx)+stepw>=cxb) break;						// 指定ポイントに達した
		} else {												// タブが行末境界に来る場合の例外処理
			if ((*sx)+2>=cxb) break;							// タブの最小幅は2ドット
		}
		(*cx) += stepp;
		(*sx) += stepw;
		(*cxr) += stepr;
		if (*cxr>=len){											// 行最後尾に達した
			if (flag && gText[rno]->chainNext!=0xFFFFFFFF){
				(*cx) -= stepp;
				(*sx) -= stepw;
				(*cxr) -= stepr;
			}
			break;
		}
	}
}

void CMSet(int *cx,float *sx,int *cxr,float cxb,long rno,int flag)
{
	CMSetSub( cx, sx, cxr, cxb, gText[rno]->text, gText[rno]->len, rno, flag );
}

//==============================================================
// カーソルを論理位置に移動
//--------------------------------------------------------------
// cs.cxr 指定論理位置
//--------------------------------------------------------------
// cxrで指定された論理位置に合うようにcx,sx,cxbを設定する。
// cxbはsxと同じ値になります。
// 行から外れた位置が指定されている場合は行末位置に設定されます。
// この場合、cxrも行末位置へと変更されます。
//--------------------------------------------------------------

void CRSet(struct strCsrSts *cs)
{
	char	c[3] = {0,0,0};
	int		pos,stepp,stepr;
	float	stepw;

	(cs->cx) = 0;
	(cs->sx) = 0;
	pos = 0;
	if (gText[cs->rno]->len==0){
		(cs->cxr) = 0;
		(cs->cxb) = 0;
		return;
	}

	while (pos<(cs->cxr)){
		if (chkSJIS(gText[cs->rno]->text[pos])){
			stepr = 2;
			stepp = 2;
			c[0] = gText[cs->rno]->text[pos+0];
			c[1] = gText[cs->rno]->text[pos+1];
			stepw = GetStrWidth( gFont,c );
		} else if (gText[cs->rno]->text[pos]=='\t'){
			stepr = 1;
			if (gTab){
				stepp = (((cs->cx) + 8) & 0xFF8) - (cs->cx);	// TAB=8
			} else {
				stepp = (((cs->cx) + 4) & 0xFFC) - (cs->cx);	// TAB=4
			}
			stepw = GetTabPos(cs->sx) - (cs->sx);
		} else if (gText[cs->rno]->text[pos]=='\n'){
			break;
		} else {
			stepr = 1;
			stepp = 1;
			c[0] = gText[cs->rno]->text[pos+0];
			c[1] = '\0';
			stepw = GetStrWidth( gFont,c );
		}
		(cs->cx) += stepp;
		(cs->sx) += stepw;
		pos += stepr;
		if (pos>=gText[cs->rno]->len) break;					// 行末に達した
	}

	(cs->cxr) = pos;
	(cs->cxb) = (cs->sx);
}

//==============================================================
// カーソルを上に移動
//--------------------------------------------------------------
// cs   カーソル情報
// flag 画面表示の更新を行うか（0:行わない 以外:変更があった場合は更新する）
//--------------------------------------------------------------
// 前の行で、cxbの物理位置にもっとも近い位置にカーソルを移動させる。
// 必要に応じて画面のスクロールも行う。
//--------------------------------------------------------------

void CMUp(struct strCsrSts *cs,int flag)
{
	long	bufRNo;

	if (gText[cs->rno]->chainBack==0xFFFFFFFE) return;			// 先頭行なら何もしない
	bufRNo = cs->rno;
	cs->rno = gText[cs->rno]->chainBack;
	if (gText[cs->rno]->enter) (cs->rline)--;					// カーソル位置の論理行№
	CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, flag );	// カーソル位置の補正
	gTSel[1].rno = cs->rno;										// テキスト範囲指定用
	gTSel[1].cxr = cs->cxr;
	gTSel[1].line = cs->pline + cs->cy -1;
	if (gTSel[0].rno!=-1 && flag){								// テキスト範囲指定中なら
		DrawRec( cs->cy, bufRNo, cs->pline );					// 移動前のカーソル行表示を更新
	}

	(cs->cy)--;
	if (cs->cy<0){												// カーソルが画面からはみ出るならスクロール
		cs->cy = 0;
		(cs->pline)--;
		if (flag){
			RollDown(0);
			DrawRec( cs->cy, cs->rno, cs->pline );
		}
	} else if (gTSel[0].rno!=-1 && flag){						// テキスト範囲指定中なら
		DrawRec( cs->cy, cs->rno, cs->pline );					// 移動後のカーソル行表示を更新
	}
}

//==============================================================
// カーソルを下に移動
//--------------------------------------------------------------
// cs   カーソル情報
// flag 画面表示の更新を行うか（0:行わない 以外:変更があった場合は更新する）
//--------------------------------------------------------------
// 次の行で、cxbの物理位置にもっとも近い位置にカーソルを移動させる。
// 必要に応じて画面のスクロールも行う。
//--------------------------------------------------------------

void CMDown(struct strCsrSts *cs,int flag)
{
	long	bufRNo;

	if (gText[cs->rno]->chainNext==0xFFFFFFFF){							// 最終行なら
		CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, 0 );	// カーソル位置の補正
		if (gTSel[0].rno!=-1){											// テキスト範囲指定中なら
			gTSel[1].rno = cs->rno;										// テキスト範囲指定用
			gTSel[1].cxr = cs->cxr;
			gTSel[1].line = cs->pline + cs->cy;
			if (flag)
				DrawRec( cs->cy, cs->rno, cs->pline );					// 最終行の表示を更新
		}
		return;
	}

	bufRNo = cs->rno;
	if (gText[cs->rno]->enter) (cs->rline)++;							// カーソル位置の論理行№
	cs->rno = gText[cs->rno]->chainNext;								// 次の行に対応しているレコード№
	if (gText[cs->rno]->chainNext==0xFFFFFFFF){							// 最終行なら
		CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, 0 );	// カーソル位置の補正（最終行だけ最終文字を越えられる）
	} else {
		CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, 1 );	// カーソル位置の補正
	}
	gTSel[1].rno = cs->rno;												// テキスト範囲指定用
	gTSel[1].cxr = cs->cxr;
	gTSel[1].line = cs->pline + cs->cy +1;
	if (gTSel[0].rno!=-1 && flag){										// テキスト範囲指定中なら
		DrawRec( cs->cy, bufRNo, cs->pline );							// 移動前のカーソル行表示を更新
	}

	(cs->cy)++;
	if (cs->cy>=gSLine-gReDraw){										// カーソルが画面からはみ出るならスクロール
		cs->cy = gSLine-1-gReDraw;
		(cs->pline)++;
		if (flag){
			RollUp(0);
			if (gReDraw){
				DrawRec( gSLine-2, ShiftRNo(cs->rno,gSLine-2-(cs->cy)), cs->pline );
			}
			DrawRec( gSLine-1, ShiftRNo(cs->rno,gSLine-1-(cs->cy)), cs->pline );
		}
	}else if (gTSel[0].rno!=-1 && flag){								// テキスト範囲指定中なら
		DrawRec( cs->cy, cs->rno, cs->pline );							// 移動後のカーソル行表示を更新
	}
}

//==============================================================
// カーソルを右に移動
//--------------------------------------------------------------
// cs   カーソル情報
// flag 画面表示の更新を行うか（0:行わない 以外:変更があった場合は更新する）
//--------------------------------------------------------------

void CMRight(struct strCsrSts *cs,int flag)
{
	char	c[3] = {0,0,0};

	if (chkSJIS(gText[cs->rno]->text[cs->cxr])){
		c[0] = gText[cs->rno]->text[(cs->cxr)+0];
		c[1] = gText[cs->rno]->text[(cs->cxr)+1];
		(cs->cxr) += 2;
		(cs->cx) += 2;
		(cs->sx) += GetStrWidth( gFont,c );
	} else if (gText[cs->rno]->text[cs->cxr]=='\t'){
		(cs->cxr)++;
		if (gTab){
			(cs->cx) = ((cs->cx) + 8) & 0xFF8;					// TAB=8
		} else {
			(cs->cx) = ((cs->cx) + 4) & 0xFFC;					// TAB=4
		}
		(cs->sx) = GetTabPos(cs->sx);
	} else if (gText[cs->rno]->text[cs->cxr]=='\n'){
		(cs->cxb) = 0;
		(cs->sx) = 0;
		CMDown( cs,flag );
	} else {
		c[0] = gText[cs->rno]->text[(cs->cxr)+0];
		c[1] = '\0';
		(cs->cxr)++;
		(cs->cx)++;
		(cs->sx) += GetStrWidth( gFont,c );
	}
	if ((cs->cxr)>=gText[cs->rno]->len){						// 行の最後を越えた
		if (gText[cs->rno]->chainNext!=0xFFFFFFFF){
			(cs->cxb) = 0;
			(cs->sx) = 0;
		} else {												// 最終行だけ最終文字を越えられる
			(cs->cxb) = (gRoll ? WWIDTH : LWIDTH);
		}
		CMDown( cs,flag );
	}
	(cs->cxb) = cs->sx;

	gTSel[1].rno = cs->rno;										// テキスト範囲指定用
	gTSel[1].cxr = cs->cxr;
	gTSel[1].line = cs->pline + cs->cy;
	if (gTSel[0].rno!=-1 && flag){								// テキスト範囲指定中なら
		DrawRec( cs->cy, cs->rno, cs->pline );					// 移動前のカーソル行表示を更新
	}
}

//==============================================================
// カーソルを左に移動
//--------------------------------------------------------------
// cs   カーソル情報
// flag 画面表示の更新を行うか（0:行わない 以外:変更があった場合は更新する）
//--------------------------------------------------------------

void CMLeft(struct strCsrSts *cs,int flag)
{
	if (cs->cx==0){
		if (gText[cs->rno]->chainBack!=0xFFFFFFFE){					// 二行目以降の１文字目（一行目では何もしない）
			cs->cxb = (gRoll ? WWIDTH : LWIDTH);					// 行末を目標に
			CMUp( cs, flag );
		}
	} else {
		cs->cxb = cs->sx;
		CMSet( &cs->cx, &cs->sx, &cs->cxr, cs->cxb, cs->rno, 1 );	// カーソル位置の補正
		gTSel[1].rno = cs->rno;										// テキスト範囲指定用
		gTSel[1].cxr = cs->cxr;
		gTSel[1].line = cs->pline + cs->cy;
		if (flag && gTSel[0].rno!=-1){								// テキスト範囲指定中なら
			DrawRec( cs->cy, cs->rno, cs->pline );					// 移動前のカーソル行表示を更新
		}
	}
	cs->cxb = cs->sx;
}

//==============================================================
// 行レコード長の補正
//--------------------------------------------------------------
// cy     補正を開始する行位置（画面上での位置）
// pline  画面最上位位置に対応する物理行№
// rno    補正を開始するレコード№
// flag   補正に伴い画面表示を行うか（0:表示しない(コピペ用) 以外:表示する）
// 戻り値  0:正常終了
//        -1:行レコードの取得ができなかった
//--------------------------------------------------------------
// 行レコードに文字を追加/削除したり行長が変化した場合などに行レコードの長さの補正を行う。
// 画面の更新も同時に行います。
// 現在の行レコードと次の行レコードを繋いだ上で改めて画面上で１行になるように
// 行レコードを分割する。
// 以下、同じ処理を行が続いている限り繰り返す。
// 新しい行レコードが必要になったら追加し、不要となった行レコードは削除する。
// 新しい行レコードが取得出来なかった場合、新しい行に繰り越される予定だった部分は失われます。
//--------------------------------------------------------------

int ClipRec(int cy,long pline,long rno,int flag)
{
	char	tmp[(WWIDTH/2+2)*2+8];									// 二行分+タブ幅
	int		err,cx,cxr,len,cyBak;
	long	i,rno2,lCnt,rnoBak;
	float	sx;

	//----- 行操作 -----

	cyBak = cy;
	rnoBak = rno;
	lCnt = 0;
	err = 0;
	rno2 = 0xFFFFFFFF;
	len = 0;
	while (1){
		if (len==0){
			len = gText[rno]->len;
			memcpy( tmp, gText[rno]->text, len );
			if (gText[rno]->next){								// 次の行に続いている場合
				rno2 = gText[rno]->chainNext;
				len += gText[rno2]->len;
				memcpy( &tmp[gText[rno]->len], gText[rno2]->text, gText[rno2]->len );
				gText[rno2]->len = 0;
			}
		}
		CMSetSub( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), tmp, len, rno, 0 );	// 画面上で１行となる位置を調べる
		if (AddRecText( rno,cxr )){								// 本文領域の拡張に失敗した？
			gText[rno]->text[gText[rno]->len] = '\n';
			gText[rno]->next = 0;								// 次の行へ続かない
			gText[rno]->enter = 1;								// 改行あり
			err = -1;
			break;												// 異常終了（行レコードが取得できない）
		}
		memcpy( gText[rno]->text, tmp, cxr );
		gText[rno]->text[cxr] = '\0';							// [EOF]が前の行に移動する際にゴミが残る？のかな？？
		gText[rno]->len = cxr;									// 一行目のレコード
		len -= cxr;
		if (len){												// 次の行に続く文字がある場合
			memcpy( tmp, &tmp[cxr], len );
			if (cy<gSLine && flag)
				DrawRec( cy, rno, pline );						// 表示を更新
			CMSetSub( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), tmp, len, rno, 0 );
			if (len!=cxr){										// あまりが一行以上あるので新しい行を追加
				rno2 = GetNewRec(TEXTBLSIZE);
				if (rno2!=-1){									// 新しい行レコードを取得できたなら
					gText[rno2]->back = 1;
					gText[rno2]->next = 1;
					gText[rno2]->enter = gText[rno]->enter;
					gText[rno2]->chainNext = gText[rno]->chainNext;
					gText[rno2]->chainBack = rno;				// リストチェインの接続
					gText[rno]->next = 1;						// 次の行へ続く
					gText[rno]->enter = 0;						// 改行は存在しない
					gText[rno]->chainNext = rno2;
					if (gText[rno2]->chainNext!=0xFFFFFFFF){
						gText[gText[rno2]->chainNext]->chainBack = rno2;
					}
					lCnt++;
				} else {										// 行レコードが取得できない！！
					CMSet( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), rno, 0 );
					gText[rno]->text[cxr] = '\n';
					gText[rno]->next = 0;						// 次の行へ続かない
					gText[rno]->enter = 1;						// 改行あり
					err = -1;
					break;										// 異常終了（行レコードが取得できない）
				}
			} else {											// あまりが一行以下
				if (gText[rno]->next){							// 次の行があるなら繋げる
					while (1){
						rno2 = gText[rno]->chainNext;
						memcpy( &tmp[len], gText[rno2]->text, gText[rno2]->len );
						len += gText[rno2]->len;
						CMSetSub( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), tmp, len, rno2, 0 );
						if (len!=cxr || !gText[rno2]->next) break;
						gText[rno]->enter = gText[rno2]->enter;	// 繋いでも一行に満たないなら今の行レコードは削除
						gText[rno]->next = gText[rno2]->next;
						gText[rno]->chainNext = gText[rno2]->chainNext;
						if (gText[rno]->chainNext!=0xFFFFFFFF)
							gText[gText[rno]->chainNext]->chainBack = rno;
						DelRec(rno2);							// 不要になった行レコードを削除
						lCnt--;
					}
				} else {										// 次の行が無いなら新しく作成する
					rno2 = GetNewRec(TEXTBLSIZE);
					if (rno2!=-1){								// 新しい行レコードを取得できたなら
						gText[rno2]->back = 1;
						gText[rno2]->next = 0;
						gText[rno2]->enter = gText[rno]->enter;
						gText[rno2]->chainNext = gText[rno]->chainNext;
						gText[rno2]->chainBack = rno;			// リストチェインの接続
						gText[rno]->next = 1;					// 次の行へ続く
						gText[rno]->enter = 0;					// 改行は存在しない
						gText[rno]->chainNext = rno2;
						if (gText[rno2]->chainNext!=0xFFFFFFFF){
							gText[gText[rno2]->chainNext]->chainBack = rno2;
						}
						lCnt++;
					} else {									// 行レコードが取得できない！！
						CMSet( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), rno, 0 );
						gText[rno]->text[cxr] = '\n';
						gText[rno]->next = 0;					// 次の行へ続かない
						gText[rno]->enter = 1;					// 改行あり
						err = -1;
						break;									// 異常終了（行レコードが取得できない）
					}
				}
			}
			rno = rno2;
			cy++;

		} else {												// 次の行に続く文字が無い場合
			if (gText[rno]->next){								// 処理前は次の行があったので、次の行は削除
				gText[rno]->enter = gText[rno2]->enter;
				gText[rno]->next = gText[rno2]->next;
				gText[rno]->chainNext = gText[rno2]->chainNext;
				if (gText[rno]->chainNext!=0xFFFFFFFF)
					gText[gText[rno]->chainNext]->chainBack = rno;
				DelRec(rno2);									// 不要になった行レコードを削除
				lCnt--;
			}
			break;												// 処理終了
		}
	}

	//----- 画面表示を更新 -----

	cy = cyBak;
	rno = rnoBak;
	if (flag){
		if (lCnt<0){
			lCnt = -lCnt;
			if (cy+1<gSLine-1){
				for ( i=lCnt; i>0 ;i-- ){
					RollUp( cy+1 );
				}
				for ( i=lCnt; i>0 ;i--){
					DrawRec( gSLine-i, ShiftRNo(rno,gSLine-i-cy), pline );
				}
				if (gReDraw || cy==gSLine-1){
					DrawRec( gSLine-2, ShiftRNo(rno,gSLine-2-cy), pline );
				}
			} else if (cy+1==gSLine-1){
				DrawRec( gSLine-1,ShiftRNo(rno,gSLine-1-cy), pline );
			}
		} else if (lCnt>0){
			if (cy+1<gSLine)
				for ( i=lCnt; i>0 ;i-- )
					RollDown(cy+1);
		}
		while (cy<gSLine){
			DrawRec( cy++, rno, pline );						// 表示を更新
			if (!gText[rno]->next) break;
			rno = gText[rno]->chainNext;
		}
	}

	return (err);
}

//==============================================================
// カーソル位置に改行を挿入
//--------------------------------------------------------------
// cs       カーソル情報
// flag     画面表示の更新を行うか（0:行わない 以外:変更があった場合は更新する）
// undoflag 「やり直し」バッファに記録するか（0:記録しない 以外:記録する）
//--------------------------------------------------------------
// 新しい行レコードが取得出来なかった場合は何もせずに-1を返す。
//--------------------------------------------------------------

int AddRet(struct strCsrSts *cs,int flag,int undoflag)
{
	char	c,buf[(WWIDTH/2+2)*2];
	int		i,pos,len,cyBak;
	long	rno2,rno3;

	//----- 新しい行レコードが取得できる？ -----

	rno2 = GetNewRec(TEXTBLSIZE);
	if (rno2==-1) return (-1);									// 新しい行レコードが取得出来ない場合
	DelRec(rno2);

	//----- 挿入位置修正 -----

	if ((cs->cxr)==0 && gText[cs->rno]->back){					// 前の行の最後尾に追加すべき場合もありうるので
		cs->cxb = (gRoll ? WWIDTH : LWIDTH);
		cyBak = cs->cy;
		CMUp( cs,0 );											// 前の行の行末にカーソルを移動
		if (gText[cs->rno]->enter){								// やっぱり次の行の先頭に追加した方がいいので
			CMRight( cs,0 );
		} else if (cyBak==0){									// 前の行が画面外の場合
			if (flag){
				RollDown(0);
				DrawRec( 0, cs->rno, cs->pline );
			}
		}
	}

	//----- やり直し -----

	if (undoflag){
		gUndo[gUndoPos].pline = cs->pline + cs->cy;					// 「やり直し」データ保存
		gUndo[gUndoPos].cxr = cs->cxr;
		gUndo[gUndoPos].str[0] = '\n';
		gUndo[gUndoPos].str[1] = 0;
		gUndo[gUndoPos].str[2] = 1;									// 文字入力
		gUndoPos++;
		if (gUndoPos>=UNDOSIZE) gUndoPos = 0;
		gUndoSize++;
		if (gUndoSize>=UNDOSIZE) gUndoSize = UNDOSIZE;
	}

	//----- 改行コードの挿入 -----

	pos = 0;
	len = 0;
	for ( i=(cs->cxr); i<gText[cs->rno]->len ;i++ ){			// 次の行に送られる文字列をバッファへ待避
		buf[pos++] = gText[cs->rno]->text[i];
		len++;
	}

	if (!gText[cs->rno]->next){									// 次の行に続いていなければ
		rno2 = GetNewRec(TEXTBLSIZE);
		if (rno2!=-1){											// 新しい行レコードを取得できたなら
			if (AddRecText(rno2,len)){							// 本文領域の拡張に失敗した？
				DelRec(rno2);									// 確保した行レコードを廃棄
				if (undoflag){
					gUndoPos--;									// 「やり直し」データをキャンセル
					if (gUndoPos<0) gUndoPos = UNDOSIZE;
					gUndoSize--;
				}
				return (-1);
			}
			gText[rno2]->back = 0;
			gText[rno2]->next = gText[cs->rno]->next;
			gText[rno2]->enter = gText[cs->rno]->enter;
			gText[rno2]->chainNext = gText[cs->rno]->chainNext;
			gText[rno2]->chainBack = cs->rno;					// リストチェインの接続
			gText[cs->rno]->next = 0;
			gText[cs->rno]->enter = 1;							// 改行あり
			gText[cs->rno]->chainNext = rno2;
			if (gText[rno2]->chainNext!=0xFFFFFFFF){
				gText[gText[rno2]->chainNext]->chainBack = rno2;
			}
			if ((cs->cy)+1<gSLine && flag){						// 新しく作成した行が画面内なら
				RollDown((cs->cy)+1);							// 追加した行の分のスペースを確保（行内容の更新は後で行われる）
			}
		} else {												// 行レコードが取得できない！！
			if (undoflag){
				gUndoPos--;										// 「やり直し」データをキャンセル
				if (gUndoPos<0) gUndoPos = UNDOSIZE;
				gUndoSize--;
			}
			return (-1);										// 何もせずに異常終了
		}
		gText[cs->rno]->len = (cs->cxr)+1;
		gText[cs->rno]->text[cs->cxr] = '\n';
		gText[rno2]->len = len;
		memcpy( gText[rno2]->text, buf, len );
		if (flag){
			DrawRec( (cs->cy)+0, cs->rno, cs->pline );			// 一行目
			if ((cs->cy)+1<gSLine){
				DrawRec( (cs->cy)+1, rno2, cs->pline    );		// 二行目
			}
		}

	} else {													// 行が折り返されている場合
		gText[cs->rno]->len = (cs->cxr)+1;
		gText[cs->rno]->text[cs->cxr] = '\n';
		gText[cs->rno]->next = 0;
		gText[cs->rno]->enter = 1;								// 改行あり
		gText[gText[cs->rno]->chainNext]->back = 0;
		if (flag){
			DrawRec( cs->cy, cs->rno, cs->pline );				// 一行目
		}
		rno2 = gText[cs->rno]->chainNext;
		while (1){												// バッファの文字列を次の行へ挿入
			memcpy( &buf[len], gText[rno2]->text, gText[rno2]->len );
			len += gText[rno2]->len;
			pos = 0;
			while (pos<gLineLen-2 && pos<len){
				if (AddRecText(rno2,pos+2)){					// 本文領域の拡張に失敗した？
					len = pos;									// バッファ内の文字列を廃棄
					break;										// 処理中断
				}
				c = buf[pos];
				if (chkSJIS(c)){
					gText[rno2]->text[pos++] = c;
					c = buf[pos];
				}
				gText[rno2]->text[pos++] = c;
			}
			gText[rno2]->len = pos;
			len -= pos;
			if (!len) break;									// 次の行に繰り越す文字列が無いなら
			memcpy( buf, &buf[pos], len );
			if (gText[rno2]->next){								// 次の行があるなら
				rno2 = gText[rno2]->chainNext;
			} else {											// 次の行が無いので新しく取得する
				rno3 = GetNewRec(TEXTBLSIZE);
				if (rno3!=-1){									// 新しい行レコードを取得できたなら
					gText[rno3]->back = 1;
					gText[rno3]->next = gText[rno2]->next;
					gText[rno3]->enter = gText[rno2]->enter;
					gText[rno3]->chainNext = gText[rno2]->chainNext;
					gText[rno3]->chainBack = rno2;				// リストチェインの接続
					gText[rno2]->next = 1;
					gText[rno2]->enter = 0;						// 改行なし
					gText[rno2]->chainNext = rno3;
					if (gText[rno3]->chainNext!=0xFFFFFFFF){
						gText[gText[rno3]->chainNext]->chainBack = rno3;
					}
					if ((cs->cy)+1<gSLine && flag){				// 新しく作成した行が画面内なら
						RollDown((cs->cy)+1);					// 追加した行の分のスペースを確保（行内容の更新は後で行われる）
					}
					gText[rno3]->len = 0;
					rno2 = rno3;
				} else {										// 行レコードが取得できない！！
					gText[rno2]->text[gText[rno2]->len] = '\n';
					gText[rno2]->len++;
					break;										// バッファ内に文字列が残っているが捨てるしかない
				}
			}
		}
		ClipRec( (cs->cy)+1, cs->pline, gText[cs->rno]->chainNext, flag );
	}

	CMRight( cs,flag );											// カーソルを一文字分動かす
	return (0);
}

//==============================================================
// カーソル位置の文字を削除
//--------------------------------------------------------------
// cs       カーソル情報
// flag     画面表示の更新を行うか（0:行わない 以外:変更があった場合は更新する）
// undoflag 「やり直し」バッファに記録するか（0:記録しない 以外:記録する）
//--------------------------------------------------------------

void DelStr(struct strCsrSts *cs,int flag,int undoflag)
{
	int		i,step;

	if ((cs->cxr)>=gText[cs->rno]->len) return;					// カーソル位置が[EOF]の場合は削除の実行はできない

	if (gText[cs->rno]->text[cs->cxr]=='\n'){					// 改行コードを削除する場合、次の行と統合させる必要があるが
		gText[cs->rno]->enter = 0;								// ここでは行レコードの設定をいじるだけで、本文の統合は
		gText[cs->rno]->next = 1;								// ClipRec()内にて処理します
		gText[gText[cs->rno]->chainNext]->back = 1;
	}

	if (chkSJIS(gText[cs->rno]->text[cs->cxr])){				// 全角半角判定
		step = 2;
	} else {
		step = 1;
	}
	if (undoflag){
		gUndo[gUndoPos].pline = cs->pline + cs->cy;				// 「やり直し」データ保存
		gUndo[gUndoPos].cxr = cs->cxr;
		gUndo[gUndoPos].str[0] = gText[cs->rno]->text[cs->cxr];
		gUndo[gUndoPos].str[1] = (step==1 ? 0 : gText[cs->rno]->text[cs->cxr+1]);
		gUndo[gUndoPos].str[2] = 0;								// 文字削除
		gUndoPos++;
		if (gUndoPos>=UNDOSIZE) gUndoPos = 0;
		gUndoSize++;
		if (gUndoSize>=UNDOSIZE) gUndoSize = UNDOSIZE;
	}
	for (i=cs->cxr; i<gText[cs->rno]->len-1 ;i++)
		gText[cs->rno]->text[i] = gText[cs->rno]->text[i+step];
	gText[cs->rno]->len -= step;
	ClipRec( cs->cy, cs->pline, cs->rno, flag );				// 行レコードの長さの補正
	if (gText[cs->rno]->chainBack!=0xFFFFFFFE || cs->cx!=0){	// 一行目１文字目ではない場合、カーソル位置補正
		CMLeft( cs,flag );
		CMRight( cs,flag );
	}
}

//==============================================================
// カーソル位置に文字列を追加
//--------------------------------------------------------------
// cs       カーソル情報
// str      追加する文字列
// flag     画面表示の更新を行うか（0:行わない 以外:変更があった場合は更新する）
// undoflag 「やり直し」バッファに記録するか（0:記録しない 以外:記録する）
// 戻り値    0:正常
//          -1:メモリが足りない
//--------------------------------------------------------------
// 指定された文字列は１文字ずつ追加しています。
// まとめて処理するよりこっちのほうが処理手順を想像しやすいので。
//--------------------------------------------------------------

int AddStr(struct strCsrSts *cs,char *str,int flag,int undoflag)
{
	char	c[3] = {0,0,0};
	int		p,pos,len,step,err,mv;
	float	wc;

	mv = 0;
	err = 0;
	pos = 0;
	len = strlen(str);
	while (pos<len){
		if (chkSJIS(str[pos])){									// 全角半角判定
			step = 2;
			c[0] = str[pos+0];
			c[1] = str[pos+1];
			wc = GetStrWidth( gFont,c );
			if ((cs->sx)+wc>=(gRoll ? WWIDTH : LWIDTH)) mv = 1;
		} else {
			step = 1;
			if (cs->cxr==0 && gText[cs->rno]->back){			// 前の行の最後尾に追加すべき場合もありうるので
				cs->cxb = (gRoll ? WWIDTH : LWIDTH);
				CMUp( cs,0 );									// 前の行の行末にカーソルを移動
			}
			c[0] = str[pos];
			c[1] = '\0';
			wc = GetStrWidth( gFont,c );
			if ((cs->sx)+wc>=(gRoll ? WWIDTH : LWIDTH)) mv = 1;	// カーソル位置補正
		}
		if (str[pos]=='\n'){
			if (AddRet( cs,flag,undoflag )){					// メモリが足りない
				err = -1;
				break;
			}
			pos++;
		} else {
			if (AddRecText(cs->rno,gText[cs->rno]->len+step)){	// 本文領域の拡張に失敗した？
				err = -1;
				break;
			}
			p = gText[cs->rno]->len;
			if (p!=0){											// 文字を追加するスペースを確保
				do {
					p--;
					gText[cs->rno]->text[p+step] = gText[cs->rno]->text[p];
				} while (p>(cs->cxr));
			}
			memcpy( &gText[cs->rno]->text[cs->cxr], &str[pos], step );	// 文字を追加
			gText[cs->rno]->len += step;
			pos += step;
			if (undoflag){
				gUndo[gUndoPos].pline = cs->pline + cs->cy;		// 「やり直し」データ保存
				gUndo[gUndoPos].cxr = cs->cxr;
				gUndo[gUndoPos].str[0] = c[0];
				gUndo[gUndoPos].str[1] = c[1];
				gUndo[gUndoPos].str[2] = 1;						// 文字入力
				gUndoPos++;
				if (gUndoPos>=UNDOSIZE) gUndoPos = 0;
				gUndoSize++;
				if (gUndoSize>=UNDOSIZE) gUndoSize = UNDOSIZE;
			}
			err = ClipRec( cs->cy, cs->pline, cs->rno, flag );	// 行レコードの長さの補正
			if (err) break;										// 行レコードが取得できないなら処理を中断
			CMRight( cs,flag );									// カーソルを一文字分動かす
			if (mv){
				CMRight( cs,flag );								// カーソル位置補正
			}
		}
		mv = 0;
	}
	return (err);
}

//==============================================================
// 文字カーソルを指定行位置に移動させる
//--------------------------------------------------------------
// cs    カーソル情報
// pline 移動させたい物理行位置
//--------------------------------------------------------------
// 指定行位置が画面中央に来るようにカーソル情報を修正する。
// 修正されるのは、
//     cs->cy
//     cs->rno
//     cs->pline
//     cs->rline
// です。
//--------------------------------------------------------------

void SetCursor(struct strCsrSts *cs,long pline)
{
	long	rno,line,rline;

	if (pline>gLineMax) pline = gLineMax;
	if (gLineMax<gSLine+1+gReDraw){									// 全文が一画面内に収まる場合
		cs->cy = pline;
		pline = 0;
	} else {
		if (pline<gSLine/2-2){
			cs->cy = pline;
			pline = 0;
		} else {
			cs->cy = gSLine / 2 -2;
			pline -= gSLine / 2 -2;
		}
		if (pline>gLineMax-gSLine+1+gReDraw){						// 文章の終わりの一画面内な場合
			cs->cy = pline + cs->cy - (gLineMax-gSLine+1+gReDraw);
			pline = gLineMax - gSLine+1 + gReDraw;
		}
	}
	cs->pline = pline;												// 処理前の論理行位置に画面をセット

	rno = 0;
	rline = 0;
	line = 0;
	while (line!=pline+cs->cy && rno!=0xFFFFFFFF){					// 指定物理行位置を探す
		if (gText[rno]->enter) rline++;
		line++;
		rno = gText[rno]->chainNext;
	};
	cs->rno = rno;
	cs->rline = rline;
}

//==============================================================
// 画面位置を文字カーソルが表示される位置に変更する
//--------------------------------------------------------------
// cs カーソル情報
//--------------------------------------------------------------

void ScreenAdj(struct strCsrSts *cs)
{
	if (cs->sx<gXOffset+20){
		if (gXOffset!=0){
			gXOffset = cs->sx - 60;
			if (gXOffset<0) gXOffset = 0;
		}
	} else if (gXOffset+LWIDTH-20<cs->sx){
		if (gXOffset+LWIDTH<WWIDTH){
			gXOffset = cs->sx - LWIDTH + 60;
			if (gXOffset+LWIDTH>WWIDTH) gXOffset = WWIDTH - LWIDTH;
		}
	}
}

//==============================================================
// メニュー（テキストの保存）
//--------------------------------------------------------------

int MenuSave(char *filepath)
{
	char			msg[128];
	int				ret;

	ret = filesave(filepath);
	if (ret==-1){
		strcpy( msg, "ワークメモリが確保できませんでした。" );
	} else if (ret==-2){
		strcpy( msg, "ファイル操作でエラーが発生しました。\n(" );
		strcat( msg, filepath );
		strcat( msg, ")\n" );
	} else if (ret==1){
		
	}
	if (ret){
		strcat( msg, "\nテキストは保存されていません。" );
		DiaBox1( -1, 120, 0, msg, gSCor[0], gSCor[3], gSCor[4] );	// 中央に表示
		Unpush();												// ボタンが離されるまで待機
		WaitBtn();												// 何か押されるまで待機
		return (-2);											// エラー発生
	}
	return (0);													// 保存した
}

//==============================================================
// メニュー（ファイルの保存）
//--------------------------------------------------------------

void MenuLoad(char *filepath)
{
	char	msg[128];
	int		ret;

	ret = fileload(filepath);
	if (ret==1 || ret==-3){
		switch (ret){
		case 2:
			strcpy( msg,"読み込みがキャンセルされました。" );
			break;
		case 1:
			strcpy( msg,"メモリが足りなくなったので文章の読み込みを中断しました。" );
			break;
		}
		DiaBox1( -1, 120, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
		Unpush();												// ボタンが離されるまで待機
		WaitBtn();												// 何か押されるまで待機
	}
}

//==============================================================
// メニュー（テキストのペースト）
//--------------------------------------------------------------
// 既に取り込まれているテキストをカーソル位置に貼り付けます。
//--------------------------------------------------------------

void TextPaste(struct strCsrSts *cs)
{
	if (gTextClip==NULL) return;

	if (strlen(gTextClip)){										// ペーストする文字があるなら
		if (AddStr( cs, gTextClip, 0, 1 )){
			DiaBox1( -1, 120, 0, "メモリが足りなくなりました。", gSCor[0], gSCor[3], gSCor[4] );	// 中央に表示
			Unpush();												// ボタンが離されるまで待機
			WaitBtn();												// 何か押されるまで待機
		}
		gSave = 1;
	}
}

//==============================================================
// メニュー（テキストのコピー）
//--------------------------------------------------------------
// gTSel[0]～gTSel[1]の範囲のテキストを取り込みます。
// 範囲指定が有効な状態で実行すること。
// 最初の時点では[0]と[1]のどちらが先頭側か分からないので注意。
// 取り込まれたテキストはgTextClipに保管されます。
//--------------------------------------------------------------

char* TextCapture(void)
{
	char	*buf;
	int		cxr,cxrEnd;
	long	rno,rno2,rnoEnd,pos,pos0,pos1,size;

	pos0 = gTSel[0].line * gLineLen + gTSel[0].cxr;
	pos1 = gTSel[1].line * gLineLen + gTSel[1].cxr;
	if (pos0>pos1){												// 処理の開始位置と終了位置
		rno = gTSel[1].rno;
		cxr = gTSel[1].cxr;
		rnoEnd = gTSel[0].rno;
		cxrEnd = gTSel[0].cxr;
	} else {
		rno = gTSel[0].rno;
		cxr = gTSel[0].cxr;
		rnoEnd = gTSel[1].rno;
		cxrEnd = gTSel[1].cxr;
	}

	rno2 = rno;
	size = gText[rno2]->len;
	while (rno2!=rnoEnd && rno2!=0xFFFFFFFF){					// 取り込みに必要なスペースを算出
		rno2 = gText[rno2]->chainNext;
		size += gText[rno2]->len;
	}
	if ((buf = (char*) malloc(size))!=NULL){					// 作業領域を確保できたなら
		pos = 0;
		while (rno!=rnoEnd || cxr!=cxrEnd){
			buf[pos++] = gText[rno]->text[cxr++];
			if (gText[rno]->len<=cxr){
				cxr = 0;
				rno = gText[rno]->chainNext;
				if (rno==0xFFFFFFFF) break;
			}
		}
		buf[pos] = '\0';
	}
	return (buf);
}

void TextCopy(void)
{
	if (gTextClip!=NULL) free(gTextClip);						// 既に取り込まれている文章を削除
	gTextClip = TextCapture();
	gTCChg = 1;													// 変更があった
}

//==============================================================
// メニュー（範囲削除）
//--------------------------------------------------------------
// gTSel[0]～gTSel[1]の範囲のテキストを削除します。
// 範囲指定が有効な状態で実行すること。
// 実行後、カーソル位置が削除開始位置に移動します。
// 画面作画は行われません。
//--------------------------------------------------------------

void TextDel(struct strCsrSts *cs)
{
	int		cxr,cxrEnd;
	long	rno,rnoEnd,pos0,pos1,lp,plineBak;

	pos0 = gTSel[0].line * gLineLen + gTSel[0].cxr;
	pos1 = gTSel[1].line * gLineLen + gTSel[1].cxr;
	if (pos0==pos1) return;
	if (pos0>pos1){												// 処理の開始位置と終了位置
		rno = gTSel[1].rno;
		cxr = gTSel[1].cxr;
		rnoEnd = gTSel[0].rno;
		cxrEnd = gTSel[0].cxr;
	} else {
		rno = gTSel[0].rno;
		cxr = gTSel[0].cxr;
		rnoEnd = gTSel[1].rno;
		cxrEnd = gTSel[1].cxr;
	}

	gSave = 1;
	cs->cxr = cxrEnd;
	cs->rno = rnoEnd;
	plineBak = cs->pline;
	CRSet(cs);													// 範囲指定の最後尾位置にカーソル位置を移動
	while ((cs->rno)!=rno || (cs->cxr)!=cxr){					// 1文字づつ消していく
		CMLeft( cs,0 );											// カーソルを一つバックさせる
		DelStr( cs,0,1 );
	}

	lp = GetLine(rno);
	if (plineBak!=cs->pline){									// カーソル位置が画面範囲から外れた
		cs->cy = 0;
		cs->pline = lp;
	} else {
		cs->cy = lp - (cs->pline);
	}
}

//==============================================================
// メニュー（ファイル内容の貼り付け）
//--------------------------------------------------------------
// これが使えるとなんか便利らしいと聞いたので実装してみた。
// 対象ファイルの文章内改行コードはCRLFとLFに対応。
// 画面作画は行われません。
//--------------------------------------------------------------

int FilePaste(struct strCsrSts *cs)
{
	char	c,*data,path[1024],msg[128];
	int		fd;
	long	i,pos1,pos2,filesize;

	//----- ファイル選択 -----

	if (SelectFile("ファイルから貼\り付け",gPFolder,"",path,0,gIME,gHisFPaste))
		return (-1);											// キャンセルされた

	//----- ファイル読み込み -----

	fd = sceIoOpen( path, PSP_O_RDONLY, 0777 );
	data = NULL;
	if (fd>=0){
		filesize = sceIoLseek( fd, 0, SEEK_END );
		sceIoLseek( fd, 0, SEEK_SET );
		data = (char*) malloc(filesize);
		if (data==NULL){										// メモリを確保できなかった
			strcpy( msg,"ワークメモリが確保できませんでした。" );
		} else {
			sceIoRead( fd, data, filesize );
		}
		sceIoClose(fd);
	} else {
		strcpy( msg,"ファイルを開けませんでした。" );
	}
	if (data==NULL){											// 問題発生
		DiaBox1( -1, 120, 0, msg, gSCor[0], gSCor[3], gSCor[4] );	// 中央に表示
		Unpush();												// ボタンが離されるまで待機
		WaitBtn();												// 何か押されるまで待機
		return (0);
	}

	//----- 改行コード修正 -----

	pos1 = 0;
	pos2 = 0;
	for (i=0; i<filesize ;i++){
		c = data[pos1++];
		if (c!='\r' && c!=0x1A)									// CRとEOFを取り除く
			data[pos2++] = c;
	}
	data[pos2] = '\0';

	//----- 文章に貼り付け -----

	if (strlen(data)){											// 文章があるなら
		if (AddStr( cs, data, 0, 1)){
			DiaBox1( -1, 120, 0, "メモリが足りなくなりました。", gSCor[0], gSCor[3], gSCor[4] );	// 中央に表示
			Unpush();												// ボタンが離されるまで待機
			WaitBtn();												// 何か押されるまで待機
		}
		gSave = 1;
	}

	//----- 後始末 -----

	free(data);
	return (0);
}

//==============================================================
// メニュー（ファイルへ切り出し）
//--------------------------------------------------------------
// gTSel[0]～gTSel[1]の範囲のテキストを指定ファイルへ書き出します。
// 範囲指定が有効な状態で実行すること。
//--------------------------------------------------------------

int TextSave(void)
{
	char	*buf,*buf2,path[1024];
	int		fd;
	long	i,pos,size;

	buf = TextCapture();										// 指定範囲のテキストを取り込む
	if (!buf) return (2);
	if (gCRLF){
		size = 0;
		for (i=0; i<strlen(buf) ;i++){
			size += (buf[i]=='\n' ? 2 : 1);
		}
		buf2 = (char*) malloc(size);
		if (buf2){
			pos = 0;
			for (i=0; i<strlen(buf) ;i++){
				if (buf[i]=='\n'){
					buf2[pos++] = '\r';
					buf2[pos++] = '\n';
				} else {
					buf2[pos++] = buf[i];
				}
			}
			free(buf);
			buf = buf2;
		}
	} else {
		size = strlen(buf);
	}

	if (SelectFile("ファイルへ切り出し",gPFolder,"",path,-1,gIME,gHisFPaste)){
		free(buf);
		return (2);												// キャンセルされた
	}
	fd = sceIoOpen( path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if (fd>=0){
		sceIoWrite( fd, buf, size );
		sceIoClose(fd);
	}
	free(buf);
	return (0);
}

//==============================================================
// メニュー（指定行へジャンプ）
//--------------------------------------------------------------
// 入力された論理行位置へ移動する。
// 画面作画は行われません。
//--------------------------------------------------------------

int TextJump(struct strCsrSts *cs)
{
	char	msg[128],str[128];
	long	rno,rline,pline,maxline;

	rno = 0;
	maxline = 0;
	do{															// 行の総数をカウント
		if (gText[rno]->enter) maxline++;
		rno = gText[rno]->chainNext;
	} while (rno!=0xFFFFFFFF);

	sprintf( msg,"ジャンプしたい行位置を入力してください。(1～%ld)",maxline+1 );
	if (InputName(msg,9,gIME,"",str,gHisLJump)){
		return (-1);											// キャンセル
	}
	if (strlen(str)==0) return (-1);
	rline = atol(str) -1;
	if (rline<0) rline = 0;
	if (rline>maxline) rline = maxline;

	rno = 0;
	pline = 0;
	maxline = 0;
	while (rline!=maxline && rno!=0xFFFFFFFF){					// 指定論理行位置を探す
		if (gText[rno]->enter) maxline++;
		pline++;
		rno = gText[rno]->chainNext;
	};
	SetCursor( cs,pline );										// 指定行位置に文字カーソルを移動

	cs->cx = 0;													// 画面上での物理カーソルX位置（文字単位）
	cs->sx = 0;													// 画面上での物理カーソルX位置（ドット単位）
	cs->cxr = 0;												// 論理カーソルX位置
	cs->cxb = 0;												// カーソルX位置指定時の到達目標ポイント（ドット単位）
	cs->adjust = 1;												// 画面位置をカーソルに合わせる（0:そのまま 1:合わせる）
	return (0);
}

//==============================================================
// メニュー（文字列検索）
//--------------------------------------------------------------
// カーソル位置から指定された文字列を検索し、その先頭位置にカーソルを移動させる。
// 必要に応じて画面作画が行われます。
//--------------------------------------------------------------

//----- 選択文字列の取り込み -----

int GetStrBlk(char *str)
{
	int		cxr,cxrEnd,pos;
	long	rno,rnoEnd,pos0,pos1;

	str[0] = '\0';
	if (gTSel[0].rno!=-1){
		pos0 = gTSel[0].line * gLineLen + gTSel[0].cxr;
		pos1 = gTSel[1].line * gLineLen + gTSel[1].cxr;
		if (pos0>pos1){											// 処理の開始位置と終了位置
			rno = gTSel[1].rno;
			cxr = gTSel[1].cxr;
			rnoEnd = gTSel[0].rno;
			cxrEnd = gTSel[0].cxr;
		} else {
			rno = gTSel[0].rno;
			cxr = gTSel[0].cxr;
			rnoEnd = gTSel[1].rno;
			cxrEnd = gTSel[1].cxr;
		}

		pos = 0;
		while (rno!=rnoEnd || cxr!=cxrEnd){
			if (gText[rno]->text[cxr]<32U) break;				// コントロールコードは対象にならない
			str[pos] = gText[rno]->text[cxr++];
			if (chkSJIS(str[pos++])){
				str[pos++] = gText[rno]->text[cxr++];
			}
			if (pos>=49) break;									// 文字数制限
			if (gText[rno]->len<=cxr){
				cxr = 0;
				rno = gText[rno]->chainNext;
				if (rno==0xFFFFFFFF) break;
			}
		}
		str[pos] = '\0';
	}
	return (strlen(str));
}

//----- 現在のカーソル位置以降で指定された文字列のある位置にカーソルをセット -----

int StrSrchSub(struct strCsrSts *cs,char *str)
{
	int		i,pos,pos1,posx;
	long	rno,rno2,pline,pline2;

	rno = cs->rno;
	pline = cs->pline + cs->cy;
	pos = cs->cxr;
	pos++;
	if (gText[rno]->len<pos){									// 検索開始はカーソル位置の次から
		pos = 0;
		rno = gText[rno]->chainNext;
		pline++;
	}
	posx = -1;
	while (rno!=0xFFFFFFFF){
		for (pos=pos; pos<gText[rno]->len ;pos++){
			if (gText[rno]->text[pos]==str[0]){					// 一文字目が合っている？
				gTSel[1].line = pline;
				gTSel[1].rno = rno;
				gTSel[1].cxr = pos;
				pos1 = pos;
				rno2 = rno;
				pline2 = pline;
				for (i=1; i<strlen(str) ;i++){					// 二文字目以降
					pos1++;
					if (gText[rno2]->len<=pos1){				// 行端を超える処理
						pos1 = 0;
						pline2++;
						rno2 = gText[rno2]->chainNext;
						if (rno2==0xFFFFFFFF) break;			// [EOF]にぶつかったら
					}
					if (gText[rno2]->text[pos1]!=str[i]) break;	// 二文字目以降で一致しないなら
				}
				if (strlen(str)==i){							// 目標発見！
					posx = pos;
					pos1++;
					if (gText[rno2]->len<pos1){					// 行端を超える処理
						if (gText[rno2]->enter) break;			// 改行があるならチェック中止
						pos1 = 0;
						pline2++;
						rno2 = gText[rno2]->chainNext;
					}
					gTSel[0].line = pline2;						// 目標を選択状態へ
					gTSel[0].rno = rno2;
					gTSel[0].cxr = pos1;
					break;
				}
			}
		}
		if (posx!=-1) break;									// 目標発見！
		pos = 0;
		rno = gText[rno]->chainNext;
		pline++;
	}

	//----- 目標位置にカーソルを移動 -----

	if (posx==-1){												// 目標発見ならず
		gTSel[0].rno = -1;
		return (-1);
	}
	SetCursor( cs,pline );										// 指定行位置に文字カーソルを移動
	cs->cxr = posx;
	CRSet(cs);													// 指定された論理文字位置にカーソルを移動
	if (gRoll){													// 画面横サイズ拡張モードなら
		ScreenAdj(cs);											// 画面位置を補正
	}
	cs->adjust = 0;												// 画面位置をカーソルに合わせる（0:そのまま 1:合わせる）
	DrawSts( cs, 1 );											// ルーラー系とスクロールバー更新
	DrawFullRec(cs->pline);										// 画面全域を書き換え
	return (0);
}

//----- ダイアログ作画 -----

void DrawStrSrch(int no)
{
	int		x,y;

	x = 480-4-130;
	y = 272-4-56;
	DialogFrame( x, y, 10+110+10, 6+12+4+13*1-1+4+12+6, "文字列検索", "○:選択 ×:ｷｬﾝｾﾙ", gSCor[1], gSCor[2] );
	Fill( x+10, y+6+12+4, 110, 12, gSCor[5] );
	CurveBox( x+10, y+6+12+4, 110, 12, 0, gSCor[10], gSCor[11] );
	mh_print( x+10+1, y+6+12+4, "次の候補へ", gSCor[0] );
}

//----- メイン -----

int StrSrch(struct strCsrSts *cs)
{
	SceCtrlData		pad;
	char	strBuf[128] = {0};
	char	str[128];
	int		x,y,no,ret,exit;
	DrawImg	*image;

	//----- 選択文字列の取り込み -----

	if (GetStrBlk(str)!=0) strcpy( strBuf,str );				// 文字列が取得出来ているなら採用

	//----- 文字列入力 -----

	if (InputName("検索する文字列を入力してください。",50,gIME,strBuf,str,gHisSarh)){
		return (-1);											// キャンセル
	}
	if (strlen(str)==0) return (-1);
	gTSel[0].rno = -1;											// 選択文字列の解除

	//----- 文字列検索 -----

	if (StrSrchSub( cs,str )){									// 次の候補が無いなら終了
		return (1);
	}
	image = ScreenCopy();
	exit = 0;
	no = 0;
	DrawStrSrch(no);
	while (!gExitRequest && !exit){								// キー入力ループ
		x = cs->sx - gXOffset;
		y = TBY + cs->cy * gFHeight;							// カーソル座標設定
		SIMEcursor( 2, x, y );									// カーソルの大きさと座標変数の指定
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		SIMEDrawCursor(ret);									// SIMEカーソルの作画
		switch (ret){
		case SIME_KEY_CIRCLE:									// ○
			ScreenPaste(image);
			if (StrSrchSub( cs,str )){							// 次の候補が無いなら終了
				exit = -1;
			} else {
				image = ScreenCopy();
				DrawStrSrch(no);
			}
			break;
		case SIME_KEY_CROSS:									// ×
			ScreenPaste(image);
			exit = -1;
			break;
		case SIME_KEY_UP:										// ↑
			no = 0;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawStrSrch(no);
			break;
		case SIME_KEY_DOWN:										// ↓
			no = 1;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawStrSrch(no);
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// 画面更新
	}

	return (0);
}

//==============================================================
// メニュー（文字列置換）
//--------------------------------------------------------------
// カーソル位置から指定された文字列を検索し、個々に置換えるかどうか確認していき
// ます。
// 必要に応じて画面作画が行われます。
//--------------------------------------------------------------

//----- ダイアログ作画（設定画面） -----

void DrawStrChgSet(int sx,int sy,int no,char *str1,char *str2)
{
	char	text1[51],text2[51];

	memcpy( text1, str1, 50 );
	text1[50] = '\0';
	memcpy( text2, str2, 50 );
	text2[50] = '\0';

	if (!no){
		CurveBox( sx+10+6*4+2  , sy+6+12+4   , 300, 12, 0, gSCor[10], gSCor[11] );
		mh_print( sx+10+6*4+2+1, sy+6+12+4   , text1, gSCor[0] );
		Fill(     sx+10+6*4+2  , sy+6+12+4+13, 300, 12, gSCor[5] );
		mh_print( sx+10+6*4+2+1, sy+6+12+4+13, text2, gSCor[0] );
	} else {
		Fill(     sx+10+6*4+2  , sy+6+12+4   , 300, 12, gSCor[5] );
		mh_print( sx+10+6*4+2+1, sy+6+12+4   , text1, gSCor[0] );
		CurveBox( sx+10+6*4+2  , sy+6+12+4+13, 300, 12, 0, gSCor[10], gSCor[11] );
		mh_print( sx+10+6*4+2+1, sy+6+12+4+13, text2, gSCor[0] );
	}
}

//----- ダイアログ作画（置換え確認） -----

void DrawStrChgNext(int no)
{
	int		x,y;

	x = 480-4-130;
	y = 272-4-69;
	DialogFrame( x, y, 10+110+10, 6+12+4+13*2-1+4+12+6, "文字列置換", "○:選択 ×:ｷｬﾝｾﾙ", gSCor[1], gSCor[2] );
	if (no){
		Fill( x+10, y+6+12+4, 110, 12, gSCor[5] );
		CurveBox( x+10, y+6+12+4+13, 110, 12, 0, gSCor[10], gSCor[11] );
	} else {
		CurveBox( x+10, y+6+12+4, 110, 12, 0, gSCor[10], gSCor[11] );
		Fill( x+10, y+6+12+4+13, 110, 12, gSCor[5] );
	}
	mh_print( x+10+1, y+6+12+4   , "置換えて次の候補へ", gSCor[0] );
	mh_print( x+10+1, y+6+12+4+13, "そのまま次の候補へ", gSCor[0] );
}

//----- メイン -----

int StrChg(struct strCsrSts *cs)
{
	SceCtrlData		pad;
	char	source[128] = {0},									// 元となる文字列
			dest[128] = {0};									// 置き換える文字列
	char	str[128];											// 一時操作用
	int		x,y,no,sx,sy,pos,ret,exit;
	DrawImg	*image;

	//----- 選択文字列の取り込み -----

	if (GetStrBlk(str)!=0){
		strcpy( source,str );									// 文字列が取得出来ているなら採用
	}

	//----- 設定 -----

	sx = 240-346/2;
	sy = 90;
	DialogFrame( sx, sy, 346, 6+12+4+13*2-1+4+12+6, "文字列置換", "△←→:設定変更 ○:決定 ×:ｷｬﾝｾﾙ", gSCor[1], gSCor[2] );
	mh_print( sx+10, sy+6+12+4   , "検索", gSCor[0] );
	mh_print( sx+10, sy+6+12+4+13, "置換", gSCor[0] );
	no = 0;
	DrawStrChgSet( sx, sy, no, source, dest );

	exit = 0;
	while (!gExitRequest && !exit){								// キー入力ループ
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		switch (ret){
		case SIME_KEY_CIRCLE:									// ○
			exit = 1;
			break;
		case SIME_KEY_CROSS:									// ×
			exit = -1;
			break;
		case SIME_KEY_TRIANGLE:									// △
		case SIME_KEY_LEFT:										// ←
		case SIME_KEY_RIGHT:									// →
			image = ScreenCopy();
			if (no==0){
				if (!InputName("置換える元の文字列を入力してください。",50,gIME,source,str,gHisSarh)){
					strcpy( source,str );
				}
			} else {
				if (!InputName("置換え後の文字列を入力してください。",50,gIME,dest,str,gHisChg)){
					strcpy( dest,str );
				}
			}
			ScreenPaste(image);
			DrawStrChgSet( sx, sy, no, source, dest );
			break;
		case SIME_KEY_UP:										// ↑
			no = 0;
			DrawStrChgSet( sx, sy, no, source, dest );
			break;
		case SIME_KEY_DOWN:										// ↓
			no = 1;
			DrawStrChgSet( sx, sy, no, source, dest );
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();
	}

	if (gExitRequest || exit==-1 || strlen(source)==0){			// キャンセルされた
		return (-1);
	}
	gTSel[0].rno = -1;											// 選択文字列の解除

	//----- 文字列検索と置換え -----

	if (StrSrchSub( cs,source )){								// 次の候補が無いなら終了
		return (1);
	}
	image = ScreenCopy();
	exit = 0;
	no = 0;
	DrawStrChgNext(no);
	while (!gExitRequest && !exit){								// キー入力ループ
		x = cs->sx - gXOffset;
		y = TBY + cs->cy * gFHeight;							// カーソル座標設定
		SIMEcursor( 2, x, y );									// カーソルの大きさと座標変数の指定
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		SIMEDrawCursor(ret);									// SIMEカーソルの作画
		switch (ret){
		case SIME_KEY_CIRCLE:									// ○
			ScreenPaste(image);
			if (no==0){											// 文字列の置換えを行う
				pos = 0;
				while (strlen(source)>pos){						// 検索文字列の後ろまで移動
					CMRight( cs, 0 );
					if (chkSJIS(source[pos++])) pos++;
				}
				pos = 0;
				while (strlen(source)>pos){						// 検索文字列を削除
					CMLeft( cs, 0 );
					DelStr( cs, 0, 1 );
					if (chkSJIS(source[pos++])) pos++;
				}
				AddStr( cs, dest, 0, 1 );						// 置換え文字列を書き込む
				gSave = 1;
			}
			if (StrSrchSub( cs,source )){						// 次の候補が無いなら終了
				DrawRec( cs->cy, cs->rno, cs->pline );			// 最後の置換えを作画に反映させる
				exit = -1;
			} else {
				image = ScreenCopy();
				DrawStrChgNext(no);
			}
			break;
		case SIME_KEY_CROSS:									// ×
			ScreenPaste(image);
			exit = -1;
			break;
		case SIME_KEY_UP:										// ↑
			no = 0;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawStrChgNext(no);
			break;
		case SIME_KEY_DOWN:										// ↓
			no = 1;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawStrChgNext(no);
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// 画面更新
	}

	return (0);
}

//==============================================================
// メニュー（環境設定）
//--------------------------------------------------------------
// 画面表示に関連する項目が変更された場合は、現在の文章を一端テンポラリファイルに
// 保存した後再ロードを行って行レコード構造体の再構築をします。
//--------------------------------------------------------------

//----- カーソル＆設定内容作画 -----

void SetEnvCsr(int sx,int sy,int pos,int *itemdata,int flag)
{
	const char	sel[7][5][13] = {
					{"東雲フォント",	"monafont12",	"monafont16",	"intraFont",	"intraFont P"},
					{"しない",			"する"},
					{"Simple IME",		"OSK"},
					{"４",				"８"},
					{"しない",			"する"},
					{"しない",			"する"},
					{"しない",			"する"},
				};
	static int	pos2;
	int			i,p;

	if (flag){													// 全体を初期化
		for (i=0; i<6 ;i++){
			Fill( sx+10+21*6, sy+6+12+6+i*13, 76, 12, gSCor[5] );
			mh_print( sx+10+21*6+2, sy+6+12+6+i*13, sel[i][itemdata[i]], gSCor[0] );
		}
		for (i=0; i<4 ;i++){
			Fill( sx+10+21*6, sy+6+12+6+6*13+i*13, 136, 12, gSCor[5] );
			p = strlen(gMonaFile[i]) -22;						// 後ろから22文字を表示
			if (p<0) p = 0;
			mh_print( sx+10+21*6+2, sy+6+12+6+6*13+i*13, &gMonaFile[i][p], gSCor[0] );
		}
	} else {													// 前回のカーソル位置を再描写
		if (pos2<6){
			Fill( sx+10+21*6, sy+6+12+6+pos2*13, 76, 12, gSCor[5] );
			mh_print( sx+10+21*6+2, sy+6+12+6+pos2*13, sel[pos2][itemdata[pos2]], gSCor[0] );
		} else {
			Fill( sx+10+21*6, sy+6+12+6+pos2*13, 136, 12, gSCor[5] );
			p = strlen(gMonaFile[pos2-6]) -22;					// 後ろから22文字を表示
			if (p<0) p = 0;
			mh_print( sx+10+21*6+2, sy+6+12+6+pos2*13, &gMonaFile[pos2-6][p], gSCor[0] );
		}
	}

	if (pos<6){
		CurveBox( sx+10+21*6, sy+6+12+6+pos*13, 76, 12, 0, gSCor[10], gSCor[11] );
		mh_print( sx+10+21*6+2, sy+6+12+6+pos*13, sel[pos][itemdata[pos]], gSCor[0] );
	} else {
		CurveBox( sx+10+21*6, sy+6+12+6+pos*13, 136, 12, 0, gSCor[10], gSCor[11] );
		p = strlen(gMonaFile[pos-6]) -22;						// 後ろから22文字を表示
		if (p<0) p = 0;
		mh_print( sx+10+21*6+2, sy+6+12+6+pos*13, &gMonaFile[pos-6][p], gSCor[0] );
	}

	pos2 = pos;
}

//----- フォントファイルの指定変更 -----

int SetEnvFont(int font)
{
	const char	itemname[][21] = {
					"12ドット半角monafont",
					"12ドット全角monafont",
					"16ドット半角monafont",
					"16ドット全角monafont",
				};
	char	*p,msg[64],path[1024],filename[128];
	int		fd,ret;
	DrawImg	*image;

	image = ScreenCopy();										// 画面イメージを待避
	strcpy( msg,itemname[font] );
	strcat( msg,"のファイルを指定" );
	ret = 0;
	strcpy( path,gMonaFile[font] );
	p = strrchr( path,'/' );
	strcpy( filename,&p[1] );
	p[1] = '\0';
	if (!SelectFile(msg,path,filename,path,0,gIME,gHisFont)){
		fd = sceIoOpen( path, PSP_O_RDONLY, 0777 );
		sceIoClose(fd);
		if (fd>=0){
			strcpy( gMonaFile[font],path );
			ret = 1;
		} else {												// 指定されたファイルが開けない
			DiaBox1( -1, 120, 0, "指定されたフォントにアクセスできません。", gSCor[0], gSCor[3], gSCor[4] );
			Unpush();											// ボタンが離されるまで待機
			WaitBtn();											// 何か押されるまで待機
		}
	}
	ScreenPaste(image);											// 画面イメージを復元/バッファ破棄
	return (ret);
}

//----- メイン -----

int SetEnv(struct strCsrSts *cs)
{
	const char	itemname[][21] = {
					"文字フォント",
					"画面横サイズを拡張",
					"IMEの種類",
					"タブ幅",
					"全角空白を記号表\示",
					"タブを記号表\示",
					"12ドット半角monafont",
					"12ドット全角monafont",
					"16ドット半角monafont",
					"16ドット全角monafont",
				};
	SceCtrlData	pad;
	char		msg[128];
	int			i,sx,sy,pos,ret,tabBak,fontBak,rollBak,saveBak,rollChg,fontChg,itemdata[6];
	long		no,rno,line,pline;
	DrawImg		*image;

	sx = 99;
	sy = 53;
	strcpy( msg,VERSION );
	strcat( msg,"   By.STEAR 2009-2010" );
	DialogFrame( sx, sy, 282, 178, msg, "←→:設定変更 ○:決定 ×:ｷｬﾝｾﾙ", gSCor[1], gSCor[2] );
	for (i=0; i<10 ;i++){
		mh_print( sx+10, sy+6+12+6+i*13, itemname[i], gSCor[0] );
	}

	itemdata[0] = gFont;
	itemdata[1] = gRoll;
	itemdata[2] = gIME;
	itemdata[3] = gTab;
	itemdata[4] = gPSpc;
	itemdata[5] = gPTab;
	tabBak = gTab;												// タブ幅を変えたかチェックするため
	fontBak = gFont;											// フォントを変えたかチェックするため
	rollBak = gRoll;
	fontChg = 0;												// フォントファイルを変更した？
	pos = 0;
	SetEnvCsr( sx, sy, pos, itemdata, 1);

	while (!gExitRequest){										// キー入力ループ
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		switch (ret){
		case SIME_KEY_CIRCLE:									// ○
			gFont = pf_fontType(itemdata[0]);					// フォント切り替え
			setScreen();										// パラメータ変更
			if (cs->cy>=gSLine) cs->cy = gSLine - (gReDraw ? 2 : 1);
			SIMEfont(gFont,(gFont==2 ? 16 : 12));				// SIME変換行フォント
			rollChg = 0;
			if (gRoll!=itemdata[1]){
				gRoll = itemdata[1];
				gXOffset = 0;
				rollChg = 1;
			}
			if (gIME!=itemdata[2] || gTab!=itemdata[3] || gPSpc!=itemdata[4] || gPTab!=itemdata[5]){
				gEnvSave = 1;									// 環境設定が変化した
			}
			gIME = itemdata[2];
			gTab = itemdata[3];
			gPSpc = itemdata[4];
			gPTab = itemdata[5];
			if (fontChg){										// フォントファイル変更
				gEnvSave = 1;									// 環境設定が変化した
				ret = 0;
				msg[0] = '\0';
				if ( (ret=pf_setMonaFont(0,gMonaFile[0],gMonaFile[1])) ){
					strcat( msg,"12ドットmonafont" );
				}
				if (pf_setMonaFont(1,gMonaFile[2],gMonaFile[3])){
					if (ret){
						strcat( msg,"と" );
					}
					strcat( msg,"16ドットmonafont" );
				}
				if (strlen(msg)){
					strcat( msg,"のロードに失敗しました。" );
					DiaBox1( -1, 110, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
					Unpush();									// ボタンが離されるまで待機
					WaitBtn();									// 何か押されるまで待機
				}
			}
			if (tabBak!=gTab || fontBak!=gFont || fontChg || rollChg){	// 行レコード構造に変化がある場合
				gEnvSave = 1;									// 環境設定が変化した
				DiaBox1( -1, 100, 0, "文章を再構\成しています。\n少々お待ちください。", gSCor[0], gSCor[1], gSCor[2] );
				ScreenView();									// 画面更新
				pline = 0;
				rno = 0;
				line = 0;
				while (pline<(cs->pline)+(cs->cy)){				// 画面上表示開始論理行を取得
					if (gText[rno]->enter) line++;
					pline++;
					rno = gText[rno]->chainNext;
					if (rno==0xFFFFFFFF) break;
				}
				saveBak = gSave;
				msg[0] = '\0';
				image = ScreenCopy();
				ret = filesave(TEMPFILE);						// 一時作業用ファイルに保存して
				ScreenPaste(image);
				if (ret==0){
					ret = fileload(TEMPFILE);					// 再ロード（この時点で文章の再構成が行われる）
					if (ret==0){								// 正常終了したなら
						sceIoRemove(TEMPFILE);					// 一時ファイルを削除
					}
					switch (ret){
					case 2:
						strcpy( msg,"処理が途中でキャンセルされました。\n文章の後半が失われています。" );
						break;
					case 1:
						strcpy( msg,"メモリ不足のため処理が中断されました。\n文章の後半が失われています。" );
						break;
					case -1:
						strcpy( msg,"メモリ不足のため処理に失敗しました。" );
						break;
					case -2:
						strcpy( msg,"メモリースティックのアクセスでエラーが発生したため\n処理に失敗しました。" );
						break;
					}
					if (ret){
						strcat( msg,"\n処理前の文章が " TEMPFILE " に残っているかもしれません。" );
					}
				} else {										// 作業ファイルへの保存に失敗したので設定を元に戻す
					gTab = tabBak;
					gFont = fontBak;
					gRoll = rollBak;
					setScreen();								// パラメータ変更
					switch (ret){
					case -1:
						strcpy( msg,"メモリ不足のため処理がキャンセルされました。" );
						break;
					case -2:
						strcpy( msg,"メモリースティックのアクセスでエラーが発生したため\n処理がキャンセルされました。" );
						break;
					}
					strcat( msg,"\n各設定を元に戻しました。" );
				}
				gSave = saveBak;
				if (strlen(msg)){								// 問題発生
					DiaBox1( -1, 130, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
					Unpush();									// ボタンが離されるまで待機
					WaitBtn();									// 何か押されるまで待機
				}
				if (ret>=0){									// エラーが発生していない場合
					no = 0;
					pline = 0;
					rno = 0;
					while (no!=line){
						if (gText[rno]->enter) no++;
						pline++;
						rno = gText[rno]->chainNext;
						if (rno==0xFFFFFFFF) break;
					}
					SetCursor( cs,pline );						// 指定行位置に文字カーソルを移動
					CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, 1 );	// カーソル位置を行レコード本文に合うように補正
				}
			}
			return (0);
			break;
		case SIME_KEY_CROSS:									// ×
			return (-1);
			break;
		case SIME_KEY_LEFT:										// ←
			if (pos<6){
				if (itemdata[pos]>0){
					itemdata[pos]--;
					SetEnvCsr( sx, sy, pos, itemdata, 0);
				}
			} else {
				if (SetEnvFont(pos-6)){							// フォント指定
					fontChg = 1;
					SetEnvCsr( sx, sy, pos, itemdata, 0);
				}
			}
			break;
		case SIME_KEY_RIGHT:									// →
			if (pos<6){
				if ((pos==0 && itemdata[pos]<4) || itemdata[pos]<1){
					itemdata[pos]++;
					SetEnvCsr( sx, sy, pos, itemdata, 0);
				}
			} else {
				if (SetEnvFont(pos-6)){							// フォント指定
					fontChg = 1;
					SetEnvCsr( sx, sy, pos, itemdata, 0);
				}
			}
			break;
		case SIME_KEY_UP:										// ↑
			if (pos>0){
				pos--;
				SetEnvCsr( sx, sy, pos, itemdata, 0);
			}
			break;
		case SIME_KEY_DOWN:										// ↓
			if (pos<9){
				pos++;
				SetEnvCsr( sx, sy, pos, itemdata, 0);
			}
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();
	}
	return (-1);
}

//==============================================================
// メニュー（やり直し）
//--------------------------------------------------------------
// 処理速度は完全に無視した実装になっています。
//--------------------------------------------------------------

//----- ダイアログ作画 -----

void DrawUndo(int no,int flag1,int flag2)
{
	int		x,y;

	x = 480-4-130;
	y = 272-4-69;
	DialogFrame( x, y, 10+110+10, 6+12+4+13*2-1+4+12+6, "やり直し", "○:選択 ×:ｷｬﾝｾﾙ", gSCor[1], gSCor[2] );
	if (no){
		Fill( x+10, y+6+12+4, 110, 12, gSCor[5] );
		CurveBox( x+10, y+6+12+4+13, 110, 12, 0, gSCor[10], gSCor[11] );
	} else {
		CurveBox( x+10, y+6+12+4, 110, 12, 0, gSCor[10], gSCor[11] );
		Fill( x+10, y+6+12+4+13, 110, 12, gSCor[5] );
	}
	mh_print( x+10+1, y+6+12+4   , "やり直し", (flag1 ? gSCor[0] : gSCor[12]) );
	mh_print( x+10+1, y+6+12+4+13, "やり直しのやり直し", (flag2 ? gSCor[0] : gSCor[12]) );
}

//----- やり直し -----

void UndoUndo(struct strCsrSts *cs,int *size,int *pos)
{
	if (*size==0) return;
	(*pos)--;
	if (*pos<0) *pos = UNDOSIZE -1;
	(*size)--;

	SetCursor( cs,gUndo[*pos].pline );							// 指定行位置に文字カーソルを移動
	cs->cxr = gUndo[*pos].cxr;
	CRSet(cs);													// 指定された論理文字位置にカーソルを移動
	if (gUndo[*pos].str[2]==1){									// 文字が入力されていた場合
		DelStr( cs, 0, 0 );
	} else {													// 文字が削除されていた場合
		AddStr( cs, gUndo[*pos].str, 0, 0 );
	}
	ScreenAdj(cs);
	cs->adjust = 0;												// 画面位置をカーソルに合わせる（0:そのまま 1:合わせる）
	DrawSts( cs, 1 );											// ルーラー系とスクロールバー更新
	DrawFullRec(cs->pline);										// 画面全域を書き換え
}

//----- やり直しのやり直し -----

void UndoRedo(struct strCsrSts *cs,int *size,int *pos)
{
	char	str[3] = {0,0,0};

	if (*size==gUndoSize) return;

	SetCursor( cs,gUndo[*pos].pline );							// 指定行位置に文字カーソルを移動
	cs->cxr = gUndo[*pos].cxr;
	CRSet(cs);													// 指定された論理文字位置にカーソルを移動
	if (gUndo[*pos].str[2]==1){									// 文字が入力されていた場合
		str[0] = gUndo[*pos].str[0];
		str[1] = gUndo[*pos].str[1];
		AddStr( cs, str, 0, 0 );
	} else {													// 文字が削除されていた場合
		DelStr( cs, 0, 0 );
	}
	ScreenAdj(cs);
	cs->adjust = 0;												// 画面位置をカーソルに合わせる（0:そのまま 1:合わせる）
	DrawSts( cs, 1 );											// ルーラー系とスクロールバー更新
	DrawFullRec(cs->pline);										// 画面全域を書き換え

	(*pos)++;
	if (*pos>=UNDOSIZE) *pos = 0;
	(*size)++;
}

//----- メイン -----

int Undo(struct strCsrSts *cs)
{
	SceCtrlData		pad;
	int		x,y,no,ret,exit,size,pos;
	DrawImg	*image;

	if (gUndoSize==0) return(-1);								// 実行不可
	size = gUndoSize;
	pos = gUndoPos;
	UndoUndo( cs,&size,&pos );
	no = 0;
	image = ScreenCopy();
	DrawUndo( no, (size!=0), (size!=gUndoSize) );

	exit = 0;
	while (!gExitRequest && !exit){								// キー入力ループ
		x = cs->sx - gXOffset;
		y = TBY + cs->cy * gFHeight;							// カーソル座標設定
		SIMEcursor( 2, x, y );									// カーソルの大きさと座標変数の指定
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		SIMEDrawCursor(ret);									// SIMEカーソルの作画
		switch (ret){
		case SIME_KEY_CIRCLE:									// ○
			if (no==0){
				if (size!=0){
					DrawImgFree(image);
					UndoUndo( cs,&size,&pos );
					gSave = 1;
					image = ScreenCopy();
					DrawUndo( no, (size!=0), (size!=gUndoSize) );
				}
			} else {
				if (size!=gUndoSize){
					DrawImgFree(image);
					UndoRedo( cs,&size,&pos );
					gSave = 1;
					image = ScreenCopy();
					DrawUndo( no, (size!=0), (size!=gUndoSize) );
				}
			}
			break;
		case SIME_KEY_CROSS:									// ×
			ScreenPaste(image);
			gUndoPos = pos;
			gUndoSize = size;
			exit = -1;
			break;
		case SIME_KEY_UP:										// ↑
			no = 0;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawUndo( no, (size!=0), (size!=gUndoSize) );
			break;
		case SIME_KEY_DOWN:										// ↓
			no = 1;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawUndo( no, (size!=0), (size!=gUndoSize) );
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// 画面更新
	}
	return (0);
}

//==============================================================
// メニュー（項目の選択時）
//--------------------------------------------------------------
// 戻り値  3:メニュー終了（画面の復元は行わない）
//         2:メニュー終了後画面更新
//         1:メニュー終了後画面初期化
//         0:メニュー終了（画面はそのまま）
//        -1:メニュー終了後アプリ終了
//        -2:キャンセル（再びメニューへ）
//--------------------------------------------------------------
// メニューの項目を選択した時に実行される具体的な内容。
//--------------------------------------------------------------

int MenuSelect(char *filepath,int tab,int pos,struct strCsrSts *cs)
{
	char	*p,path[1024];
	int		ret;

	//----- 「ファイル」 -----

	switch (tab){
	case 0:
		switch (pos){
		case 0:													// 新規作成
			if (gSave){											// 更新されている場合は破棄確認
				if (!DialogYN( "編集中の文章は破棄されます。", gSCor[3], gSCor[4] ))
					return (-2);
			}
			InitRec();
			p = strrchr( filepath,'/' );
			strcpy( &p[1],"NEW.TXT" );
			return (1);
			break;
		case 1:													// 開く
			if (gSave){											// 更新されている場合は破棄確認
				if (!DialogYN( "編集中の文章は破棄されます。", gSCor[3], gSCor[4] ))
					return (-2);
			}
			p = strrchr( filepath,'/' );
			if (!SelectFile("編集する文章を選んでください",gFolder,&p[1],filepath,0,gIME,gHisFile)){
				DiaBox1( -1, 100, 0, "ファイルを読み込んでいます。\n少々お待ちください。", gSCor[0], gSCor[1], gSCor[2] );
				MenuLoad(filepath);
				return (1);
			}
			return (-2);										// 「開く」をキャンセル
			break;
		case 2:													// 上書き保存
			if (DialogYN( "テキストを保存しますか？", gSCor[1], gSCor[2] )){
				MenuSave(filepath);
				return (0);
			}
			return (-2);										// キャンセル
			break;
		case 3:													// 名前を付けて保存
			strcpy( path,filepath );
			p = strrchr( path,'/' );
			if (!SelectFile("名前を付けて保存",gFolder,&p[1],path,-1,gIME,gHisFile)){
				if (!MenuSave(path)){							// 保存した場合はファイルパス関連を更新
					strcpy( filepath,path );
				}
				return (1);										// 編集対象が変更になるので画面初期化
			}
			return (-2);
			break;
		case 4:													// 保存して終了
			if (MenuSave(filepath)){
				return (-2);									// エラー/キャンセルだと終了しない
			} else {
				gExitRequest = -1;
				return (-1);									// 保存したので終了
			}
			break;
		case 5:													// 終了
//			if (gSave){											// 更新されている場合は破棄確認
//				if (!DialogYN( "編集中の文章は破棄されます。", gSCor[3], gSCor[4] ))
//					return (-2);
//			}
			gExitRequest = -1;
			return (-1);
			break;
		case 6:													// 環境設定
			if (SetEnv(cs)){
				return (-2);									// キャンセル
			} else {
				return (2);
			}
			break;
		}
		break;

	//----- 「編集」 -----

	case 1:
		switch (pos){
		case 0:													// やり直し
			gTSel[0].rno = -1;									// 範囲選択の解除
			if (!Undo(cs)){
				return (3);
			}
			break;
		case 1:													// 貼り付け
			if (gTSel[0].rno!=-1){
				TextDel(cs);
				gTSel[0].rno = -1;								// 範囲選択の解除
			}
			if (gTextClip==NULL) return (-2);
			TextPaste(cs);
			cs->adjust = 1;										// 画面位置をカーソル位置へ合わせる
			return (2);
			break;
		case 2:													// コピー
			if (gTSel[0].rno==-1) return (-2);
			TextCopy();
			gTSel[0].rno = -1;									// 範囲選択の解除
			return (2);
			break;
		case 3:													// 切り取り
			if (gTSel[0].rno==-1) return (-2);
			TextCopy();
			TextDel(cs);
			gTSel[0].rno = -1;									// 範囲選択の解除
			cs->adjust = 1;										// 画面位置をカーソル位置へ合わせる
			return (2);
			break;
		case 4:													// 削除
			if (gTSel[0].rno==-1) return (-2);
			TextDel(cs);
			gTSel[0].rno = -1;									// 範囲選択の解除
			cs->adjust = 1;										// 画面位置をカーソル位置へ合わせる
			return (2);
			break;
		case 5:													// ファイルから貼り付け
			if (gTSel[0].rno!=-1){
				TextDel(cs);
				cs->adjust = 1;									// 画面位置をカーソル位置へ合わせる
				gTSel[0].rno = -1;
			}
			if (FilePaste(cs)){
				return (-2);									// キャンセル
			} else {
				cs->adjust = 1;									// 画面位置をカーソル位置へ合わせる
				return (2);
			}
			break;
		case 6:													// ファイルへ切り出し
			if (gTSel[0].rno==-1) return (-2);
			if (TextSave()){
				return (-2);									// キャンセル
			} else {
				gTSel[0].rno = -1;
				return (2);
			}
			break;
		}
		break;

	//----- 検索 -----

	case 2:
		switch (pos){
		case 0:													// 文字列検索
			ret = StrSrch(cs);
			if (ret<0){
				return (-2);
			} else if (ret==0){
				return (3);
			} else {
				return (2);
			}
			break;
		case 1:													// 文字列置換
			ret = StrChg(cs);
			if (ret<0){
				return (-2);
			} else if (ret==0){
				return (3);
			} else {
				return (2);
			}
			break;
		case 2:													// 指定行へジャンプ
			if (TextJump(cs)){
				return (-2);
			} else {
				gTSel[1].rno = cs->rno;							// テキスト範囲指定用
				gTSel[1].cxr = cs->cxr;
				gTSel[1].line = cs->pline + cs->cy;
				return (2);
			}
			break;
		case 3:													// ファイルの先頭
			SetCursor( cs,0 );									// 指定行位置に文字カーソルを移動
			cs->cx = 0;											// 画面上での物理カーソルX位置（文字単位）
			cs->sx = 0;											// 画面上での物理カーソルX位置（ドット単位）
			cs->cxr = 0;										// 論理カーソルX位置
			cs->cxb = 0;										// カーソルX位置指定時の到達目標ポイント（ドット単位）
			cs->adjust = 1;										// 画面位置をカーソルに合わせる（0:そのまま 1:合わせる）
			gTSel[1].rno = cs->rno;								// テキスト範囲指定用
			gTSel[1].cxr = cs->cxr;
			gTSel[1].line = cs->pline + cs->cy;
			return (2);
			break;
		case 4:													// ファイルの最後
			SetCursor( cs,gLineMax );							// 指定行位置に文字カーソルを移動
			cs->cxr = gText[cs->rno]->len;
			CRSet(cs);											// 指定された論理文字位置にカーソルを移動
			gTSel[1].rno = cs->rno;								// テキスト範囲指定用
			gTSel[1].cxr = cs->cxr;
			gTSel[1].line = cs->pline + cs->cy;
			return (2);
			break;
		case 5:													// 最後に編集した所
			if (gUndoSize!=0){
				pos = gUndoPos -1;
				if (pos<0) pos = UNDOSIZE -1;
				SetCursor( cs,gUndo[pos].pline );				// 指定行位置に文字カーソルを移動
				cs->cxr = gUndo[pos].cxr;
				CRSet(cs);										// 指定された論理文字位置にカーソルを移動
				gTSel[1].rno = cs->rno;							// テキスト範囲指定用
				gTSel[1].cxr = cs->cxr;
				gTSel[1].line = cs->pline + cs->cy;
				return (2);
			}
			break;
		case 6:
			break;
		}
		break;
	}
	return (-2);
}

//==============================================================
// メニュー
//--------------------------------------------------------------
// メニューの作画とキー入力処理。
//--------------------------------------------------------------

//----- 項目作画 -----

void DrawMenuSub(int sx,int sy,int tab,int pos,int sel)
{
	const char	item[3][7][17] = {{
					"新規作成",
					"開く",
					"上書き保存",
					"名前を付けて保存",
					"保存して終了",
					"終了",
					"環境設定",
				},{
					"やり直し",
					"貼\り付け",
					"コピー",
					"切り取り",
					"削除",
					"ﾌｧｲﾙから貼\り付け",
					"ﾌｧｲﾙへ切り出し",
				},{
					"文字列検索",
					"文字列置換",
					"指定行へジャンプ",
					"ファイルの先頭",
					"ファイルの最後",
					"最後に編集した所",
					"",
				}};
	long		cor;

	cor = gSCor[0];
	if (tab==1){												// グレーアウト
		if (!(sel & 0x01) && pos==1) cor = gSCor[12];
		if (!(sel & 0x02) && ((pos>1 && pos<5) || pos == 6)) cor = gSCor[12];
		if (!(sel & 0x04) && pos==0) cor = gSCor[12];
	} else if (tab==2){
		if (!(sel & 0x04) && pos==5) cor = gSCor[12];
	}
	mh_print( sx+10+1+tab*100, sy+4+12+4+12+4+pos*13, item[tab][pos], cor );
}

//----- タブ作画 -----

void DrawMenuTab(int sx,int sy,int tab)
{
	Fill( sx+10,     sy+4+12+4, 52, 12, gSCor[5] );
	Fill( sx+10+100, sy+4+12+4, 28, 12, gSCor[5] );
	Fill( sx+10+200, sy+4+12+4, 28, 12, gSCor[5] );
	switch (tab){
	case 0:
		CurveBox( sx+10,     sy+4+12+4, 52, 12, 0, gSCor[10], gSCor[11] );
		break;
	case 1:
		CurveBox( sx+10+100, sy+4+12+4, 28, 12, 0, gSCor[10], gSCor[11] );
		break;
	case 2:
		CurveBox( sx+10+200, sy+4+12+4, 28, 12, 0, gSCor[10], gSCor[11] );
		break;
	}
	mh_print( sx+10+1+1,     sy+4+12+4, "ファイル", gSCor[1] );
	mh_print( sx+10+1+100+1, sy+4+12+4, "編集", gSCor[1] );
	mh_print( sx+10+1+200+1, sy+4+12+4, "検索", gSCor[1] );
}

//----- カーソル作画 -----

void DrawMenu(int sx,int sy,int tab,int pos,int flag,int sel)
{
	static int	pos2,tab2;
	int			i,j;

	if (flag){													// メニュー項目を表示して前回のカーソルを消す
		for (j=0; j<3 ;j++){
			for (i=0; i<7 ;i++){
				Fill( sx+10+j*100, sy+4+12+4+12+4+i*13, 98, 12, gSCor[5] );
				DrawMenuSub( sx, sy, j, i, sel );
			}
		}
	} else if (pos!=pos2 || tab!=tab2){
		Fill( sx+10+tab2*100, sy+4+12+4+12+4+pos2*13, 98, 12, gSCor[5] );
		DrawMenuSub( sx, sy, tab2, pos2, sel );
	}

	CurveBox( sx+10+tab*100, sy+4+12+4+12+4+pos*13, 98, 12, 0, gSCor[10], gSCor[11] );
	DrawMenuSub( sx, sy, tab, pos, sel );

	pos2 = pos;
	tab2 = tab;
}

//----- メイン -----

int menu(char *filepath,int tab,int sel,struct strCsrSts *cs)
{
	SceCtrlData		pad;
	int				sx,sy,pos,ret;
	DrawImg			*image;

	sx = 240-316/2;
	sy = 70;
	DialogFrame( sx, sy, 318, 4+12+4+12+4+7*13+8, "メニュー", "", gSCor[1], gSCor[2] );
	DrawMenuTab( sx, sy, tab );

	pos = 0;
	if (tab==0 && gSave) pos = 2;								// 更新されているなら「上書き保存」
	if (tab==1){
		if (sel&0x02){
			pos = 2;											// 範囲指定中なら「コピー」
		} else {
			pos = 1;											// テキストクリップに何かあるなら「貼り付け」
		}
	}
	DrawMenu( sx, sy, tab, pos, -1, sel );						// メニュー項目の初期作画
	Unpush();													// ボタンが離されるまで待機

	while (!gExitRequest){										// キー入力ループ
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		switch (ret){
		case SIME_KEY_CIRCLE:									// ○
			image = ScreenCopy();								// 画面イメージをメニューを含めて保存
			ret = MenuSelect( filepath,tab,pos,cs );
			if (ret!=-2){
				DrawImgFree(image);								// 画面イメージを廃棄
				if (ret!=1){									// 画面初期化時は実行しない
					DrawSts( cs, 1 );							// ルーラー系とスクロールバー更新
				}
				return (ret);
			} else {
				ScreenPaste(image);								// 画面イメージを復元
				DrawSts( cs, 1 );								// ルーラー系とスクロールバー更新
			}
			break;
		case SIME_KEY_START:									// [START]
		case SIME_KEY_CROSS:									// ×
			return (0);
			break;
		case SIME_KEY_TRIANGLE:									// △
			break;
		case SIME_KEY_LEFT:										// ←
			tab--;
			if (tab<0) tab = 2;
			DrawMenuTab( sx, sy, tab );
			DrawMenu( sx, sy, tab, pos, 0, sel );
			break;
		case SIME_KEY_RIGHT:									// →
			tab++;
			if (tab>2) tab = 0;
			DrawMenuTab( sx, sy, tab );
			DrawMenu( sx, sy, tab, pos, 0, sel );
			break;
		case SIME_KEY_UP:										// ↑
			pos--;
			if (pos<0) pos = 6;
			DrawMenu( sx, sy, tab, pos, 0, sel );
			break;
		case SIME_KEY_DOWN:										// ↓
			pos++;
			if (pos>6) pos = 0;
			DrawMenu( sx, sy, tab, pos, 0, sel );
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// 画面更新
	}
	return (-1);
}

//==============================================================
// main
//--------------------------------------------------------------

//----- 右スクロール -----

void RightShift(struct strCsrSts *cs,int vx)
{
	SIMEDrawCursor(-1);											// SIMEのカーソルとウィンドウ等の消去
	RollRight(vx);
	gXShift = 0;
	gXDraw = vx;												// 画面左側の部分書き換え設定
	DrawFullRec2( ShiftRNo(cs->rno,-cs->cy),cs->pline );
	gXShift = 0;
	gXDraw = LWIDTH;
	DrawSts( cs, 1 );											// ルーラー系とスクロールバー
}

//----- 左スクロール -----

void LeftShift(struct strCsrSts *cs,int vx)
{
	SIMEDrawCursor(-1);											// SIMEのカーソルとウィンドウ等の消去
	RollLeft(vx);
	gXShift = LWIDTH - vx - gFWidth;
	gXDraw = LWIDTH;											// 画面右側の部分書き換え設定
	DrawFullRec2( ShiftRNo(cs->rno,-cs->cy),cs->pline );
	gXShift = 0;
	gXDraw = LWIDTH;
	DrawSts( cs, 1 );											// ルーラー系とスクロールバー
}

//----- 下スクロール -----

void DownShift(struct strCsrSts *cs,int dcFlag)
{
	if (cs->pline!=0){
		cs->pline--;
		cs->rno = ShiftRNo( cs->rno,-1 );
		if (gText[cs->rno]->enter) (cs->rline)--;				// カーソル位置の論理行№
		CMSet( &cs->cx, &cs->sx, &cs->cxr, cs->cxb+1, cs->rno, 1 );	// カーソル位置の補正
		gTSel[1].rno = cs->rno;
		gTSel[1].cxr = cs->cxr;
		gTSel[1].line = cs->pline + cs->cy;
		if (!dcFlag) SIMEDrawCursor(-1);						// カーソルの消去
		RollDown(0);
		if (gTSel[0].rno!=-1){
			DrawRec( cs->cy, cs->rno, cs->pline );
			if (cs->cy<gSLine-1){
				DrawRec( cs->cy+1, gText[cs->rno]->chainNext, cs->pline );
			}
		}
		DrawRec( 0, ShiftRNo(cs->rno,-cs->cy), cs->pline );
		DrawSts( cs, 1 );										// ルーラー系とスクロールバー
	}
}

//----- 上スクロール -----

void UpShift(struct strCsrSts *cs,int dcFlag)
{
	if (cs->pline+gSLine-1-gReDraw<gLineMax){
		cs->pline++;
		if (gText[cs->rno]->enter) (cs->rline)++;				// カーソル位置の論理行№
		cs->rno = ShiftRNo( cs->rno,1 );
		CMSet( &cs->cx, &cs->sx, &cs->cxr, cs->cxb+1, cs->rno, 1 );	// カーソル位置の補正
		gTSel[1].rno = cs->rno;
		gTSel[1].cxr = cs->cxr;
		gTSel[1].line = cs->pline + cs->cy;
		if (!dcFlag) SIMEDrawCursor(-1);						// カーソルの消去
		RollUp( 0 );
		if (gTSel[0].rno!=-1 && cs->cy!=gSLine-1){
			if (cs->cy!=0){
				DrawRec( (cs->cy)-1, gText[cs->rno]->chainBack, cs->pline );
			}
			DrawRec( cs->cy, cs->rno, cs->pline );
		}
		if (gReDraw || cs->cy==gSLine-1){
			DrawRec( gSLine-2, ShiftRNo(cs->rno,gSLine-2-(cs->cy)), cs->pline );
		}
		DrawRec( gSLine-1, ShiftRNo(cs->rno,gSLine-1-(cs->cy)), cs->pline );
		DrawSts( cs, 1 );										// ルーラー系とスクロールバー
	}
}

//----- ワーク初期化 -----
// これはアプリ起動時に実行します

void SMEMOinit(void)
{
	char	msg[128];
	int		x,y,ret;

	initGraphics();
	flipScreen2();												// デフォルトではページ設定に不都合があるので
	sceGuStart( GU_DIRECT, gList );
	sceGuClearColor(0x00000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	if (getcwd( gFolder,1024 )==NULL){							// カレントディレクトリの取得
		strcpy( gFolder,"ms0:");
	}
	strcat( gFolder,"/" );
	strcpy( gPFolder,gFolder );									// ファイルから貼り付け時の対象フォルダ

	//----- 配色調整 -----

	if (kuKernelGetModel()) {									// PSP-1000以外なら
		gSCor[1] = 0x70F070;									// 通知ダイアログ（枠）
		gSCor[2] = 0x183018;									// 通知ダイアログ（内部）
		gSCor[4] = 0x202060;									// 警告ダイアログ（内部）
		gSCor[5] = 0x002000;									// 通知ダイアログ（背景）
	}

	//----- intraFont初期化 -----

	x = 0;
	y = 0;

/*	DiaBox1( x, y, 0, "intraFontロード中", gSCor[0], gSCor[1], gSCor[2] );
	ScreenView();												// 画面更新
	if (pf_setIntraFont()){
		DiaBox1( x+20, y+10, 0, "intraFont 'japanese font' のロードに失敗しました。", gSCor[0], gSCor[3], gSCor[4] );
		WaitBtn();												// 何か押されるまで待機
	}
	x += 15;
	y += 20;
*/
	//----- monafont読み込み -----

	strcpy( gMonaFile[0],gFolder );
	strcat( gMonaFile[0],"FONT/monafontA.bin" );
	strcpy( gMonaFile[1],gFolder );
	strcat( gMonaFile[1],"FONT/monafontJ.bin" );
	strcpy( gMonaFile[2],gFolder );
	strcat( gMonaFile[2],"FONT/monafont16A.bin" );
	strcpy( gMonaFile[3],gFolder );
	strcat( gMonaFile[3],"FONT/monafont16W.bin" );

	LoadEnv();													// 設定ファイル読み込み

	DiaBox1( x, y, 0, "monafontロード中", gSCor[0], gSCor[1], gSCor[2] );
	ScreenView();												// 画面更新
	msg[0] = '\0';
	ret = 0;
	if ( (ret=pf_setMonaFont(0,gMonaFile[0],gMonaFile[1])) ){
		strcat( msg,"12ドットmonafont" );
	}
	if (pf_setMonaFont(1,gMonaFile[2],gMonaFile[3])){
		if (ret){
			strcat( msg,"と" );
		}
		strcat( msg,"16ドットmonafont" );
	}
	if (strlen(msg)){
		strcat( msg,"のロードに失敗しました。" );
		DiaBox1( x+20, y+10, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
		Unpush();												// ボタンが離されるまで待機
		WaitBtn();												// 何か押されるまで待機
	}
	x += 15;
	y += 20;

	//----- SimpleIME初期化 -----

	DiaBox1( x, y, 0, "辞書ファイルロード中", gSCor[0], gSCor[1], gSCor[2] );
	ScreenView();												// 画面更新
	ret = InitSIME(0);											// 辞書読み込みと各種初期化（背景の自己復元モード）
	SIMESetDraw(0);												// DRAWページへ書き込み
	switch(ret){
	case -1:
		strcpy( msg,"不正な辞書ファイルです。" );
		break;
	case -2:
		strcpy( msg,"メモリが足りなくて辞書が読めませんでした。" );
		break;
	case -3:
		strcpy( msg, "辞書（SIME.DIC）が開けませんでした。" );
		break;
	}
	if (ret<0){													// エラーが発生していたら
		strcat( msg,"\n漢字変換機能\が使用出来ません。" );
		DiaBox1( x+20, y+10, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
		Unpush();												// ボタンが離されるまで待機
		WaitBtn();												// 何か押されるまで待機
	}

	Fill(0, 0, 480, 272*2, 0);									// 画面消去
}

//----- ワーク開放 -----
// これはアプリ終了時に実行します

void SMEMOend(void)
{
	EndSIME();													// Simple IME関連ワーク開放
	pf_endMonaFont();											// MonaFont開放
}

//----- メイン -----

//--- 指定テキストを編集

int	SMEMOtext(char *message,int maxsize,int maxline)
{
	return SMEMOmain( 0, message, maxsize, maxline );
}

//--- 指定ファイルの内容を編集
// path 編集を行うファイル名（フルパス）
//      NULLが指定された場合はファイル選択ダイアログを表示

int SMEMOfile(char *path)
{
	return SMEMOmain( 1, path, 0, 0 );
}

//int	main(int argc,char **argv)
int	SMEMOmain(int mode,char *text,int maxsize,int maxline)
{
	SceCtrlData			pad,pad2;
	struct strCsrSts	cs;
	unsigned char		c;
	char				str[256],msg[128],textFile[1024];
	int					i,x,y,fd,tab,cppst,ret,mret,save,sime,off1,off2,dcFlag;
	long				filesize,pos1,pos2;
	float				vy,xoffBuf;
	int					line,pos;
	long				rno;

//	SetupCallbacks();

//	sceCtrlSetSamplingCycle(0);
//	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

	initGraphics();
	flipScreen2();												// デフォルトではページ設定に不都合があるので
	sceGuStart( GU_DIRECT, gList );
	sceGuClearColor(0x00000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	gLineListMax = 0;

//	SMEMOinit();

	//----- 入力履歴の初期化 -----

	gHisLJump = InputHisLoad( HISFILEJUMP );
	gHisSarh = InputHisLoad( HISFILESARH );
	gHisChg = InputHisLoad( HISFILECHG );
	gHisFPaste = InputHisLoad( HISFILEPASTE );
	gHisFont = InputHisLoad( HISFILEFONT );
	gHisFile = InputHisLoad( HISFILEFILE );

	//----- クリップボードファイル読み込み -----

	gTextClip = NULL;
	fd = sceIoOpen( CLIPBOARD, PSP_O_RDONLY, 0777 );
	if (fd>=0){
		filesize = sceIoLseek(fd, 0, SEEK_END);
		sceIoLseek(fd, 0, SEEK_SET);
		gTextClip = (char*) malloc (filesize+1);
		if (gTextClip){
			sceIoRead( fd, gTextClip, filesize );
		}
		sceIoClose(fd);
		pos2 = 0;
		for (pos1=0; pos1<filesize ;pos1++){					// テキスト以外のコードを削除
			c = gTextClip[pos1];
			if ((c>=32 && c!=0x7F) || c=='\n' || c=='\t') gTextClip[pos2++] = c;
		}
		gTextClip[pos2] = '\0';
	}
	gTCChg = 0;

	//----- ファイル選択 -----

	gFont = pf_fontType(gFont);									// デフォルトフォントを指定
	setScreen();												// パラメータ変更
	SIMEfont(gFont,(gFont==2 ? 16 : 12));						// SIME変換行フォント

	InitRec();													// 行レコードリストを初期化

	if (mode==1){												// 指定ファイルの編集
		if (text==NULL){
			if (SelectFile("編集する文章を選んでください",gFolder,"NEW.TXT",textFile,0,gIME,gHisFile)){
				strcpy( textFile,gFolder );
				strcat( textFile,"NEW.TXT" );					// ファイルが選択されなかった時のファイル名
			}
		} else {
			strcpy( textFile,text );
		}
		DiaBox1( -1, 100, 0, "ファイルを読み込んでいます。\n少々お待ちください。", gSCor[0], gSCor[1], gSCor[2] );
		MenuLoad(textFile);
	} else {
		strcpy( textFile,gFolder );
		strcat( textFile,"NEW.TXT" );
	}

	Fill(0, 0, 480, 272, 0);									// 画面消去

	//----- メイン -----

	cs.cx = 0;
	cs.sx = 0;
	cs.cy = 0;													// カーソル位置
	cs.cxb = 0;													// カーソル位置保存用
	cs.cxr = 0;													// カーソルの論理位置
	cs.pline = 0;												// 画面最上部の物理行№
	cs.rline = 0;												// カーソル位置の論理行№
	cs.adjust = 0;												// 画面位置修正
	gXOffset = 0;												// 左右スクロール用
	gXShift = 0;
	gXDraw = LWIDTH;
	vy = 0;														// 縦スクロール時の端数管理
	save = 0;
	tab = 0;													// メニュー種類
	cppst = 0;													// メニューの「編集」グレーアウト指定
	gTSel[0].rno = -1;											// 範囲選択1
	gTSel[1].rno = -1;											// 範囲選択2
	cs.rno = GetRNo(cs.pline+cs.cy);							// カーソル位置に対応するレコード№
	if (mode==0){
		AddStr( &cs, text, 0, 1 );								// ■■■ owata側からの文字列を展開 ■■■
	}
	gUndoSize = 0;												// やり直しバッファサイズ
	gUndoPos = 0;												// やり直しバッファ位置
	DrawTitle(textFile);
	DrawSts( &cs, 1 );											// ルーラー系とスクロールバー
	DrawFullRec(cs.pline);										// 画面全域を書き換え
	Unpush();													// ボタンが全て離されるまで待機

	x = 1;
	y = TBY;
	pad2.Buttons = 0;
	while(!gExitRequest){
		ret = 0;												// -1:新しい行レコードが取得できなかった
		sceCtrlReadBufferPositive(&pad, 1);
		if (cs.adjust && gRoll){								// カーソルが表示される位置に画面を移動させる
			if (cs.sx<gXOffset+20){
				if (gXOffset!=0){
					xoffBuf = gXOffset;
					gXOffset = cs.sx - 60;
					if (gXOffset<0) gXOffset = 0;
					if (xoffBuf-gXOffset<400){
						RightShift( &cs,xoffBuf-gXOffset );
					} else {
						DrawFullRec(cs.pline);					// 画面全域を書き換え
						DrawSts( &cs, 1 );						// ルーラー系とスクロールバー
					}
				}
			} else if (gXOffset+LWIDTH-20<cs.sx){
				if (gXOffset+LWIDTH<WWIDTH){
					xoffBuf = gXOffset;
					gXOffset = cs.sx - LWIDTH + 60;
					if (gXOffset+LWIDTH>WWIDTH) gXOffset = WWIDTH - LWIDTH;
					if (gXOffset-xoffBuf<400){
						LeftShift( &cs,gXOffset-xoffBuf );
					} else {
						DrawFullRec(cs.pline);					// 画面全域を書き換え
						DrawSts( &cs, 1 );						// ルーラー系とスクロールバー
					}
				}
			}
		}
		cs.adjust = 0;
		x = cs.sx - gXOffset;
		y = TBY + cs.cy*gFHeight;								// カーソル座標設定
		if (x<0 || x>=LWIDTH){									// カーソルが画面外ならカーソルは表示しない
			x = (x<0 ? 0 : LWIDTH-2);							// カーソルが画面外の時のSIME変換枠表示位置
			SIMEcursor( 0, x, y);
		} else {
			SIMEcursor( 2, x, y);								// カーソルの大きさと座標変数の指定
		}
		SIMEselInput( gIME, str,pad );							// 文字列入力
		if (str[0]!=0){											// 入力があった
			if ((str[0]<32U && str[0]!='\t') || str[0]==0x7F){	// コントロールコードだった場合
				switch(str[0]){
				case SIME_KEY_START:							// [START]メニュー
					tab = 0;
					cppst = 0;
					if (gTextClip!=NULL){						// コピーした文章があるなら
						tab = 1;
					}
					if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr)
						gTSel[0].rno = -1;
					if (gTSel[0].rno!=-1){						// 範囲選択時
						tab = 1;
						cppst |= 0x02;
					}
					if (gTextClip!=NULL) cppst |= 0x01;
					if (gUndoSize!=0) cppst |= 0x04;
					mret = menu(textFile,tab,cppst,&cs);
					if (mret==1){								// 画面を初期化
						cs.cx = cs.cy = cs.cxb = cs.cxr = cs.pline = cs.rline = 0;
						cs.sx = 0;
						cs.adjust = 0;
						vy = 0;									// 縦スクロール時の端数管理
						cs.rno = GetRNo(cs.pline+cs.cy);
						DrawTitle(textFile);
						DrawSts( &cs, 1 );
						DrawFullRec(cs.pline);					// 画面全域を書き換え
					} else if (mret!=3){
						DrawFullRec(cs.pline);					// 画面全域を書き換え
					}
					Unpush();									// ボタンが離されるまで待機
					break;
				case SIME_KEY_SELECT:							// フリーズオワタへ戻る
					gExitRequest = -1;							// 終了
					Unpush();									// ボタンが離されるまで待機
//					if (gTSel[0].rno==-1){
//						gTSel[0].rno = cs.rno;
//						gTSel[0].cxr = cs.cxr;
//					} else {
//						gTSel[0].rno = -1;
//						DrawFullRec(cs.pline);
//					}
					break;
				case SIME_KEY_LTRIGGER:							// [L]
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					for (i=0; i<PAGEMOVE ;i++){
						DownShift( &cs,0 );						// 下スクロール
						ScreenView();							// 画面更新
					}
					break;
				case SIME_KEY_RTRIGGER:							// [R]
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					for (i=0; i<PAGEMOVE ;i++){
						UpShift( &cs,0 );						// 上スクロール
						ScreenView();							// 画面更新
					}
					break;
				case SIME_KEY_CIRCLE:							// ○
					if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr)
						gTSel[0].rno = -1;
					if (gTSel[0].rno!=-1){
						TextDel(&cs);
						gTSel[0].rno = -1;
						DrawFullRec(cs.pline);					// 画面全域を書き換え
					}
					ret = AddRet( &cs,1,1 );
					if (!ret) gSave = 1;
					cs.adjust = 1;
					break;
				case SIME_KEY_UP:								// ↑
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					CMUp( &cs, 1 );
					cs.adjust = 1;
					break;
				case SIME_KEY_DOWN:								// ↓
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					CMDown( &cs, 1 );
					cs.adjust = 1;
					break;
				case 0x7F:										// [DEL]
				case 0x08:										// [BS]
					if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr)
						gTSel[0].rno = -1;
					if (gTSel[0].rno!=-1){
						TextDel(&cs);
						gTSel[0].rno = -1;
						DrawFullRec(cs.pline);					// 画面全域を書き換え
						cs.adjust = 1;
						break;
					}
					gSave = 1;
					if (str[0]==0x7F){							// [DEL]
						DelStr( &cs,1,1 );						// カーソル位置の文字を削除
						cs.adjust = 1;
						break;
					}
				case SIME_KEY_LEFT:								// ←
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					CMLeft( &cs,1 );
					if (str[0]==0x08){
						DelStr( &cs,1,1 );						// カーソル位置の文字を削除
					}
					cs.adjust = 1;
					break;
				case SIME_KEY_RIGHT:							// →
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					CMRight( &cs, 1 );
					cs.adjust = 1;
					break;
				}
			} else {											// 文字だった場合
				if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr)
					gTSel[0].rno = -1;
				if (gTSel[0].rno!=-1){
					TextDel(&cs);
					gTSel[0].rno = -1;
					DrawFullRec(cs.pline);						// 画面全域を書き換え
				}
				gSave = 1;
				ret = AddStr( &cs, str, 1, 1 );
				cs.adjust = 1;
			}
			if (save!=gSave){									// 「更新」の書き換え
				save = gSave;
				DrawTitle(textFile);
				DrawSts( &cs, 1 );								// ステータス更新
			} else {
				DrawSts( &cs, 0 );								// ステータス更新
			}
			if (str[0]==SIME_KEY_START && gTSel[0].rno!=-1){	// メニューを開いても範囲選択が維持されるように
				pad.Buttons |= PSP_CTRL_CROSS;
				pad2.Buttons |= PSP_CTRL_CROSS;
			}
			if (!(pad.Buttons & PSP_CTRL_CROSS) && (pad2.Buttons & PSP_CTRL_CROSS)){
				if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr){
					gTSel[0].rno = -1;							// 範囲が指定されていないのでこれだけ
				} else {
					gTSel[0].rno = -1;
					DrawFullRec(cs.pline);						// 画面全域を書き換え
				}
			} else if ((pad.Buttons & PSP_CTRL_CROSS) && !(pad2.Buttons & PSP_CTRL_CROSS)){
				gTSel[0].rno = cs.rno;
				gTSel[0].cxr = cs.cxr;
				gTSel[0].line = cs.pline + cs.cy;
			}
			pad2 = pad;
		}

		dcFlag = 0;												// カーソルの消去を重複実行しないためのフラグ（ナナメにスクロールする場合の対応）
		if (gRoll){
			if (pad.Lx<128){									// アナログパッドによる横スクロール
				if (pad.Lx<(128-40)){							// 左へ
					off1 = gXOffset;
					gXOffset += (pad.Lx-128+41)/16;
					if (gXOffset<0) gXOffset = 0;
					off2 = gXOffset;
					if (off1!=off2){
						RightShift( &cs,off1-off2 );			// 画面表示を右にずらす
						dcFlag = 1;
					}
				}
			} else {
				if (pad.Lx>128+40){								// 右へ
					off1 = gXOffset;
					gXOffset += (pad.Lx-128-40)/16;
					if (gXOffset>WWIDTH-LWIDTH) gXOffset = WWIDTH-LWIDTH;
					off2 = gXOffset;
					if (off1!=off2){
						LeftShift( &cs,off2-off1 );				// 画面表示を左にずらす
						dcFlag = 1;
					}
				}
			}
		}
		if (pad.Ly<128){										// アナログパッドによる縦スクロール
			if (pad.Ly<(128-40)){
				off1 = vy;
				vy += (float)(pad.Ly-128+41)/200;
				off2 = vy;
				if (off1!=off2){
					vy += 1;
					DownShift( &cs,dcFlag );					// 下スクロール
				}
			}
		} else {
			if (pad.Ly>128+40){
				off1 = vy;
				vy += (float)(pad.Ly-128-40)/200;
				off2 = vy;
				if (off1!=off2){
					vy -= 1;
					UpShift( &cs,dcFlag );						// 上スクロール
				}
			}
		}

		if (ret){
			strcpy( msg,"使用できるメモリが足りなくなりました。\nこれ以上文字を追加できません。" );
			DiaBox1( -1, 110, 0, msg, gSCor[0], gSCor[3], gSCor[4] );	// 中央に表示
			Unpush();											// ボタンが離されるまで待機
			WaitBtn();											// 何か押されるまで待機
			DrawFullRec(cs.pline);								// 画面全域を書き換え
		}

		sceDisplayWaitVblankStart();
		ScreenView();											// 画面更新
	}

	//----- owata側へ文字列を返す -----
	// 行数制限、サイズ制限を超えた部分は破棄されます。

	if (mode==0){
		line = 1;
		pos = 0;
		rno = 0;
		while (line<=maxline){
			for (i=0; i<gText[rno]->len ;i++){
				text[pos] = gText[rno]->text[i];
				if (chkSJIS(text[pos++])){
					text[pos++] = gText[rno]->text[++i];
				}
				if (pos>maxsize-2) break;						// 文字数制限
			}
			if (pos>maxsize-2) break;							// 文字数制限
			if (gText[rno]->enter) line++;						// 改行数カウント
			if (gText[rno]->chainNext==0xFFFFFFFF) break;		// 最終行に達したら終了
			rno = gText[rno]->chainNext;
		}
		text[pos] = '\0';
	}

	ClrRec();													// 行レコード構造体の全削除

	//----- 終了処理 -----

	SaveEnv();
	InputHisSave( HISFILEJUMP ,gHisLJump );
	InputHisFree( gHisLJump );
	InputHisSave( HISFILESARH ,gHisSarh );
	InputHisFree( gHisSarh );
	InputHisSave( HISFILECHG  ,gHisChg );
	InputHisFree( gHisChg );
	InputHisSave( HISFILEPASTE,gHisFPaste );
	InputHisFree( gHisFPaste );
	InputHisSave( HISFILEFONT ,gHisFont );
	InputHisFree( gHisFont );
	InputHisSave( HISFILEFILE ,gHisFile );
	InputHisFree( gHisFile );
	if (gTCChg){												// テキストクリップに変更があった
		fd = sceIoOpen( CLIPBOARD, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
		if (fd>=0){
			sceIoWrite( fd, gTextClip, strlen(gTextClip) );
			sceIoClose( fd );
		}
	}
	free(gTextClip);
//	pf_endIntraFont();
//	sceKernelExitGame();
//	SMEMOend();

	sceGuStart( GU_DIRECT, gList );
	sceGuClearColor(0x00000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);												// VRAMクリア

	gExitRequest = 0;
	return 0;
}


//==============================================================
// Exit callback
//--------------------------------------------------------------
/*
int exit_callback(int arg1, int arg2, void *common)
{
	gExitRequest = 1;
	return 0;
}

//==============================================================
// Callback thread
//--------------------------------------------------------------

int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);

	sceKernelSleepThreadCB();

	return 0;
}

//==============================================================
// Sets up the callback thread and returns its thread id
//--------------------------------------------------------------

int SetupCallbacks(void)
{
	int thid = 0;

	thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, THREAD_ATTR_USER, 0);
	if(thid >= 0){
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}
*/

