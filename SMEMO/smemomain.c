//==============================================================
// PSP������   (STPP.05)
//     for PSP CFW5.00 M33-6
// STEAR 2009-2010
//--------------------------------------------------------------
// Simple IME�ɂ����{��e�L�X�g�G�f�B�^�B
// ���i������Ă���^�G�f�B�^����{�ɂ��Ă݂܂����B
// �Ή����Ă�����s�R�[�h��CRLF��LF�ł��BCR�P�̂ɂ͑Ή����Ă܂���B
//
// ���̃o�[�W�����ł͎g�p�������팸�̂��߂�intraFont���g�p�ł��Ȃ����Ă��܂��B
//
// SMEMOtext() �w�肳�ꂽ�e�L�X�g��ҏW����
// SMEMOfile() �w�肳�ꂽ�t�@�C�����e��ҏW����
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

//----- �}�N�� -----


//----- �萔�ݒ� -----

#define	VERSION			"PSP������ Ver1.09��"					// �A�v������
#define	INITFILE		"SMEMO.INI"								// �ݒ�t�@�C����
#define	TEMPFILE		"SMEMO_TEMP.TMP"						// �ꎞ��ƃt�@�C�����i���ݒ�Ŏg�p�j
#define	CLIPBOARD		"ms0:/PSP/COMMON/CLIPBOARD.TXT"			// �N���b�v�{�[�h�t�@�C��
#define	HISFILEJUMP		"JUMP.HIS"
#define	HISFILESARH		"SARH.HIS"
#define	HISFILECHG		"CHANGE.HIS"
#define	HISFILEPASTE	"PASTE.HIS"
#define	HISFILEFONT		"FONT.HIS"
#define	HISFILEFILE		"FILE.HIS"
#define	TBY				(24)									// �e�L�X�g�̈�̊J�nY���W
#define	LWIDTH			(475)									// ��ʏ��s�ɕ\���ł���h�b�g��
#define	WWIDTH			(900)									// ���z��ʏ��s�ɕ\���ł���h�b�g��
#define	PAGEMOVE		(12)									// �y�[�W����̍s��
#define	TEXTBLSIZE		(30)									// �s���R�[�h�擾���̏����{���̈�T�C�Y
#define	UNDOSIZE		(2000)									// �u��蒼���v�L�^������

//----- �v���g�^�C�v�錾 -----


//- �V�X�e���֘A -

//int		SetupCallbacks(void);
//int		callbackThread(SceSize args, void *argp);
//int		exitCallback(int arg1, int arg2, void *common);

//- ���̑� -

int		MenuDLGyn(char *msg,long corFrm,long corIn);

//------ �O���[�o���ϐ� -----

extern S_2CH s2ch; // psp2ch.c

unsigned int __attribute__((aligned(16)))	gList[1000];
//volatile int								gExitRequest = 0;

static strInpHis	**gHisLJump,								// ���͗����i�s�W�����v�j
					**gHisSarh,									// ���͗����i�����j
					**gHisChg,									// ���͗����i�u�����j
					**gHisFPaste,								// ���͗����i�t�@�C������R�s�y�j
					**gHisFont,									// ���͗����i�t�H���g�w��j
					**gHisFile;									// ���͗����i���͂��J���j

static char	gFolder[1024],										// ��Ƃ��Ă���t�H���_
			gPFolder[1024],										// �t�@�C������\��t�����̑Ώۃt�H���_
			gMonaFile[4][1024];									// monafont�t�@�C�����Ǘ�

long gSCor[] = {												// �V�X�e���֘A�̔z�F
				0xFFFFFF,										// �����F
				0x80FF80,										// �ʒm�_�C�A���O�i�g�j
				0x185018,										// �ʒm�_�C�A���O�i�����j
				0x8080A0,										// �x���_�C�A���O�i�g�j
				0x404090,										// �x���_�C�A���O�i�����j
				0x103010,										// �ʒm�_�C�A���O�i�w�i�j
				0x40A040,										// �X�N���[���o�[�i�g�j
				0x306030,										// �X�N���[���o�[�i�����j
				0x90F090,										// �N�X���[���o�[�i�o�[�j
				0x40FFFF,										// �f�B���N�g���̕����F
				0xC04020,										// �J�[�\���i�g�j
				0x602010,										// �J�[�\���i�����j
				0x809080,										// ���j���[�̑I��s����
				0xFFC0C0,										// �_�C�A���O�̃L�[�K�C�_���X
				0x40A040,										// �v���O���X�o�[�̐F
			};
static long	gCor[] = {
				0x804040,										// �^�C�g��
				0xFFFFFF,										// �^�C�g���������F
				0x000000,										// �w�i�F
				0xFFFFFF,										// �����F
				0x40A040,										// �X�N���[���o�[�i�g�j
				0x306030,										// �X�N���[���o�[�i�����j
				0xB0FFB0,										// �N�X���[���o�[�i�o�[�j
				0x707070,										// �^�u����
				0x80FF80,										// ���s����
				0x80FF80,										// [EOF]����
				0x80FFFF,										// ���̑��̃R���g���[���R�[�h
				0x707070										// �S�p�󔒕���
			};

static char	*gTextClip;											// �e�L�X�g�̃R�s�y�p

static int	gSave,												// �t�@�C�����ύX����Ă��邩�i0:No�j
			gEnvSave,											// ���ݒ肪�ύX����Ă��邩�i0:No�j
			gTCChg,												// �e�L�X�g�N���b�v�̓��e�ɕύX����������
			gCRLF,												// ���s�R�[�h�i0:LF 1:CRLF�j
			gTab = 0,											// �^�u���i0:4���� �ȊO:8�����j
			gPSpc = 0,											// �S�p�󔒂��L���\�����邩�i0:No �ȊO:Yes�j
			gPTab = 0,											// �^�u���L���\�����邩�i0:No �ȊO:Yes�j
			gFont = 1,											// �\���Ɏg���t�H���g�i0:���_�t�H���g 1:monafont 2:intraFont 3:intraFontP�j
			gIME = 0,											// IME�̎�ށi0:Simple IME �ȊO:OSK�j
			gRoll = 0,											// ���������g���\�����邩�i0:��ʌŒ胂�[�h 1:���z��ʃ��[�h�j
			gLineLen,											// �s���R�[�h�̖{���̃T�C�Y
			gFHeight,											// �t�H���g�̍���
			gFWidth,											// �t�H���g�̍ő啝
			gSLine,												// ��ʏ�ɕ\���o����s��
			gReDraw,											// �ŉ��s���͂ݏo�����i0:�o�Ȃ� �ȊO:�o��j
			gUndoSize,											// �u��蒼���v�f�[�^��
			gUndoPos;											// �u��蒼���v�f�[�^�擪�ʒu
static long	gLineMax,											// �S�̂̍s���i=�g�p����Ă���s���R�[�h�̑����j
			gLineListMax;										// �s���R�[�h���X�g�̌�

static float	gXDraw,											// �����s�������̃T�C�Y
				gXOffset,										// �����\���J�n�ʒu�i���E�X�N���[���p�j
				gXShift;										// ��ʓ������̍�悵�Ȃ������̃T�C�Y

static struct strText{
	long			chainBack;									// ���X�g�`�F�C���i�O�̃��R�[�h���j
																//     0xFFFFFFFF�Ȃ疢�g�p���R�[�h�A0xFFFFFFFE�Ȃ�擪�s
	long			chainNext;									// ���X�g�`�F�C���i���̃��R�[�h���j
																//     0xFFFFFFFF�Ȃ�ŏI�s
	unsigned int	back : 1;									// 1 �O�̃��R�[�h���瑱���Ă���
	unsigned int	next : 1;									// 1 ���̃��R�[�h�ɑ����Ă���
	unsigned int	enter : 1;									// 1 ���s�R�[�h����
	int				textsize;									// �{���p�Ɋm�ۂ���Ă���̈�̃T�C�Y
	int				len;										// �{���̒���
	char			*text;										// �{���ւ̃|�C���^�i���͎̂g���Ƃ��ɕʂɊm�ۂ���j
} **gText;														// �s�Ǘ��i��ʏ�̈�s����̃��R�[�h�ɑΉ�����j

static struct {													// �e�L�X�g�͈͎w��ʒu�Ǘ��p
	int		cxr;
	long	rno;
	long	line;
} gTSel[3];

struct strCsrSts{												// �J�[�\���ʒu�w��p
	int		cx;													// ��ʏ�ł̕����J�[�\��X�ʒu�i�����P�ʁj
	float	sx;													// ��ʏ�ł̕����J�[�\��X�ʒu�i�h�b�g�P�ʁj
	int		cxr;												// �_���J�[�\��X�ʒu
	float	cxb;												// �J�[�\��X�ʒu�w�莞�̓��B�ڕW�|�C���g�i�h�b�g�P�ʁj
	int		cy;													// ��ʏ�ł̕����J�[�\��Y�ʒu
	long	pline;												// ��ʏ�̕\���J�n�s��
	long	rno;												// �J�[�\���ʒu�̃��R�[�h��
	long	rline;												// �J�[�\���ʒu�̘_���s��
	int		adjust;												// ��ʈʒu���J�[�\���ɍ��킹��i0:���̂܂� 1:���킹��j
};

static struct {													// �u��蒼���v�Ǘ��p
	long	pline;												// ���삪�s��ꂽ�����s���R�[�h
	int		cxr;												// ���삪�s��ꂽ�s���R�[�h���_���ʒu
	char	str[3];												// str[0]�`[1] ����/�폜����
																// str[2]��1�Ȃ當�����́A0�Ȃ當���폜
} gUndo[UNDOSIZE];


//------ ���s���[�h�w�� -----

#ifndef	PSP_MODULE_KERNEL
#define	PSP_MODULE_KERNEL	0x1000
#endif
#ifndef	PSP_MODULE_USER
#define	PSP_MODULE_USER		0
#endif

//PSP_MODULE_INFO( "PSP������", PSP_MODULE_USER, 1, 1 );
//PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
//PSP_HEAP_SIZE_KB(-800);


//==============================================================
// �{�^�����������܂őҋ@
//--------------------------------------------------------------

void WaitBtn(void)
{
	SceCtrlData		pad;

	ScreenView();												// ��ʍX�V
	while (!gExitRequest){										// �����������܂őҋ@
		sceCtrlReadBufferPositive( &pad, 1 );
		if (SIMEgetchar(pad)!=0) break;
		sceDisplayWaitVblankStart();
	}
}

//==============================================================
// �{�^�����������܂őҋ@
//--------------------------------------------------------------

void Unpush(void)
{
	SceCtrlData		pad;

	ScreenView();												// ��ʍX�V
	while (1){
		sceCtrlReadBufferPositive(&pad, 1);
		if (!(pad.Buttons & 0x00F3F9)) break;					// ����n�{�^�����S�ė����ꂽ
		sceDisplayWaitVblankStart();
	}
}

//==============================================================
// �^�C�g�������
//--------------------------------------------------------------

void DrawTitle(char *path)
{
	char	str[65],filename[256];
	int		pos,len;

	pos = 0;
	if (s2ch.cfg.hbl){											// HBL�����ł̓t�@�C������UTF8���V�t�gJIS�ϊ����s��
		psp2chUTF82Sjis(filename, path);
	} else {
		strcpy(filename, path);
	}
	len = strlen(filename);
	if (len>61) pos = len - 61;
	strcpy( str, &filename[pos] );								// ��ʂ���͂ݏo���Ȃ��悤�ɕ���������
	if (gSave)
		strcat( str, " (�X�V)" );
	CurveBox( 0, 0, 480, 21, 3, gCor[0], gCor[0] );
	mh_print( 6, 1, str, gCor[1] );
	Fill( LWIDTH, 14, 5, 8, gCor[0] );
	HLine( 0, 22, LWIDTH, gCor[3] );
}

//==============================================================
// �X�e�[�^�X�\��
//--------------------------------------------------------------
// flag 1:�J�[�\���ʒu�ƃ��[���[�̋������
//      2:���[���[�̋������
//--------------------------------------------------------------

void DrawSts(struct strCsrSts *cs,int flag)
{
	static int	bx,lxBak = 0;
	static long	lineBak = 0;
	int			i,wy,lx,len;
	float		cw,cx;
	long		rno,pos,rline;

	if (flag==1 || lineBak!=cs->rline){							// �ω����Ă��Ȃ����͍�悵�Ȃ�
		rline = (cs->rline) +1;
		if (rline>999999) rline = 999999;
		Fill( 414, 1, 36, 12, gCor[0] );
		DrawVal( 414, 1, (cs->rline)+1, 0, 1, gCor[1] );		// �_���s�ʒu
		lineBak = cs->rline;
	}
	mh_print( 450, 1, ":", gCor[1] );
	lx = cs->cx;
	rno = cs->rno;
	while (gText[rno]->back){									// �����s�ɐ܂�Ԃ���Ă���ꍇ
		rno = gText[rno]->chainBack;
		pos = 0;
		len = 0;
		while (pos<gText[rno]->len){							// �s�̕����������𒲂ׂ�
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
		DrawVal( 456, 1, lx+1, -1, 1, gCor[1] );				// ��ʒu
		lxBak = lx;
	}

	cw = GetStrWidth( gFont,"��" )/2;
	if (flag){													// ���[���[�����
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
			XFill( cx, 14, cw-1, 8, gCor[1] );					// �J�[�\���ʒu�i�����j
	}
	cx = 1 + (cs->cx)*cw - gXOffset;
	if (cx>=0 && cx<LWIDTH-cw-1)
		XFill( cx, 14, cw-1, 8, gCor[1] );						// �J�[�\���ʒu�i�\���j
	bx = cs->cx;
	VRollBar( LWIDTH, 22, 272-22, cs->pline, gSLine, gLineMax, flag, gCor[4], gCor[5], gCor[6] );
}

//==============================================================
// �w��s���R�[�h���폜
//--------------------------------------------------------------

void DelRec(long rno)
{
	if (!gText) return;

	if (gText[rno]){
		free(gText[rno]->text);									// �{���p�̈�����
		free(gText[rno]);										// �s���R�[�h�����
		gText[rno] = NULL;										// �Ή����郌�R�[�h���X�g�𖢎g�p��
		gLineMax--;
	}
}

//==============================================================
// �V�����s���R�[�h���擾����
//--------------------------------------------------------------
// size   �����{���̈�T�C�Y�i0:�̈�͊m�ۂ��Ȃ��j
// �߂�l  -1:������������Ȃ�
//        0�`:�擾�����s���R�[�h�i���o�[
//--------------------------------------------------------------
// �{���p�̗̈�͍ŏ��Œ�����m�ۂ��A��ő���Ȃ��Ȃ����炻�̓s�x��萔����
// �g�������܂��B
//--------------------------------------------------------------

long GetNewRec(int size)
{
	struct strText	*tmp,**text;
	char	*tmpc;
	long	i,rno;

	if (gLineMax+1>=gLineListMax){								// ���R�[�h���X�g������Ȃ��Ȃ�ǉ�
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
		for (i=gLineMax; i<gLineListMax ;i++){					// �g�p�̈�̍Ō�̕����疢�g�p�̈��T���Ă݂�
			if (gText[i]==NULL){
				rno = i;
				break;
			}
		}
		if (rno==-1){
			for (i=0; i<gLineListMax ;i++){						// ���g�p�̈��T��
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
		if (tmpc==NULL){										// �{���p�̈���m�ۏo���Ȃ�����
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
// �s���R�[�h�̖{���p�̈�̊g��
//--------------------------------------------------------------
// rno �s���R�[�h���i0xFFFFFFFF���Ɖ������Ȃ��j
// len �{���̈悪�����菬�����ꍇ�A�g�������
// �߂�l  0:����I��
//        -1:������������Ȃ�
//--------------------------------------------------------------
// �w�肳�ꂽ�s���R�[�h�̖{���̈悪�w�蕶������菬�����ꍇ�A�{���̈���g��
// ������B
// �g������ꍇ�͎w��l���+50�o�C�g���߁i������gLineLen�͒����Ȃ��j�Ɋg�����܂��B
// ���Ȃ݂Ɉ�x�g�����ꂽ�{���̈�͍s���R�[�h���폜���Ȃ�����k������邱�Ƃ�
// ����܂���B
//--------------------------------------------------------------

int AddRecText(long rno,int len)
{
	char	*buf;

	if (rno!=0xFFFFFFFF){
		if (gText[rno]->textsize<=len){
			len += 50;
			if (len>gLineLen) len = gLineLen;
			buf = (char*) realloc( gText[rno]->text,len );
			if (buf==NULL) return (-1);							// ���������m�ۂł��Ȃ�
			gText[rno]->textsize = len;
			gText[rno]->text = buf;
		}
	}
	return (0);
}

//==============================================================
// �s���R�[�h�\���̂̑S�폜
//--------------------------------------------------------------
// ���Ɋm�ۂ���Ă���s���R�[�h�֘A�̃�������S�ĉ������B
//--------------------------------------------------------------

void ClrRec(void)
{
	long	i;

	for (i=0; i<gLineListMax ;i++){								// �s���R�[�h��S�ĉ��
		DelRec(i);
	}
	free(gText);												// �s���R�[�h���X�g�����
	gText = NULL;
	gLineListMax = 0;
}

//==============================================================
// �s���R�[�h�̑S������
//--------------------------------------------------------------
// ���Ɋm�ۂ���Ă���s���R�[�h�֘A�̃�������S�ĉ��������A��s���̍s���R�[�h
// ���m�ۂ��܂��B
//--------------------------------------------------------------

void InitRec(void)
{
	long	i;

	ClrRec();													// �s���R�[�h��S�ĉ��
	gLineListMax = 1000;
	gText = (struct strText**) malloc( sizeof(struct strText*) * gLineListMax );
	for (i=0; i<gLineListMax ;i++){
		gText[i] = NULL;
	}

	gLineMax = -1;
	GetNewRec(TEXTBLSIZE);
	gText[0]->chainBack = 0xFFFFFFFE;
	gLineMax = 0;

	gUndoSize = 0;												// �u��蒼���v������
	gUndoPos = 0;
	gSave = 0;
	gCRLF = 1;													// �f�t�H���g�̉��s�R�[�h��CR,LF
	gTSel[0].rno = -1;											// �͈͑I��������
	gTSel[1].rno = -1;											// �͈͑I��������
	gXOffset = 0;												// ���E�X�N���[���p
	gXShift = 0;
	gXDraw = LWIDTH;
}

//==============================================================
// �h�b�g�P�ʂŎ��̃^�u�ʒu�����߂�
//--------------------------------------------------------------
// �S�p�����̔����̕�����ɂ��Ă��܂��B
//--------------------------------------------------------------

float GetTabPos(float x)
{
	float	px,wx;

	wx = GetStrWidth(gFont,"��") * (gTab ? 8 : 4) /2;			// '��'�ɓ��ʂȈӖ��͂Ȃ��A�S�p���������m�肽������
	px = 0;
	while (px<=x){
		px += wx;
	}
	return (px);
}

//==============================================================
// �t�@�C���ǂݍ���
//--------------------------------------------------------------
// �߂�l  2:���[�U�[�ɂ�蒆�f���ꂽ
//         1:�s���R�[�h���擾�o���Ȃ��i������������Ȃ��j
//         0:����
//        -1:���s�i���[�N������������Ȃ��j
//        -2:���s�i�t�@�C�����J���Ȃ��j
//--------------------------------------------------------------
// �s���R�[�h�̎擾�Ɏ��s�����ꍇ�A���̎��_�œǂݍ��݂��I�����܂��B
// �e�L�X�g��4096�o�C�g���ǂݍ��݂Ȃ��烌�R�[�h�P�ʂɕ������Ă��܂��B
// �s���R�[�h�̖{���̈�͍ŏ��ő�l�Ŋm�ۂ�����A���e���m�肵�����_�ŃT�C�Y��
// �œK�����]�v�ȃ��������g��Ȃ��悤�ɂ��c�悤�Ƃ������A���ʂȂ��B
// realloc�ŏk�������Ă��������͉������Ȃ��炵���B
// �d�����Ȃ��̂Ŗ{�����m�肵�����_�Ŗ{���̈���m�ۂ���悤�ɂ��Ă��܂��B
//--------------------------------------------------------------

//----- �s���R�[�h�̖{�����Z�b�g -----

int fileloadText(long rno,char *text,int cnt)
{
	char	*buf;
	long	rno2;

	buf = (char*) malloc(cnt+1);								// �{���̈�̊m��
	if (buf){													// �m�ۂł�����
		gText[rno]->text = buf;
		gText[rno]->textsize = cnt +1;
		memcpy( gText[rno]->text,text,cnt );
		gText[rno]->len = cnt;
		return (0);
	} else {													// ������������Ȃ�
		rno2 = gText[rno]->chainBack;
		DelRec(rno);
		gText[rno2]->chainNext = 0xFFFFFFFF;
		gText[rno2]->next = 0;
		return (-1);
	}
}

//----- ���̍s���R�[�h���쐬 -----

int fileloadNextLine(long *rno,char *text,int *cnt,int *len,float *wx)
{
	long	rno2;

	if (fileloadText( *rno,text,*cnt )) return (-1);			// �{�����Z�b�g
	*cnt = 0;
	*len = 0;
	*wx = 0;
	rno2 = GetNewRec(0);
	if (rno2==-1) return (-1);									// �s���R�[�h���擾�o���Ȃ�
	gText[*rno]->chainNext = rno2;
	gText[*rno]->next = 1;										// ���̍s�ɑ����Ă���
	gText[rno2]->chainBack = (*rno);
	gText[rno2]->back = 1;										// �O�̍s���瑱���Ă���
	*rno = rno2;
	return (0);
}

//----- ���C�� -----

int fileload(char *fname)
{
	const int		ws[5] = {16384,4096,1024,128};
	SceCtrlData		pad;
	char	*data,text[WWIDTH/2+2],msg[128],c[3] = {0,0,0};
	int		i,fd,cnt,len,ret,worksize;
	long	filesize,pos,poscnt,rno,rno2;
	float	wc,wx;

	//----- �v���O���X�o�[ -----

	Progressbar1( -1, 135, 0, "�~:�L�����Z��", gSCor[7], gSCor[13], gSCor[1], gSCor[2] );
	ScreenView();												// ��ʍX�V

	//----- �f�[�^���R�[�h������ -----

	InitRec();

	//----- �t�@�C���ǂݍ��� -----

	msg[0] = '\0';
	filesize = 0;
	fd = sceIoOpen( fname, PSP_O_RDONLY, 0777 );
	data = NULL;
	if (fd>=0){
		filesize = sceIoLseek( fd, 0, SEEK_END );
		sceIoLseek( fd, 0, SEEK_SET );
		for (i=0; i<4 ;i++){									// �o���邾���傫���o�b�t�@���m�ۂ���
			worksize = ws[i];
			data = (char*) malloc(worksize);
			if (data) break;									// �o�b�t�@�̊m�ۂɐ�������
		}
		if (!data){												// ���������m�ۂł��Ȃ�����
			sceIoClose(fd);
			strcpy( msg, "���[�N���������m�ۂł��܂���ł���" );
			DiaBox1( -1, 120, 0, msg, gSCor[0], gSCor[3], gSCor[4] );	// �����ɕ\��
		} else {
			sceIoRead( fd, data, worksize );
		}
	} else {
		return (-2);											// �t�@�C�����J���Ȃ�����
	}

	if (msg[0]){												// �G���[����
		WaitBtn();												// �����������܂őҋ@
		return (-1);
	}

	//----- ���R�[�h�P�ʂɕ��� -----

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
		if (pos>=worksize){													// �e�L�X�g�������[�h
			poscnt += pos;
			pos = 0;
			sceIoRead( fd, data, worksize );
			Progressbar2( poscnt, filesize, gSCor[14] );				// �v���O���X�o�[�̍X�V
			ScreenView();												// ��ʍX�V
			sceCtrlReadBufferPositive( &pad, 1 );
			if (SIMEgetchar(pad)==SIME_KEY_CROSS){
				ret = 2;
				break;
			}
		}
		if (c[0]==0) c[0] = 1;											// �o�C�i���f�[�^'\0'���܂܂��ƕs�s������������̂�
		if (c[0]=='\r'){												// CR�͎̂Ă�
			gCRLF = 1;
		} else if (c[0]=='\n'){											// LF����
			text[cnt++] = '\n';
			gText[rno]->len = cnt;
			if (fileloadText( rno,text,cnt )){							// �{�����Z�b�g
				ret = 1;
				break;
			}
			cnt = 0;
			len = 0;
			wx = 0;
			rno2 = GetNewRec(0);
			if (rno2==-1){												// �s���R�[�h���擾�ł��Ȃ�
				ret = 1;
				break;
			}
			gText[rno]->chainNext = rno2;
			gText[rno]->enter = 1;										// ���s����
			gText[rno2]->chainBack = rno;
			rno = rno2;
		} else {
			if (chkSJIS(c[0])){											// �S�p�����������ꍇ
				c[1] = data[pos++];
				if (pos>=worksize){											// �e�L�X�g�������[�h
					poscnt += pos;
					pos = 0;
					sceIoRead( fd, data, worksize );
					Progressbar2( poscnt, filesize, gSCor[14] );		// �v���O���X�o�[�̍X�V
					ScreenView();										// ��ʍX�V
					sceCtrlReadBufferPositive( &pad, 1 );
					if (SIMEgetchar(pad)==SIME_KEY_CROSS){
						ret = 2;
						break;
					}
				}
				wc = GetStrWidth( gFont,c );
				if (len>=gLineLen-1 || (wx+wc>=(gRoll ? WWIDTH : LWIDTH))){	// �s�Ɏ��܂�Ȃ�
					if (fileloadNextLine( &rno, text, &cnt, &len, &wx )){	// ���̍s���쐬
						ret = 1;
						break;
					}
				}
				text[cnt++] = c[0];
				text[cnt++] = c[1];
				len += 2;
				wx += wc;
			} else {													// ���p�����������ꍇ
				c[1] = '\0';
				wc = GetStrWidth( gFont,c );
				if (wx+wc>=(gRoll ? WWIDTH : LWIDTH) && c[0]!='\t'){	// �s�Ɏ��܂�Ȃ�
					if (fileloadNextLine( &rno, text, &cnt, &len, &wx )){	// ���̍s���쐬
						ret = 1;
						break;
					}
				}
				text[cnt++] = c[0];
				if (c[0]=='\t'){										// �^�u
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
					if (fileloadNextLine( &rno, text,&cnt, &len, &wx )){	// ���̍s���쐬
						ret = 1;
						break;
					}
				}
			}
		}
	}
	fileloadText( rno,text,cnt+1 );										// �ŏI�s����s�������ꍇ�̑΍�
	gText[rno]->len = cnt;
	gText[rno]->chainNext = 0xFFFFFFFF;

	//----- ��n�� -----

	if (gExitRequest) ret = 2;
	sceIoClose(fd);
	free(data);
	return (ret);
}

//==============================================================
// �t�@�C����������
//--------------------------------------------------------------
// �߂�l  0:����
//        -1:���[�N�������̎擾�Ɏ��s
//        -2:�ۑ���t�@�C���̃I�[�v���Ɏ��s
//--------------------------------------------------------------
// ���[�N�̈��Ƀv���[���e�L�X�g�����Ȃ���t�@�C���ɏ����o���Ă��܂��B
// ���[�N�̈�̓T�C�Y���Ƃɉ��p�^�[�����m�ۂɃg���C���A�\�Ȍ���傫���̈��
// �擾����悤�ɂ��Ă��܂��B
// �������c�ʂ����Ȃ��ꍇ�A�����ɒ����Ԃ����鎖������ł��傤�B
// ���s�R�[�h�͓ǂݍ��݌��Ɠ����ɂȂ�悤�ɕ␳���܂��B
//--------------------------------------------------------------

int filesave(char *fname)
{
	char	*data;
	int		i,fd;
	long	ws[5] = {65536,16384,4096,1024,WWIDTH/2};
	long	rno,pos,line,worksize;

	//----- ���[�N�̈�̊m�� -----

	for (i=0; i<5 ;i++){										// ���[�N�̈�̑傫��������
		worksize = ws[i];
		data = (char*) malloc(worksize);
		if (data) break;
	}
	if (data==NULL) return (-1);								// ���[�N�̈悪�m�ۂł��Ȃ�����

	//----- �v���O���X�o�[ -----

	Progressbar1( -1, 135, 0, "�L�����Z���o���܂���", gSCor[7], gSCor[13], gSCor[1], gSCor[2] );
	ScreenView();												// ��ʍX�V

	//----- �{�����t�@�C���֏����o�� -----

	fd = sceIoOpen( fname, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if (fd<0){
		free(data);
		return (-2);											// �t�@�C�����J���Ȃ�
	}
	rno = 0;
	pos = 0;
	line = 0;
	do{
		if (pos+gText[rno]->len > worksize){					// ���[�N�Ɏ��܂�Ȃ��Ȃ�t�@�C���ɏo��
			Progressbar2( line, gLineMax, gSCor[14] );			// �v���O���X�o�[�̍X�V
			ScreenView();										// ��ʍX�V
			sceIoWrite( fd, data, pos );
			pos = 0;
		}
		memcpy( &data[pos], gText[rno]->text, gText[rno]->len );
		if (gText[rno]->enter && gCRLF){						// ���s�R�[�h�C��
			data[pos+gText[rno]->len-1] = '\r';
			data[pos+gText[rno]->len] = '\n';
			pos++;
		}
		pos += gText[rno]->len;
		line++;
		rno = gText[rno]->chainNext;
	}while (rno!=0xFFFFFFFF && !gExitRequest);
	if (pos){													// ���[�N�Ɏc���Ă���e�L�X�g���t�@�C����
		sceIoWrite( fd, data, pos );
	}
	sceIoClose(fd);

	//----- �I������ -----

	gSave = 0;
	free(data);
	return (0);
}

//==============================================================
// ���f�[�^�̓ǂݍ���
//--------------------------------------------------------------

void LoadEnv(void)
{
	const char		it[12][11] = {"FONT","ROLL","IME","TABW","SPCP","TABP","FOLDER1","FOLDER2","FONT12A","FONT12W","FONT16A","FONT16W"};
	char	c,path[1024],buf[1030],*p;
	int		i,fd,no;
	FILE*	fp1;

	gEnvSave = 0;												// ���ݒ肪�ύX����Ă��邩
	if (getcwd( path,1024 )==NULL){								// �J�����g�f�B���N�g���̎擾
		strcpy( path,"ms0:");
	}
	strcat( path,"/" );
	strcat( path,INITFILE );
	fp1 = fopen( path,"r" );
	if (fp1!=NULL){
		while (1){												// �g�[�N����؂�o���A�e�ϐ��ɒl��ݒ肷��
			if (fgets( buf,1030,fp1 )==NULL) break;
			for (i=0; i<strlen(buf) ;i++){						// ���s�R�[�h���폜
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
				if (fd>=0){										// �t�H���_�����݂���Ȃ�
					strncpy( gFolder,p,1023 );
					gFolder[1023] = '\0';
				}
				break;
			case 7:
				fd = sceIoDopen(p);
				sceIoDclose(fd);
				if (fd>=0){										// �t�H���_�����݂���Ȃ�
					strncpy( gPFolder,p,1023 );
					gPFolder[1023] = '\0';
				}
				break;
			case 8:
				fd = sceIoOpen( p, PSP_O_RDONLY, 0777 );
				sceIoDclose(fd);
				if (fd>=0){										// �t�@�C�������݂���Ȃ�
					strncpy( gMonaFile[0],p,1023 );
					gMonaFile[0][1023] = '\0';
				}
				break;
			case 9:
				fd = sceIoOpen( p, PSP_O_RDONLY, 0777 );
				sceIoDclose(fd);
				if (fd>=0){										// �t�@�C�������݂���Ȃ�
					strncpy( gMonaFile[1],p,1023 );
					gMonaFile[1][1023] = '\0';
				}
				break;
			case 10:
				fd = sceIoOpen( p, PSP_O_RDONLY, 0777 );
				sceIoDclose(fd);
				if (fd>=0){										// �t�@�C�������݂���Ȃ�
					strncpy( gMonaFile[2],p,1023 );
					gMonaFile[2][1023] = '\0';
				}
				break;
			case 11:
				fd = sceIoOpen( p, PSP_O_RDONLY, 0777 );
				sceIoDclose(fd);
				if (fd>=0){										// �t�@�C�������݂���Ȃ�
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
// ���f�[�^�̕ۑ�
//--------------------------------------------------------------

//----- ���l�n -----

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

//----- ������n -----

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

//----- ���C�� -----

void SaveEnv(void)
{
	char	path[1024],buf[3100];
	int		fd,pos;

	if (gEnvSave==0) return;									// ���ݒ肪�ύX����Ă��Ȃ��Ȃ�I��

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

	if (getcwd( path,1024 )==NULL){								// �J�����g�f�B���N�g���̎擾
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
// ���ύX���̊e���ʊ֌W�̃p�����[�^��ݒ�
//--------------------------------------------------------------

void setScreen(void)
{
	if (gFont==2){												// 16�h�b�gmonafont
		gSLine = 14;											// ��ʍs��
		gFHeight = 18;											// �s��
		gFWidth = 16;											// �ő�t�H���g��
		gReDraw = 1;											// �ŉ��s���͂ݏo��
	} else {													// ���̑��̃t�H���g
		gSLine = 19;											// ��ʍs��
		gFHeight = 13;											// �s��
		gFWidth = 12;											// �ő�t�H���g��
		gReDraw = 0;											// �ŉ��s���͂ݏo���Ȃ�
	}

	if (gRoll){													// �������̃T�C�Y�ɍ��킹�čs���R�[�h�̖{���̃T�C�Y��ύX
		gLineLen = WWIDTH / 2 +2;
	} else {
		gLineLen = LWIDTH / 2 +2;
	}
}

//==============================================================
// �w�蕨���s�ɑΉ�����s���R�[�h�����擾����
//--------------------------------------------------------------
// �s���R�[�h�͒ǉ����ꂽ��폜���ꂽ�肷�邽�߃��R�[�h���ƕ����s�ԍ��͈�v
// ���܂���B
// �����s�ԍ��i��ʂɕ\�����ꂽ��Ԃł̍s�ԍ��j�ɑΉ����Ă��郌�R�[�h�����擾
// ���邽�߂ɐ擪�s���烊�X�g�`�F�C����H��w��s��T���܂��B
// ����͍s���ɔ�Ⴕ�ď������Ԃ��|����̂ŁA���ՂɎg���ׂ��ł͂���܂���B
// �ꖜ�s���炢���珈�����Ԃ������ł��Ȃ��Ȃ�܂��B
//--------------------------------------------------------------

long GetRNo(long line)
{
	long	no,rno;

	no = 0;
	rno = 0;
	while (no!=line){											// ���X�g�`�F�C����H��Ȃ���s���J�E���g
		if (gText[rno]->chainNext==0xFFFFFFFF) break;
		rno = gText[rno]->chainNext;
		no++;
	}
	return (rno);
}

//==============================================================
// ���݂̍s���R�[�h�����瑊�ΓI�Ɉړ������s���R�[�h�����擾����
//--------------------------------------------------------------
// ���݂̍s���R�[�h�����������Ă���ꍇ�ɗL���ł��B
// �擪�s���O�ɂ͈ړ����܂���A�擪�s�Ŏ~�܂�܂��B
// �ŏI�s�𒴂����0xFFFFFFFF��Ԃ��܂��B
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
// �w��s���R�[�h���ɑΉ����镨���s���擾����
//--------------------------------------------------------------
// ����͍s���ɔ�Ⴕ�ď������Ԃ��|����̂ŁA���ՂɎg���ׂ��ł͂���܂���B
// �ꖜ�s���炢���珈�����Ԃ������ł��Ȃ��Ȃ�܂��B
//--------------------------------------------------------------

long GetLine(long rno)
{
	long	line,rnoP;

	line = 0;
	rnoP = 0;
	while (rnoP!=rno){											// ���X�g�`�F�C����H��Ȃ���s���J�E���g
		if (gText[rnoP]->chainNext==0xFFFFFFFF) break;
		rnoP = gText[rnoP]->chainNext;
		line++;
	}
	return (line);
}

//==============================================================
// ���R�[�h���e����ʂɕ\��
//--------------------------------------------------------------
// y       �\������s�ʒu
// rno     �\������s�̃��R�[�h��
// line    �\������s�̕����s��
//--------------------------------------------------------------
// �����P�ʂŔw�i����������A�����������Ă܂��B
// �s���ȍ~�͔w�i�F�ŏ����B
// gXOffset,gXShift,gXDraw�̎w��ɏ]���ĕ\�����s���܂��B
// gXOffset�͉�ʊO���̕\������Ȃ��͈́AgXShift�͉�ʓ������ō����s��Ȃ��͈́A
// gXDraw�͂��̈ʒu�ȍ~�͍����s��Ȃ��A�Ƃ������ݒ�ł��B
// �`�ʔ͈͂���O��č�悳�ꂽ�ꍇ�i�X�N���[���o�[�ɕ������㏑�������j�ɔ����A
// �`�ʔ͈͊O�C���[�W�𖢎g�pVRAM�i�y�[�W3�j�ɑҔ������Ă܂��B
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
		if (selPos1>selPos2){											// selPos1����selPos2�܂ł��I��͈�
			selPos3 = selPos2;
			selPos2 = selPos1;
			selPos1 = selPos3;
		}
	}
	cr = 0;
	if (gFHeight>13) cr = 4;											// 16�h�b�g�t�H���g���̓���L��
	wy = gFHeight;
	if (TBY+(y+1)*gFHeight>272) wy = 272 - (TBY+y*gFHeight);			// ��ʉ��ɂ͂ݏo��Ȃ�
	pos = 0;
	sx = 0;
	bx = 0;
	selPos3 = line * gLineLen;
	ImgMove( LWIDTH,TBY+y*gFHeight,480-LWIDTH,wy, 0,272*2.5 );		// �s���ȍ~�C���[�W��Ҕ�
	corBak[0] = gCor[2];												// �ʏ핶���w�i
	if (gXShift==0){													// ��ʍ��[�Ɏc��S�~�̍폜
		wx = gFWidth;
		if (wx>gXDraw) wx = gXDraw;
		Fill( 0, TBY+y*gFHeight, wx, wy, corBak[0] );
	}
	while (gText[rno]->len){
		if (gTSel[0].rno==-1 || selPos3<selPos1 || selPos3>=selPos2){
			corStr[0] = gCor[3];										// �ʏ핶��
			corStr[1] = gCor[11];										// �S�p��
			corStr[2] = gCor[10];										// �R���g���[���R�[�h
			corStr[3] = gCor[8];										// ���s����
			corStr[4] = gCor[7];										// �^�u
			corBak[0] = gCor[2];										// �ʏ핶���w�i
			corBak[1] = gCor[2];										// ���s�w�i
		} else {														// �e�L�X�g�I�����̐F�w��
			corStr[0] = gCor[2];										// �ʏ핶��
			corStr[1] = gCor[2];										// �S�p��
			corStr[2] = gCor[2];										// �R���g���[���R�[�h
			corStr[3] = gCor[2];										// ���s����
			corStr[4] = gCor[2];										// �^�u
			corBak[0] = gCor[3];										// �ʏ핶���w�i
			corBak[1] = gCor[8];										// ���s�w�i
		}
		c[0] = gText[rno]->text[pos++];
		selPos3++;
		xbx = bx - gXOffset;
		xsx = sx - gXOffset;
		if (c[0]=='\t'){												// �^�u����
			sx = GetTabPos(sx);											// ���̃^�u�ʒu
			xsx = sx - gXOffset;
			if (xbx<gXShift && xsx>=gXShift){
				Fill( gXShift, TBY+y*gFHeight, xsx-gXShift, wy, corBak[0] );
			}else {
				if (xbx>=0 && xsx>gXDraw){								// �^�u����ʂ���͂ݏo��ꍇ
					Fill( xbx, TBY+y*gFHeight, gXDraw-xbx, wy, corBak[0] );
				} else if (xbx>=gXShift && xsx>=gXShift){
					Fill( xbx, TBY+y*gFHeight, xsx-xbx, wy, corBak[0] );
				}
			}
			if (gPTab){
				if (xbx>=gXShift || (gXShift>3 && xbx>=gXShift-3)){		// 3�̓^�u�}�[�N�̕��i���X�N���[������Ƃ��Ƀ}�[�N���Ԃ����̂Łj
					DrawChar( xbx, TBY+y*gFHeight+2, 0+cr, corStr[4] );
				}
			}
		} else if (c[0]=='\n'){											// ���s����
			if (xsx<gXDraw && xsx>=gXShift){
				Fill( xsx, TBY+y*gFHeight, 6+1, wy, corBak[1] );		// �����w�i
				DrawChar( xsx, TBY+y*gFHeight+2, 1+cr, corStr[3] );
			}
			sx += 6;
		} else if (c[0]==0x1A){											// EOF�����i�����\�����Ȃ��j
		} else if ((unsigned char)c[0]<32){								// ���̑��̃R���g���[���R�[�h�𔭌�
			wx = GetStrWidth(gFont," ");
			if (xsx>=gXShift){
				Fill( xsx, TBY+y*gFHeight, wx, wy, corBak[0] );			// �����w�i
				Fill( xsx, TBY+y*gFHeight, wx-1, wy-1, corStr[2] );
			}
			sx += wx;
		} else if (chkSJIS(c[0])){										// �S�p��������
			c[1] = gText[rno]->text[pos++];
			wx = GetStrWidth(gFont,c);
			if (xsx>=gXShift){
				Fill( xsx, TBY+y*gFHeight, wx+0.5, wy, corBak[0] );		// �����w�i
			}
			selPos3++;
			if (strcmp(c,"�@")==0 && gPSpc){							// �S�p��
				if (xsx>=gXShift){
					DrawChar( xsx+0, TBY+y*gFHeight+2, 2+cr, corStr[1] );
					DrawChar( xsx+5, TBY+y*gFHeight+2, 3+cr, corStr[1] );
				}
			} else {													// ���̑��̕���
				if (xsx>=gXShift){
					pf_print( xsx, TBY+y*gFHeight, c, corStr[0] );
				}
			}
			c[1] = '\0';
			sx += wx;
		} else {														// ���p��������
			wx = GetStrWidth(gFont,c);
			if (xsx>=gXShift){
				Fill( xsx, TBY+y*gFHeight, wx+0.5, wy, corBak[0] );		// �����w�i
				pf_print( xsx, TBY+y*gFHeight, c, corStr[0] );
			}
			sx += wx;
		}
		bx = sx;
		if (gText[rno]->len<=pos || sx-gXOffset>=gXDraw){				// �\���I��
			break;
		}
	}
	if (gText[rno]->chainNext==0xFFFFFFFF && gText[rno]->len<=pos){		// �ŏI�s�Ȃ�[EOF]�ƕ\��
		strcpy( str,"[EOF]" );
		for (i=0; i<5 ;i++){
			xsx = sx - gXOffset;
			c[0] = str[i];
			wx = GetStrWidth( gFont,c );
			if (xsx>=gXShift && xsx<LWIDTH){
				Fill( xsx, TBY+y*gFHeight, wx+1, wy, gCor[2] );			// �����w�i
				pf_print( xsx, TBY+y*gFHeight, c, gCor[9] );
			}
			sx += wx;
		}
	}

	xsx = sx - gXOffset;
	if (xsx<gXDraw){													// �s�������ȍ~�̗]�������
		if (xsx>=gXShift){
			Fill( xsx, TBY+y*gFHeight, (gXDraw-(xsx-0.9f)), wy, gCor[2] );
		} else {
			Fill( gXShift, TBY+y*gFHeight, gXDraw-gXShift, wy, gCor[2] );
		}
	} else if (xsx>=LWIDTH){											// ��ʂ���͂ݏo�����ꍇ�̏C��
		ImgMove( 0,272*2.5,480-LWIDTH,wy, LWIDTH,TBY+y*gFHeight );		// �Ҕ����Ă����s���ȍ~�C���[�W�𕜌�
	}
}

//==============================================================
// �w��ʒu�̃��R�[�h���e����ʂɕ\��
//--------------------------------------------------------------

void DrawRec(int y,long rno,long pline)
{
	int		wy;

	if (rno!=0xFFFFFFFF){
		DrawRecLine( y, rno, pline + y );
	} else {
		wy = gFHeight;
		if (TBY+(y+1)*gFHeight>272) wy = 272 - TBY+y*gFHeight;	// ��ʉ��ɂ͂ݏo��Ȃ�
		Fill( 0, TBY+y*gFHeight, LWIDTH, wy, gCor[2] );			// �ŏI�s�ȍ~
	}
}

//==============================================================
// ��ʑS�̂̃��R�[�h���e��\��
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
	if (l<gSLine || !gReDraw){									// ��ʉ������������ăS�~���폜
		Fill( 0, TBY+l*gFHeight, LWIDTH, 272-(TBY+l*gFHeight), gCor[2] );
	}
}

void DrawFullRec(long pline)
{
	DrawFullRec2(GetRNo(pline),pline);
}

//==============================================================
// �X�N���[���_�E��
//--------------------------------------------------------------
// �w��s���牺����s���X�N���[���_�E���B
// �X�N���[���̌��ʊJ�����ꏊ�̍X�V�͂����ł͍s��Ȃ��B
// GU�ɏ��������Ă܂��B
// �ꔭ�œ]������ɂ͉�������]�������Ȃ��Ƃ����Ȃ��̂����A����Ȏw��͂ł��Ȃ�
// ���ۂ��̂ŁA��x�o�b�t�@�ɓ]�����Ă���ړI�ʒu�ɕ`���߂��Ă��܂��B
//--------------------------------------------------------------

void RollDown(int y)
{
	int		wy;

	if (y==gSLine-1) return;

	wy = (gSLine-1-y) * gFHeight;
	if (TBY+(y+1)*gFHeight + wy>=272) wy = 272 - (TBY+(y+1)*gFHeight);	// ��ʉ�����͂ݏo���ꍇ
	sceGuStart( GU_DIRECT, gList );
	sceGuCopyImage( GU_PSM_8888, 0, TBY+y*gFHeight, LWIDTH, wy, 512, VRAM, 0,            272*2.5, 512, VRAM );
	sceGuCopyImage( GU_PSM_8888, 0,        272*2.5, LWIDTH, wy, 512, VRAM, 0, TBY+(y+1)*gFHeight, 512, VRAM );
	sceGuFinish();
	sceGuSync(0,0);
}

//==============================================================
// �X�N���[���A�b�v
//--------------------------------------------------------------
// �w��s���牺���X�N���[���A�b�v�B
// �X�N���[���̌��ʊJ�����ꏊ�̍X�V�͂����ł͍s��Ȃ��B
// GU�ɏ��������Ă܂��B
// ������͂��̂܂ܑf���ɏ������Ă܂��B
//--------------------------------------------------------------

void RollUp(int y)
{
	int		wy,yy;

	wy = (gSLine-1-y)*gFHeight;
	yy = 0;
	if (TBY+y*gFHeight+gFHeight + wy >= 272){					// ��ʉ�����͂ݏo���ꍇ
		yy = TBY+y*gFHeight+gFHeight + wy - 272;				// ��ʊO�ɂ͂ݏo���T�C�Y
		wy = 272 - (TBY+y*gFHeight+gFHeight);					// ��ʓ��̃T�C�Y
	}
	ImgMove(0, TBY+y*gFHeight+gFHeight, LWIDTH, wy, 0, TBY+y*gFHeight);
	if (yy){													// �͂ݏo�����������C�����K�v
		Fill( 0, TBY+y*gFHeight+gFHeight+wy, LWIDTH, yy, gCor[2] );
	}
}

//==============================================================
// �X�N���[�����C�g
//--------------------------------------------------------------
// ��ʂ��E�ɃX�N���[��������B
// �X�N���[���̌��ʊJ�����ꏊ�̍X�V�͂����ł͍s��Ȃ��B
// GU�ɏ��������Ă܂��B
// �ꔭ�ł͓]���ł��Ȃ��̂ŁA��x�o�b�t�@�ɓ]�����Ă���ړI�ʒu�ɕ`���߂��Ă��܂��B
// ��xCPU�ŏ��������Ă݂����ǁA����ς�GU�̕��������B
// CPU���ʍ����GU���ʍ��*2�̕��������̂ˁB
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
// �X�N���[�����t�g
//--------------------------------------------------------------
// ��ʂ����ɃX�N���[��������B
// �X�N���[���̌��ʊJ�����ꏊ�̍X�V�͂����ł͍s��Ȃ��B
// GU�ɏ��������Ă܂��B
//--------------------------------------------------------------

void RollLeft(int wx)
{
	ImgMove( wx, TBY, LWIDTH-wx, 272-TBY, 0, TBY );
}

//==============================================================
// �w�蕨���ʒu���z���Ȃ��ł����Ƃ��߂��J�[�\���ʒu������
//--------------------------------------------------------------
// cx   �����J�[�\���ʒu
// sx   �h�b�g�P�ʂł̃J�[�\���ʒu
// cxr  �_���J�[�\���ʒu
// cxb  �ڕW�J�[�\���ʒu
// rno  �s���R�[�h��
// flag �s���̈����i0:�s�����E���o �ȊO:�\����I�[�ʒu�܂ňړ��j
//--------------------------------------------------------------
// cxb�ɍł��߂��ʒu��cx,sx,cxr���Z�b�g����B
// �V�t�gJIS�ł͑S�p�𐳂����������邽�߂ɂ͍s��������S�p���p���`�F�b�N
// ���Ă����������@������܂���B
// flag�͉�ʕ\���ɔ����J�[�\���ړ��p�i1�j���AClipRec()�ɂ��s����p�i0�j��
// �̎��ʗp�ł��B
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
			if ((*sx)+stepw>=cxb) break;						// �w��|�C���g�ɒB����
		} else {												// �^�u���s�����E�ɗ���ꍇ�̗�O����
			if ((*sx)+2>=cxb) break;							// �^�u�̍ŏ�����2�h�b�g
		}
		(*cx) += stepp;
		(*sx) += stepw;
		(*cxr) += stepr;
		if (*cxr>=len){											// �s�Ō���ɒB����
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
// �J�[�\����_���ʒu�Ɉړ�
//--------------------------------------------------------------
// cs.cxr �w��_���ʒu
//--------------------------------------------------------------
// cxr�Ŏw�肳�ꂽ�_���ʒu�ɍ����悤��cx,sx,cxb��ݒ肷��B
// cxb��sx�Ɠ����l�ɂȂ�܂��B
// �s����O�ꂽ�ʒu���w�肳��Ă���ꍇ�͍s���ʒu�ɐݒ肳��܂��B
// ���̏ꍇ�Acxr���s���ʒu�ւƕύX����܂��B
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
		if (pos>=gText[cs->rno]->len) break;					// �s���ɒB����
	}

	(cs->cxr) = pos;
	(cs->cxb) = (cs->sx);
}

//==============================================================
// �J�[�\������Ɉړ�
//--------------------------------------------------------------
// cs   �J�[�\�����
// flag ��ʕ\���̍X�V���s�����i0:�s��Ȃ� �ȊO:�ύX���������ꍇ�͍X�V����j
//--------------------------------------------------------------
// �O�̍s�ŁAcxb�̕����ʒu�ɂ����Ƃ��߂��ʒu�ɃJ�[�\�����ړ�������B
// �K�v�ɉ����ĉ�ʂ̃X�N���[�����s���B
//--------------------------------------------------------------

void CMUp(struct strCsrSts *cs,int flag)
{
	long	bufRNo;

	if (gText[cs->rno]->chainBack==0xFFFFFFFE) return;			// �擪�s�Ȃ牽�����Ȃ�
	bufRNo = cs->rno;
	cs->rno = gText[cs->rno]->chainBack;
	if (gText[cs->rno]->enter) (cs->rline)--;					// �J�[�\���ʒu�̘_���s��
	CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, flag );	// �J�[�\���ʒu�̕␳
	gTSel[1].rno = cs->rno;										// �e�L�X�g�͈͎w��p
	gTSel[1].cxr = cs->cxr;
	gTSel[1].line = cs->pline + cs->cy -1;
	if (gTSel[0].rno!=-1 && flag){								// �e�L�X�g�͈͎w�蒆�Ȃ�
		DrawRec( cs->cy, bufRNo, cs->pline );					// �ړ��O�̃J�[�\���s�\�����X�V
	}

	(cs->cy)--;
	if (cs->cy<0){												// �J�[�\������ʂ���͂ݏo��Ȃ�X�N���[��
		cs->cy = 0;
		(cs->pline)--;
		if (flag){
			RollDown(0);
			DrawRec( cs->cy, cs->rno, cs->pline );
		}
	} else if (gTSel[0].rno!=-1 && flag){						// �e�L�X�g�͈͎w�蒆�Ȃ�
		DrawRec( cs->cy, cs->rno, cs->pline );					// �ړ���̃J�[�\���s�\�����X�V
	}
}

//==============================================================
// �J�[�\�������Ɉړ�
//--------------------------------------------------------------
// cs   �J�[�\�����
// flag ��ʕ\���̍X�V���s�����i0:�s��Ȃ� �ȊO:�ύX���������ꍇ�͍X�V����j
//--------------------------------------------------------------
// ���̍s�ŁAcxb�̕����ʒu�ɂ����Ƃ��߂��ʒu�ɃJ�[�\�����ړ�������B
// �K�v�ɉ����ĉ�ʂ̃X�N���[�����s���B
//--------------------------------------------------------------

void CMDown(struct strCsrSts *cs,int flag)
{
	long	bufRNo;

	if (gText[cs->rno]->chainNext==0xFFFFFFFF){							// �ŏI�s�Ȃ�
		CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, 0 );	// �J�[�\���ʒu�̕␳
		if (gTSel[0].rno!=-1){											// �e�L�X�g�͈͎w�蒆�Ȃ�
			gTSel[1].rno = cs->rno;										// �e�L�X�g�͈͎w��p
			gTSel[1].cxr = cs->cxr;
			gTSel[1].line = cs->pline + cs->cy;
			if (flag)
				DrawRec( cs->cy, cs->rno, cs->pline );					// �ŏI�s�̕\�����X�V
		}
		return;
	}

	bufRNo = cs->rno;
	if (gText[cs->rno]->enter) (cs->rline)++;							// �J�[�\���ʒu�̘_���s��
	cs->rno = gText[cs->rno]->chainNext;								// ���̍s�ɑΉ����Ă��郌�R�[�h��
	if (gText[cs->rno]->chainNext==0xFFFFFFFF){							// �ŏI�s�Ȃ�
		CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, 0 );	// �J�[�\���ʒu�̕␳�i�ŏI�s�����ŏI�������z������j
	} else {
		CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, 1 );	// �J�[�\���ʒu�̕␳
	}
	gTSel[1].rno = cs->rno;												// �e�L�X�g�͈͎w��p
	gTSel[1].cxr = cs->cxr;
	gTSel[1].line = cs->pline + cs->cy +1;
	if (gTSel[0].rno!=-1 && flag){										// �e�L�X�g�͈͎w�蒆�Ȃ�
		DrawRec( cs->cy, bufRNo, cs->pline );							// �ړ��O�̃J�[�\���s�\�����X�V
	}

	(cs->cy)++;
	if (cs->cy>=gSLine-gReDraw){										// �J�[�\������ʂ���͂ݏo��Ȃ�X�N���[��
		cs->cy = gSLine-1-gReDraw;
		(cs->pline)++;
		if (flag){
			RollUp(0);
			if (gReDraw){
				DrawRec( gSLine-2, ShiftRNo(cs->rno,gSLine-2-(cs->cy)), cs->pline );
			}
			DrawRec( gSLine-1, ShiftRNo(cs->rno,gSLine-1-(cs->cy)), cs->pline );
		}
	}else if (gTSel[0].rno!=-1 && flag){								// �e�L�X�g�͈͎w�蒆�Ȃ�
		DrawRec( cs->cy, cs->rno, cs->pline );							// �ړ���̃J�[�\���s�\�����X�V
	}
}

//==============================================================
// �J�[�\�����E�Ɉړ�
//--------------------------------------------------------------
// cs   �J�[�\�����
// flag ��ʕ\���̍X�V���s�����i0:�s��Ȃ� �ȊO:�ύX���������ꍇ�͍X�V����j
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
	if ((cs->cxr)>=gText[cs->rno]->len){						// �s�̍Ō���z����
		if (gText[cs->rno]->chainNext!=0xFFFFFFFF){
			(cs->cxb) = 0;
			(cs->sx) = 0;
		} else {												// �ŏI�s�����ŏI�������z������
			(cs->cxb) = (gRoll ? WWIDTH : LWIDTH);
		}
		CMDown( cs,flag );
	}
	(cs->cxb) = cs->sx;

	gTSel[1].rno = cs->rno;										// �e�L�X�g�͈͎w��p
	gTSel[1].cxr = cs->cxr;
	gTSel[1].line = cs->pline + cs->cy;
	if (gTSel[0].rno!=-1 && flag){								// �e�L�X�g�͈͎w�蒆�Ȃ�
		DrawRec( cs->cy, cs->rno, cs->pline );					// �ړ��O�̃J�[�\���s�\�����X�V
	}
}

//==============================================================
// �J�[�\�������Ɉړ�
//--------------------------------------------------------------
// cs   �J�[�\�����
// flag ��ʕ\���̍X�V���s�����i0:�s��Ȃ� �ȊO:�ύX���������ꍇ�͍X�V����j
//--------------------------------------------------------------

void CMLeft(struct strCsrSts *cs,int flag)
{
	if (cs->cx==0){
		if (gText[cs->rno]->chainBack!=0xFFFFFFFE){					// ��s�ڈȍ~�̂P�����ځi��s�ڂł͉������Ȃ��j
			cs->cxb = (gRoll ? WWIDTH : LWIDTH);					// �s����ڕW��
			CMUp( cs, flag );
		}
	} else {
		cs->cxb = cs->sx;
		CMSet( &cs->cx, &cs->sx, &cs->cxr, cs->cxb, cs->rno, 1 );	// �J�[�\���ʒu�̕␳
		gTSel[1].rno = cs->rno;										// �e�L�X�g�͈͎w��p
		gTSel[1].cxr = cs->cxr;
		gTSel[1].line = cs->pline + cs->cy;
		if (flag && gTSel[0].rno!=-1){								// �e�L�X�g�͈͎w�蒆�Ȃ�
			DrawRec( cs->cy, cs->rno, cs->pline );					// �ړ��O�̃J�[�\���s�\�����X�V
		}
	}
	cs->cxb = cs->sx;
}

//==============================================================
// �s���R�[�h���̕␳
//--------------------------------------------------------------
// cy     �␳���J�n����s�ʒu�i��ʏ�ł̈ʒu�j
// pline  ��ʍŏ�ʈʒu�ɑΉ����镨���s��
// rno    �␳���J�n���郌�R�[�h��
// flag   �␳�ɔ�����ʕ\�����s�����i0:�\�����Ȃ�(�R�s�y�p) �ȊO:�\������j
// �߂�l  0:����I��
//        -1:�s���R�[�h�̎擾���ł��Ȃ�����
//--------------------------------------------------------------
// �s���R�[�h�ɕ�����ǉ�/�폜������s�����ω������ꍇ�Ȃǂɍs���R�[�h�̒����̕␳���s���B
// ��ʂ̍X�V�������ɍs���܂��B
// ���݂̍s���R�[�h�Ǝ��̍s���R�[�h���q������ŉ��߂ĉ�ʏ�łP�s�ɂȂ�悤��
// �s���R�[�h�𕪊�����B
// �ȉ��A�����������s�������Ă������J��Ԃ��B
// �V�����s���R�[�h���K�v�ɂȂ�����ǉ����A�s�v�ƂȂ����s���R�[�h�͍폜����B
// �V�����s���R�[�h���擾�o���Ȃ������ꍇ�A�V�����s�ɌJ��z�����\�肾���������͎����܂��B
//--------------------------------------------------------------

int ClipRec(int cy,long pline,long rno,int flag)
{
	char	tmp[(WWIDTH/2+2)*2+8];									// ��s��+�^�u��
	int		err,cx,cxr,len,cyBak;
	long	i,rno2,lCnt,rnoBak;
	float	sx;

	//----- �s���� -----

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
			if (gText[rno]->next){								// ���̍s�ɑ����Ă���ꍇ
				rno2 = gText[rno]->chainNext;
				len += gText[rno2]->len;
				memcpy( &tmp[gText[rno]->len], gText[rno2]->text, gText[rno2]->len );
				gText[rno2]->len = 0;
			}
		}
		CMSetSub( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), tmp, len, rno, 0 );	// ��ʏ�łP�s�ƂȂ�ʒu�𒲂ׂ�
		if (AddRecText( rno,cxr )){								// �{���̈�̊g���Ɏ��s�����H
			gText[rno]->text[gText[rno]->len] = '\n';
			gText[rno]->next = 0;								// ���̍s�֑����Ȃ�
			gText[rno]->enter = 1;								// ���s����
			err = -1;
			break;												// �ُ�I���i�s���R�[�h���擾�ł��Ȃ��j
		}
		memcpy( gText[rno]->text, tmp, cxr );
		gText[rno]->text[cxr] = '\0';							// [EOF]���O�̍s�Ɉړ�����ۂɃS�~���c��H�̂��ȁH�H
		gText[rno]->len = cxr;									// ��s�ڂ̃��R�[�h
		len -= cxr;
		if (len){												// ���̍s�ɑ�������������ꍇ
			memcpy( tmp, &tmp[cxr], len );
			if (cy<gSLine && flag)
				DrawRec( cy, rno, pline );						// �\�����X�V
			CMSetSub( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), tmp, len, rno, 0 );
			if (len!=cxr){										// ���܂肪��s�ȏ゠��̂ŐV�����s��ǉ�
				rno2 = GetNewRec(TEXTBLSIZE);
				if (rno2!=-1){									// �V�����s���R�[�h���擾�ł����Ȃ�
					gText[rno2]->back = 1;
					gText[rno2]->next = 1;
					gText[rno2]->enter = gText[rno]->enter;
					gText[rno2]->chainNext = gText[rno]->chainNext;
					gText[rno2]->chainBack = rno;				// ���X�g�`�F�C���̐ڑ�
					gText[rno]->next = 1;						// ���̍s�֑���
					gText[rno]->enter = 0;						// ���s�͑��݂��Ȃ�
					gText[rno]->chainNext = rno2;
					if (gText[rno2]->chainNext!=0xFFFFFFFF){
						gText[gText[rno2]->chainNext]->chainBack = rno2;
					}
					lCnt++;
				} else {										// �s���R�[�h���擾�ł��Ȃ��I�I
					CMSet( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), rno, 0 );
					gText[rno]->text[cxr] = '\n';
					gText[rno]->next = 0;						// ���̍s�֑����Ȃ�
					gText[rno]->enter = 1;						// ���s����
					err = -1;
					break;										// �ُ�I���i�s���R�[�h���擾�ł��Ȃ��j
				}
			} else {											// ���܂肪��s�ȉ�
				if (gText[rno]->next){							// ���̍s������Ȃ�q����
					while (1){
						rno2 = gText[rno]->chainNext;
						memcpy( &tmp[len], gText[rno2]->text, gText[rno2]->len );
						len += gText[rno2]->len;
						CMSetSub( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), tmp, len, rno2, 0 );
						if (len!=cxr || !gText[rno2]->next) break;
						gText[rno]->enter = gText[rno2]->enter;	// �q���ł���s�ɖ����Ȃ��Ȃ獡�̍s���R�[�h�͍폜
						gText[rno]->next = gText[rno2]->next;
						gText[rno]->chainNext = gText[rno2]->chainNext;
						if (gText[rno]->chainNext!=0xFFFFFFFF)
							gText[gText[rno]->chainNext]->chainBack = rno;
						DelRec(rno2);							// �s�v�ɂȂ����s���R�[�h���폜
						lCnt--;
					}
				} else {										// ���̍s�������Ȃ�V�����쐬����
					rno2 = GetNewRec(TEXTBLSIZE);
					if (rno2!=-1){								// �V�����s���R�[�h���擾�ł����Ȃ�
						gText[rno2]->back = 1;
						gText[rno2]->next = 0;
						gText[rno2]->enter = gText[rno]->enter;
						gText[rno2]->chainNext = gText[rno]->chainNext;
						gText[rno2]->chainBack = rno;			// ���X�g�`�F�C���̐ڑ�
						gText[rno]->next = 1;					// ���̍s�֑���
						gText[rno]->enter = 0;					// ���s�͑��݂��Ȃ�
						gText[rno]->chainNext = rno2;
						if (gText[rno2]->chainNext!=0xFFFFFFFF){
							gText[gText[rno2]->chainNext]->chainBack = rno2;
						}
						lCnt++;
					} else {									// �s���R�[�h���擾�ł��Ȃ��I�I
						CMSet( &cx, &sx, &cxr, (gRoll ? WWIDTH : LWIDTH), rno, 0 );
						gText[rno]->text[cxr] = '\n';
						gText[rno]->next = 0;					// ���̍s�֑����Ȃ�
						gText[rno]->enter = 1;					// ���s����
						err = -1;
						break;									// �ُ�I���i�s���R�[�h���擾�ł��Ȃ��j
					}
				}
			}
			rno = rno2;
			cy++;

		} else {												// ���̍s�ɑ��������������ꍇ
			if (gText[rno]->next){								// �����O�͎��̍s���������̂ŁA���̍s�͍폜
				gText[rno]->enter = gText[rno2]->enter;
				gText[rno]->next = gText[rno2]->next;
				gText[rno]->chainNext = gText[rno2]->chainNext;
				if (gText[rno]->chainNext!=0xFFFFFFFF)
					gText[gText[rno]->chainNext]->chainBack = rno;
				DelRec(rno2);									// �s�v�ɂȂ����s���R�[�h���폜
				lCnt--;
			}
			break;												// �����I��
		}
	}

	//----- ��ʕ\�����X�V -----

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
			DrawRec( cy++, rno, pline );						// �\�����X�V
			if (!gText[rno]->next) break;
			rno = gText[rno]->chainNext;
		}
	}

	return (err);
}

//==============================================================
// �J�[�\���ʒu�ɉ��s��}��
//--------------------------------------------------------------
// cs       �J�[�\�����
// flag     ��ʕ\���̍X�V���s�����i0:�s��Ȃ� �ȊO:�ύX���������ꍇ�͍X�V����j
// undoflag �u��蒼���v�o�b�t�@�ɋL�^���邩�i0:�L�^���Ȃ� �ȊO:�L�^����j
//--------------------------------------------------------------
// �V�����s���R�[�h���擾�o���Ȃ������ꍇ�͉���������-1��Ԃ��B
//--------------------------------------------------------------

int AddRet(struct strCsrSts *cs,int flag,int undoflag)
{
	char	c,buf[(WWIDTH/2+2)*2];
	int		i,pos,len,cyBak;
	long	rno2,rno3;

	//----- �V�����s���R�[�h���擾�ł���H -----

	rno2 = GetNewRec(TEXTBLSIZE);
	if (rno2==-1) return (-1);									// �V�����s���R�[�h���擾�o���Ȃ��ꍇ
	DelRec(rno2);

	//----- �}���ʒu�C�� -----

	if ((cs->cxr)==0 && gText[cs->rno]->back){					// �O�̍s�̍Ō���ɒǉ����ׂ��ꍇ�����肤��̂�
		cs->cxb = (gRoll ? WWIDTH : LWIDTH);
		cyBak = cs->cy;
		CMUp( cs,0 );											// �O�̍s�̍s���ɃJ�[�\�����ړ�
		if (gText[cs->rno]->enter){								// ����ς莟�̍s�̐擪�ɒǉ��������������̂�
			CMRight( cs,0 );
		} else if (cyBak==0){									// �O�̍s����ʊO�̏ꍇ
			if (flag){
				RollDown(0);
				DrawRec( 0, cs->rno, cs->pline );
			}
		}
	}

	//----- ��蒼�� -----

	if (undoflag){
		gUndo[gUndoPos].pline = cs->pline + cs->cy;					// �u��蒼���v�f�[�^�ۑ�
		gUndo[gUndoPos].cxr = cs->cxr;
		gUndo[gUndoPos].str[0] = '\n';
		gUndo[gUndoPos].str[1] = 0;
		gUndo[gUndoPos].str[2] = 1;									// ��������
		gUndoPos++;
		if (gUndoPos>=UNDOSIZE) gUndoPos = 0;
		gUndoSize++;
		if (gUndoSize>=UNDOSIZE) gUndoSize = UNDOSIZE;
	}

	//----- ���s�R�[�h�̑}�� -----

	pos = 0;
	len = 0;
	for ( i=(cs->cxr); i<gText[cs->rno]->len ;i++ ){			// ���̍s�ɑ����镶������o�b�t�@�֑Ҕ�
		buf[pos++] = gText[cs->rno]->text[i];
		len++;
	}

	if (!gText[cs->rno]->next){									// ���̍s�ɑ����Ă��Ȃ����
		rno2 = GetNewRec(TEXTBLSIZE);
		if (rno2!=-1){											// �V�����s���R�[�h���擾�ł����Ȃ�
			if (AddRecText(rno2,len)){							// �{���̈�̊g���Ɏ��s�����H
				DelRec(rno2);									// �m�ۂ����s���R�[�h��p��
				if (undoflag){
					gUndoPos--;									// �u��蒼���v�f�[�^���L�����Z��
					if (gUndoPos<0) gUndoPos = UNDOSIZE;
					gUndoSize--;
				}
				return (-1);
			}
			gText[rno2]->back = 0;
			gText[rno2]->next = gText[cs->rno]->next;
			gText[rno2]->enter = gText[cs->rno]->enter;
			gText[rno2]->chainNext = gText[cs->rno]->chainNext;
			gText[rno2]->chainBack = cs->rno;					// ���X�g�`�F�C���̐ڑ�
			gText[cs->rno]->next = 0;
			gText[cs->rno]->enter = 1;							// ���s����
			gText[cs->rno]->chainNext = rno2;
			if (gText[rno2]->chainNext!=0xFFFFFFFF){
				gText[gText[rno2]->chainNext]->chainBack = rno2;
			}
			if ((cs->cy)+1<gSLine && flag){						// �V�����쐬�����s����ʓ��Ȃ�
				RollDown((cs->cy)+1);							// �ǉ������s�̕��̃X�y�[�X���m�ہi�s���e�̍X�V�͌�ōs����j
			}
		} else {												// �s���R�[�h���擾�ł��Ȃ��I�I
			if (undoflag){
				gUndoPos--;										// �u��蒼���v�f�[�^���L�����Z��
				if (gUndoPos<0) gUndoPos = UNDOSIZE;
				gUndoSize--;
			}
			return (-1);										// ���������Ɉُ�I��
		}
		gText[cs->rno]->len = (cs->cxr)+1;
		gText[cs->rno]->text[cs->cxr] = '\n';
		gText[rno2]->len = len;
		memcpy( gText[rno2]->text, buf, len );
		if (flag){
			DrawRec( (cs->cy)+0, cs->rno, cs->pline );			// ��s��
			if ((cs->cy)+1<gSLine){
				DrawRec( (cs->cy)+1, rno2, cs->pline    );		// ��s��
			}
		}

	} else {													// �s���܂�Ԃ���Ă���ꍇ
		gText[cs->rno]->len = (cs->cxr)+1;
		gText[cs->rno]->text[cs->cxr] = '\n';
		gText[cs->rno]->next = 0;
		gText[cs->rno]->enter = 1;								// ���s����
		gText[gText[cs->rno]->chainNext]->back = 0;
		if (flag){
			DrawRec( cs->cy, cs->rno, cs->pline );				// ��s��
		}
		rno2 = gText[cs->rno]->chainNext;
		while (1){												// �o�b�t�@�̕���������̍s�֑}��
			memcpy( &buf[len], gText[rno2]->text, gText[rno2]->len );
			len += gText[rno2]->len;
			pos = 0;
			while (pos<gLineLen-2 && pos<len){
				if (AddRecText(rno2,pos+2)){					// �{���̈�̊g���Ɏ��s�����H
					len = pos;									// �o�b�t�@���̕������p��
					break;										// �������f
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
			if (!len) break;									// ���̍s�ɌJ��z�������񂪖����Ȃ�
			memcpy( buf, &buf[pos], len );
			if (gText[rno2]->next){								// ���̍s������Ȃ�
				rno2 = gText[rno2]->chainNext;
			} else {											// ���̍s�������̂ŐV�����擾����
				rno3 = GetNewRec(TEXTBLSIZE);
				if (rno3!=-1){									// �V�����s���R�[�h���擾�ł����Ȃ�
					gText[rno3]->back = 1;
					gText[rno3]->next = gText[rno2]->next;
					gText[rno3]->enter = gText[rno2]->enter;
					gText[rno3]->chainNext = gText[rno2]->chainNext;
					gText[rno3]->chainBack = rno2;				// ���X�g�`�F�C���̐ڑ�
					gText[rno2]->next = 1;
					gText[rno2]->enter = 0;						// ���s�Ȃ�
					gText[rno2]->chainNext = rno3;
					if (gText[rno3]->chainNext!=0xFFFFFFFF){
						gText[gText[rno3]->chainNext]->chainBack = rno3;
					}
					if ((cs->cy)+1<gSLine && flag){				// �V�����쐬�����s����ʓ��Ȃ�
						RollDown((cs->cy)+1);					// �ǉ������s�̕��̃X�y�[�X���m�ہi�s���e�̍X�V�͌�ōs����j
					}
					gText[rno3]->len = 0;
					rno2 = rno3;
				} else {										// �s���R�[�h���擾�ł��Ȃ��I�I
					gText[rno2]->text[gText[rno2]->len] = '\n';
					gText[rno2]->len++;
					break;										// �o�b�t�@���ɕ����񂪎c���Ă��邪�̂Ă邵���Ȃ�
				}
			}
		}
		ClipRec( (cs->cy)+1, cs->pline, gText[cs->rno]->chainNext, flag );
	}

	CMRight( cs,flag );											// �J�[�\�����ꕶ����������
	return (0);
}

//==============================================================
// �J�[�\���ʒu�̕������폜
//--------------------------------------------------------------
// cs       �J�[�\�����
// flag     ��ʕ\���̍X�V���s�����i0:�s��Ȃ� �ȊO:�ύX���������ꍇ�͍X�V����j
// undoflag �u��蒼���v�o�b�t�@�ɋL�^���邩�i0:�L�^���Ȃ� �ȊO:�L�^����j
//--------------------------------------------------------------

void DelStr(struct strCsrSts *cs,int flag,int undoflag)
{
	int		i,step;

	if ((cs->cxr)>=gText[cs->rno]->len) return;					// �J�[�\���ʒu��[EOF]�̏ꍇ�͍폜�̎��s�͂ł��Ȃ�

	if (gText[cs->rno]->text[cs->cxr]=='\n'){					// ���s�R�[�h���폜����ꍇ�A���̍s�Ɠ���������K�v�����邪
		gText[cs->rno]->enter = 0;								// �����ł͍s���R�[�h�̐ݒ�������邾���ŁA�{���̓�����
		gText[cs->rno]->next = 1;								// ClipRec()���ɂď������܂�
		gText[gText[cs->rno]->chainNext]->back = 1;
	}

	if (chkSJIS(gText[cs->rno]->text[cs->cxr])){				// �S�p���p����
		step = 2;
	} else {
		step = 1;
	}
	if (undoflag){
		gUndo[gUndoPos].pline = cs->pline + cs->cy;				// �u��蒼���v�f�[�^�ۑ�
		gUndo[gUndoPos].cxr = cs->cxr;
		gUndo[gUndoPos].str[0] = gText[cs->rno]->text[cs->cxr];
		gUndo[gUndoPos].str[1] = (step==1 ? 0 : gText[cs->rno]->text[cs->cxr+1]);
		gUndo[gUndoPos].str[2] = 0;								// �����폜
		gUndoPos++;
		if (gUndoPos>=UNDOSIZE) gUndoPos = 0;
		gUndoSize++;
		if (gUndoSize>=UNDOSIZE) gUndoSize = UNDOSIZE;
	}
	for (i=cs->cxr; i<gText[cs->rno]->len-1 ;i++)
		gText[cs->rno]->text[i] = gText[cs->rno]->text[i+step];
	gText[cs->rno]->len -= step;
	ClipRec( cs->cy, cs->pline, cs->rno, flag );				// �s���R�[�h�̒����̕␳
	if (gText[cs->rno]->chainBack!=0xFFFFFFFE || cs->cx!=0){	// ��s�ڂP�����ڂł͂Ȃ��ꍇ�A�J�[�\���ʒu�␳
		CMLeft( cs,flag );
		CMRight( cs,flag );
	}
}

//==============================================================
// �J�[�\���ʒu�ɕ������ǉ�
//--------------------------------------------------------------
// cs       �J�[�\�����
// str      �ǉ����镶����
// flag     ��ʕ\���̍X�V���s�����i0:�s��Ȃ� �ȊO:�ύX���������ꍇ�͍X�V����j
// undoflag �u��蒼���v�o�b�t�@�ɋL�^���邩�i0:�L�^���Ȃ� �ȊO:�L�^����j
// �߂�l    0:����
//          -1:������������Ȃ�
//--------------------------------------------------------------
// �w�肳�ꂽ������͂P�������ǉ����Ă��܂��B
// �܂Ƃ߂ď��������肱�����̂ق��������菇��z�����₷���̂ŁB
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
		if (chkSJIS(str[pos])){									// �S�p���p����
			step = 2;
			c[0] = str[pos+0];
			c[1] = str[pos+1];
			wc = GetStrWidth( gFont,c );
			if ((cs->sx)+wc>=(gRoll ? WWIDTH : LWIDTH)) mv = 1;
		} else {
			step = 1;
			if (cs->cxr==0 && gText[cs->rno]->back){			// �O�̍s�̍Ō���ɒǉ����ׂ��ꍇ�����肤��̂�
				cs->cxb = (gRoll ? WWIDTH : LWIDTH);
				CMUp( cs,0 );									// �O�̍s�̍s���ɃJ�[�\�����ړ�
			}
			c[0] = str[pos];
			c[1] = '\0';
			wc = GetStrWidth( gFont,c );
			if ((cs->sx)+wc>=(gRoll ? WWIDTH : LWIDTH)) mv = 1;	// �J�[�\���ʒu�␳
		}
		if (str[pos]=='\n'){
			if (AddRet( cs,flag,undoflag )){					// ������������Ȃ�
				err = -1;
				break;
			}
			pos++;
		} else {
			if (AddRecText(cs->rno,gText[cs->rno]->len+step)){	// �{���̈�̊g���Ɏ��s�����H
				err = -1;
				break;
			}
			p = gText[cs->rno]->len;
			if (p!=0){											// ������ǉ�����X�y�[�X���m��
				do {
					p--;
					gText[cs->rno]->text[p+step] = gText[cs->rno]->text[p];
				} while (p>(cs->cxr));
			}
			memcpy( &gText[cs->rno]->text[cs->cxr], &str[pos], step );	// ������ǉ�
			gText[cs->rno]->len += step;
			pos += step;
			if (undoflag){
				gUndo[gUndoPos].pline = cs->pline + cs->cy;		// �u��蒼���v�f�[�^�ۑ�
				gUndo[gUndoPos].cxr = cs->cxr;
				gUndo[gUndoPos].str[0] = c[0];
				gUndo[gUndoPos].str[1] = c[1];
				gUndo[gUndoPos].str[2] = 1;						// ��������
				gUndoPos++;
				if (gUndoPos>=UNDOSIZE) gUndoPos = 0;
				gUndoSize++;
				if (gUndoSize>=UNDOSIZE) gUndoSize = UNDOSIZE;
			}
			err = ClipRec( cs->cy, cs->pline, cs->rno, flag );	// �s���R�[�h�̒����̕␳
			if (err) break;										// �s���R�[�h���擾�ł��Ȃ��Ȃ珈���𒆒f
			CMRight( cs,flag );									// �J�[�\�����ꕶ����������
			if (mv){
				CMRight( cs,flag );								// �J�[�\���ʒu�␳
			}
		}
		mv = 0;
	}
	return (err);
}

//==============================================================
// �����J�[�\�����w��s�ʒu�Ɉړ�������
//--------------------------------------------------------------
// cs    �J�[�\�����
// pline �ړ��������������s�ʒu
//--------------------------------------------------------------
// �w��s�ʒu����ʒ����ɗ���悤�ɃJ�[�\�������C������B
// �C�������̂́A
//     cs->cy
//     cs->rno
//     cs->pline
//     cs->rline
// �ł��B
//--------------------------------------------------------------

void SetCursor(struct strCsrSts *cs,long pline)
{
	long	rno,line,rline;

	if (pline>gLineMax) pline = gLineMax;
	if (gLineMax<gSLine+1+gReDraw){									// �S�������ʓ��Ɏ��܂�ꍇ
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
		if (pline>gLineMax-gSLine+1+gReDraw){						// ���͂̏I���̈��ʓ��ȏꍇ
			cs->cy = pline + cs->cy - (gLineMax-gSLine+1+gReDraw);
			pline = gLineMax - gSLine+1 + gReDraw;
		}
	}
	cs->pline = pline;												// �����O�̘_���s�ʒu�ɉ�ʂ��Z�b�g

	rno = 0;
	rline = 0;
	line = 0;
	while (line!=pline+cs->cy && rno!=0xFFFFFFFF){					// �w�蕨���s�ʒu��T��
		if (gText[rno]->enter) rline++;
		line++;
		rno = gText[rno]->chainNext;
	};
	cs->rno = rno;
	cs->rline = rline;
}

//==============================================================
// ��ʈʒu�𕶎��J�[�\�����\�������ʒu�ɕύX����
//--------------------------------------------------------------
// cs �J�[�\�����
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
// ���j���[�i�e�L�X�g�̕ۑ��j
//--------------------------------------------------------------

int MenuSave(char *filepath)
{
	char			msg[128];
	int				ret;

	ret = filesave(filepath);
	if (ret==-1){
		strcpy( msg, "���[�N���������m�ۂł��܂���ł����B" );
	} else if (ret==-2){
		strcpy( msg, "�t�@�C������ŃG���[���������܂����B\n(" );
		strcat( msg, filepath );
		strcat( msg, ")\n" );
	} else if (ret==1){
		
	}
	if (ret){
		strcat( msg, "\n�e�L�X�g�͕ۑ�����Ă��܂���B" );
		DiaBox1( -1, 120, 0, msg, gSCor[0], gSCor[3], gSCor[4] );	// �����ɕ\��
		Unpush();												// �{�^�����������܂őҋ@
		WaitBtn();												// �����������܂őҋ@
		return (-2);											// �G���[����
	}
	return (0);													// �ۑ�����
}

//==============================================================
// ���j���[�i�t�@�C���̕ۑ��j
//--------------------------------------------------------------

void MenuLoad(char *filepath)
{
	char	msg[128];
	int		ret;

	ret = fileload(filepath);
	if (ret==1 || ret==-3){
		switch (ret){
		case 2:
			strcpy( msg,"�ǂݍ��݂��L�����Z������܂����B" );
			break;
		case 1:
			strcpy( msg,"������������Ȃ��Ȃ����̂ŕ��͂̓ǂݍ��݂𒆒f���܂����B" );
			break;
		}
		DiaBox1( -1, 120, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
		Unpush();												// �{�^�����������܂őҋ@
		WaitBtn();												// �����������܂őҋ@
	}
}

//==============================================================
// ���j���[�i�e�L�X�g�̃y�[�X�g�j
//--------------------------------------------------------------
// ���Ɏ�荞�܂�Ă���e�L�X�g���J�[�\���ʒu�ɓ\��t���܂��B
//--------------------------------------------------------------

void TextPaste(struct strCsrSts *cs)
{
	if (gTextClip==NULL) return;

	if (strlen(gTextClip)){										// �y�[�X�g���镶��������Ȃ�
		if (AddStr( cs, gTextClip, 0, 1 )){
			DiaBox1( -1, 120, 0, "������������Ȃ��Ȃ�܂����B", gSCor[0], gSCor[3], gSCor[4] );	// �����ɕ\��
			Unpush();												// �{�^�����������܂őҋ@
			WaitBtn();												// �����������܂őҋ@
		}
		gSave = 1;
	}
}

//==============================================================
// ���j���[�i�e�L�X�g�̃R�s�[�j
//--------------------------------------------------------------
// gTSel[0]�`gTSel[1]�͈̔͂̃e�L�X�g����荞�݂܂��B
// �͈͎w�肪�L���ȏ�ԂŎ��s���邱�ƁB
// �ŏ��̎��_�ł�[0]��[1]�̂ǂ��炪�擪����������Ȃ��̂Œ��ӁB
// ��荞�܂ꂽ�e�L�X�g��gTextClip�ɕۊǂ���܂��B
//--------------------------------------------------------------

char* TextCapture(void)
{
	char	*buf;
	int		cxr,cxrEnd;
	long	rno,rno2,rnoEnd,pos,pos0,pos1,size;

	pos0 = gTSel[0].line * gLineLen + gTSel[0].cxr;
	pos1 = gTSel[1].line * gLineLen + gTSel[1].cxr;
	if (pos0>pos1){												// �����̊J�n�ʒu�ƏI���ʒu
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
	while (rno2!=rnoEnd && rno2!=0xFFFFFFFF){					// ��荞�݂ɕK�v�ȃX�y�[�X���Z�o
		rno2 = gText[rno2]->chainNext;
		size += gText[rno2]->len;
	}
	if ((buf = (char*) malloc(size))!=NULL){					// ��Ɨ̈���m�ۂł����Ȃ�
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
	if (gTextClip!=NULL) free(gTextClip);						// ���Ɏ�荞�܂�Ă��镶�͂��폜
	gTextClip = TextCapture();
	gTCChg = 1;													// �ύX��������
}

//==============================================================
// ���j���[�i�͈͍폜�j
//--------------------------------------------------------------
// gTSel[0]�`gTSel[1]�͈̔͂̃e�L�X�g���폜���܂��B
// �͈͎w�肪�L���ȏ�ԂŎ��s���邱�ƁB
// ���s��A�J�[�\���ʒu���폜�J�n�ʒu�Ɉړ����܂��B
// ��ʍ��͍s���܂���B
//--------------------------------------------------------------

void TextDel(struct strCsrSts *cs)
{
	int		cxr,cxrEnd;
	long	rno,rnoEnd,pos0,pos1,lp,plineBak;

	pos0 = gTSel[0].line * gLineLen + gTSel[0].cxr;
	pos1 = gTSel[1].line * gLineLen + gTSel[1].cxr;
	if (pos0==pos1) return;
	if (pos0>pos1){												// �����̊J�n�ʒu�ƏI���ʒu
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
	CRSet(cs);													// �͈͎w��̍Ō���ʒu�ɃJ�[�\���ʒu���ړ�
	while ((cs->rno)!=rno || (cs->cxr)!=cxr){					// 1�����Â����Ă���
		CMLeft( cs,0 );											// �J�[�\������o�b�N������
		DelStr( cs,0,1 );
	}

	lp = GetLine(rno);
	if (plineBak!=cs->pline){									// �J�[�\���ʒu����ʔ͈͂���O�ꂽ
		cs->cy = 0;
		cs->pline = lp;
	} else {
		cs->cy = lp - (cs->pline);
	}
}

//==============================================================
// ���j���[�i�t�@�C�����e�̓\��t���j
//--------------------------------------------------------------
// ���ꂪ�g����ƂȂ񂩕֗��炵���ƕ������̂Ŏ������Ă݂��B
// �Ώۃt�@�C���̕��͓����s�R�[�h��CRLF��LF�ɑΉ��B
// ��ʍ��͍s���܂���B
//--------------------------------------------------------------

int FilePaste(struct strCsrSts *cs)
{
	char	c,*data,path[1024],msg[128];
	int		fd;
	long	i,pos1,pos2,filesize;

	//----- �t�@�C���I�� -----

	if (SelectFile("�t�@�C������\\��t��",gPFolder,"",path,0,gIME,gHisFPaste))
		return (-1);											// �L�����Z�����ꂽ

	//----- �t�@�C���ǂݍ��� -----

	fd = sceIoOpen( path, PSP_O_RDONLY, 0777 );
	data = NULL;
	if (fd>=0){
		filesize = sceIoLseek( fd, 0, SEEK_END );
		sceIoLseek( fd, 0, SEEK_SET );
		data = (char*) malloc(filesize);
		if (data==NULL){										// ���������m�ۂł��Ȃ�����
			strcpy( msg,"���[�N���������m�ۂł��܂���ł����B" );
		} else {
			sceIoRead( fd, data, filesize );
		}
		sceIoClose(fd);
	} else {
		strcpy( msg,"�t�@�C�����J���܂���ł����B" );
	}
	if (data==NULL){											// ��蔭��
		DiaBox1( -1, 120, 0, msg, gSCor[0], gSCor[3], gSCor[4] );	// �����ɕ\��
		Unpush();												// �{�^�����������܂őҋ@
		WaitBtn();												// �����������܂őҋ@
		return (0);
	}

	//----- ���s�R�[�h�C�� -----

	pos1 = 0;
	pos2 = 0;
	for (i=0; i<filesize ;i++){
		c = data[pos1++];
		if (c!='\r' && c!=0x1A)									// CR��EOF����菜��
			data[pos2++] = c;
	}
	data[pos2] = '\0';

	//----- ���͂ɓ\��t�� -----

	if (strlen(data)){											// ���͂�����Ȃ�
		if (AddStr( cs, data, 0, 1)){
			DiaBox1( -1, 120, 0, "������������Ȃ��Ȃ�܂����B", gSCor[0], gSCor[3], gSCor[4] );	// �����ɕ\��
			Unpush();												// �{�^�����������܂őҋ@
			WaitBtn();												// �����������܂őҋ@
		}
		gSave = 1;
	}

	//----- ��n�� -----

	free(data);
	return (0);
}

//==============================================================
// ���j���[�i�t�@�C���֐؂�o���j
//--------------------------------------------------------------
// gTSel[0]�`gTSel[1]�͈̔͂̃e�L�X�g���w��t�@�C���֏����o���܂��B
// �͈͎w�肪�L���ȏ�ԂŎ��s���邱�ƁB
//--------------------------------------------------------------

int TextSave(void)
{
	char	*buf,*buf2,path[1024];
	int		fd;
	long	i,pos,size;

	buf = TextCapture();										// �w��͈͂̃e�L�X�g����荞��
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

	if (SelectFile("�t�@�C���֐؂�o��",gPFolder,"",path,-1,gIME,gHisFPaste)){
		free(buf);
		return (2);												// �L�����Z�����ꂽ
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
// ���j���[�i�w��s�փW�����v�j
//--------------------------------------------------------------
// ���͂��ꂽ�_���s�ʒu�ֈړ�����B
// ��ʍ��͍s���܂���B
//--------------------------------------------------------------

int TextJump(struct strCsrSts *cs)
{
	char	msg[128],str[128];
	long	rno,rline,pline,maxline;

	rno = 0;
	maxline = 0;
	do{															// �s�̑������J�E���g
		if (gText[rno]->enter) maxline++;
		rno = gText[rno]->chainNext;
	} while (rno!=0xFFFFFFFF);

	sprintf( msg,"�W�����v�������s�ʒu����͂��Ă��������B(1�`%ld)",maxline+1 );
	if (InputName(msg,9,gIME,"",str,gHisLJump)){
		return (-1);											// �L�����Z��
	}
	if (strlen(str)==0) return (-1);
	rline = atol(str) -1;
	if (rline<0) rline = 0;
	if (rline>maxline) rline = maxline;

	rno = 0;
	pline = 0;
	maxline = 0;
	while (rline!=maxline && rno!=0xFFFFFFFF){					// �w��_���s�ʒu��T��
		if (gText[rno]->enter) maxline++;
		pline++;
		rno = gText[rno]->chainNext;
	};
	SetCursor( cs,pline );										// �w��s�ʒu�ɕ����J�[�\�����ړ�

	cs->cx = 0;													// ��ʏ�ł̕����J�[�\��X�ʒu�i�����P�ʁj
	cs->sx = 0;													// ��ʏ�ł̕����J�[�\��X�ʒu�i�h�b�g�P�ʁj
	cs->cxr = 0;												// �_���J�[�\��X�ʒu
	cs->cxb = 0;												// �J�[�\��X�ʒu�w�莞�̓��B�ڕW�|�C���g�i�h�b�g�P�ʁj
	cs->adjust = 1;												// ��ʈʒu���J�[�\���ɍ��킹��i0:���̂܂� 1:���킹��j
	return (0);
}

//==============================================================
// ���j���[�i�����񌟍��j
//--------------------------------------------------------------
// �J�[�\���ʒu����w�肳�ꂽ��������������A���̐擪�ʒu�ɃJ�[�\�����ړ�������B
// �K�v�ɉ����ĉ�ʍ�悪�s���܂��B
//--------------------------------------------------------------

//----- �I�𕶎���̎�荞�� -----

int GetStrBlk(char *str)
{
	int		cxr,cxrEnd,pos;
	long	rno,rnoEnd,pos0,pos1;

	str[0] = '\0';
	if (gTSel[0].rno!=-1){
		pos0 = gTSel[0].line * gLineLen + gTSel[0].cxr;
		pos1 = gTSel[1].line * gLineLen + gTSel[1].cxr;
		if (pos0>pos1){											// �����̊J�n�ʒu�ƏI���ʒu
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
			if (gText[rno]->text[cxr]<32U) break;				// �R���g���[���R�[�h�͑ΏۂɂȂ�Ȃ�
			str[pos] = gText[rno]->text[cxr++];
			if (chkSJIS(str[pos++])){
				str[pos++] = gText[rno]->text[cxr++];
			}
			if (pos>=49) break;									// ����������
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

//----- ���݂̃J�[�\���ʒu�ȍ~�Ŏw�肳�ꂽ������̂���ʒu�ɃJ�[�\�����Z�b�g -----

int StrSrchSub(struct strCsrSts *cs,char *str)
{
	int		i,pos,pos1,posx;
	long	rno,rno2,pline,pline2;

	rno = cs->rno;
	pline = cs->pline + cs->cy;
	pos = cs->cxr;
	pos++;
	if (gText[rno]->len<pos){									// �����J�n�̓J�[�\���ʒu�̎�����
		pos = 0;
		rno = gText[rno]->chainNext;
		pline++;
	}
	posx = -1;
	while (rno!=0xFFFFFFFF){
		for (pos=pos; pos<gText[rno]->len ;pos++){
			if (gText[rno]->text[pos]==str[0]){					// �ꕶ���ڂ������Ă���H
				gTSel[1].line = pline;
				gTSel[1].rno = rno;
				gTSel[1].cxr = pos;
				pos1 = pos;
				rno2 = rno;
				pline2 = pline;
				for (i=1; i<strlen(str) ;i++){					// �񕶎��ڈȍ~
					pos1++;
					if (gText[rno2]->len<=pos1){				// �s�[�𒴂��鏈��
						pos1 = 0;
						pline2++;
						rno2 = gText[rno2]->chainNext;
						if (rno2==0xFFFFFFFF) break;			// [EOF]�ɂԂ�������
					}
					if (gText[rno2]->text[pos1]!=str[i]) break;	// �񕶎��ڈȍ~�ň�v���Ȃ��Ȃ�
				}
				if (strlen(str)==i){							// �ڕW�����I
					posx = pos;
					pos1++;
					if (gText[rno2]->len<pos1){					// �s�[�𒴂��鏈��
						if (gText[rno2]->enter) break;			// ���s������Ȃ�`�F�b�N���~
						pos1 = 0;
						pline2++;
						rno2 = gText[rno2]->chainNext;
					}
					gTSel[0].line = pline2;						// �ڕW��I����Ԃ�
					gTSel[0].rno = rno2;
					gTSel[0].cxr = pos1;
					break;
				}
			}
		}
		if (posx!=-1) break;									// �ڕW�����I
		pos = 0;
		rno = gText[rno]->chainNext;
		pline++;
	}

	//----- �ڕW�ʒu�ɃJ�[�\�����ړ� -----

	if (posx==-1){												// �ڕW�����Ȃ炸
		gTSel[0].rno = -1;
		return (-1);
	}
	SetCursor( cs,pline );										// �w��s�ʒu�ɕ����J�[�\�����ړ�
	cs->cxr = posx;
	CRSet(cs);													// �w�肳�ꂽ�_�������ʒu�ɃJ�[�\�����ړ�
	if (gRoll){													// ��ʉ��T�C�Y�g�����[�h�Ȃ�
		ScreenAdj(cs);											// ��ʈʒu��␳
	}
	cs->adjust = 0;												// ��ʈʒu���J�[�\���ɍ��킹��i0:���̂܂� 1:���킹��j
	DrawSts( cs, 1 );											// ���[���[�n�ƃX�N���[���o�[�X�V
	DrawFullRec(cs->pline);										// ��ʑS�����������
	return (0);
}

//----- �_�C�A���O��� -----

void DrawStrSrch(int no)
{
	int		x,y;

	x = 480-4-130;
	y = 272-4-56;
	DialogFrame( x, y, 10+110+10, 6+12+4+13*1-1+4+12+6, "�����񌟍�", "��:�I�� �~:��ݾ�", gSCor[1], gSCor[2] );
	Fill( x+10, y+6+12+4, 110, 12, gSCor[5] );
	CurveBox( x+10, y+6+12+4, 110, 12, 0, gSCor[10], gSCor[11] );
	mh_print( x+10+1, y+6+12+4, "���̌���", gSCor[0] );
}

//----- ���C�� -----

int StrSrch(struct strCsrSts *cs)
{
	SceCtrlData		pad;
	char	strBuf[128] = {0};
	char	str[128];
	int		x,y,no,ret,exit;
	DrawImg	*image;

	//----- �I�𕶎���̎�荞�� -----

	if (GetStrBlk(str)!=0) strcpy( strBuf,str );				// �����񂪎擾�o���Ă���Ȃ�̗p

	//----- ��������� -----

	if (InputName("�������镶�������͂��Ă��������B",50,gIME,strBuf,str,gHisSarh)){
		return (-1);											// �L�����Z��
	}
	if (strlen(str)==0) return (-1);
	gTSel[0].rno = -1;											// �I�𕶎���̉���

	//----- �����񌟍� -----

	if (StrSrchSub( cs,str )){									// ���̌�₪�����Ȃ�I��
		return (1);
	}
	image = ScreenCopy();
	exit = 0;
	no = 0;
	DrawStrSrch(no);
	while (!gExitRequest && !exit){								// �L�[���̓��[�v
		x = cs->sx - gXOffset;
		y = TBY + cs->cy * gFHeight;							// �J�[�\�����W�ݒ�
		SIMEcursor( 2, x, y );									// �J�[�\���̑傫���ƍ��W�ϐ��̎w��
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		SIMEDrawCursor(ret);									// SIME�J�[�\���̍��
		switch (ret){
		case SIME_KEY_CIRCLE:									// ��
			ScreenPaste(image);
			if (StrSrchSub( cs,str )){							// ���̌�₪�����Ȃ�I��
				exit = -1;
			} else {
				image = ScreenCopy();
				DrawStrSrch(no);
			}
			break;
		case SIME_KEY_CROSS:									// �~
			ScreenPaste(image);
			exit = -1;
			break;
		case SIME_KEY_UP:										// ��
			no = 0;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawStrSrch(no);
			break;
		case SIME_KEY_DOWN:										// ��
			no = 1;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawStrSrch(no);
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// ��ʍX�V
	}

	return (0);
}

//==============================================================
// ���j���[�i������u���j
//--------------------------------------------------------------
// �J�[�\���ʒu����w�肳�ꂽ��������������A�X�ɒu�����邩�ǂ����m�F���Ă���
// �܂��B
// �K�v�ɉ����ĉ�ʍ�悪�s���܂��B
//--------------------------------------------------------------

//----- �_�C�A���O���i�ݒ��ʁj -----

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

//----- �_�C�A���O���i�u�����m�F�j -----

void DrawStrChgNext(int no)
{
	int		x,y;

	x = 480-4-130;
	y = 272-4-69;
	DialogFrame( x, y, 10+110+10, 6+12+4+13*2-1+4+12+6, "������u��", "��:�I�� �~:��ݾ�", gSCor[1], gSCor[2] );
	if (no){
		Fill( x+10, y+6+12+4, 110, 12, gSCor[5] );
		CurveBox( x+10, y+6+12+4+13, 110, 12, 0, gSCor[10], gSCor[11] );
	} else {
		CurveBox( x+10, y+6+12+4, 110, 12, 0, gSCor[10], gSCor[11] );
		Fill( x+10, y+6+12+4+13, 110, 12, gSCor[5] );
	}
	mh_print( x+10+1, y+6+12+4   , "�u�����Ď��̌���", gSCor[0] );
	mh_print( x+10+1, y+6+12+4+13, "���̂܂܎��̌���", gSCor[0] );
}

//----- ���C�� -----

int StrChg(struct strCsrSts *cs)
{
	SceCtrlData		pad;
	char	source[128] = {0},									// ���ƂȂ镶����
			dest[128] = {0};									// �u�������镶����
	char	str[128];											// �ꎞ����p
	int		x,y,no,sx,sy,pos,ret,exit;
	DrawImg	*image;

	//----- �I�𕶎���̎�荞�� -----

	if (GetStrBlk(str)!=0){
		strcpy( source,str );									// �����񂪎擾�o���Ă���Ȃ�̗p
	}

	//----- �ݒ� -----

	sx = 240-346/2;
	sy = 90;
	DialogFrame( sx, sy, 346, 6+12+4+13*2-1+4+12+6, "������u��", "������:�ݒ�ύX ��:���� �~:��ݾ�", gSCor[1], gSCor[2] );
	mh_print( sx+10, sy+6+12+4   , "����", gSCor[0] );
	mh_print( sx+10, sy+6+12+4+13, "�u��", gSCor[0] );
	no = 0;
	DrawStrChgSet( sx, sy, no, source, dest );

	exit = 0;
	while (!gExitRequest && !exit){								// �L�[���̓��[�v
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		switch (ret){
		case SIME_KEY_CIRCLE:									// ��
			exit = 1;
			break;
		case SIME_KEY_CROSS:									// �~
			exit = -1;
			break;
		case SIME_KEY_TRIANGLE:									// ��
		case SIME_KEY_LEFT:										// ��
		case SIME_KEY_RIGHT:									// ��
			image = ScreenCopy();
			if (no==0){
				if (!InputName("�u�����錳�̕��������͂��Ă��������B",50,gIME,source,str,gHisSarh)){
					strcpy( source,str );
				}
			} else {
				if (!InputName("�u������̕��������͂��Ă��������B",50,gIME,dest,str,gHisChg)){
					strcpy( dest,str );
				}
			}
			ScreenPaste(image);
			DrawStrChgSet( sx, sy, no, source, dest );
			break;
		case SIME_KEY_UP:										// ��
			no = 0;
			DrawStrChgSet( sx, sy, no, source, dest );
			break;
		case SIME_KEY_DOWN:										// ��
			no = 1;
			DrawStrChgSet( sx, sy, no, source, dest );
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();
	}

	if (gExitRequest || exit==-1 || strlen(source)==0){			// �L�����Z�����ꂽ
		return (-1);
	}
	gTSel[0].rno = -1;											// �I�𕶎���̉���

	//----- �����񌟍��ƒu���� -----

	if (StrSrchSub( cs,source )){								// ���̌�₪�����Ȃ�I��
		return (1);
	}
	image = ScreenCopy();
	exit = 0;
	no = 0;
	DrawStrChgNext(no);
	while (!gExitRequest && !exit){								// �L�[���̓��[�v
		x = cs->sx - gXOffset;
		y = TBY + cs->cy * gFHeight;							// �J�[�\�����W�ݒ�
		SIMEcursor( 2, x, y );									// �J�[�\���̑傫���ƍ��W�ϐ��̎w��
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		SIMEDrawCursor(ret);									// SIME�J�[�\���̍��
		switch (ret){
		case SIME_KEY_CIRCLE:									// ��
			ScreenPaste(image);
			if (no==0){											// ������̒u�������s��
				pos = 0;
				while (strlen(source)>pos){						// ����������̌��܂ňړ�
					CMRight( cs, 0 );
					if (chkSJIS(source[pos++])) pos++;
				}
				pos = 0;
				while (strlen(source)>pos){						// ������������폜
					CMLeft( cs, 0 );
					DelStr( cs, 0, 1 );
					if (chkSJIS(source[pos++])) pos++;
				}
				AddStr( cs, dest, 0, 1 );						// �u�������������������
				gSave = 1;
			}
			if (StrSrchSub( cs,source )){						// ���̌�₪�����Ȃ�I��
				DrawRec( cs->cy, cs->rno, cs->pline );			// �Ō�̒u���������ɔ��f������
				exit = -1;
			} else {
				image = ScreenCopy();
				DrawStrChgNext(no);
			}
			break;
		case SIME_KEY_CROSS:									// �~
			ScreenPaste(image);
			exit = -1;
			break;
		case SIME_KEY_UP:										// ��
			no = 0;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawStrChgNext(no);
			break;
		case SIME_KEY_DOWN:										// ��
			no = 1;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawStrChgNext(no);
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// ��ʍX�V
	}

	return (0);
}

//==============================================================
// ���j���[�i���ݒ�j
//--------------------------------------------------------------
// ��ʕ\���Ɋ֘A���鍀�ڂ��ύX���ꂽ�ꍇ�́A���݂̕��͂���[�e���|�����t�@�C����
// �ۑ�������ă��[�h���s���čs���R�[�h�\���̂̍č\�z�����܂��B
//--------------------------------------------------------------

//----- �J�[�\�����ݒ���e��� -----

void SetEnvCsr(int sx,int sy,int pos,int *itemdata,int flag)
{
	const char	sel[7][5][13] = {
					{"���_�t�H���g",	"monafont12",	"monafont16",	"intraFont",	"intraFont P"},
					{"���Ȃ�",			"����"},
					{"Simple IME",		"OSK"},
					{"�S",				"�W"},
					{"���Ȃ�",			"����"},
					{"���Ȃ�",			"����"},
					{"���Ȃ�",			"����"},
				};
	static int	pos2;
	int			i,p;

	if (flag){													// �S�̂�������
		for (i=0; i<6 ;i++){
			Fill( sx+10+21*6, sy+6+12+6+i*13, 76, 12, gSCor[5] );
			mh_print( sx+10+21*6+2, sy+6+12+6+i*13, sel[i][itemdata[i]], gSCor[0] );
		}
		for (i=0; i<4 ;i++){
			Fill( sx+10+21*6, sy+6+12+6+6*13+i*13, 136, 12, gSCor[5] );
			p = strlen(gMonaFile[i]) -22;						// ��납��22������\��
			if (p<0) p = 0;
			mh_print( sx+10+21*6+2, sy+6+12+6+6*13+i*13, &gMonaFile[i][p], gSCor[0] );
		}
	} else {													// �O��̃J�[�\���ʒu���ĕ`��
		if (pos2<6){
			Fill( sx+10+21*6, sy+6+12+6+pos2*13, 76, 12, gSCor[5] );
			mh_print( sx+10+21*6+2, sy+6+12+6+pos2*13, sel[pos2][itemdata[pos2]], gSCor[0] );
		} else {
			Fill( sx+10+21*6, sy+6+12+6+pos2*13, 136, 12, gSCor[5] );
			p = strlen(gMonaFile[pos2-6]) -22;					// ��납��22������\��
			if (p<0) p = 0;
			mh_print( sx+10+21*6+2, sy+6+12+6+pos2*13, &gMonaFile[pos2-6][p], gSCor[0] );
		}
	}

	if (pos<6){
		CurveBox( sx+10+21*6, sy+6+12+6+pos*13, 76, 12, 0, gSCor[10], gSCor[11] );
		mh_print( sx+10+21*6+2, sy+6+12+6+pos*13, sel[pos][itemdata[pos]], gSCor[0] );
	} else {
		CurveBox( sx+10+21*6, sy+6+12+6+pos*13, 136, 12, 0, gSCor[10], gSCor[11] );
		p = strlen(gMonaFile[pos-6]) -22;						// ��납��22������\��
		if (p<0) p = 0;
		mh_print( sx+10+21*6+2, sy+6+12+6+pos*13, &gMonaFile[pos-6][p], gSCor[0] );
	}

	pos2 = pos;
}

//----- �t�H���g�t�@�C���̎w��ύX -----

int SetEnvFont(int font)
{
	const char	itemname[][21] = {
					"12�h�b�g���pmonafont",
					"12�h�b�g�S�pmonafont",
					"16�h�b�g���pmonafont",
					"16�h�b�g�S�pmonafont",
				};
	char	*p,msg[64],path[1024],filename[128];
	int		fd,ret;
	DrawImg	*image;

	image = ScreenCopy();										// ��ʃC���[�W��Ҕ�
	strcpy( msg,itemname[font] );
	strcat( msg,"�̃t�@�C�����w��" );
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
		} else {												// �w�肳�ꂽ�t�@�C�����J���Ȃ�
			DiaBox1( -1, 120, 0, "�w�肳�ꂽ�t�H���g�ɃA�N�Z�X�ł��܂���B", gSCor[0], gSCor[3], gSCor[4] );
			Unpush();											// �{�^�����������܂őҋ@
			WaitBtn();											// �����������܂őҋ@
		}
	}
	ScreenPaste(image);											// ��ʃC���[�W�𕜌�/�o�b�t�@�j��
	return (ret);
}

//----- ���C�� -----

int SetEnv(struct strCsrSts *cs)
{
	const char	itemname[][21] = {
					"�����t�H���g",
					"��ʉ��T�C�Y���g��",
					"IME�̎��",
					"�^�u��",
					"�S�p�󔒂��L���\\��",
					"�^�u���L���\\��",
					"12�h�b�g���pmonafont",
					"12�h�b�g�S�pmonafont",
					"16�h�b�g���pmonafont",
					"16�h�b�g�S�pmonafont",
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
	DialogFrame( sx, sy, 282, 178, msg, "����:�ݒ�ύX ��:���� �~:��ݾ�", gSCor[1], gSCor[2] );
	for (i=0; i<10 ;i++){
		mh_print( sx+10, sy+6+12+6+i*13, itemname[i], gSCor[0] );
	}

	itemdata[0] = gFont;
	itemdata[1] = gRoll;
	itemdata[2] = gIME;
	itemdata[3] = gTab;
	itemdata[4] = gPSpc;
	itemdata[5] = gPTab;
	tabBak = gTab;												// �^�u����ς������`�F�b�N���邽��
	fontBak = gFont;											// �t�H���g��ς������`�F�b�N���邽��
	rollBak = gRoll;
	fontChg = 0;												// �t�H���g�t�@�C����ύX�����H
	pos = 0;
	SetEnvCsr( sx, sy, pos, itemdata, 1);

	while (!gExitRequest){										// �L�[���̓��[�v
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		switch (ret){
		case SIME_KEY_CIRCLE:									// ��
			gFont = pf_fontType(itemdata[0]);					// �t�H���g�؂�ւ�
			setScreen();										// �p�����[�^�ύX
			if (cs->cy>=gSLine) cs->cy = gSLine - (gReDraw ? 2 : 1);
			SIMEfont(gFont,(gFont==2 ? 16 : 12));				// SIME�ϊ��s�t�H���g
			rollChg = 0;
			if (gRoll!=itemdata[1]){
				gRoll = itemdata[1];
				gXOffset = 0;
				rollChg = 1;
			}
			if (gIME!=itemdata[2] || gTab!=itemdata[3] || gPSpc!=itemdata[4] || gPTab!=itemdata[5]){
				gEnvSave = 1;									// ���ݒ肪�ω�����
			}
			gIME = itemdata[2];
			gTab = itemdata[3];
			gPSpc = itemdata[4];
			gPTab = itemdata[5];
			if (fontChg){										// �t�H���g�t�@�C���ύX
				gEnvSave = 1;									// ���ݒ肪�ω�����
				ret = 0;
				msg[0] = '\0';
				if ( (ret=pf_setMonaFont(0,gMonaFile[0],gMonaFile[1])) ){
					strcat( msg,"12�h�b�gmonafont" );
				}
				if (pf_setMonaFont(1,gMonaFile[2],gMonaFile[3])){
					if (ret){
						strcat( msg,"��" );
					}
					strcat( msg,"16�h�b�gmonafont" );
				}
				if (strlen(msg)){
					strcat( msg,"�̃��[�h�Ɏ��s���܂����B" );
					DiaBox1( -1, 110, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
					Unpush();									// �{�^�����������܂őҋ@
					WaitBtn();									// �����������܂őҋ@
				}
			}
			if (tabBak!=gTab || fontBak!=gFont || fontChg || rollChg){	// �s���R�[�h�\���ɕω�������ꍇ
				gEnvSave = 1;									// ���ݒ肪�ω�����
				DiaBox1( -1, 100, 0, "���͂��č\\�����Ă��܂��B\n���X���҂����������B", gSCor[0], gSCor[1], gSCor[2] );
				ScreenView();									// ��ʍX�V
				pline = 0;
				rno = 0;
				line = 0;
				while (pline<(cs->pline)+(cs->cy)){				// ��ʏ�\���J�n�_���s���擾
					if (gText[rno]->enter) line++;
					pline++;
					rno = gText[rno]->chainNext;
					if (rno==0xFFFFFFFF) break;
				}
				saveBak = gSave;
				msg[0] = '\0';
				image = ScreenCopy();
				ret = filesave(TEMPFILE);						// �ꎞ��Ɨp�t�@�C���ɕۑ�����
				ScreenPaste(image);
				if (ret==0){
					ret = fileload(TEMPFILE);					// �ă��[�h�i���̎��_�ŕ��͂̍č\�����s����j
					if (ret==0){								// ����I�������Ȃ�
						sceIoRemove(TEMPFILE);					// �ꎞ�t�@�C�����폜
					}
					switch (ret){
					case 2:
						strcpy( msg,"�������r���ŃL�����Z������܂����B\n���͂̌㔼�������Ă��܂��B" );
						break;
					case 1:
						strcpy( msg,"�������s���̂��ߏ��������f����܂����B\n���͂̌㔼�������Ă��܂��B" );
						break;
					case -1:
						strcpy( msg,"�������s���̂��ߏ����Ɏ��s���܂����B" );
						break;
					case -2:
						strcpy( msg,"�������[�X�e�B�b�N�̃A�N�Z�X�ŃG���[��������������\n�����Ɏ��s���܂����B" );
						break;
					}
					if (ret){
						strcat( msg,"\n�����O�̕��͂� " TEMPFILE " �Ɏc���Ă��邩������܂���B" );
					}
				} else {										// ��ƃt�@�C���ւ̕ۑ��Ɏ��s�����̂Őݒ�����ɖ߂�
					gTab = tabBak;
					gFont = fontBak;
					gRoll = rollBak;
					setScreen();								// �p�����[�^�ύX
					switch (ret){
					case -1:
						strcpy( msg,"�������s���̂��ߏ������L�����Z������܂����B" );
						break;
					case -2:
						strcpy( msg,"�������[�X�e�B�b�N�̃A�N�Z�X�ŃG���[��������������\n�������L�����Z������܂����B" );
						break;
					}
					strcat( msg,"\n�e�ݒ�����ɖ߂��܂����B" );
				}
				gSave = saveBak;
				if (strlen(msg)){								// ��蔭��
					DiaBox1( -1, 130, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
					Unpush();									// �{�^�����������܂őҋ@
					WaitBtn();									// �����������܂őҋ@
				}
				if (ret>=0){									// �G���[���������Ă��Ȃ��ꍇ
					no = 0;
					pline = 0;
					rno = 0;
					while (no!=line){
						if (gText[rno]->enter) no++;
						pline++;
						rno = gText[rno]->chainNext;
						if (rno==0xFFFFFFFF) break;
					}
					SetCursor( cs,pline );						// �w��s�ʒu�ɕ����J�[�\�����ړ�
					CMSet( &cs->cx, &cs->sx, &cs->cxr, (cs->cxb)+1, cs->rno, 1 );	// �J�[�\���ʒu���s���R�[�h�{���ɍ����悤�ɕ␳
				}
			}
			return (0);
			break;
		case SIME_KEY_CROSS:									// �~
			return (-1);
			break;
		case SIME_KEY_LEFT:										// ��
			if (pos<6){
				if (itemdata[pos]>0){
					itemdata[pos]--;
					SetEnvCsr( sx, sy, pos, itemdata, 0);
				}
			} else {
				if (SetEnvFont(pos-6)){							// �t�H���g�w��
					fontChg = 1;
					SetEnvCsr( sx, sy, pos, itemdata, 0);
				}
			}
			break;
		case SIME_KEY_RIGHT:									// ��
			if (pos<6){
				if ((pos==0 && itemdata[pos]<4) || itemdata[pos]<1){
					itemdata[pos]++;
					SetEnvCsr( sx, sy, pos, itemdata, 0);
				}
			} else {
				if (SetEnvFont(pos-6)){							// �t�H���g�w��
					fontChg = 1;
					SetEnvCsr( sx, sy, pos, itemdata, 0);
				}
			}
			break;
		case SIME_KEY_UP:										// ��
			if (pos>0){
				pos--;
				SetEnvCsr( sx, sy, pos, itemdata, 0);
			}
			break;
		case SIME_KEY_DOWN:										// ��
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
// ���j���[�i��蒼���j
//--------------------------------------------------------------
// �������x�͊��S�ɖ������������ɂȂ��Ă��܂��B
//--------------------------------------------------------------

//----- �_�C�A���O��� -----

void DrawUndo(int no,int flag1,int flag2)
{
	int		x,y;

	x = 480-4-130;
	y = 272-4-69;
	DialogFrame( x, y, 10+110+10, 6+12+4+13*2-1+4+12+6, "��蒼��", "��:�I�� �~:��ݾ�", gSCor[1], gSCor[2] );
	if (no){
		Fill( x+10, y+6+12+4, 110, 12, gSCor[5] );
		CurveBox( x+10, y+6+12+4+13, 110, 12, 0, gSCor[10], gSCor[11] );
	} else {
		CurveBox( x+10, y+6+12+4, 110, 12, 0, gSCor[10], gSCor[11] );
		Fill( x+10, y+6+12+4+13, 110, 12, gSCor[5] );
	}
	mh_print( x+10+1, y+6+12+4   , "��蒼��", (flag1 ? gSCor[0] : gSCor[12]) );
	mh_print( x+10+1, y+6+12+4+13, "��蒼���̂�蒼��", (flag2 ? gSCor[0] : gSCor[12]) );
}

//----- ��蒼�� -----

void UndoUndo(struct strCsrSts *cs,int *size,int *pos)
{
	if (*size==0) return;
	(*pos)--;
	if (*pos<0) *pos = UNDOSIZE -1;
	(*size)--;

	SetCursor( cs,gUndo[*pos].pline );							// �w��s�ʒu�ɕ����J�[�\�����ړ�
	cs->cxr = gUndo[*pos].cxr;
	CRSet(cs);													// �w�肳�ꂽ�_�������ʒu�ɃJ�[�\�����ړ�
	if (gUndo[*pos].str[2]==1){									// ���������͂���Ă����ꍇ
		DelStr( cs, 0, 0 );
	} else {													// �������폜����Ă����ꍇ
		AddStr( cs, gUndo[*pos].str, 0, 0 );
	}
	ScreenAdj(cs);
	cs->adjust = 0;												// ��ʈʒu���J�[�\���ɍ��킹��i0:���̂܂� 1:���킹��j
	DrawSts( cs, 1 );											// ���[���[�n�ƃX�N���[���o�[�X�V
	DrawFullRec(cs->pline);										// ��ʑS�����������
}

//----- ��蒼���̂�蒼�� -----

void UndoRedo(struct strCsrSts *cs,int *size,int *pos)
{
	char	str[3] = {0,0,0};

	if (*size==gUndoSize) return;

	SetCursor( cs,gUndo[*pos].pline );							// �w��s�ʒu�ɕ����J�[�\�����ړ�
	cs->cxr = gUndo[*pos].cxr;
	CRSet(cs);													// �w�肳�ꂽ�_�������ʒu�ɃJ�[�\�����ړ�
	if (gUndo[*pos].str[2]==1){									// ���������͂���Ă����ꍇ
		str[0] = gUndo[*pos].str[0];
		str[1] = gUndo[*pos].str[1];
		AddStr( cs, str, 0, 0 );
	} else {													// �������폜����Ă����ꍇ
		DelStr( cs, 0, 0 );
	}
	ScreenAdj(cs);
	cs->adjust = 0;												// ��ʈʒu���J�[�\���ɍ��킹��i0:���̂܂� 1:���킹��j
	DrawSts( cs, 1 );											// ���[���[�n�ƃX�N���[���o�[�X�V
	DrawFullRec(cs->pline);										// ��ʑS�����������

	(*pos)++;
	if (*pos>=UNDOSIZE) *pos = 0;
	(*size)++;
}

//----- ���C�� -----

int Undo(struct strCsrSts *cs)
{
	SceCtrlData		pad;
	int		x,y,no,ret,exit,size,pos;
	DrawImg	*image;

	if (gUndoSize==0) return(-1);								// ���s�s��
	size = gUndoSize;
	pos = gUndoPos;
	UndoUndo( cs,&size,&pos );
	no = 0;
	image = ScreenCopy();
	DrawUndo( no, (size!=0), (size!=gUndoSize) );

	exit = 0;
	while (!gExitRequest && !exit){								// �L�[���̓��[�v
		x = cs->sx - gXOffset;
		y = TBY + cs->cy * gFHeight;							// �J�[�\�����W�ݒ�
		SIMEcursor( 2, x, y );									// �J�[�\���̑傫���ƍ��W�ϐ��̎w��
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		SIMEDrawCursor(ret);									// SIME�J�[�\���̍��
		switch (ret){
		case SIME_KEY_CIRCLE:									// ��
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
		case SIME_KEY_CROSS:									// �~
			ScreenPaste(image);
			gUndoPos = pos;
			gUndoSize = size;
			exit = -1;
			break;
		case SIME_KEY_UP:										// ��
			no = 0;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawUndo( no, (size!=0), (size!=gUndoSize) );
			break;
		case SIME_KEY_DOWN:										// ��
			no = 1;
			ScreenPaste(image);
			image = ScreenCopy();
			DrawUndo( no, (size!=0), (size!=gUndoSize) );
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// ��ʍX�V
	}
	return (0);
}

//==============================================================
// ���j���[�i���ڂ̑I�����j
//--------------------------------------------------------------
// �߂�l  3:���j���[�I���i��ʂ̕����͍s��Ȃ��j
//         2:���j���[�I�����ʍX�V
//         1:���j���[�I�����ʏ�����
//         0:���j���[�I���i��ʂ͂��̂܂܁j
//        -1:���j���[�I����A�v���I��
//        -2:�L�����Z���i�Ăу��j���[�ցj
//--------------------------------------------------------------
// ���j���[�̍��ڂ�I���������Ɏ��s������̓I�ȓ��e�B
//--------------------------------------------------------------

int MenuSelect(char *filepath,int tab,int pos,struct strCsrSts *cs)
{
	char	*p,path[1024];
	int		ret;

	//----- �u�t�@�C���v -----

	switch (tab){
	case 0:
		switch (pos){
		case 0:													// �V�K�쐬
			if (gSave){											// �X�V����Ă���ꍇ�͔j���m�F
				if (!DialogYN( "�ҏW���̕��͔͂j������܂��B", gSCor[3], gSCor[4] ))
					return (-2);
			}
			InitRec();
			p = strrchr( filepath,'/' );
			strcpy( &p[1],"NEW.TXT" );
			return (1);
			break;
		case 1:													// �J��
			if (gSave){											// �X�V����Ă���ꍇ�͔j���m�F
				if (!DialogYN( "�ҏW���̕��͔͂j������܂��B", gSCor[3], gSCor[4] ))
					return (-2);
			}
			p = strrchr( filepath,'/' );
			if (!SelectFile("�ҏW���镶�͂�I��ł�������",gFolder,&p[1],filepath,0,gIME,gHisFile)){
				DiaBox1( -1, 100, 0, "�t�@�C����ǂݍ���ł��܂��B\n���X���҂����������B", gSCor[0], gSCor[1], gSCor[2] );
				MenuLoad(filepath);
				return (1);
			}
			return (-2);										// �u�J���v���L�����Z��
			break;
		case 2:													// �㏑���ۑ�
			if (DialogYN( "�e�L�X�g��ۑ����܂����H", gSCor[1], gSCor[2] )){
				MenuSave(filepath);
				return (0);
			}
			return (-2);										// �L�����Z��
			break;
		case 3:													// ���O��t���ĕۑ�
			strcpy( path,filepath );
			p = strrchr( path,'/' );
			if (!SelectFile("���O��t���ĕۑ�",gFolder,&p[1],path,-1,gIME,gHisFile)){
				if (!MenuSave(path)){							// �ۑ������ꍇ�̓t�@�C���p�X�֘A���X�V
					strcpy( filepath,path );
				}
				return (1);										// �ҏW�Ώۂ��ύX�ɂȂ�̂ŉ�ʏ�����
			}
			return (-2);
			break;
		case 4:													// �ۑ����ďI��
			if (MenuSave(filepath)){
				return (-2);									// �G���[/�L�����Z�����ƏI�����Ȃ�
			} else {
				gExitRequest = -1;
				return (-1);									// �ۑ������̂ŏI��
			}
			break;
		case 5:													// �I��
//			if (gSave){											// �X�V����Ă���ꍇ�͔j���m�F
//				if (!DialogYN( "�ҏW���̕��͔͂j������܂��B", gSCor[3], gSCor[4] ))
//					return (-2);
//			}
			gExitRequest = -1;
			return (-1);
			break;
		case 6:													// ���ݒ�
			if (SetEnv(cs)){
				return (-2);									// �L�����Z��
			} else {
				return (2);
			}
			break;
		}
		break;

	//----- �u�ҏW�v -----

	case 1:
		switch (pos){
		case 0:													// ��蒼��
			gTSel[0].rno = -1;									// �͈͑I���̉���
			if (!Undo(cs)){
				return (3);
			}
			break;
		case 1:													// �\��t��
			if (gTSel[0].rno!=-1){
				TextDel(cs);
				gTSel[0].rno = -1;								// �͈͑I���̉���
			}
			if (gTextClip==NULL) return (-2);
			TextPaste(cs);
			cs->adjust = 1;										// ��ʈʒu���J�[�\���ʒu�֍��킹��
			return (2);
			break;
		case 2:													// �R�s�[
			if (gTSel[0].rno==-1) return (-2);
			TextCopy();
			gTSel[0].rno = -1;									// �͈͑I���̉���
			return (2);
			break;
		case 3:													// �؂���
			if (gTSel[0].rno==-1) return (-2);
			TextCopy();
			TextDel(cs);
			gTSel[0].rno = -1;									// �͈͑I���̉���
			cs->adjust = 1;										// ��ʈʒu���J�[�\���ʒu�֍��킹��
			return (2);
			break;
		case 4:													// �폜
			if (gTSel[0].rno==-1) return (-2);
			TextDel(cs);
			gTSel[0].rno = -1;									// �͈͑I���̉���
			cs->adjust = 1;										// ��ʈʒu���J�[�\���ʒu�֍��킹��
			return (2);
			break;
		case 5:													// �t�@�C������\��t��
			if (gTSel[0].rno!=-1){
				TextDel(cs);
				cs->adjust = 1;									// ��ʈʒu���J�[�\���ʒu�֍��킹��
				gTSel[0].rno = -1;
			}
			if (FilePaste(cs)){
				return (-2);									// �L�����Z��
			} else {
				cs->adjust = 1;									// ��ʈʒu���J�[�\���ʒu�֍��킹��
				return (2);
			}
			break;
		case 6:													// �t�@�C���֐؂�o��
			if (gTSel[0].rno==-1) return (-2);
			if (TextSave()){
				return (-2);									// �L�����Z��
			} else {
				gTSel[0].rno = -1;
				return (2);
			}
			break;
		}
		break;

	//----- ���� -----

	case 2:
		switch (pos){
		case 0:													// �����񌟍�
			ret = StrSrch(cs);
			if (ret<0){
				return (-2);
			} else if (ret==0){
				return (3);
			} else {
				return (2);
			}
			break;
		case 1:													// ������u��
			ret = StrChg(cs);
			if (ret<0){
				return (-2);
			} else if (ret==0){
				return (3);
			} else {
				return (2);
			}
			break;
		case 2:													// �w��s�փW�����v
			if (TextJump(cs)){
				return (-2);
			} else {
				gTSel[1].rno = cs->rno;							// �e�L�X�g�͈͎w��p
				gTSel[1].cxr = cs->cxr;
				gTSel[1].line = cs->pline + cs->cy;
				return (2);
			}
			break;
		case 3:													// �t�@�C���̐擪
			SetCursor( cs,0 );									// �w��s�ʒu�ɕ����J�[�\�����ړ�
			cs->cx = 0;											// ��ʏ�ł̕����J�[�\��X�ʒu�i�����P�ʁj
			cs->sx = 0;											// ��ʏ�ł̕����J�[�\��X�ʒu�i�h�b�g�P�ʁj
			cs->cxr = 0;										// �_���J�[�\��X�ʒu
			cs->cxb = 0;										// �J�[�\��X�ʒu�w�莞�̓��B�ڕW�|�C���g�i�h�b�g�P�ʁj
			cs->adjust = 1;										// ��ʈʒu���J�[�\���ɍ��킹��i0:���̂܂� 1:���킹��j
			gTSel[1].rno = cs->rno;								// �e�L�X�g�͈͎w��p
			gTSel[1].cxr = cs->cxr;
			gTSel[1].line = cs->pline + cs->cy;
			return (2);
			break;
		case 4:													// �t�@�C���̍Ō�
			SetCursor( cs,gLineMax );							// �w��s�ʒu�ɕ����J�[�\�����ړ�
			cs->cxr = gText[cs->rno]->len;
			CRSet(cs);											// �w�肳�ꂽ�_�������ʒu�ɃJ�[�\�����ړ�
			gTSel[1].rno = cs->rno;								// �e�L�X�g�͈͎w��p
			gTSel[1].cxr = cs->cxr;
			gTSel[1].line = cs->pline + cs->cy;
			return (2);
			break;
		case 5:													// �Ō�ɕҏW������
			if (gUndoSize!=0){
				pos = gUndoPos -1;
				if (pos<0) pos = UNDOSIZE -1;
				SetCursor( cs,gUndo[pos].pline );				// �w��s�ʒu�ɕ����J�[�\�����ړ�
				cs->cxr = gUndo[pos].cxr;
				CRSet(cs);										// �w�肳�ꂽ�_�������ʒu�ɃJ�[�\�����ړ�
				gTSel[1].rno = cs->rno;							// �e�L�X�g�͈͎w��p
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
// ���j���[
//--------------------------------------------------------------
// ���j���[�̍��ƃL�[���͏����B
//--------------------------------------------------------------

//----- ���ڍ�� -----

void DrawMenuSub(int sx,int sy,int tab,int pos,int sel)
{
	const char	item[3][7][17] = {{
					"�V�K�쐬",
					"�J��",
					"�㏑���ۑ�",
					"���O��t���ĕۑ�",
					"�ۑ����ďI��",
					"�I��",
					"���ݒ�",
				},{
					"��蒼��",
					"�\\��t��",
					"�R�s�[",
					"�؂���",
					"�폜",
					"̧�ق���\\��t��",
					"̧�ق֐؂�o��",
				},{
					"�����񌟍�",
					"������u��",
					"�w��s�փW�����v",
					"�t�@�C���̐擪",
					"�t�@�C���̍Ō�",
					"�Ō�ɕҏW������",
					"",
				}};
	long		cor;

	cor = gSCor[0];
	if (tab==1){												// �O���[�A�E�g
		if (!(sel & 0x01) && pos==1) cor = gSCor[12];
		if (!(sel & 0x02) && ((pos>1 && pos<5) || pos == 6)) cor = gSCor[12];
		if (!(sel & 0x04) && pos==0) cor = gSCor[12];
	} else if (tab==2){
		if (!(sel & 0x04) && pos==5) cor = gSCor[12];
	}
	mh_print( sx+10+1+tab*100, sy+4+12+4+12+4+pos*13, item[tab][pos], cor );
}

//----- �^�u��� -----

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
	mh_print( sx+10+1+1,     sy+4+12+4, "�t�@�C��", gSCor[1] );
	mh_print( sx+10+1+100+1, sy+4+12+4, "�ҏW", gSCor[1] );
	mh_print( sx+10+1+200+1, sy+4+12+4, "����", gSCor[1] );
}

//----- �J�[�\����� -----

void DrawMenu(int sx,int sy,int tab,int pos,int flag,int sel)
{
	static int	pos2,tab2;
	int			i,j;

	if (flag){													// ���j���[���ڂ�\�����đO��̃J�[�\��������
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

//----- ���C�� -----

int menu(char *filepath,int tab,int sel,struct strCsrSts *cs)
{
	SceCtrlData		pad;
	int				sx,sy,pos,ret;
	DrawImg			*image;

	sx = 240-316/2;
	sy = 70;
	DialogFrame( sx, sy, 318, 4+12+4+12+4+7*13+8, "���j���[", "", gSCor[1], gSCor[2] );
	DrawMenuTab( sx, sy, tab );

	pos = 0;
	if (tab==0 && gSave) pos = 2;								// �X�V����Ă���Ȃ�u�㏑���ۑ��v
	if (tab==1){
		if (sel&0x02){
			pos = 2;											// �͈͎w�蒆�Ȃ�u�R�s�[�v
		} else {
			pos = 1;											// �e�L�X�g�N���b�v�ɉ�������Ȃ�u�\��t���v
		}
	}
	DrawMenu( sx, sy, tab, pos, -1, sel );						// ���j���[���ڂ̏������
	Unpush();													// �{�^�����������܂őҋ@

	while (!gExitRequest){										// �L�[���̓��[�v
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		switch (ret){
		case SIME_KEY_CIRCLE:									// ��
			image = ScreenCopy();								// ��ʃC���[�W�����j���[���܂߂ĕۑ�
			ret = MenuSelect( filepath,tab,pos,cs );
			if (ret!=-2){
				DrawImgFree(image);								// ��ʃC���[�W��p��
				if (ret!=1){									// ��ʏ��������͎��s���Ȃ�
					DrawSts( cs, 1 );							// ���[���[�n�ƃX�N���[���o�[�X�V
				}
				return (ret);
			} else {
				ScreenPaste(image);								// ��ʃC���[�W�𕜌�
				DrawSts( cs, 1 );								// ���[���[�n�ƃX�N���[���o�[�X�V
			}
			break;
		case SIME_KEY_START:									// [START]
		case SIME_KEY_CROSS:									// �~
			return (0);
			break;
		case SIME_KEY_TRIANGLE:									// ��
			break;
		case SIME_KEY_LEFT:										// ��
			tab--;
			if (tab<0) tab = 2;
			DrawMenuTab( sx, sy, tab );
			DrawMenu( sx, sy, tab, pos, 0, sel );
			break;
		case SIME_KEY_RIGHT:									// ��
			tab++;
			if (tab>2) tab = 0;
			DrawMenuTab( sx, sy, tab );
			DrawMenu( sx, sy, tab, pos, 0, sel );
			break;
		case SIME_KEY_UP:										// ��
			pos--;
			if (pos<0) pos = 6;
			DrawMenu( sx, sy, tab, pos, 0, sel );
			break;
		case SIME_KEY_DOWN:										// ��
			pos++;
			if (pos>6) pos = 0;
			DrawMenu( sx, sy, tab, pos, 0, sel );
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// ��ʍX�V
	}
	return (-1);
}

//==============================================================
// main
//--------------------------------------------------------------

//----- �E�X�N���[�� -----

void RightShift(struct strCsrSts *cs,int vx)
{
	SIMEDrawCursor(-1);											// SIME�̃J�[�\���ƃE�B���h�E���̏���
	RollRight(vx);
	gXShift = 0;
	gXDraw = vx;												// ��ʍ����̕������������ݒ�
	DrawFullRec2( ShiftRNo(cs->rno,-cs->cy),cs->pline );
	gXShift = 0;
	gXDraw = LWIDTH;
	DrawSts( cs, 1 );											// ���[���[�n�ƃX�N���[���o�[
}

//----- ���X�N���[�� -----

void LeftShift(struct strCsrSts *cs,int vx)
{
	SIMEDrawCursor(-1);											// SIME�̃J�[�\���ƃE�B���h�E���̏���
	RollLeft(vx);
	gXShift = LWIDTH - vx - gFWidth;
	gXDraw = LWIDTH;											// ��ʉE���̕������������ݒ�
	DrawFullRec2( ShiftRNo(cs->rno,-cs->cy),cs->pline );
	gXShift = 0;
	gXDraw = LWIDTH;
	DrawSts( cs, 1 );											// ���[���[�n�ƃX�N���[���o�[
}

//----- ���X�N���[�� -----

void DownShift(struct strCsrSts *cs,int dcFlag)
{
	if (cs->pline!=0){
		cs->pline--;
		cs->rno = ShiftRNo( cs->rno,-1 );
		if (gText[cs->rno]->enter) (cs->rline)--;				// �J�[�\���ʒu�̘_���s��
		CMSet( &cs->cx, &cs->sx, &cs->cxr, cs->cxb+1, cs->rno, 1 );	// �J�[�\���ʒu�̕␳
		gTSel[1].rno = cs->rno;
		gTSel[1].cxr = cs->cxr;
		gTSel[1].line = cs->pline + cs->cy;
		if (!dcFlag) SIMEDrawCursor(-1);						// �J�[�\���̏���
		RollDown(0);
		if (gTSel[0].rno!=-1){
			DrawRec( cs->cy, cs->rno, cs->pline );
			if (cs->cy<gSLine-1){
				DrawRec( cs->cy+1, gText[cs->rno]->chainNext, cs->pline );
			}
		}
		DrawRec( 0, ShiftRNo(cs->rno,-cs->cy), cs->pline );
		DrawSts( cs, 1 );										// ���[���[�n�ƃX�N���[���o�[
	}
}

//----- ��X�N���[�� -----

void UpShift(struct strCsrSts *cs,int dcFlag)
{
	if (cs->pline+gSLine-1-gReDraw<gLineMax){
		cs->pline++;
		if (gText[cs->rno]->enter) (cs->rline)++;				// �J�[�\���ʒu�̘_���s��
		cs->rno = ShiftRNo( cs->rno,1 );
		CMSet( &cs->cx, &cs->sx, &cs->cxr, cs->cxb+1, cs->rno, 1 );	// �J�[�\���ʒu�̕␳
		gTSel[1].rno = cs->rno;
		gTSel[1].cxr = cs->cxr;
		gTSel[1].line = cs->pline + cs->cy;
		if (!dcFlag) SIMEDrawCursor(-1);						// �J�[�\���̏���
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
		DrawSts( cs, 1 );										// ���[���[�n�ƃX�N���[���o�[
	}
}

//----- ���[�N������ -----
// ����̓A�v���N�����Ɏ��s���܂�

void SMEMOinit(void)
{
	char	msg[128];
	int		x,y,ret;

	initGraphics();
	flipScreen2();												// �f�t�H���g�ł̓y�[�W�ݒ�ɕs�s��������̂�
	sceGuStart( GU_DIRECT, gList );
	sceGuClearColor(0x00000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	if (getcwd( gFolder,1024 )==NULL){							// �J�����g�f�B���N�g���̎擾
		strcpy( gFolder,"ms0:");
	}
	strcat( gFolder,"/" );
	strcpy( gPFolder,gFolder );									// �t�@�C������\��t�����̑Ώۃt�H���_

	//----- �z�F���� -----

	if (kuKernelGetModel()) {									// PSP-1000�ȊO�Ȃ�
		gSCor[1] = 0x70F070;									// �ʒm�_�C�A���O�i�g�j
		gSCor[2] = 0x183018;									// �ʒm�_�C�A���O�i�����j
		gSCor[4] = 0x202060;									// �x���_�C�A���O�i�����j
		gSCor[5] = 0x002000;									// �ʒm�_�C�A���O�i�w�i�j
	}

	//----- intraFont������ -----

	x = 0;
	y = 0;

/*	DiaBox1( x, y, 0, "intraFont���[�h��", gSCor[0], gSCor[1], gSCor[2] );
	ScreenView();												// ��ʍX�V
	if (pf_setIntraFont()){
		DiaBox1( x+20, y+10, 0, "intraFont 'japanese font' �̃��[�h�Ɏ��s���܂����B", gSCor[0], gSCor[3], gSCor[4] );
		WaitBtn();												// �����������܂őҋ@
	}
	x += 15;
	y += 20;
*/
	//----- monafont�ǂݍ��� -----

	strcpy( gMonaFile[0],gFolder );
	strcat( gMonaFile[0],"FONT/monafontA.bin" );
	strcpy( gMonaFile[1],gFolder );
	strcat( gMonaFile[1],"FONT/monafontJ.bin" );
	strcpy( gMonaFile[2],gFolder );
	strcat( gMonaFile[2],"FONT/monafont16A.bin" );
	strcpy( gMonaFile[3],gFolder );
	strcat( gMonaFile[3],"FONT/monafont16W.bin" );

	LoadEnv();													// �ݒ�t�@�C���ǂݍ���

	DiaBox1( x, y, 0, "monafont���[�h��", gSCor[0], gSCor[1], gSCor[2] );
	ScreenView();												// ��ʍX�V
	msg[0] = '\0';
	ret = 0;
	if ( (ret=pf_setMonaFont(0,gMonaFile[0],gMonaFile[1])) ){
		strcat( msg,"12�h�b�gmonafont" );
	}
	if (pf_setMonaFont(1,gMonaFile[2],gMonaFile[3])){
		if (ret){
			strcat( msg,"��" );
		}
		strcat( msg,"16�h�b�gmonafont" );
	}
	if (strlen(msg)){
		strcat( msg,"�̃��[�h�Ɏ��s���܂����B" );
		DiaBox1( x+20, y+10, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
		Unpush();												// �{�^�����������܂őҋ@
		WaitBtn();												// �����������܂őҋ@
	}
	x += 15;
	y += 20;

	//----- SimpleIME������ -----

	DiaBox1( x, y, 0, "�����t�@�C�����[�h��", gSCor[0], gSCor[1], gSCor[2] );
	ScreenView();												// ��ʍX�V
	ret = InitSIME(0);											// �����ǂݍ��݂Ɗe�평�����i�w�i�̎��ȕ������[�h�j
	SIMESetDraw(0);												// DRAW�y�[�W�֏�������
	switch(ret){
	case -1:
		strcpy( msg,"�s���Ȏ����t�@�C���ł��B" );
		break;
	case -2:
		strcpy( msg,"������������Ȃ��Ď������ǂ߂܂���ł����B" );
		break;
	case -3:
		strcpy( msg, "�����iSIME.DIC�j���J���܂���ł����B" );
		break;
	}
	if (ret<0){													// �G���[���������Ă�����
		strcat( msg,"\n�����ϊ��@�\\���g�p�o���܂���B" );
		DiaBox1( x+20, y+10, 0, msg, gSCor[0], gSCor[3], gSCor[4] );
		Unpush();												// �{�^�����������܂őҋ@
		WaitBtn();												// �����������܂őҋ@
	}

	Fill(0, 0, 480, 272*2, 0);									// ��ʏ���
}

//----- ���[�N�J�� -----
// ����̓A�v���I�����Ɏ��s���܂�

void SMEMOend(void)
{
	EndSIME();													// Simple IME�֘A���[�N�J��
	pf_endMonaFont();											// MonaFont�J��
}

//----- ���C�� -----

//--- �w��e�L�X�g��ҏW

int	SMEMOtext(char *message,int maxsize,int maxline)
{
	return SMEMOmain( 0, message, maxsize, maxline );
}

//--- �w��t�@�C���̓��e��ҏW
// path �ҏW���s���t�@�C�����i�t���p�X�j
//      NULL���w�肳�ꂽ�ꍇ�̓t�@�C���I���_�C�A���O��\��

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
	flipScreen2();												// �f�t�H���g�ł̓y�[�W�ݒ�ɕs�s��������̂�
	sceGuStart( GU_DIRECT, gList );
	sceGuClearColor(0x00000000);
	sceGuClearDepth(0);
	sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);
	sceGuFinish();
	sceGuSync(0,0);

	gLineListMax = 0;

//	SMEMOinit();

	//----- ���͗����̏����� -----

	gHisLJump = InputHisLoad( HISFILEJUMP );
	gHisSarh = InputHisLoad( HISFILESARH );
	gHisChg = InputHisLoad( HISFILECHG );
	gHisFPaste = InputHisLoad( HISFILEPASTE );
	gHisFont = InputHisLoad( HISFILEFONT );
	gHisFile = InputHisLoad( HISFILEFILE );

	//----- �N���b�v�{�[�h�t�@�C���ǂݍ��� -----

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
		for (pos1=0; pos1<filesize ;pos1++){					// �e�L�X�g�ȊO�̃R�[�h���폜
			c = gTextClip[pos1];
			if ((c>=32 && c!=0x7F) || c=='\n' || c=='\t') gTextClip[pos2++] = c;
		}
		gTextClip[pos2] = '\0';
	}
	gTCChg = 0;

	//----- �t�@�C���I�� -----

	gFont = pf_fontType(gFont);									// �f�t�H���g�t�H���g���w��
	setScreen();												// �p�����[�^�ύX
	SIMEfont(gFont,(gFont==2 ? 16 : 12));						// SIME�ϊ��s�t�H���g

	InitRec();													// �s���R�[�h���X�g��������

	if (mode==1){												// �w��t�@�C���̕ҏW
		if (text==NULL){
			if (SelectFile("�ҏW���镶�͂�I��ł�������",gFolder,"NEW.TXT",textFile,0,gIME,gHisFile)){
				strcpy( textFile,gFolder );
				strcat( textFile,"NEW.TXT" );					// �t�@�C�����I������Ȃ��������̃t�@�C����
			}
		} else {
			strcpy( textFile,text );
		}
		DiaBox1( -1, 100, 0, "�t�@�C����ǂݍ���ł��܂��B\n���X���҂����������B", gSCor[0], gSCor[1], gSCor[2] );
		MenuLoad(textFile);
	} else {
		strcpy( textFile,gFolder );
		strcat( textFile,"NEW.TXT" );
	}

	Fill(0, 0, 480, 272, 0);									// ��ʏ���

	//----- ���C�� -----

	cs.cx = 0;
	cs.sx = 0;
	cs.cy = 0;													// �J�[�\���ʒu
	cs.cxb = 0;													// �J�[�\���ʒu�ۑ��p
	cs.cxr = 0;													// �J�[�\���̘_���ʒu
	cs.pline = 0;												// ��ʍŏ㕔�̕����s��
	cs.rline = 0;												// �J�[�\���ʒu�̘_���s��
	cs.adjust = 0;												// ��ʈʒu�C��
	gXOffset = 0;												// ���E�X�N���[���p
	gXShift = 0;
	gXDraw = LWIDTH;
	vy = 0;														// �c�X�N���[�����̒[���Ǘ�
	save = 0;
	tab = 0;													// ���j���[���
	cppst = 0;													// ���j���[�́u�ҏW�v�O���[�A�E�g�w��
	gTSel[0].rno = -1;											// �͈͑I��1
	gTSel[1].rno = -1;											// �͈͑I��2
	cs.rno = GetRNo(cs.pline+cs.cy);							// �J�[�\���ʒu�ɑΉ����郌�R�[�h��
	if (mode==0){
		AddStr( &cs, text, 0, 1 );								// ������ owata������̕������W�J ������
	}
	gUndoSize = 0;												// ��蒼���o�b�t�@�T�C�Y
	gUndoPos = 0;												// ��蒼���o�b�t�@�ʒu
	DrawTitle(textFile);
	DrawSts( &cs, 1 );											// ���[���[�n�ƃX�N���[���o�[
	DrawFullRec(cs.pline);										// ��ʑS�����������
	Unpush();													// �{�^�����S�ė������܂őҋ@

	x = 1;
	y = TBY;
	pad2.Buttons = 0;
	while(!gExitRequest){
		ret = 0;												// -1:�V�����s���R�[�h���擾�ł��Ȃ�����
		sceCtrlReadBufferPositive(&pad, 1);
		if (cs.adjust && gRoll){								// �J�[�\�����\�������ʒu�ɉ�ʂ��ړ�������
			if (cs.sx<gXOffset+20){
				if (gXOffset!=0){
					xoffBuf = gXOffset;
					gXOffset = cs.sx - 60;
					if (gXOffset<0) gXOffset = 0;
					if (xoffBuf-gXOffset<400){
						RightShift( &cs,xoffBuf-gXOffset );
					} else {
						DrawFullRec(cs.pline);					// ��ʑS�����������
						DrawSts( &cs, 1 );						// ���[���[�n�ƃX�N���[���o�[
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
						DrawFullRec(cs.pline);					// ��ʑS�����������
						DrawSts( &cs, 1 );						// ���[���[�n�ƃX�N���[���o�[
					}
				}
			}
		}
		cs.adjust = 0;
		x = cs.sx - gXOffset;
		y = TBY + cs.cy*gFHeight;								// �J�[�\�����W�ݒ�
		if (x<0 || x>=LWIDTH){									// �J�[�\������ʊO�Ȃ�J�[�\���͕\�����Ȃ�
			x = (x<0 ? 0 : LWIDTH-2);							// �J�[�\������ʊO�̎���SIME�ϊ��g�\���ʒu
			SIMEcursor( 0, x, y);
		} else {
			SIMEcursor( 2, x, y);								// �J�[�\���̑傫���ƍ��W�ϐ��̎w��
		}
		SIMEselInput( gIME, str,pad );							// ���������
		if (str[0]!=0){											// ���͂�������
			if ((str[0]<32U && str[0]!='\t') || str[0]==0x7F){	// �R���g���[���R�[�h�������ꍇ
				switch(str[0]){
				case SIME_KEY_START:							// [START]���j���[
					tab = 0;
					cppst = 0;
					if (gTextClip!=NULL){						// �R�s�[�������͂�����Ȃ�
						tab = 1;
					}
					if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr)
						gTSel[0].rno = -1;
					if (gTSel[0].rno!=-1){						// �͈͑I����
						tab = 1;
						cppst |= 0x02;
					}
					if (gTextClip!=NULL) cppst |= 0x01;
					if (gUndoSize!=0) cppst |= 0x04;
					mret = menu(textFile,tab,cppst,&cs);
					if (mret==1){								// ��ʂ�������
						cs.cx = cs.cy = cs.cxb = cs.cxr = cs.pline = cs.rline = 0;
						cs.sx = 0;
						cs.adjust = 0;
						vy = 0;									// �c�X�N���[�����̒[���Ǘ�
						cs.rno = GetRNo(cs.pline+cs.cy);
						DrawTitle(textFile);
						DrawSts( &cs, 1 );
						DrawFullRec(cs.pline);					// ��ʑS�����������
					} else if (mret!=3){
						DrawFullRec(cs.pline);					// ��ʑS�����������
					}
					Unpush();									// �{�^�����������܂őҋ@
					break;
				case SIME_KEY_SELECT:							// �t���[�Y�I���^�֖߂�
					gExitRequest = -1;							// �I��
					Unpush();									// �{�^�����������܂őҋ@
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
						DownShift( &cs,0 );						// ���X�N���[��
						ScreenView();							// ��ʍX�V
					}
					break;
				case SIME_KEY_RTRIGGER:							// [R]
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					for (i=0; i<PAGEMOVE ;i++){
						UpShift( &cs,0 );						// ��X�N���[��
						ScreenView();							// ��ʍX�V
					}
					break;
				case SIME_KEY_CIRCLE:							// ��
					if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr)
						gTSel[0].rno = -1;
					if (gTSel[0].rno!=-1){
						TextDel(&cs);
						gTSel[0].rno = -1;
						DrawFullRec(cs.pline);					// ��ʑS�����������
					}
					ret = AddRet( &cs,1,1 );
					if (!ret) gSave = 1;
					cs.adjust = 1;
					break;
				case SIME_KEY_UP:								// ��
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					CMUp( &cs, 1 );
					cs.adjust = 1;
					break;
				case SIME_KEY_DOWN:								// ��
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
						DrawFullRec(cs.pline);					// ��ʑS�����������
						cs.adjust = 1;
						break;
					}
					gSave = 1;
					if (str[0]==0x7F){							// [DEL]
						DelStr( &cs,1,1 );						// �J�[�\���ʒu�̕������폜
						cs.adjust = 1;
						break;
					}
				case SIME_KEY_LEFT:								// ��
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					CMLeft( &cs,1 );
					if (str[0]==0x08){
						DelStr( &cs,1,1 );						// �J�[�\���ʒu�̕������폜
					}
					cs.adjust = 1;
					break;
				case SIME_KEY_RIGHT:							// ��
					if (!(pad.Buttons & PSP_CTRL_CROSS)) gTSel[0].rno = -1;
					CMRight( &cs, 1 );
					cs.adjust = 1;
					break;
				}
			} else {											// �����������ꍇ
				if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr)
					gTSel[0].rno = -1;
				if (gTSel[0].rno!=-1){
					TextDel(&cs);
					gTSel[0].rno = -1;
					DrawFullRec(cs.pline);						// ��ʑS�����������
				}
				gSave = 1;
				ret = AddStr( &cs, str, 1, 1 );
				cs.adjust = 1;
			}
			if (save!=gSave){									// �u�X�V�v�̏�������
				save = gSave;
				DrawTitle(textFile);
				DrawSts( &cs, 1 );								// �X�e�[�^�X�X�V
			} else {
				DrawSts( &cs, 0 );								// �X�e�[�^�X�X�V
			}
			if (str[0]==SIME_KEY_START && gTSel[0].rno!=-1){	// ���j���[���J���Ă��͈͑I�����ێ������悤��
				pad.Buttons |= PSP_CTRL_CROSS;
				pad2.Buttons |= PSP_CTRL_CROSS;
			}
			if (!(pad.Buttons & PSP_CTRL_CROSS) && (pad2.Buttons & PSP_CTRL_CROSS)){
				if (gTSel[0].rno==gTSel[1].rno && gTSel[0].cxr==gTSel[1].cxr){
					gTSel[0].rno = -1;							// �͈͂��w�肳��Ă��Ȃ��̂ł��ꂾ��
				} else {
					gTSel[0].rno = -1;
					DrawFullRec(cs.pline);						// ��ʑS�����������
				}
			} else if ((pad.Buttons & PSP_CTRL_CROSS) && !(pad2.Buttons & PSP_CTRL_CROSS)){
				gTSel[0].rno = cs.rno;
				gTSel[0].cxr = cs.cxr;
				gTSel[0].line = cs.pline + cs.cy;
			}
			pad2 = pad;
		}

		dcFlag = 0;												// �J�[�\���̏������d�����s���Ȃ����߂̃t���O�i�i�i���ɃX�N���[������ꍇ�̑Ή��j
		if (gRoll){
			if (pad.Lx<128){									// �A�i���O�p�b�h�ɂ�鉡�X�N���[��
				if (pad.Lx<(128-40)){							// ����
					off1 = gXOffset;
					gXOffset += (pad.Lx-128+41)/16;
					if (gXOffset<0) gXOffset = 0;
					off2 = gXOffset;
					if (off1!=off2){
						RightShift( &cs,off1-off2 );			// ��ʕ\�����E�ɂ��炷
						dcFlag = 1;
					}
				}
			} else {
				if (pad.Lx>128+40){								// �E��
					off1 = gXOffset;
					gXOffset += (pad.Lx-128-40)/16;
					if (gXOffset>WWIDTH-LWIDTH) gXOffset = WWIDTH-LWIDTH;
					off2 = gXOffset;
					if (off1!=off2){
						LeftShift( &cs,off2-off1 );				// ��ʕ\�������ɂ��炷
						dcFlag = 1;
					}
				}
			}
		}
		if (pad.Ly<128){										// �A�i���O�p�b�h�ɂ��c�X�N���[��
			if (pad.Ly<(128-40)){
				off1 = vy;
				vy += (float)(pad.Ly-128+41)/200;
				off2 = vy;
				if (off1!=off2){
					vy += 1;
					DownShift( &cs,dcFlag );					// ���X�N���[��
				}
			}
		} else {
			if (pad.Ly>128+40){
				off1 = vy;
				vy += (float)(pad.Ly-128-40)/200;
				off2 = vy;
				if (off1!=off2){
					vy -= 1;
					UpShift( &cs,dcFlag );						// ��X�N���[��
				}
			}
		}

		if (ret){
			strcpy( msg,"�g�p�ł��郁����������Ȃ��Ȃ�܂����B\n����ȏ㕶����ǉ��ł��܂���B" );
			DiaBox1( -1, 110, 0, msg, gSCor[0], gSCor[3], gSCor[4] );	// �����ɕ\��
			Unpush();											// �{�^�����������܂őҋ@
			WaitBtn();											// �����������܂őҋ@
			DrawFullRec(cs.pline);								// ��ʑS�����������
		}

		sceDisplayWaitVblankStart();
		ScreenView();											// ��ʍX�V
	}

	//----- owata���֕������Ԃ� -----
	// �s�������A�T�C�Y�����𒴂��������͔j������܂��B

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
				if (pos>maxsize-2) break;						// ����������
			}
			if (pos>maxsize-2) break;							// ����������
			if (gText[rno]->enter) line++;						// ���s���J�E���g
			if (gText[rno]->chainNext==0xFFFFFFFF) break;		// �ŏI�s�ɒB������I��
			rno = gText[rno]->chainNext;
		}
		text[pos] = '\0';
	}

	ClrRec();													// �s���R�[�h�\���̂̑S�폜

	//----- �I������ -----

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
	if (gTCChg){												// �e�L�X�g�N���b�v�ɕύX��������
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
	sceGuSync(0,0);												// VRAM�N���A

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

