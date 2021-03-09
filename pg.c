/*
* $Id: pg.c 145 2008-08-21 08:26:11Z bird_may_nike $
*/

#include "pg.h"
#include <pspgu.h>
#include <pspge.h>
#include <pspdisplay.h>
#include <psppower.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "charConv.h"

extern S_2CH s2ch; // psp2chRes.c
extern int dispPalette; // psp2chMenu.c
extern char palette[512]; // psp2chMenu.c
extern unsigned short colorPalette[256]; // colorMap.h

/*************************
文字参照用の配列を作成
追加するときは
#define MAX_ENTITIES 
を修正
**************************/
#define MAX_ENTITIES 15
static struct entityTag entity[MAX_ENTITIES] = 
{
	{"&amp;",	4,	1,	'&',	0},	
	{"&gt;",	3,	1,	'>',	0},	
	{"&lt;",	3,	1,	'<',	0},	
	{"&quot;",	5,	1,	'"',	0},	
	{"&nbsp;",	5,	1,	'	',	0},	
	{"&rsquo;",	6,	2,	0x81,	0x66},	
	{"&rdquo;",	6,	2,	0x81,	0x68},	
	{"&deg;",	4,	2,	0x81,	0x8B},	
	{"&rarr;",	5,	2,	0x81,	0xA8},	
	{"&larr;",	5,	2,	0x81,	0xA9},	
	{"&uarr;",	5,	2,	0x81,	0xAA},	
	{"&darr;",	5,	2,	0x81,	0xAB},	
	{"&sub;",	4,	2,	0x81,	0xBC},	
	{"&and;",	4,	2,	0x81,	0xC8},	
	{"&or;",	3,	2,	0x81,	0xC9}
};

#define MAX_FRAME (3)
#define SCREEN_TEXTURE (BUF_WIDTH * BUF_HEIGHT * 2)
static int frame_num;
static Window_Layer windowLayer[MAX_FRAME];
static unsigned char *printBuf; // 描画先選択用ポインタ（pixelsとwinPixelsを切り替えて使用）
Window_Layer *winParam;

unsigned char *temp_buf;

static RECT barSrcRect[2]; // psp2chInit(psp2ch.c)で初期化
static RECT menuDstRect[2]; // psp2chInit(psp2ch.c)で初期化
static RECT titleDstRect[2]; // psp2chInit(psp2ch.c)で初期化

unsigned int  __attribute__((aligned(16))) list[512 * 512];
void* framebuffer; // drawbufferを保存
static void* preframeBuffer;

static TEX tex = {BUF_WIDTH*2, BUF_HEIGHT, BUF_WIDTH*2};
static unsigned char *fontA, *fontJ;
static unsigned int size_fontA, size_fontJ;
static S_PUTCHAR sChar;
// VRAM内にメモリ確保してメインメモリ節約 1671168 + 32768 + 32768 + 1440*5
static unsigned char* barPixels = (unsigned char*)(0x04000000 + 0x110000 + 0x88000); // BUF_WIDTH*64
static unsigned char* titlePixels = (unsigned char*)(0x04000000 + 0x110000 + 0x88000 + BUF_WIDTH * 64); // BUF_WIDTH*64
static unsigned char* cursorImg[5] = {(unsigned char*)(0x04000000 + 0x110000 + 0x88000 + BUF_WIDTH * 64 * 2), // 32*45 *5
                                      (unsigned char*)(0x04000000 + 0x110000 + 0x88000 + BUF_WIDTH * 64 * 2 + 1440*1),
                                      (unsigned char*)(0x04000000 + 0x110000 + 0x88000 + BUF_WIDTH * 64 * 2 + 1440*2),
                                      (unsigned char*)(0x04000000 + 0x110000 + 0x88000 + BUF_WIDTH * 64 * 2 + 1440*3),
                                      (unsigned char*)(0x04000000 + 0x110000 + 0x88000 + BUF_WIDTH * 64 * 2 + 1440*4)};

// prototype
static void pgCopyRect(void *src, TEX *tex, RECT *src_rect, RECT *dst_rect);
static void pgCopyRectRotate(void *src, TEX *tex, RECT *src_rect, RECT *dst_rect);
static inline unsigned char* pgGetVramAddr(unsigned long x,unsigned long y, int w);
static void pgFillRect(unsigned short color, RECT *rect);
static int pgPutCharA(const unsigned char c);
static int pgPutCharW(unsigned char hi,unsigned char lo);
static int pgSpecialChars(char** string);
static void idEnd(void);
static void resEnd(void);
static void urlEnd(void);
static int pgCountCharA(const unsigned char c, int width);
static int pgCountCharW(unsigned char hi,unsigned char lo, int width);
static int pgCountSpecialChars(char** string, int width);
static void (*pgCopyFunc[2])(void *src, TEX *tex, RECT *src_rect, RECT *dst_rect) = {pgCopyRect, pgCopyRectRotate};

void DiaBox1(int x,int y,int font,char *text,long corStr,long corFrm,long corIn);	// PSPメモ帳
void WaitBtn(void);																	// PSPメモ帳
void Unpush(void);																	// PSPメモ帳

/*************************
カーソルのフォントからビットマップ画像を作る
**************************/
void pgCursorColorSet(void)
{
	#define O 0
	#define B 1
	#define W 2
	#define G 3
	const unsigned char cursorFont[32*45] = {
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,B,O,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,W,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,W,W,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,W,W,W,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,W,W,W,W,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,W,W,W,B,B,B,B,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,W,B,W,W,B,G,G,G,G,G,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,W,B,G,B,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,B,G,G,B,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    B,O,G,O,O,B,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,B,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,B,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,B,W,W,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,B,B,B,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,0,G,G,G,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,
	    O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O,O
	};
	const unsigned char cursorNum[4][10*10] = {{
		O,O,O,B,B,B,G,O,O,O,
		O,O,B,W,W,B,G,G,O,O,
		O,B,W,W,W,B,G,G,O,O,
		O,B,W,W,W,B,G,G,O,O,
		O,B,B,W,W,B,G,G,O,O,
		O,O,B,W,W,B,G,G,O,O,
		O,O,B,W,W,B,G,G,O,O,
		O,O,B,W,W,B,G,G,O,O,
		O,O,B,W,W,B,G,G,O,O,
		O,O,B,B,B,B,G,G,O,O,
	},{
		B,B,B,B,B,B,B,B,G,O,
		B,W,W,W,W,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,B,B,B,B,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,W,W,B,B,B,B,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,B,B,B,B,B,B,B,G,G,
	},{
		B,B,B,B,B,B,B,B,G,O,
		B,W,W,W,W,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,B,B,B,B,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,B,B,B,B,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,B,B,B,B,B,B,B,G,G,
	},{
		B,B,B,B,G,O,O,O,O,O,
		B,W,W,B,G,G,O,O,O,O,
		B,W,W,B,B,B,B,G,O,O,
		B,W,W,B,W,W,B,G,G,O,
		B,W,W,B,W,W,B,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,W,W,W,W,W,W,B,G,G,
		B,B,B,B,W,W,B,B,G,G,
		O,G,G,B,W,W,B,G,G,G,
		O,O,O,B,B,B,B,G,G,O,
	}};
	#undef O
	#undef B
	#undef W
	#undef G
    int i, j, k;

	for (j = 0; j < 5; j++){									// ベースになるカーソルイメージ
	    for (i = 0; i < 32*45; i++)
	    {
	        if (cursorFont[i] == 1)
	        {
	            cursorImg[j][i] = 144;
	        }
	        else if (cursorFont[i] == 2)
	        {
	            cursorImg[j][i] = 145;
	        }
	        else if (cursorFont[i] == 3)
	        {
	            cursorImg[j][i] = 146;
	        }
	        else
	        {
	            cursorImg[j][i] = 0;
	        }
	    }
	}

	for (i = 0; i < 4; i++){									// 数字イメージを重ね合わせる
		for (j = 0; j < 10; j++){
			for (k = 0; k < 10; k++){
				if (cursorNum[i][j*10+k] == 1){
					cursorImg[i+1][(22+j)*32+(12+k)] = 144;
				} else if (cursorNum[i][j*10+k] == 2){
					cursorImg[i+1][(22+j)*32+(12+k)] = 145;
				} else if (cursorNum[i][j*10+k] == 3){
					cursorImg[i+1][(22+j)*32+(12+k)] = 146;
				}
			}
		}
	}
}

//==============================================================
// エラーダイアログ
//--------------------------------------------------------------
// フォントのロードに失敗した場合、エラーメッセージが表示できないので、
// PSPメモ帳側のルーチンでメッセージ表示を行う。
//--------------------------------------------------------------
void pgErrorDialog(int type,char *path)
{
	char msg[256];

	switch (type){
	case 0:
		strcpy( msg, "指定されたフォントが見つかりません\n" );
		break;
	case 1:
		strcpy( msg, "フォント管理用メモリの取得に失敗しました\n" );
		break;
	case 2:
		strcpy( msg, "フォントのロードに失敗しました\n" );
		break;
	case 3:
		strcpy( msg, "フォルダの作成に失敗しました\n" );
		break;
	}
	strcat( msg, path );
	strcat( msg, "\n\nプログラムの実行を強制終了します" );
	DiaBox1( -1, 100, 0, msg, 0xFFFFFF, 0x8080A0, 0x404090 );
	Unpush();													// ボタンが離されるまで待機
	WaitBtn();													// 何か押されるまで待機
}

/*****************************
外部フォントファイルを読み込む
*****************************/
int pgExtraFontInit(void)
{
    SceUID fd;
    SceIoStat st;
    char path[FILE_PATH];
    int ret;

    sprintf(path, "%s/%s/%s", s2ch.cwDir, FONT_DIR, s2ch.font.fileA);
    ret = sceIoGetstat(path, &st);
    if (ret < 0)
    {
//        psp2chNormalError(FILE_STAT_ERR, path);
		pgErrorDialog( 0, path );
        return -1;
    }
    size_fontA = st.st_size;
    free(fontA);
    fontA = (unsigned char*)malloc(st.st_size);
    if (fontA == NULL)
    {
//    	psp2chNormalError(MEM_ALLC_ERR, path);
		pgErrorDialog( 1, path );
        return -1;
    }
    fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
//    	psp2chNormalError(FILE_OPEN_ERR, path);
		pgErrorDialog( 2, path );
        return -1;
    }
    else
    {
        sceIoRead(fd, fontA, st.st_size);
        sceIoClose(fd);
    }
    sprintf(path, "%s/%s/%s", s2ch.cwDir, FONT_DIR, s2ch.font.fileJ);
    ret = sceIoGetstat(path, &st);
    if (ret < 0)
    {
//        psp2chNormalError(FILE_STAT_ERR, path);
		pgErrorDialog( 0, path );
        return -1;
    }
    size_fontJ = st.st_size;
    free(fontJ);
    fontJ = (unsigned char*)malloc(st.st_size);
    if (fontJ == NULL)
    {
//        psp2chNormalError(MEM_ALLC_ERR, path);
		pgErrorDialog( 1, path );
        return -1;
    }
    fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
    if (fd < 0)
    {
//        psp2chNormalError(FILE_OPEN_ERR, path);
		pgErrorDialog( 2, path );
        return -1;
    }
    else
    {
        sceIoRead(fd, fontJ, st.st_size);
        sceIoClose(fd);
    }
    return 0;
}

void pgSetupGu(void)
{
    framebuffer = (void*)0;
//    sceGuDisplay(GU_FALSE);
    sceGuInit();
    sceGuStart(GU_DIRECT, list);
    sceGuDrawBuffer(GU_PSM_8888, framebuffer, BUF_WIDTH);
    sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, (void*)0x88000, BUF_WIDTH);
    sceGuDepthBuffer((void*)0x110000, BUF_WIDTH);
    sceGuOffset(2048 - (SCR_WIDTH/2), 2048 - (SCR_HEIGHT/2));
    sceGuViewport(2048, 2048, SCR_WIDTH, SCR_HEIGHT);
    sceGuDepthRange(0xc350,0x2710);
    sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuAlphaFunc(GU_NOTEQUAL, 0, 0xF);
	sceGuEnable(GU_ALPHA_TEST);
	sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);
    sceGuEnable(GU_BLEND);
    sceGuEnable(GU_TEXTURE_2D);
    sceGuClutMode(GU_PSM_4444,0,0xff,0); // 16-bit palette
	sceGuClutLoad((256/8),colorPalette); // upload 32*8 entries (256)
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);
}

/*****************************
内蔵フォントをロード
*****************************/
void pgFontLoad(void)
{
    if (pgExtraFontInit() < 0)
    {
        s2ch.running = 0;
        sceKernelExitGame();
    }
}

void pgFontUnload(void)
{
	free(fontA);
	free(fontJ);
}

/***********************************
フォントのパラメータをセット
************************************/
void psp2chSetFontParam(void)
{
    switch(s2ch.font.pitch)
    {
    case 10:
        s2ch.font.lineH = 26;
        s2ch.font.lineV = 46;
        break;
    case 11:
        s2ch.font.lineH = 24;
        s2ch.font.lineV = 42;
        break;
    case 12:
        s2ch.font.lineH = 22;
        s2ch.font.lineV = 38;
        break;
    case 13:
        s2ch.font.lineH = 20;
        s2ch.font.lineV = 35;
        break;
    case 14:
        s2ch.font.lineH = 18;
        s2ch.font.lineV = 32;
        break;
    case 15:
        s2ch.font.lineH = 17;
        s2ch.font.lineV = 30;
        break;
    case 16:
        s2ch.font.lineH = 16;
        s2ch.font.lineV = 28;
        break;
    }
}

/***********************************
フォントサイズでメニュー・タイトルバーのパラメータをセット
************************************/
void psp2chSetBarParam(void)
{
	barSrcRect[0].left = 0;
	barSrcRect[0].top = 0;
	barSrcRect[0].right = SCR_WIDTH;
	barSrcRect[0].bottom = FONT_HEIGHT;

	barSrcRect[1].left = 0;
	barSrcRect[1].top = 0;
	barSrcRect[1].right = SCR_HEIGHT;
	barSrcRect[1].bottom = FONT_HEIGHT + LINE_PITCH;

	menuDstRect[0].left = 0;
	menuDstRect[0].top = SCR_HEIGHT - FONT_HEIGHT;
	menuDstRect[0].right = SCR_WIDTH;
	menuDstRect[0].bottom = SCR_HEIGHT;

	menuDstRect[1].left = 0;
	menuDstRect[1].top = 0;
	menuDstRect[1].right = FONT_HEIGHT + LINE_PITCH;
	menuDstRect[1].bottom = SCR_HEIGHT;

	titleDstRect[0].left = 0;
	titleDstRect[0].top = 0;
	titleDstRect[0].right = SCR_WIDTH;
	titleDstRect[0].bottom = FONT_HEIGHT;

	titleDstRect[1].left = SCR_WIDTH - (FONT_HEIGHT + LINE_PITCH);
	titleDstRect[1].top = 0;
	titleDstRect[1].right = SCR_WIDTH;
	titleDstRect[1].bottom = SCR_HEIGHT;
}

/*****************************
全レイヤーに対して画面設定を更新
*****************************/
void psp2chSetScreenParam(int axis)
{
	int i;
	for (i = 0; i < frame_num; i++) {
		if (axis)
			windowLayer[i].tateFlag = 1 - windowLayer[i].tateFlag;
		windowLayer[i].width = (windowLayer[i].tateFlag) ? SCR_HEIGHT : SCR_WIDTH;
		windowLayer[i].height = (windowLayer[i].tateFlag) ? SCR_WIDTH : SCR_HEIGHT;
		windowLayer[i].lineEnd = (windowLayer[i].tateFlag) ? DRAW_LINE_V : DRAW_LINE_H;
	}
}

/*****************************
CLUTを更新
*****************************/
void pgCLUTUpdate(void)
{
	sceKernelDcacheWritebackAll();
	sceGuStart(GU_DIRECT, list);
	sceGuClutLoad((256/8),colorPalette);
	sceGuFinish();
	sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
}

void pgTermGu(void)
{
    sceGuTerm();
}

void pgWaitVn(unsigned long count)
{
    while (count--) {
        sceDisplayWaitVblankStart();
    }
}

/*****************************
現在の書き込みバッファにおける座標x,yのアドレスを返す
printBufを変更することで書き込みバッファを変えられます
*****************************/
static inline unsigned char* pgGetVramAddr(unsigned long x,unsigned long y, int w)
{
    return printBuf + x + y * BUF_WIDTH * w;
}

/*****************************
左上座標x1,y1、幅、高さw,h、色colorで四角形を塗りつぶす
*****************************/
void pgFillvram(unsigned char color, int x1, int y1, unsigned int w, unsigned int h, int wide)
{
    unsigned char *vptr0;       //pointer to vram
    unsigned long i;

    vptr0 = pgGetVramAddr(x1, y1 & 0x01FF, wide);
    for (i = 0; i < h;) {
    	memset(vptr0, color, w);
        vptr0 += BUF_WIDTH * wide;
        if (((++i + y1)&0x01FF) == 0) {
            vptr0 -= ZBUF_SIZE * wide;
        }
    }
}

/*--------------------------------------------------------
    矩形範囲をコピー
--------------------------------------------------------*/
#define SLICE_SIZE 64
static void pgCopyRect(void *src, TEX *tex, RECT *src_rect, RECT *dst_rect)
{
    int j, sw, dw, sh, dh;
    struct Vertex *vertices;

    sw = src_rect->right - src_rect->left;
    dw = dst_rect->right - dst_rect->left;
    sh = src_rect->bottom - src_rect->top;
    dh = dst_rect->bottom - dst_rect->top;

    sceKernelDcacheWritebackAll();
    sceGuStart(GU_DIRECT, list);
    //sceGuDrawBufferList(GU_PSM_8888, framebuffer, BUF_WIDTH);
    sceGuScissor(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);
    sceGuTexMode(GU_PSM_T8, 0, 0, GU_FALSE); // 8-bit image
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);
    if (sw == dw && sh == dh)
        sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    else
        sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexImage(0, tex->w, tex->h, tex->tb, (char*)src);
    for (j = 0; j < sw; j = j + SLICE_SIZE)
    {
    	int width = (j + SLICE_SIZE < sw) ? SLICE_SIZE : sw - j;
        vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));
        vertices[0].u = src_rect->left + j;
        vertices[0].v = src_rect->top;
        vertices[0].color = 0;
        vertices[0].x = dst_rect->left + j * dw / sw;
        vertices[0].y = dst_rect->top;
        vertices[0].z = 0;
        vertices[1].u = src_rect->left + j + width;
        vertices[1].v = src_rect->bottom;
        vertices[1].color = 0;
        vertices[1].x = dst_rect->left + (j + width) * dw / sw;
        vertices[1].y = dst_rect->bottom;
        vertices[1].z = 0;
        sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_4444 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, vertices);
    }
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
}

/*--------------------------------------------------------
    矩形範囲を90度回転してコピー
--------------------------------------------------------*/
static void pgCopyRectRotate(void *src, TEX *tex, RECT *src_rect, RECT *dst_rect)
{
    short j, sw, dw, sh, dh;
    struct Vertex *vertices;

    sw = src_rect->right - src_rect->left;
    dw = dst_rect->right - dst_rect->left;
    sh = src_rect->bottom - src_rect->top;
    dh = dst_rect->bottom - dst_rect->top;

    sceKernelDcacheWritebackAll();
    sceGuStart(GU_DIRECT, list);
    //sceGuDrawBufferList(GU_PSM_8888, framebuffer, BUF_WIDTH);
    sceGuScissor(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);
    sceGuTexMode(GU_PSM_T8, 0, 0, GU_FALSE);
    sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
    sceGuTexWrap(GU_REPEAT, GU_REPEAT);
    if (sw == dh && sh == dw)
        sceGuTexFilter(GU_NEAREST, GU_NEAREST);
    else
        sceGuTexFilter(GU_LINEAR, GU_LINEAR);
    sceGuTexImage(0, tex->w, tex->h, tex->tb, (char*)src);
    for (j = 0; j < sw; j = j + SLICE_SIZE)
    {
    	int width = (j + SLICE_SIZE < sw) ? SLICE_SIZE : sw - j;
        vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));
        vertices[0].u = src_rect->left + j;
        vertices[0].v = src_rect->top;
        vertices[0].color = 0;
        vertices[0].x = dst_rect->right;
        vertices[0].y = dst_rect->top + j * dh / sw;
        vertices[0].z = 0;
        vertices[1].u = src_rect->left + j + width;
        vertices[1].v = src_rect->bottom;
        vertices[1].color = 0;
        vertices[1].x = dst_rect->left;
        vertices[1].y = dst_rect->top + (j + width) * dh / sw;
        vertices[1].z = 0;
        sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_4444 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, vertices);
    }
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
}

/*--------------------------------------------------------
    指定した矩形範囲を塗りつぶし
--------------------------------------------------------*/
static void pgFillRect(unsigned short color, RECT *rect)
{
    unsigned int r, g, b, a, c;
    // RGBA4444 => RGBA8888へ
    r = color & 0x0F;
    g = (color >> 4) & 0x0F;
    b = (color >> 8) & 0x0F;
    a = (color >> 12) & 0x0F;
    r = (r << 4) | r;
    g = (g << 4) | g;
    b = (b << 4) | b;
    a = (a << 4) | a;
    c = GU_RGBA(r,g,b,a);
    sceGuStart(GU_DIRECT, list);
    sceGuScissor(rect->left, rect->top, rect->right, rect->bottom);
    sceGuClearColor(c);
    sceGuClear(GU_COLOR_BUFFER_BIT);
    sceGuFinish();
    sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
}

/*****************************
タイトルバーを描画
*****************************/
void pgPrintTitleBar(char* ita, char* title, int size)
{
    unsigned char *temp;
    pspTime rtime;
    char date[20];
    char buf[32];

    sceRtcGetCurrentClockLocalTime(&rtime);
    sprintf(date, "%4dKB %02d:%02d:%02d", size/1024, rtime.hour, rtime.minutes, rtime.seconds);
    sprintf(buf, " [%s]", ita);
    temp = printBuf;
    printBuf = titlePixels; 
    winParam->pgCursorX = 0;
    if (winParam->tateFlag)
    {
        pgFillvram(s2ch.formColor.title_bg, 0, 0, SCR_HEIGHT, FONT_HEIGHT + LINE_PITCH + 1, 2);
        winParam->pgCursorY = 1;
        pgPrint(buf, s2ch.formColor.ita, s2ch.formColor.title_bg, SCR_HEIGHT);
        winParam->pgCursorX += 8;
        title = pgPrint(title, s2ch.formColor.title, s2ch.formColor.title_bg, SCR_HEIGHT);
        winParam->pgCursorY += LINE_PITCH;
        if (title)												// 折り返しがあるなら
        {
            winParam->pgCursorX = 0;
            pgPrint(title, s2ch.formColor.title, s2ch.formColor.title_bg, SCR_HEIGHT);
        }
        winParam->pgCursorX = SCR_HEIGHT - FONT_HEIGHT * (4+3) - 2;
        pgFillvram(s2ch.formColor.title_bg, winParam->pgCursorX, winParam->pgCursorY, SCR_HEIGHT - winParam->pgCursorX, FONT_HEIGHT, 2);
        pgPrint(date, s2ch.formColor.title, s2ch.formColor.title_bg, SCR_HEIGHT);
    }
    else
    {
        pgFillvram(s2ch.formColor.title_bg, 0, 0, SCR_WIDTH, FONT_HEIGHT, 2);
        winParam->pgCursorY = 0;
        pgPrint(buf, s2ch.formColor.ita, s2ch.formColor.title_bg, SCR_WIDTH);
        winParam->pgCursorX += 8;
        pgPrint(title, s2ch.formColor.title, s2ch.formColor.title_bg, SCR_WIDTH);
        winParam->pgCursorX = SCR_WIDTH - FONT_HEIGHT * (4+3) - 2;
        pgFillvram(s2ch.formColor.title_bg, winParam->pgCursorX, 0, SCR_WIDTH - winParam->pgCursorX, FONT_HEIGHT, 2);
        pgPrint(date, s2ch.formColor.title, s2ch.formColor.title_bg, SCR_WIDTH);
    }
    printBuf = temp;
}

/*****************************
タイトルバーを表示
*****************************/
void pgCopyTitleBar(void)
{
    pgCopyFunc[winParam->tateFlag](titlePixels, &tex, &barSrcRect[winParam->tateFlag], &titleDstRect[winParam->tateFlag]);
}

/*****************************
メニューバーを描画
*****************************/
void pgPrintMenuBar(char* str)
{
    unsigned char *temp;
    int battery;
    int batteryColor;

    temp = printBuf;
    printBuf = barPixels;
    winParam->pgCursorX = 0;
    if (winParam->tateFlag)
    {
        pgFillvram(s2ch.menuColor.bg, 0, 0, SCR_HEIGHT, FONT_HEIGHT + LINE_PITCH, 2);
        winParam->pgCursorY = 0;
        str = pgPrint(str, s2ch.menuColor.text, s2ch.menuColor.bg, SCR_HEIGHT);
        winParam->pgCursorY += LINE_PITCH;
        if (str)
        {
            winParam->pgCursorX = 0;
            pgPrint(str, s2ch.menuColor.text, s2ch.menuColor.bg, SCR_WIDTH);
        }
        winParam->pgCursorX = SCR_HEIGHT - FONT_HEIGHT * 3;
    }
    else
    {
        pgFillvram(s2ch.menuColor.bg, 0, 0, SCR_WIDTH, FONT_HEIGHT, 2);
        winParam->pgCursorY = 0;
        pgPrint(str, s2ch.menuColor.text, s2ch.menuColor.bg, SCR_WIDTH);
        winParam->pgCursorX = SCR_WIDTH - FONT_HEIGHT * 3;
    }
    battery = scePowerGetBatteryLifePercent();
    if (battery < 0)
    {
        pgPrint("  ---", s2ch.menuColor.bat1, s2ch.menuColor.bg, SCR_WIDTH);
    }
    else
    {
        if (battery > 40)
        {
            batteryColor = s2ch.menuColor.bat1;
        }
        else if (battery > 20)
        {
            batteryColor = s2ch.menuColor.bat2;
        }
        else
        {
            batteryColor = s2ch.menuColor.bat3;
        }
        pgPrintNumber(battery, batteryColor, s2ch.menuColor.bg);
        pgPrint("%", batteryColor, s2ch.menuColor.bg, SCR_WIDTH);
    }
    printBuf = temp;
}
/*****************************
メニューバーを表示
*****************************/
void pgCopyMenuBar(void)
{
    pgCopyFunc[winParam->tateFlag](barPixels, &tex, &barSrcRect[winParam->tateFlag], &menuDstRect[winParam->tateFlag]);
}

/*****************************
枠付で四角形を塗りつぶす
*****************************/
void pgEditBox(unsigned char color, int x1, int y1, int x2, int y2)
{
    unsigned char *vptr0;       //pointer to vram
    long i, j;

    if (x1 > x2)
    {
        return;
    }
    if (y1 > y2)
    {
        return;
    }
    x1 -= 2;
    y1 -= 2;
    x2++;
    y2++;
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 > SCR_WIDTH) {
        x2 = SCR_WIDTH;
    }
    if (y2 > SCR_HEIGHT) {
        y2 = SCR_HEIGHT;
    }
    vptr0 = pgGetVramAddr(0, y1, 2);
    for (j = x1; j < x2; j++) {
        vptr0[j] = 1;
    }
    vptr0 += BUF_WIDTH * 2;
    vptr0[x1+0] = 1;
    for (j = x1+1; j < x2-1; j++) {
        vptr0[j] = 2;
    }
    vptr0[x2-1] = 1;
    for (i = y1+2; i < y2-1; i++) {
        vptr0 += BUF_WIDTH * 2;
        vptr0[x1+0] = 1;
        vptr0[x1+1] = 2;
        for (j = x1+2; j < x2-1; j++) {
            vptr0[j] = color;
        }
        vptr0[x2-1] = 1;
    }
    vptr0 += BUF_WIDTH * 2;
    for (j = x1; j < x2; j++) {
        vptr0[j] = 1;
    }
}

/*****************************
枠だけVRAMに直接表示
*****************************/
void pgWindowFrame(int x1, int y1, int x2, int y2)
{
    unsigned int *vptr0;       //pointer to vram
    long i, j;
    int tmp;

    if (winParam->tateFlag)
    {
        tmp = x1;
        x1 = SCR_WIDTH - y2 + 1;
        y2 = x2;
        x2 = SCR_WIDTH - y1;
        y1 = tmp;
    }
    if (x1 > x2)
    {
        return;
    }
    if (y1 > y2)
    {
        return;
    }
    x1 -= 2;
    y1 -= 2;
    x2++;
    y2++;
    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 > SCR_WIDTH) {
        x2 = SCR_WIDTH;
    }
    if (y2 > SCR_HEIGHT) {
        y2 = SCR_HEIGHT;
    }
    vptr0 = (unsigned int*)(framebuffer + 0x04000000) + y1 * BUF_WIDTH;
    for (j = x1; j < x2; j++) {
        vptr0[j] = GU_RGBA(0x99, 0x99, 0x99, 0xFF);
    }
    vptr0 += BUF_WIDTH;
    vptr0[x1+0] = GU_RGBA(0x99, 0x99, 0x99, 0xFF);
    for (j = x1+1; j < x2-1; j++) {
        vptr0[j] = GU_RGBA(0x33, 0x33, 0x33, 0xFF);
    }
    vptr0[x2-1] = GU_RGBA(0x99, 0x99, 0x99, 0xFF);
    for (i = y1+2; i < y2-1; i++) {
        vptr0 += BUF_WIDTH;
        vptr0[x1+0] = GU_RGBA(0x99, 0x99, 0x99, 0xFF);
        vptr0[x1+1] = GU_RGBA(0x33, 0x33, 0x33, 0xFF);
        vptr0[x2-1] = GU_RGBA(0x99, 0x99, 0x99, 0xFF);
    }
    vptr0 += BUF_WIDTH;
    for (j = x1; j < x2; j++) {
        vptr0[j] = GU_RGBA(0x99, 0x99, 0x99, 0xFF);
    }
}

/*****************************
スクロールバー表示
*****************************/
void pgScrollbar(S_SCROLLBAR* bar, S_2CH_BAR_COLOR c)
{
    int sliderH, sliderY;
    RECT rect;

	if (bar==NULL) return;										// スクロールバー構造体が指定されていないなら処理しない

    sliderH = bar->view * bar->h / bar->total;
    sliderY = bar->start * bar->h / bar->total + bar->y;
    if (sliderH < 2) {
        sliderH = 2;
    }
    if (sliderY >= (bar->y + bar->h - 1)) {
        sliderY = bar->y + bar->h - 2;
    }
    if (winParam->tateFlag)
    {
        rect.left = SCR_WIDTH - bar->y - bar->h;
        rect.top = bar->x;
        rect.right = SCR_WIDTH - bar->y;
        rect.bottom = bar->x + bar->w;
        pgFillRect(colorPalette[c.bg], &rect);
        rect.left = SCR_WIDTH - sliderY - sliderH;
        rect.top = bar->x + 1;
        rect.right = SCR_WIDTH - sliderY;
        rect.bottom = bar->x + bar->w;
    }
    else
    {
        rect.left = bar->x;
        rect.top = bar->y;
        rect.right = bar->x + bar->w;
        rect.bottom = bar->y + bar->h;
        pgFillRect(colorPalette[c.bg], &rect);
        rect.left = bar->x + 1;
        rect.top = sliderY;
        rect.right = bar->x + bar->w;
        rect.bottom = sliderY + sliderH;
    }
    pgFillRect(colorPalette[c.slider], &rect);
}

/*****************************
矢印カーソルを表示
*****************************/
void pgPadCursor(int x, int y, int type)
{
    RECT src_rect, dst_rect;
    TEX cur = {32, 32, 32};
    
    if (winParam->tateFlag)
    {
        src_rect.left = 0;
        src_rect.top = 0;
        src_rect.right = cur.w;
        src_rect.bottom = cur.h;
        dst_rect.left = SCR_WIDTH - y - cur.h + 13;
        dst_rect.top = x;
        dst_rect.right = SCR_WIDTH - y + 13;
        dst_rect.bottom = x + cur.w;
    }
    else
    {
        src_rect.left = 0;
        src_rect.top = 13;
        src_rect.right = cur.w;
        src_rect.bottom = 13+cur.h;
        dst_rect.left = x;
        dst_rect.top = y;
        dst_rect.right = x + cur.w;
        dst_rect.bottom = y + cur.h;
    }
    pgCopyFunc[winParam->tateFlag](cursorImg[type], &cur, &src_rect, &dst_rect);
}

/*****************************
現在の書き込みバッファをoffsetの位置からVRAMバッファへウィンドウ転送
*****************************/
void pgCopyWindow(int offset, int x, int y, int w, int h)
{
    int offsetY = (offset + y) & 0x01FF;
    RECT src_rect, dst_rect;

    offsetY &= 0x01FF;
    if (winParam->tateFlag)
    {
        src_rect.left = x;
        src_rect.top = offsetY;
        src_rect.right = x + w;
        src_rect.bottom = offsetY + h;
        dst_rect.left = SCR_WIDTH - y - h;
        dst_rect.top = x;
        dst_rect.right = SCR_WIDTH - y;
        dst_rect.bottom = x + w;
    }
    else
    {
        src_rect.left = x;
        src_rect.top = offsetY;
        src_rect.right = x + w;
        src_rect.bottom = offsetY + h;
        dst_rect.left = x;
        dst_rect.top = y;
        dst_rect.right = x + w;
        dst_rect.bottom = y + h;
    }
    pgCopyFunc[winParam->tateFlag](printBuf, &tex, &src_rect, &dst_rect);
}

/*****************************
現在の書き込みバッファをoffsetの位置からVRAMバッファに全画面転送
*****************************/

void pgCopy(int offsetX, int offsetY)
{
    RECT src_rect, dst_rect;
    int sx;

    sx = offsetX & 0x1F;
    offsetX &= 0xFFFFFFE0;
    offsetY &= 0x01FF;
    if (winParam->tateFlag)
    {
    	src_rect.left = sx;
        src_rect.top = offsetY;
        src_rect.right = sx + SCR_HEIGHT;
        src_rect.bottom = offsetY + SCR_WIDTH;
        dst_rect.left = 0;
        dst_rect.top = 0;
        dst_rect.right = SCR_WIDTH;
        dst_rect.bottom = SCR_HEIGHT;
    }
    else
    {
    	src_rect.left = sx;
        src_rect.top = offsetY;
        src_rect.right = sx + SCR_WIDTH;
        src_rect.bottom = offsetY + SCR_HEIGHT;
        dst_rect.left = 0;
        dst_rect.top = 0;
        dst_rect.right = SCR_WIDTH;
        dst_rect.bottom = SCR_HEIGHT;
    }
    pgCopyFunc[winParam->tateFlag](printBuf + offsetX, &tex, &src_rect, &dst_rect);
}

/*****************************
数字を表示
*****************************/
void pgPrintNumber(int num, int color,int bgcolor)
{
    unsigned char *vptr0, *vptr;
    char buf[16];
    int i, j, cx, cy, b, count;
    unsigned short* font;

    winParam->pgCursorY &= 0x01FF;
    sprintf(buf, "%d", num);
    count = strlen(buf);
    // '0'の文字幅を使う
    font = (unsigned short*)(fontA + (('0' - 0x20) << 5));
    cx = *font;
    winParam->pgCursorX += (4 - count) * cx;
    for (j = 0; j < count;j++) {
        font = (unsigned short*)(fontA + ((buf[j] - 0x20) << 5));
        cx = *font++;
        vptr0 = pgGetVramAddr(winParam->pgCursorX, winParam->pgCursorY, 2);
        winParam->pgCursorX += cx;
        for (cy = 0; cy < FONT_HEIGHT; cy++) {
            vptr = vptr0;
            b = 0x8000;
            for (i = 0; i < cx; i++) {
                if (*font & b)
                	*vptr = color;
                vptr++;
                b >>= 1;
            }
            vptr0 += BUF_WIDTH * 2;
            if (vptr0 >= printBuf + ZBUF_SIZE * 2) {
                vptr0 -= ZBUF_SIZE * 2;
            }
            font++;
        }
    }
}

/*****************************
1文字をフォントから読み込んでwidth内で表示可能なら表示して0を返す
widthを超える場合は表示しないで1を返す
*****************************/
// 1バイト文字(ASCII, 半角カナ
static int pgPutCharA(const unsigned char c)
{
    unsigned long index = c;
    unsigned char *vptr0;       //pointer to vram
    unsigned char *vptr;        //pointer to vram
    unsigned short *font;
    int cx, cy, i, b;

    if (index < 0x20) {
        return 0;
    } else if (index < 0x80) {
        index -= 0x20;
    } else if (index > 0xa0) {
        index -= 0x41;
    } else {
        return 0;
    }
    if ((index << 5) >= size_fontA) {
        index = '?' - 0x20;
    }
    font = (unsigned short*)(fontA + (index<<5));
    cx = *font++;
    if ((winParam->pgCursorX + cx) >= sChar.width) {
        return 1;
    }
    winParam->pgCursorY &= 0x01FF;
    vptr0 = pgGetVramAddr(winParam->pgCursorX, winParam->pgCursorY, 2);
    winParam->pgCursorX += cx;
    for (cy = 0; cy < FONT_HEIGHT; cy++) {
        vptr = vptr0;
        b = 0x8000;
        for (i = 0; i < cx; i++) {
            if (*font & b)
            	*vptr = sChar.color;
            vptr++;
            b >>= 1;
        }
        vptr0 += BUF_WIDTH * 2;
        if (vptr0 >= printBuf + ZBUF_SIZE * 2) {
            vptr0 -= ZBUF_SIZE * 2;
        }
        font++;
    }
    return 0;
}
// 2バイト文字
static int pgPutCharW(unsigned char hi,unsigned char lo)
{
    unsigned long index;
    unsigned char *vptr0;
    unsigned char *vptr;
    unsigned short *font;
    int cx, cy, i, b;

    // sjis2jis
    hi -= (hi <= 0x9f) ? 0x71 : 0xb1;
    hi <<= 1;
    hi++;
    if (lo > 0x7f)
        lo--;
    if (lo >= 0x9e) {
        lo -= 0x7d;
        hi++;
    }
    else {
        lo -= 0x1f;
    }
    // hi : 0x21-0x7e, lo : 0x21-0x7e
    hi -= 0x21;
    lo -= 0x21;
    index = hi * (0x7e - 0x20);
    index += lo;
    if ((index << 5) >= size_fontJ) {
        index = 8; // '？'
    }

    font = (unsigned short*)(fontJ + (index<<5));
    cx = *font++;
    if ((winParam->pgCursorX + cx) >= sChar.width) {
        return 1;
    }
    winParam->pgCursorY &= 0x01FF;
    vptr0 = pgGetVramAddr(winParam->pgCursorX, winParam->pgCursorY, 2);
    winParam->pgCursorX += cx;
    for (cy = 0; cy < FONT_HEIGHT; cy++) {
        vptr = vptr0;
        b = 0x8000;
        for (i = 0; i < cx; i++) {
            if (*font & b)
            	*vptr = sChar.color;
            vptr++;
            b >>= 1;
        }
        vptr0 += BUF_WIDTH * 2;
        if (vptr0 >= printBuf + ZBUF_SIZE * 2) {
            vptr0 -= ZBUF_SIZE * 2;
        }
        font++;
    }
    return 0;
}

/*****************************
実体参照を変換
*****************************/
static int pgSpecialChars(char** string)
{
    int i, val;
    char* str;
    unsigned char hi, lo;

    str = *string + 1;
    // Unicode変換
    if (*str == '#') {
        str++;
        if (*str == 'x') {
        	val = strtoul(str, &str, 16);
        }
        else {
	        val = strtoul(str, &str, 10);
	    }
        if (val) {
            *string = str;
            ucs2sjis(&val, val);
            lo = val >> 8;
            hi = val & 0xFF;
            if (lo) {
                return pgPutCharW(hi, lo);
            }
            else if (hi) {
                return pgPutCharA(hi);
            }
            // 変換できない文字
            else {
                return pgPutCharA('?');
            }
        }
    }
    else {
        for (i = 0; i < MAX_ENTITIES; i++) {
            if (memcmp(*string, entity[i].str, entity[i].len) == 0) {
                (*string) += entity[i].len;
                if (entity[i].byte == 1) {
                    return pgPutCharA(entity[i].c1);
                }
                else {
                    return pgPutCharW(entity[i].c1, entity[i].c2);
                }
            }
        }
    }
    return pgPutCharA('&');
}

/*****************************
文字列strを画面幅widthで1行分表示して改行部分のポインタを返す
strを全部表示したらNULLを返す
*****************************/
char* pgPrint(char *str,unsigned char color,unsigned char bgcolor, int width)
{
    unsigned char ch = 0,bef = 0;
    int ret = 0;

    sChar.color = color;
    sChar.bgcolor = bgcolor;
    sChar.width = width;
    while(*str) {
        ch = (unsigned char)*str;
        if (bef!=0) {
            ret = pgPutCharW(bef, ch);
            if (ret) { // 改行部の位置を返す
                return --str;
            }
            bef=0;
        } else {
            if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
                bef = ch;
            } else {
                if (ch == '&') {
                    ret = pgSpecialChars((char**)(&str));
                }
                else if (ch == '\n') {
                    ret = 1;
                    str++;
                }
                else {
                    ret = pgPutCharA(ch);
                }
                if (ret) {
                    return str;
                }
                bef = 0;
            }
        }
        str++;
    }
    return NULL;
}

static void idEnd(void)
{
	s2ch.idAnchor[s2ch.idAnchorCount].x2 = winParam->pgCursorX + 6;
	if (s2ch.idAnchor[s2ch.idAnchorCount].x2 - s2ch.idAnchor[s2ch.idAnchorCount].x1 < 12){
		s2ch.idAnchor[s2ch.idAnchorCount].x2 += 6;				// アンカーの幅が10以下だとアンカー間移動で不都合が起こるので
	}
	s2ch.idAnchorCount++;
	if (s2ch.idAnchorCount >= 40) {
		s2ch.idAnchorCount = 0;
	}
	s2ch.idAnchor[s2ch.idAnchorCount].x1 = 0;
}

static void beEnd(void)
{
	s2ch.beAnchor[s2ch.beAnchorCount].x2 = winParam->pgCursorX + 6;
	if (s2ch.beAnchor[s2ch.beAnchorCount].x2 - s2ch.beAnchor[s2ch.beAnchorCount].x1 < 12){
		s2ch.beAnchor[s2ch.beAnchorCount].x2 += 6;				// アンカーの幅が10以下だとアンカー間移動で不都合が起こるので
	}
	s2ch.beAnchorCount++;
	if (s2ch.beAnchorCount >= 40) {
		s2ch.beAnchorCount = 0;
	}
	s2ch.beAnchor[s2ch.beAnchorCount].x1 = 0;
}

static void resEnd(void)
{
    s2ch.resAnchor[s2ch.resAnchorCount].x2 = winParam->pgCursorX + 6;
    s2ch.resAnchorCount++;
    if (s2ch.resAnchorCount >= 50) {
        s2ch.resAnchorCount = 0;
    }
    s2ch.resAnchor[s2ch.resAnchorCount].x1 = 0;
}

static void urlEnd(void)
{
    s2ch.urlAnchor[s2ch.urlAnchorCount].x2 = winParam->pgCursorX + 6;
	if (s2ch.urlAnchor[s2ch.urlAnchorCount].x2 - s2ch.urlAnchor[s2ch.urlAnchorCount].x1 < 12){
		s2ch.urlAnchor[s2ch.urlAnchorCount].x2 += 6;				// アンカーの幅が10以下だとアンカー間移動で不都合が起こるので
	}
    s2ch.urlAnchorCount++;
    if (s2ch.urlAnchorCount >= 50) {
        s2ch.urlAnchorCount = 0;
    }
    s2ch.urlAnchor[s2ch.urlAnchorCount].x1 = 0;
}

/*****************************
文字列strを画面幅widthで1行分表示して改行部分のポインタを返す
strを全部表示したらNULLを返す
<br>で改行
他のHTMLタグは削除
レスアンカーやURLアンカーがあれば位置情報を保存
*****************************/

//===== URLアンカー登録判定 =====

static void pgPrintHtmlHttp(char *str, char *http, int type, S_2CH_RES_COLOR *c,int drawLine,int *anchorOn)
{
	int j;
	char *p;

	if (memcmp(str, http, strlen(http)) == 0) {
		s2ch.urlAnchor[s2ch.urlAnchorCount].x1 = winParam->pgCursorX;
		s2ch.urlAnchor[s2ch.urlAnchorCount].line = drawLine;
		sChar.color = c->link;
	}
	else if (memcmp(str, (http+1), strlen((http+1))) == 0) {
		if (s2ch.urlAnchor[s2ch.urlAnchorCount].x1 == 0) {
			s2ch.urlAnchor[s2ch.urlAnchorCount].x1 = winParam->pgCursorX;
			s2ch.urlAnchor[s2ch.urlAnchorCount].line = drawLine;
			sChar.color = c->link;
		}
	}
	else if (memcmp(str, (http+2), strlen((http+2))) == 0){
		if (s2ch.urlAnchor[s2ch.urlAnchorCount].x1 == 0) {
			s2ch.urlAnchor[s2ch.urlAnchorCount].x1 = winParam->pgCursorX;
			s2ch.urlAnchor[s2ch.urlAnchorCount].line = drawLine;
		}
		s2ch.urlAnchor[s2ch.urlAnchorCount].http = type;		// 0 : http// , 1 : https//
		p = str + strlen((http+2));
		j = 0;
		while (isgraph((unsigned char)*p) && *p != '<') {
			if (*p == '/') {
				p++;
				break;
			}
			s2ch.urlAnchor[s2ch.urlAnchorCount].host[j] = *p++;
			j++;
			if (j >= 63) {
				break;
			}
		}
		if (j < 64) {
			s2ch.urlAnchor[s2ch.urlAnchorCount].host[j] = '\0';
			j = 0;
			while (isgraph((unsigned char)*p) && *p != '<') {
				s2ch.urlAnchor[s2ch.urlAnchorCount].path[j] = *p++;
				j++;
				if (j >= 255) {
					break;
				}
			}
			if (j < 256) {
				s2ch.urlAnchor[s2ch.urlAnchorCount].path[j] = '\0';
			}
			*anchorOn = 1;
			sChar.color = c->link;
		}
	}
}

//===== メイン =====

char* pgPrintHtml(char *str, S_2CH_RES_COLOR *c, int startX, int width,int drawLine)
{
    static int anchorOn = 0;
    unsigned char ch = 0,bef = 0;
    int ret = 0;
    char *p;
    int i, j, start, end;

    sChar.bgcolor = c->bg;
    sChar.width = width;
    if (anchorOn && anchorOn!=3) {
        sChar.color = c->link;
    }
    else {
        sChar.color = c->text;
    }
    
    while(*str) {
        ch = (unsigned char)*str;
        if (bef!=0) {
            ret = pgPutCharW(bef, ch);
            if (ret) {
				if (anchorOn == 3){
					idEnd();
					s2ch.idAnchor[s2ch.idAnchorCount].line = drawLine+1;
					s2ch.idAnchor[s2ch.idAnchorCount].x1 = startX;
					strcpy(s2ch.idAnchor[s2ch.idAnchorCount].id, s2ch.idAnchor[s2ch.idAnchorCount-1].id);
					anchorOn = 0;
				}
				if (anchorOn == 4){
					beEnd();
					s2ch.beAnchor[s2ch.beAnchorCount].line = drawLine+1;
					s2ch.beAnchor[s2ch.beAnchorCount].x1 = startX;
					strcpy(s2ch.beAnchor[s2ch.beAnchorCount].be, s2ch.beAnchor[s2ch.beAnchorCount-1].be);
					anchorOn = 0;
				}
                return --str;
            }
            bef=0;
        } else {
            if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
                bef = ch;
				if (memcmp(str, "発信元:", strlen("発信元:")) == 0) {
					p = str + strlen("発信元:");
					if (*p && isgraph((unsigned char)*p) && *p != '<') {
						s2ch.idAnchor[s2ch.idAnchorCount].x1 = winParam->pgCursorX;
						s2ch.idAnchor[s2ch.idAnchorCount].line = drawLine;
						j = 0;
						while (isgraph((unsigned char)*p) && *p != '<') {
							s2ch.idAnchor[s2ch.idAnchorCount].id[j] = *p++;
							j++;
							if (j >= 16)
								break;
						}
						s2ch.idAnchor[s2ch.idAnchorCount].id[j] = '\0';
						anchorOn = 3;
						//sChar.color = c->link;
					}
				}
				if (anchorOn != 3){
					switch (anchorOn) {
					case 1: urlEnd(); break;
					case 2: resEnd(); break;
					case 3: idEnd(); break;
					case 4: beEnd(); break;
					}
					anchorOn = 0;
					sChar.color = c->text;
				}
            } else {
                if (ch >= 0xa0 && ch < 0xe0) {
                    anchorOn = 0;
                    sChar.color = c->text;
                }
                else {
                	switch (anchorOn) {
                	case 1:
                		if (!isgraph(ch) && ch != '<')
	                    {
	                    	anchorOn = 0;
	                    	sChar.color = c->text;
	                    	urlEnd();
	                    }
	                    break;
	                case 2:
	                	if (!isdigit(ch) && ch != '-' && ch != ',' && ch !='<') {
	                        anchorOn = 0;
	                        sChar.color = c->text;
	                        resEnd();
	                    }
	                    break;
	                case 3:
						if (!isgraph(ch) && ch != '<') {
	                		anchorOn = 0;
	                		sChar.color = c->text;
	                		idEnd();
	                	}
	                	break;
	                }
	            }

				pgPrintHtmlHttp(str, "http://", 0, c, drawLine, &anchorOn);
				pgPrintHtmlHttp(str, "https://", 1, c, drawLine, &anchorOn);

				if (memcmp(str, "ID:", strlen("ID:")) == 0) {
					p = str + strlen("ID:");
					if (*p && isgraph((unsigned char)*p) && *p != '<') {
						s2ch.idAnchor[s2ch.idAnchorCount].x1 = winParam->pgCursorX;
						s2ch.idAnchor[s2ch.idAnchorCount].line = drawLine;
						j = 0;
						while (isgraph((unsigned char)*p) && *p != '<') {
							s2ch.idAnchor[s2ch.idAnchorCount].id[j] = *p++;
							j++;
							if (j >= 11)
								break;
						}
						s2ch.idAnchor[s2ch.idAnchorCount].id[j] = '\0';
						anchorOn = 3;
						//sChar.color = c->link;
					}
				}

				if (memcmp(str, "BE:", strlen("BE:")) == 0) {
					p = str + strlen("BE:");
					if (*p && isgraph((unsigned char)*p) && *p != '<' && *p != '-') {
						s2ch.beAnchor[s2ch.beAnchorCount].x1 = winParam->pgCursorX;
						s2ch.beAnchor[s2ch.beAnchorCount].line = drawLine;
						j = 0;
						while (isgraph((unsigned char)*p) && *p != '<' && *p != '-') {
							s2ch.beAnchor[s2ch.beAnchorCount].be[j] = *p++;
							j++;
							if (j >= 11)
								break;
						}
						s2ch.beAnchor[s2ch.beAnchorCount].be[j] = '\0';
						if (*p=='-') str = p;					// Be基礎番号は表示しない
						anchorOn = 4;
						ch = '?';								// "BE:"→"?"に置き換え
					}
				}

                if (ch == '<') {
                    switch (anchorOn) {
                    case 1: urlEnd(); break;
                    case 2: resEnd(); break;
                    case 3: idEnd(); break;
					case 4: beEnd(); break;
                    }
                    anchorOn = 0;
                    sChar.color = c->text;
                    
                    if (memcmp(str, "<br>", strlen("<br>")) == 0) {
                        str += strlen("<br>");
                        return str;
                    }
                    else {
                        str = strchr(str, '>');
						if (str == NULL) return NULL;			// '>'の無いタグがあるとフリーズするので
                        str++;
                        continue;
                    }
                }
                else if (ch == '&') {
                    if (memcmp(str, "&gt;", strlen("&gt;")) == 0) {
                        if ((memcmp((str + 4), "&gt;", strlen("&gt;")) == 0) && isdigit((unsigned char)*(str + 8))) {
                            s2ch.resAnchor[s2ch.resAnchorCount].x1 = winParam->pgCursorX;
                            s2ch.resAnchor[s2ch.resAnchorCount].line = drawLine;
                            sChar.color = c->link;
                        }
                        else if (isdigit((unsigned char)*(str + 4))) {
                            if (s2ch.resAnchor[s2ch.resAnchorCount].x1 == 0) {
                                s2ch.resAnchor[s2ch.resAnchorCount].x1 = winParam->pgCursorX;
                                s2ch.resAnchor[s2ch.resAnchorCount].line = drawLine;
                            }
                            j = 0;
                            p = str + 4;
                            while (isdigit((unsigned char)*p) || *p == '-' || *p == ',' || *p == '<') {
                                start = 0;
                                while (isdigit((unsigned char)*p)) {
                                    start = start * 10 + *p - '0';
                                    p++;
                                }
                                if (start <= s2ch.res.count && start > 0) {
                                    s2ch.resAnchor[s2ch.resAnchorCount].res[j] = start - 1;
                                    j++;
                                }
                                if (*p == '-') {
                                    end = 0;
                                    p++;
                                    while (isdigit((unsigned char)*p)) {
                                        end = end * 10 + *p - '0';
                                        p++;
                                    }
                                    for (i = start; i < end; i++) {
                                        if (i >= s2ch.res.count) {
                                            break;
                                        }
                                        if (i > 0) {
                                            s2ch.resAnchor[s2ch.resAnchorCount].res[j] = i;
                                            j++;
                                        }
                                    }
                                }
                                if (*p == '<') {
                                    p = strchr(p, '>');
                                    p++;
                                }
                                if (*p != ',') {
                                    break;
                                }
                                p++;
                            }
                            s2ch.resAnchor[s2ch.resAnchorCount].resCount = j;
                            anchorOn = 2;
                            sChar.color = c->link;
                        }
                    }
                    ret = pgSpecialChars((char**)(&str));
                }
                else if (ch == ' ' || ch == '\n') {
                    while (ch == ' ' || ch == '\n') {
                        ch = *(++str);
                    }
                    anchorOn = 0;
                    sChar.color = c->text;
                    str--;
                    ret = pgPutCharA(' ');
                }
                else {
                    ret = pgPutCharA(ch);
                }
                
                // 改行を含む場合
                if (ret) {
                	switch (anchorOn) {
                	case 1:
                		urlEnd();
                        s2ch.urlAnchor[s2ch.urlAnchorCount].line = drawLine+1;
                        s2ch.urlAnchor[s2ch.urlAnchorCount].x1 = startX;
						s2ch.urlAnchor[s2ch.urlAnchorCount].http = s2ch.urlAnchor[s2ch.urlAnchorCount-1].http;
                        strcpy(s2ch.urlAnchor[s2ch.urlAnchorCount].host, s2ch.urlAnchor[s2ch.urlAnchorCount-1].host);
                        strcpy(s2ch.urlAnchor[s2ch.urlAnchorCount].path, s2ch.urlAnchor[s2ch.urlAnchorCount-1].path);
                        break;
                	case 2:
                		resEnd();
                        for (i = 0; i < s2ch.resAnchor[s2ch.resAnchorCount-1].resCount; i++) {
                            s2ch.resAnchor[s2ch.resAnchorCount].res[i] = s2ch.resAnchor[s2ch.resAnchorCount-1].res[i];
                        }
                        s2ch.resAnchor[s2ch.resAnchorCount].resCount = s2ch.resAnchor[s2ch.resAnchorCount-1].resCount;
                        s2ch.resAnchor[s2ch.resAnchorCount].line = drawLine+1;
                        s2ch.resAnchor[s2ch.resAnchorCount].x1 = startX;
                        break;
                	case 3:
                		idEnd();
                		s2ch.idAnchor[s2ch.idAnchorCount].line = drawLine+1;
                		s2ch.idAnchor[s2ch.idAnchorCount].x1 = startX;
                		strcpy(s2ch.idAnchor[s2ch.idAnchorCount].id, s2ch.idAnchor[s2ch.idAnchorCount-1].id);
                		break;
                	case 4:
                		beEnd();
                		s2ch.beAnchor[s2ch.beAnchorCount].line = drawLine+1;
                		s2ch.beAnchor[s2ch.beAnchorCount].x1 = startX;
                		strcpy(s2ch.beAnchor[s2ch.beAnchorCount].be, s2ch.beAnchor[s2ch.beAnchorCount-1].be);
                		break;
                	}
                    return str;
                }
            }
        }
        str++;
    }
    // アンカーのすぐ後ろに'\0'がきた場合
    switch (anchorOn) {
    case 1: urlEnd(); break;
    case 2: resEnd(); break;
    case 3: idEnd(); break;
	case 4: beEnd(); break;
    }
    anchorOn = 0;
    return NULL;
}

static int pgCountCharA(const unsigned char c, int width)
{
    unsigned long index;
    unsigned short *font;
	int cx;

    index = c;
    if (c < 0x20) {
        return 0;
    } else if (c < 0x80) {
        index -= 0x20;
    } else if (c > 0xa0) {
        index -= 0x41;
    } else {
        return 0;
    }
    if ((index<<5) >= size_fontA) {
        index = '?' - 0x20;
    }
    font = (unsigned short*)(fontA + (index<<5));
	cx = *font;
    if ((winParam->pgCursorX + cx) >= width) {
        return 1;
    }
    winParam->pgCursorX += cx;
    return 0;
}

static int pgCountCharW(unsigned char hi,unsigned char lo, int width)
{
    unsigned long index;
    unsigned short *font;
	int cx;

    // sjis2jis
    hi -= (hi <= 0x9f) ? 0x71 : 0xb1;
    hi <<= 1;
    hi++;
    if (lo > 0x7f)
        lo--;
    if (lo >= 0x9e) {
        lo -= 0x7d;
        hi++;
    }
    else {
        lo -= 0x1f;
    }
    // hi : 0x21-0x7e, lo : 0x21-0x7e
    index = ((hi - 0x21) * (0x7e - 0x20)) + (lo - 0x21);
    if ((index<<5) >= size_fontJ) {
        index = 8; // '？'
    }
    font = (unsigned short*)(fontJ + (index<<5));
	cx = *font;
    if ((winParam->pgCursorX + cx) >= width) {
        return 1;
    }
    winParam->pgCursorX += cx;
    return 0;
}

static int pgCountSpecialChars(char** string, int width)
{
    int i, val;
    char* str;
    unsigned char hi, lo;

    str = *string + 1;
    if (*str == '#') {
        str++;
        if (*str == 'x') {
        	val = strtoul(str, &str, 16);
        }
        else {
	        val = strtoul(str, &str, 10);
	    }
        if (val) {
            *string = str;
            ucs2sjis(&val, val);
            lo = val >> 8;
            hi = val & 0xFF;
            if (lo) {
                return pgCountCharW(hi, lo, width);
            }
            else if (hi) {
                return pgCountCharA(hi, width);
            }
            else {
                return pgCountCharA('?', width);
            }
        }
    }
    else {
        for (i = 0; i < MAX_ENTITIES; i++) {
            if (memcmp(*string, entity[i].str, entity[i].len) == 0) {
                (*string) += entity[i].len;
                if (entity[i].byte == 1) {
                    return pgCountCharA(entity[i].c1, width);
                }
                else {
                    return pgCountCharW(entity[i].c1, entity[i].c2, width);
                }
            }
        }
    }
    return pgCountCharA('&', width);
}

/*****************************
strを画面幅widthで表示したときの行数を数えるのに使う関数
表示は行われない
*****************************/
char* pgCountHtml(char *str, int width, int specialchar)
{
    unsigned char ch = 0,bef = 0;
	char *p;
    int ret = 0;

    while(*str) {
        ch = (unsigned char)*str;
        if (bef!=0) {
            ret = pgCountCharW(bef, ch, width);
            if (ret) {
                return --str;
            }
            bef=0;
        } else {
			if (memcmp(str, "BE:", strlen("BE:")) == 0) {
				p = strchr(str, '-');							// Be基礎番号は表示しない
				if (p) {
					str = p;
					ch = '?';									// "BE:"→"?"に置き換え
				}
			}
            if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
                bef = ch;
            } else {
                if (ch == '<') {
                    if (memcmp(str, "<br>", strlen("<br>")) == 0) {
                        str +=4;
                        return str;
                    }
                    else {
                        str = strchr(str, '>');
						if (str == NULL) return NULL;			// '>'の無いタグがあるとフリーズするので
                        str++;
                        continue;
                    }
                }
                else if (ch == '&') {
                    ret = pgCountSpecialChars((char**)(&str), width);
                }
                else if (ch == ' ') {
                    while (ch == ' ') {
                        ch = *(++str);
                    }
                    str--;
                    ret = pgCountCharA(' ', width);
                }
                else {
                    ret = pgCountCharA(ch, width);
                }
                if (ret) {										// 折り返すなら
                    return str;
                }
            }
        }
        str++;
    }
    return NULL;
}

void pgPrintOwata(void)
{
    pgFillvram(BLACK, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
    winParam->pgCursorX = 160;
    winParam->pgCursorY = 80;
    pgPrint("人生ｵﾜﾀ＼(^o^)／", WHITE, BLACK, SCR_WIDTH);
}

void flipScreen(int mode)
{
    // パレット用
    if (dispPalette)
    {
    	printBuf = titlePixels;
    	winParam->pgCursorX = winParam->pgCursorY = 0;
    	pgFillvram(12, 0, 0, SCR_WIDTH, FONT_HEIGHT, 2);
    	pgPrint(palette, WHITE, BLACK, SCR_WIDTH);
    	pgCopyTitleBar();
    	printBuf = winParam->frame;
    }
    // 更新処理
    preframeBuffer = framebuffer;
    sceDisplayWaitVblankStart();
    framebuffer = sceGuSwapBuffers();
}

void pgReDraw(void)
{
	// 前描画バッファから画面をコピーする
	memcpy(framebuffer + 0x04000000, preframeBuffer + 0x04000000, 0x88000);
}

int pgCreateTexture(void)
{
	if (frame_num == MAX_FRAME)
		return 0;
	windowLayer[frame_num].frame = (unsigned char*)memalign(16, sizeof(unsigned char) * SCREEN_TEXTURE);
	if (windowLayer[frame_num].frame == NULL)
		return -1;
	memset(windowLayer[frame_num].frame, 0, SCREEN_TEXTURE);
	windowLayer[frame_num].tateFlag = 0;
	windowLayer[frame_num].width = SCR_WIDTH;
	windowLayer[frame_num].height = SCR_HEIGHT;
	windowLayer[frame_num].lineEnd = DRAW_LINE_H;
	windowLayer[frame_num].pgCursorX = windowLayer[frame_num].pgCursorY = 0;
	windowLayer[frame_num].viewX = windowLayer[frame_num].viewY = 0;
	printBuf = windowLayer[frame_num].frame;
	winParam = &windowLayer[frame_num];
	frame_num++;
	return 1;
}

void pgDeleteTexture(void)
{
	if (frame_num == 0)
		return;
	free(windowLayer[frame_num-1].frame);
	windowLayer[frame_num-1].frame = NULL;
	frame_num--;
	printBuf = windowLayer[frame_num-1].frame;
	winParam = &windowLayer[frame_num-1];
	return;
}

void pgDrawTexture(int num)
{
	int i;
	
	if (frame_num == 0)
		return;
	if (frame_num < num || num < 0)
		num = frame_num;
	for (i = 0; i < num; i++)
	{
		printBuf = windowLayer[i].frame;
		pgCopy(windowLayer[i].viewX, windowLayer[i].viewY);
	}
	printBuf = windowLayer[frame_num-1].frame;
	return;
}

void pgRewrite(void)
{
	pgDrawTexture(-1);
	pgCopyMenuBar();
	flipScreen(0);
	pgReDraw();
}

void pgSetDrawStart(int x, int y, int addX, int addY)
{
	if (x > -1)
		winParam->pgCursorX = x;
	winParam->pgCursorX += addX;
	if (y > -1)
		winParam->pgCursorY = y;
	winParam->pgCursorY += addY;
}

void pgGuRender(void)
{
	sceGuStart(GU_DIRECT,list);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuClearColor(0);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT|GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);
}
