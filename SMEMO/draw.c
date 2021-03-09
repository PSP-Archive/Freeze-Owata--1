//==============================================================
// 画面作画関連
// STEAR 2009-2010
//--------------------------------------------------------------

#include <pspuser.h>
#include <pspctrl.h>
#include <psputils.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "smemo.h"
#include "graphics.h"
//#include "intraFont.h"
#include "zenkaku.h"
#include "sime.h"
#include "draw.h"
#include "osk.h"

typedef struct
{
	unsigned short	u,v;
	short			x,y,z;
} Vertex;

static struct {													// monafont管理用
	char	*fontA;
	long	sizeA;
	char	*fontW;
	long	sizeW;
	int		height;
} gMona[2];

//static intraFont*	gJpn0;										// intraFont用
static int*			gJpn0;										// ダミー
static int			gFont = 0,									// pf_print()で使用されるフォント
					gProgX,gProgY;								// プログレスバーの位置

//==============================================================
// 横線の作画
//--------------------------------------------------------------

void HLine(int x,int y,int wx,long cor)
{
	int i;

	for (i=0; i<wx ;i++)
		VRAM[y][x+i] = cor;
}

//==============================================================
// 縦線の作画
//--------------------------------------------------------------

void VLine(int x,int y,int wy,long cor)
{
	int i;

	for (i=0; i<wy ;i++)
		VRAM[y+i][x] = cor;
}

//==============================================================
// BOXの作画
//--------------------------------------------------------------

void Box(int x,int y,int wx,int wy,long corFrame)
{
	HLine(x       ,y       ,wx,corFrame);
	VLine(x       ,y       ,wy,corFrame);
	VLine(x + wx-1,y       ,wy,corFrame);
	HLine(x       ,y + wy-1,wx,corFrame);
}

//==============================================================
// 塗りつぶされたBOXの作画
//--------------------------------------------------------------
// 最初の１ライン目はCPUで作画し、残りの部分はGUにより１ライン目をコピーする、
// という方法を使っています。
// こっちのほうがCPUで行うより早いっぽい。
//--------------------------------------------------------------

void Fill(int x,int y,int wx,int wy,long corIn)
{
	if (wx==0 || wy==0) return;
	HLine(x,y,wx,corIn);
	if (wy>1){
		sceGuStart(GU_DIRECT,gList);
		sceGuCopyImage(GU_PSM_8888, x,y,wx,wy-1,512,VRAM, x,y+1,512,VRAM);
		sceGuFinish();
		sceGuSync(0,0);
	}
}

//==============================================================
// 塗りつぶされたBOXの作画（枠あり）
//--------------------------------------------------------------

void BoxFill(int x,int y,int wx,int wy,long corFrame,long corIn)
{
	Fill(x ,y ,wx ,wy ,corIn);
	Box(x ,y ,wx ,wy ,corFrame);
}

//==============================================================
// 背景反転でBOXの作画
//--------------------------------------------------------------

void XFill(int x,int y,int wx,int wy,long corIn)
{
	int i,j;

	for (i=0; i<wy ;i++)
		for (j=0; j<wx ;j++)
			VRAM[y+i][x+j] ^= corIn;
}

//==============================================================
// 角丸なBOXの作画（枠あり）
//--------------------------------------------------------------
// type   角丸の大きさ（0:小さい ～ 3:大きい , 4～5:グラデーション）
// corFrm 枠の色
// corIn  内部の色
//---------------------------------------------------------------
// グラデーションパターンの場合はcorFrmの指定は無視され、corInを元に明るさを
// 変えつつ作画します。
// また、グラデーションパターンでの作画時は透過処理を行います。
// 透過処理には graphics.c を利用しています。
//--------------------------------------------------------------

//----- コントラストの調整 -----

static long CorSet(int ar,long cor,int Alpha)
{
	unsigned long	cor1,cor2,cor3;

	cor1 = ((cor & 0x0000FF) * ar /128);
	if (cor1>0x0000FF) cor1 = 0x0000FF;
	cor2 = ((cor & 0x00FF00) * ar /128);
	if (cor2>0x00FF00) cor2 = 0x00FF00;
	cor3 = ((cor & 0x0FF0000) * ar /128);
	if (cor3>0x0FF0000) cor3 = 0x0FF0000;
	if (Alpha>0xFF) Alpha = 0xFF;
	return ( Alpha<<24 | cor1 | cor2 | cor3 );
}

//----- 始点と終点が色違いの横線を作画 -----

static void CurveBoxSub(int x,int y,int wx,long corFrm,long corIn)
{
	VRAM[y][x] = corFrm;
	HLine(x+1 ,y ,wx-2 ,corIn);
	VRAM[y][x+wx-1] = corFrm;
}

//----- テクスチャーに角丸グラデーションパターンを作成 -----

static void CurveBoxSub2(int x,int y,int wx,int type,int no,long corIn,Image *source,int Alpha)
{
	const int	map1[3][9][8] = {{								// 角のパターン
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70 },
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF },
					{ 0x00,0x00,0x00,0x00,0x00,0xFF,0xF0,0xF0 },
					{ 0x00,0x00,0x00,0x00,0xFF,0xF0,0xE0,0xD0 },
					{ 0x00,0x00,0x00,0xFF,0xF0,0xE0,0xC0,0xB0 },
					{ 0x00,0x00,0x70,0xFF,0xF0,0xC0,0xA0,0x90 },
					{ 0x00,0x00,0xFF,0xF0,0xD0,0xB0,0x90,0x80 },
				},{
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70 },
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF },
					{ 0x00,0x00,0x00,0x00,0x00,0xFF,0x40,0x40 },
					{ 0x00,0x00,0x00,0x00,0xFF,0x40,0x50,0x50 },
					{ 0x00,0x00,0x00,0xFF,0x40,0x50,0x60,0x60 },
					{ 0x00,0x00,0x70,0xFF,0x40,0x50,0x60,0x70 },
					{ 0x00,0x00,0xFF,0x40,0x50,0x60,0x70,0x80 },
				},{
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 },
					{ 0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x10 },
					{ 0x00,0x00,0x00,0x00,0x00,0x10,0x20,0x30 },
					{ 0x00,0x00,0x00,0x00,0x10,0x20,0x30,0x40 },
					{ 0x00,0x00,0x00,0x10,0x20,0x30,0x40,0x60 },
					{ 0x00,0x00,0x10,0x20,0x30,0x40,0x60,0x80 },
					{ 0x00,0x00,0x10,0x30,0x40,0x60,0x80,0x90 },
					{ 0x00,0x10,0x20,0x40,0x60,0x80,0x90,0x90 },
					{ 0x10,0x20,0x40,0x60,0x80,0x90,0x90,0x90 },
				}},
				map2[3][9] = {									// 横枠のバターン
					{ 0x00,0x00,0xFF,0xF0,0xD0,0xB0,0x90,0x80,0x80 },
					{ 0x00,0x00,0xFF,0x40,0x50,0x60,0x70,0x80,0x80 },
					{ 0x10,0x20,0x40,0x60,0x80,0x90,0x90,0x90,0x90 },
				};
	int	i;

	if (no>7) no = 8;
	if (type>2) type = 2;

	for (i=0; i<8 ;i++){
		if (map1[type][no][i]){
			if (type!=2){
				source->data[y*(source->textureWidth)+x+i] = CorSet( map1[type][no][i],corIn,Alpha );
			} else {
				source->data[y*(source->textureWidth)+x+i] = ((map1[type][no][i] * Alpha / 0x80) << 24) + corIn;
			}
		}
	}
	for (i=0; i<wx-16 ;i++)
		if (type!=2){
			source->data[y*(source->textureWidth)+x+8+i] = CorSet( map2[type][no],corIn,Alpha );
		} else {
			source->data[y*(source->textureWidth)+x+8+i] = ((map2[type][no] * Alpha / 0x80) << 24) + corIn;
		}
	for (i=0; i<8 ;i++){
		if (map1[type][no][7-i]){
			if (type!=2){
				source->data[y*(source->textureWidth)+x+wx-8+i] = CorSet( map1[type][no][7-i],corIn,Alpha );
			} else {
				source->data[y*(source->textureWidth)+x+wx-8+i] = ((map1[type][no][7-i] * Alpha / 0x80) << 24) + corIn;
			}
		}
	}
}

//----- メイン -----

void CurveBox(int x,int y,int wx,int wy,int type,long corFrm,long corIn)
{
	const int	rx[4][4] = {
					{1,0,0,0},
					{2,1,0,0},
					{3,2,1,0},
					{4,2,1,1}
				};
	Image		*source;
	int			i;


	if (type>3){
		sceGuStart(GU_DIRECT,gList);
		sceGuTexMode(GU_PSM_8888, 0, 0, 0);						// これを省略するとintraFont使用時に表示が化けます
		sceGuFinish();
		source = createImage(wx+16,wy+16);
		for (i=0; i<wy+15 ;i++){								// ウィンドウの影
			if (i<8){
				CurveBoxSub2( 0 ,i ,wx+14 ,2 ,i ,0x000000 ,source ,0x80 );
			} else if (i>=(wy+15)-8){
				CurveBoxSub2( 0 ,i ,wx+14 ,2 ,(wy+15) -1 - i ,0x000000 ,source ,0x80 );
			} else {
				CurveBoxSub2( 0 ,i ,wx+14 ,2 ,8 ,0x000000 ,source ,0x80 );
			}
		}
		type -= 4;
		for (i=0; i<wy+4 ;i++){									// ウィンドウの本体
			if (i<8){
				CurveBoxSub2( 5 ,i+5 ,wx+4 ,type ,i ,corIn ,source ,0xE0 );
			} else if (i>=(wy+4)-8){
				CurveBoxSub2( 5 ,i+5 ,wx+4 ,type ,(wy+4) -1 - i ,corIn ,source ,0xE0 );
			} else {
				CurveBoxSub2( 5 ,i+5 ,wx+4 ,type ,8 ,corIn ,source ,0xE0 );
			}
		}
		blitAlphaImageToScreen( 0, 0, wx+16, wy+16, source, x-7, y-7 );
		freeImage(source);

	} else {
		for (i=0; i<wy ;i++){
			if (i==0){
				CurveBoxSub(x+rx[type][i] ,y + i ,wx-rx[type][i]*2 ,corFrm ,corFrm);
			} else if (i<4){
				CurveBoxSub(x+rx[type][i] ,y + i ,wx-rx[type][i]*2 ,corFrm ,corIn);
			} else if (i>=wy-1){
				CurveBoxSub(x+rx[type][wy-i-1] ,y + i ,wx-rx[type][wy-i-1]*2 ,corFrm ,corFrm);
			} else if (i>=wy-4){
				CurveBoxSub(x+rx[type][wy-i-1] ,y + i ,wx-rx[type][wy-i-1]*2 ,corFrm ,corIn);
			} else {
				CurveBoxSub(x ,y + i ,wx,corFrm ,corIn);
			}
		}
	}
}

//==============================================================
// 指定領域の0クリア
//--------------------------------------------------------------

void BoxClr(int x,int y,int wx,int wy)
{
	int i;

	for (i=0; i<wy ;i++)
		HLine(x,y + i,wx,0);
}

//==============================================================
// 画面イメージの転送
//--------------------------------------------------------------
// 画面上の指定範囲を指定位置に転送させます。
//--------------------------------------------------------------

void ImgMove(int sx,int sy,int wx,int wy,int dx,int dy)
{
	sceGuStart(GU_DIRECT,gList);
	sceGuCopyImage(GU_PSM_8888, sx,sy,wx,wy,512,VRAM, dx,dy,512,VRAM);
	sceGuFinish();
	sceGuSync(0,0);
}

//==============================================================
// イメージ操作用バッファの確保
//--------------------------------------------------------------

DrawImg *DrawImgCreate(int wx,int wy)
{
	DrawImg	*image;

	if (wx==0 || wy==0) return (NULL);

	image = (DrawImg*) malloc(sizeof(DrawImg));
	if (!image) return (NULL);
	image->wx = wx;
	image->wy = wy;
	image->width = 512;
	image->data = (long*) memalign(16, image->width * wy * sizeof(long));
	if (!image->data){
		free(image);
		return (NULL);
	}
	return (image);
}

//==============================================================
// イメージ操作用バッファの解放
//--------------------------------------------------------------

void DrawImgFree(DrawImg *image)
{
	if (image) free(image->data);
	free(image);
	image = NULL;
}

//==============================================================
// 指定領域のコピー
//--------------------------------------------------------------
// 画面上の指定された領域をimageに取り込む。
// imageは予め必要な大きさをDrawImgCreate()で確保しておいてください。
// 16バイトのアライメントに合わせてメモリを確保しているはずなのにゴミがでる
// のでゴミが出ないように小細工をしてみました。
// ↑はキャッシュの問題だったらしい。
// sceKernelDcacheWritebackInvalidateAll()を実行するようにしたら直ったっぽい。
//--------------------------------------------------------------

void BoxCopy(DrawImg *image,int x,int y)
{
	void	*framebuffer;

	if (image){
		sceGuSwapBuffers();
		framebuffer = sceGuSwapBuffers();
		sceKernelDcacheWritebackInvalidateAll();				// キャッシュのライトバック
		sceGuStart(GU_DIRECT,gList);
		sceGuCopyImage(GU_PSM_8888, x,y,image->wx,image->wy,512,framebuffer+0x04000000, 0,0,image->width,image->data);
		sceGuFinish();
		sceGuSync(0,0);
	}
}

//==============================================================
// 指定領域のペースト
//--------------------------------------------------------------
// 画面上の指定された位置にimageを展開する。
// 単純に転送するだけでアルファ合成とかはしない分、早い。
//--------------------------------------------------------------

void BoxPaste(DrawImg *image,int x,int y)
{
	void	*framebuffer;

	if (image){
		sceGuSwapBuffers();
		framebuffer = sceGuSwapBuffers();
		sceGuStart(GU_DIRECT,gList);
		sceGuCopyImage(GU_PSM_8888, 0,0,image->wx,image->wy,image->width,image->data, x,y,512,framebuffer+0x04000000);
		sceGuFinish();
		sceGuSync(0,0);
	}
}

//==============================================================
// 画面イメージの保存
//--------------------------------------------------------------
// 現在の画面全体を待避させる。
// 必ずScreenPaste()かDrawImgFree()でバッファを解放する事。
//--------------------------------------------------------------

DrawImg *ScreenCopy(void)
{
	DrawImg	*image;

	image = DrawImgCreate(480,272);
	BoxCopy( image, 0, 0 );
	return (image);
}

//==============================================================
// 画面イメージの復元とバッファの削除
//--------------------------------------------------------------
// 待避させていた画面イメージを復元し、バッファを削除する。
// 復元は一回しかできません。
// 必ずScreenPaste()かDrawImgFree()でバッファを解放する事。
//--------------------------------------------------------------

void ScreenPaste(DrawImg *image)
{
	BoxPaste( image, 0, 0 );
	DrawImgFree(image);
}

//==============================================================
// intraFontロード
//--------------------------------------------------------------
// 戻り値  0:成功
//        -1:ロード失敗
//--------------------------------------------------------------
// intraFontの日本語フォントを読み込む。
// これを実行していない場合、intraFontは使えません。（代替で東雲フォントが使われます。）
//--------------------------------------------------------------

int pf_setIntraFont(void)
{
/*	intraFontInit();											// Init intraFont library
	gJpn0 = intraFontLoad("flash0:/font/jpn0.pgf",
				INTRAFONT_STRING_SJIS | INTRAFONT_CACHE_LARGE);	// japanese font (with SJIS text string encoding)
	if (!gJpn0) return (-1);
	intraFontSetStyle( gJpn0, 0.65f, 0xFFFFFFFF, 0x00000000, 0 );
	intraFontSetEncoding( gJpn0, INTRAFONT_STRING_SJIS );
*/
	return (0);
}

//==============================================================
// intraFont終了処理
//--------------------------------------------------------------

void pf_endIntraFont(void)
{
//	intraFontUnload(gJpn0);
//	intraFontShutdown();
}

//==============================================================
// monafontロード
//--------------------------------------------------------------
// no     フォントの種別（0:monafont12 1:monafont16）
// pathA  半角フォントのファイル名
// pathW  全角フォントのファイル名
// 戻り値  0:成功
//        -1:ロード失敗
//--------------------------------------------------------------
// monafontを読み込む。
// これを実行していない場合、monafontは使えません。（代替で東雲フォントが使われます。）
//--------------------------------------------------------------

int pf_setMonaFont(int no,char *pathA,char *pathW)
{
	int	fd;

	fd = sceIoOpen( pathA, PSP_O_RDONLY, 0777 );				// 半角フォント
	if (fd>=0){
		gMona[no].sizeA = sceIoLseek( fd, 0, SEEK_END );
		sceIoLseek( fd, 0, SEEK_SET );
		gMona[no].fontA = (char*) malloc(gMona[no].sizeA);
		if (gMona[no].fontA){
			sceIoRead( fd, gMona[no].fontA, gMona[no].sizeA );
		}
		sceIoClose(fd);
	}
	fd = sceIoOpen( pathW, PSP_O_RDONLY, 0777 );				// 全角フォント
	if (fd>=0){
		gMona[no].sizeW = sceIoLseek( fd, 0, SEEK_END );
		sceIoLseek( fd, 0, SEEK_SET );
		gMona[no].fontW = (char*) malloc(gMona[no].sizeW);
		if (gMona[no].fontW){
			sceIoRead( fd, gMona[no].fontW, gMona[no].sizeW );
		}
		sceIoClose(fd);
	}

	if (!gMona[no].fontA || !gMona[no].fontW){					// フォントのロードに失敗していた場合
		free(gMona[no].fontA);
		gMona[no].sizeA = 0;
		free(gMona[no].fontW);
		gMona[no].sizeW = 0;
		return (-1);
	}
	return (0);
}

//==============================================================
// monafontの終了
//--------------------------------------------------------------

void pf_endMonaFont(void)
{
	free(gMona[0].fontA);
	free(gMona[0].fontW);
	free(gMona[1].fontA);
	free(gMona[1].fontW);
}

//==============================================================
// フォントのチェック
//--------------------------------------------------------------
// 指定されたフォントが本当に使えるかチェックし、使えない場合は東雲フォントに切り替える。
//--------------------------------------------------------------

static int chkFont(int font)
{
	if (font>4) font = 0;										// 範囲外の時は東雲フォント
	if ((font==3 || font==4) && !gJpn0) font = 0;				// intraFontが使えない
	if ((font==1 && !gMona[0].fontA) || (font==2 && !gMona[1].fontA)){
		font = 0;												// monafontが使えない
	}
	return (font);
}

//==============================================================
// 文字フォントの指定
//--------------------------------------------------------------
// font   文字のフォント（0:東雲フォント    1:monafont12   2:monafont16
//                        3:intraFont等幅   4:intraFontプロポーショナル）
// 戻り値 実際に選択された文字フォント
//--------------------------------------------------------------
// pf_print()で文字表示する場合のフォントを指定する。
// 指定されたフォントが使用できない場合は東雲フォントが使われます。
//--------------------------------------------------------------

int pf_fontType(int font)
{
	gFont = chkFont(font);
	return (gFont);
}

//==============================================================
// monafontエントリーアドレスの取得
//--------------------------------------------------------------
// no     monafontの種類（0:monafont12 1:monafont16）
// str    最初の一文字目に取得したい文字を入れる
// fwidth 文字の横幅
// 戻り値 フォントデータ
//--------------------------------------------------------------
// 指定された文字に対応するフォントデータへのアドレスを取得する。
// フォントデータのフォーマットは
//     12ドットmonafont:文字幅(1)、フォントイメージ(15)
//     16ドットmonafont:文字幅(1)、フォントイメージ(16)
//
// fwidthで返される文字幅はフォントの幅ではなく、画面上での占有幅で実数値になります。
// 指定フォントが使えない場合はNULLが返されます。
//--------------------------------------------------------------

static unsigned short *GetMonaFont(int no,const char *str,float *fwidth)
{
	int	hi,lo,index;
	unsigned short *font;

	if (!gMona[no].fontA) return (NULL);						// フォントが使えない

	if (chkSJIS(str[0])){										// 全角文字なら
		hi = (unsigned char) str[0];
		lo = (unsigned char) str[1];
		hi -= (hi <= 0x9f) ? 0x71 : 0xb1;
		hi <<= 1;
		hi++;
		if (lo > 0x7f)
			lo--;
		if (lo >= 0x9e){
			lo -= 0x7d;
			hi++;
		}else{
			lo -= 0x1f;
		}
		hi -= 0x21;
		lo -= 0x21;
		index = hi * (0x7e - 0x20);
		index += lo;
		if (no==0){
			if ((index << 5) >= gMona[no].sizeW){
				index = 0;
			}
			font = (unsigned short*)(gMona[no].fontW + (index<<5));
		} else {
			if ((index*34) >= gMona[no].sizeW)
				index = 0;
			font = (unsigned short*)(gMona[no].fontW + (index*34));
		}
		*fwidth = font[0];
		if (*fwidth>16) *fwidth = 16;
		if (*fwidth<0) *fwidth = 1;

	} else {													// 半角文字なら
		index = (unsigned char) str[0];
		if ((unsigned char)str[0] < 0x20) {
			index = 0;
		} else if ((unsigned char)str[0] < 0x80) {
			index -= 0x20;
		} else if ((unsigned char)str[0] > 0xa0) {
			index -= 0x41;
		} else {
			index = 0;
		}
		if (no==0){
			if ((index << 5) >= gMona[no].sizeA) {
				index = 0;
			}
			font = (unsigned short*)(gMona[no].fontA + (index<<5));
		} else {
			if ((index *34) >= gMona[no].sizeA) {
				index = 0;
			}
			font = (unsigned short*)(gMona[no].fontA + (index*34));
		}
		*fwidth = font[0];
		if (*fwidth>16) *fwidth = 16;
		if (*fwidth<0) *fwidth = 1;
	}

	return (font);
}

//==============================================================
// monafontで文字表示
//--------------------------------------------------------------
// no     monafontの種類（0:monafont12 1:monafont16）
// 戻り値 最後に表示した文字の終了X座標
//--------------------------------------------------------------
// 表示X座標が画面外になった時点で表示は中断されます。
// この時最後の文字は画面外にはみ出します。
// 表示Y座標が画面外になった部分の作画は行われません。
// （Y座標が0～271の範囲でないと使えない）
// 指定フォントが使えない場合は作画は行われません。
//--------------------------------------------------------------

static float monaPrint(float x,int y,int no,const char *str,long cor)
{
	unsigned short	*fd,bit;
	int				sPos,fx,fy,fdw,fdh;
	float			fw;
	long			*vram;

	sPos = 0;
	if (no==0){
		fdh = 12;
	} else {
		fdh = 16;
	}
	while (str[sPos]){
		fd = GetMonaFont( no,&str[sPos], &fw );					// フォントデータ取得
		if (!fd) break;
		fdw = *fd++;											// 文字幅
		if (fdw>16) fdw = 16;
		for (fy=0; fy<fdh ;fy++){								// フォントイメージ作画
			bit = 0x8000;
			if (y+fy>=SCR_HEIGHT) break;						// 画面からはみ出すなら
			vram = &VRAM[y+fy][(int)x];
			for (fx=0; fx<fdw ;fx++){
				if (*fd & bit) *vram = cor;
				vram++;
				bit >>= 1;
			}
			fd++;
		}
		x += fw;
		if (x>SCR_WIDTH) break;									// 画面外にはみ出したなら
		if (chkSJIS(str[sPos])) sPos++;
		sPos++;
	}

	return (x);
}

//==============================================================
// 文字フォントの表示
//--------------------------------------------------------------
// 戻り値 表示したフォントの文字幅
//--------------------------------------------------------------
// pf_fontType()で指定されたフォントで文字を表示します。
//--------------------------------------------------------------

float pf_print(float x,int y,const char *str,long cor)
{
	return (pf_print2( x, y, gFont, str, cor ));
}

//==============================================================
// 文字フォントの表示2
//--------------------------------------------------------------
// font   文字のフォント（0:東雲フォント    1:monafont12   2:monafont16
//                        3:intraFont等幅   4:intraFontプロポーショナル）
// 戻り値 表示した文字の次の位置
//--------------------------------------------------------------
// 指定されたフォントで文字を表示します。
// 指定フォントが使えない場合は東雲フォントが使われます。
//--------------------------------------------------------------

float pf_print2(float x,int y,int font,const char *str,long cor)
{
	int	x2 = 0;

	if (font>4) font = 0;										// 範囲外の時は東雲フォント
	if (!gJpn0 && (font==3 || font==4)){						// intraFontが使えないなら
		font -= 3;												// monafontに代替
	}
	if ((font==1 && !gMona[0].fontA) || (font==2 && !gMona[1].fontA)){
		font = 0;
	}

	switch (font){
	case 0:														// monafont等幅
		mh_print( x,y,str,cor );
		x2 = x + strlen(str) * 6;
		break;
	case 1:														// monafont12プロポーショナル
		x2 = monaPrint( x,y,0,str,cor );
		break;
	case 2:														// monafont16プロポーショナル
		x2 = monaPrint( x,y,1,str,cor );
		break;
	case 3:														// intraFont等幅
		if (chkSJIS(*str)){
			guStart();
//			intraFontSetStyle( gJpn0, 0.65f, cor | 0xFF000000, 0x00000000, 0 );
//			intraFontPrint(gJpn0, x, y+10, str);
			sceGuFinish();
		} else {
			mh_print( x,y,str,cor );
		}
		x2 = x + strlen(str) * 6;
		break;
	case 4:														// intraFontプロポーショナル
		guStart();
//		intraFontSetStyle( gJpn0, 0.65f, cor | 0xFF000000, 0x00000000, 0 );
//		x2 = intraFontPrint(gJpn0, x, y+10, str);
		sceGuFinish();
		break;
	}

	return (x2);
}

//==============================================================
// 文字列の幅
//--------------------------------------------------------------
// font   文字のフォント（0:東雲フォント    1:monafont12   2:monafont16
//                        3:intraFont等幅   4:intraFontプロポーショナル）
// str    文字列
// 戻り値 占有幅（ドット）
//--------------------------------------------------------------
// 画面に文字を表示した際の文字幅をドット単位で取得する。
// 指定フォントが使えない場合は東雲フォントが使われます。
// …本当は小数点以下の端数まで考慮して作画を行おうと思っていたんだけど、
// それをやると左右スクロール時に文字表示位置に1ドットのズレが出て補正が困難に
// なったので止めました。
// 端数が無い方が表示がすっきりした感じがするので、かえってよかったかも。
//--------------------------------------------------------------

float GetStrWidth(int font,const char *str)
{
	unsigned short	*fd;
	int				pos;
	float			w,fw;

	font = chkFont(font);										// 範囲外の時は東雲フォント

	if (font==0 || font==3){									// 等幅フォント
		w = strlen(str) * 6;
	} else if (font==4){										// intraFontプロポーショナル
//		w = (int) intraFontMeasureText( gJpn0,str );			// 小数部があると作画時に位置ズレが発生するので
		w = 6;
	} else {													// monafontプロポーショナル
		w = 0;
		pos = 0;
		while (str[pos]){
			fd = GetMonaFont( font-1, &str[pos], &fw );
			w += fw;
			if (chkSJIS(str[pos])) pos++;
			pos++;
		}
	}
	return (w);
}

//==============================================================
// 複数行文字列の解析
//--------------------------------------------------------------
// text内改行（'\n'）を解析して行数と最大行長を取得する。
// pf_fontType()の影響を受けます。
//--------------------------------------------------------------

void ChkText(int font,char *text,int *maxLen,int *line,int *wx)
{
	char	c[128];
	int		l,p,pos;

	l = 0;
	p = 0;
	*maxLen = 0;
	*line = 0;
	*wx = 0;
	pos = 0;
	while (1){													// 行数の確認
		c[pos] = text[p];
		l++;
		if (text[p]=='\0' || text[p]=='\n'){
			(*line)++;
			if (*maxLen<l){
				*maxLen = l -1;
				c[pos] = '\0';
				*wx = GetStrWidth( font,c );
			}
			l = 0;
			pos = 0;
		}
		if (text[p]=='\0') break;
		p++;
		pos++;
	}
}

//==============================================================
// 複数行文字列の表示
//--------------------------------------------------------------
// text内改行（'\n'）に対応してるので複数行の表示もいけます。
//--------------------------------------------------------------

void DrawText(int x,int y,int font,char *text,long corStr)
{
	char	s,str[128];
	int		p,pos;

	pos = 0;
	while (text[pos]!='\0'){
		p = 0;
		while (1){												// 一行分を取り出す
			s = text[pos++];
			if (s=='\0'){
				pos--;
				break;
			}
			if (s=='\n') break;
			str[p++] = s;
		}
		str[p] = '\0';
		pf_print2(x, y, font, str, corStr);						// 一行分表示
		y += 12 +1;
	}
}

//==============================================================
// メッセージダイアログの表示
//--------------------------------------------------------------
// text内改行（'\n'）に対応してるので複数行の表示もいけます。
// xにマイナスの値を指定すると画面中央に表示します。
//--------------------------------------------------------------

void DiaBox1(int x,int y,int font,char *text,long corStr,long corFrm,long corIn)
{
	int len,wx,wy;

	ChkText( font, text, &len, &wy, &wx );
	wy *= 13;
	if (x<0) x = 240 - (10+wx+10)/2;
	CurveBox( x, y, 10+wx+10, 6+wy+5, 4, corFrm, corIn );
	DrawText( x+10, y+6, font, text, corStr );
}

//==============================================================
// 確認ダイアログの表示
//--------------------------------------------------------------
// text内改行（'\n'）に対応してるので複数行の表示もいけます。
// メッセージの後に0.5行空けてキーガイダンスを表示します。
// xにマイナスの値を指定すると画面中央に表示します。
//--------------------------------------------------------------

void DiaBox2(int x,int y,int font,char *text,char *key,long corStr,long corKey,long corFrm,long corIn)
{
	int		len,wx,wx2,wy;

	ChkText( font, text, &len, &wy, &wx );
	wy *= 13;
	if (x<0) x = 240 - (10+wx+10)/2;
	CurveBox( x, y, 10+wx+10, 6+wy+18+5, 4, corFrm, corIn );
	DrawText( x+10, y+6, font, text, corStr );
	wx2 = GetStrWidth( gFont,key );
	pf_print2( x+10+wx/2-wx2/2, y+6+wy+6, font, key, corKey );
}

//==============================================================
// Yes/No確認ダイアログ
//--------------------------------------------------------------
// msg    メッセージ（改行あり）
// 戻り値 1:○が押された
//        0:○以外
//--------------------------------------------------------------
// メッセージに「○:Yes  ×:No」を追加した上で入力処理を行います。
//--------------------------------------------------------------

int DialogYN(char *msg,long corFrm,long corIn)
{
	SceCtrlData		pad;
	int				ret;

	DiaBox2( -1, 104, 0, msg, "○:Yes  ×:No", gSCor[0], gSCor[13], corFrm, corIn );	// 中央に表示
	ScreenView();												// 画面更新
	while (1){
		sceCtrlReadBufferPositive(&pad, 1);
		if (!(pad.Buttons & 0x00F3F9)) break;					// 操作系ボタンが全て離された
		sceDisplayWaitVblankStart();
	}
	ret = 0;
	while (!gExitRequest){										// ○か×が押されるまで待機
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		if (ret==SIME_KEY_CIRCLE || ret==SIME_KEY_CROSS) break;
		sceDisplayWaitVblankStart();
	}
	if (ret==SIME_KEY_CIRCLE){
		ret = 1;
	} else {
		ret = 0;
	}
	return (ret);
}

//==============================================================
// ダイアログフレームの構築
//--------------------------------------------------------------

void DialogFrame(int x,int y,int wx,int wy,char *title,char *guide,long corFrm,long corIn)
{
	CurveBox( x, y, wx, wy, 4, corFrm, corIn );
	mh_print( x+10, y+6, title, corFrm );
	mh_print( x+wx-10-6*strlen(guide), y+wy-6-12, guide, corFrm );
}

//==============================================================
// プログレスバーの表示1
//--------------------------------------------------------------
// プログレスバーの初期表示を行います。
// 進行状況はProgressbar2()で更新します。
// サイズは固定です。
//--------------------------------------------------------------

void Progressbar1(int x,int y,int font,char *key,long corBak,long corKey,long corFrm,long corIn)
{
	int		wx2;

	if (x<0) x = 240 - (10+200+10)/2;
	gProgX = x + 10;
	gProgY = y + 8;
	CurveBox( x, y, 10+200+10, 6+13+18+5, 4, corFrm, corIn );
	CurveBox( gProgX, gProgY, 200, 11, 0, corFrm, corBak );
	wx2 = GetStrWidth( gFont,key );
	pf_print2( x+10+200/2-wx2/2, y+6+13+6, font, key, corKey );
}

//==============================================================
// プログレスバーの表示2
//--------------------------------------------------------------
// プログレスバーの更新を行います。
// 予めProgressbar1()で初期設定を行っておくこと。
// バーが減る場合は考慮してません。
//--------------------------------------------------------------

void Progressbar2(long pos,long max,long corBar)
{
	const int	cont[9] = {0x70,0xA0,0x90,0x80,0x78,0x70,0x68,0x60,0x60};
	int		i;

	if (max==0) max = 1;
	if (pos>=max) pos = max -1;
	for (i=0; i<9 ;i++){
		HLine( gProgX+1, gProgY+1+i, 199 * pos/max, CorSet(cont[i],corBar,0xFF) );
	}
}

//==============================================================
// スクロールバーの表示（縦）
//--------------------------------------------------------------
// x,y  スクロールバー位置
// wy   スクロールバーの大きさ
// pos  表示位置
// size 表示サイズ
// max  全体の大きさ
// flag 強制作画するか？（0:前回と同じなら作画しない）
//--------------------------------------------------------------

void VRollBar(int x,int y,int wy,long pos,long size,long max,int flag,long corFrm,long corIn,long corBar)
{
	static long	lenBuf = 0,pBuf = 0;
	long	len,p;

	if (max == 0)
		p = 0;
	else
		p = pos * (wy -2) / max;
	if (max == 0)
		len = wy -2;
	else
		len = (wy -1) * size / max +1;
	if (len > (wy -2)) len = wy -2;								// 作画範囲からはみ出る場合の補正
	if (p + len > (wy -2)) p = wy -2 - len;						// 作画範囲からはみ出る場合の補正

	if (lenBuf==len && pBuf==p && flag==0) return;				// 前回と同じなら作画しない
	lenBuf = len;
	pBuf = p;

	BoxFill(x, y, 5, wy, corFrm, corIn);						// 一度消して
	Fill(x+1, y+1+p, 3, len, corBar);							// スクロールバーを作画
}

//==============================================================
// 数字の表示
//--------------------------------------------------------------
// x,y    表示開始位置
// val    表示する値（６桁まで）
// ztype  先頭の空白を前詰めするか（0:しない）
// ctype  文字の種類（0:3x5フォント 1:東雲フォント 2:monafont12
//                    3:monafont16  4:intraFont    5:intraFontP）
// corStr 文字色
// 戻り値 表示終了座標
//--------------------------------------------------------------

//----- 3x5数値フォント表示 -----

static void sv_print(int x,int y,char c,long corStr)
{
	const int font[10][15] = {{
					1,1,1,
					1,0,1,
					1,0,1,
					1,0,1,
					1,1,1,
				},{
					0,1,0,
					0,1,0,
					0,1,0,
					0,1,0,
					0,1,0,
				},{
					1,1,1,
					0,0,1,
					1,1,1,
					1,0,0,
					1,1,1,
				},{
					1,1,1,
					0,0,1,
					0,1,1,
					0,0,1,
					1,1,1,
				},{
					0,1,0,
					1,1,0,
					1,1,0,
					1,1,1,
					0,1,0,
				},{
					1,1,1,
					1,0,0,
					1,1,1,
					0,0,1,
					1,1,1,
				},{
					1,1,1,
					1,0,0,
					1,1,1,
					1,0,1,
					1,1,1,
				},{
					1,1,1,
					0,0,1,
					0,0,1,
					0,0,1,
					0,0,1,
				},{
					1,1,1,
					1,0,1,
					1,1,1,
					1,0,1,
					1,1,1,
				},{
					1,1,1,
					1,0,1,
					1,1,1,
					0,0,1,
					1,1,1,
				}};
	int i,j;

	if (c==' ') return;
	c -= '0';
	for (i=0; i<5 ;i++)
		for (j=0; j<3 ;j++)
			if (font[(int)c][i*3+j]) VRAM[y+i][x+j] = corStr;
}

//----- 各桁の値を表示 -----

static int dvsub(int x,int y,int val,int flag,int ztype,int ctype,long corStr)
{
	static char	zero;
	char		c[2] = {0,0};

	if (flag==0){
		zero = ' ';
	} else if (flag==2){
		zero = '0';
	}

	if (val!=0){												// 先行する'0'を空白に変換するか
		zero = '0';
		c[0] = '0' + val;
	} else {
		c[0] = zero;
	}
	if (ztype && c[0]==' ') return (x);							// 空白の前詰めをするなら
	if (ctype){
		pf_print2( x, y, ctype-1, c, corStr );
		return (x+GetStrWidth(ctype-1,c));
	} else {
		sv_print( x, y, c[0], corStr );
		return (x+4);
	}
}

int DrawVal(int x,int y,long val,int ztype,int ctype,long corStr)
{
	x = dvsub(x, y, (val/100000)%10, 0, ztype, ctype, corStr);
	x = dvsub(x, y, (val/10000 )%10, 1, ztype, ctype, corStr);
	x = dvsub(x, y, (val/1000  )%10, 1, ztype, ctype, corStr);
	x = dvsub(x, y, (val/100   )%10, 1, ztype, ctype, corStr);
	x = dvsub(x, y, (val/10    )%10, 1, ztype, ctype, corStr);
	x = dvsub(x, y, (val/1     )%10, 2, ztype, ctype, corStr);
	return (x);
}

//==============================================================
// 記号の表示
//--------------------------------------------------------------

void DrawChar(int x,int y,int code,long corStr)
{
	const int font[][55] = {{
					0,0,0,0,0,
					0,0,0,0,0,
					1,0,0,0,0,
					0,1,1,0,0,
					1,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
				},{
					0,0,0,0,0,
					0,1,0,0,0,
					0,1,0,0,0,
					0,1,0,0,0,
					1,1,1,0,0,
					0,1,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
				},{
					0,0,1,0,1,
					0,1,0,0,0,
					0,0,0,0,0,
					0,1,0,0,0,
					0,0,0,0,0,
					0,1,0,0,0,
					0,0,1,0,1,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
				},{
					0,1,0,0,0,
					0,0,1,0,0,
					0,0,0,0,0,
					0,0,1,0,0,
					0,0,0,0,0,
					0,0,1,0,0,
					0,1,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
				},{
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					1,0,0,0,0,
					0,1,1,0,0,
					1,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
				},{
					0,0,0,0,0,
					0,0,0,0,0,
					0,0,1,0,0,
					0,0,1,0,0,
					0,0,1,0,0,
					0,0,1,0,0,
					1,0,1,0,1,
					0,1,1,1,0,
					0,0,1,0,0,
					0,0,0,0,0,
					0,0,0,0,0,
				},{
					0,0,1,0,1,
					0,1,0,0,0,
					0,0,0,0,0,
					0,1,0,0,0,
					0,0,0,0,0,
					0,1,0,0,0,
					0,0,0,0,0,
					0,1,0,0,0,
					0,0,0,0,0,
					0,1,0,0,0,
					0,0,1,0,1,
				},{
					0,1,0,1,0,
					0,0,0,0,1,
					0,0,0,0,0,
					0,0,0,0,1,
					0,0,0,0,0,
					0,0,0,0,1,
					0,0,0,0,0,
					0,0,0,0,1,
					0,0,0,0,0,
					0,0,0,0,1,
					0,1,0,1,0,
				}};
	int i,j;

	for (i=0; i<11 ;i++)
		for (j=0; j<5 ;j++)
			if (font[code][i*5+j]) VRAM[y+i][x+j] = corStr;
}

void DrawChar2(int x,int y,int code,long corStr)
{
	const int font[][256] = {{
					0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,
					0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
					0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
					0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
					0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
					0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
					0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
					0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
					0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
					0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
					0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,
				},{
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
				},{
					0,1,1,1,1,1,0,0,0,0,1,1,1,1,1,0,
					1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
					0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
					0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,
					0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
					1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,
					0,1,1,1,1,1,0,0,0,0,1,1,1,1,1,0,
				}};
	int i,j;

	for (i=0; i<16 ;i++)
		for (j=0; j<16 ;j++)
			if (font[code][i*16+j]) VRAM[y+i][x+j] = corStr;
}

//==============================================================
// 画面表示の更新
//--------------------------------------------------------------
// VRAM作画画面のイメージ情報を表示画面へ転送することで画面表示を更新させる。
// ダブルバッファを使用しつつ必要最低限の画面書き換えで表示を行うための苦肉の策。
// VRAMページ0が作画ページ、ページ1が表示ページで決め打ちしてます。
//--------------------------------------------------------------

void ScreenView(void)
{
	sceGuStart( GU_DIRECT, gList );
	sceGuCopyImage( GU_PSM_8888, 0, 0, 480, 272, 512, VRAM, 0, 272, 512, VRAM);
	sceGuFinish();
	sceGuSync(0,0);
}

