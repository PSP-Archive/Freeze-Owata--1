//==============================================================
// Simple IME   (STPP.04)
//     for PSP CFW5.00 M33-6
// STEAR 2009-2010
//--------------------------------------------------------------
// PSP用の簡易IMEを作成してみた。
// ソフトウェアキーボードと熟語漢字変換とユーザー辞書（予測変換）を実装。
//--------------------------------------------------------------

#include <pspuser.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

//#include "intraFont.h"
#include "graphics.h"
#include "zenkaku.h"
#include "draw.h"
#include "osk.h"
#include "sime.h"

//----- マクロ -----


//----- 定数設定 -----

#define SIMENAME	"Simple IME Ver1.32"

#define DICFILE1	"SIME.DIC"					// 基本辞書ファイル名
#define DICFILE2	"ms0:/PSP/COMMON/SIME.DIC"
#define DIC2FILE1	"SIMEUSER.DIC"				// ユーザー辞書ファイル名
#define DIC2FILE2	"ms0:/PSP/COMMON/SIMEUSER.DIC"
#define INIFILE1	"SIME.INI"					// 環境設定ファイル
#define INIFILE2	"ms0:/PSP/COMMON/SIME.INI"

#define KBPOSX1		(268)						// ソフトウェアキーの位置X（５０音配列アシストカーソル）
#define KBPOSY1		(157)						// ソフトウェアキーの位置Y（５０音配列アシストカーソル）
#define KBPOSX3		(217)						// ソフトウェアキーの位置X（×５かな配置）
#define KBPOSY3		(142)						// ソフトウェアキーの位置Y（×５かな配置）
#define MEPOSX		(131)						// 設定画面の位置
#define MEPOSY		(35)						// 設定画面の位置
#define KEYREP1		(20)						// キーリピート開始までの時間
#define KEYREP2		(3)							// キーリピートの間隔
#define FONTX		(6)							// フォントの長さ
#define FONTY		(16)						// フォントの最大高さ
#define CHLEN		(32)						// 変換ラインの最大文字数
#define KLIST		(6)							// 漢字候補リストの行数
#define KMAX		(102)						// 漢字候補リストの最大数
#define CBTIME		(25)						// カーソル点滅周期
#define CURSTEP		(7)							// アシストカーソルの移動所要時間
#define YOMILMAX	(102)						// この数以下に「よみ」候補数がなったらリストを表示させる

#define CORFL1		0xC0A0A0					// 枠
#define CORFL2		0xF0D0D0					// 明るい枠
#define CORFL3		0x707070					// 暗い枠
#define CORIN		0x600060					// ウィンドウ内背景
#define CORFR		0xA08080					// ウィンドウ内前景
#define CORCUR		0xFF8080					// カーソル
#define CORCUR1		0x7070FF					// カーソル○
#define CORCUR2		0x008000					// カーソル△
#define CORCUR3		0xA0A0E0					// カーソル□
#define CORCUR4		0xFF8080					// カーソル×
#define CORRBAR		0x70E070					// スクロールバー
#define CORWCHR		0xFFFFFF					// ウィンドウ内文字
#define CORSCHR		0xB0FFB0					// ウィンドウ内特殊文字
#define CORFCHR		0x000000					// ウィンドウ枠文字
#define CORCHBK		0xFFFFFF					// 変換ライン背景
#define CORCHCR		0x000000					// 変換ライン文字
#define CORCHCU		0xFF8080					// 変換ライン選択領域

//----- プロトタイプ宣言 -----

static void putBack(void);
static unsigned int	fcode(char *str);
static unsigned int	getInt(char *str);
static void kList(char wordList[][2][33],int count,int index);
static void getkList(char wordList[][2][33],int count);
static void putkList(void);

//------ グローバル変数 -----

static char			gKeyName1[7][25] = {
						"１：全角 ひらがな",
						"２：全角 カタカナ",
						"３：半角 カタカナ",
						"４：全角 アルファベット",
						"５：全角 記号",
						"６：半角 アルファベット",
						"７：半角 記号",
					};
static char			gKeyTable1[7][161] = {
						"０あいうえお１かきくけこ２さしすせそ３たちつてと４なにぬねの５はひふへほ６まみむめも７やゆよ゛゜８らりるれろ９わをん、。！ぁぃぅぇぉ？ゃゅょゎっ”ー「」・　",
						"０アイウエオ１カキクケコ２サシスセソ\３タチツテト４ナニヌネノ５ハヒフヘホ６マミムメモ７ヤユヨ゛゜８ラリルレロ９ワヲン、。！ァィゥェォ？ャュョヮッ”ー「」・　",
						"0 ｱ ｲ ｳ ｴ ｵ 1 ｶ ｷ ｸ ｹ ｺ 2 ｻ ｼ ｽ ｾ ｿ 3 ﾀ ﾁ ﾂ ﾃ ﾄ 4 ﾅ ﾆ ﾇ ﾈ ﾉ 5 ﾊ ﾋ ﾌ ﾍ ﾎ 6 ﾏ ﾐ ﾑ ﾒ ﾓ 7 ﾔ ﾕ ﾖ ﾞ ﾟ 8 ﾗ ﾘ ﾙ ﾚ ﾛ 9 ﾜ ｦ ﾝ ､ ｡ ! ｧ ｨ ｩ ｪ ｫ ? ｬ ｭ ｮ   ｯ \" ｰ ｢ ｣ ･  ",
						"０ＡＢＣＤＥ１ａｂｃｄｅ２ＦＧＨＩＪ３ｆｇｈｉｊ４ＫＬＭＮＯ５ｋｌｍｎｏ６ＰＱＲＳＴ７ｐｑｒｓｔ８ＵＶＷＸＹ９ｕｖｗｘｙ！Ｚｚ－，．？［（＜｛”’］）＞｝　",
						"０！”＃（％１？’＆）＄２↑↓←→＝３＊＋－±×４／＜｛，；５＼＞｝．：６［＿＠￥＾７］｜～・…８℃※〒♪☆９△□○×◎▽◇ゝゞ∀Λξ∩∪⊂⊃φωД彡∑⊿　",
						"0 A B C D E 1 a b c d e 2 F G H I J 3 f g h i j 4 K L M N O 5 k l m n o 6 P Q R S T 7 p q r s t 8 U V W X Y 9 u v w x y ! Z z - , . ? [ ( < { : / ] ) > }   ",
						"7 4 1 0 + * 8 5 2 , - / 9 6 3 . = #             ( < [ { : ! ) > ] } ; ? \" $ % & @ \\ ' ^ _ | ~                                                     \r \b \x7F \t   "
					};

static char			gKeyTable3[7][181] = {
						"あいうえおかきくけこさしすせそたちつてとなにぬねのはひふへほまみむめもらりるれろやゆよわをゃゅょゎんぁぃぅぇぉっ゛゜、。ー「！」？０１２３４５６７８９・（…）～－＋＝換＊\t \b \x7F \r 　",
						"アイウエオカキクケコサシスセソ\タチツテトナニヌネノハヒフヘホマミムメモラリルレロヤユヨワヲャュョヮンァィゥェォッ゛゜、。ー「！」？０１２３４５６７８９・ヱヵヶヴ－（～）＋\t \b \x7F \r 　",	// 「ソ\」注意！
						"ｱ ｲ ｳ ｴ ｵ ｶ ｷ ｸ ｹ ｺ ｻ ｼ ｽ ｾ ｿ ﾀ ﾁ ﾂ ﾃ ﾄ ﾅ ﾆ ﾇ ﾈ ﾉ ﾊ ﾋ ﾌ ﾍ ﾎ ﾏ ﾐ ﾑ ﾒ ﾓ ﾗ ﾘ ﾙ ﾚ ﾛ ﾔ ﾕ ﾖ ﾜ ｦ ｬ ｭ ｮ   ﾝ ｧ ｨ ｩ ｪ ｫ ｯ ﾞ ﾟ ､ ｡ ｰ ｢ ! ｣ ? 0 1 2 3 4 5 6 7 8 9 ･ ~ | / _ - ( = ) + \t \b \x7F \r   ",
						"０１２３４５６７８９ＡＢＣＤＥＦＧＨＩＪＫＬＭＮＯＰＱＲＳＴＵＶＷＸＹＺａｂｃｄｅｆｇｈｉｊｋｌｍｎｏｐｑｒｓｔｕｖｗｘｙｚ’，＿！（．）？＜［＠］＞：｛／｝～＝－＊＋￥\t \b \x7F \r 　",
						"０１２３４５６７８９”＃＄％＆，（＝）．＊＋±－×：／＠＼；！＜＿＞？＾［￥］’｜｛～｝・…℃※〒♪↑←☆→↓△□◎○×▽◇ゝゞΛ∩⊂∀⊃∪ξφΦωД彡∑⊿лΩ≒≪√≫≠\t \b \x7F \r 　",
						"0 1 2 3 4 5 6 7 8 9 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z ' , _ ! ( . ) ? < [ @ ] > : { / } ; = - * + \\ \t \b \x7F \r   ",
						"0 1 2 3 4 5 6 7 8 9 ! \" # $ % & ( ' ) * + , - . / : < = > ; ? [ \\ ] @ ^ { _ } | ' ｢ ~ ｣ ､                                                                                 \t \b \x7F \r   "
					};
static long			gHEDtbl[85],gDicWordPos,gDicSize;			// 基本辞書管理用
static DrawImg 		*gBackBuf,									// ソフトキー背景待避バッファ
					*gInLnBuf,									// 変換ライン背景待避バッファ
					*gkListBuf;									// 漢字候補リスト背景待避バッファ
static char			*gDic,gDicFile[33],gDic2File[33],gIniFile[33],
					gSt[CHLEN +1],								// 変換行の文字
					gYomi[CHLEN +1],gKouho[CHLEN +1];			// 変換候補学習用
static int			gSetup = 0,									// 初期化されている？（0:No）
					gMode,										// 遷移状態
					gBDFlag,									// ウィンドウ消去時の挙動
					gSaveFlag,									// 基本辞書に変化があった？（0:No）
					gSave,										// 基本辞書の学習内容を保存するか
					gUSaveFlag,									// ユーザー辞書に変化があった？（0:No）
					gUSave,										// ユーザー辞書を使用するか
					gUDicAdd,									// ユーザー辞書に候補を追加する？（0:Yes）
					gIni,										// 環境設定に変化があった？（0:No）
					gKey,										// ソフトキーの種類
					gGetFlag = 0,								// ウィンドウ系を消去した？（0:No）
					gCount = 0,									// 変換候補の総数
					gPage;										// 作画を行う画面ページ
static int			gCx,gCy,									// 文字カーソルの位置
					gCxw,gCyw,									// 文字カーソルの形状
					gFont;										// 文字種別

static struct {
	int	x;
	int	y;
} gBackBufP,													// ソフトキー背景の位置
  gInLnBufP,													// 変換ライン背景の位置
  gkListBufP;													// 漢字候補リスト背景の位置

static struct strUsDic {										// ユーザー辞書管理構造体
	struct strUsDic	*chain;										// 次の候補へのポインタ（NULLだと終端）
	char			yomi[CHLEN +1];								// 「よみ」
	char			kouho[CHLEN +1];							// 「候補」
} **gUsDic;


//==============================================================
// 基本辞書ファイルの読み込み
//--------------------------------------------------------------
// 戻り値  0:辞書の読み込み成功
//        -1:不正な辞書ファイル
//        -2:モリが足りない
//        -3:辞書ファイルが開けない（存在しない？）
//--------------------------------------------------------------

static int DicLoad(char *DicFile)
{
	int		i,fa,fd;
	long	pos,filesize;

	fa = 0;
	fd = sceIoOpen( DicFile, PSP_O_RDONLY, 0777 );
	if (fd>=0){
		filesize = sceIoLseek(fd, 0, SEEK_END);
		sceIoLseek(fd, 0, SEEK_SET);
		pos = sceIoRead( fd,gHEDtbl, sizeof(gHEDtbl) );
		if (pos!=sizeof(gHEDtbl)){
			fa = -1;											// ジャンプテーブルが足りない
		} else {
			for (i=0; i<85 ;i++){
				if (gHEDtbl[i]!=0xFFFFFFFF && (gHEDtbl[i]>filesize-pos)){
					fa = -1;									// ジャンプ先がデータ外を示している
				}
			}
			if (!fa){
				gDic = (char*) malloc( filesize - pos );
				if (gDic==NULL){
					fa = -2;									// メモリを確保できない
				} else {
					gDicSize = filesize - pos;
					sceIoRead( fd,gDic, gDicSize );
				}
			}
		}
		sceIoClose(fd);
	} else {
		fa = -3;
	}

	if (fa){													// 辞書ファイルが開けなかった
		for (i=0; i<85 ;i++){
			gHEDtbl[i] = 0xFFFFFFFF;
		}
	}

	return (fa);
}


//==============================================================
// ユーザー辞書ファイルの初期化
//--------------------------------------------------------------

static void UserDicInit(void)
{
	int		i;

	gUsDic = (struct strUsDic**) malloc( sizeof(struct strUsDic*) * 85 );
	if (gUsDic){
		for (i=0; i<85 ;i++){
			gUsDic[i] = NULL;
		}
	}
}


//==============================================================
// ユーザー辞書用メモリ領域を開放
//--------------------------------------------------------------

static void UserDicFree(void)
{
	struct strUsDic	*usdic,*usdic2;
	int		i;

	for (i=0; i<85 ;i++){
		usdic = gUsDic[i];
		if (usdic){												// 「候補」群があるなら
			do{
				usdic2 = usdic->chain;
				free(usdic);
				usdic = usdic2;
			}while (usdic);										// 「候補」が続く限り削除し続ける
		}
	}
	free(gUsDic);												// ユーザー辞書を削除
}


//==============================================================
// ユーザー辞書ファイルの読み込み
//--------------------------------------------------------------
// 対象となるユーザー辞書は gDic2File で指定されているモノです。
// ユーザー辞書ファイル構造に異常が発見された場合は読み込みを中止し、ユーザー辞書
// を初期化します。
//--------------------------------------------------------------

//----- バッファ読み込み -----

static void UserDicLoadBuf(int fd,char *data,int *pos,int worksize)
{
	if (*pos<worksize) return;
	sceIoRead( fd, data, worksize );
	*pos = 0;
}

//----- 単語を取り込む -----

static int UserDicLoadWord(char *str,int fd,char *data,int *pos,int worksize)
{
	char	c;
	int		len;

	len = 0;
	c = -1;
	while (len<33 && c!='\0'){
		c = data[(*pos)++];
		UserDicLoadBuf( fd, data, pos, worksize );				// バッファ内データが尽きたら続きをロード
		str[len++] = c;
	}
	if (c!='\0'){												// 文字数オーバー（不正な辞書）
		return (-1);
	}
	return (0);
}

//----- メイン -----

static void UserDicLoad(void)
{
	const int		ws[4] = {16384,4096,1024,128};
	struct strUsDic	*usdic,*usdic2;
	char	*data,yomi[33],kouho[33];
	int		i,fd,hp,pos,flag,worksize;
	long	filesize,poscnt;

	//----- 辞書初期化（通常はしなくてもいいんだけど一応） -----

	if (gUsDic){
		UserDicFree();
		UserDicInit();											// ユーザー辞書初期化
	}

	//----- 準備 -----

	for (i=0; i<4 ;i++){										// 出来るだけ大きくバッファを確保する
		worksize = ws[i];
		data = (char*) malloc(worksize);
		if (data) break;										// バッファの確保に成功した
	}
	if (!data) return;											// バッファが確保できなかったので終了
	fd = sceIoOpen( gDic2File, PSP_O_RDONLY, 0777 );
	if (fd>=0){
		filesize = sceIoLseek(fd, 0, SEEK_END);
		sceIoLseek(fd, 0, SEEK_SET);
	} else {													// 指定ファイルが開けなかったので終了
		free(data);
		return;
	}

	//----- 辞書読み込み -----

	sceIoRead( fd, data, worksize );
	poscnt = 0;
	pos = 0;
	hp = 0;
	flag = 0;
	while (poscnt<filesize && hp<85){
		if (data[pos]!=0){										// 「候補」群があるなら
			usdic2 = NULL;
			while (data[pos]!=0){
				if (UserDicLoadWord( yomi, fd, data, &pos, worksize )){
					flag = -1;									// 文字数オーバー（不正な辞書）
					break;
				}
				if (UserDicLoadWord( kouho, fd, data, &pos, worksize )){
					flag = -1;									// 文字数オーバー（不正な辞書）
					break;
				}
				usdic = (struct strUsDic*) malloc( sizeof(struct strUsDic) );
				if (usdic){										// メモリが取得出来たなら
					usdic->chain = NULL;
					strcpy( usdic->yomi,yomi );
					strcpy( usdic->kouho,kouho );
					if (!usdic2){								// 最初の「候補」
						gUsDic[hp] = usdic;
					} else {									// ２番目以降の「候補」
						usdic2->chain = usdic;
					}
					usdic2 = usdic;
				} else {										// メモリが取得出来なかった
					flag = 1;									// 読めたところまでは採用
					break;
				}
			}
		}
		if (flag) break;										// 読み込み中断
		pos++;
		UserDicLoadBuf( fd, data, &pos, worksize );				// バッファ内データが尽きたら続きをロード
		hp++;
	}
	sceIoClose( fd );

	//----- 異常発生対策 -----

	if (flag<0){												// 辞書内容が不正だった
		UserDicFree();
		UserDicInit();											// ユーザー辞書初期化
	}
}


//==============================================================
// 辞書ファイルの読み込みと各種初期化
//--------------------------------------------------------------
// flag      0:ウィンドウ消去時に背景を復元する
//        以外:ウィンドウ消去時に何もしない（メイン側で作画するべし）
// 戻り値    1:既に初期化されている
//           0:辞書の読み込み成功
//          -1:不正な辞書ファイル
//          -2:メモリが足りない
//          -3:辞書ファイルが開けない（存在しない？）
//--------------------------------------------------------------
// 辞書ファイルとIME環境設定を読み込みます。
// 既に初期化されている場合は flag の設定変更のみ実行します。
// 環境設定ファイルは辞書ファイルと同じフォルダに配置されます。
// ユーザー辞書が読めなかった場合の警告はありません。
//--------------------------------------------------------------

int InitSIME(int flag)
{
	char	str[256],*p;
	int		ret,type,val;
	FILE*	fp1;

	//----- 環境変数初期化 -----

	if (flag){
		gBDFlag = 1;											// ウィンドウ消去時に何もしない
	} else {
		gBDFlag = 0;											// ウィンドウ消去時に背景を復元する
	}

	if (gSetup) return (1);
	gSetup = 1;

	gMode = 0;
	gCxw = 2;													// カーソルの大きさ
	gDicWordPos = -1;
	gDicSize = 0;
	gSaveFlag = 0;
	gSave = 0;													// 終了時に辞書の記載順を保存する
	gUSaveFlag = 0;
	gUSave = 0;													// ユーザー辞書を使う
	gUDicAdd = 0;												// ユーザー辞書に候補を追加する
	gIni = 0;
	gKey = 0;													// ソフトキーの種類
	gPage = 0;
	gSt[0] = '\0';
	gkListBufP.x = -1;

	//----- 基本辞書ロード -----

	ret = DicLoad( DICFILE1 );
	strcpy( gDicFile ,DICFILE1 );
	strcpy( gDic2File ,DIC2FILE1 );
	strcpy( gIniFile ,INIFILE1 );
	if (ret){
		ret = DicLoad( DICFILE2 );								// COMMONフォルダ
		strcpy( gDicFile ,DICFILE2 );
		strcpy( gDic2File ,DIC2FILE2 );
		strcpy( gIniFile ,INIFILE2 );
		if (ret){
			gDicFile[0] = '\0';
			strcpy( gIniFile ,INIFILE1 );
		}
	}

	//----- 環境設定ロード -----

	fp1 = fopen( gIniFile, "r" );								// 環境設定ファイルの読み込み
	if (fp1!=NULL){
		while (1){
			if (fgets( str,256,fp1 )==NULL) break;
			p = strtok( str," =\t" );
			type = 0;
			if (strstr( p,"KEYTYPE" )!=NULL) type = 1;
			if (strstr( p,"SAVEMODE" )!=NULL) type = 2;
			if (strstr( p,"SAVEMODE2" )!=NULL) type = 3;
			if (strstr( p,"USERDICADD" )!=NULL) type = 4;
			p = strtok( NULL," =\t" );
			val = (int)strtol( p,NULL,0 );						// 文字列→数値変換
			if (val==0){										// 数字ではないっぽいので
				if (strstr( p,"YES" )!=NULL) val = 1;
				if (strstr( p,"NO" )!=NULL) val = 2;
				if (type==2 && val>0) gSave = val -1;
				if (type==3 && val>0) gUSave = val -1;
				if (type==4 && val>0) gUDicAdd = val -1;
			} else {
				if (type==1 && val>0 && val<=3) gKey = val -1;
			}
		}
		fclose (fp1);
	}

	//----- ユーザー辞書読み込み -----

	UserDicInit();												// ユーザー辞書初期化
	if (!gUSave) UserDicLoad();

	return (ret);
}


//==============================================================
// 辞書ファイルへ書き込み
//--------------------------------------------------------------
// 漢字候補の並べ替え結果を辞書に書き込む
//--------------------------------------------------------------

static void DicSave(void)
{
	int		fd;

	if (!gSave && gSaveFlag && gDicSize){
		fd = sceIoOpen( gDicFile, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
		if (fd>=0){
			sceIoWrite( fd, gHEDtbl, sizeof(gHEDtbl) );
			sceIoWrite( fd, gDic, gDicSize );
			sceIoClose( fd );
		}
	}
}


//==============================================================
// ユーザー辞書ファイルへ書き込み
//--------------------------------------------------------------

//----- バッファをファイルへ書き出す -----
// バッファがいっぱいになったらファイルへ書き出す。
// バッファに余裕があるときは何もしない。
// ファイルへ書き出したときはポインタの値を0にする。
// バッファへのデータ追加は1バイトずつ行う事。

static void UserDicFlash(int fd,char *tmp,int *pos,int bufsize)
{
	if (*pos<bufsize) return;
	sceIoWrite( fd, tmp, *pos );
	*pos = 0;
}

//----- メイン -----

static void UserDicSave(void)
{
	const int		ws[4] = {16384,4096,1024,128};
	struct strUsDic	*usdic;
	char	*data;
	int		i,j,fd,pos,worksize;

	if (!gUSaveFlag) return;									// ユーザー辞書に変化がない

	for (i=0; i<4 ;i++){										// 出来るだけ大きくバッファを確保する
		worksize = ws[i];
		data = (char*) malloc(worksize);
		if (data) break;										// バッファの確保に成功した
	}
	if (!data) return;											// バッファが確保できなかったので終了

	fd = sceIoOpen( gDic2File, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if (fd>=0){
		pos = 0;
		for (i=0; i<85 ;i++){
			usdic = gUsDic[i];
			if (!usdic){										// 「候補」群が登録されていない
				data[pos++] = 0;
				UserDicFlash( fd, data, &pos, worksize );
			} else {
				do{
					for (j=0; j<=strlen(usdic->yomi) ;j++){		// '\0'も含める
						data[pos++] = usdic->yomi[j];
						UserDicFlash( fd, data, &pos, worksize );
					}
					for (j=0; j<=strlen(usdic->kouho) ;j++){	// '\0'も含める
						data[pos++] = usdic->kouho[j];
						UserDicFlash( fd, data, &pos, worksize );
					}
					usdic = usdic->chain;
				}while (usdic);									// 次の「候補」が続く限り繰り返す
				data[pos++] = 0;								// 「候補」群終了
				UserDicFlash( fd, data, &pos, worksize );
			}
		}
		UserDicFlash( fd, data, &pos, 0 );						// バッファの強制書き出し
		sceIoClose(fd);
	}
	free(data);													// バッファを解放
}


//==============================================================
// 終了処理
//--------------------------------------------------------------
// メモリに保持していた辞書をファイルに書き戻した後、開放する。
// これを実行せずに電源を切る等をすると漢字候補の並べ替えが次回に反映されません。
// また、IME環境データを設定ファイルに保存します。
// 設定ファイルの位置は辞書ファイルと同じフォルダになりますが、
// 辞書が見つからなかった場合は EBOOT.PBP と同じフォルダになります。
//--------------------------------------------------------------

void EndSIME(void)
{
	FILE*	fp1;
	char	str[256],item[2][4] = {"YES","NO"},s[3] = {"1\n"};

	if (gIni){
		fp1 = fopen( gIniFile, "w" );
		if (fp1!=NULL){
			strcpy( str,"KEYTYPE = " );
			s[0] = '1' + gKey;
			strcat( str,s );
			fputs( str,fp1 );
			strcpy( str,"SAVEMODE = " );
			strcat( str,item[gSave] );
			strcat( str,"\n" );
			fputs( str,fp1 );
			strcpy( str,"SAVEMODE2 = " );
			strcat( str,item[gUSave] );
			strcat( str,"\n" );
			fputs( str,fp1 );
			fclose(fp1);
		}
	}
	UserDicSave();												// ユーザー辞書保存
	UserDicFree();												// ユーザー辞書用メモリを解放
	DicSave();													// 基本辞書保存
	free(gDic);
	DrawImgFree(gBackBuf);
	DrawImgFree(gInLnBuf);
	DrawImgFree(gkListBuf);
	gSetup = 0;
}


//==============================================================
// 文字フォント指定
//--------------------------------------------------------------
// font   フォント種別
// cyw    フォントの高さ（16まで）
// 戻り値 指定する前のフォント種別
//--------------------------------------------------------------
// 変換行に使用する文字フォントを指定する。
// ただし各種ウィンドゥに表示される文字は東雲フォントに固定。
//--------------------------------------------------------------

int SIMEfont(int font,int cyw)
{
	int		fontBak;

	if (cyw<1) cyw = 1;
	if (cyw>16) cyw = 16;
	gCyw = cyw;
	fontBak = gFont;
	gFont = font;
	return (fontBak);
}


//==============================================================
// 文字カーソル指定
//--------------------------------------------------------------
// xw  カーソルのX方向の幅（0以下：カーソル表示なし）
// yw  カーソルのY方向の幅
// x,y カーソル位置
//--------------------------------------------------------------
// カーソル座標を指定する変数とカーソル形状を指定する。
// カーソル表示はSgets()を実行している間に更新されます。
// カーソルはI型で、Y方向の大きさはSIMEfont()で指定します。
//--------------------------------------------------------------

void SIMEcursor(int xw,int x,int y)
{
	gCx = x;
	gCy = y;
	gCxw = xw;
	if (gCxw<0) gCxw = 0;
	if (gCxw>16) gCxw =16;
}


//==============================================================
// 画面作画設定
//--------------------------------------------------------------
// page 作画を行う画面ページ（0か1）
//--------------------------------------------------------------
// 作画を行う画面ページを指定します。
// sceGuDrawBuffer()とかの設定は考慮していません。
// VRAMの先頭から縦272ドット毎にページ0,1と決め打ちしています。
// メイン側の作画仕様によっては使えない事もあると思いますが、その時は適当に
// 修正するべし。
//--------------------------------------------------------------

void SIMESetDraw(int page)
{
	gPage = page * 272;
}


//==============================================================
// キー入力
//--------------------------------------------------------------
// 戻り値 0:入力なし
//        →:0x1C ←:0x1D ↑:0x1E ↓:0x1F
//        ○:0x0D（[Enter]） ×:0x1B（[Esc]） □:0x08（[BS]） △:0x02 L:0x03 R:0x04
//        [START]:0x05 [SELECT]:0x06
//--------------------------------------------------------------
// 各ボタンの状態をキーボード風なキーリピートありで取得する。
// 同時押しの場合は最後に押し下げたボタンに対応するコードを返す。
// これは文字入力はしないけど、キーリピートありでキー入力を行いたい場合などを
// 想定して用意してあります。
//--------------------------------------------------------------

int SIMEgetchar(SceCtrlData pad1)
{
	static SceCtrlData	pad2;
	static int			rep = 0,kcode = 0,button = 0;

	if ((pad1.Buttons & PSP_CTRL_UP) && !(pad2.Buttons & PSP_CTRL_UP)){			// 押し下げた瞬間を検出
		rep = 0;																// リピートカウンタリセット
		kcode = 0x1E;															// 出力コード
		button = PSP_CTRL_UP;													// リピートを監視するボタン
	}
	if ((pad1.Buttons & PSP_CTRL_DOWN) && !(pad2.Buttons & PSP_CTRL_DOWN)){
		rep = 0;
		kcode = 0x1F;
		button = PSP_CTRL_DOWN;
	}
	if ((pad1.Buttons & PSP_CTRL_LEFT) && !(pad2.Buttons & PSP_CTRL_LEFT)){
		rep = 0;
		kcode = 0x1D;
		button = PSP_CTRL_LEFT;
	}
	if ((pad1.Buttons & PSP_CTRL_RIGHT) && !(pad2.Buttons & PSP_CTRL_RIGHT)){
		rep = 0;
		kcode = 0x1C;
		button = PSP_CTRL_RIGHT;
	}
	if ((pad1.Buttons & PSP_CTRL_TRIANGLE) && !(pad2.Buttons & PSP_CTRL_TRIANGLE)){
		rep = 0;
		kcode = 0x02;
		button = PSP_CTRL_TRIANGLE;
	}
	if ((pad1.Buttons & PSP_CTRL_CROSS) && !(pad2.Buttons & PSP_CTRL_CROSS)){
		rep = 0;
		kcode = 0x1B;
		button = PSP_CTRL_CROSS;
	}
	if ((pad1.Buttons & PSP_CTRL_SQUARE) && !(pad2.Buttons & PSP_CTRL_SQUARE)){
		rep = 0;
		kcode = 0x08;
		button = PSP_CTRL_SQUARE;
	}
	if ((pad1.Buttons & PSP_CTRL_CIRCLE) && !(pad2.Buttons & PSP_CTRL_CIRCLE)){
		rep = 0;
		kcode = 0x0D;
		button = PSP_CTRL_CIRCLE;
	}
	if ((pad1.Buttons & PSP_CTRL_LTRIGGER) && !(pad2.Buttons & PSP_CTRL_LTRIGGER)){
		rep = 0;
		kcode = 0x03;
		button = PSP_CTRL_LTRIGGER;
	}
	if ((pad1.Buttons & PSP_CTRL_RTRIGGER) && !(pad2.Buttons & PSP_CTRL_RTRIGGER)){
		rep = 0;
		kcode = 0x04;
		button = PSP_CTRL_RTRIGGER;
	}
	if ((pad1.Buttons & PSP_CTRL_START) && !(pad2.Buttons & PSP_CTRL_START)){
		rep = 0;
		kcode = 0x05;
		button = PSP_CTRL_START;
	}
	if ((pad1.Buttons & PSP_CTRL_SELECT) && !(pad2.Buttons & PSP_CTRL_SELECT)){
		rep = 0;
		kcode = 0x06;
		button = PSP_CTRL_SELECT;
	}

	pad2 = pad1;

	if (pad1.Buttons & button){
		rep++;
		if (rep>KEYREP1) rep -= KEYREP2;						// キーリピート
		if (rep==1 || rep==KEYREP1) return (kcode);				// 押していた
	} else {
		rep = 0;
		kcode = 0;
		button = 0;
	}

	return (0);													// 何も押していない
}


//==============================================================
// テキスト枠の表示
//--------------------------------------------------------------
// 凹んだフレームにテキストを表示します。
//--------------------------------------------------------------

static void DrawTextbox(char *str,int x,int y,int wx,long txcor,long bkcor)
{
	BoxFill( x, y, wx, 12+2, CORFL2, bkcor );
	HLine( x, y, wx-1, CORFL3 );								// 上
	VLine( x, y, 12+2-1, CORFL3 );								// 左
	mh_print( x+2, y+1, str, txcor );							// 文字
}


//==============================================================
// ウィンドウ枠の表示
//--------------------------------------------------------------

static void DrawWindow(char *title,int x,int y,int wx,int wy)
{
	y += gPage;

	BoxFill( x, y, wx-1, 12+3, CORFL1 , CORFL1 );
	HLine( x +2       , y +2 , wx-4   , CORFL3 );				// 上
	VLine( x +2       , y +2 , 12+2   , CORFL3 );				// 左
	HLine( x +2       , y +14, wx-4   , CORFL2 );				// 下
	VLine( x +2 +wx -4, y +2 , 12+2   , CORFL2 );				// 右
	mh_print( x +5, y +2, title, CORFCHR );						// タイトル
	HLine( x       , y    , wx, CORFL2 );						// 上
	VLine( x       , y    , wy, CORFL2 );						// 左
	HLine( x       , y +wy, wx, CORFL3 );						// 下
	VLine( x +wx -1, y    , wy, CORFL3 );						// 右
}


//==============================================================
// ウィンドウ背景の待避
//--------------------------------------------------------------

static void getBack(void)
{
	if (gBDFlag) return;

	if (gBackBuf){												// バッファを解放
		DrawImgFree(gBackBuf);
		gBackBuf = NULL;
	}

	switch (gKey){
	case 0:
	case 1:
		gBackBuf = DrawImgCreate( 210,98 +15 );					// ソフトキー背景待避バッファ
		gBackBufP.x = KBPOSX1;
		gBackBufP.y = KBPOSY1;
		BoxCopy( gBackBuf, gBackBufP.x, gPage+gBackBufP.y );
		break;
	case 2:
		gBackBufP.x = KBPOSX3;
		gBackBufP.y = KBPOSY3;
		gBackBuf = DrawImgCreate( 261,129 );					// ソフトキー背景待避バッファ
		BoxCopy( gBackBuf, gBackBufP.x, gPage+gBackBufP.y );
		break;
	}
}


//==============================================================
// ウィンドウ背景の復元
//--------------------------------------------------------------

static void putBack(void)
{
	if (gBDFlag) return;

	BoxPaste( gBackBuf, gBackBufP.x, gPage+gBackBufP.y );
}


//==============================================================
// ソフトウェアキーボードの表示（５０音配列アシストカーソル）
//--------------------------------------------------------------
// x[],y[]   カーソルの到達目標位置
// xx[],yy[] カーソルの元位置
// step      カーソルの到達係数（0:目標位置）
//--------------------------------------------------------------
// xかyがNULLだった場合はカーソルは表示しません。
//--------------------------------------------------------------

static void DrawKey1(int *x,int *y,int t,int *xx,int *yy,int step)
{
	char	key[3] = "  ";
	int		i,j,s,mx,my;
	long	cor[3] = { CORCUR1,CORCUR3,CORCUR4 };

	DrawWindow( gKeyName1[t],KBPOSX1,KBPOSY1,210,97 +15 );						// ウィンドウフレーム
	if (t==0){
		mh_print( KBPOSX1 +210-2-3.5*13, gPage+KBPOSY1 +2, "▲:変換", CORCUR2 );
	}
	BoxFill( KBPOSX1 +1, gPage+KBPOSY1 +15, 210-2, 97, CORFL1, CORIN );			// 内側
	if (x!=NULL && y!=NULL){
		for (i=0; i<3 ;i++){													// カーソルの表示
			mx = (x[i] - xx[i]) *16 * step / CURSTEP;
			my = (y[i] - yy[i]) *16 * step / CURSTEP;
			DrawChar2( KBPOSX1 +1 + x[i]*16 -mx, gPage+KBPOSY1 +15 + y[i]*16 -my, i, cor[i] );
		}
	}
	if (t==2 || t>4){															// 半角文字の位置補正
		s = 3;
	}else{
		s = 0;
	}
	for (i=0; i<6 ;i++){
		for (j=0; j<13 ;j++){
			key[0] = gKeyTable1[t][(j*6+i)*2];
			key[1] = gKeyTable1[t][(j*6+i)*2+1];								// キーマップから一文字取り出す
			mh_print( KBPOSX1 +1 +2 + j*16 +s, gPage+KBPOSY1 +15 +2 + i*16, key, CORWCHR );
		}
	}
	if (t==6){
		mh_print( KBPOSX1 +1 +2 + 12*16-4 , gPage+KBPOSY1 +15 +2 + 1*16, "Ret", CORSCHR );
		mh_print( KBPOSX1 +1 +2 + 12*16-1 , gPage+KBPOSY1 +15 +2 + 2*16, "BS", CORSCHR );
		mh_print( KBPOSX1 +1 +2 + 12*16-4 , gPage+KBPOSY1 +15 +2 + 3*16, "Del", CORSCHR );
		mh_print( KBPOSX1 +1 +2 + 12*16-4 , gPage+KBPOSY1 +15 +2 + 4*16, "Tab", CORSCHR );
	}
	mh_print( KBPOSX1 +1 +2 + 12*16 , gPage+KBPOSY1 +15 +2 + 5*16, "SP", CORSCHR );
}


//==============================================================
// ソフトウェアキーボードの表示（×５かな変換）
//--------------------------------------------------------------
// xかyがマイナスだった場合はカーソルは表示しません。
//--------------------------------------------------------------

static void DrawKey3(int x,int y,int t)
{
	char key[3] = "  ";
	int i,j,k,s;
	int map[5][2] = {{1,0} , {0,1} , {1,1} , {2,1} , {1,2} };

	DrawWindow( gKeyName1[t],KBPOSX3,KBPOSY3,261,127 );						// ウィンドウフレーム
	BoxFill(KBPOSX3 +1, gPage+KBPOSY3 +15, 261 -2, 112, CORFL1, CORIN);		// 内側
	for (i=0; i<3 ;i++){													// 罫線
		HLine(KBPOSX3 +1       , gPage+KBPOSY3 +15 + i*37, 261 -2, CORFL1);
	}
	for (i=0; i<6 ;i++){
		VLine(KBPOSX3 +1 + i*43, gPage+KBPOSY3 +15       , 112, CORFL1);
	}
	if (x>=0 && y>=0)
		BoxFill(KBPOSX3 +1 + x*43, gPage+KBPOSY3 +15 + y*37, 44, 38, CORFL1, CORCUR);

	if (t==2 || t>4){														// 半角文字の位置補正
		s = 3;
	}else{
		s = 0;
	}

	for (i=0; i<3 ;i++){													// ソフトウェアキーボード
		for (j=0; j<6 ;j++){
			for (k=0; k<5 ;k++){
				key[0] = gKeyTable3[t][(i*30+j*5+k)*2];
				key[1] = gKeyTable3[t][(i*30+j*5+k)*2+1];					// キーマップから一文字取り出す
				mh_print(KBPOSX3 +1 + j*43 + map[k][0]*13+3 +s, gPage+KBPOSY3 +15 + i*37 + map[k][1]*12+1, key, CORWCHR);
			}
		}
	}
	if (t==0){
		mh_print(KBPOSX3 +1 +4*43 +2 +27, gPage+KBPOSY3 +15 +2*37+1*12+1, "換", CORSCHR);
	}
	mh_print(KBPOSX3 +1 +5*43 +2 +12, gPage+KBPOSY3 +15 +2*37+0*12+1, "Tab", CORSCHR);
	mh_print(KBPOSX3 +1 +5*43 +2    , gPage+KBPOSY3 +15 +2*37+1*12+1, "BS", CORSCHR);
	mh_print(KBPOSX3 +1 +5*43 +2 +24, gPage+KBPOSY3 +15 +2*37+1*12+1, "Ret", CORSCHR);
	mh_print(KBPOSX3 +1 +5*43 +2 + 6, gPage+KBPOSY3 +15 +2*37+2*12+1, "space", CORSCHR);
}


//==============================================================
// 変換ラインの位置
//--------------------------------------------------------------
// gCx,gCyで指定されているカーソル位置に変換ラインを展開した場合の実際の座標とサイズを計算。
// 画面端やソフトキーとぶつかるなら開始位置を修正する。
//--------------------------------------------------------------

static void InLinePos(char *str,int *x,int *wx)
{
	int		cx,cy;

	if (gKey==2){
		cx = KBPOSX3;
		cy = KBPOSY3;
	} else {
		cx = KBPOSX1;
		cy = KBPOSY1;
	}

	*wx = GetStrWidth( gFont,str );
	*x = gCx;
	if (*x+*wx>480) *x = 480 - *wx;								// 画面からはみ出すなら開始位置をずらす
	if (gCy+gCyw>cy && gMode==1){								// ソフトキーと重なるなら開始位置をずらす
		if (*x+*wx>cx) *x = cx - *wx -gCxw;
	}
}


//==============================================================
// 変換ラインの表示
//--------------------------------------------------------------
// blk 変換対象の文字数
//--------------------------------------------------------------
// gCx,gCyで指定されているカーソル位置に変換ライン文字列の表示を行う。
// ソフトキーに掛からないように位置補正されます。
// blkで指定された文字数分だけ先頭部分の背景色を変えます。
//--------------------------------------------------------------

static void InLine(char *str,int blk)
{
	char	buf[64];
	int	x,wx,wx1,wx2,blkLen;

	InLinePos( str,&x,&wx );									// 変換ライン位置
	if (wx){
		strncpy( buf,str,blk );
		buf[blk] = '\0';
		blkLen = GetStrWidth( gFont,buf );
		if (blkLen >= wx){
			wx1 = wx;
			wx2 = 0;
		} else {
			wx1 = blkLen;
			wx2 = wx - wx1;
		}
		if (wx1){
			BoxFill( x,gPage+gCy,wx1,gCyw,CORCHCU,CORCHCU );	// 文字背景（変換中部位）
		}
		if (wx2){
			BoxFill( x+wx1,gPage+gCy,wx2,gCyw,CORCHBK,CORCHBK );	// 文字背景（未変換部位）
		}
		pf_print2( x,gPage+gCy,gFont,str,CORCHCR );				// 変換ライン文字列
	}
}


//==============================================================
// 変換ラインの背景の待避
//--------------------------------------------------------------

static void getInLine(char *str)
{
	int	x,wx;

	if (gBDFlag) return;

	if (gInLnBuf){												// バッファを解放
		DrawImgFree(gInLnBuf);
		gInLnBuf = NULL;
	}

	InLinePos( str,&x,&wx );									// 変換ライン位置
	gInLnBuf = DrawImgCreate( wx,16 );
	gInLnBufP.x = x;
	gInLnBufP.y = gCy;
	BoxCopy( gInLnBuf, gInLnBufP.x, gPage+gInLnBufP.y );
}


//==============================================================
// 変換ラインの背景の復元
//--------------------------------------------------------------

static void putInLine(void)
{
	if (gBDFlag) return;

	BoxPaste( gInLnBuf, gInLnBufP.x, gPage+gInLnBufP.y );
}


//==============================================================
// 文字カーソルの作画
//--------------------------------------------------------------

static void DrawCursorSub()
{
	int x,cx,cy;

	if (gCxw==0 || gCyw==0) return;								// カーソルの大きさが0

	if (gKey==2){												// ソフトキーの位置
		cx = KBPOSX3;
		cy = KBPOSY3;
	} else {
		cx = KBPOSX1;
		cy = KBPOSY1;
	}

	x = gCx;
	if (gCy+gCyw>cy && gMode==1){								// ソフトキーと重なるなら開始位置をずらす
		if (x+gCxw>cx) x = cx - gCxw;
	}

	XFill( x, gPage+gCy, gCxw, gCyw, 0xFFFFFF );
}


//==============================================================
// 文字カーソルを表示
//--------------------------------------------------------------
// ch -1  ：ウィンドウ系とカーソルを消去
//    0   ：カーソルを点滅させる
//    以外：カーソルを消去/表示しない
//--------------------------------------------------------------
// SIMEgetchar()単体ではカーソル表示が行われないため、カーソルを表示したい場合
// にこれを使用する。
// 例)
//   ch = SIMEgetchar(pad1);
//   SIMEDrawCursor(ch);
//
// また、キー操作とは別にウィンドウ等の表示系を消したい場合に-1を指定する。
//--------------------------------------------------------------

void SIMEDrawCursor(int ch)
{
	static int bk = 0,count = 0;

	if (ch==-1 && gMode){										// ウィンドウ系を消去
		putBack();
		if (gCount){
			putkList();
		}
		putInLine();
		gGetFlag = 1;
	}

	if (ch){
		if (bk && !gBDFlag) DrawCursorSub();					// キー入力があったらカーソルを消す
		bk = 0;
		count = CBTIME;											// 次回は直ぐ表示させる
	} else {
		count++;
		if (count>CBTIME){										// カーソル点滅周期
			count = 0;
			bk ^= 1;
		}
		if (gBDFlag){
			if (bk) DrawCursorSub();
		} else {
			if (!count) DrawCursorSub();
		}
	}
}


//==============================================================
// 移動モード時の処理
//--------------------------------------------------------------

static void move(char *str,SceCtrlData pad1)
{
	int		ch;

	ch = SIMEgetchar(pad1);
	SIMEDrawCursor(ch);											// カーソルの作画
	if (ch==0x02){												// △ （ソフトキーモードへ）
		ch = 0;
		gMode = 3;												// ソフトキーモード遷移準備
	} else {
		str[0] = ch;
		str[1] = 0;
	}
}


//==============================================================
// シフトJISの第１文字チェック
//--------------------------------------------------------------
// 戻り値  0:第１文字ではない
//        -1:第１文字である
//--------------------------------------------------------------

int chkSJIS(unsigned char cr)
{
	if (cr<0x80U || (cr>=0xA0U && cr<0xE0U) || cr>=0xFDU){
		return (0);
	} else {
		return (-1);
	}
}


//==============================================================
// 辞書の頭文字インデックスを取得
//--------------------------------------------------------------
// 漢字辞書は頭文字ごとに候補群を分割（85個に分けられる）して管理しています。
//（基本辞書、ユーザー辞書ともに）
// 指定された文字がどの候補群に対応するかを調べます。
//--------------------------------------------------------------

static int dicIndex(char *str)
{
	unsigned int	cr;
	int				hp;

	if (chkSJIS(str[0])){										// 先頭が全角文字なら
		cr = fcode(str);										// 先頭文字
		if (cr<0x8200U){										// 記号なら
			hp = 0;
		} else if (cr<0x829FU){									// 数字とアルファベット
			hp = -3;
		} else if (cr<0x82F2U){									// ひらがななら
			hp = cr - 0x829FU +1;
		} else if (cr==0x8394U){								// 「ヴ」
			hp = 84;
		} else {
			hp = -1;											// 漢字検索を行わない
		}
	} else {													// 先頭が半角なら
		hp = -2;
	}
	return (hp);
}


//==============================================================
// ユーザー辞書から候補を検索する
//--------------------------------------------------------------
// count    単語候補の数
// wordList 単語リスト（char wordList[50][2][33]の配列を指定すること）
// 戻り値   count   :単語リストの件数（１～）
//          wordList:単語リスト
//--------------------------------------------------------------
// gStと辞書の「よみ」を比較し、不一致文字が出ない限り候補とする。
//--------------------------------------------------------------

static void setUserKanji(char wordList[][2][33],int *count)
{
	struct strUsDic	*usdic;
	int		hp,pos;

	hp = dicIndex( gSt );										// 辞書の頭文字インデックスを取得
	if (hp<0) return;											// 辞書登録対象外なので終了
	if (gUsDic[hp]==NULL) return;								// 登録されている「候補」が無いので終了

	usdic = gUsDic[hp];
	do{
		pos = 0;
		while (gSt[pos]!='\0'){
			if (gSt[pos]!=usdic->yomi[pos]) break;
			pos++;
		}
		if (gSt[pos]=='\0'){									// 「候補」発見
			strcpy( wordList[(*count)  ][0],usdic->yomi );		// 「よみ」後で優先順位を入れ替える時に使用
			strcpy( wordList[(*count)++][1],usdic->kouho );		// 「候補」
		}
		usdic = usdic->chain;
	}while (usdic && *count<KMAX-1);
}


//==============================================================
// ユーザー辞書に候補を追加する
//--------------------------------------------------------------
// ユーザー辞書は「よみ」「単語」を１セットにして単方向チェイン構造で管理しています。
// 「よみ」の頭文字ごとに候補群を分割して検索時の手間を省いてます。
// メモリが取得出来なかった場合は何もしません。
// 追加された/追加しようとした候補は先頭位置に置かれるので、変換時にはよく使う
// 候補ほど前の方に来ます。
//--------------------------------------------------------------

//----- 候補をユーザー辞書に追加する -----

static void dicAddSub(int hp,char *yomi,char *kouho,struct strUsDic *usdic)
{
	struct strUsDic	*usdic2;

	usdic2 = (struct strUsDic*) malloc( sizeof(struct strUsDic) );
	if (usdic2){												// メモリが取得出来たなら
		usdic2->chain = usdic;
		strcpy( usdic2->yomi,yomi );
		strcpy( usdic2->kouho,kouho );
		gUsDic[hp] = usdic2;
	}
	gUSaveFlag = -1;											// ユーザー辞書に変化があった
}

//----- メイン -----

static void dicAdd(char *yomi,char *kouho)
{
	struct strUsDic	*usdic,*usdic2;
	int		hp,flag;

	hp = dicIndex( yomi );										// 辞書の頭文字インデックスを取得
	if (hp<0) return;											// 辞書登録対象外なので終了

	if (gUsDic[hp]==NULL){										// 頭文字に対応する「候補」群が無いので新規追加
		dicAddSub( hp, yomi, kouho, NULL );						// 「候補」群はここで終わり
	} else {													// 頭文字に対応する「候補」群が有る場合
		usdic = gUsDic[hp];
		usdic2 = NULL;
		flag = 0;
		do{
			if (strcmp(usdic->yomi,yomi)==0 && strcmp(usdic->kouho,kouho)==0){
				flag = 1;										// 既に登録済みだった
				break;
			}
			usdic2 = usdic;
			usdic = usdic->chain;								// 次の「候補」へ
		}while (usdic);
		if (flag){												// 登録済み「候補」を先頭位置に移動
			if (usdic2){										// 「候補」が一つしかない場合は何もしない
				usdic2->chain = usdic->chain;
				usdic2 = gUsDic[hp];
				gUsDic[hp] = usdic;
				usdic->chain = usdic2;
				gUSaveFlag = -1;								// ユーザー辞書に変化があった
			}
		} else {												// 新しい「候補」を先頭位置に追加
			if (!gUDicAdd){										// 候補を追加する設定なら
				dicAddSub( hp,yomi, kouho, gUsDic[hp] );
			}
		}
	}
}


//==============================================================
// ユーザー辞書から候補を削除する
//--------------------------------------------------------------
// 指定された「よみ」「候補」の組み合わせが見つからない時は何もしません。
//--------------------------------------------------------------

static void dicDel(char *yomi,char *kouho)
{
	struct strUsDic	*usdic,*usdic2;
	int		hp,flag;

	hp = dicIndex( yomi );										// 辞書の頭文字インデックスを取得
	if (hp<0) return;											// 辞書登録対象外なので終了

	if (gUsDic[hp]){
		usdic = gUsDic[hp];
		usdic2 = NULL;
		flag = 0;
		do{
			if (strcmp(usdic->yomi,yomi)==0 && strcmp(usdic->kouho,kouho)==0){
				flag = 1;
				break;
			}
			usdic2 = usdic;
			usdic = usdic->chain;								// 次の「候補」へ
		}while (usdic);
		if (flag){												// 目的の「候補」を発見したら
			if (usdic2){										// ２番目以降の場合
				usdic2->chain = usdic->chain;
			} else {											// 最初の一個目の場合
				gUsDic[hp] = usdic->chain;
			}
			free(usdic);
			gUSaveFlag = -1;									// ユーザー辞書に変化があった
		}
	}
}


//==============================================================
// 次の入力文字候補を調べる
//--------------------------------------------------------------
// 戻り値                 文字候補の数
// str[4][3]              次に使用されている頻度が高い上位２個の文字と場合によっては「゛」
// wordList[KMAX][2][33] 「よみ」候補リスト
// count                  「よみ」候補リストの個数（最大KMAX）
//--------------------------------------------------------------
// これまでに変換行に入力された文字を元に辞書を調べて入力中と思われる語句の候補
// を取得し文字入力のアシストを行う。
// ひらがなの入力中の場合にのみ動作します。
// 次のよみに使われる頻度の高い上位２個の「かな」を選択し、また直前に入力されて
// いる「かな」に濁音等が付加可能なら更に「゛」を選択します。
//--------------------------------------------------------------

static int nextChr(char str[][3],char wordList[][2][33],int *count)
{
	char	sTbl1[] = "かきくけこさしすせそたちつてとはひふへほ",
			sTbl2[] = "がぎぐげござじずぜぞだぢづでどばびぶべぼ",
			sTbl3[] = "ぱぴぷぺぽ";
	unsigned int	cr,p[2];
	int		i,hp,sp,sp2,tmp,max[2],len,crCnt,cc[85];
	long	pos;

	cr = 0;

	if (chkSJIS(gSt[0])){										// 先頭が全角文字なら
		cr = fcode(&gSt[0]);									// 先頭文字
		if (cr<0x829FU){										// 記号、数字、英語なら
			return (0);
		} else if (cr<0x82F2U){									// ひらがななら
			hp = cr - 0x829FU +1;
		} else if (cr==0x8394U){								// 「ヴ」
			return (0);
		} else {
			return (0);											// 漢字検索を行わない
		}
	} else {													// 先頭が半角なら
		return (0);
	}

	pos = gHEDtbl[hp];											// 頭文字に対応する辞書レコードの開始位置
	if (pos==0xFFFFFFFF) return (0);							// 辞書に登録なし

	pos += 2;
	sp2 = 0;
	len = strlen(gSt);
	for (i=0; i<85 ;i++){
		cc[i] = 0;
	}
	while(1){
		sp = 0;
		while (gDic[pos+sp]!=0){								// よみに対応する単語を辞書より検索
			if (gDic[pos+sp]!=gSt[sp]) break;
			sp++;
			if (sp==len) break;
		}
		if (sp==len){
			if (*count<KMAX){									// 「よみ」候補リストの取得
				wordList[*count][0][0] = '\0';					// 基本辞書なので「よみ」は使わない
				strcpy( wordList[*count][1],&gDic[pos] );		// 「候補」
				(*count)++;
			}
			cr = fcode(&gDic[pos+sp]);
			if (cr==0x815B){									// 「ー」
				cr = 84;
			} else {
				cr -= 0x829F;
			}
			if (cr<85) cc[cr]++;								// 次の文字候補の該当数をカウント
		}
		if (sp<sp2){											// 一致する文字数が前回より短くなったら探査終了
			break;
		}
		sp2 = sp;
		tmp = getInt(&gDic[pos-2]);								// 次の単語候補への相対距離（int）
		if (tmp==0xFFFF) break;									// 辞書が終了したら探査終了
		pos += tmp;
	}

	max[0] = max[1] = 0;
	for (i=0; i<85 ;i++){										// 出現頻度の高い２個を探索
		if (cc[i]>=max[0]){
			max[1] = max[0];
			p[1] = p[0];
			max[0] = cc[i];
			p[0] = i;
		}
	}
	crCnt = 0;
	for (i=0; i<2 ;i++){
		if (max[i]){
			if (p[i]<84){
				p[i] += 0x829F;
			} else {
				p[i] = 0x815B;									// 「ー」
			}
			str[i][0] = p[i] / 0x100;
			str[i][1] = p[i] & 0xFF;
			str[i][2] = '\0';
			if (strstr( sTbl2,str[i] )!=NULL){					// 「が」→「か」変換
				str[i][1]--;
			}
			if (strstr( sTbl3,str[i] )!=NULL){					// 「ぱ」→「は」変換
				str[i][1] -= 2;
			}
			crCnt++;
		}
	}

	if (len>=2){
		len -= 2;												// 変換行最後の文字
		if (strstr( sTbl1,&gSt[len] )!=NULL){					// 前回濁音とかが付く文字が入力されているなら
			str[crCnt][0] = 0x81;
			str[crCnt][1] = 0x4A;								// 「゛」
			str[crCnt][2] = '\0';
			crCnt++;
		}
	}

	return (crCnt);
}


//==============================================================
// カーソルを指定文字位置に移動させる
//--------------------------------------------------------------
// reset カーソル位置の修正動作 0:入力アシストする 1:フォーメーションリセット 2:アシストしない
//--------------------------------------------------------------
// アシストカーソル専用。
// 辞書から変換候補を探索し、カーソル位置のアシストを行う。
//--------------------------------------------------------------

static void setCur(int *x,int *y,int bt,int *xx,int *yy,int reset,char wordList[][2][33],int *count)
{
	static int lock = 0;
	char	nchr[4][3];
	int		i,j,pos,flag,crCnt,zx,zy;
	int		dx[3] = {0,4,9},dy[3] = {0,2,4};

	for (i=0; i<3 ;i++){										// 移動元を保管
		xx[i] = x[i];
		yy[i] = y[i];
	}
	if (lock || reset){											// カーソルフォーメーションのリセット
		zx = x[bt];
		zy = y[bt];
		for (i=0; i<3 ;i++){
			x[bt] = zx + dx[i];
			if (x[bt]>12) x[bt] -= 13;
			y[bt] = zy + dy[i];
			if (y[bt]>5) y[bt] -= 6;
			bt++;
			if (bt>2) bt = 0;
		}
		lock = 0;
	}

	*count = 0;
	if (!gUSave){
		setUserKanji( wordList,count );							// ユーザー辞書検索
	}
	crCnt = nextChr( nchr,wordList,count );						// 次の入力文字候補を調べる
	if (crCnt){													// 文字候補があるなら
		for (j=0; j<crCnt ;j++){
			pos = -1;
			for (i=0; i<78 ;i++)
				if (gKeyTable1[0][i*2]==nchr[j][0] && gKeyTable1[0][i*2+1]==nchr[j][1]){
					pos = i;									// 文字の位置を特定
					break;
				}
			if (pos!=-1){										// 文字位置を対応する座標に変換
				zx = pos / 6;
				zy = pos % 6;
				flag = 1;
				for (i=0; i<3 ;i++){							// 重複チェック
					if (x[i]==zx && y[i]==zy) flag = 0;
				}
				if (flag && !reset){							// 重複していないなら
					x[bt] = zx;
					y[bt] = zy;
				}
				bt++;
				if (bt>2) bt = 0;
			}
		}
		lock = 1;
	}
}


//==============================================================
// 変換行に文字を追加する
//--------------------------------------------------------------
// ただ追加するだけではなく、濁音等の付加処理などもしています。
//--------------------------------------------------------------

static void addInline(char *str,char *si,int t)
{
	char	sdt[] = "かきくけこさしすせそたちつてとはひふへほカキクケコサシスセソ\タチツテトハヒフヘホ",	// 「゛」が付く文字
			sht[] = "はひふへほハヒフヘホ";																	// 「゜」が付く文字
	char	cr[3] = "  ";
	int		pos,len;

	if (!chkSJIS(si[0])) si[1] = '\0';
	if (gSt[0]!=0){												// ２文字目以降
		putInLine();											// 変換ライン背景を復元
		pos = strlen(gSt) - 2;
		cr[0] = gSt[pos];
		cr[1] = gSt[pos+1];
		if (strcmp( si,"゛" )==0){
			if ((unsigned char)cr[0]==0x83 &&
			    (unsigned char)cr[1]==0x45){					//「ウ」→「ヴ」変換
				gSt[pos+1] = 0x94;
				si[0] = 0;
				si[1] = 0;
			} else {
				if (strstr( sdt,cr )!=NULL){					// 「゛」の付く文字なら前の文字を修正
					gSt[pos+1]++;
					si[0] = 0;
					si[1] = 0;
				}
			}
		}else if (strcmp( si,"゜" )==0){
			if (strstr( sht,cr )!=NULL){						// 「゜」の付く文字なら前の文字を修正
				gSt[pos+1] += 2;
				si[0] = 0;
				si[1] = 0;
			}
		}
	} else {													// １文字目
		if (t==2 || t>4 || si[0]==0x7F || (unsigned char)si[0]<32){	// 半角の場合
			str[0] = si[0];										// そのまま出力
			str[1] = 0;
			si[0] = 0;
		}
	}

	if (si[1]){													// 全角文字を追加する場合
		len = CHLEN -2;
	} else {													// 半角文字を追加する場合
		len = CHLEN -1;
	}
	if (strlen(gSt)<=len){										// 変換ライン文字数制限
		strcat( gSt,si );										// 文字を追加
		getInLine(gSt);											// 変換ライン背景の取得
	}
}


//==============================================================
// ソフトキーモード時の処理（５０音配列アシストカーソル）
//--------------------------------------------------------------

static void softkey1(char *str,SceCtrlData pad1)
{
	static char	wordList[KMAX][2][33];
	static int	t = 0,bt = 0,pc = 0,index = 0,
				x[3] = {0,4,9},y[3] = {1,3,5},xx[3],yy[3];
	int		i,p,s,ch,fp,pos;
	char	si[3];

	fp = 0;														// 画面に変更があったか
	si[2] = 0;

	ch = SIMEgetchar(pad1);
	if (!strlen(gSt)){
		if (ch==0 || ch>=32 || ch==0x02 || ch==0x08 || ch==0x0D || ch==0x1B){
			SIMEDrawCursor(ch);									// カーソル表示
		}
	}
	switch (ch){
	case 0x03:													// L
		break;
	case 0x04:													// R
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			t++;												// 文字種変更
			if (t>6) t = 0;
		} else {
			setCur( x,y,bt,xx,yy,1,wordList,&gCount );			// カーソルフォーメーションをデフォルトに
			pc = CURSTEP;
		}
		break;
	case 0x06:													// [SELECT]
		t--;													// 文字種変更
		if (t<0) t = 6;
		break;
	case 0x05:													// [START]
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount){
				if (strlen(wordList[index][0])){				// [L]+[START] ユーザー辞書から項目を削除
					dicDel( wordList[index][0], wordList[index][1] );
					si[0] = '\0';
					si[1] = '\0';
					fp = 1;
				}
			}
		} else {
			if (gSt[0]!=0){
				putInLine();									// 変換ラインを一旦消す
				if (gCount){
					putkList();
				}
			}
			gCount = 0;
			gMode = 5;											// 設定画面へ
		}
		break;
	case 0x02:													// △
		setCur( x,y,bt,xx,yy,1,wordList,&gCount );				// カーソルフォーメーションをデフォルトに
		pc = CURSTEP;
		if (gCount){
			putkList();
		}
		gCount = 0;
		if (gSt[0]==0){
			gMode = 0;
		} else {
			putInLine();										// 変換ラインを一旦消す
			gMode = 2;											// 漢字変換
			strcpy( gYomi,gSt );								// 学習用に「よみ」を保管しておく
			gKouho[0] = '\0';									// 学習用「候補」をクリアしておく
		}
		break;
	case 0x08:													// □
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]!=0){
				putInLine();									// 変換ライン背景を復元
				if (gCount){
					putkList();
				}
				pos = strlen(gSt);
				pos--;
				p = 0;
				while (1){										// 全角文字補正
					s = 1 -chkSJIS(gSt[p]);
					if (p+s>pos) break;
					p += s;
				}
				gSt[p] = 0;
				if (s==2) gSt[p+1] = 0;
				getInLine(gSt);									// 変換ライン背景を取得
				if (t==0){
					setCur( x,y,bt,xx,yy,0,wordList,&gCount );	// カーソルを次の文字候補位置へ
				} else {
					setCur( x,y,bt,xx,yy,1,wordList,&gCount );	// カーソルを次の文字候補位置へ
				}
				index = 0;
				getkList( wordList,gCount );
			} else {
				str[0] = ch;									// バックスペースを出力
				SIMEDrawCursor(ch);
			}
		} else {
			si[0] = gKeyTable1[t][(x[1]*6+y[1]) *2];
			si[1] = gKeyTable1[t][(x[1]*6+y[1]) *2+1];
			bt = 1;
			fp = 1;
		}
		break;
	case 0x0D:													// ○
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount==0){
				setCur( x,y,bt,xx,yy,1,wordList,&gCount );		// カーソルフォーメーションをデフォルトに
				pc = CURSTEP;
				if (gCount){
					putkList();
				}
				gCount = 0;
				if (gSt[0]==0){
					str[0] = ch;								// [Enter]を出力
					SIMEDrawCursor(ch);
				} else {
					putInLine();								// 変換ライン背景を復元
					for (i=0; i<CHLEN ;i++){
						str[i] = gSt[i];
						gSt[i] = 0;
					}
				}
			} else {
				if (gCount && gCount<=YOMILMAX){
					putInLine();								// 変換ライン背景を復元
					putkList();
					strcpy( str,wordList[index][1] );
					if (strlen(wordList[index][0])){			// ユーザー辞書記載候補なら
						dicAdd( wordList[index][0], wordList[index][1] );	// 記載位置を最初に移動
					}
					gSt[0] = '\0';
					gCount = 0;
				}
			}
		} else {
			si[0] = gKeyTable1[t][(x[0]*6+y[0]) *2];
			si[1] = gKeyTable1[t][(x[0]*6+y[0]) *2+1];
			bt = 0;
			fp = 1;
		}
		break;
	case 0x1B:													// ×
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			setCur( x,y,bt,xx,yy,1,wordList,&gCount );			// カーソルフォーメーションをデフォルトに
			pc = CURSTEP;
			if (gCount){
				putkList();
			}
			gCount = 0;
			if (gSt[0]==0){
				gMode = 0;
			} else {
				putInLine();									// 変換ライン背景を復元
				for (i=0; i<CHLEN ;i++){
					gSt[i] = 0;
				}
			}
		} else {
			si[0] = gKeyTable1[t][(x[2]*6+y[2]) *2];
			si[1] = gKeyTable1[t][(x[2]*6+y[2]) *2+1];
			bt = 2;
			fp = 1;
		}
		break;
	case 0x1C:													// →
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			} else {
				if (gCount && gCount<=YOMILMAX){
					putInLine();								// 変換ライン背景を復元
					putkList();
					strcpy( gSt,wordList[index][1] );
					getInLine(gSt);								// 変換ライン背景を取得
					gCount = 0;
				}
			}
		} else {
			for (i=0; i<3 ;i++){
				x[i]++;
				if (x[i]>12) x[i] = 0;
			}
		}
		break;
	case 0x1D:													// ←
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			}
		} else {
			for (i=0; i<3 ;i++){
				x[i]--;
				if (x[i]<0) x[i] = 12;
			}
		}
		break;
	case 0x1E:													// ↑
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			} else {
				if (gCount){
					index--;
					if (index<0) index = gCount -1;
				}
			}
		} else {
			for (i=0; i<3 ;i++){
				y[i]--;
				if (y[i]<0) y[i] = 5;
			}
		}
		break;
	case 0x1F:													// ↓
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			} else {
				if (gCount){
					index++;
					if (index>gCount-1) index = 0;
				}
			}
		} else {
			for (i=0; i<3 ;i++){
				y[i]++;
				if (y[i]>5) y[i] = 0;
			}
		}
		break;
	default:													// その他（文字の直接入力？）
		if (ch>=32){
			si[0] = ch;
			fp = 1;
		}
		break;
	}

	if (fp){													// 文字を変換行に追加
		if (gCount){
			putkList();
		}
		addInline( str,si,t );
		if (pad1.Buttons & PSP_CTRL_RTRIGGER || gKey==1){		// [R]を押しているかアシストOFFなら
			setCur( x,y,bt,xx,yy,2,wordList,&gCount );			// カーソル位置はそのまま
		} else {
			setCur( x,y,bt,xx,yy,0,wordList,&gCount );			// カーソルを次の文字候補位置へ
		}
		getkList( wordList,gCount );
		pc = CURSTEP;
		index = 0;
	}

	if (str[0]){												// 文字を出力する場合はウィンドウを一時的に消去
		putBack();
		if (gCount){
			putkList();
		}
		gGetFlag = 1;
	} else {
		if (gGetFlag){											// 前回ウィンドウを消去していた（メイン側で作画してるかも）なら背景を再取得
			getBack();
			getkList( wordList,gCount );
			getInLine(gSt);										// 変換ライン背景を取得
			gGetFlag = 0;
		}
		if (gMode==1){											// 他モードに遷移するときは実行しない
			InLine( gSt,0 );									// 変換ライン表示
			if (pad1.Buttons & PSP_CTRL_LTRIGGER){
				if (gCount<=YOMILMAX){
					kList( wordList,gCount,index );				// 「かな」候補リスト表示
				}
				DrawKey1( NULL,NULL,t,xx,yy,pc );				// ソフトキー表示
			} else {
				if (gCount<=YOMILMAX){
					kList( wordList,gCount,-1 );				// 「かな」候補リスト表示
				}
				DrawKey1( x,y,t,xx,yy,pc );						// ソフトキー表示
			}
			pc--;
			if (pc<0) pc = 0;
		} else {
			putBack();											// ソフトキーを消去
			if (gCount){
				putkList();
			}
		}
	}
}


//==============================================================
// ソフトキーモード時の処理（×５かな配列）
//--------------------------------------------------------------

static void softkey3(char *str,SceCtrlData pad1)
{
	static char	wordList[KMAX][2][33];
	static int	x = 0,y = 0,t = 0,index = 0,
				xd[4],yd[4],xx[4],yy[4];
	int		i,p,s,ch,fp,pos;
	char	si[3];

	fp = 0;														// 画面に変更があったか
	si[2] = 0;

	ch = SIMEgetchar(pad1);
	if (!strlen(gSt)){
		if (ch==0 || ch>=32 || ch==0x02 || ch==0x08 || ch==0x0D || ch==0x1B){
			SIMEDrawCursor(ch);										// カーソル表示
		}
	}
	switch (ch){												// カーソルキー入力
	case 0x02:													// △
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount){
				putkList();
			}
			gCount = 0;
			if (gSt[0]==0){
				gMode = 0;
			} else {
				putInLine();									// 変換ラインを一旦消す
				strcpy( gYomi,gSt );							// 学習用に「よみ」を保管しておく
				gKouho[0] = '\0';								// 学習用「候補」をクリアしておく
				gMode = 2;
			}
		} else {
			si[0] = gKeyTable3[t][(y*30+x*5+0) *2];
			si[1] = gKeyTable3[t][(y*30+x*5+0) *2+1];
			fp = 1;
		}
		break;
	case 0x04:													// R
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			t++;												// 文字種変更
			if (t>6) t = 0;
		} else {
			si[0] = gKeyTable3[t][(y*30+x*5+2) *2];
			si[1] = gKeyTable3[t][(y*30+x*5+2) *2+1];
			fp = 1;
		}
		break;
	case 0x06:													// [SELECT]
		t--;													// 文字種変更
		if (t<0) t = 6;
		break;
	case 0x05:													// [START]
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount){
				if (strlen(wordList[index][0])){				// [L]+[START] ユーザー辞書から項目を削除
					dicDel( wordList[index][0], wordList[index][1] );
					si[0] = '\0';
					si[1] = '\0';
					fp = 1;
				}
			}
		} else {
			if (gSt[0]!=0){
				putInLine();									// 変換ラインを一旦消す
				if (gCount){
					putkList();
				}
			}
			gCount = 0;
			gMode = 5;											// 設定画面へ
		}
		break;
	case 0x08:													// □
		if (pad1.Buttons & PSP_CTRL_LTRIGGER || gKeyTable3[t][(y*30+x*5+1) *2]=='\b'){
			if (gSt[0]!=0){
				putInLine();									// 変換ライン背景を復元
				if (gCount){
					putkList();
				}
				pos = strlen(gSt);
				pos--;
				p = 0;
				while (1){										// 全角文字補正
					s = 1 -chkSJIS(gSt[p]);
					if (p+s>pos) break;
					p += s;
				}
				gSt[p] = 0;
				if (s==2) gSt[p+1] = 0;
				getInLine(gSt);									// 変換ライン背景を取得
				if (t==0){
					setCur( xd,yd,0,xx,yy,0,wordList,&gCount );	// カーソルを次の文字候補位置へ
				} else {
					setCur( xd,yd,0,xx,yy,1,wordList,&gCount );	// カーソルを次の文字候補位置へ
				}
				index = 0;
				getkList( wordList,gCount );
			} else {
				str[0] = ch;									// バックスペースを出力
				SIMEDrawCursor(ch);
			}
		} else {
			si[0] = gKeyTable3[t][(y*30+x*5+1) *2];
			si[1] = gKeyTable3[t][(y*30+x*5+1) *2+1];
			fp = 1;
		}
		break;
	case 0x0D:													// ○
		if (pad1.Buttons & PSP_CTRL_LTRIGGER || gKeyTable3[t][(y*30+x*5+3) *2]=='\r'){
			if (gCount==0){
				if (gCount){
					putkList();
				}
				gCount = 0;
				if (gSt[0]==0){
					str[0] = ch;								// [Enter]を出力
					SIMEDrawCursor(ch);
				} else {
					putInLine();								// 変換ライン背景を復元
					for (i=0; i<CHLEN ;i++){
						str[i] = gSt[i];
						gSt[i] = 0;
					}
				}
			} else {
				if (gCount && gCount<=YOMILMAX){
					putInLine();								// 変換ライン背景を復元
					putkList();
					strcpy( str,wordList[index][1] );
					if (strlen(wordList[index][0])){			// ユーザー辞書記載候補なら
						dicAdd( wordList[index][0], wordList[index][1] );	// 記載位置を最初に移動
					}
					gSt[0] = '\0';
					gCount = 0;
				}
			}
		} else {
			si[0] = gKeyTable3[t][(y*30+x*5+3) *2];
			si[1] = gKeyTable3[t][(y*30+x*5+3) *2+1];
			fp = 1;
		}
		break;
	case 0x1B:													// ×
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount){
				putkList();
			}
			gCount = 0;
			if (gSt[0]==0){
				gMode = 0;
			} else {
				putInLine();									// 変換ライン背景を復元
				for (i=0; i<CHLEN ;i++){
					gSt[i] = 0;
				}
			}
		} else {
			si[0] = gKeyTable3[t][(y*30+x*5+4) *2];
			si[1] = gKeyTable3[t][(y*30+x*5+4) *2+1];
			fp = 1;
		}
		break;
	case 0x1C:													// →
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			} else {
				if (gCount && gCount<=YOMILMAX){
					putInLine();								// 変換ライン背景を復元
					putkList();
					strcpy( gSt,wordList[index][1] );
					getInLine(gSt);								// 変換ライン背景を取得
					gCount = 0;
				}
			}
		} else {
			x++;
			if (x>5) x = 0;
		}
		break;
	case 0x1D:													// ←
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			}
		} else {
			x--;
			if (x<0) x = 5;
		}
		break;
	case 0x1E:													// ↑
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			} else {
				if (gCount){
					index--;
					if (index<0) index = gCount -1;
				}
			}
		} else {
			y--;
			if (y<0) y = 2;
		}
		break;
	case 0x1F:													// ↓
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			} else {
				if (gCount){
					index++;
					if (index>gCount-1) index = 0;
				}
			}
		} else {
			y++;
			if (y>2) y = 0;
		}
		break;
	default:													// その他（文字の直接入力？）
		if (ch>=32){
			si[0] = ch;
			fp = 1;
		}
		break;
	}

	if (fp){													// 文字を変換行に追加
		if (strcmp(si,"換")==0){								// 漢字変換開始
			if (gCount){
				putkList();
			}
			gCount = 0;
			if (gSt[0]==0){
				gMode = 0;
			} else {
				putInLine();									// 変換ラインを一旦消す
				strcpy( gYomi,gSt );							// 学習用に「よみ」を保管しておく
				gKouho[0] = '\0';								// 学習用「候補」をクリアしておく
				gMode = 2;
			}
		} else {
			if (gCount){
				putkList();
			}
			addInline( str,si,t );
			setCur( xd,yd,0,xx,yy,0,wordList,&gCount );			// 「よみ」候補リストの取得のみ使用
			getkList( wordList,gCount );
			index = 0;
		}
	}

	if (str[0]){												// 文字を出力する場合はウィンドウを一時的に消去
		putBack();
		if (gCount){
			putkList();
		}
		gGetFlag = 1;
	} else {
		if (gGetFlag){											// 前回ウィンドウを消去していたなら背景を再取得
			getBack();
			getkList( wordList,gCount );
			getInLine(gSt);										// 変換ライン背景を取得
			gGetFlag = 0;
		}
		if (gMode==1){											// 他モードに遷移するときは実行しない
			InLine( gSt,0 );									// 変換ライン表示
			if (pad1.Buttons & PSP_CTRL_LTRIGGER){
				if (gCount<=YOMILMAX){
					kList( wordList,gCount,index );				// 「かな」候補リスト表示
				}
				DrawKey3( -1,-1,t );							// ソフトキー表示（カーソルなし）
			} else {
				if (gCount<=YOMILMAX){
					kList( wordList,gCount,-1 );				// 「かな」候補リスト表示
				}
				DrawKey3( x,y,t );								// ソフトキー表示
			}
		} else {
			putBack();											// ソフトキーを消去
			if (gCount){
				putkList();
			}
		}
	}
}


//==============================================================
// ソフトキーモード時の処理
//--------------------------------------------------------------
// セレクトされている各配列の処理先に分岐
//--------------------------------------------------------------

static void keyboard(char *str,SceCtrlData pad1)
{
	switch (gKey){
	case 0:														// 50音配列アシストカーソル
	case 1:														// 50音配列
		softkey1( str,pad1 );
		break;
	case 2:														// ×５かな配列
		softkey3( str,pad1 );
		break;
	default:
		gKey = 0;
		break;
	}
}


//==============================================================
// char列から全角文字コードを取得
//--------------------------------------------------------------

static unsigned int fcode(char *str)
{
	if (str[0]=='\0'){
		return (0);
	} else {
		return ((unsigned char)str[0] * 0x100 + (unsigned char)str[1]);
	}
}


//==============================================================
// char列からintデータを取得
//--------------------------------------------------------------

static unsigned int getInt(char *str)
{
	return ((unsigned char)str[0] + (unsigned char)str[1] * 0x100);
}


//==============================================================
// 全角文字種別の切り分け
//--------------------------------------------------------------

static int codeChk(char *str,unsigned int ar1,unsigned int ar2)
{
	unsigned int cr;
	int pos;

	pos = 0;
	while (pos<strlen(str)){
		if (chkSJIS(str[pos])){
			cr = fcode(&str[pos]);
			pos += 2;
		} else {												// 半角を見つけたら即終了
			break;
		}
		if (cr<ar1 || cr>ar2){
			break;												// 範囲外の文字を見つけたら終了
		}
	}
	return (pos);
}


//==============================================================
// 漢字変換準備
//--------------------------------------------------------------
// len      変換対象となる文節長（バイト）
// adjust      0:文節長は指定値に強制固定
//          以外:文節長は指定値より短く変更される場合あり
// count    単語候補の数
// wordList 単語リスト（char wordList[KMAX][2][33]の配列を指定すること）
//
// 戻り値   探査を行った「よみ」の長さ（バイト）
//          count   :単語リストの件数（１～）
//          wordList:単語リスト
//--------------------------------------------------------------
// gStの読みを元に基本辞書を検索し漢字候補を取得する。
// 先にユーザー辞書の候補検索が行われたあとにコレが実行されるので、漢字候補は
// 登録済み候補に追加する形になります。
// wordListの最後には探査を行った文節長の「よみ」そのものが入っている。
// 辞書に一致するものが無かった場合でも最低でも「よみ」だけは返されます。
//--------------------------------------------------------------

static int setkanji(int len,int adjust,int *count,char wordList[][2][33])
{
	unsigned int	cr;
	const char		kazu[21] = {"〇一二三四五六七八九"};
	int		i,j,hp,sp,sp2,sp3,tmp,top,sjis;
	long	pos,pos2;

	cr = 0;
	gDicWordPos = -1;

	hp = dicIndex( &gSt[0] );									// 辞書の頭文字インデックスを取得

	if (hp<0){													// 漢字検索を行わない場合（辞書で対応していない先頭文字）
		if (adjust){
			if (hp==-2){										// 半角の部分を選択
				for (sp=0; sp<strlen(gSt) ;sp++){
					if (chkSJIS(gSt[sp])) break;
				}
			} else {											// 同種の全角文字タイプ毎にまとめる
				cr = fcode(&gSt[0]);							// 先頭文字
				if (cr<0x8259U){								// 数字
					sp = codeChk( gSt,0x824F,0x8258 );
				} else if (cr<0x829BU){							// アルファベット
					sp = codeChk( gSt,0x8260,0x829A );
				} else if (cr<0x8397U){							// カタカナ
					sp = codeChk( gSt,0x8340,0x8396 );
				} else if (cr<0x83D7U){							// ギリシャ
					sp = codeChk( gSt,0x839F,0x83D6 );
				} else if (cr<0x8492U){							// ロシア
					sp = codeChk( gSt,0x8440,0x8491 );
				} else {										// 記号
					sp = codeChk( gSt,0x84A0,0x879F );
				}
			}
		} else {
			sp = len;											// 文字タイプは無視
		}

	} else {													// 漢字変換を行う場合
		pos = gHEDtbl[hp];										// 頭文字に対応する辞書レコードの開始位置
		if (pos==0xFFFFFFFF){									// 辞書に登録なし
			sp = 2;
		} else {												// 辞書に先頭文字が登録されている場合
			pos += 2;
			sp2 = sp3 = 0;
			pos2 = -1;
			while(1){
				sp = 0;
				while (gDic[pos+sp]!=0){						// よみに対応する単語を辞書より検索
					if (gDic[pos+sp]!=gSt[sp]) break;
					sp++;
					if (sp==len) break;
				}
				if (gDic[pos+sp]==0){							// 単語候補発見（ただしもっと長く一致するものがあるかもしれないので探査は終わらない）
					pos2 = pos;
					sp3 = sp;
				}
				if (hp==0){										// 記号の場合の終了条件
					if (fcode(&gDic[pos])>0x829E){
						break;
					}
				} else {										// ひらがなの場合の終了条件
					if (sp<sp2){								// 一致する文字数が前回より短くなったら探査終了
						break;
					}
				}
				sp2 = sp;
				tmp = getInt(&gDic[pos-2]);						// 次の単語候補への相対距離（int）
				if (tmp==0xFFFF) break;							// 辞書が終了したら探査終了
				pos += tmp;
			}

			if (pos2==-1){										// 辞書に載っていない（記号を変換しようとした場合に起こりえる）
				sp = 2;
			} else if (adjust==0 && sp3!=len){					// 指定された文節長に足りない
				sp = len;
			} else {											// 単語リストを取り込む
				sp = sp3;
				pos = pos2 + strlen(&gDic[pos2]) +1;
				gDicWordPos = pos;								// 後で辞書記載順を入れ替えるために
				top = *count;
				*count += gDic[pos++];							// 単語数（byte）
				if (*count>KMAX-1) *count = KMAX-1;				// 単語数制限（バッファオーバーフロー対策）
				for (i=top; i<*count ;i++){						// 単語取り込み
					tmp = strlen(&gDic[pos]);
					if (tmp>32) tmp = 32;						// 単語の長さ制限（バッファオーバーフロー対策）
					sjis = 0;
					for (j=0; j<tmp ;j++){
						wordList[i][1][j] = gDic[pos+j];
						if (sjis){								// 長さ制限に引っかかった場合にゴミが残る恐れがあるので
							sjis = 0;
						} else {
							sjis = chkSJIS(gDic[pos+j]);
						}
					}
					wordList[i][1][j+sjis] = '\0';
					pos += strlen(&gDic[pos])+1;
				}
			}
		}
	}

	if (hp>0 || hp==-3){										// 文字種変換
		tmp = 0;
		for (i=0; i<sp ;i+=2){
			cr = fcode(&gSt[i]);
			if (cr>=0x824F && cr<0x8259){						// 数字を漢数字へ
				wordList[*count][1][i  ] = kazu[(cr-0x824F)*2  ];
				wordList[*count][1][i+1] = kazu[(cr-0x824F)*2+1];
				tmp = 1;
			} else if (cr>=0x829F && cr<0x82F2){
				if (cr>=0x82DE) cr++;							// sJISコードの「ミ」と「ム」の間にある空欄は何だろう？
				cr += 161;										// ひらがな→カタカナ
				wordList[*count][1][i  ] = cr >> 8;
				wordList[*count][1][i+1] = cr & 0x00FF;
				tmp = 1;
			} else {											// その他はそのまま
				wordList[*count][1][i  ] = cr >> 8;
				wordList[*count][1][i+1] = cr & 0x00FF;
				tmp = 1;
			}
		}
		if (tmp){												// コード変換を行っていた場合は
			wordList[*count][1][i] = '\0';
			(*count)++;
		}
	}

	for (i=0; i<sp ;i++){										// 最後に「よみ」そのものを単語リストに追加
		wordList[*count][1][i] = gSt[i];
	}
	wordList[*count][1][i] = '\0';
	(*count)++;

	return (sp);
}


//==============================================================
// 漢字候補リストの位置
//--------------------------------------------------------------
// 変換ラインの下か上に漢字候補リストの位置を設定する。
// 画面端とぶつからない方向に設定します。
//--------------------------------------------------------------

static void kListPos(int *x,int *y,int *wx,int *wy,char wordList[][2][33],int count)
{
	int i,len,max,xl,yl,xx;

	InLinePos( gSt,x,wx );										// x座標取得
	xx = *x;

	max = 0;
	for (i=0; i<count ;i++){
		len = strlen(wordList[i][1]);
		if (len>max) max = len;
	}
	*wx = 2+ max * FONTX +3 +4 +1;
	if (count>KLIST){
		*wy = KLIST * (12+1)+2 -1;
	} else {
		*wy = count * (12+1)+2 -1;
	}

	if (*x+*wx>480) *x = 480 - *wx;								// 画面からはみ出すなら開始位置をずらす
	*y = gCy + gCyw+1;
	if (*y+*wy>272) *y = gCy -1 - *wy;

	if (gMode==1){												// ソフトキーモードならソフトキーを回避する
		if (gKey==2){
			xl = KBPOSX3;
			yl = KBPOSY3;
		} else {
			xl = KBPOSX1;
			yl = KBPOSY1;
		}
		if (*x+*wx>=xl && *y+*wy>=yl){							// ソフトキーに重なる
			if (*x<xl){
				*x = xl - *wx;
			} else {
				*y = gCy -1 - *wy;
				if (*y<0){
					*x = xx - *wx;
					*y = yl -1 -*wy;
				}
			}
		}
	}
}


//==============================================================
// 漢字候補リストの表示
//--------------------------------------------------------------
// gCx,gCyで指定されているカーソル位置の下か上に漢字候補リストの表示を行う。
// index が -1 のときはカーソルを表示しない。
//--------------------------------------------------------------

static void kList(char wordList[][2][33],int count,int index)
{
	int	i,page,len,pos,n,x,y,wx,wy,df;

	if (!count) return;

	kListPos( &x, &y, &wx, &wy, wordList, count );				// 漢字候補リスト位置
	y += gPage;
	BoxFill( x, y, wx-5, wy, CORFL1, CORIN );

	BoxFill( x + wx-1-4-1, y, 5, wy, CORFL1, CORFR );
	if (index>=0){
		df = 1;
	} else {
		df = 0;
		index = 0;
	}
	page = index / KLIST;
	if (count<KLIST){
		pos = 0;
		len = wy -2;
	} else {
		pos = page * (wy-2) * KLIST / count;
		len = (wy-1) * KLIST / count;
	}
	if (len>wy-2) len = wy -2;									// スクロールバーのサイズ補正
	if (pos+len>wy-2) pos = wy-2 -len;							// スクロールバーの位置補正
	for (i=0; i<4 ;i++){										// スクロールバー作画
		VLine( x + wx-2-i, y+1 +pos, len, CORRBAR );
	}

	pos = index % KLIST;
	y += 1;
	x += 1;
	if (df){
		BoxFill( x, y +pos *(12+1), wx-2-4-1, 12, CORCUR, CORCUR );
	}
	n = page * KLIST;
	for (i=0; i<KLIST ;i++){
		if (n>=count) break;
		mh_print( x+2,y,wordList[n][1],CORWCHR );				// 変換ライン文字列
		y += 12 +1;
		n++;
	}
}


//==============================================================
// 漢字候補リストの背景の待避
//--------------------------------------------------------------

static void getkList(char wordList[][2][33],int count)
{
	int	x,y,wx,wy;

	if (gBDFlag) return;

	if (!count){
		gkListBufP.x = -1;
		return;
	}
	if (gkListBuf){												// バッファを解放
		DrawImgFree(gkListBuf);
		gkListBuf = NULL;
	}
	kListPos( &x, &y, &wx, &wy, wordList, count );				// 漢字候補リスト位置
	gkListBuf = DrawImgCreate( wx,wy );
	gkListBufP.x = x;
	gkListBufP.y = y;
	BoxCopy( gkListBuf, gkListBufP.x, gPage+gkListBufP.y );
}


//==============================================================
// 漢字候補リストの背景の復元
//--------------------------------------------------------------

static void putkList(void)
{
	if (gkListBufP.x==-1) return;

	if (gBDFlag) return;

	BoxPaste( gkListBuf, gkListBufP.x, gPage+gkListBufP.y );
}


//==============================================================
// 基本辞書記載順の入れ替え
//--------------------------------------------------------------

static void dicWordEx(int index)
{
	long	pos,pos2;
	char	str[65];
	int		i,n,len;

	if (gDicWordPos==-1) return;								// 位置が指定されていない
	if (gDic[gDicWordPos]<(index+1)) return;					// 登録語数以上を指定している
	if (!index) return;											// 既に先頭位置にある

	gSaveFlag = -1;

	pos = gDicWordPos +1;
	pos2 = pos;
	n = 0;
	while (n<index){											// 目標語句の位置を探す
		pos += strlen(&gDic[pos]) +1;
		n++;
	}
	strcpy(str,&gDic[pos]);										// 先頭に移動させる語句を待避
	len = strlen(&gDic[pos]) +1;								// 移動させるサイズ
	n = pos - pos2;
	pos--;
	for (i=n; i>0 ;i--){										// その他の語句を移動
		gDic[pos+len] = gDic[pos];
		pos--;
	}
	strcpy(&gDic[pos2],str);									// 目標語句を先頭に書き戻す
}


//==============================================================
// 漢字選択モード
//--------------------------------------------------------------

static void change(char *str,SceCtrlData pad1)
{
	static char	wordList[KMAX][2][33],ILstr[64];
	static int	mode = 0,count,len,index,getFlag = 0;
	int	l,p,s,ch,df;

	if (!mode){													// 変換開始直後の初期設定
		count = 0;
		len = setkanji( strlen(gSt),1,&count,wordList );		// 基本辞書検索
		index = 0;
		mode = 1;
		strcpy( ILstr,wordList[index][1] );						// 変換対象
		strcat( ILstr,&gSt[len] );								// 未変換対象
		getInLine(ILstr);										// 初期状態の変換ライン背景を取得
		getkList( wordList,count );
	}

	df = 0;

	ch = SIMEgetchar(pad1);
	switch (mode){
	case 1:														// 漢字選択モード
		switch (ch){
		case 0x03:												// L
			putkList();											// 漢字候補リストを消す
			if (index==0){
				index = count -1;
			} else {
				index -= KLIST;
				if (index<0) index = 0;
			}
			df = 1;
			break;
		case 0x04:												// R
			putkList();											// 漢字候補リストを消す
			if (index==count-1){
				index = 0;
			} else {
				index += KLIST;
				if (index>count-1) index = count -1;
			}
			df = 1;
			break;
		case 0x0D:												// ○
			strcat( gKouho,wordList[index][1] );				// 学習用に、選択された候補を蓄積していく
			strcpy( str,wordList[index][1] );
			strcpy( gSt,&gSt[len] );
			if (strlen(gSt)==0){								// 「よみ」が全て無くなったらソフトキーモードへ
				if (!gUSave){
					dicAdd( gYomi,gKouho );						// ユーザー辞書に項目を追加する
				}
				gMode = 3;
			}
			if ((count-1)!=index){								// 候補リストの最後の語句は「よみ」なので入れ替え処理は行わない
				dicWordEx(index);
			}
			mode = 0;
			break;
		case 0x08:												// □
		case 0x1B:												// ×
			putInLine();										// 変換ラインを消す
			putkList();											// 漢字候補リストを消す
			mode = 0;
			gMode = 3;											// ソフトキーモードへ
			break;
		case 0x1C:												// →
			putInLine();										// 変換ラインを消す
			putkList();											// 漢字候補リストを消す
			len += 1 -chkSJIS(gSt[len]);						// 全角文字補正
			l = strlen(gSt);
			if (len>l) len = l;
			strcpy( ILstr,gSt );
			getInLine(ILstr);
			getkList( wordList,count );
			mode = 2;
			break;
		case 0x1D:												// ←
			putInLine();										// 変換ラインを消す
			putkList();											// 漢字候補リストを消す
			len--;
			if (len<1) len = 1;
			p = 0;
			while (1){											// 全角文字補正
				s = 1 -chkSJIS(gSt[p]);
				if (p+s>len) break;
				p += s;
			}
			if (p==0) p = 2;
			len = p;
			strcpy( ILstr,gSt );
			getInLine(ILstr);
			getkList( wordList,count );
			mode = 2;
			break;
		case 0x1E:												// ↑
			putkList();											// 漢字候補リストを消す
			index--;
			if (index<0) index = count -1;
			df = 1;
			break;
		case 0x1F:												// ↓
		case 0x02:												// △
			putkList();											// 漢字候補リストを消す
			index++;
			if (index>count-1) index = 0;
			df = 1;
			break;
		}
		break;
	
	case 2:														// 文節長選択モード
		switch (ch){
		case 0x03:												// L
			break;
		case 0x04:												// R
			break;
		case 0x0D:												// ○
			strncpy( str,gSt,len );
			str[len] = '\0';
			strcat( gKouho,str );								// 学習用に、選択された候補を蓄積していく
			strcpy( gSt,&gSt[len] );
			if (strlen(gSt)==0){								// 「よみ」が全て無くなったらソフトキーモードへ
				putInLine();									// 変換ラインを消す
				if (!gUSave){
					dicAdd( gYomi,gKouho );						// ユーザー辞書に項目を追加する
				}
				gMode = 3;
			}
			mode = 0;
			break;
		case 0x08:												// □
		case 0x1B:												// ×
			putInLine();										// 変換ラインを消す
			mode = 0;
			gMode = 3;											// ソフトキーモードへ
			break;
		case 0x1C:												// →
			len += 1 -chkSJIS(gSt[len]);						// 全角文字補正
			l = strlen(gSt);
			if (len>l) len = l;
			break;
		case 0x1D:												// ←
			len--;
			if (len<1) len = 1;
			p = 0;
			while (1){											// 全角文字補正
				s = 1 -chkSJIS(gSt[p]);
				if (p+s>len) break;
				p += s;
			}
			if (p==0) p = 2;
			len = p;
			break;
		case 0x1E:												// ↑
			count = 0;
			len = setkanji( len,0,&count,wordList );
			index = count -1;
			mode = 1;
			df = 1;
			break;
		case 0x1F:												// ↓
		case 0x02:												// △
			count = 0;
			len = setkanji( len,0,&count,wordList );
			index = 0;
			mode = 1;
			df = 1;
			break;
		}
		break;
	}

	if (df){													// 変換候補に変化があった
		putInLine();											// 変換ラインを消す
		strcpy( ILstr,wordList[index][1] );						// 変換対象
		strcat( ILstr,&gSt[len] );								// 未変換対象
		getInLine(ILstr);
		getkList( wordList,count );
	}

	if (str[0]){												// 文字を出力する場合はウィンドウを一時的に消去
		putInLine();											// 変換ラインを消す
		putkList();												// 漢字候補リストを消す
		getFlag = 1;
	} else {
		if (getFlag){											// 前回ウィンドウを消去していたなら背景を再取得
			getInLine(ILstr);
			getkList( wordList,count );
			getFlag = 0;
		}
		if (gMode==2){											// ソフトキーモードに遷移するときは実行しない
			switch (mode){										// 変換ラインを表示
			case 1:
				InLine( ILstr,strlen(wordList[index][1]) );
				kList( wordList,count,index );
				break;
			case 2:
				InLine(ILstr,len);
				break;
			}
		}
	}
}


//==============================================================
// 設定ウィンドウの作画
//--------------------------------------------------------------

static void DrawMenu(int pos,char *strKey,char *strSave,char *strUSave,char *strUDAdd)
{
	int		y;

	DrawWindow( SIMENAME,MEPOSX,MEPOSY,218,212 );				// ウィンドウフレーム
	y = gPage + MEPOSY +15;
	BoxFill( MEPOSX +1, y, 218-2, 197, CORFL1, CORFR );			// 内側
	y += 6;

	mh_print( MEPOSX +4 +4 , y, "使用している辞書", CORFCHR );
	DrawTextbox( gDicFile,MEPOSX +4 +26,y +12+2,176,CORFCHR,CORFL1 );
	DrawTextbox( gDic2File,MEPOSX +4 +26,y +12+2+12+2,176,CORFCHR,CORFL1 );
	y += 48;

	if (pos==0)
		BoxFill( MEPOSX +4 +4 -1, y, 96 +2, 12, CORCUR, CORCUR );
	mh_print( MEPOSX +4 +4 , y, "ソ\フトキーの種類", CORFCHR );
	DrawTextbox( strKey,MEPOSX +4 +94,y +12+2,108,CORWCHR,CORIN );
	y += 36;

	if (pos==1)
		BoxFill( MEPOSX +4 +4 -1, y, 180 +2, 12, CORCUR, CORCUR );
	mh_print( MEPOSX +4 +4 , y, "終了時に辞書の記載順を保存する", CORFCHR );
	DrawTextbox( strSave,MEPOSX +4 +154,y +12+2,48,CORWCHR,CORIN );
	y += 36;

	if (pos==2)
		BoxFill( MEPOSX +4 +4 -1, y, 132 +2, 12, CORCUR, CORCUR );
	mh_print( MEPOSX +4 +4 , y, "ユーザー辞書を使用する", CORFCHR );
	DrawTextbox( strUSave,MEPOSX +4 +154,y +12+2,48,CORWCHR,CORIN );
	y += 36;

	if (pos==3)
		BoxFill( MEPOSX +4 +4 -1, y, 168 +2, 12, CORCUR, CORCUR );
	mh_print( MEPOSX +4 +4 , y, "ユーザー辞書に候補を追加する", CORFCHR );
	DrawTextbox( strUDAdd,MEPOSX +4 +154,y +12+2,48,CORWCHR,CORIN );
}


//==============================================================
// 設定ウィンドウ背景の待避
//--------------------------------------------------------------

static void getMenu()
{
	if (gBDFlag) return;

	if (gBackBuf){												// バッファを解放
		DrawImgFree(gBackBuf);
		gBackBuf = NULL;
	}
	gBackBuf = DrawImgCreate( 218,214 );						// ソフトキー背景待避バッファ
	gBackBufP.x = MEPOSX;
	gBackBufP.y = MEPOSY;
	BoxCopy( gBackBuf, gBackBufP.x, gPage+gBackBufP.y );
}


//==============================================================
// 各種設定
//--------------------------------------------------------------

static void menu(SceCtrlData pad1)
{
	static int pos = 0,mode = 0,item[4];
	char	strKey[][21] = {"５０音配列AC","５０音配列","×５かな配置"},
			strSave[][7] = {"する","しない"};
	int ch;

	ch = SIMEgetchar(pad1);
	switch (mode){
	case 0:														// 初期設定
		item[0] = gKey;
		item[1] = gSave;
		item[2] = gUSave;
		item[3] = gUDicAdd;
		pos = 0;
		mode = 1;
		break;

	case 1:														// 漢字選択モード
		switch (ch){
		case 0x03:												// L
			break;
		case 0x04:												// R
			break;
		case 0x0D:												// ○
			if (gKey!=item[0] || gSave!=item[1] || gUSave!=item[2]){
				gIni = 1;										// 環境データが変更された
			}
			gKey = item[0];
			gSave = item[1];
			gUSave = item[2];
			gUDicAdd = item[3];
			mode = 0;
			gMode = 3;											// ソフトキーモードへ
			break;
		case 0x08:												// □
			break;
		case 0x05:												// [START]
		case 0x1B:												// ×
			mode = 0;
			gMode = 3;											// ソフトキーモードへ
			break;
		case 0x1C:												// →
			if (pos==0){
				item[pos]++;
				if (item[pos]>2) item[pos] = 2;
			} else {
				item[pos] = 1;
			}
			break;
		case 0x1D:												// ←
			item[pos]--;
			if (item[pos]<0) item[pos] = 0;
			break;
		case 0x1E:												// ↑
			if (pos>0) pos--;
			break;
		case 0x1F:												// ↓
			if (pos<3) pos++;
			break;
		case 0x02:												// △
			break;
		}
		break;
	}

	if (gMode==4){
		DrawMenu( pos, strKey[item[0]], strSave[item[1]], strSave[item[2]], strSave[item[3]] );
	} else {
		putBack();
	}
}


//==============================================================
// 文字列入力
//--------------------------------------------------------------
// *str   入力された文字（最大33バイト asciiz）
// pad1   パッド情報
// 戻り値 strそのもの
//--------------------------------------------------------------
// ソフトキーボードによる文字列の入力。
// かな英数およびIMEによる漢字変換処理も行う。
// 文字だけでなくカーソルキー等の各ボタンもコントロールコードとして返す。
// 入力文字列は（確定された文字がある場合は）strに入っています。
//--------------------------------------------------------------

char *SIMEgets(char *str,SceCtrlData pad1)
{
	int i;

	for (i=0; i<=CHLEN ;i++){
		str[i] = 0;
	}

	switch (gMode){
	case 0:														// 移動モード
		move( str,pad1 );
		break;

	case 1:														// キーボードモード
		keyboard( str,pad1 );
		break;

	case 2:														// 漢字選択モード
		change( str,pad1 );
		break;

	case 3:														// ソフトキーモードに遷移する
		gMode = 1;
		getInLine(gSt);
		getBack();
		keyboard( str,pad1 );
		break;

	case 4:														// 各種設定画面
		menu(pad1);
		break;

	case 5:														// 設定画面に遷移する
		gMode = 4;
		getMenu();
		menu(pad1);
		break;

	}
	return (str);
}


//==============================================================
// 文字入力切り替え
//--------------------------------------------------------------
// ime 入力ソース（0:Simple IME 以外:OSK）
// str 入力された文字列
// pad パッド情報
//--------------------------------------------------------------
// 入力ソースをSimple IMEとOSKで切り替えます。
// OSK使用時に画面イメージを待避するバッファが確保できなかった場合は
// Simple IMEが使用されます。
//--------------------------------------------------------------

void SIMEselInput(int ime,char *str,SceCtrlData pad)
{
	DrawImg	*image;

	if (ime){													// OSK選択時
		image = DrawImgCreate(480,272);
		if (image){
			str[0] = SIMEgetchar(pad);
			SIMEDrawCursor(str[0]);								// SIMEカーソルの作画
			if (str[0]==0x02){									// △を押していた
				BoxCopy( image, 0, 0 );
				oskSetupGu();
				oskInput( "文字入力", "", str );
				initGraphics();
				flipScreen();									// デフォルトではページ設定に不都合があるので
				BoxPaste( image, 0, 0 );
			}
			if (image){
				DrawImgFree(image);
			}
		} else {												// 背景の待避が出来ない場合
			SIMEgets( str, pad );
		}

	} else {													// Simple IME選択時
		SIMEgets( str, pad );									// 入力文字列の取得
	}
}


