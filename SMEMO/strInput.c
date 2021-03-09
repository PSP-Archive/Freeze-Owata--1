//==============================================================
// ��������͊֘A
// STEAR 2009
//--------------------------------------------------------------
// �ėp�P���C����������́B
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

#define	LISTMAX		(20)										// ���͗����ɕۊǂ���ő�l
#define	STRMAX		(128)										// �����������̍ő�


//==============================================================
// ���͗����̑S�폜
//--------------------------------------------------------------
// inphis ���͗���
//--------------------------------------------------------------
// ���͗���p�̃�������������܂��B
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
// ���͗����̓ǂݍ���
//--------------------------------------------------------------
// path   ���͗����t�@�C��
// �߂�l ���͗����ւ̃|�C���^
//--------------------------------------------------------------
// ���͗���p�̃��������m�ۂ��A�w��t�@�C�����̗������Z�b�g����B
// �t�@�C�����ǂ߂Ȃ������ꍇ�̓����������m�ۂ��܂��B
// ����œ��͗���p�̃��������m�ۂ��Ȃ��Ɠ��͗����@�\���g���܂���B
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
	data = (char*) malloc( filesize );							// �t�@�C���T�C�Y�͂�������1000�o�C�g���x�Ȃ̂�
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
		inphis2->save = 0;										// ���ꂪ1�ɂȂ�����ύX����
		if (!inphis3){											// �ŏ��̃��X�g����
			*inphis = inphis2;
		} else {												// �Q�Ԗڈȍ~�̃��X�g����
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
// ���͗����̕ۑ�
//--------------------------------------------------------------
// path   ���͗���ۑ��t�@�C����
// inphis ���͗���
//--------------------------------------------------------------
// ���e�ɕύX���������ꍇ�̂ݕۑ����܂��B
//--------------------------------------------------------------

void InputHisSave(char *path,strInpHis **inphis)
{
	strInpHis	*inphis2;
	int		fd;

	if (!inphis) return;
	if (!*inphis) return;

	inphis2 = *inphis;
	if (!inphis2->save) return;									// �����ɕύX���Ȃ�
	fd = sceIoOpen( path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777 );
	if (fd<0) return;
	while (inphis2){
		sceIoWrite( fd, inphis2->text, strlen(inphis2->text)+1 );
		inphis2 = inphis2->chain;
	}
	sceIoClose(fd);
}

//==============================================================
// ���͗����Ƀ��X�g��ǉ�
//--------------------------------------------------------------
// �������e���������ꍇ�͓�d�o�^�͂��Ȃ��B
// �擪�ʒu�ɓo�^����܂��B
// ���X�g�̐���LISTMAX�𒴂����ꍇ�͈�ԌÂ����X�g���폜����܂��B
// ���������擾�o���Ȃ������ꍇ�͉������܂���B
//--------------------------------------------------------------

//----- ���X�g�ɒǉ� -----

static void InputHisAddSub(strInpHis **inphis2,char *text)
{
	strInpHis	*inphis;

	inphis = (strInpHis*) malloc( sizeof(strInpHis) );
	if (inphis){
		inphis->text = (char*) malloc( strlen(text)+1 );
		if (!inphis->text){										// ���e�p�������̊m�ۂɎ��s
			free(inphis);
		} else {												// ����Ƀ��������m�ۂł���
			strcpy( inphis->text,text );
			inphis->chain = *inphis2;
			inphis->save = 1;									// �������ύX���ꂽ
			*inphis2 = inphis;
		}
	}
}

//----- ���C�� -----

void InputHisAdd(strInpHis **inphis,char *text)
{
	strInpHis	*inphis2,*inphis3;
	int		count;

	if (!inphis) return;
	if (strlen(text)==0) return;

	if (!*inphis){												// �ŏ��̈���
		InputHisAddSub( inphis, text );
		return;
	}

	inphis2 = *inphis;
	inphis3 = NULL;
	while (inphis2){											// �������e�����邩
		if (!strcmp(inphis2->text,text)) break;
		inphis3 = inphis2;
		inphis2 = inphis2->chain;
	}
	if (inphis2){												// �������e��������
		if (inphis3){											// �ʒu���Q�Ԗڈȍ~�Ȃ�
			inphis3->chain = inphis2->chain;					// �`�F�C������O����
			inphis3 = *inphis;
			*inphis = inphis2;
			inphis2->save = 1;									// �����ɕύX��������
			inphis2->chain = inphis3;							// ��ԍŏ��ֈړ�������
		}
	} else {													// �V�K�ǉ�
		InputHisAddSub( inphis, text );
		count = 0;
		inphis2 = *inphis;
		while (inphis2 && count<LISTMAX){						// ���X�g�̌��𐔂���
			count++;
			inphis3 = inphis2;
			inphis2 = inphis2->chain;
		}
		if (inphis2){											// �������𒴂��Ă���
			inphis3->chain = NULL;								// ��폜
			free(inphis2->text);
			free(inphis2);
		}
	}
}

//==============================================================
// ���͗������烊�X�g���ڂ��擾
//--------------------------------------------------------------
// text   �擾���ꂽ���X�g���e�i���͗����j
// size   text�̃T�C�Y
// inphis ���͗���
// index  ���o�����X�g�̈ʒu
// �߂�l �擾���ꂽ���X�g�̎��ۂ̈ʒu�i�擪�ʒu��0�j
//--------------------------------------------------------------
// �����������ꍇ��text�̓��e�ɕω��͂���܂���B
// ���̎��̖߂�l��-1�ł��B
// �����̃��X�g���𒴂����ʒu���w�肳�ꂽ�ꍇ�A�Ō�̃��X�g���e���Ԃ���܂��B
//--------------------------------------------------------------

int InputHisGet(char *text,int size,strInpHis **inphis,int index)
{
	strInpHis	*inphis2;
	int		i,pos;

	if (!inphis) return (-1);
	if (!*inphis) return (-1);
	if (index<0) return (-1);

	inphis2 = *inphis;
	for (i=0; i<index ;i++){									// �w��ʒu��T��
		if (!inphis2->chain) break;								// ���X�g���s����
		inphis2 = inphis2->chain;
	}
	for (pos=0; pos<size-1 ;pos++){								// �o�b�t�@�I�[�o�[�t���[�΍�
		text[pos] = inphis2->text[pos];
	}
	text[pos] = '\0';
	return (i);
}


//==============================================================
// �S�p/���p���肠��ňꕶ��������
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

	//----- �\���ʒu�␳ -----

	if (*spos>pos){
		*spos = pos - 6;
		if (*spos<0) *spos = 0;
		for (i=0; i<*spos; i++){								// �S�p�����␳
			if (chkSJIS(str[i])) i++;
		}
		*spos = i;
	}

	return (pos);
}

//==============================================================
// ���������
//--------------------------------------------------------------
// title   �^�C�g��������
// size    ���͗��̕������i�ő�50�����܂Łj
// ime     ���̓\�[�X�i0:Simple IME �ȊO:OSK�j
// inStr   ���͕�����̏����l
// outStr  �������͌��ʁi128�o�C�g�j
// inphis  ���͗���
// �߂�l     0:���{�^���ŏI��
//         �ȊO:�~�{�^���ŃL�����Z�����ꂽ
//--------------------------------------------------------------
// �ėp�P���C�����͂ł��B
// �_�C�A���O�ʒu�͎w��ł��܂���B
// ���{�^���ŏI�������ꍇ�AoutStr�ɓ��͕����񂪕Ԃ����B
// �~�{�^���ŏI�������ꍇ�AoutStr�ɂ�inStr���Ԃ����B
// �_�C�A���O�̔w�i�͕������Ȃ��̂ŁA�K�v�ɉ����ČĂяo�����ōĕ`�ʂ��s���Ă��������B
// ���͗��̕��𒴂��镶�����̏ꍇ�͉��X�N���[�����܂��B
//--------------------------------------------------------------

int InputName(char *title,int size,int ime,char *inStr,char *outStr,strInpHis **inphis)
{
	SceCtrlData pad;
	char	str[STRMAX],cr[128],text[51];
	int		i,cx,cy,x,pos,spos,spos2,len,p,dr,end,font,index;

	if (size<2) size = 2;
	if (size>50) size = 50;

	font = SIMEfont( 0, 12 );									// SIME�̃t�H���g�𓌉_�t�H���g��

	//----- �_�C�A���O�\�z -----

	DialogFrame( 78, 70, 24+300, 57, title, "����:���͗���  ��:����  �~:��ݾ�", gSCor[1], gSCor[2] );
	x = 78 + 324/2 - (size*6+2)/2;								// ���͗��ʒu
	BoxFill(  x, 91, size*6+2, 14, 0, 0 );
	x += 1;
	cy = 92;
	strcpy( str, inStr );										// �������̏����l
	len = strlen(str);
	pos = len;
	if (pos>=size){												// ���͘g�𒴂��镶�����̏ꍇ
		spos = pos - 50;
	} else {
		spos = 0;
	}
	memcpy( text, &str[spos], size );
	text[size] = '\0';
	mh_print( x, cy, text, gSCor[0] );

	//----- ���͏��� -----

	end = 1;
	index = -1;
	while (!gExitRequest && end){
		cx = x + (pos - spos) * 6;
		SIMEcursor( 2, cx, cy );								// �����J�[�\���̈ʒu�w��
		sceCtrlReadBufferPositive( &pad,1 );
		SIMEselInput( ime, cr, pad );
		if (cr[0]!=0){											// ���͂�������
			dr = 0;												// ����������������邩�i0:No�j
			if (cr[0]<32U || cr[0]==0x7F){						// �R���g���[���R�[�h�������ꍇ
				switch(cr[0]){
				case SIME_KEY_CIRCLE:							// �� [Enter]
					InputHisAdd( inphis,str );
					end = 0;
					break;
				case SIME_KEY_CROSS:							// �~ [ESC]
					end = 0;
					break;
				case SIME_KEY_UP:								// ���i���͗����j
					if (index>-1) index--;
					index = InputHisGet( str, STRMAX, inphis, index );
					if (index>=0){
						len = strlen(str);
						pos = len;
						if (pos>=size){							// ���͘g�𒴂��镶�����̏ꍇ
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
				case SIME_KEY_DOWN:								// ���i���͗����j
					index++;
					index = InputHisGet( str, STRMAX, inphis, index );
					if (index>=0){
						len = strlen(str);
						pos = len;
						if (pos>=size){							// ���͘g�𒴂��镶�����̏ꍇ
							spos = pos - 50;
						} else {
							spos = 0;
						}
						dr = 1;
					}
					break;
				case SIME_KEY_LEFT:								// ��
					spos2 = spos;
					pos = INBack( str, pos, &spos );
					if (spos2!=spos) dr = 1;					// �����\���ʒu���ύX���ꂽ
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
				case SIME_KEY_RIGHT:							// ��
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
						for (i=0; i<spos; i++){					// �S�p�����␳
							if (chkSJIS(str[i])) i++;
						}
						spos = i;
					}
					if (spos2!=spos) dr = 1;					// �����\���ʒu���ύX���ꂽ
					break;
				}
			} else {											// �����������ꍇ
				p = 0;
				while (cr[p]!='\0'){
					if (chkSJIS(cr[p])){						// �S�p����
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
					} else if (cr[p]>=32U){						// ���p����
						if (len<=STRMAX-1-1){
							for (i=0; i<len-pos ;i++)
								str[len-1+1-i] = str[len-1-i];
							str[pos++] = cr[p++];
							len += 1;
							dr = 1;
						} else {
							break;
						}
					} else {									// �R���g���[���R�[�h�͖���
						p++;
					}
				}
				str[len] = '\0';
				if (spos+size<pos){
					spos = pos - size + 6;
					if (spos>STRMAX-size) spos = STRMAX - size;
					for (i=0; i<spos; i++){						// �S�p�����␳
						if (chkSJIS(str[i])) i++;
					}
					spos = i;
				}
			}
			if (dr){											// ������̍X�V
				BoxClr( x, cy, size*6, 12 );
				memcpy( text, &str[spos], size );
				text[size] = '\0';
				mh_print( x, cy, text, gSCor[0] );
			}
		}

		sceDisplayWaitVblankStart();
		ScreenView();
	}

	SIMEfont(font,(font==2 ? 16 : 12));							// SIME�t�H���g�����ɖ߂�

	//----- �߂�l -----

	if (cr[0]==0x0D){											// [Enter]�ŏI�����Ă�����
		strcpy( outStr, str );
		return (0);
	} else {
		strcpy( outStr, inStr );
		return (-1);
	}
}

