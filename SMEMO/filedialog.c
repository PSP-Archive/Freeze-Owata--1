//==============================================================
// ファイルダイアログ関連
// STEAR 2009,2010
//--------------------------------------------------------------
// ファイル選択機能。
//--------------------------------------------------------------

#include <pspuser.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <string.h>
#include <malloc.h>

#include "smemo.h"
#include "graphics.h"
#include "zenkaku.h"
#include "draw.h"
#include "sime.h"
#include "strInput.h"
#include "filedialog.h"

#include "psp2ch.h"
#include "charConv.h"

extern S_2CH s2ch; // psp2ch.c


//==============================================================
// 指定されたディレクトリのエントリ一覧を取得する
//--------------------------------------------------------------
// 戻り値 取得したエントリ数（０なら無し）
//--------------------------------------------------------------

static struct strDList* GetDir(char *DirName,int *count,struct strDList *DList)
{
	static SceIoDirent	dir;									// こうしないとフリーズする
	int 	dfd,ret,DListMax;

	free(DList);
	dfd = sceIoDopen(DirName);
	DListMax = 1;
	if (dfd>=0){
		do{														// エントリー数をカウント
			memset( &dir, 0, sizeof(dir) );						// こうしないとフリーズする
			ret = sceIoDread( dfd, &dir );
			if (strlen(dir.d_name)) DListMax++;
		}while (ret>0);
		sceIoDclose(dfd);
	}
	DList = (struct strDList*) malloc(sizeof(struct strDList) * DListMax);
	if (!DList) return (NULL);									// メモリが確保出来なかった
	DList[DListMax-1].name[0] = '\0';

	(*count) = 0;
	if (DListMax==1) return(0);									// エントリーが無い/ディレクトリが無い

	dfd = sceIoDopen(DirName);
	do{
		DList[*count].name[0] = '\0';
		memset( &dir, 0, sizeof(dir) );							// こうしないとフリーズする
		ret = sceIoDread( dfd, &dir );
		if (strlen(dir.d_name)){								// 名前が無いものは無視
			if (dir.d_stat.st_attr & FIO_SO_IFDIR){
				if (strcmp(dir.d_name,".")!=0 && strcmp(dir.d_name,"..")!=0 ){
					strcpy( DList[*count].name, dir.d_name );
					DList[(*count)++].flag = 0;					// ディレクトリ
				}
			} else {
				strcpy( DList[*count].name, dir.d_name );
				DList[(*count)++].flag = 1;						// ファイル
			}
		}
	}while (*count<DListMax && ret>0);
	sceIoDclose(dfd);
	return (DList);
}

//==============================================================
// ディレクトリ削除
//--------------------------------------------------------------
// path 削除を行うディレクトリへのパス
//--------------------------------------------------------------
// 指定されたディレクトリを内部ファイルも含めて削除する。
// 内部にあるサブディレクトリも削除します。
//--------------------------------------------------------------

static void removeDir(char *path)
{
	SceIoDirent	dir;
	SceUID		fd;
	char		file[1024];

	if ((fd = sceIoDopen(path)) < 0) return;					// 指定ディレクトリを開けなかったので終了
	memset(&dir, 0, sizeof(dir));								// 初期化しないとreadに失敗する
	while (sceIoDread(fd, &dir) > 0){
		if(strcmp(dir.d_name, ".") == 0) continue;
		if(strcmp(dir.d_name, "..") == 0) continue;
		strcpy( file, path );
		strcat( file, "/" );
		strcat( file, dir.d_name );
		if (dir.d_stat.st_attr & FIO_SO_IFDIR){					// ディレクトリか？ファイルか？
			removeDir(file);									// ディレクトリ（再帰処理でサブディレクトリを処理する）
		} else {
			sceIoRemove(file);									// ファイル
		}
	}
	sceIoDclose(fd);
	sceIoRmdir(path);
}

//==============================================================
// ファイル選択の画面作画
//--------------------------------------------------------------
// flag 1:強制作画
//--------------------------------------------------------------
// 作画する必要の無い場合は作画しない。
// １画面は14行表示。
//--------------------------------------------------------------

static void DrawSFsub(struct strDList *DList,int y,int pos)
{
	char	str[256];

	if (s2ch.cfg.hbl){											// HBL環境下ではファイル名のUTF8→シフトJIS変換を行う
		psp2chUTF82Sjis(str, DList[pos+y].name);
	} else {
		strcpy(str, DList[pos+y].name);
	}
	str[63] = '\0';												// 簡易的な文字数制限
	if (DList[pos+y].flag){
		mh_print( 40+11, 40+6+12+4+y*12, str, gSCor[0] );
	} else {
		mh_print( 40+11, 40+6+12+4+y*12, str, gSCor[9] );
	}
}

static void DrawSF(struct strDList *DList,int y,int pos,int max,int flag)
{
	static int	y2,pos2;
	int			i;

	VRollBar( 40+10+380-6, 40+6+12+4, 168, pos, 14, max, flag, gSCor[6], gSCor[7], gSCor[8] );
	if (flag){
		pos2 = -1;
		y2 = -1;
	}
	if (pos!=pos2){												// 文字/カーソルの削除
		Fill( 40+10, 40+6+12+4, 380-6, 168, gSCor[5] );
	} else if (y!=y2){
		Fill( 40+10, 40+6+12+4+y2*12, 380-6, 12, gSCor[5] );
	}

	if ((max && y!=y2) || pos!=pos2){							// エントリがあるならカーソルを表示
		CurveBox( 40+10, 40+6+12+4+y*12, 380-6, 12, 0, gSCor[10], gSCor[11] );
	}

	if (pos!=pos2){												// 画面全体を再描写
		pos2 = pos;
		y2 = y;
		for (i=0; i<14 ;i++){
			if (pos+i>=max) break;
			DrawSFsub( DList, i, pos );
		}
	} else if (y!=y2){											// カーソル位置を再描写
		DrawSFsub( DList, y2, pos2 );							// 前回のカーソル位置
		DrawSFsub( DList, y, pos );								// 今回のカーソル位置
		y2 = y;
	}
}

//==============================================================
// ファイル選択
//--------------------------------------------------------------
// InitDir  初期ディレクトリ位置
// FileName デフォルトファイル名（ファイル名の手入力時のデフォルト名）
// dir      選択されたファイル名（フルパス）
// flag      0:ファイル選択（ファイル読み込み用）
//          -1:ファイル名入力（名前を付けて保存用）
// ime      入力ソース（0:Simple IME  以外:OSK）
// 戻り値    0:正常終了（ファイルが選択された）
//          -1:キャンセルされた
//--------------------------------------------------------------
// ディレクトリ間を移動しファイルを選択する。
// 扱えるエントリ数はメモリの許す限りです。
// キャンセルされた場合、dirの内容は変化しない。
// ファイルを選択した場合、InitDir、dirに値がセットされます。
// ダイアログ背景は復元しないので、必要に応じて呼び出し側で再描写を行ってください。
//--------------------------------------------------------------

//----- 現在のパスを表示 -----

static void DrawSFPath(char *path)
{
	char	str[64];
	int		pos,len;

	pos = 0;
	len = strlen(path);
	if (len>61){
		pos = len - 61;
	}
	strcpy( str, &path[pos] );									// パスから後方61文字を取り出す

	Fill( 40+10, 25+6+12+4, 380-6, 12, gSCor[5] );
	mh_print( 40+11, 25+6+12+4, str, gSCor[0] );
}

//----- ダイアログの構築 -----

static void DrawSFRe(char *title,int y,int pos,int count,struct strDList *DList,char *DirName)
{
	CurveBox( 40, 25, 400, 227, 4, gSCor[1], gSCor[2] );
	mh_print( 40+10, 25+6, title, gSCor[1] );
	mh_print( 40+400-10-54*6, 25+227-6-12, "[START]:ﾌｧｲﾙ削除  ○:選択  ×:ｷｬﾝｾﾙ  △:ファイル名入力", gSCor[1] );
	DrawSFPath( DirName );
	DrawSF( DList, y, pos, count, 1 );
}

//----- 上書きチェック -----

static int DrawSFFile(char *DirName,char *FileName)
{
	char	path[1024];
	int		fd;

	strcpy( path,DirName );
	strcat( path,FileName );
	fd = sceIoOpen( path, PSP_O_RDONLY, 0777 );
	sceIoClose(fd);
	if (fd>=0){
		if (!DialogYN( "指定されたファイルは既に存在しています。\n上書きしますがよろしいですか？", gSCor[3], gSCor[4] ))
			return (0);
	}
	return (-1);
}

//----- ワークメモリの取得に失敗 -----

static void memerr(void)
{
	SceCtrlData		pad;
	DiaBox1( -1, 110, 0, "ワークメモリの取得に失敗しました", gSCor[0], gSCor[3], gSCor[4] );
	ScreenView();												// 画面更新
	while (1){
		sceCtrlReadBufferPositive(&pad, 1);
		if (!(pad.Buttons & 0x00F3F9)) break;					// 操作系ボタンが全て離された
		sceDisplayWaitVblankStart();
	}
	while (!gExitRequest){										// 何か押されるまで待機
		sceCtrlReadBufferPositive( &pad, 1 );
		if (SIMEgetchar(pad)!=0) break;
		sceDisplayWaitVblankStart();
	}
}

//----- メイン処理 -----

int SelectFile(char *title,char *InitDir,char *FileName,char *dir,int flag,int ime,strInpHis **inphis)
{
	SceCtrlData		pad;
	struct strDList	*DList;
	char			*p1,*p2,DirName[1024],backDir[256],outName[64],str[1024];
	int				i,y,padp,pos,ret,count,len,err;
	float			padv;
	unsigned short	ucs[1024];

	strcpy( DirName, InitDir );
	DList = GetDir( DirName, &count, NULL );
	if (!DList){
		memerr();
		return (-1);
	}

	err = 0;
	y = 0;
	pos = 0;
	DrawSFRe( title, y, pos, count, DList, DirName );			// ダイアログ構築

	if (flag){													// ファイル名入力
		if (s2ch.cfg.hbl){										// HBL環境下ではファイル名のUTF8→シフトJIS変換を行う
			psp2chUTF82Sjis(str, FileName);
		} else {
			strcpy(str, FileName);
		}
		if (!InputName( "ファイル名を入力してください",50,ime,str,outName,inphis)){
			if (s2ch.cfg.hbl){									// HBL環境下ではファイル名のシフトJIS→UTF8変換を行う
				psp2chSJIS2UCS(ucs, outName, 1024);
				psp2chUCS2UTF8(outName, ucs);
			}
			if (DrawSFFile(DirName,outName)){					// 上書き警告
				strcpy( dir, DirName );
				strcat( dir, outName );
				free(DList);									// ファイルリストを廃棄
				return (0);										// ○で決定された場合
			}
		}
		DrawSF( DList, y, pos, count, 1 );						// 画面更新
	}

	padp = 0;													// アナログパッドの押し続け監視(横方向のみ)
	padv = 0;													// アナログパッドによる移動量端数(縦方向のみ)
	while (!gExitRequest && !err){								// キー入力メインループ
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		if (pad.Lx<20 && !padp){								// アナログパッド ←
			padp = 1;
			ret = SIME_KEY_LEFT;
		} else if (pad.Lx>256-20 && !padp){						// アナログパッド →
			padp = 1;
			ret = SIME_KEY_RIGHT;
		} else if (pad.Lx>128-40 && pad.Lx<128+40){				// ニュートラルチェック
			padp = 0;
		}
		if (pad.Ly<128-40 || pad.Ly>128+40){					// アナログパッド ↑↓
			padv += (float)(pad.Ly - 128) / 400;
			if (padv<-1){										// ↑
				padv = padv + 1;
				if (pos>0){										// リスト全体を上へスクロール
					pos--;
					DrawSF( DList, y, pos, count, 0 );
				}
			}
			if (padv>1){										// ↓
				padv = padv - 1;
				if (pos<count-14){								// リスト全体を下へスクロール
					pos++;
					DrawSF( DList, y, pos, count, 0 );
				}
			}
		}
		switch (ret){
		case SIME_KEY_CIRCLE:									// ○
		case SIME_KEY_RIGHT:									// →
			if (strlen(DList[pos+y].name)!=0){
				if (DList[pos+y].flag){							// ファイルの場合
					if (ret==SIME_KEY_CIRCLE){					// ファイル選択はあくまで決定キー
						if (!flag || DrawSFFile(DirName,DList[pos+y].name)){	// 上書き警告
							strcpy( InitDir, DirName );
							strcpy( dir, DirName );
							strcat( dir, DList[pos+y].name );
							free(DList);						// ファイルリストを廃棄
							return (0);							// ファイル選択
						} else {
							DrawSF( DList, y, pos, count, 1 );	// 画面更新（警告ダイアログの消去）
						}
					}
				} else {										// ディレクトリの場合
					strcat( DirName, DList[pos+y].name );
					strcat( DirName, "/" );
					DrawSFPath( DirName );
					DList = GetDir( DirName, &count, DList );	// ディレクトリ一覧の取得
					if (DList){
						pos = 0;
						y = 0;
						DrawSF( DList, y, pos, count, 1 );		// 画面更新
					} else {
						err = -1;
					}
				}
			}
			break;
		case SIME_KEY_CROSS:									// ×
			free(DList);										// ファイルリストを廃棄
			return (-1);
			break;
		case SIME_KEY_TRIANGLE:									// △
			if (s2ch.cfg.hbl){									// HBL環境下ではファイル名のUTF8→シフトJIS変換を行う
				psp2chUTF82Sjis(str, FileName);
			} else {
				strcpy(str, FileName);
			}
			if (!InputName( "ファイル名を入力してください",50,ime,str,outName,inphis)){
				if (s2ch.cfg.hbl){								// HBL環境下ではファイル名のシフトJIS→UTF8変換を行う
					psp2chSJIS2UCS(ucs, outName, 1024);
					psp2chUCS2UTF8(outName, ucs);
				}
				if (!flag || DrawSFFile(DirName,outName)){		// 上書き警告
					strcpy( InitDir, DirName );
					strcpy( dir, DirName );
					strcat( dir, outName );
					free(DList);								// ファイルリストを廃棄
					return (0);									// ○で決定された場合
				}
			}
			DrawSF( DList, y, pos, count, 1 );					// 画面更新
			break;
		case 0x08:
		case SIME_KEY_LEFT:										// ←
			p1 = strchr( DirName, '/' );
			p2 = strrchr( DirName, '/' );
			if (p1!=p2){										// 現在位置がルートだった場合は処理しない
				len = strlen(DirName);
				DirName[len-1] = '\0';							// 最後尾の'/'を削除
				p2 = strrchr( DirName, '/' );					// 一番後ろにある'/'の位置
				strcpy( backDir, &p2[1] );						// さっきまで居たフォルダ名を保存
				p2[1] = '\0';									// 一番後ろにある'/'の位置以後を削除
				DrawSFPath( DirName );
				DList = GetDir( DirName, &count, DList );		// ディレクトリ一覧の取得
				if (DList){
					pos = 0;
					y = 0;
					for (i=0; i<count ;i++){					// さっきまで居たフォルダ名の所にカーソルをセット
						if (!strcmp( backDir,DList[i].name )){
							if (i<13){
								y = i;
							} else {
								y = 13;
								pos = i - 13;
							}
						}
					}
					DrawSF( DList, y, pos, count, 1 );			// 画面更新
				} else {
					err = -1;
				}
			}
			break;
		case SIME_KEY_UP:										// ↑
			if (y>0){
				y--;
			} else if (pos>0) pos--;
			DrawSF( DList, y, pos, count, 0 );
			break;
		case SIME_KEY_DOWN:										// ↓
			if (pos+y<count-1){
				if (y<13){
					y++;
				} else {
					pos++;
				}
			}
			DrawSF( DList, y, pos, count, 0 );
			break;
		case SIME_KEY_LTRIGGER:									// [L]
			pos -= 12;
			if (pos<0) pos = 0;
			DrawSF( DList, y, pos, count, 0 );
			break;
		case SIME_KEY_RTRIGGER:									// [R]
			pos += 12;
			if (pos+13>count-1){
				pos = count-1-13;
				if (pos<0) pos = 0;
			}
			DrawSF( DList, y, pos, count, 0 );
			break;
		case SIME_KEY_START:									// [START]
			if (strlen(DList[pos+y].name)!=0){					// ファイル/ディレクトリがあるなら
				strcpy( str, DList[pos+y].name );
				if (strlen(str)>63) str[64] = '\0';
				strcat( str, " を削除します。" );
				if (!DList[pos+y].flag){						// ディレクトリの場合
					strcat( str,"\n注意：このフォルダの内容は全消去されます。" );
				}
				if (DialogYN( str, gSCor[1], gSCor[2] )){		// Yesなら
					strcpy( str, DirName );
					strcat( str, DList[pos+y].name );
					if (!DList[pos+y].flag){					// ディレクトリの場合
						removeDir( str );						// ディレクトリ削除
					} else {
						sceIoRemove( str );						// ファイル削除
					}
					DList = GetDir( DirName, &count, DList );	// ディレクトリ一覧の取得
					if (pos+y > count){							// カーソル位置補正
						if (y>0){
							y--;
						} else {
							if (pos>0) pos--;
						}
					}
				}
				if (DList){
					DrawSF( DList, y, pos, count, 1 );		// 画面更新
				} else {
					err = -1;
				}
			}
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// 画面更新
	}
	free(DList);												// ファイルリストを廃棄
	return (-1);
}

