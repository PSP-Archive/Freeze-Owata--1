//==============================================================
// �t�@�C���_�C�A���O�֘A
// STEAR 2009
//--------------------------------------------------------------

#ifndef FileDialog_H
#define FileDialog_H

//----- �\���� -----

struct strDList{												// �f�B���N�g���ꗗ�p�\����
	int		flag;												// 0:�f�B���N�g�� 1:�t�@�C��
	char	name[256];											// ���O
};

//----- �v���g�^�C�v�錾 -----

int		SelectFile(char *title,char *InitDir,char *FileName,char *dir,int flag,int ime,strInpHis **inphis);

#endif
