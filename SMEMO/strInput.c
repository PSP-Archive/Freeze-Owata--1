//==============================================================
// 文字列入力関連
// STEAR 2009
//--------------------------------------------------------------
// 汎用１ライン文字列入力。
//--------------------------------------------------------------

#include <pspuser.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "smemo.h"
#include "graphics.h"
#include "zenkaku.h"
#include "draw.h"
#include "sime.h"
#include "strInput.h"

#define	LISTMAX		(20)										// 入力履歴に保管する最大値
#define	STRMAX		(128)										// 扱う文字数の最大


//==============================================================
// 入力履歴の全削除
//--------------------------------------------------------------
// inphis 入力履歴
//--------------------------------------------------------------
// 入力履歴用のメモリを解放します。
//--------------------------------------------------------------

void InputHisFree(strInpHis **inphis)
{
	strInpHis	*inphis2,*inphis3;

	if (!inphis) return;
	if (!*inphis) return;

	inphis3 = *inphis;
	while (inphis3){
		inphis2 = inphis3->chain;
		free(inphis3->text);
		free(inphis3);
		inphis3 = inphis2;
	}
	free(inphis);
}

//==============================================================
// 入力履歴の読み込み
//--------------------------------------------------------------
// path   入力履歴ファイル
// 戻り値 入力履歴へのポインタ
//--------------------------------------------------------------
// 入力履歴用のメモリを確保し、指定ファイル内の履歴をセットする。
// ファイルが読めなかった場合はメモリだけ確保します。
// これで入力履歴用のメモリを確保しないと入力履歴機能が使えません。
//--------------------------------------------------------------

strInpHis **InputHisLoad(char *path)
{
	strInpHis	**inphis,*inphis2,*inphis3;
	char	*data;
	int		fd,pos,count;
	long	filesize;

	inphis = (strInpHis**) malloc( sizeof(strInpHis*) );
	if (!inphis) return (NULL);
	*inphis = NULL;

	fd = sceIoOpen( path, PSP_O_RDONLY, 0777 );
	if (fd<0) return (inphis);
	filesize = sceIoLseek(fd, 0, SEEK_END);
	sceIoLseek(fd, 0, SEEK_SET);
	data = (char*) malloc( filesize );							// ファイルサイズはせいぜい1000バイト程度なので
	if (!data){
		sceIoClose(fd);
		return (inphis);
	}
	sceIoRead( fd, data, filesize );
	sceIoClose(fd);

	pos = 0;
	count = 0;
	inphis3 = NULL;
	while (pos<filesize && count<LISTMAX){
		inphis2 = (strInpHis*) malloc( sizeof(strInpHis) );
		if (!inphis2) break;
		inphis2->text = (char*) malloc( strlen(&data[pos])+1 );
		if (!inphis2->text){
			free(inphis2);
			break;
		}
		strcpy( inphis2->text,&data[pos] );
		inphis2->chain = NULL;
		inphis2->save = 0;										// これが1になったら変更あり
		if (!inphis3){											// 最初のリスト項目
			*inphis = inphis2;
		} else {												// ２番目以降のリスト項目
			inphis3->chain = inphis2;
		}
		inphis3 = inphis2;
		pos += strlen(&data[pos]) +1;
		count++;
	}
	free(data);
	return (inphis);
}

//==============================================================
// 入力履歴の保存
//--------------------------------------------------------------
// path   入力履歴保存ファイル名
// inphis 入力履歴
//--------------------------------------------------------------
// 内容に変更があった場合のみ保存します。
//--------------------------------------------------------------

void InputHisSave(char *path,strInpHis **inphis)
{
	strInpHis	*inphis2;
	int		fd;

	if (!inphis) return;
	if (!*inphis) return;

	inphis2 = *inphis;
	if (!inphis2->save) return;									// 履歴に変更がない
	fd = sceIoOpen( path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if (fd<0) return;
	while (inphis2){
		sceIoWrite( fd, inphis2->text, strlen(inphis2->text)+1 );
		inphis2 = inphis2->chain;
	}
	sceIoClose(fd);
}

//==============================================================
// 入力履歴にリストを追加
//--------------------------------------------------------------
// 同じ内容があった場合は二重登録はしない。
// 先頭位置に登録されます。
// リストの数がLISTMAX個を超えた場合は一番古いリストが削除されます。
// メモリが取得出来なかった場合は何もしません。
//--------------------------------------------------------------

//----- リストに追加 -----

static void InputHisAddSub(strInpHis **inphis2,char *text)
{
	strInpHis	*inphis;

	inphis = (strInpHis*) malloc( sizeof(strInpHis) );
	if (inphis){
		inphis->text = (char*) malloc( strlen(text)+1 );
		if (!inphis->text){										// 内容用メモリの確保に失敗
			free(inphis);
		} else {												// 正常にメモリが確保できた
			strcpy( inphis->text,text );
			inphis->chain = *inphis2;
			inphis->save = 1;									// 履歴が変更された
			*inphis2 = inphis;
		}
	}
}

//----- メイン -----

void InputHisAdd(strInpHis **inphis,char *text)
{
	strInpHis	*inphis2,*inphis3;
	int		count;

	if (!inphis) return;
	if (strlen(text)==0) return;

	if (!*inphis){												// 最初の一回目
		InputHisAddSub( inphis, text );
		return;
	}

	inphis2 = *inphis;
	inphis3 = NULL;
	while (inphis2){											// 同じ内容があるか
		if (!strcmp(inphis2->text,text)) break;
		inphis3 = inphis2;
		inphis2 = inphis2->chain;
	}
	if (inphis2){												// 同じ内容があった
		if (inphis3){											// 位置が２番目以降なら
			inphis3->chain = inphis2->chain;					// チェインから外して
			inphis3 = *inphis;
			*inphis = inphis2;
			inphis2->save = 1;									// 履歴に変更があった
			inphis2->chain = inphis3;							// 一番最初へ移動させる
		}
	} else {													// 新規追加
		InputHisAddSub( inphis, text );
		count = 0;
		inphis2 = *inphis;
		while (inphis2 && count<LISTMAX){						// リストの個数を数える
			count++;
			inphis3 = inphis2;
			inphis2 = inphis2->chain;
		}
		if (inphis2){											// 制限数を超えている
			inphis3->chain = NULL;								// 一つ削除
			free(inphis2->text);
			free(inphis2);
		}
	}
}

//==============================================================
// 入力履歴からリスト項目を取得
//--------------------------------------------------------------
// text   取得されたリスト内容（入力履歴）
// size   textのサイズ
// inphis 入力履歴
// index  取り出すリストの位置
// 戻り値 取得されたリストの実際の位置（先頭位置は0）
//--------------------------------------------------------------
// 履歴が無い場合はtextの内容に変化はありません。
// この時の戻り値は-1です。
// 履歴のリスト数を超えた位置を指定された場合、最後のリスト内容が返されます。
//--------------------------------------------------------------

int InputHisGet(char *text,int size,strInpHis **inphis,int index)
{
	strInpHis	*inphis2;
	int		i,pos;

	if (!inphis) return (-1);
	if (!*inphis) return (-1);
	if (index<0) return (-1);

	inphis2 = *inphis;
	for (i=0; i<index ;i++){									// 指定位置を探す
		if (!inphis2->chain) break;								// リストが尽きた
		inphis2 = inphis2->chain;
	}
	for (pos=0; pos<size-1 ;pos++){								// バッファオーバーフロー対策
		text[pos] = inphis2->text[pos];
	}
	text[pos] = '\0';
	return (i);
}


//==============================================================
// 全角/半角判定ありで一文字さがる
//--------------------------------------------------------------

static int INBack(char *str,int pos,int *spos)
{
	int i,pos2,step;

	if (pos>0){
		pos2 = 0;
		while (1){
			if (chkSJIS(str[pos2])){
				step = 2;
			} else {
				step = 1;
			}
			if (pos2+step>=pos) break;
			pos2 += step;
		}
		pos = pos2;
	}

	//----- 表示位置補正 -----

	if (*spos>pos){
		*spos = pos - 6;
		if (*spos<0) *spos = 0;
		for (i=0; i<*spos; i++){								// 全角文字補正
			if (chkSJIS(str[i])) i++;
		}
		*spos = i;
	}

	return (pos);
}

//==============================================================
// 文字列入力
//--------------------------------------------------------------
// title   タイトル文字列
// size    入力欄の文字数（最大50文字まで）
// ime     入力ソース（0:Simple IME 以外:OSK）
// inStr   入力文字列の初期値
// outStr  文字入力結果（128バイト）
// inphis  入力履歴
// 戻り値     0:○ボタンで終了
//         以外:×ボタンでキャンセルされた
//--------------------------------------------------------------
// 汎用１ライン入力です。
// ダイアログ位置は指定できません。
// ○ボタンで終了した場合、outStrに入力文字列が返される。
// ×ボタンで終了した場合、outStrにはinStrが返される。
// ダイアログの背景は復元しないので、必要に応じて呼び出し側で再描写を行ってください。
// 入力欄の幅を超える文字数の場合は横スクロールします。
//--------------------------------------------------------------

int InputName(char *title,int size,int ime,char *inStr,char *outStr,strInpHis **inphis)
{
	SceCtrlData pad;
	char	str[STRMAX],cr[128],text[51];
	int		i,cx,cy,x,pos,spos,spos2,len,p,dr,end,font,index;

	if (size<2) size = 2;
	if (size>50) size = 50;

	font = SIMEfont( 0, 12 );									// SIMEのフォントを東雲フォントへ

	//----- ダイアログ構築 -----

	DialogFrame( 78, 70, 24+300, 57, title, "↑↓:入力履歴  ○:決定  ×:ｷｬﾝｾﾙ", gSCor[1], gSCor[2] );
	x = 78 + 324/2 - (size*6+2)/2;								// 入力欄位置
	BoxFill(  x, 91, size*6+2, 14, 0, 0 );
	x += 1;
	cy = 92;
	strcpy( str, inStr );										// 文字名称初期値
	len = strlen(str);
	pos = len;
	if (pos>=size){												// 入力枠を超える文字数の場合
		spos = pos - 50;
	} else {
		spos = 0;
	}
	memcpy( text, &str[spos], size );
	text[size] = '\0';
	mh_print( x, cy, text, gSCor[0] );

	//----- 入力処理 -----

	end = 1;
	index = -1;
	while (!gExitRequest && end){
		cx = x + (pos - spos) * 6;
		SIMEcursor( 2, cx, cy );								// 文字カーソルの位置指定
		sceCtrlReadBufferPositive( &pad,1 );
		SIMEselInput( ime, cr, pad );
		if (cr[0]!=0){											// 入力があった
			dr = 0;												// 文字列を書き換えるか（0:No）
			if (cr[0]<32U || cr[0]==0x7F){						// コントロールコードだった場合
				switch(cr[0]){
				case SIME_KEY_CIRCLE:							// ○ [Enter]
					InputHisAdd( inphis,str );
					end = 0;
					break;
				case SIME_KEY_CROSS:							// × [ESC]
					end = 0;
					break;
				case SIME_KEY_UP:								// ↑（入力履歴）
					if (index>-1) index--;
					index = InputHisGet( str, STRMAX, inphis, index );
					if (index>=0){
						len = strlen(str);
						pos = len;
						if (pos>=size){							// 入力枠を超える文字数の場合
							spos = pos - 50;
						} else {
							spos = 0;
						}
						dr = 1;
					} else {
						str[0] = '\0';
						len = 0;
						pos = 0;
						spos = 0;
						dr = 1;
					}
					break;
				case SIME_KEY_DOWN:								// ↓（入力履歴）
					index++;
					index = InputHisGet( str, STRMAX, inphis, index );
					if (index>=0){
						len = strlen(str);
						pos = len;
						if (pos>=size){							// 入力枠を超える文字数の場合
							spos = pos - 50;
						} else {
							spos = 0;
						}
						dr = 1;
					}
					break;
				case SIME_KEY_LEFT:								// ←
					spos2 = spos;
					pos = INBack( str, pos, &spos );
					if (spos2!=spos) dr = 1;					// 文字表示位置が変更された
					break;
				case 0x08:										// [BS]
				case 0x7F:										// [DEL]
					if (len>0){
						if (cr[0]==0x08){
							pos = INBack( str, pos, &spos );
						}
						if (chkSJIS(str[pos])){
							strcpy( &str[pos], &str[pos+2] );
							len -= 2;
						} else {
							strcpy( &str[pos], &str[pos+1] );
							len -= 1;
						}
						dr = 1;
					}
					break;
				case SIME_KEY_RIGHT:							// →
					if (chkSJIS(str[pos])){
						pos += 2;
					} else {
						if (pos<len){
							pos++;
						}
					}
					spos2 = spos;
					if (spos+size<pos){
						spos = pos - size + 6;
						if (spos>STRMAX-size) spos = STRMAX - size;
						for (i=0; i<spos; i++){					// 全角文字補正
							if (chkSJIS(str[i])) i++;
						}
						spos = i;
					}
					if (spos2!=spos) dr = 1;					// 文字表示位置が変更された
					break;
				}
			} else {											// 文字だった場合
				p = 0;
				while (cr[p]!='\0'){
					if (chkSJIS(cr[p])){						// 全角文字
						if (len<=STRMAX-1-2){
							for (i=0; i<len-pos ;i++)
								str[len-1+2-i] = str[len-1-i];
							str[pos++] = cr[p++];
							str[pos++] = cr[p++];
							len += 2;
							dr = 1;
						} else {
							break;
						}
					} else if (cr[p]>=32U){						// 半角文字
						if (len<=STRMAX-1-1){
							for (i=0; i<len-pos ;i++)
								str[len-1+1-i] = str[len-1-i];
							str[pos++] = cr[p++];
							len += 1;
							dr = 1;
						} else {
							break;
						}
					} else {									// コントロールコードは無視
						p++;
					}
				}
				str[len] = '\0';
				if (spos+size<pos){
					spos = pos - size + 6;
					if (spos>STRMAX-size) spos = STRMAX - size;
					for (i=0; i<spos; i++){						// 全角文字補正
						if (chkSJIS(str[i])) i++;
					}
					spos = i;
				}
			}
			if (dr){											// 文字列の更新
				BoxClr( x, cy, size*6, 12 );
				memcpy( text, &str[spos], size );
				text[size] = '\0';
				mh_print( x, cy, text, gSCor[0] );
			}
		}

		sceDisplayWaitVblankStart();
		ScreenView();
	}

	SIMEfont(font,(font==2 ? 16 : 12));							// SIMEフォントを元に戻す

	//----- 戻り値 -----

	if (cr[0]==0x0D){											// [Enter]で終了していたら
		strcpy( outStr, str );
		return (0);
	} else {
		strcpy( outStr, inStr );
		return (-1);
	}
}

