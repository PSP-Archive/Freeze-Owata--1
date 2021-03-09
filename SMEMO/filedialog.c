//==============================================================
// �t�@�C���_�C�A���O�֘A
// STEAR 2009,2010
//--------------------------------------------------------------
// �t�@�C���I���@�\�B
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
// �w�肳�ꂽ�f�B���N�g���̃G���g���ꗗ���擾����
//--------------------------------------------------------------
// �߂�l �擾�����G���g�����i�O�Ȃ疳���j
//--------------------------------------------------------------

static struct strDList* GetDir(char *DirName,int *count,struct strDList *DList)
{
	static SceIoDirent	dir;									// �������Ȃ��ƃt���[�Y����
	int 	dfd,ret,DListMax;

	free(DList);
	dfd = sceIoDopen(DirName);
	DListMax = 1;
	if (dfd>=0){
		do{														// �G���g���[�����J�E���g
			memset( &dir, 0, sizeof(dir) );						// �������Ȃ��ƃt���[�Y����
			ret = sceIoDread( dfd, &dir );
			if (strlen(dir.d_name)) DListMax++;
		}while (ret>0);
		sceIoDclose(dfd);
	}
	DList = (struct strDList*) malloc(sizeof(struct strDList) * DListMax);
	if (!DList) return (NULL);									// ���������m�ۏo���Ȃ�����
	DList[DListMax-1].name[0] = '\0';

	(*count) = 0;
	if (DListMax==1) return(0);									// �G���g���[������/�f�B���N�g��������

	dfd = sceIoDopen(DirName);
	do{
		DList[*count].name[0] = '\0';
		memset( &dir, 0, sizeof(dir) );							// �������Ȃ��ƃt���[�Y����
		ret = sceIoDread( dfd, &dir );
		if (strlen(dir.d_name)){								// ���O���������͖̂���
			if (dir.d_stat.st_attr & FIO_SO_IFDIR){
				if (strcmp(dir.d_name,".")!=0 && strcmp(dir.d_name,"..")!=0 ){
					strcpy( DList[*count].name, dir.d_name );
					DList[(*count)++].flag = 0;					// �f�B���N�g��
				}
			} else {
				strcpy( DList[*count].name, dir.d_name );
				DList[(*count)++].flag = 1;						// �t�@�C��
			}
		}
	}while (*count<DListMax && ret>0);
	sceIoDclose(dfd);
	return (DList);
}

//==============================================================
// �f�B���N�g���폜
//--------------------------------------------------------------
// path �폜���s���f�B���N�g���ւ̃p�X
//--------------------------------------------------------------
// �w�肳�ꂽ�f�B���N�g��������t�@�C�����܂߂č폜����B
// �����ɂ���T�u�f�B���N�g�����폜���܂��B
//--------------------------------------------------------------

static void removeDir(char *path)
{
	SceIoDirent	dir;
	SceUID		fd;
	char		file[1024];

	if ((fd = sceIoDopen(path)) < 0) return;					// �w��f�B���N�g�����J���Ȃ������̂ŏI��
	memset(&dir, 0, sizeof(dir));								// ���������Ȃ���read�Ɏ��s����
	while (sceIoDread(fd, &dir) > 0){
		if(strcmp(dir.d_name, ".") == 0) continue;
		if(strcmp(dir.d_name, "..") == 0) continue;
		strcpy( file, path );
		strcat( file, "/" );
		strcat( file, dir.d_name );
		if (dir.d_stat.st_attr & FIO_SO_IFDIR){					// �f�B���N�g�����H�t�@�C�����H
			removeDir(file);									// �f�B���N�g���i�ċA�����ŃT�u�f�B���N�g������������j
		} else {
			sceIoRemove(file);									// �t�@�C��
		}
	}
	sceIoDclose(fd);
	sceIoRmdir(path);
}

//==============================================================
// �t�@�C���I���̉�ʍ��
//--------------------------------------------------------------
// flag 1:�������
//--------------------------------------------------------------
// ��悷��K�v�̖����ꍇ�͍�悵�Ȃ��B
// �P��ʂ�14�s�\���B
//--------------------------------------------------------------

static void DrawSFsub(struct strDList *DList,int y,int pos)
{
	char	str[256];

	if (s2ch.cfg.hbl){											// HBL�����ł̓t�@�C������UTF8���V�t�gJIS�ϊ����s��
		psp2chUTF82Sjis(str, DList[pos+y].name);
	} else {
		strcpy(str, DList[pos+y].name);
	}
	str[63] = '\0';												// �ȈՓI�ȕ���������
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
	if (pos!=pos2){												// ����/�J�[�\���̍폜
		Fill( 40+10, 40+6+12+4, 380-6, 168, gSCor[5] );
	} else if (y!=y2){
		Fill( 40+10, 40+6+12+4+y2*12, 380-6, 12, gSCor[5] );
	}

	if ((max && y!=y2) || pos!=pos2){							// �G���g��������Ȃ�J�[�\����\��
		CurveBox( 40+10, 40+6+12+4+y*12, 380-6, 12, 0, gSCor[10], gSCor[11] );
	}

	if (pos!=pos2){												// ��ʑS�̂��ĕ`��
		pos2 = pos;
		y2 = y;
		for (i=0; i<14 ;i++){
			if (pos+i>=max) break;
			DrawSFsub( DList, i, pos );
		}
	} else if (y!=y2){											// �J�[�\���ʒu���ĕ`��
		DrawSFsub( DList, y2, pos2 );							// �O��̃J�[�\���ʒu
		DrawSFsub( DList, y, pos );								// ����̃J�[�\���ʒu
		y2 = y;
	}
}

//==============================================================
// �t�@�C���I��
//--------------------------------------------------------------
// InitDir  �����f�B���N�g���ʒu
// FileName �f�t�H���g�t�@�C�����i�t�@�C�����̎���͎��̃f�t�H���g���j
// dir      �I�����ꂽ�t�@�C�����i�t���p�X�j
// flag      0:�t�@�C���I���i�t�@�C���ǂݍ��ݗp�j
//          -1:�t�@�C�������́i���O��t���ĕۑ��p�j
// ime      ���̓\�[�X�i0:Simple IME  �ȊO:OSK�j
// �߂�l    0:����I���i�t�@�C�����I�����ꂽ�j
//          -1:�L�����Z�����ꂽ
//--------------------------------------------------------------
// �f�B���N�g���Ԃ��ړ����t�@�C����I������B
// ������G���g�����̓������̋�������ł��B
// �L�����Z�����ꂽ�ꍇ�Adir�̓��e�͕ω����Ȃ��B
// �t�@�C����I�������ꍇ�AInitDir�Adir�ɒl���Z�b�g����܂��B
// �_�C�A���O�w�i�͕������Ȃ��̂ŁA�K�v�ɉ����ČĂяo�����ōĕ`�ʂ��s���Ă��������B
//--------------------------------------------------------------

//----- ���݂̃p�X��\�� -----

static void DrawSFPath(char *path)
{
	char	str[64];
	int		pos,len;

	pos = 0;
	len = strlen(path);
	if (len>61){
		pos = len - 61;
	}
	strcpy( str, &path[pos] );									// �p�X������61���������o��

	Fill( 40+10, 25+6+12+4, 380-6, 12, gSCor[5] );
	mh_print( 40+11, 25+6+12+4, str, gSCor[0] );
}

//----- �_�C�A���O�̍\�z -----

static void DrawSFRe(char *title,int y,int pos,int count,struct strDList *DList,char *DirName)
{
	CurveBox( 40, 25, 400, 227, 4, gSCor[1], gSCor[2] );
	mh_print( 40+10, 25+6, title, gSCor[1] );
	mh_print( 40+400-10-54*6, 25+227-6-12, "[START]:̧�ٍ폜  ��:�I��  �~:��ݾ�  ��:�t�@�C��������", gSCor[1] );
	DrawSFPath( DirName );
	DrawSF( DList, y, pos, count, 1 );
}

//----- �㏑���`�F�b�N -----

static int DrawSFFile(char *DirName,char *FileName)
{
	char	path[1024];
	int		fd;

	strcpy( path,DirName );
	strcat( path,FileName );
	fd = sceIoOpen( path, PSP_O_RDONLY, 0777 );
	sceIoClose(fd);
	if (fd>=0){
		if (!DialogYN( "�w�肳�ꂽ�t�@�C���͊��ɑ��݂��Ă��܂��B\n�㏑�����܂�����낵���ł����H", gSCor[3], gSCor[4] ))
			return (0);
	}
	return (-1);
}

//----- ���[�N�������̎擾�Ɏ��s -----

static void memerr(void)
{
	SceCtrlData		pad;
	DiaBox1( -1, 110, 0, "���[�N�������̎擾�Ɏ��s���܂���", gSCor[0], gSCor[3], gSCor[4] );
	ScreenView();												// ��ʍX�V
	while (1){
		sceCtrlReadBufferPositive(&pad, 1);
		if (!(pad.Buttons & 0x00F3F9)) break;					// ����n�{�^�����S�ė����ꂽ
		sceDisplayWaitVblankStart();
	}
	while (!gExitRequest){										// �����������܂őҋ@
		sceCtrlReadBufferPositive( &pad, 1 );
		if (SIMEgetchar(pad)!=0) break;
		sceDisplayWaitVblankStart();
	}
}

//----- ���C������ -----

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
	DrawSFRe( title, y, pos, count, DList, DirName );			// �_�C�A���O�\�z

	if (flag){													// �t�@�C��������
		if (s2ch.cfg.hbl){										// HBL�����ł̓t�@�C������UTF8���V�t�gJIS�ϊ����s��
			psp2chUTF82Sjis(str, FileName);
		} else {
			strcpy(str, FileName);
		}
		if (!InputName( "�t�@�C��������͂��Ă�������",50,ime,str,outName,inphis)){
			if (s2ch.cfg.hbl){									// HBL�����ł̓t�@�C�����̃V�t�gJIS��UTF8�ϊ����s��
				psp2chSJIS2UCS(ucs, outName, 1024);
				psp2chUCS2UTF8(outName, ucs);
			}
			if (DrawSFFile(DirName,outName)){					// �㏑���x��
				strcpy( dir, DirName );
				strcat( dir, outName );
				free(DList);									// �t�@�C�����X�g��p��
				return (0);										// ���Ō��肳�ꂽ�ꍇ
			}
		}
		DrawSF( DList, y, pos, count, 1 );						// ��ʍX�V
	}

	padp = 0;													// �A�i���O�p�b�h�̉��������Ď�(�������̂�)
	padv = 0;													// �A�i���O�p�b�h�ɂ��ړ��ʒ[��(�c�����̂�)
	while (!gExitRequest && !err){								// �L�[���̓��C�����[�v
		sceCtrlReadBufferPositive(&pad, 1);
		ret = SIMEgetchar(pad);
		if (pad.Lx<20 && !padp){								// �A�i���O�p�b�h ��
			padp = 1;
			ret = SIME_KEY_LEFT;
		} else if (pad.Lx>256-20 && !padp){						// �A�i���O�p�b�h ��
			padp = 1;
			ret = SIME_KEY_RIGHT;
		} else if (pad.Lx>128-40 && pad.Lx<128+40){				// �j���[�g�����`�F�b�N
			padp = 0;
		}
		if (pad.Ly<128-40 || pad.Ly>128+40){					// �A�i���O�p�b�h ����
			padv += (float)(pad.Ly - 128) / 400;
			if (padv<-1){										// ��
				padv = padv + 1;
				if (pos>0){										// ���X�g�S�̂���փX�N���[��
					pos--;
					DrawSF( DList, y, pos, count, 0 );
				}
			}
			if (padv>1){										// ��
				padv = padv - 1;
				if (pos<count-14){								// ���X�g�S�̂����փX�N���[��
					pos++;
					DrawSF( DList, y, pos, count, 0 );
				}
			}
		}
		switch (ret){
		case SIME_KEY_CIRCLE:									// ��
		case SIME_KEY_RIGHT:									// ��
			if (strlen(DList[pos+y].name)!=0){
				if (DList[pos+y].flag){							// �t�@�C���̏ꍇ
					if (ret==SIME_KEY_CIRCLE){					// �t�@�C���I���͂����܂Ō���L�[
						if (!flag || DrawSFFile(DirName,DList[pos+y].name)){	// �㏑���x��
							strcpy( InitDir, DirName );
							strcpy( dir, DirName );
							strcat( dir, DList[pos+y].name );
							free(DList);						// �t�@�C�����X�g��p��
							return (0);							// �t�@�C���I��
						} else {
							DrawSF( DList, y, pos, count, 1 );	// ��ʍX�V�i�x���_�C�A���O�̏����j
						}
					}
				} else {										// �f�B���N�g���̏ꍇ
					strcat( DirName, DList[pos+y].name );
					strcat( DirName, "/" );
					DrawSFPath( DirName );
					DList = GetDir( DirName, &count, DList );	// �f�B���N�g���ꗗ�̎擾
					if (DList){
						pos = 0;
						y = 0;
						DrawSF( DList, y, pos, count, 1 );		// ��ʍX�V
					} else {
						err = -1;
					}
				}
			}
			break;
		case SIME_KEY_CROSS:									// �~
			free(DList);										// �t�@�C�����X�g��p��
			return (-1);
			break;
		case SIME_KEY_TRIANGLE:									// ��
			if (s2ch.cfg.hbl){									// HBL�����ł̓t�@�C������UTF8���V�t�gJIS�ϊ����s��
				psp2chUTF82Sjis(str, FileName);
			} else {
				strcpy(str, FileName);
			}
			if (!InputName( "�t�@�C��������͂��Ă�������",50,ime,str,outName,inphis)){
				if (s2ch.cfg.hbl){								// HBL�����ł̓t�@�C�����̃V�t�gJIS��UTF8�ϊ����s��
					psp2chSJIS2UCS(ucs, outName, 1024);
					psp2chUCS2UTF8(outName, ucs);
				}
				if (!flag || DrawSFFile(DirName,outName)){		// �㏑���x��
					strcpy( InitDir, DirName );
					strcpy( dir, DirName );
					strcat( dir, outName );
					free(DList);								// �t�@�C�����X�g��p��
					return (0);									// ���Ō��肳�ꂽ�ꍇ
				}
			}
			DrawSF( DList, y, pos, count, 1 );					// ��ʍX�V
			break;
		case 0x08:
		case SIME_KEY_LEFT:										// ��
			p1 = strchr( DirName, '/' );
			p2 = strrchr( DirName, '/' );
			if (p1!=p2){										// ���݈ʒu�����[�g�������ꍇ�͏������Ȃ�
				len = strlen(DirName);
				DirName[len-1] = '\0';							// �Ō����'/'���폜
				p2 = strrchr( DirName, '/' );					// ��Ԍ��ɂ���'/'�̈ʒu
				strcpy( backDir, &p2[1] );						// �������܂ŋ����t�H���_����ۑ�
				p2[1] = '\0';									// ��Ԍ��ɂ���'/'�̈ʒu�Ȍ���폜
				DrawSFPath( DirName );
				DList = GetDir( DirName, &count, DList );		// �f�B���N�g���ꗗ�̎擾
				if (DList){
					pos = 0;
					y = 0;
					for (i=0; i<count ;i++){					// �������܂ŋ����t�H���_���̏��ɃJ�[�\�����Z�b�g
						if (!strcmp( backDir,DList[i].name )){
							if (i<13){
								y = i;
							} else {
								y = 13;
								pos = i - 13;
							}
						}
					}
					DrawSF( DList, y, pos, count, 1 );			// ��ʍX�V
				} else {
					err = -1;
				}
			}
			break;
		case SIME_KEY_UP:										// ��
			if (y>0){
				y--;
			} else if (pos>0) pos--;
			DrawSF( DList, y, pos, count, 0 );
			break;
		case SIME_KEY_DOWN:										// ��
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
			if (strlen(DList[pos+y].name)!=0){					// �t�@�C��/�f�B���N�g��������Ȃ�
				strcpy( str, DList[pos+y].name );
				if (strlen(str)>63) str[64] = '\0';
				strcat( str, " ���폜���܂��B" );
				if (!DList[pos+y].flag){						// �f�B���N�g���̏ꍇ
					strcat( str,"\n���ӁF���̃t�H���_�̓��e�͑S��������܂��B" );
				}
				if (DialogYN( str, gSCor[1], gSCor[2] )){		// Yes�Ȃ�
					strcpy( str, DirName );
					strcat( str, DList[pos+y].name );
					if (!DList[pos+y].flag){					// �f�B���N�g���̏ꍇ
						removeDir( str );						// �f�B���N�g���폜
					} else {
						sceIoRemove( str );						// �t�@�C���폜
					}
					DList = GetDir( DirName, &count, DList );	// �f�B���N�g���ꗗ�̎擾
					if (pos+y > count){							// �J�[�\���ʒu�␳
						if (y>0){
							y--;
						} else {
							if (pos>0) pos--;
						}
					}
				}
				if (DList){
					DrawSF( DList, y, pos, count, 1 );		// ��ʍX�V
				} else {
					err = -1;
				}
			}
			break;
		}
		sceDisplayWaitVblankStart();
		ScreenView();											// ��ʍX�V
	}
	free(DList);												// �t�@�C�����X�g��p��
	return (-1);
}

