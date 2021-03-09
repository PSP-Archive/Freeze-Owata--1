//==============================================================
// ファイルダイアログ関連
// STEAR 2009
//--------------------------------------------------------------

#ifndef FileDialog_H
#define FileDialog_H

//----- 構造体 -----

struct strDList{												// ディレクトリ一覧用構造体
	int		flag;												// 0:ディレクトリ 1:ファイル
	char	name[256];											// 名前
};

//----- プロトタイプ宣言 -----

int		SelectFile(char *title,char *InitDir,char *FileName,char *dir,int flag,int ime,strInpHis **inphis);

#endif
