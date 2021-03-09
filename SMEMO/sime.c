//==============================================================
// Simple IME   (STPP.04)
//     for PSP CFW5.00 M33-6
// STEAR 2009-2010
//--------------------------------------------------------------
// PSP�p�̊Ȉ�IME���쐬���Ă݂��B
// �\�t�g�E�F�A�L�[�{�[�h�Ən�ꊿ���ϊ��ƃ��[�U�[�����i�\���ϊ��j�������B
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

//----- �}�N�� -----


//----- �萔�ݒ� -----

#define SIMENAME	"Simple IME Ver1.32"

#define DICFILE1	"SIME.DIC"					// ��{�����t�@�C����
#define DICFILE2	"ms0:/PSP/COMMON/SIME.DIC"
#define DIC2FILE1	"SIMEUSER.DIC"				// ���[�U�[�����t�@�C����
#define DIC2FILE2	"ms0:/PSP/COMMON/SIMEUSER.DIC"
#define INIFILE1	"SIME.INI"					// ���ݒ�t�@�C��
#define INIFILE2	"ms0:/PSP/COMMON/SIME.INI"

#define KBPOSX1		(268)						// �\�t�g�E�F�A�L�[�̈ʒuX�i�T�O���z��A�V�X�g�J�[�\���j
#define KBPOSY1		(157)						// �\�t�g�E�F�A�L�[�̈ʒuY�i�T�O���z��A�V�X�g�J�[�\���j
#define KBPOSX3		(217)						// �\�t�g�E�F�A�L�[�̈ʒuX�i�~�T���Ȕz�u�j
#define KBPOSY3		(142)						// �\�t�g�E�F�A�L�[�̈ʒuY�i�~�T���Ȕz�u�j
#define MEPOSX		(131)						// �ݒ��ʂ̈ʒu
#define MEPOSY		(35)						// �ݒ��ʂ̈ʒu
#define KEYREP1		(20)						// �L�[���s�[�g�J�n�܂ł̎���
#define KEYREP2		(3)							// �L�[���s�[�g�̊Ԋu
#define FONTX		(6)							// �t�H���g�̒���
#define FONTY		(16)						// �t�H���g�̍ő卂��
#define CHLEN		(32)						// �ϊ����C���̍ő啶����
#define KLIST		(6)							// ������⃊�X�g�̍s��
#define KMAX		(102)						// ������⃊�X�g�̍ő吔
#define CBTIME		(25)						// �J�[�\���_�Ŏ���
#define CURSTEP		(7)							// �A�V�X�g�J�[�\���̈ړ����v����
#define YOMILMAX	(102)						// ���̐��ȉ��Ɂu��݁v��␔���Ȃ����烊�X�g��\��������

#define CORFL1		0xC0A0A0					// �g
#define CORFL2		0xF0D0D0					// ���邢�g
#define CORFL3		0x707070					// �Â��g
#define CORIN		0x600060					// �E�B���h�E���w�i
#define CORFR		0xA08080					// �E�B���h�E���O�i
#define CORCUR		0xFF8080					// �J�[�\��
#define CORCUR1		0x7070FF					// �J�[�\����
#define CORCUR2		0x008000					// �J�[�\����
#define CORCUR3		0xA0A0E0					// �J�[�\����
#define CORCUR4		0xFF8080					// �J�[�\���~
#define CORRBAR		0x70E070					// �X�N���[���o�[
#define CORWCHR		0xFFFFFF					// �E�B���h�E������
#define CORSCHR		0xB0FFB0					// �E�B���h�E�����ꕶ��
#define CORFCHR		0x000000					// �E�B���h�E�g����
#define CORCHBK		0xFFFFFF					// �ϊ����C���w�i
#define CORCHCR		0x000000					// �ϊ����C������
#define CORCHCU		0xFF8080					// �ϊ����C���I��̈�

//----- �v���g�^�C�v�錾 -----

static void putBack(void);
static unsigned int	fcode(char *str);
static unsigned int	getInt(char *str);
static void kList(char wordList[][2][33],int count,int index);
static void getkList(char wordList[][2][33],int count);
static void putkList(void);

//------ �O���[�o���ϐ� -----

static char			gKeyName1[7][25] = {
						"�P�F�S�p �Ђ炪��",
						"�Q�F�S�p �J�^�J�i",
						"�R�F���p �J�^�J�i",
						"�S�F�S�p �A���t�@�x�b�g",
						"�T�F�S�p �L��",
						"�U�F���p �A���t�@�x�b�g",
						"�V�F���p �L��",
					};
static char			gKeyTable1[7][161] = {
						"�O�����������P�����������Q�����������R�����ĂƂS�Ȃɂʂ˂̂T�͂ЂӂւقU�܂݂ނ߂��V����J�K�W������X�����A�B�I�����������H�������h�[�u�v�E�@",
						"�O�A�C�E�G�I�P�J�L�N�P�R�Q�T�V�X�Z�\\�R�^�`�c�e�g�S�i�j�k�l�m�T�n�q�t�w�z�U�}�~�������V�������J�K�W�����������X�������A�B�I�@�B�D�F�H�H���������b�h�[�u�v�E�@",
						"0 � � � � � 1 � � � � � 2 � � � � � 3 � � � � � 4 � � � � � 5 � � � � � 6 � � � � � 7 � � � � � 8 � � � � � 9 � � � � � ! � � � � � ? � � �   � \" � � � �  ",
						"�O�`�a�b�c�d�P�����������Q�e�f�g�h�i�R�����������S�j�k�l�m�n�T�����������U�o�p�q�r�s�V�����������W�t�u�v�w�x�X�����������I�y���|�C�D�H�m�i���o�h�f�n�j���p�@",
						"�O�I�h���i���P�H�f���j���Q�����������R���{�|�}�~�S�^���o�C�G�T�_���p�D�F�U�m�Q�����O�V�n�b�`�E�c�W�������􁙂X�������~�������T�U�̓��́��������ӃքD�c�����@",
						"0 A B C D E 1 a b c d e 2 F G H I J 3 f g h i j 4 K L M N O 5 k l m n o 6 P Q R S T 7 p q r s t 8 U V W X Y 9 u v w x y ! Z z - , . ? [ ( < { : / ] ) > }   ",
						"7 4 1 0 + * 8 5 2 , - / 9 6 3 . = #             ( < [ { : ! ) > ] } ; ? \" $ % & @ \\ ' ^ _ | ~                                                     \r \b \x7F \t   "
					};

static char			gKeyTable3[7][181] = {
						"�����������������������������������ĂƂȂɂʂ˂̂͂Ђӂւق܂݂ނ߂�����������������񂟂����������J�K�A�B�[�u�I�v�H�O�P�Q�R�S�T�U�V�W�X�E�i�c�j�`�|�{������\t \b \x7F \r �@",
						"�A�C�E�G�I�J�L�N�P�R�T�V�X�Z�\\�^�`�c�e�g�i�j�k�l�m�n�q�t�w�z�}�~�������������������������������������@�B�D�F�H�b�J�K�A�B�[�u�I�v�H�O�P�Q�R�S�T�U�V�W�X�E���������|�i�`�j�{\t \b \x7F \r �@",	// �u�\\�v���ӁI
						"� � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � � �   � � � � � � � � � � � � � ! � ? 0 1 2 3 4 5 6 7 8 9 � ~ | / _ - ( = ) + \t \b \x7F \r   ",
						"�O�P�Q�R�S�T�U�V�W�X�`�a�b�c�d�e�f�g�h�i�j�k�l�m�n�o�p�q�r�s�t�u�v�w�x�y�����������������������������������������������������f�C�Q�I�i�D�j�H���m���n���F�o�^�p�`���|���{��\t \b \x7F \r �@",
						"�O�P�Q�R�S�T�U�V�W�X�h���������C�i���j�D���{�}�|�~�F�^���_�G�I���Q���H�O�m���n�f�b�o�`�p�E�c�������􁪁����������������~�����T�U�������́����̃Ӄ��քD�c�����|�������⁂\t \b \x7F \r �@",
						"0 1 2 3 4 5 6 7 8 9 A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z ' , _ ! ( . ) ? < [ @ ] > : { / } ; = - * + \\ \t \b \x7F \r   ",
						"0 1 2 3 4 5 6 7 8 9 ! \" # $ % & ( ' ) * + , - . / : < = > ; ? [ \\ ] @ ^ { _ } | ' � ~ � �                                                                                 \t \b \x7F \r   "
					};
static long			gHEDtbl[85],gDicWordPos,gDicSize;			// ��{�����Ǘ��p
static DrawImg 		*gBackBuf,									// �\�t�g�L�[�w�i�Ҕ��o�b�t�@
					*gInLnBuf,									// �ϊ����C���w�i�Ҕ��o�b�t�@
					*gkListBuf;									// ������⃊�X�g�w�i�Ҕ��o�b�t�@
static char			*gDic,gDicFile[33],gDic2File[33],gIniFile[33],
					gSt[CHLEN +1],								// �ϊ��s�̕���
					gYomi[CHLEN +1],gKouho[CHLEN +1];			// �ϊ����w�K�p
static int			gSetup = 0,									// ����������Ă���H�i0:No�j
					gMode,										// �J�ڏ��
					gBDFlag,									// �E�B���h�E�������̋���
					gSaveFlag,									// ��{�����ɕω����������H�i0:No�j
					gSave,										// ��{�����̊w�K���e��ۑ����邩
					gUSaveFlag,									// ���[�U�[�����ɕω����������H�i0:No�j
					gUSave,										// ���[�U�[�������g�p���邩
					gUDicAdd,									// ���[�U�[�����Ɍ���ǉ�����H�i0:Yes�j
					gIni,										// ���ݒ�ɕω����������H�i0:No�j
					gKey,										// �\�t�g�L�[�̎��
					gGetFlag = 0,								// �E�B���h�E�n�����������H�i0:No�j
					gCount = 0,									// �ϊ����̑���
					gPage;										// �����s����ʃy�[�W
static int			gCx,gCy,									// �����J�[�\���̈ʒu
					gCxw,gCyw,									// �����J�[�\���̌`��
					gFont;										// �������

static struct {
	int	x;
	int	y;
} gBackBufP,													// �\�t�g�L�[�w�i�̈ʒu
  gInLnBufP,													// �ϊ����C���w�i�̈ʒu
  gkListBufP;													// ������⃊�X�g�w�i�̈ʒu

static struct strUsDic {										// ���[�U�[�����Ǘ��\����
	struct strUsDic	*chain;										// ���̌��ւ̃|�C���^�iNULL���ƏI�[�j
	char			yomi[CHLEN +1];								// �u��݁v
	char			kouho[CHLEN +1];							// �u���v
} **gUsDic;


//==============================================================
// ��{�����t�@�C���̓ǂݍ���
//--------------------------------------------------------------
// �߂�l  0:�����̓ǂݍ��ݐ���
//        -1:�s���Ȏ����t�@�C��
//        -2:����������Ȃ�
//        -3:�����t�@�C�����J���Ȃ��i���݂��Ȃ��H�j
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
			fa = -1;											// �W�����v�e�[�u��������Ȃ�
		} else {
			for (i=0; i<85 ;i++){
				if (gHEDtbl[i]!=0xFFFFFFFF && (gHEDtbl[i]>filesize-pos)){
					fa = -1;									// �W�����v�悪�f�[�^�O�������Ă���
				}
			}
			if (!fa){
				gDic = (char*) malloc( filesize - pos );
				if (gDic==NULL){
					fa = -2;									// ���������m�ۂł��Ȃ�
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

	if (fa){													// �����t�@�C�����J���Ȃ�����
		for (i=0; i<85 ;i++){
			gHEDtbl[i] = 0xFFFFFFFF;
		}
	}

	return (fa);
}


//==============================================================
// ���[�U�[�����t�@�C���̏�����
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
// ���[�U�[�����p�������̈���J��
//--------------------------------------------------------------

static void UserDicFree(void)
{
	struct strUsDic	*usdic,*usdic2;
	int		i;

	for (i=0; i<85 ;i++){
		usdic = gUsDic[i];
		if (usdic){												// �u���v�Q������Ȃ�
			do{
				usdic2 = usdic->chain;
				free(usdic);
				usdic = usdic2;
			}while (usdic);										// �u���v����������폜��������
		}
	}
	free(gUsDic);												// ���[�U�[�������폜
}


//==============================================================
// ���[�U�[�����t�@�C���̓ǂݍ���
//--------------------------------------------------------------
// �ΏۂƂȂ郆�[�U�[������ gDic2File �Ŏw�肳��Ă��郂�m�ł��B
// ���[�U�[�����t�@�C���\���Ɉُ킪�������ꂽ�ꍇ�͓ǂݍ��݂𒆎~���A���[�U�[����
// �����������܂��B
//--------------------------------------------------------------

//----- �o�b�t�@�ǂݍ��� -----

static void UserDicLoadBuf(int fd,char *data,int *pos,int worksize)
{
	if (*pos<worksize) return;
	sceIoRead( fd, data, worksize );
	*pos = 0;
}

//----- �P�����荞�� -----

static int UserDicLoadWord(char *str,int fd,char *data,int *pos,int worksize)
{
	char	c;
	int		len;

	len = 0;
	c = -1;
	while (len<33 && c!='\0'){
		c = data[(*pos)++];
		UserDicLoadBuf( fd, data, pos, worksize );				// �o�b�t�@���f�[�^���s�����瑱�������[�h
		str[len++] = c;
	}
	if (c!='\0'){												// �������I�[�o�[�i�s���Ȏ����j
		return (-1);
	}
	return (0);
}

//----- ���C�� -----

static void UserDicLoad(void)
{
	const int		ws[4] = {16384,4096,1024,128};
	struct strUsDic	*usdic,*usdic2;
	char	*data,yomi[33],kouho[33];
	int		i,fd,hp,pos,flag,worksize;
	long	filesize,poscnt;

	//----- �����������i�ʏ�͂��Ȃ��Ă������񂾂��ǈꉞ�j -----

	if (gUsDic){
		UserDicFree();
		UserDicInit();											// ���[�U�[����������
	}

	//----- ���� -----

	for (i=0; i<4 ;i++){										// �o���邾���傫���o�b�t�@���m�ۂ���
		worksize = ws[i];
		data = (char*) malloc(worksize);
		if (data) break;										// �o�b�t�@�̊m�ۂɐ�������
	}
	if (!data) return;											// �o�b�t�@���m�ۂł��Ȃ������̂ŏI��
	fd = sceIoOpen( gDic2File, PSP_O_RDONLY, 0777 );
	if (fd>=0){
		filesize = sceIoLseek(fd, 0, SEEK_END);
		sceIoLseek(fd, 0, SEEK_SET);
	} else {													// �w��t�@�C�����J���Ȃ������̂ŏI��
		free(data);
		return;
	}

	//----- �����ǂݍ��� -----

	sceIoRead( fd, data, worksize );
	poscnt = 0;
	pos = 0;
	hp = 0;
	flag = 0;
	while (poscnt<filesize && hp<85){
		if (data[pos]!=0){										// �u���v�Q������Ȃ�
			usdic2 = NULL;
			while (data[pos]!=0){
				if (UserDicLoadWord( yomi, fd, data, &pos, worksize )){
					flag = -1;									// �������I�[�o�[�i�s���Ȏ����j
					break;
				}
				if (UserDicLoadWord( kouho, fd, data, &pos, worksize )){
					flag = -1;									// �������I�[�o�[�i�s���Ȏ����j
					break;
				}
				usdic = (struct strUsDic*) malloc( sizeof(struct strUsDic) );
				if (usdic){										// ���������擾�o�����Ȃ�
					usdic->chain = NULL;
					strcpy( usdic->yomi,yomi );
					strcpy( usdic->kouho,kouho );
					if (!usdic2){								// �ŏ��́u���v
						gUsDic[hp] = usdic;
					} else {									// �Q�Ԗڈȍ~�́u���v
						usdic2->chain = usdic;
					}
					usdic2 = usdic;
				} else {										// ���������擾�o���Ȃ�����
					flag = 1;									// �ǂ߂��Ƃ���܂ł͍̗p
					break;
				}
			}
		}
		if (flag) break;										// �ǂݍ��ݒ��f
		pos++;
		UserDicLoadBuf( fd, data, &pos, worksize );				// �o�b�t�@���f�[�^���s�����瑱�������[�h
		hp++;
	}
	sceIoClose( fd );

	//----- �ُ픭���΍� -----

	if (flag<0){												// �������e���s��������
		UserDicFree();
		UserDicInit();											// ���[�U�[����������
	}
}


//==============================================================
// �����t�@�C���̓ǂݍ��݂Ɗe�평����
//--------------------------------------------------------------
// flag      0:�E�B���h�E�������ɔw�i�𕜌�����
//        �ȊO:�E�B���h�E�������ɉ������Ȃ��i���C�����ō�悷��ׂ��j
// �߂�l    1:���ɏ���������Ă���
//           0:�����̓ǂݍ��ݐ���
//          -1:�s���Ȏ����t�@�C��
//          -2:������������Ȃ�
//          -3:�����t�@�C�����J���Ȃ��i���݂��Ȃ��H�j
//--------------------------------------------------------------
// �����t�@�C����IME���ݒ��ǂݍ��݂܂��B
// ���ɏ���������Ă���ꍇ�� flag �̐ݒ�ύX�̂ݎ��s���܂��B
// ���ݒ�t�@�C���͎����t�@�C���Ɠ����t�H���_�ɔz�u����܂��B
// ���[�U�[�������ǂ߂Ȃ������ꍇ�̌x���͂���܂���B
//--------------------------------------------------------------

int InitSIME(int flag)
{
	char	str[256],*p;
	int		ret,type,val;
	FILE*	fp1;

	//----- ���ϐ������� -----

	if (flag){
		gBDFlag = 1;											// �E�B���h�E�������ɉ������Ȃ�
	} else {
		gBDFlag = 0;											// �E�B���h�E�������ɔw�i�𕜌�����
	}

	if (gSetup) return (1);
	gSetup = 1;

	gMode = 0;
	gCxw = 2;													// �J�[�\���̑傫��
	gDicWordPos = -1;
	gDicSize = 0;
	gSaveFlag = 0;
	gSave = 0;													// �I�����Ɏ����̋L�ڏ���ۑ�����
	gUSaveFlag = 0;
	gUSave = 0;													// ���[�U�[�������g��
	gUDicAdd = 0;												// ���[�U�[�����Ɍ���ǉ�����
	gIni = 0;
	gKey = 0;													// �\�t�g�L�[�̎��
	gPage = 0;
	gSt[0] = '\0';
	gkListBufP.x = -1;

	//----- ��{�������[�h -----

	ret = DicLoad( DICFILE1 );
	strcpy( gDicFile ,DICFILE1 );
	strcpy( gDic2File ,DIC2FILE1 );
	strcpy( gIniFile ,INIFILE1 );
	if (ret){
		ret = DicLoad( DICFILE2 );								// COMMON�t�H���_
		strcpy( gDicFile ,DICFILE2 );
		strcpy( gDic2File ,DIC2FILE2 );
		strcpy( gIniFile ,INIFILE2 );
		if (ret){
			gDicFile[0] = '\0';
			strcpy( gIniFile ,INIFILE1 );
		}
	}

	//----- ���ݒ胍�[�h -----

	fp1 = fopen( gIniFile, "r" );								// ���ݒ�t�@�C���̓ǂݍ���
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
			val = (int)strtol( p,NULL,0 );						// �����񁨐��l�ϊ�
			if (val==0){										// �����ł͂Ȃ����ۂ��̂�
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

	//----- ���[�U�[�����ǂݍ��� -----

	UserDicInit();												// ���[�U�[����������
	if (!gUSave) UserDicLoad();

	return (ret);
}


//==============================================================
// �����t�@�C���֏�������
//--------------------------------------------------------------
// �������̕��בւ����ʂ������ɏ�������
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
// ���[�U�[�����t�@�C���֏�������
//--------------------------------------------------------------

//----- �o�b�t�@���t�@�C���֏����o�� -----
// �o�b�t�@�������ς��ɂȂ�����t�@�C���֏����o���B
// �o�b�t�@�ɗ]�T������Ƃ��͉������Ȃ��B
// �t�@�C���֏����o�����Ƃ��̓|�C���^�̒l��0�ɂ���B
// �o�b�t�@�ւ̃f�[�^�ǉ���1�o�C�g���s�����B

static void UserDicFlash(int fd,char *tmp,int *pos,int bufsize)
{
	if (*pos<bufsize) return;
	sceIoWrite( fd, tmp, *pos );
	*pos = 0;
}

//----- ���C�� -----

static void UserDicSave(void)
{
	const int		ws[4] = {16384,4096,1024,128};
	struct strUsDic	*usdic;
	char	*data;
	int		i,j,fd,pos,worksize;

	if (!gUSaveFlag) return;									// ���[�U�[�����ɕω����Ȃ�

	for (i=0; i<4 ;i++){										// �o���邾���傫���o�b�t�@���m�ۂ���
		worksize = ws[i];
		data = (char*) malloc(worksize);
		if (data) break;										// �o�b�t�@�̊m�ۂɐ�������
	}
	if (!data) return;											// �o�b�t�@���m�ۂł��Ȃ������̂ŏI��

	fd = sceIoOpen( gDic2File, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if (fd>=0){
		pos = 0;
		for (i=0; i<85 ;i++){
			usdic = gUsDic[i];
			if (!usdic){										// �u���v�Q���o�^����Ă��Ȃ�
				data[pos++] = 0;
				UserDicFlash( fd, data, &pos, worksize );
			} else {
				do{
					for (j=0; j<=strlen(usdic->yomi) ;j++){		// '\0'���܂߂�
						data[pos++] = usdic->yomi[j];
						UserDicFlash( fd, data, &pos, worksize );
					}
					for (j=0; j<=strlen(usdic->kouho) ;j++){	// '\0'���܂߂�
						data[pos++] = usdic->kouho[j];
						UserDicFlash( fd, data, &pos, worksize );
					}
					usdic = usdic->chain;
				}while (usdic);									// ���́u���v����������J��Ԃ�
				data[pos++] = 0;								// �u���v�Q�I��
				UserDicFlash( fd, data, &pos, worksize );
			}
		}
		UserDicFlash( fd, data, &pos, 0 );						// �o�b�t�@�̋��������o��
		sceIoClose(fd);
	}
	free(data);													// �o�b�t�@�����
}


//==============================================================
// �I������
//--------------------------------------------------------------
// �������ɕێ����Ă����������t�@�C���ɏ����߂�����A�J������B
// ��������s�����ɓd����؂铙������Ɗ������̕��בւ�������ɔ��f����܂���B
// �܂��AIME���f�[�^��ݒ�t�@�C���ɕۑ����܂��B
// �ݒ�t�@�C���̈ʒu�͎����t�@�C���Ɠ����t�H���_�ɂȂ�܂����A
// ������������Ȃ������ꍇ�� EBOOT.PBP �Ɠ����t�H���_�ɂȂ�܂��B
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
	UserDicSave();												// ���[�U�[�����ۑ�
	UserDicFree();												// ���[�U�[�����p�����������
	DicSave();													// ��{�����ۑ�
	free(gDic);
	DrawImgFree(gBackBuf);
	DrawImgFree(gInLnBuf);
	DrawImgFree(gkListBuf);
	gSetup = 0;
}


//==============================================================
// �����t�H���g�w��
//--------------------------------------------------------------
// font   �t�H���g���
// cyw    �t�H���g�̍����i16�܂Łj
// �߂�l �w�肷��O�̃t�H���g���
//--------------------------------------------------------------
// �ϊ��s�Ɏg�p���镶���t�H���g���w�肷��B
// �������e��E�B���h�D�ɕ\������镶���͓��_�t�H���g�ɌŒ�B
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
// �����J�[�\���w��
//--------------------------------------------------------------
// xw  �J�[�\����X�����̕��i0�ȉ��F�J�[�\���\���Ȃ��j
// yw  �J�[�\����Y�����̕�
// x,y �J�[�\���ʒu
//--------------------------------------------------------------
// �J�[�\�����W���w�肷��ϐ��ƃJ�[�\���`����w�肷��B
// �J�[�\���\����Sgets()�����s���Ă���ԂɍX�V����܂��B
// �J�[�\����I�^�ŁAY�����̑傫����SIMEfont()�Ŏw�肵�܂��B
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
// ��ʍ��ݒ�
//--------------------------------------------------------------
// page �����s����ʃy�[�W�i0��1�j
//--------------------------------------------------------------
// �����s����ʃy�[�W���w�肵�܂��B
// sceGuDrawBuffer()�Ƃ��̐ݒ�͍l�����Ă��܂���B
// VRAM�̐擪����c272�h�b�g���Ƀy�[�W0,1�ƌ��ߑł����Ă��܂��B
// ���C�����̍��d�l�ɂ���Ă͎g���Ȃ���������Ǝv���܂����A���̎��͓K����
// �C������ׂ��B
//--------------------------------------------------------------

void SIMESetDraw(int page)
{
	gPage = page * 272;
}


//==============================================================
// �L�[����
//--------------------------------------------------------------
// �߂�l 0:���͂Ȃ�
//        ��:0x1C ��:0x1D ��:0x1E ��:0x1F
//        ��:0x0D�i[Enter]�j �~:0x1B�i[Esc]�j ��:0x08�i[BS]�j ��:0x02 L:0x03 R:0x04
//        [START]:0x05 [SELECT]:0x06
//--------------------------------------------------------------
// �e�{�^���̏�Ԃ��L�[�{�[�h���ȃL�[���s�[�g����Ŏ擾����B
// ���������̏ꍇ�͍Ō�ɉ����������{�^���ɑΉ�����R�[�h��Ԃ��B
// ����͕������͂͂��Ȃ����ǁA�L�[���s�[�g����ŃL�[���͂��s�������ꍇ�Ȃǂ�
// �z�肵�ėp�ӂ��Ă���܂��B
//--------------------------------------------------------------

int SIMEgetchar(SceCtrlData pad1)
{
	static SceCtrlData	pad2;
	static int			rep = 0,kcode = 0,button = 0;

	if ((pad1.Buttons & PSP_CTRL_UP) && !(pad2.Buttons & PSP_CTRL_UP)){			// �����������u�Ԃ����o
		rep = 0;																// ���s�[�g�J�E���^���Z�b�g
		kcode = 0x1E;															// �o�̓R�[�h
		button = PSP_CTRL_UP;													// ���s�[�g���Ď�����{�^��
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
		if (rep>KEYREP1) rep -= KEYREP2;						// �L�[���s�[�g
		if (rep==1 || rep==KEYREP1) return (kcode);				// �����Ă���
	} else {
		rep = 0;
		kcode = 0;
		button = 0;
	}

	return (0);													// ���������Ă��Ȃ�
}


//==============================================================
// �e�L�X�g�g�̕\��
//--------------------------------------------------------------
// ���񂾃t���[���Ƀe�L�X�g��\�����܂��B
//--------------------------------------------------------------

static void DrawTextbox(char *str,int x,int y,int wx,long txcor,long bkcor)
{
	BoxFill( x, y, wx, 12+2, CORFL2, bkcor );
	HLine( x, y, wx-1, CORFL3 );								// ��
	VLine( x, y, 12+2-1, CORFL3 );								// ��
	mh_print( x+2, y+1, str, txcor );							// ����
}


//==============================================================
// �E�B���h�E�g�̕\��
//--------------------------------------------------------------

static void DrawWindow(char *title,int x,int y,int wx,int wy)
{
	y += gPage;

	BoxFill( x, y, wx-1, 12+3, CORFL1 , CORFL1 );
	HLine( x +2       , y +2 , wx-4   , CORFL3 );				// ��
	VLine( x +2       , y +2 , 12+2   , CORFL3 );				// ��
	HLine( x +2       , y +14, wx-4   , CORFL2 );				// ��
	VLine( x +2 +wx -4, y +2 , 12+2   , CORFL2 );				// �E
	mh_print( x +5, y +2, title, CORFCHR );						// �^�C�g��
	HLine( x       , y    , wx, CORFL2 );						// ��
	VLine( x       , y    , wy, CORFL2 );						// ��
	HLine( x       , y +wy, wx, CORFL3 );						// ��
	VLine( x +wx -1, y    , wy, CORFL3 );						// �E
}


//==============================================================
// �E�B���h�E�w�i�̑Ҕ�
//--------------------------------------------------------------

static void getBack(void)
{
	if (gBDFlag) return;

	if (gBackBuf){												// �o�b�t�@�����
		DrawImgFree(gBackBuf);
		gBackBuf = NULL;
	}

	switch (gKey){
	case 0:
	case 1:
		gBackBuf = DrawImgCreate( 210,98 +15 );					// �\�t�g�L�[�w�i�Ҕ��o�b�t�@
		gBackBufP.x = KBPOSX1;
		gBackBufP.y = KBPOSY1;
		BoxCopy( gBackBuf, gBackBufP.x, gPage+gBackBufP.y );
		break;
	case 2:
		gBackBufP.x = KBPOSX3;
		gBackBufP.y = KBPOSY3;
		gBackBuf = DrawImgCreate( 261,129 );					// �\�t�g�L�[�w�i�Ҕ��o�b�t�@
		BoxCopy( gBackBuf, gBackBufP.x, gPage+gBackBufP.y );
		break;
	}
}


//==============================================================
// �E�B���h�E�w�i�̕���
//--------------------------------------------------------------

static void putBack(void)
{
	if (gBDFlag) return;

	BoxPaste( gBackBuf, gBackBufP.x, gPage+gBackBufP.y );
}


//==============================================================
// �\�t�g�E�F�A�L�[�{�[�h�̕\���i�T�O���z��A�V�X�g�J�[�\���j
//--------------------------------------------------------------
// x[],y[]   �J�[�\���̓��B�ڕW�ʒu
// xx[],yy[] �J�[�\���̌��ʒu
// step      �J�[�\���̓��B�W���i0:�ڕW�ʒu�j
//--------------------------------------------------------------
// x��y��NULL�������ꍇ�̓J�[�\���͕\�����܂���B
//--------------------------------------------------------------

static void DrawKey1(int *x,int *y,int t,int *xx,int *yy,int step)
{
	char	key[3] = "  ";
	int		i,j,s,mx,my;
	long	cor[3] = { CORCUR1,CORCUR3,CORCUR4 };

	DrawWindow( gKeyName1[t],KBPOSX1,KBPOSY1,210,97 +15 );						// �E�B���h�E�t���[��
	if (t==0){
		mh_print( KBPOSX1 +210-2-3.5*13, gPage+KBPOSY1 +2, "��:�ϊ�", CORCUR2 );
	}
	BoxFill( KBPOSX1 +1, gPage+KBPOSY1 +15, 210-2, 97, CORFL1, CORIN );			// ����
	if (x!=NULL && y!=NULL){
		for (i=0; i<3 ;i++){													// �J�[�\���̕\��
			mx = (x[i] - xx[i]) *16 * step / CURSTEP;
			my = (y[i] - yy[i]) *16 * step / CURSTEP;
			DrawChar2( KBPOSX1 +1 + x[i]*16 -mx, gPage+KBPOSY1 +15 + y[i]*16 -my, i, cor[i] );
		}
	}
	if (t==2 || t>4){															// ���p�����̈ʒu�␳
		s = 3;
	}else{
		s = 0;
	}
	for (i=0; i<6 ;i++){
		for (j=0; j<13 ;j++){
			key[0] = gKeyTable1[t][(j*6+i)*2];
			key[1] = gKeyTable1[t][(j*6+i)*2+1];								// �L�[�}�b�v����ꕶ�����o��
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
// �\�t�g�E�F�A�L�[�{�[�h�̕\���i�~�T���ȕϊ��j
//--------------------------------------------------------------
// x��y���}�C�i�X�������ꍇ�̓J�[�\���͕\�����܂���B
//--------------------------------------------------------------

static void DrawKey3(int x,int y,int t)
{
	char key[3] = "  ";
	int i,j,k,s;
	int map[5][2] = {{1,0} , {0,1} , {1,1} , {2,1} , {1,2} };

	DrawWindow( gKeyName1[t],KBPOSX3,KBPOSY3,261,127 );						// �E�B���h�E�t���[��
	BoxFill(KBPOSX3 +1, gPage+KBPOSY3 +15, 261 -2, 112, CORFL1, CORIN);		// ����
	for (i=0; i<3 ;i++){													// �r��
		HLine(KBPOSX3 +1       , gPage+KBPOSY3 +15 + i*37, 261 -2, CORFL1);
	}
	for (i=0; i<6 ;i++){
		VLine(KBPOSX3 +1 + i*43, gPage+KBPOSY3 +15       , 112, CORFL1);
	}
	if (x>=0 && y>=0)
		BoxFill(KBPOSX3 +1 + x*43, gPage+KBPOSY3 +15 + y*37, 44, 38, CORFL1, CORCUR);

	if (t==2 || t>4){														// ���p�����̈ʒu�␳
		s = 3;
	}else{
		s = 0;
	}

	for (i=0; i<3 ;i++){													// �\�t�g�E�F�A�L�[�{�[�h
		for (j=0; j<6 ;j++){
			for (k=0; k<5 ;k++){
				key[0] = gKeyTable3[t][(i*30+j*5+k)*2];
				key[1] = gKeyTable3[t][(i*30+j*5+k)*2+1];					// �L�[�}�b�v����ꕶ�����o��
				mh_print(KBPOSX3 +1 + j*43 + map[k][0]*13+3 +s, gPage+KBPOSY3 +15 + i*37 + map[k][1]*12+1, key, CORWCHR);
			}
		}
	}
	if (t==0){
		mh_print(KBPOSX3 +1 +4*43 +2 +27, gPage+KBPOSY3 +15 +2*37+1*12+1, "��", CORSCHR);
	}
	mh_print(KBPOSX3 +1 +5*43 +2 +12, gPage+KBPOSY3 +15 +2*37+0*12+1, "Tab", CORSCHR);
	mh_print(KBPOSX3 +1 +5*43 +2    , gPage+KBPOSY3 +15 +2*37+1*12+1, "BS", CORSCHR);
	mh_print(KBPOSX3 +1 +5*43 +2 +24, gPage+KBPOSY3 +15 +2*37+1*12+1, "Ret", CORSCHR);
	mh_print(KBPOSX3 +1 +5*43 +2 + 6, gPage+KBPOSY3 +15 +2*37+2*12+1, "space", CORSCHR);
}


//==============================================================
// �ϊ����C���̈ʒu
//--------------------------------------------------------------
// gCx,gCy�Ŏw�肳��Ă���J�[�\���ʒu�ɕϊ����C����W�J�����ꍇ�̎��ۂ̍��W�ƃT�C�Y���v�Z�B
// ��ʒ[��\�t�g�L�[�ƂԂ���Ȃ�J�n�ʒu���C������B
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
	if (*x+*wx>480) *x = 480 - *wx;								// ��ʂ���͂ݏo���Ȃ�J�n�ʒu�����炷
	if (gCy+gCyw>cy && gMode==1){								// �\�t�g�L�[�Əd�Ȃ�Ȃ�J�n�ʒu�����炷
		if (*x+*wx>cx) *x = cx - *wx -gCxw;
	}
}


//==============================================================
// �ϊ����C���̕\��
//--------------------------------------------------------------
// blk �ϊ��Ώۂ̕�����
//--------------------------------------------------------------
// gCx,gCy�Ŏw�肳��Ă���J�[�\���ʒu�ɕϊ����C��������̕\�����s���B
// �\�t�g�L�[�Ɋ|����Ȃ��悤�Ɉʒu�␳����܂��B
// blk�Ŏw�肳�ꂽ�������������擪�����̔w�i�F��ς��܂��B
//--------------------------------------------------------------

static void InLine(char *str,int blk)
{
	char	buf[64];
	int	x,wx,wx1,wx2,blkLen;

	InLinePos( str,&x,&wx );									// �ϊ����C���ʒu
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
			BoxFill( x,gPage+gCy,wx1,gCyw,CORCHCU,CORCHCU );	// �����w�i�i�ϊ������ʁj
		}
		if (wx2){
			BoxFill( x+wx1,gPage+gCy,wx2,gCyw,CORCHBK,CORCHBK );	// �����w�i�i���ϊ����ʁj
		}
		pf_print2( x,gPage+gCy,gFont,str,CORCHCR );				// �ϊ����C��������
	}
}


//==============================================================
// �ϊ����C���̔w�i�̑Ҕ�
//--------------------------------------------------------------

static void getInLine(char *str)
{
	int	x,wx;

	if (gBDFlag) return;

	if (gInLnBuf){												// �o�b�t�@�����
		DrawImgFree(gInLnBuf);
		gInLnBuf = NULL;
	}

	InLinePos( str,&x,&wx );									// �ϊ����C���ʒu
	gInLnBuf = DrawImgCreate( wx,16 );
	gInLnBufP.x = x;
	gInLnBufP.y = gCy;
	BoxCopy( gInLnBuf, gInLnBufP.x, gPage+gInLnBufP.y );
}


//==============================================================
// �ϊ����C���̔w�i�̕���
//--------------------------------------------------------------

static void putInLine(void)
{
	if (gBDFlag) return;

	BoxPaste( gInLnBuf, gInLnBufP.x, gPage+gInLnBufP.y );
}


//==============================================================
// �����J�[�\���̍��
//--------------------------------------------------------------

static void DrawCursorSub()
{
	int x,cx,cy;

	if (gCxw==0 || gCyw==0) return;								// �J�[�\���̑傫����0

	if (gKey==2){												// �\�t�g�L�[�̈ʒu
		cx = KBPOSX3;
		cy = KBPOSY3;
	} else {
		cx = KBPOSX1;
		cy = KBPOSY1;
	}

	x = gCx;
	if (gCy+gCyw>cy && gMode==1){								// �\�t�g�L�[�Əd�Ȃ�Ȃ�J�n�ʒu�����炷
		if (x+gCxw>cx) x = cx - gCxw;
	}

	XFill( x, gPage+gCy, gCxw, gCyw, 0xFFFFFF );
}


//==============================================================
// �����J�[�\����\��
//--------------------------------------------------------------
// ch -1  �F�E�B���h�E�n�ƃJ�[�\��������
//    0   �F�J�[�\����_�ł�����
//    �ȊO�F�J�[�\��������/�\�����Ȃ�
//--------------------------------------------------------------
// SIMEgetchar()�P�̂ł̓J�[�\���\�����s���Ȃ����߁A�J�[�\����\���������ꍇ
// �ɂ�����g�p����B
// ��)
//   ch = SIMEgetchar(pad1);
//   SIMEDrawCursor(ch);
//
// �܂��A�L�[����Ƃ͕ʂɃE�B���h�E���̕\���n�����������ꍇ��-1���w�肷��B
//--------------------------------------------------------------

void SIMEDrawCursor(int ch)
{
	static int bk = 0,count = 0;

	if (ch==-1 && gMode){										// �E�B���h�E�n������
		putBack();
		if (gCount){
			putkList();
		}
		putInLine();
		gGetFlag = 1;
	}

	if (ch){
		if (bk && !gBDFlag) DrawCursorSub();					// �L�[���͂���������J�[�\��������
		bk = 0;
		count = CBTIME;											// ����͒����\��������
	} else {
		count++;
		if (count>CBTIME){										// �J�[�\���_�Ŏ���
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
// �ړ����[�h���̏���
//--------------------------------------------------------------

static void move(char *str,SceCtrlData pad1)
{
	int		ch;

	ch = SIMEgetchar(pad1);
	SIMEDrawCursor(ch);											// �J�[�\���̍��
	if (ch==0x02){												// �� �i�\�t�g�L�[���[�h�ցj
		ch = 0;
		gMode = 3;												// �\�t�g�L�[���[�h�J�ڏ���
	} else {
		str[0] = ch;
		str[1] = 0;
	}
}


//==============================================================
// �V�t�gJIS�̑�P�����`�F�b�N
//--------------------------------------------------------------
// �߂�l  0:��P�����ł͂Ȃ�
//        -1:��P�����ł���
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
// �����̓������C���f�b�N�X���擾
//--------------------------------------------------------------
// ���������͓��������ƂɌ��Q�𕪊��i85�ɕ�������j���ĊǗ����Ă��܂��B
//�i��{�����A���[�U�[�����Ƃ��Ɂj
// �w�肳�ꂽ�������ǂ̌��Q�ɑΉ����邩�𒲂ׂ܂��B
//--------------------------------------------------------------

static int dicIndex(char *str)
{
	unsigned int	cr;
	int				hp;

	if (chkSJIS(str[0])){										// �擪���S�p�����Ȃ�
		cr = fcode(str);										// �擪����
		if (cr<0x8200U){										// �L���Ȃ�
			hp = 0;
		} else if (cr<0x829FU){									// �����ƃA���t�@�x�b�g
			hp = -3;
		} else if (cr<0x82F2U){									// �Ђ炪�ȂȂ�
			hp = cr - 0x829FU +1;
		} else if (cr==0x8394U){								// �u���v
			hp = 84;
		} else {
			hp = -1;											// �����������s��Ȃ�
		}
	} else {													// �擪�����p�Ȃ�
		hp = -2;
	}
	return (hp);
}


//==============================================================
// ���[�U�[�������������������
//--------------------------------------------------------------
// count    �P����̐�
// wordList �P�ꃊ�X�g�ichar wordList[50][2][33]�̔z����w�肷�邱�Ɓj
// �߂�l   count   :�P�ꃊ�X�g�̌����i�P�`�j
//          wordList:�P�ꃊ�X�g
//--------------------------------------------------------------
// gSt�Ǝ����́u��݁v���r���A�s��v�������o�Ȃ�������Ƃ���B
//--------------------------------------------------------------

static void setUserKanji(char wordList[][2][33],int *count)
{
	struct strUsDic	*usdic;
	int		hp,pos;

	hp = dicIndex( gSt );										// �����̓������C���f�b�N�X���擾
	if (hp<0) return;											// �����o�^�ΏۊO�Ȃ̂ŏI��
	if (gUsDic[hp]==NULL) return;								// �o�^����Ă���u���v�������̂ŏI��

	usdic = gUsDic[hp];
	do{
		pos = 0;
		while (gSt[pos]!='\0'){
			if (gSt[pos]!=usdic->yomi[pos]) break;
			pos++;
		}
		if (gSt[pos]=='\0'){									// �u���v����
			strcpy( wordList[(*count)  ][0],usdic->yomi );		// �u��݁v��ŗD�揇�ʂ����ւ��鎞�Ɏg�p
			strcpy( wordList[(*count)++][1],usdic->kouho );		// �u���v
		}
		usdic = usdic->chain;
	}while (usdic && *count<KMAX-1);
}


//==============================================================
// ���[�U�[�����Ɍ���ǉ�����
//--------------------------------------------------------------
// ���[�U�[�����́u��݁v�u�P��v���P�Z�b�g�ɂ��ĒP�����`�F�C���\���ŊǗ����Ă��܂��B
// �u��݁v�̓��������ƂɌ��Q�𕪊����Č������̎�Ԃ��Ȃ��Ă܂��B
// ���������擾�o���Ȃ������ꍇ�͉������܂���B
// �ǉ����ꂽ/�ǉ����悤�Ƃ������͐擪�ʒu�ɒu�����̂ŁA�ϊ����ɂ͂悭�g��
// ���قǑO�̕��ɗ��܂��B
//--------------------------------------------------------------

//----- �������[�U�[�����ɒǉ����� -----

static void dicAddSub(int hp,char *yomi,char *kouho,struct strUsDic *usdic)
{
	struct strUsDic	*usdic2;

	usdic2 = (struct strUsDic*) malloc( sizeof(struct strUsDic) );
	if (usdic2){												// ���������擾�o�����Ȃ�
		usdic2->chain = usdic;
		strcpy( usdic2->yomi,yomi );
		strcpy( usdic2->kouho,kouho );
		gUsDic[hp] = usdic2;
	}
	gUSaveFlag = -1;											// ���[�U�[�����ɕω���������
}

//----- ���C�� -----

static void dicAdd(char *yomi,char *kouho)
{
	struct strUsDic	*usdic,*usdic2;
	int		hp,flag;

	hp = dicIndex( yomi );										// �����̓������C���f�b�N�X���擾
	if (hp<0) return;											// �����o�^�ΏۊO�Ȃ̂ŏI��

	if (gUsDic[hp]==NULL){										// �������ɑΉ�����u���v�Q�������̂ŐV�K�ǉ�
		dicAddSub( hp, yomi, kouho, NULL );						// �u���v�Q�͂����ŏI���
	} else {													// �������ɑΉ�����u���v�Q���L��ꍇ
		usdic = gUsDic[hp];
		usdic2 = NULL;
		flag = 0;
		do{
			if (strcmp(usdic->yomi,yomi)==0 && strcmp(usdic->kouho,kouho)==0){
				flag = 1;										// ���ɓo�^�ς݂�����
				break;
			}
			usdic2 = usdic;
			usdic = usdic->chain;								// ���́u���v��
		}while (usdic);
		if (flag){												// �o�^�ς݁u���v��擪�ʒu�Ɉړ�
			if (usdic2){										// �u���v��������Ȃ��ꍇ�͉������Ȃ�
				usdic2->chain = usdic->chain;
				usdic2 = gUsDic[hp];
				gUsDic[hp] = usdic;
				usdic->chain = usdic2;
				gUSaveFlag = -1;								// ���[�U�[�����ɕω���������
			}
		} else {												// �V�����u���v��擪�ʒu�ɒǉ�
			if (!gUDicAdd){										// ����ǉ�����ݒ�Ȃ�
				dicAddSub( hp,yomi, kouho, gUsDic[hp] );
			}
		}
	}
}


//==============================================================
// ���[�U�[������������폜����
//--------------------------------------------------------------
// �w�肳�ꂽ�u��݁v�u���v�̑g�ݍ��킹��������Ȃ����͉������܂���B
//--------------------------------------------------------------

static void dicDel(char *yomi,char *kouho)
{
	struct strUsDic	*usdic,*usdic2;
	int		hp,flag;

	hp = dicIndex( yomi );										// �����̓������C���f�b�N�X���擾
	if (hp<0) return;											// �����o�^�ΏۊO�Ȃ̂ŏI��

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
			usdic = usdic->chain;								// ���́u���v��
		}while (usdic);
		if (flag){												// �ړI�́u���v�𔭌�������
			if (usdic2){										// �Q�Ԗڈȍ~�̏ꍇ
				usdic2->chain = usdic->chain;
			} else {											// �ŏ��̈�ڂ̏ꍇ
				gUsDic[hp] = usdic->chain;
			}
			free(usdic);
			gUSaveFlag = -1;									// ���[�U�[�����ɕω���������
		}
	}
}


//==============================================================
// ���̓��͕������𒲂ׂ�
//--------------------------------------------------------------
// �߂�l                 �������̐�
// str[4][3]              ���Ɏg�p����Ă���p�x��������ʂQ�̕����Əꍇ�ɂ���Ắu�J�v
// wordList[KMAX][2][33] �u��݁v��⃊�X�g
// count                  �u��݁v��⃊�X�g�̌��i�ő�KMAX�j
//--------------------------------------------------------------
// ����܂łɕϊ��s�ɓ��͂��ꂽ���������Ɏ����𒲂ׂē��͒��Ǝv������̌��
// ���擾���������͂̃A�V�X�g���s���B
// �Ђ炪�Ȃ̓��͒��̏ꍇ�ɂ̂ݓ��삵�܂��B
// ���̂�݂Ɏg����p�x�̍�����ʂQ�́u���ȁv��I�����A�܂����O�ɓ��͂����
// ����u���ȁv�ɑ��������t���\�Ȃ�X�Ɂu�J�v��I�����܂��B
//--------------------------------------------------------------

static int nextChr(char str[][3],char wordList[][2][33],int *count)
{
	char	sTbl1[] = "�������������������������ĂƂ͂Ђӂւ�",
			sTbl2[] = "�������������������������Âłǂ΂тԂׂ�",
			sTbl3[] = "�ς҂Ղ؂�";
	unsigned int	cr,p[2];
	int		i,hp,sp,sp2,tmp,max[2],len,crCnt,cc[85];
	long	pos;

	cr = 0;

	if (chkSJIS(gSt[0])){										// �擪���S�p�����Ȃ�
		cr = fcode(&gSt[0]);									// �擪����
		if (cr<0x829FU){										// �L���A�����A�p��Ȃ�
			return (0);
		} else if (cr<0x82F2U){									// �Ђ炪�ȂȂ�
			hp = cr - 0x829FU +1;
		} else if (cr==0x8394U){								// �u���v
			return (0);
		} else {
			return (0);											// �����������s��Ȃ�
		}
	} else {													// �擪�����p�Ȃ�
		return (0);
	}

	pos = gHEDtbl[hp];											// �������ɑΉ����鎫�����R�[�h�̊J�n�ʒu
	if (pos==0xFFFFFFFF) return (0);							// �����ɓo�^�Ȃ�

	pos += 2;
	sp2 = 0;
	len = strlen(gSt);
	for (i=0; i<85 ;i++){
		cc[i] = 0;
	}
	while(1){
		sp = 0;
		while (gDic[pos+sp]!=0){								// ��݂ɑΉ�����P���������茟��
			if (gDic[pos+sp]!=gSt[sp]) break;
			sp++;
			if (sp==len) break;
		}
		if (sp==len){
			if (*count<KMAX){									// �u��݁v��⃊�X�g�̎擾
				wordList[*count][0][0] = '\0';					// ��{�����Ȃ̂Łu��݁v�͎g��Ȃ�
				strcpy( wordList[*count][1],&gDic[pos] );		// �u���v
				(*count)++;
			}
			cr = fcode(&gDic[pos+sp]);
			if (cr==0x815B){									// �u�[�v
				cr = 84;
			} else {
				cr -= 0x829F;
			}
			if (cr<85) cc[cr]++;								// ���̕������̊Y�������J�E���g
		}
		if (sp<sp2){											// ��v���镶�������O����Z���Ȃ�����T���I��
			break;
		}
		sp2 = sp;
		tmp = getInt(&gDic[pos-2]);								// ���̒P����ւ̑��΋����iint�j
		if (tmp==0xFFFF) break;									// �������I��������T���I��
		pos += tmp;
	}

	max[0] = max[1] = 0;
	for (i=0; i<85 ;i++){										// �o���p�x�̍����Q��T��
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
				p[i] = 0x815B;									// �u�[�v
			}
			str[i][0] = p[i] / 0x100;
			str[i][1] = p[i] & 0xFF;
			str[i][2] = '\0';
			if (strstr( sTbl2,str[i] )!=NULL){					// �u���v���u���v�ϊ�
				str[i][1]--;
			}
			if (strstr( sTbl3,str[i] )!=NULL){					// �u�ρv���u�́v�ϊ�
				str[i][1] -= 2;
			}
			crCnt++;
		}
	}

	if (len>=2){
		len -= 2;												// �ϊ��s�Ō�̕���
		if (strstr( sTbl1,&gSt[len] )!=NULL){					// �O������Ƃ����t�����������͂���Ă���Ȃ�
			str[crCnt][0] = 0x81;
			str[crCnt][1] = 0x4A;								// �u�J�v
			str[crCnt][2] = '\0';
			crCnt++;
		}
	}

	return (crCnt);
}


//==============================================================
// �J�[�\�����w�蕶���ʒu�Ɉړ�������
//--------------------------------------------------------------
// reset �J�[�\���ʒu�̏C������ 0:���̓A�V�X�g���� 1:�t�H�[���[�V�������Z�b�g 2:�A�V�X�g���Ȃ�
//--------------------------------------------------------------
// �A�V�X�g�J�[�\����p�B
// ��������ϊ�����T�����A�J�[�\���ʒu�̃A�V�X�g���s���B
//--------------------------------------------------------------

static void setCur(int *x,int *y,int bt,int *xx,int *yy,int reset,char wordList[][2][33],int *count)
{
	static int lock = 0;
	char	nchr[4][3];
	int		i,j,pos,flag,crCnt,zx,zy;
	int		dx[3] = {0,4,9},dy[3] = {0,2,4};

	for (i=0; i<3 ;i++){										// �ړ�����ۊ�
		xx[i] = x[i];
		yy[i] = y[i];
	}
	if (lock || reset){											// �J�[�\���t�H�[���[�V�����̃��Z�b�g
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
		setUserKanji( wordList,count );							// ���[�U�[��������
	}
	crCnt = nextChr( nchr,wordList,count );						// ���̓��͕������𒲂ׂ�
	if (crCnt){													// ������₪����Ȃ�
		for (j=0; j<crCnt ;j++){
			pos = -1;
			for (i=0; i<78 ;i++)
				if (gKeyTable1[0][i*2]==nchr[j][0] && gKeyTable1[0][i*2+1]==nchr[j][1]){
					pos = i;									// �����̈ʒu�����
					break;
				}
			if (pos!=-1){										// �����ʒu��Ή�������W�ɕϊ�
				zx = pos / 6;
				zy = pos % 6;
				flag = 1;
				for (i=0; i<3 ;i++){							// �d���`�F�b�N
					if (x[i]==zx && y[i]==zy) flag = 0;
				}
				if (flag && !reset){							// �d�����Ă��Ȃ��Ȃ�
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
// �ϊ��s�ɕ�����ǉ�����
//--------------------------------------------------------------
// �����ǉ����邾���ł͂Ȃ��A�������̕t�������Ȃǂ����Ă��܂��B
//--------------------------------------------------------------

static void addInline(char *str,char *si,int t)
{
	char	sdt[] = "�������������������������ĂƂ͂ЂӂւكJ�L�N�P�R�T�V�X�Z�\\�^�`�c�e�g�n�q�t�w�z",	// �u�J�v���t������
			sht[] = "�͂Ђӂւكn�q�t�w�z";																	// �u�K�v���t������
	char	cr[3] = "  ";
	int		pos,len;

	if (!chkSJIS(si[0])) si[1] = '\0';
	if (gSt[0]!=0){												// �Q�����ڈȍ~
		putInLine();											// �ϊ����C���w�i�𕜌�
		pos = strlen(gSt) - 2;
		cr[0] = gSt[pos];
		cr[1] = gSt[pos+1];
		if (strcmp( si,"�J" )==0){
			if ((unsigned char)cr[0]==0x83 &&
			    (unsigned char)cr[1]==0x45){					//�u�E�v���u���v�ϊ�
				gSt[pos+1] = 0x94;
				si[0] = 0;
				si[1] = 0;
			} else {
				if (strstr( sdt,cr )!=NULL){					// �u�J�v�̕t�������Ȃ�O�̕������C��
					gSt[pos+1]++;
					si[0] = 0;
					si[1] = 0;
				}
			}
		}else if (strcmp( si,"�K" )==0){
			if (strstr( sht,cr )!=NULL){						// �u�K�v�̕t�������Ȃ�O�̕������C��
				gSt[pos+1] += 2;
				si[0] = 0;
				si[1] = 0;
			}
		}
	} else {													// �P������
		if (t==2 || t>4 || si[0]==0x7F || (unsigned char)si[0]<32){	// ���p�̏ꍇ
			str[0] = si[0];										// ���̂܂܏o��
			str[1] = 0;
			si[0] = 0;
		}
	}

	if (si[1]){													// �S�p������ǉ�����ꍇ
		len = CHLEN -2;
	} else {													// ���p������ǉ�����ꍇ
		len = CHLEN -1;
	}
	if (strlen(gSt)<=len){										// �ϊ����C������������
		strcat( gSt,si );										// ������ǉ�
		getInLine(gSt);											// �ϊ����C���w�i�̎擾
	}
}


//==============================================================
// �\�t�g�L�[���[�h���̏����i�T�O���z��A�V�X�g�J�[�\���j
//--------------------------------------------------------------

static void softkey1(char *str,SceCtrlData pad1)
{
	static char	wordList[KMAX][2][33];
	static int	t = 0,bt = 0,pc = 0,index = 0,
				x[3] = {0,4,9},y[3] = {1,3,5},xx[3],yy[3];
	int		i,p,s,ch,fp,pos;
	char	si[3];

	fp = 0;														// ��ʂɕύX����������
	si[2] = 0;

	ch = SIMEgetchar(pad1);
	if (!strlen(gSt)){
		if (ch==0 || ch>=32 || ch==0x02 || ch==0x08 || ch==0x0D || ch==0x1B){
			SIMEDrawCursor(ch);									// �J�[�\���\��
		}
	}
	switch (ch){
	case 0x03:													// L
		break;
	case 0x04:													// R
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			t++;												// ������ύX
			if (t>6) t = 0;
		} else {
			setCur( x,y,bt,xx,yy,1,wordList,&gCount );			// �J�[�\���t�H�[���[�V�������f�t�H���g��
			pc = CURSTEP;
		}
		break;
	case 0x06:													// [SELECT]
		t--;													// ������ύX
		if (t<0) t = 6;
		break;
	case 0x05:													// [START]
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount){
				if (strlen(wordList[index][0])){				// [L]+[START] ���[�U�[�������獀�ڂ��폜
					dicDel( wordList[index][0], wordList[index][1] );
					si[0] = '\0';
					si[1] = '\0';
					fp = 1;
				}
			}
		} else {
			if (gSt[0]!=0){
				putInLine();									// �ϊ����C������U����
				if (gCount){
					putkList();
				}
			}
			gCount = 0;
			gMode = 5;											// �ݒ��ʂ�
		}
		break;
	case 0x02:													// ��
		setCur( x,y,bt,xx,yy,1,wordList,&gCount );				// �J�[�\���t�H�[���[�V�������f�t�H���g��
		pc = CURSTEP;
		if (gCount){
			putkList();
		}
		gCount = 0;
		if (gSt[0]==0){
			gMode = 0;
		} else {
			putInLine();										// �ϊ����C������U����
			gMode = 2;											// �����ϊ�
			strcpy( gYomi,gSt );								// �w�K�p�Ɂu��݁v��ۊǂ��Ă���
			gKouho[0] = '\0';									// �w�K�p�u���v���N���A���Ă���
		}
		break;
	case 0x08:													// ��
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]!=0){
				putInLine();									// �ϊ����C���w�i�𕜌�
				if (gCount){
					putkList();
				}
				pos = strlen(gSt);
				pos--;
				p = 0;
				while (1){										// �S�p�����␳
					s = 1 -chkSJIS(gSt[p]);
					if (p+s>pos) break;
					p += s;
				}
				gSt[p] = 0;
				if (s==2) gSt[p+1] = 0;
				getInLine(gSt);									// �ϊ����C���w�i���擾
				if (t==0){
					setCur( x,y,bt,xx,yy,0,wordList,&gCount );	// �J�[�\�������̕������ʒu��
				} else {
					setCur( x,y,bt,xx,yy,1,wordList,&gCount );	// �J�[�\�������̕������ʒu��
				}
				index = 0;
				getkList( wordList,gCount );
			} else {
				str[0] = ch;									// �o�b�N�X�y�[�X���o��
				SIMEDrawCursor(ch);
			}
		} else {
			si[0] = gKeyTable1[t][(x[1]*6+y[1]) *2];
			si[1] = gKeyTable1[t][(x[1]*6+y[1]) *2+1];
			bt = 1;
			fp = 1;
		}
		break;
	case 0x0D:													// ��
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount==0){
				setCur( x,y,bt,xx,yy,1,wordList,&gCount );		// �J�[�\���t�H�[���[�V�������f�t�H���g��
				pc = CURSTEP;
				if (gCount){
					putkList();
				}
				gCount = 0;
				if (gSt[0]==0){
					str[0] = ch;								// [Enter]���o��
					SIMEDrawCursor(ch);
				} else {
					putInLine();								// �ϊ����C���w�i�𕜌�
					for (i=0; i<CHLEN ;i++){
						str[i] = gSt[i];
						gSt[i] = 0;
					}
				}
			} else {
				if (gCount && gCount<=YOMILMAX){
					putInLine();								// �ϊ����C���w�i�𕜌�
					putkList();
					strcpy( str,wordList[index][1] );
					if (strlen(wordList[index][0])){			// ���[�U�[�����L�ڌ��Ȃ�
						dicAdd( wordList[index][0], wordList[index][1] );	// �L�ڈʒu���ŏ��Ɉړ�
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
	case 0x1B:													// �~
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			setCur( x,y,bt,xx,yy,1,wordList,&gCount );			// �J�[�\���t�H�[���[�V�������f�t�H���g��
			pc = CURSTEP;
			if (gCount){
				putkList();
			}
			gCount = 0;
			if (gSt[0]==0){
				gMode = 0;
			} else {
				putInLine();									// �ϊ����C���w�i�𕜌�
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
	case 0x1C:													// ��
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			} else {
				if (gCount && gCount<=YOMILMAX){
					putInLine();								// �ϊ����C���w�i�𕜌�
					putkList();
					strcpy( gSt,wordList[index][1] );
					getInLine(gSt);								// �ϊ����C���w�i���擾
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
	case 0x1D:													// ��
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
	case 0x1E:													// ��
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
	case 0x1F:													// ��
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
	default:													// ���̑��i�����̒��ړ��́H�j
		if (ch>=32){
			si[0] = ch;
			fp = 1;
		}
		break;
	}

	if (fp){													// ������ϊ��s�ɒǉ�
		if (gCount){
			putkList();
		}
		addInline( str,si,t );
		if (pad1.Buttons & PSP_CTRL_RTRIGGER || gKey==1){		// [R]�������Ă��邩�A�V�X�gOFF�Ȃ�
			setCur( x,y,bt,xx,yy,2,wordList,&gCount );			// �J�[�\���ʒu�͂��̂܂�
		} else {
			setCur( x,y,bt,xx,yy,0,wordList,&gCount );			// �J�[�\�������̕������ʒu��
		}
		getkList( wordList,gCount );
		pc = CURSTEP;
		index = 0;
	}

	if (str[0]){												// �������o�͂���ꍇ�̓E�B���h�E���ꎞ�I�ɏ���
		putBack();
		if (gCount){
			putkList();
		}
		gGetFlag = 1;
	} else {
		if (gGetFlag){											// �O��E�B���h�E���������Ă����i���C�����ō�悵�Ă邩���j�Ȃ�w�i���Ď擾
			getBack();
			getkList( wordList,gCount );
			getInLine(gSt);										// �ϊ����C���w�i���擾
			gGetFlag = 0;
		}
		if (gMode==1){											// �����[�h�ɑJ�ڂ���Ƃ��͎��s���Ȃ�
			InLine( gSt,0 );									// �ϊ����C���\��
			if (pad1.Buttons & PSP_CTRL_LTRIGGER){
				if (gCount<=YOMILMAX){
					kList( wordList,gCount,index );				// �u���ȁv��⃊�X�g�\��
				}
				DrawKey1( NULL,NULL,t,xx,yy,pc );				// �\�t�g�L�[�\��
			} else {
				if (gCount<=YOMILMAX){
					kList( wordList,gCount,-1 );				// �u���ȁv��⃊�X�g�\��
				}
				DrawKey1( x,y,t,xx,yy,pc );						// �\�t�g�L�[�\��
			}
			pc--;
			if (pc<0) pc = 0;
		} else {
			putBack();											// �\�t�g�L�[������
			if (gCount){
				putkList();
			}
		}
	}
}


//==============================================================
// �\�t�g�L�[���[�h���̏����i�~�T���Ȕz��j
//--------------------------------------------------------------

static void softkey3(char *str,SceCtrlData pad1)
{
	static char	wordList[KMAX][2][33];
	static int	x = 0,y = 0,t = 0,index = 0,
				xd[4],yd[4],xx[4],yy[4];
	int		i,p,s,ch,fp,pos;
	char	si[3];

	fp = 0;														// ��ʂɕύX����������
	si[2] = 0;

	ch = SIMEgetchar(pad1);
	if (!strlen(gSt)){
		if (ch==0 || ch>=32 || ch==0x02 || ch==0x08 || ch==0x0D || ch==0x1B){
			SIMEDrawCursor(ch);										// �J�[�\���\��
		}
	}
	switch (ch){												// �J�[�\���L�[����
	case 0x02:													// ��
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount){
				putkList();
			}
			gCount = 0;
			if (gSt[0]==0){
				gMode = 0;
			} else {
				putInLine();									// �ϊ����C������U����
				strcpy( gYomi,gSt );							// �w�K�p�Ɂu��݁v��ۊǂ��Ă���
				gKouho[0] = '\0';								// �w�K�p�u���v���N���A���Ă���
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
			t++;												// ������ύX
			if (t>6) t = 0;
		} else {
			si[0] = gKeyTable3[t][(y*30+x*5+2) *2];
			si[1] = gKeyTable3[t][(y*30+x*5+2) *2+1];
			fp = 1;
		}
		break;
	case 0x06:													// [SELECT]
		t--;													// ������ύX
		if (t<0) t = 6;
		break;
	case 0x05:													// [START]
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount){
				if (strlen(wordList[index][0])){				// [L]+[START] ���[�U�[�������獀�ڂ��폜
					dicDel( wordList[index][0], wordList[index][1] );
					si[0] = '\0';
					si[1] = '\0';
					fp = 1;
				}
			}
		} else {
			if (gSt[0]!=0){
				putInLine();									// �ϊ����C������U����
				if (gCount){
					putkList();
				}
			}
			gCount = 0;
			gMode = 5;											// �ݒ��ʂ�
		}
		break;
	case 0x08:													// ��
		if (pad1.Buttons & PSP_CTRL_LTRIGGER || gKeyTable3[t][(y*30+x*5+1) *2]=='\b'){
			if (gSt[0]!=0){
				putInLine();									// �ϊ����C���w�i�𕜌�
				if (gCount){
					putkList();
				}
				pos = strlen(gSt);
				pos--;
				p = 0;
				while (1){										// �S�p�����␳
					s = 1 -chkSJIS(gSt[p]);
					if (p+s>pos) break;
					p += s;
				}
				gSt[p] = 0;
				if (s==2) gSt[p+1] = 0;
				getInLine(gSt);									// �ϊ����C���w�i���擾
				if (t==0){
					setCur( xd,yd,0,xx,yy,0,wordList,&gCount );	// �J�[�\�������̕������ʒu��
				} else {
					setCur( xd,yd,0,xx,yy,1,wordList,&gCount );	// �J�[�\�������̕������ʒu��
				}
				index = 0;
				getkList( wordList,gCount );
			} else {
				str[0] = ch;									// �o�b�N�X�y�[�X���o��
				SIMEDrawCursor(ch);
			}
		} else {
			si[0] = gKeyTable3[t][(y*30+x*5+1) *2];
			si[1] = gKeyTable3[t][(y*30+x*5+1) *2+1];
			fp = 1;
		}
		break;
	case 0x0D:													// ��
		if (pad1.Buttons & PSP_CTRL_LTRIGGER || gKeyTable3[t][(y*30+x*5+3) *2]=='\r'){
			if (gCount==0){
				if (gCount){
					putkList();
				}
				gCount = 0;
				if (gSt[0]==0){
					str[0] = ch;								// [Enter]���o��
					SIMEDrawCursor(ch);
				} else {
					putInLine();								// �ϊ����C���w�i�𕜌�
					for (i=0; i<CHLEN ;i++){
						str[i] = gSt[i];
						gSt[i] = 0;
					}
				}
			} else {
				if (gCount && gCount<=YOMILMAX){
					putInLine();								// �ϊ����C���w�i�𕜌�
					putkList();
					strcpy( str,wordList[index][1] );
					if (strlen(wordList[index][0])){			// ���[�U�[�����L�ڌ��Ȃ�
						dicAdd( wordList[index][0], wordList[index][1] );	// �L�ڈʒu���ŏ��Ɉړ�
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
	case 0x1B:													// �~
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gCount){
				putkList();
			}
			gCount = 0;
			if (gSt[0]==0){
				gMode = 0;
			} else {
				putInLine();									// �ϊ����C���w�i�𕜌�
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
	case 0x1C:													// ��
		if (pad1.Buttons & PSP_CTRL_LTRIGGER){
			if (gSt[0]==0){
				str[0] = ch;
				SIMEDrawCursor(ch);
			} else {
				if (gCount && gCount<=YOMILMAX){
					putInLine();								// �ϊ����C���w�i�𕜌�
					putkList();
					strcpy( gSt,wordList[index][1] );
					getInLine(gSt);								// �ϊ����C���w�i���擾
					gCount = 0;
				}
			}
		} else {
			x++;
			if (x>5) x = 0;
		}
		break;
	case 0x1D:													// ��
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
	case 0x1E:													// ��
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
	case 0x1F:													// ��
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
	default:													// ���̑��i�����̒��ړ��́H�j
		if (ch>=32){
			si[0] = ch;
			fp = 1;
		}
		break;
	}

	if (fp){													// ������ϊ��s�ɒǉ�
		if (strcmp(si,"��")==0){								// �����ϊ��J�n
			if (gCount){
				putkList();
			}
			gCount = 0;
			if (gSt[0]==0){
				gMode = 0;
			} else {
				putInLine();									// �ϊ����C������U����
				strcpy( gYomi,gSt );							// �w�K�p�Ɂu��݁v��ۊǂ��Ă���
				gKouho[0] = '\0';								// �w�K�p�u���v���N���A���Ă���
				gMode = 2;
			}
		} else {
			if (gCount){
				putkList();
			}
			addInline( str,si,t );
			setCur( xd,yd,0,xx,yy,0,wordList,&gCount );			// �u��݁v��⃊�X�g�̎擾�̂ݎg�p
			getkList( wordList,gCount );
			index = 0;
		}
	}

	if (str[0]){												// �������o�͂���ꍇ�̓E�B���h�E���ꎞ�I�ɏ���
		putBack();
		if (gCount){
			putkList();
		}
		gGetFlag = 1;
	} else {
		if (gGetFlag){											// �O��E�B���h�E���������Ă����Ȃ�w�i���Ď擾
			getBack();
			getkList( wordList,gCount );
			getInLine(gSt);										// �ϊ����C���w�i���擾
			gGetFlag = 0;
		}
		if (gMode==1){											// �����[�h�ɑJ�ڂ���Ƃ��͎��s���Ȃ�
			InLine( gSt,0 );									// �ϊ����C���\��
			if (pad1.Buttons & PSP_CTRL_LTRIGGER){
				if (gCount<=YOMILMAX){
					kList( wordList,gCount,index );				// �u���ȁv��⃊�X�g�\��
				}
				DrawKey3( -1,-1,t );							// �\�t�g�L�[�\���i�J�[�\���Ȃ��j
			} else {
				if (gCount<=YOMILMAX){
					kList( wordList,gCount,-1 );				// �u���ȁv��⃊�X�g�\��
				}
				DrawKey3( x,y,t );								// �\�t�g�L�[�\��
			}
		} else {
			putBack();											// �\�t�g�L�[������
			if (gCount){
				putkList();
			}
		}
	}
}


//==============================================================
// �\�t�g�L�[���[�h���̏���
//--------------------------------------------------------------
// �Z���N�g����Ă���e�z��̏�����ɕ���
//--------------------------------------------------------------

static void keyboard(char *str,SceCtrlData pad1)
{
	switch (gKey){
	case 0:														// 50���z��A�V�X�g�J�[�\��
	case 1:														// 50���z��
		softkey1( str,pad1 );
		break;
	case 2:														// �~�T���Ȕz��
		softkey3( str,pad1 );
		break;
	default:
		gKey = 0;
		break;
	}
}


//==============================================================
// char�񂩂�S�p�����R�[�h���擾
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
// char�񂩂�int�f�[�^���擾
//--------------------------------------------------------------

static unsigned int getInt(char *str)
{
	return ((unsigned char)str[0] + (unsigned char)str[1] * 0x100);
}


//==============================================================
// �S�p������ʂ̐؂蕪��
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
		} else {												// ���p���������瑦�I��
			break;
		}
		if (cr<ar1 || cr>ar2){
			break;												// �͈͊O�̕�������������I��
		}
	}
	return (pos);
}


//==============================================================
// �����ϊ�����
//--------------------------------------------------------------
// len      �ϊ��ΏۂƂȂ镶�ߒ��i�o�C�g�j
// adjust      0:���ߒ��͎w��l�ɋ����Œ�
//          �ȊO:���ߒ��͎w��l���Z���ύX�����ꍇ����
// count    �P����̐�
// wordList �P�ꃊ�X�g�ichar wordList[KMAX][2][33]�̔z����w�肷�邱�Ɓj
//
// �߂�l   �T�����s�����u��݁v�̒����i�o�C�g�j
//          count   :�P�ꃊ�X�g�̌����i�P�`�j
//          wordList:�P�ꃊ�X�g
//--------------------------------------------------------------
// gSt�̓ǂ݂����Ɋ�{���������������������擾����B
// ��Ƀ��[�U�[�����̌�⌟�����s��ꂽ���ƂɃR�������s�����̂ŁA��������
// �o�^�ς݌��ɒǉ�����`�ɂȂ�܂��B
// wordList�̍Ō�ɂ͒T�����s�������ߒ��́u��݁v���̂��̂������Ă���B
// �����Ɉ�v������̂����������ꍇ�ł��Œ�ł��u��݁v�����͕Ԃ���܂��B
//--------------------------------------------------------------

static int setkanji(int len,int adjust,int *count,char wordList[][2][33])
{
	unsigned int	cr;
	const char		kazu[21] = {"�Z���O�l�ܘZ������"};
	int		i,j,hp,sp,sp2,sp3,tmp,top,sjis;
	long	pos,pos2;

	cr = 0;
	gDicWordPos = -1;

	hp = dicIndex( &gSt[0] );									// �����̓������C���f�b�N�X���擾

	if (hp<0){													// �����������s��Ȃ��ꍇ�i�����őΉ����Ă��Ȃ��擪�����j
		if (adjust){
			if (hp==-2){										// ���p�̕�����I��
				for (sp=0; sp<strlen(gSt) ;sp++){
					if (chkSJIS(gSt[sp])) break;
				}
			} else {											// ����̑S�p�����^�C�v���ɂ܂Ƃ߂�
				cr = fcode(&gSt[0]);							// �擪����
				if (cr<0x8259U){								// ����
					sp = codeChk( gSt,0x824F,0x8258 );
				} else if (cr<0x829BU){							// �A���t�@�x�b�g
					sp = codeChk( gSt,0x8260,0x829A );
				} else if (cr<0x8397U){							// �J�^�J�i
					sp = codeChk( gSt,0x8340,0x8396 );
				} else if (cr<0x83D7U){							// �M���V��
					sp = codeChk( gSt,0x839F,0x83D6 );
				} else if (cr<0x8492U){							// ���V�A
					sp = codeChk( gSt,0x8440,0x8491 );
				} else {										// �L��
					sp = codeChk( gSt,0x84A0,0x879F );
				}
			}
		} else {
			sp = len;											// �����^�C�v�͖���
		}

	} else {													// �����ϊ����s���ꍇ
		pos = gHEDtbl[hp];										// �������ɑΉ����鎫�����R�[�h�̊J�n�ʒu
		if (pos==0xFFFFFFFF){									// �����ɓo�^�Ȃ�
			sp = 2;
		} else {												// �����ɐ擪�������o�^����Ă���ꍇ
			pos += 2;
			sp2 = sp3 = 0;
			pos2 = -1;
			while(1){
				sp = 0;
				while (gDic[pos+sp]!=0){						// ��݂ɑΉ�����P���������茟��
					if (gDic[pos+sp]!=gSt[sp]) break;
					sp++;
					if (sp==len) break;
				}
				if (gDic[pos+sp]==0){							// �P���┭���i�����������ƒ�����v������̂����邩������Ȃ��̂ŒT���͏I���Ȃ��j
					pos2 = pos;
					sp3 = sp;
				}
				if (hp==0){										// �L���̏ꍇ�̏I������
					if (fcode(&gDic[pos])>0x829E){
						break;
					}
				} else {										// �Ђ炪�Ȃ̏ꍇ�̏I������
					if (sp<sp2){								// ��v���镶�������O����Z���Ȃ�����T���I��
						break;
					}
				}
				sp2 = sp;
				tmp = getInt(&gDic[pos-2]);						// ���̒P����ւ̑��΋����iint�j
				if (tmp==0xFFFF) break;							// �������I��������T���I��
				pos += tmp;
			}

			if (pos2==-1){										// �����ɍڂ��Ă��Ȃ��i�L����ϊ����悤�Ƃ����ꍇ�ɋN���肦��j
				sp = 2;
			} else if (adjust==0 && sp3!=len){					// �w�肳�ꂽ���ߒ��ɑ���Ȃ�
				sp = len;
			} else {											// �P�ꃊ�X�g����荞��
				sp = sp3;
				pos = pos2 + strlen(&gDic[pos2]) +1;
				gDicWordPos = pos;								// ��Ŏ����L�ڏ������ւ��邽�߂�
				top = *count;
				*count += gDic[pos++];							// �P�ꐔ�ibyte�j
				if (*count>KMAX-1) *count = KMAX-1;				// �P�ꐔ�����i�o�b�t�@�I�[�o�[�t���[�΍�j
				for (i=top; i<*count ;i++){						// �P���荞��
					tmp = strlen(&gDic[pos]);
					if (tmp>32) tmp = 32;						// �P��̒��������i�o�b�t�@�I�[�o�[�t���[�΍�j
					sjis = 0;
					for (j=0; j<tmp ;j++){
						wordList[i][1][j] = gDic[pos+j];
						if (sjis){								// ���������Ɉ������������ꍇ�ɃS�~���c�鋰�ꂪ����̂�
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

	if (hp>0 || hp==-3){										// ������ϊ�
		tmp = 0;
		for (i=0; i<sp ;i+=2){
			cr = fcode(&gSt[i]);
			if (cr>=0x824F && cr<0x8259){						// ��������������
				wordList[*count][1][i  ] = kazu[(cr-0x824F)*2  ];
				wordList[*count][1][i+1] = kazu[(cr-0x824F)*2+1];
				tmp = 1;
			} else if (cr>=0x829F && cr<0x82F2){
				if (cr>=0x82DE) cr++;							// sJIS�R�[�h�́u�~�v�Ɓu���v�̊Ԃɂ���󗓂͉����낤�H
				cr += 161;										// �Ђ炪�ȁ��J�^�J�i
				wordList[*count][1][i  ] = cr >> 8;
				wordList[*count][1][i+1] = cr & 0x00FF;
				tmp = 1;
			} else {											// ���̑��͂��̂܂�
				wordList[*count][1][i  ] = cr >> 8;
				wordList[*count][1][i+1] = cr & 0x00FF;
				tmp = 1;
			}
		}
		if (tmp){												// �R�[�h�ϊ����s���Ă����ꍇ��
			wordList[*count][1][i] = '\0';
			(*count)++;
		}
	}

	for (i=0; i<sp ;i++){										// �Ō�Ɂu��݁v���̂��̂�P�ꃊ�X�g�ɒǉ�
		wordList[*count][1][i] = gSt[i];
	}
	wordList[*count][1][i] = '\0';
	(*count)++;

	return (sp);
}


//==============================================================
// ������⃊�X�g�̈ʒu
//--------------------------------------------------------------
// �ϊ����C���̉�����Ɋ�����⃊�X�g�̈ʒu��ݒ肷��B
// ��ʒ[�ƂԂ���Ȃ������ɐݒ肵�܂��B
//--------------------------------------------------------------

static void kListPos(int *x,int *y,int *wx,int *wy,char wordList[][2][33],int count)
{
	int i,len,max,xl,yl,xx;

	InLinePos( gSt,x,wx );										// x���W�擾
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

	if (*x+*wx>480) *x = 480 - *wx;								// ��ʂ���͂ݏo���Ȃ�J�n�ʒu�����炷
	*y = gCy + gCyw+1;
	if (*y+*wy>272) *y = gCy -1 - *wy;

	if (gMode==1){												// �\�t�g�L�[���[�h�Ȃ�\�t�g�L�[���������
		if (gKey==2){
			xl = KBPOSX3;
			yl = KBPOSY3;
		} else {
			xl = KBPOSX1;
			yl = KBPOSY1;
		}
		if (*x+*wx>=xl && *y+*wy>=yl){							// �\�t�g�L�[�ɏd�Ȃ�
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
// ������⃊�X�g�̕\��
//--------------------------------------------------------------
// gCx,gCy�Ŏw�肳��Ă���J�[�\���ʒu�̉�����Ɋ�����⃊�X�g�̕\�����s���B
// index �� -1 �̂Ƃ��̓J�[�\����\�����Ȃ��B
//--------------------------------------------------------------

static void kList(char wordList[][2][33],int count,int index)
{
	int	i,page,len,pos,n,x,y,wx,wy,df;

	if (!count) return;

	kListPos( &x, &y, &wx, &wy, wordList, count );				// ������⃊�X�g�ʒu
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
	if (len>wy-2) len = wy -2;									// �X�N���[���o�[�̃T�C�Y�␳
	if (pos+len>wy-2) pos = wy-2 -len;							// �X�N���[���o�[�̈ʒu�␳
	for (i=0; i<4 ;i++){										// �X�N���[���o�[���
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
		mh_print( x+2,y,wordList[n][1],CORWCHR );				// �ϊ����C��������
		y += 12 +1;
		n++;
	}
}


//==============================================================
// ������⃊�X�g�̔w�i�̑Ҕ�
//--------------------------------------------------------------

static void getkList(char wordList[][2][33],int count)
{
	int	x,y,wx,wy;

	if (gBDFlag) return;

	if (!count){
		gkListBufP.x = -1;
		return;
	}
	if (gkListBuf){												// �o�b�t�@�����
		DrawImgFree(gkListBuf);
		gkListBuf = NULL;
	}
	kListPos( &x, &y, &wx, &wy, wordList, count );				// ������⃊�X�g�ʒu
	gkListBuf = DrawImgCreate( wx,wy );
	gkListBufP.x = x;
	gkListBufP.y = y;
	BoxCopy( gkListBuf, gkListBufP.x, gPage+gkListBufP.y );
}


//==============================================================
// ������⃊�X�g�̔w�i�̕���
//--------------------------------------------------------------

static void putkList(void)
{
	if (gkListBufP.x==-1) return;

	if (gBDFlag) return;

	BoxPaste( gkListBuf, gkListBufP.x, gPage+gkListBufP.y );
}


//==============================================================
// ��{�����L�ڏ��̓���ւ�
//--------------------------------------------------------------

static void dicWordEx(int index)
{
	long	pos,pos2;
	char	str[65];
	int		i,n,len;

	if (gDicWordPos==-1) return;								// �ʒu���w�肳��Ă��Ȃ�
	if (gDic[gDicWordPos]<(index+1)) return;					// �o�^�ꐔ�ȏ���w�肵�Ă���
	if (!index) return;											// ���ɐ擪�ʒu�ɂ���

	gSaveFlag = -1;

	pos = gDicWordPos +1;
	pos2 = pos;
	n = 0;
	while (n<index){											// �ڕW���̈ʒu��T��
		pos += strlen(&gDic[pos]) +1;
		n++;
	}
	strcpy(str,&gDic[pos]);										// �擪�Ɉړ����������Ҕ�
	len = strlen(&gDic[pos]) +1;								// �ړ�������T�C�Y
	n = pos - pos2;
	pos--;
	for (i=n; i>0 ;i--){										// ���̑��̌����ړ�
		gDic[pos+len] = gDic[pos];
		pos--;
	}
	strcpy(&gDic[pos2],str);									// �ڕW����擪�ɏ����߂�
}


//==============================================================
// �����I�����[�h
//--------------------------------------------------------------

static void change(char *str,SceCtrlData pad1)
{
	static char	wordList[KMAX][2][33],ILstr[64];
	static int	mode = 0,count,len,index,getFlag = 0;
	int	l,p,s,ch,df;

	if (!mode){													// �ϊ��J�n����̏����ݒ�
		count = 0;
		len = setkanji( strlen(gSt),1,&count,wordList );		// ��{��������
		index = 0;
		mode = 1;
		strcpy( ILstr,wordList[index][1] );						// �ϊ��Ώ�
		strcat( ILstr,&gSt[len] );								// ���ϊ��Ώ�
		getInLine(ILstr);										// ������Ԃ̕ϊ����C���w�i���擾
		getkList( wordList,count );
	}

	df = 0;

	ch = SIMEgetchar(pad1);
	switch (mode){
	case 1:														// �����I�����[�h
		switch (ch){
		case 0x03:												// L
			putkList();											// ������⃊�X�g������
			if (index==0){
				index = count -1;
			} else {
				index -= KLIST;
				if (index<0) index = 0;
			}
			df = 1;
			break;
		case 0x04:												// R
			putkList();											// ������⃊�X�g������
			if (index==count-1){
				index = 0;
			} else {
				index += KLIST;
				if (index>count-1) index = count -1;
			}
			df = 1;
			break;
		case 0x0D:												// ��
			strcat( gKouho,wordList[index][1] );				// �w�K�p�ɁA�I�����ꂽ����~�ς��Ă���
			strcpy( str,wordList[index][1] );
			strcpy( gSt,&gSt[len] );
			if (strlen(gSt)==0){								// �u��݁v���S�Ė����Ȃ�����\�t�g�L�[���[�h��
				if (!gUSave){
					dicAdd( gYomi,gKouho );						// ���[�U�[�����ɍ��ڂ�ǉ�����
				}
				gMode = 3;
			}
			if ((count-1)!=index){								// ��⃊�X�g�̍Ō�̌��́u��݁v�Ȃ̂œ���ւ������͍s��Ȃ�
				dicWordEx(index);
			}
			mode = 0;
			break;
		case 0x08:												// ��
		case 0x1B:												// �~
			putInLine();										// �ϊ����C��������
			putkList();											// ������⃊�X�g������
			mode = 0;
			gMode = 3;											// �\�t�g�L�[���[�h��
			break;
		case 0x1C:												// ��
			putInLine();										// �ϊ����C��������
			putkList();											// ������⃊�X�g������
			len += 1 -chkSJIS(gSt[len]);						// �S�p�����␳
			l = strlen(gSt);
			if (len>l) len = l;
			strcpy( ILstr,gSt );
			getInLine(ILstr);
			getkList( wordList,count );
			mode = 2;
			break;
		case 0x1D:												// ��
			putInLine();										// �ϊ����C��������
			putkList();											// ������⃊�X�g������
			len--;
			if (len<1) len = 1;
			p = 0;
			while (1){											// �S�p�����␳
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
		case 0x1E:												// ��
			putkList();											// ������⃊�X�g������
			index--;
			if (index<0) index = count -1;
			df = 1;
			break;
		case 0x1F:												// ��
		case 0x02:												// ��
			putkList();											// ������⃊�X�g������
			index++;
			if (index>count-1) index = 0;
			df = 1;
			break;
		}
		break;
	
	case 2:														// ���ߒ��I�����[�h
		switch (ch){
		case 0x03:												// L
			break;
		case 0x04:												// R
			break;
		case 0x0D:												// ��
			strncpy( str,gSt,len );
			str[len] = '\0';
			strcat( gKouho,str );								// �w�K�p�ɁA�I�����ꂽ����~�ς��Ă���
			strcpy( gSt,&gSt[len] );
			if (strlen(gSt)==0){								// �u��݁v���S�Ė����Ȃ�����\�t�g�L�[���[�h��
				putInLine();									// �ϊ����C��������
				if (!gUSave){
					dicAdd( gYomi,gKouho );						// ���[�U�[�����ɍ��ڂ�ǉ�����
				}
				gMode = 3;
			}
			mode = 0;
			break;
		case 0x08:												// ��
		case 0x1B:												// �~
			putInLine();										// �ϊ����C��������
			mode = 0;
			gMode = 3;											// �\�t�g�L�[���[�h��
			break;
		case 0x1C:												// ��
			len += 1 -chkSJIS(gSt[len]);						// �S�p�����␳
			l = strlen(gSt);
			if (len>l) len = l;
			break;
		case 0x1D:												// ��
			len--;
			if (len<1) len = 1;
			p = 0;
			while (1){											// �S�p�����␳
				s = 1 -chkSJIS(gSt[p]);
				if (p+s>len) break;
				p += s;
			}
			if (p==0) p = 2;
			len = p;
			break;
		case 0x1E:												// ��
			count = 0;
			len = setkanji( len,0,&count,wordList );
			index = count -1;
			mode = 1;
			df = 1;
			break;
		case 0x1F:												// ��
		case 0x02:												// ��
			count = 0;
			len = setkanji( len,0,&count,wordList );
			index = 0;
			mode = 1;
			df = 1;
			break;
		}
		break;
	}

	if (df){													// �ϊ����ɕω���������
		putInLine();											// �ϊ����C��������
		strcpy( ILstr,wordList[index][1] );						// �ϊ��Ώ�
		strcat( ILstr,&gSt[len] );								// ���ϊ��Ώ�
		getInLine(ILstr);
		getkList( wordList,count );
	}

	if (str[0]){												// �������o�͂���ꍇ�̓E�B���h�E���ꎞ�I�ɏ���
		putInLine();											// �ϊ����C��������
		putkList();												// ������⃊�X�g������
		getFlag = 1;
	} else {
		if (getFlag){											// �O��E�B���h�E���������Ă����Ȃ�w�i���Ď擾
			getInLine(ILstr);
			getkList( wordList,count );
			getFlag = 0;
		}
		if (gMode==2){											// �\�t�g�L�[���[�h�ɑJ�ڂ���Ƃ��͎��s���Ȃ�
			switch (mode){										// �ϊ����C����\��
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
// �ݒ�E�B���h�E�̍��
//--------------------------------------------------------------

static void DrawMenu(int pos,char *strKey,char *strSave,char *strUSave,char *strUDAdd)
{
	int		y;

	DrawWindow( SIMENAME,MEPOSX,MEPOSY,218,212 );				// �E�B���h�E�t���[��
	y = gPage + MEPOSY +15;
	BoxFill( MEPOSX +1, y, 218-2, 197, CORFL1, CORFR );			// ����
	y += 6;

	mh_print( MEPOSX +4 +4 , y, "�g�p���Ă��鎫��", CORFCHR );
	DrawTextbox( gDicFile,MEPOSX +4 +26,y +12+2,176,CORFCHR,CORFL1 );
	DrawTextbox( gDic2File,MEPOSX +4 +26,y +12+2+12+2,176,CORFCHR,CORFL1 );
	y += 48;

	if (pos==0)
		BoxFill( MEPOSX +4 +4 -1, y, 96 +2, 12, CORCUR, CORCUR );
	mh_print( MEPOSX +4 +4 , y, "�\\�t�g�L�[�̎��", CORFCHR );
	DrawTextbox( strKey,MEPOSX +4 +94,y +12+2,108,CORWCHR,CORIN );
	y += 36;

	if (pos==1)
		BoxFill( MEPOSX +4 +4 -1, y, 180 +2, 12, CORCUR, CORCUR );
	mh_print( MEPOSX +4 +4 , y, "�I�����Ɏ����̋L�ڏ���ۑ�����", CORFCHR );
	DrawTextbox( strSave,MEPOSX +4 +154,y +12+2,48,CORWCHR,CORIN );
	y += 36;

	if (pos==2)
		BoxFill( MEPOSX +4 +4 -1, y, 132 +2, 12, CORCUR, CORCUR );
	mh_print( MEPOSX +4 +4 , y, "���[�U�[�������g�p����", CORFCHR );
	DrawTextbox( strUSave,MEPOSX +4 +154,y +12+2,48,CORWCHR,CORIN );
	y += 36;

	if (pos==3)
		BoxFill( MEPOSX +4 +4 -1, y, 168 +2, 12, CORCUR, CORCUR );
	mh_print( MEPOSX +4 +4 , y, "���[�U�[�����Ɍ���ǉ�����", CORFCHR );
	DrawTextbox( strUDAdd,MEPOSX +4 +154,y +12+2,48,CORWCHR,CORIN );
}


//==============================================================
// �ݒ�E�B���h�E�w�i�̑Ҕ�
//--------------------------------------------------------------

static void getMenu()
{
	if (gBDFlag) return;

	if (gBackBuf){												// �o�b�t�@�����
		DrawImgFree(gBackBuf);
		gBackBuf = NULL;
	}
	gBackBuf = DrawImgCreate( 218,214 );						// �\�t�g�L�[�w�i�Ҕ��o�b�t�@
	gBackBufP.x = MEPOSX;
	gBackBufP.y = MEPOSY;
	BoxCopy( gBackBuf, gBackBufP.x, gPage+gBackBufP.y );
}


//==============================================================
// �e��ݒ�
//--------------------------------------------------------------

static void menu(SceCtrlData pad1)
{
	static int pos = 0,mode = 0,item[4];
	char	strKey[][21] = {"�T�O���z��AC","�T�O���z��","�~�T���Ȕz�u"},
			strSave[][7] = {"����","���Ȃ�"};
	int ch;

	ch = SIMEgetchar(pad1);
	switch (mode){
	case 0:														// �����ݒ�
		item[0] = gKey;
		item[1] = gSave;
		item[2] = gUSave;
		item[3] = gUDicAdd;
		pos = 0;
		mode = 1;
		break;

	case 1:														// �����I�����[�h
		switch (ch){
		case 0x03:												// L
			break;
		case 0x04:												// R
			break;
		case 0x0D:												// ��
			if (gKey!=item[0] || gSave!=item[1] || gUSave!=item[2]){
				gIni = 1;										// ���f�[�^���ύX���ꂽ
			}
			gKey = item[0];
			gSave = item[1];
			gUSave = item[2];
			gUDicAdd = item[3];
			mode = 0;
			gMode = 3;											// �\�t�g�L�[���[�h��
			break;
		case 0x08:												// ��
			break;
		case 0x05:												// [START]
		case 0x1B:												// �~
			mode = 0;
			gMode = 3;											// �\�t�g�L�[���[�h��
			break;
		case 0x1C:												// ��
			if (pos==0){
				item[pos]++;
				if (item[pos]>2) item[pos] = 2;
			} else {
				item[pos] = 1;
			}
			break;
		case 0x1D:												// ��
			item[pos]--;
			if (item[pos]<0) item[pos] = 0;
			break;
		case 0x1E:												// ��
			if (pos>0) pos--;
			break;
		case 0x1F:												// ��
			if (pos<3) pos++;
			break;
		case 0x02:												// ��
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
// ���������
//--------------------------------------------------------------
// *str   ���͂��ꂽ�����i�ő�33�o�C�g asciiz�j
// pad1   �p�b�h���
// �߂�l str���̂���
//--------------------------------------------------------------
// �\�t�g�L�[�{�[�h�ɂ�镶����̓��́B
// ���ȉp�������IME�ɂ�銿���ϊ��������s���B
// ���������łȂ��J�[�\���L�[���̊e�{�^�����R���g���[���R�[�h�Ƃ��ĕԂ��B
// ���͕�����́i�m�肳�ꂽ����������ꍇ�́jstr�ɓ����Ă��܂��B
//--------------------------------------------------------------

char *SIMEgets(char *str,SceCtrlData pad1)
{
	int i;

	for (i=0; i<=CHLEN ;i++){
		str[i] = 0;
	}

	switch (gMode){
	case 0:														// �ړ����[�h
		move( str,pad1 );
		break;

	case 1:														// �L�[�{�[�h���[�h
		keyboard( str,pad1 );
		break;

	case 2:														// �����I�����[�h
		change( str,pad1 );
		break;

	case 3:														// �\�t�g�L�[���[�h�ɑJ�ڂ���
		gMode = 1;
		getInLine(gSt);
		getBack();
		keyboard( str,pad1 );
		break;

	case 4:														// �e��ݒ���
		menu(pad1);
		break;

	case 5:														// �ݒ��ʂɑJ�ڂ���
		gMode = 4;
		getMenu();
		menu(pad1);
		break;

	}
	return (str);
}


//==============================================================
// �������͐؂�ւ�
//--------------------------------------------------------------
// ime ���̓\�[�X�i0:Simple IME �ȊO:OSK�j
// str ���͂��ꂽ������
// pad �p�b�h���
//--------------------------------------------------------------
// ���̓\�[�X��Simple IME��OSK�Ő؂�ւ��܂��B
// OSK�g�p���ɉ�ʃC���[�W��Ҕ�����o�b�t�@���m�ۂł��Ȃ������ꍇ��
// Simple IME���g�p����܂��B
//--------------------------------------------------------------

void SIMEselInput(int ime,char *str,SceCtrlData pad)
{
	DrawImg	*image;

	if (ime){													// OSK�I����
		image = DrawImgCreate(480,272);
		if (image){
			str[0] = SIMEgetchar(pad);
			SIMEDrawCursor(str[0]);								// SIME�J�[�\���̍��
			if (str[0]==0x02){									// ���������Ă���
				BoxCopy( image, 0, 0 );
				oskSetupGu();
				oskInput( "��������", "", str );
				initGraphics();
				flipScreen();									// �f�t�H���g�ł̓y�[�W�ݒ�ɕs�s��������̂�
				BoxPaste( image, 0, 0 );
			}
			if (image){
				DrawImgFree(image);
			}
		} else {												// �w�i�̑Ҕ����o���Ȃ��ꍇ
			SIMEgets( str, pad );
		}

	} else {													// Simple IME�I����
		SIMEgets( str, pad );									// ���͕�����̎擾
	}
}


