//==============================================================
// ��ʍ��֘A
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

static struct {													// monafont�Ǘ��p
	char	*fontA;
	long	sizeA;
	char	*fontW;
	long	sizeW;
	int		height;
} gMona[2];

//static intraFont*	gJpn0;										// intraFont�p
static int*			gJpn0;										// �_�~�[
static int			gFont = 0,									// pf_print()�Ŏg�p�����t�H���g
					gProgX,gProgY;								// �v���O���X�o�[�̈ʒu

//==============================================================
// �����̍��
//--------------------------------------------------------------

void HLine(int x,int y,int wx,long cor)
{
	int i;

	for (i=0; i<wx ;i++)
		VRAM[y][x+i] = cor;
}

//==============================================================
// �c���̍��
//--------------------------------------------------------------

void VLine(int x,int y,int wy,long cor)
{
	int i;

	for (i=0; i<wy ;i++)
		VRAM[y+i][x] = cor;
}

//==============================================================
// BOX�̍��
//--------------------------------------------------------------

void Box(int x,int y,int wx,int wy,long corFrame)
{
	HLine(x       ,y       ,wx,corFrame);
	VLine(x       ,y       ,wy,corFrame);
	VLine(x + wx-1,y       ,wy,corFrame);
	HLine(x       ,y + wy-1,wx,corFrame);
}

//==============================================================
// �h��Ԃ��ꂽBOX�̍��
//--------------------------------------------------------------
// �ŏ��̂P���C���ڂ�CPU�ō�悵�A�c��̕�����GU�ɂ��P���C���ڂ��R�s�[����A
// �Ƃ������@���g���Ă��܂��B
// �������̂ق���CPU�ōs����葁�����ۂ��B
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
// �h��Ԃ��ꂽBOX�̍��i�g����j
//--------------------------------------------------------------

void BoxFill(int x,int y,int wx,int wy,long corFrame,long corIn)
{
	Fill(x ,y ,wx ,wy ,corIn);
	Box(x ,y ,wx ,wy ,corFrame);
}

//==============================================================
// �w�i���]��BOX�̍��
//--------------------------------------------------------------

void XFill(int x,int y,int wx,int wy,long corIn)
{
	int i,j;

	for (i=0; i<wy ;i++)
		for (j=0; j<wx ;j++)
			VRAM[y+i][x+j] ^= corIn;
}

//==============================================================
// �p�ۂ�BOX�̍��i�g����j
//--------------------------------------------------------------
// type   �p�ۂ̑傫���i0:������ �` 3:�傫�� , 4�`5:�O���f�[�V�����j
// corFrm �g�̐F
// corIn  �����̐F
//---------------------------------------------------------------
// �O���f�[�V�����p�^�[���̏ꍇ��corFrm�̎w��͖�������AcorIn�����ɖ��邳��
// �ς���悵�܂��B
// �܂��A�O���f�[�V�����p�^�[���ł̍�掞�͓��ߏ������s���܂��B
// ���ߏ����ɂ� graphics.c �𗘗p���Ă��܂��B
//--------------------------------------------------------------

//----- �R���g���X�g�̒��� -----

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

//----- �n�_�ƏI�_���F�Ⴂ�̉�������� -----

static void CurveBoxSub(int x,int y,int wx,long corFrm,long corIn)
{
	VRAM[y][x] = corFrm;
	HLine(x+1 ,y ,wx-2 ,corIn);
	VRAM[y][x+wx-1] = corFrm;
}

//----- �e�N�X�`���[�Ɋp�ۃO���f�[�V�����p�^�[�����쐬 -----

static void CurveBoxSub2(int x,int y,int wx,int type,int no,long corIn,Image *source,int Alpha)
{
	const int	map1[3][9][8] = {{								// �p�̃p�^�[��
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
				map2[3][9] = {									// ���g�̃o�^�[��
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

//----- ���C�� -----

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
		sceGuTexMode(GU_PSM_8888, 0, 0, 0);						// ������ȗ������intraFont�g�p���ɕ\���������܂�
		sceGuFinish();
		source = createImage(wx+16,wy+16);
		for (i=0; i<wy+15 ;i++){								// �E�B���h�E�̉e
			if (i<8){
				CurveBoxSub2( 0 ,i ,wx+14 ,2 ,i ,0x000000 ,source ,0x80 );
			} else if (i>=(wy+15)-8){
				CurveBoxSub2( 0 ,i ,wx+14 ,2 ,(wy+15) -1 - i ,0x000000 ,source ,0x80 );
			} else {
				CurveBoxSub2( 0 ,i ,wx+14 ,2 ,8 ,0x000000 ,source ,0x80 );
			}
		}
		type -= 4;
		for (i=0; i<wy+4 ;i++){									// �E�B���h�E�̖{��
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
// �w��̈��0�N���A
//--------------------------------------------------------------

void BoxClr(int x,int y,int wx,int wy)
{
	int i;

	for (i=0; i<wy ;i++)
		HLine(x,y + i,wx,0);
}

//==============================================================
// ��ʃC���[�W�̓]��
//--------------------------------------------------------------
// ��ʏ�̎w��͈͂��w��ʒu�ɓ]�������܂��B
//--------------------------------------------------------------

void ImgMove(int sx,int sy,int wx,int wy,int dx,int dy)
{
	sceGuStart(GU_DIRECT,gList);
	sceGuCopyImage(GU_PSM_8888, sx,sy,wx,wy,512,VRAM, dx,dy,512,VRAM);
	sceGuFinish();
	sceGuSync(0,0);
}

//==============================================================
// �C���[�W����p�o�b�t�@�̊m��
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
// �C���[�W����p�o�b�t�@�̉��
//--------------------------------------------------------------

void DrawImgFree(DrawImg *image)
{
	if (image) free(image->data);
	free(image);
	image = NULL;
}

//==============================================================
// �w��̈�̃R�s�[
//--------------------------------------------------------------
// ��ʏ�̎w�肳�ꂽ�̈��image�Ɏ�荞�ށB
// image�͗\�ߕK�v�ȑ傫����DrawImgCreate()�Ŋm�ۂ��Ă����Ă��������B
// 16�o�C�g�̃A���C�����g�ɍ��킹�ă��������m�ۂ��Ă���͂��Ȃ̂ɃS�~���ł�
// �̂ŃS�~���o�Ȃ��悤�ɏ��׍H�����Ă݂܂����B
// ���̓L���b�V���̖�肾�����炵���B
// sceKernelDcacheWritebackInvalidateAll()�����s����悤�ɂ����璼�������ۂ��B
//--------------------------------------------------------------

void BoxCopy(DrawImg *image,int x,int y)
{
	void	*framebuffer;

	if (image){
		sceGuSwapBuffers();
		framebuffer = sceGuSwapBuffers();
		sceKernelDcacheWritebackInvalidateAll();				// �L���b�V���̃��C�g�o�b�N
		sceGuStart(GU_DIRECT,gList);
		sceGuCopyImage(GU_PSM_8888, x,y,image->wx,image->wy,512,framebuffer+0x04000000, 0,0,image->width,image->data);
		sceGuFinish();
		sceGuSync(0,0);
	}
}

//==============================================================
// �w��̈�̃y�[�X�g
//--------------------------------------------------------------
// ��ʏ�̎w�肳�ꂽ�ʒu��image��W�J����B
// �P���ɓ]�����邾���ŃA���t�@�����Ƃ��͂��Ȃ����A�����B
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
// ��ʃC���[�W�̕ۑ�
//--------------------------------------------------------------
// ���݂̉�ʑS�̂�Ҕ�������B
// �K��ScreenPaste()��DrawImgFree()�Ńo�b�t�@��������鎖�B
//--------------------------------------------------------------

DrawImg *ScreenCopy(void)
{
	DrawImg	*image;

	image = DrawImgCreate(480,272);
	BoxCopy( image, 0, 0 );
	return (image);
}

//==============================================================
// ��ʃC���[�W�̕����ƃo�b�t�@�̍폜
//--------------------------------------------------------------
// �Ҕ������Ă�����ʃC���[�W�𕜌����A�o�b�t�@���폜����B
// �����͈�񂵂��ł��܂���B
// �K��ScreenPaste()��DrawImgFree()�Ńo�b�t�@��������鎖�B
//--------------------------------------------------------------

void ScreenPaste(DrawImg *image)
{
	BoxPaste( image, 0, 0 );
	DrawImgFree(image);
}

//==============================================================
// intraFont���[�h
//--------------------------------------------------------------
// �߂�l  0:����
//        -1:���[�h���s
//--------------------------------------------------------------
// intraFont�̓��{��t�H���g��ǂݍ��ށB
// ��������s���Ă��Ȃ��ꍇ�AintraFont�͎g���܂���B�i��ւœ��_�t�H���g���g���܂��B�j
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
// intraFont�I������
//--------------------------------------------------------------

void pf_endIntraFont(void)
{
//	intraFontUnload(gJpn0);
//	intraFontShutdown();
}

//==============================================================
// monafont���[�h
//--------------------------------------------------------------
// no     �t�H���g�̎�ʁi0:monafont12 1:monafont16�j
// pathA  ���p�t�H���g�̃t�@�C����
// pathW  �S�p�t�H���g�̃t�@�C����
// �߂�l  0:����
//        -1:���[�h���s
//--------------------------------------------------------------
// monafont��ǂݍ��ށB
// ��������s���Ă��Ȃ��ꍇ�Amonafont�͎g���܂���B�i��ւœ��_�t�H���g���g���܂��B�j
//--------------------------------------------------------------

int pf_setMonaFont(int no,char *pathA,char *pathW)
{
	int	fd;

	fd = sceIoOpen( pathA, PSP_O_RDONLY, 0777 );				// ���p�t�H���g
	if (fd>=0){
		gMona[no].sizeA = sceIoLseek( fd, 0, SEEK_END );
		sceIoLseek( fd, 0, SEEK_SET );
		gMona[no].fontA = (char*) malloc(gMona[no].sizeA);
		if (gMona[no].fontA){
			sceIoRead( fd, gMona[no].fontA, gMona[no].sizeA );
		}
		sceIoClose(fd);
	}
	fd = sceIoOpen( pathW, PSP_O_RDONLY, 0777 );				// �S�p�t�H���g
	if (fd>=0){
		gMona[no].sizeW = sceIoLseek( fd, 0, SEEK_END );
		sceIoLseek( fd, 0, SEEK_SET );
		gMona[no].fontW = (char*) malloc(gMona[no].sizeW);
		if (gMona[no].fontW){
			sceIoRead( fd, gMona[no].fontW, gMona[no].sizeW );
		}
		sceIoClose(fd);
	}

	if (!gMona[no].fontA || !gMona[no].fontW){					// �t�H���g�̃��[�h�Ɏ��s���Ă����ꍇ
		free(gMona[no].fontA);
		gMona[no].sizeA = 0;
		free(gMona[no].fontW);
		gMona[no].sizeW = 0;
		return (-1);
	}
	return (0);
}

//==============================================================
// monafont�̏I��
//--------------------------------------------------------------

void pf_endMonaFont(void)
{
	free(gMona[0].fontA);
	free(gMona[0].fontW);
	free(gMona[1].fontA);
	free(gMona[1].fontW);
}

//==============================================================
// �t�H���g�̃`�F�b�N
//--------------------------------------------------------------
// �w�肳�ꂽ�t�H���g���{���Ɏg���邩�`�F�b�N���A�g���Ȃ��ꍇ�͓��_�t�H���g�ɐ؂�ւ���B
//--------------------------------------------------------------

static int chkFont(int font)
{
	if (font>4) font = 0;										// �͈͊O�̎��͓��_�t�H���g
	if ((font==3 || font==4) && !gJpn0) font = 0;				// intraFont���g���Ȃ�
	if ((font==1 && !gMona[0].fontA) || (font==2 && !gMona[1].fontA)){
		font = 0;												// monafont���g���Ȃ�
	}
	return (font);
}

//==============================================================
// �����t�H���g�̎w��
//--------------------------------------------------------------
// font   �����̃t�H���g�i0:���_�t�H���g    1:monafont12   2:monafont16
//                        3:intraFont����   4:intraFont�v���|�[�V���i���j
// �߂�l ���ۂɑI�����ꂽ�����t�H���g
//--------------------------------------------------------------
// pf_print()�ŕ����\������ꍇ�̃t�H���g���w�肷��B
// �w�肳�ꂽ�t�H���g���g�p�ł��Ȃ��ꍇ�͓��_�t�H���g���g���܂��B
//--------------------------------------------------------------

int pf_fontType(int font)
{
	gFont = chkFont(font);
	return (gFont);
}

//==============================================================
// monafont�G���g���[�A�h���X�̎擾
//--------------------------------------------------------------
// no     monafont�̎�ށi0:monafont12 1:monafont16�j
// str    �ŏ��̈ꕶ���ڂɎ擾����������������
// fwidth �����̉���
// �߂�l �t�H���g�f�[�^
//--------------------------------------------------------------
// �w�肳�ꂽ�����ɑΉ�����t�H���g�f�[�^�ւ̃A�h���X���擾����B
// �t�H���g�f�[�^�̃t�H�[�}�b�g��
//     12�h�b�gmonafont:������(1)�A�t�H���g�C���[�W(15)
//     16�h�b�gmonafont:������(1)�A�t�H���g�C���[�W(16)
//
// fwidth�ŕԂ���镶�����̓t�H���g�̕��ł͂Ȃ��A��ʏ�ł̐�L���Ŏ����l�ɂȂ�܂��B
// �w��t�H���g���g���Ȃ��ꍇ��NULL���Ԃ���܂��B
//--------------------------------------------------------------

static unsigned short *GetMonaFont(int no,const char *str,float *fwidth)
{
	int	hi,lo,index;
	unsigned short *font;

	if (!gMona[no].fontA) return (NULL);						// �t�H���g���g���Ȃ�

	if (chkSJIS(str[0])){										// �S�p�����Ȃ�
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

	} else {													// ���p�����Ȃ�
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
// monafont�ŕ����\��
//--------------------------------------------------------------
// no     monafont�̎�ށi0:monafont12 1:monafont16�j
// �߂�l �Ō�ɕ\�����������̏I��X���W
//--------------------------------------------------------------
// �\��X���W����ʊO�ɂȂ������_�ŕ\���͒��f����܂��B
// ���̎��Ō�̕����͉�ʊO�ɂ͂ݏo���܂��B
// �\��Y���W����ʊO�ɂȂ��������̍��͍s���܂���B
// �iY���W��0�`271�͈̔͂łȂ��Ǝg���Ȃ��j
// �w��t�H���g���g���Ȃ��ꍇ�͍��͍s���܂���B
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
		fd = GetMonaFont( no,&str[sPos], &fw );					// �t�H���g�f�[�^�擾
		if (!fd) break;
		fdw = *fd++;											// ������
		if (fdw>16) fdw = 16;
		for (fy=0; fy<fdh ;fy++){								// �t�H���g�C���[�W���
			bit = 0x8000;
			if (y+fy>=SCR_HEIGHT) break;						// ��ʂ���͂ݏo���Ȃ�
			vram = &VRAM[y+fy][(int)x];
			for (fx=0; fx<fdw ;fx++){
				if (*fd & bit) *vram = cor;
				vram++;
				bit >>= 1;
			}
			fd++;
		}
		x += fw;
		if (x>SCR_WIDTH) break;									// ��ʊO�ɂ͂ݏo�����Ȃ�
		if (chkSJIS(str[sPos])) sPos++;
		sPos++;
	}

	return (x);
}

//==============================================================
// �����t�H���g�̕\��
//--------------------------------------------------------------
// �߂�l �\�������t�H���g�̕�����
//--------------------------------------------------------------
// pf_fontType()�Ŏw�肳�ꂽ�t�H���g�ŕ�����\�����܂��B
//--------------------------------------------------------------

float pf_print(float x,int y,const char *str,long cor)
{
	return (pf_print2( x, y, gFont, str, cor ));
}

//==============================================================
// �����t�H���g�̕\��2
//--------------------------------------------------------------
// font   �����̃t�H���g�i0:���_�t�H���g    1:monafont12   2:monafont16
//                        3:intraFont����   4:intraFont�v���|�[�V���i���j
// �߂�l �\�����������̎��̈ʒu
//--------------------------------------------------------------
// �w�肳�ꂽ�t�H���g�ŕ�����\�����܂��B
// �w��t�H���g���g���Ȃ��ꍇ�͓��_�t�H���g���g���܂��B
//--------------------------------------------------------------

float pf_print2(float x,int y,int font,const char *str,long cor)
{
	int	x2 = 0;

	if (font>4) font = 0;										// �͈͊O�̎��͓��_�t�H���g
	if (!gJpn0 && (font==3 || font==4)){						// intraFont���g���Ȃ��Ȃ�
		font -= 3;												// monafont�ɑ��
	}
	if ((font==1 && !gMona[0].fontA) || (font==2 && !gMona[1].fontA)){
		font = 0;
	}

	switch (font){
	case 0:														// monafont����
		mh_print( x,y,str,cor );
		x2 = x + strlen(str) * 6;
		break;
	case 1:														// monafont12�v���|�[�V���i��
		x2 = monaPrint( x,y,0,str,cor );
		break;
	case 2:														// monafont16�v���|�[�V���i��
		x2 = monaPrint( x,y,1,str,cor );
		break;
	case 3:														// intraFont����
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
	case 4:														// intraFont�v���|�[�V���i��
		guStart();
//		intraFontSetStyle( gJpn0, 0.65f, cor | 0xFF000000, 0x00000000, 0 );
//		x2 = intraFontPrint(gJpn0, x, y+10, str);
		sceGuFinish();
		break;
	}

	return (x2);
}

//==============================================================
// ������̕�
//--------------------------------------------------------------
// font   �����̃t�H���g�i0:���_�t�H���g    1:monafont12   2:monafont16
//                        3:intraFont����   4:intraFont�v���|�[�V���i���j
// str    ������
// �߂�l ��L���i�h�b�g�j
//--------------------------------------------------------------
// ��ʂɕ�����\�������ۂ̕��������h�b�g�P�ʂŎ擾����B
// �w��t�H���g���g���Ȃ��ꍇ�͓��_�t�H���g���g���܂��B
// �c�{���͏����_�ȉ��̒[���܂ōl�����č����s�����Ǝv���Ă����񂾂��ǁA
// ��������ƍ��E�X�N���[�����ɕ����\���ʒu��1�h�b�g�̃Y�����o�ĕ␳�������
// �Ȃ����̂Ŏ~�߂܂����B
// �[�������������\�����������肵������������̂ŁA�������Ă悩���������B
//--------------------------------------------------------------

float GetStrWidth(int font,const char *str)
{
	unsigned short	*fd;
	int				pos;
	float			w,fw;

	font = chkFont(font);										// �͈͊O�̎��͓��_�t�H���g

	if (font==0 || font==3){									// �����t�H���g
		w = strlen(str) * 6;
	} else if (font==4){										// intraFont�v���|�[�V���i��
//		w = (int) intraFontMeasureText( gJpn0,str );			// ������������ƍ�掞�Ɉʒu�Y������������̂�
		w = 6;
	} else {													// monafont�v���|�[�V���i��
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
// �����s������̉��
//--------------------------------------------------------------
// text�����s�i'\n'�j����͂��čs���ƍő�s�����擾����B
// pf_fontType()�̉e�����󂯂܂��B
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
	while (1){													// �s���̊m�F
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
// �����s������̕\��
//--------------------------------------------------------------
// text�����s�i'\n'�j�ɑΉ����Ă�̂ŕ����s�̕\���������܂��B
//--------------------------------------------------------------

void DrawText(int x,int y,int font,char *text,long corStr)
{
	char	s,str[128];
	int		p,pos;

	pos = 0;
	while (text[pos]!='\0'){
		p = 0;
		while (1){												// ��s�������o��
			s = text[pos++];
			if (s=='\0'){
				pos--;
				break;
			}
			if (s=='\n') break;
			str[p++] = s;
		}
		str[p] = '\0';
		pf_print2(x, y, font, str, corStr);						// ��s���\��
		y += 12 +1;
	}
}

//==============================================================
// ���b�Z�[�W�_�C�A���O�̕\��
//--------------------------------------------------------------
// text�����s�i'\n'�j�ɑΉ����Ă�̂ŕ����s�̕\���������܂��B
// x�Ƀ}�C�i�X�̒l���w�肷��Ɖ�ʒ����ɕ\�����܂��B
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
// �m�F�_�C�A���O�̕\��
//--------------------------------------------------------------
// text�����s�i'\n'�j�ɑΉ����Ă�̂ŕ����s�̕\���������܂��B
// ���b�Z�[�W�̌��0.5�s�󂯂ăL�[�K�C�_���X��\�����܂��B
// x�Ƀ}�C�i�X�̒l���w�肷��Ɖ�ʒ����ɕ\�����܂��B
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
// Yes/No�m�F�_�C�A���O
//--------------------------------------------------------------
// msg    ���b�Z�[�W�i���s����j
// �߂�l 1:���������ꂽ
//        0:���ȊO
//--------------------------------------------------------------
// ���b�Z�[�W�Ɂu��:Yes  �~:No�v��ǉ�������œ��͏������s���܂��B
//--------------------------------------------------------------

int DialogYN(char *msg,long corFrm,long corIn)
{
	SceCtrlData		pad;
	int				ret;

	DiaBox2( -1, 104, 0, msg, "��:Yes  �~:No", gSCor[0], gSCor[13], corFrm, corIn );	// �����ɕ\��
	ScreenView();												// ��ʍX�V
	while (1){
		sceCtrlReadBufferPositive(&pad, 1);
		if (!(pad.Buttons & 0x00F3F9)) break;					// ����n�{�^�����S�ė����ꂽ
		sceDisplayWaitVblankStart();
	}
	ret = 0;
	while (!gExitRequest){										// �����~���������܂őҋ@
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
// �_�C�A���O�t���[���̍\�z
//--------------------------------------------------------------

void DialogFrame(int x,int y,int wx,int wy,char *title,char *guide,long corFrm,long corIn)
{
	CurveBox( x, y, wx, wy, 4, corFrm, corIn );
	mh_print( x+10, y+6, title, corFrm );
	mh_print( x+wx-10-6*strlen(guide), y+wy-6-12, guide, corFrm );
}

//==============================================================
// �v���O���X�o�[�̕\��1
//--------------------------------------------------------------
// �v���O���X�o�[�̏����\�����s���܂��B
// �i�s�󋵂�Progressbar2()�ōX�V���܂��B
// �T�C�Y�͌Œ�ł��B
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
// �v���O���X�o�[�̕\��2
//--------------------------------------------------------------
// �v���O���X�o�[�̍X�V���s���܂��B
// �\��Progressbar1()�ŏ����ݒ���s���Ă������ƁB
// �o�[������ꍇ�͍l�����Ă܂���B
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
// �X�N���[���o�[�̕\���i�c�j
//--------------------------------------------------------------
// x,y  �X�N���[���o�[�ʒu
// wy   �X�N���[���o�[�̑傫��
// pos  �\���ʒu
// size �\���T�C�Y
// max  �S�̂̑傫��
// flag ������悷�邩�H�i0:�O��Ɠ����Ȃ��悵�Ȃ��j
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
	if (len > (wy -2)) len = wy -2;								// ���͈͂���͂ݏo��ꍇ�̕␳
	if (p + len > (wy -2)) p = wy -2 - len;						// ���͈͂���͂ݏo��ꍇ�̕␳

	if (lenBuf==len && pBuf==p && flag==0) return;				// �O��Ɠ����Ȃ��悵�Ȃ�
	lenBuf = len;
	pBuf = p;

	BoxFill(x, y, 5, wy, corFrm, corIn);						// ��x������
	Fill(x+1, y+1+p, 3, len, corBar);							// �X�N���[���o�[�����
}

//==============================================================
// �����̕\��
//--------------------------------------------------------------
// x,y    �\���J�n�ʒu
// val    �\������l�i�U���܂Łj
// ztype  �擪�̋󔒂�O�l�߂��邩�i0:���Ȃ��j
// ctype  �����̎�ށi0:3x5�t�H���g 1:���_�t�H���g 2:monafont12
//                    3:monafont16  4:intraFont    5:intraFontP�j
// corStr �����F
// �߂�l �\���I�����W
//--------------------------------------------------------------

//----- 3x5���l�t�H���g�\�� -----

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

//----- �e���̒l��\�� -----

static int dvsub(int x,int y,int val,int flag,int ztype,int ctype,long corStr)
{
	static char	zero;
	char		c[2] = {0,0};

	if (flag==0){
		zero = ' ';
	} else if (flag==2){
		zero = '0';
	}

	if (val!=0){												// ��s����'0'���󔒂ɕϊ����邩
		zero = '0';
		c[0] = '0' + val;
	} else {
		c[0] = zero;
	}
	if (ztype && c[0]==' ') return (x);							// �󔒂̑O�l�߂�����Ȃ�
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
// �L���̕\��
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
// ��ʕ\���̍X�V
//--------------------------------------------------------------
// VRAM����ʂ̃C���[�W����\����ʂ֓]�����邱�Ƃŉ�ʕ\�����X�V������B
// �_�u���o�b�t�@���g�p���K�v�Œ���̉�ʏ��������ŕ\�����s�����߂̋���̍�B
// VRAM�y�[�W0�����y�[�W�A�y�[�W1���\���y�[�W�Ō��ߑł����Ă܂��B
//--------------------------------------------------------------

void ScreenView(void)
{
	sceGuStart( GU_DIRECT, gList );
	sceGuCopyImage( GU_PSM_8888, 0, 0, 480, 272, 512, VRAM, 0, 272, 512, VRAM);
	sceGuFinish();
	sceGuSync(0,0);
}

