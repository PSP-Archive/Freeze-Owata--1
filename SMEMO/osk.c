//==============================================================
// OSK関連
// STEAR 2009-2010
//--------------------------------------------------------------
// UCS - シフトJIS変換部はフリーズオワタのソースを参考にさせていただきました。
//--------------------------------------------------------------

#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <string.h>
#include <psputility.h>
#include <psputility_osk.h>

#include "smemo.h"
#include "osk.h"

#define BUF_WIDTH	(512)
#define SCR_WIDTH	(480)
#define SCR_HEIGHT	(272)
#define MAXTEXT		(256)										// 扱う文字数の最大（半角での数）


//==============================================================
// GU初期化
//--------------------------------------------------------------
// OSK用にGUをセットアップします。
//--------------------------------------------------------------

void oskSetupGu(void)
{
	sceGuInit();

	sceGuStart(GU_DIRECT,gList);
	sceGuDrawBuffer(GU_PSM_8888,(void*)0,BUF_WIDTH);
	sceGuDispBuffer(SCR_WIDTH,SCR_HEIGHT,(void*)0x88000,BUF_WIDTH);
	sceGuDepthBuffer((void*)0x110000,BUF_WIDTH);
	sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
	sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
	sceGuDepthRange(0xc350,0x2710);
	sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDepthFunc(GU_GEQUAL);
	sceGuEnable(GU_DEPTH_TEST);
	sceGuFrontFace(GU_CW);
	sceGuShadeModel(GU_FLAT);
	sceGuEnable(GU_CULL_FACE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuEnable(GU_CLIP_PLANES);
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);
}

//==============================================================
// シフトJIS→UCS変換
//--------------------------------------------------------------
// str1 変換後UCS文字列格納用
// str2 元となるシフトJIS文字列（文末は'\0'）
//--------------------------------------------------------------

/*
static void sjis2ucs(unsigned short *str1,char *str2)
{
	int	pos1,pos2,ucs,max;

	max = MAXTEXT;
	pos1 = 0;
	pos2 = 0;
	while (str2[pos2] && pos1<max){
		ucs = conv_sjiswin_wchar((unsigned char)str2[pos2++]);
		if (ucs==-1) continue;
		str1[pos1++] = ucs;
	}
	str1[pos1] = 0;
}
*/

//==============================================================
// UCS→シフトJIS変換
//--------------------------------------------------------------
// str1 変換後シフトJIS文字列格納用
// str2 元となるUCS文字列
//--------------------------------------------------------------

/*
static void ucs2sjis(char *str1,unsigned short *str2)
{
	int	pos1,pos2,sjis,max;

	max = MAXTEXT;
	pos1 = 0;
	pos2 = 0;
	while (str2[pos2]){
		sjis = conv_wchar_sjiswin(str2[pos2++]);
		if (sjis<0x100){
			str1[pos1++] = sjis;
		} else {
			if (pos1>=max-2) break;
			str1[pos1++] = sjis & 0xFF;
			str1[pos1++] = (sjis>>8) & 0xFF;
		}
		if (pos1>=max-1) break;
	}
	str1[pos1] = '\0';
}
*/

//==============================================================
// OSK入力処理
//--------------------------------------------------------------
// title   タイトル
// intext  OSK起動時に予め入力しておきたい文字列
// outtext OSKから入力された文字列
// 戻り値  0:問題発生/キャンセル 以外:正常終了
//--------------------------------------------------------------
// ここではGUの初期化は行わないので、予めセットアップしておくこと。
// gExitRequestによる中断処理は main.c でセットアップしておくこと。
// 文字列はシフトJISで入力/出力されます。
//--------------------------------------------------------------

int oskInput(char *title,char *intext,char *outtext)
{
	int done = 0;
	unsigned char*	p;
	unsigned short	oskIn[MAXTEXT],oskOut[MAXTEXT],oskDesc[128];

	outtext[0] = '\0';
	oskOut[0] = 0;
	p = (unsigned char*) intext;
	psp2chSJIS2UCS(oskIn, p, MAXTEXT);
	p = (unsigned char*) title;
	psp2chSJIS2UCS(oskDesc, p, MAXTEXT);

	PspOskData data;
	memset(&data, 0, sizeof(data));
	data.unk_00 = 1;
	data.unk_04 = 0;
	data.language = PSP_UTILITY_OSK_LANGUAGE_DEFAULT;
	data.unk_12 = 0;
	data.unk_16 = PSP_UTILITY_OSK_INPUTTYPE_ALL;
	data.lines = MAXTEXT / 2;									// 扱える行数
	data.unk_24 = 0;
	data.desc = oskDesc;
	data.intext = oskIn;
	data.outtextlength = MAXTEXT / 2;
	data.outtextlimit = MAXTEXT / 2;
	data.outtext = oskOut;

	PspOsk osk;
	memset(&osk, 0, sizeof(osk));
	osk.size = sizeof(osk);
//	osk.language = 0;
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&osk.language);
//	osk.buttonswap = 0;
	sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &osk.buttonswap);
	osk.unk_12 = 17; // What
	osk.unk_16 = 19; // the
	osk.unk_20 = 18; // fuck
	osk.unk_24 = 16; // ???
	osk.unk_48 = 1;
	osk.data = &data;

	if(sceUtilityOskInitStart((SceUtilityOskParams*) &osk)) return 0;

	while(!done && !gExitRequest){
		sceGuStart(GU_DIRECT,gList);
		sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
		sceGuClearColor(0x666666);
		sceGuClearDepth(0);
		sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

		sceGuFinish();
		sceGuSync(0,0);

		switch(sceUtilityOskGetStatus()){
		case PSP_UTILITY_DIALOG_NONE:
			break;
		case PSP_UTILITY_DIALOG_INIT:
			break;
		case PSP_UTILITY_DIALOG_VISIBLE:
			sceUtilityOskUpdate(2); // 2 is taken from ps2dev.org recommendation
			sceDisplayWaitVblankStart();						// 画面更新時のディレイ（下記の工夫）
			break;
		case PSP_UTILITY_DIALOG_QUIT:
			sceUtilityOskShutdownStart();
			break;
		case PSP_UTILITY_DIALOG_FINISHED:
			done = 1;
			break;
		default:
			break;
		}

		sceDisplayWaitVblankStart();							// そのままだと早すぎるのでフレーム周期を1/30へ
																// 単に1/30にすると333MHz駆動時にOSKのFINISHEDフラグを見落とす
																// ので少々工夫してます
		sceGuSwapBuffers();
	}

	psp2chUCS2SJIS((unsigned char*)outtext, data.outtext, MAXTEXT);	// 入力文字列をシフトJISへ変換

	return 1;
}

