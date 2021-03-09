/*
* $Id: psp2ch.c 157 2008-09-16 23:13:56Z bird_may_nike $
*/

#include "psp2ch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psputility.h>
#include <psprtc.h>
#include "psp2chIta.h"
#include "psp2chThread.h"
#include "psp2chRes.h"
#include "psp2chFavorite.h"
#include "psp2chSearch.h"
#include "psp2chMenu.h"
#include "psp2chNet.h"
#include "psp2chIni.h"
#include "utf8.h"
#include "pg.h"
#include "charConv.h"
#include "psp2chAudio.h"
#include "oniguruma.h"
#include "smemo/strInput.h"
#include "smemo/sime.h"
#include "smemo/graphics.h"
	
extern int favSortType; // psp2chFavorite.c
extern int threadSortType; // psp2chThread.c
extern const char* colorFile; // psp2chMenu.c
extern Window_Layer *winParam;

S_2CH s2ch;
char keyWords[128];
const char searchWords[] = "searchHistory.dat";

// prototype
int psp2chInputKana(char* buf, unsigned int max);	// psp2chForm.c

static void psp2chStart(void);


/*********************************
���C�����[�v
running���^�̊ԃ��[�v�i�~�{�^���̏I����home�{�^���ł̏I����running��0�j
�Z���N�^�[(sel)�Ŋe�֐��֕���
���X�\���͂��C�ɓ���A�ꗗ�A���X�\���������N�W�����v�A�S�������ʂ��烉�b�p�[�֐��ֈړ�
�ړ����ɖ߂邽��retSel���g�p�i���X�\�������"�߂�"��sel = retSel���s�j
�S�������ړ����ɖ߂邽��retSel�g�p
*********************************/
int psp2ch(void)
{
    int retSel = 0, sel;

    while (s2ch.running)
    {
    	sceCtrlPeekBufferPositive(&s2ch.pad, 1);
		sel = s2ch.sel;
        switch (s2ch.sel)
        {
        case 1:
            psp2chFavorite();
            retSel = 1;
            break;
        case 2:
            psp2chIta();
            retSel = 2;
            break;
        case 3:
            psp2chThread(retSel);
            retSel = 3;
            break;
        case 4:
            retSel = psp2chFavoriteRes(retSel);
            break;
        case 5:
            retSel = psp2chThreadRes(retSel);
            break;
        case 6:
            retSel = psp2chJumpRes(retSel);
            break;
        case 7:
            psp2chSearch(retSel);
            retSel = 7;
            break;
        case 8:
            retSel = psp2chSearchRes(retSel);
            break;
        default:
            psp2chStart();
            break;
        }
        s2ch.oldPad = s2ch.pad;
        // ��ʍX�V
        flipScreen(0);
		if (s2ch.sel != sel) pgReDraw();						// ��ʂ��ς��ꍇ�͍��_�u���o�b�t�@�𓯈��
    }
    return 0;
}

/*****************************
�X�^�[�g���
*****************************/
static void psp2chStartColorSet(void)
{
	if (s2ch.cfg.colorSet){
		psp2chMenuColor(NULL);									// psp2chMenu.c����
		while (s2ch.running){
			sceCtrlPeekBufferPositive(&s2ch.pad, 1);
			if (!(s2ch.pad.Buttons & PSP_CTRL_CROSS)) break;
			flipScreen(0);
		}
	}
}

static void psp2chStart(void)
{
	pgFillvram(FORM_INDEX + 3, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
	pgSetDrawStart(0, 0, 0, 0);
	pgPrint("Freeze Owata+1 based Freeze Owata(by KMA) rev 100331", RED, WHITE, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH*2);
	pgPrint("Freeze Owata based owata browser(by toyo) rev 161", WHITE, WHITE, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("used libraries: Many thanks!!", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("  libCat Copyright (c) 2008 Sofiya�L", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("  libjpeg�@Copyright (c) 1991-1998, Thomas G. Lane.", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("  zlib�@Copyright (c) 1995-2004 Jean-loup Gailly and Mark Adler.", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("  libpng�@Copyright (c) 1998-2004 Glenn Randers-Pehrson.", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("  libungif 4.1.4 Copyright (c) 1997  Eric S. Raymond", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("  �S�� 5.9.0 Copyright (c) 2002-2006  K.Kosako", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("  �S�� 5.9.0 for PSP Copyright (c) 2007 NIPS", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("  unzip Copyright (c) 1998-2004 Gilles Vollant", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH*2);
	pgPrint("PSP������ Ver1.10�� By.STEAR 2009-2011", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH*2);
	pgPrint("START: ", GREEN, BLACK, SCR_WIDTH);
	pgSetDrawStart(50, -1, 0, 0);
	pgPrint("view category list", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("CIRCLE: ", GREEN, BLACK, SCR_WIDTH);
	pgSetDrawStart(50, -1, 0, 0);
	pgPrint("favorite thread list", WHITE, BLACK, SCR_WIDTH);
	pgSetDrawStart(0, -1, 0, LINE_PITCH);
	pgPrint("CROSS: ", GREEN, BLACK, SCR_WIDTH);
	pgSetDrawStart(50, -1, 0, 0);
	pgPrint("exit", WHITE, BLACK, SCR_WIDTH);
    if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
    {
        if(s2ch.pad.Buttons & PSP_CTRL_CROSS)
        {
            if (psp2chOwata())
            {
                return;
            }
        }
        else if(s2ch.pad.Buttons & PSP_CTRL_START)
        {
            s2ch.sel = 2;
			psp2chStartColorSet();
            return;
        }
        else if(s2ch.pad.Buttons & PSP_CTRL_CIRCLE)
        {
            s2ch.sel = 1;
			psp2chStartColorSet();
            return;
        }
    }
    pgCopy(0, 0);
}

/*****************************
�I�����
*****************************/
int psp2chOwata(void)
{
    if (psp2chErrorDialog(1, TEXT_3) == PSP_UTILITY_MSGDIALOG_RESULT_YES)
    {
        winParam->tateFlag = 0;
        pgPrintOwata();
        pgCopy(0, 0);
        flipScreen(0);
        //sceKernelDelayThread(DISPLAY_WAIT);
        s2ch.running = 0;
        return 1;
    }
    return 0;
}

/*****************************
�J�[�\���ړ����[�`��
S_2CH_SCREEN�\���̂�
start:�\���J�n�s
select:�J�[�\���I���s
��ύX��R�{�^������Ԃ�
*****************************/
int psp2chCursorSet(S_2CH_SCREEN* line, int lineEnd, int shift, int pad, int* change)
{
    static int keyStart = 0, keyRepeat = 0;
    static clock_t keyTime = 0;
    int rMenu, start, select;
    int padUp = 0, padDown = 0;

	*change = 0;
	rMenu = s2ch.pad.Buttons & shift;
    if (winParam->tateFlag)
    {
        if (s2ch.pad.Lx == 255)
        {
            padUp = 1;
        }
        else if (s2ch.pad.Lx == 0)
        {
            padDown = 1;
        }
    }
    else
    {
        if (s2ch.pad.Ly == 0)
        {
            padUp = 1;
        }
        else if (s2ch.pad.Ly == 255)
        {
            padDown = 1;
        }
    }

	start = line->start;
	select = line->select;
	if (padUp)
	{
        line->select--;
		if (line->start)
		{
			line->start--;
		}
	}
	else if (padDown)
	{
        line->select++;
		if (line->start < (line->count - lineEnd))
		{
			line->start++;
		}
	}

    if (s2ch.pad.Buttons != s2ch.oldPad.Buttons || keyRepeat)
    {
        if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
        {
            keyStart = 1;
        }
        else
        {
            keyStart = 0;
        }
        keyTime = sceKernelLibcClock();
        keyRepeat = 0;
        if (s2ch.pad.Buttons & s2ch.listB[winParam->tateFlag].up)
        {
            line->select--;
            if (line->select < line->start)
            {
                line->start = line->select;
            }
        }
        else if (s2ch.pad.Buttons & s2ch.listB[winParam->tateFlag].down)
        {
            line->select++;
            if (line->select >= line->start + lineEnd)
            {
                line->start = line->select - lineEnd + 1;
            }
        }
        else if (s2ch.pad.Buttons & s2ch.listB[winParam->tateFlag].pUp)
        {
            if (line->select == line->start)
            {
                line->start -= lineEnd;
            }
            line->select = line->start;
        }
        else if (s2ch.pad.Buttons & s2ch.listB[winParam->tateFlag].pDown)
        {
            if (line->select == line->start + lineEnd - 1)
            {
                line->start += lineEnd;
            }
            line->select = line->start + lineEnd - 1;
        }
        // top, end
        if (s2ch.pad.Buttons & s2ch.listB[winParam->tateFlag].top)
        {
            if (rMenu && !padUp)
            {
                line->start = 0;
                line->select = 0;
            }
        }
        else if (s2ch.pad.Buttons & s2ch.listB[winParam->tateFlag].end)
        {
            if (rMenu && !padDown)
            {
                line->start = line->count - lineEnd;
                line->select = line->count - 1;
            }
        }
    }
    else
    {
        if (keyStart)
        {
            if (sceKernelLibcClock() - keyTime > 300000)
            {
                keyRepeat = 1;
            }
        }
        else
        {
            if (sceKernelLibcClock() - keyTime > 1000)
            {
                keyRepeat = 1;
            }
        }
    }
    if (pad)
    {
    	int ret;
    	ret = psp2chPadSet(winParam->viewX);
    	if (ret != winParam->viewX)
    	{
    		winParam->viewX = ret;
    		*change = 1;
    	}
    }
    
    // �͈͊O�̕␳
    if (line->start < 0) {
    	line->start = 0;
    }
    else if (line->start + lineEnd >= line->count) {
    	line->start = line->count - lineEnd;
    }
    if (line->select < 0) {
    	line->select = 0;
    }
    else if (line->select >= line->count) {
    	line->select = line->count - 1;
    }
    
	if (start != line->start || select != line->select)
	{
		*change = 1;
	}
    return rMenu;
}

/*****************************
�A�i���O�p�b�h�ŉ��X�N���[��
s2ch.cfg.xReverse��-1�ɂ���ƃX�N���[�����������]����
*****************************/
int psp2chPadSet(int scrollX)
{
    if (winParam->tateFlag)
    {
        if (s2ch.pad.Ly < 127 - s2ch.cfg.padCutoff *2)
        {
            scrollX += 8 * s2ch.cfg.padReverse * ((s2ch.pad.Ly == 0) ? 2 : 1);
        }
        else if (s2ch.pad.Ly > 127 + s2ch.cfg.padCutoff *2)
        {
            scrollX -= 8 * s2ch.cfg.padReverse * ((s2ch.pad.Ly == 255) ? 2 : 1);
        }
        if (scrollX > BUF_HEIGHT * 2 - SCR_HEIGHT)
        {
            scrollX = BUF_HEIGHT * 2 - SCR_HEIGHT;
        }
    }
    else
    {
        if (s2ch.pad.Lx < 127 - s2ch.cfg.padCutoff *2)
        {
            scrollX += 4 * s2ch.cfg.padReverse * ((s2ch.pad.Lx == 0) ? 2 : 1);
        }
        else if (s2ch.pad.Lx > 127 + s2ch.cfg.padCutoff *2)
        {
            scrollX -= 4 * s2ch.cfg.padReverse * ((s2ch.pad.Lx == 255) ? 2 : 1);
        }
        if (scrollX > BUF_WIDTH * 2 - SCR_WIDTH)
        {
            scrollX = BUF_WIDTH * 2 - SCR_WIDTH;
        }
    }
    if (scrollX < 0)
    {
        scrollX = 0;
    }
    return scrollX;
}

/***********************************
���W���[���̃��[�h
������
************************************/
int psp2chInit(void)
{
	const char *sBtnH[] = {"Sel", "", "", "St", "��", "��", "��", "��", "L", "R", "", "", "��", "��", "�~", "��", ""};
	const char *sBtnV[] = {"Sel", "", "", "St", "��", "��", "��", "��", "L", "R", "", "", "��", "��", "�~", "��", ""};
	char	path[FILE_PATH],*data;
	SceUID	fd;
	int		saveflag;
	long	i,filesize;

    psp2chIniLoadConfig();
    psp2chIniSetColor((char*)colorFile);
    psp2chIniSetButtons();
    psp2chItaSetMenuString(sBtnH, sBtnV);
    psp2chFavSetMenuString(sBtnH, sBtnV);
    psp2chThreadSetMenuString(sBtnH, sBtnV);
    psp2chSearchSetMenuString(sBtnH, sBtnV);
    psp2chResSetMenuString(sBtnH, sBtnV);
    psp2chMenuSetMenuString(sBtnH, sBtnV);
    psp2chMenuFontSet(0);
    s2ch.running = 1;
    s2ch.sel = 0;
    s2ch.urlAnchorCount = s2ch.resAnchorCount = s2ch.idAnchorCount = s2ch.beAnchorCount = s2ch.numAnchorCount = 0;
    s2ch.categoryList = NULL;
    s2ch.itaList = NULL;
    s2ch.favList = NULL;
    s2ch.findList = NULL;
    s2ch.favItaList = NULL;
    s2ch.threadList = NULL;
    s2ch.resList = NULL;
	psp2chSetFontParam();
	psp2chSetBarParam();
	pgCursorColorSet();
	// add �t�H���_�����쐬
	if ((fd = sceIoDopen(s2ch.cfg.imageDir)) < 0) // image�t�H���_
	{
		if (sceIoMkdir(s2ch.cfg.imageDir, FILE_PARMISSION) < 0)
		{
//			psp2chNormalError(DIR_MAKE_ERR, s2ch.cfg.imageDir);
			pgErrorDialog( 3, s2ch.cfg.imageDir );
			return -1;
		}
	}
	else
		sceIoDclose(fd);
	if ((fd = sceIoDopen(s2ch.cfg.logDir)) < 0) // log�t�H���_
	{
		if (sceIoMkdir(s2ch.cfg.logDir, FILE_PARMISSION) < 0)
		{
//			psp2chNormalError(DIR_MAKE_ERR, s2ch.cfg.logDir);
			pgErrorDialog( 3, s2ch.cfg.logDir );
			return -1;
		}
	}
	else
		sceIoDclose(fd);
	// sort �����ݒ�t�@�C��
	sprintf(path, "%s/sort.ini", s2ch.cfg.logDir);
	if ((fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION)) > 0)
	{
		psp2chFileRead(fd, path, sizeof(path));
    	sscanf(path, "%d %d", &threadSortType, &favSortType);
    	sceIoClose(fd);
    }
    sprintf(path, "%s/dat", s2ch.cwDir);
    if ((fd = sceIoDopen(path)) < 0) {
    	psp2chNormalError(DIR_MAKE_ERR, path);
    	return -1;
    }
    sceIoDclose(fd);

	// samba24�Ǘ��p�t�@�C��
	s2ch.wait[0].host[0] = '\0';								// host���o�^����Ă��Ȃ���Ζ��g�p
	sprintf(path, "%s/writetime.ini", s2ch.cfg.logDir);
	if ((fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION)) > 0){
		char	buf[TMP_BUF_SIZE],*pos1,*pos2;
		int		ret,count;
		ret = psp2chFileRead(fd, buf, TMP_BUF_SIZE);
		sceIoClose(fd);
		buf[ret-1] = '\0';
		pos1 = buf;
		count = 0;
		do{
			pos2 = pos1;
			pos1 = strchr(pos1,'\n');							// ���s��\0�ɒu�������A�|�C���^�����̍s��
			if (pos1!=NULL){
				*pos1 = '\0';
				if (*(pos1-1)=='\r') *(pos1-1) = '\0';
				pos1++;
			}
			s2ch.wait[count].waitTime = 0;
			sscanf(pos2,"%s = %d",s2ch.wait[count].host,&s2ch.wait[count].waitTime);
			s2ch.wait[count].resTime = 0;
			count++;
			if (count>=RES_WRITETIME_MAX-1) break;
		}while (pos1!=NULL);
		s2ch.wait[count].host[0] = '\0';						// ����Ȍ�͖��g�p
	}

	// ���͗����t�@�C���̃t�H�[�}�b�g�ϊ��i�P����Ώ\���ŁA�K������������K�v�͖����񂾂��ǂˁj
	sprintf(path, "%s/%s", s2ch.cfg.logDir, searchWords);
	saveflag = 0;
	if ((fd = sceIoOpen(path, PSP_O_RDWR, FILE_PARMISSION)) > 0){
		filesize = sceIoLseek(fd, 0, SEEK_END);
		sceIoLseek(fd, 0, SEEK_SET);
		data = (char*) malloc( filesize );
		if (data){
			psp2chFileRead(fd, data, filesize);
			for (i=0; i<filesize ;i++){
				if (data[i] == '\n'){							// LF �� \0
					data[i] = 0;
					saveflag = 1;
				}
			}
			if (saveflag){
				sceIoLseek(fd, 0, SEEK_SET);
				psp2chFileWrite(fd, data, filesize);
			}
		}
		sceIoClose(fd);
		free(data);
	}

    return 0;
}

/**************************
�I���O�̌�n��
**************************/
int psp2chTerm(void)
{
	SceUID fd;
	char path[FILE_PATH];
	
	sprintf(path, "%s/sort.ini", s2ch.cfg.logDir);
	fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
	if(fd > 0) {
		sprintf(path, "%d\n%d\n", threadSortType, favSortType);
		psp2chFileWrite(fd, path, strlen(path));
		sceIoClose(fd);
	}
    return 0;
}

/*****************************
OnScreenKeyboard�ŕ�������
title: OSK�̉E���ɕ\�������^�C�g��������(SJIS)
text:���͕�����ۑ�����|�C���^
num:���͕�����̒���
lines:���͕�����̍s��
OSK����̓��C�h����(UCS)�ŋA���Ă���̂�SJIS�ɕϊ����Ă��܂��B
*****************************/
void psp2chGets(char* title, char* text, int num, int lines)
{
    unsigned char* p;
    unsigned short string[128];
    unsigned short pretext[RES_MESSAGE_LENGTH], text_buf[RES_MESSAGE_LENGTH];
    int running = 0;
    char* buf = "�l������_(^o^)�^";

    if (num > RES_MESSAGE_LENGTH)
    {
        num = RES_MESSAGE_LENGTH;
    }
    /* sjis => ucs */
    if (title && *title != '\0')
    {
        p = (unsigned char*)title;
    }
    else
    {
        p = (unsigned char*)buf;
    }
    psp2chSJIS2UCS(string, p, 127);
    p = (unsigned char*)text;
    psp2chSJIS2UCS(pretext, p, RES_MESSAGE_LENGTH);
    
    SceUtilityOskData data;
    SceUtilityOskParams param;
    
    memset(&data, 0, sizeof(data));
    data.unk_00 = 1;
    data.unk_04 = 0;
    data.language = PSP_UTILITY_OSK_LANGUAGE_DEFAULT;
    data.unk_12 = 0;
    data.inputtype = PSP_UTILITY_OSK_INPUTTYPE_ALL;
    data.lines = lines;
    data.unk_24 = 1;
    data.desc = string;
    data.intext = pretext;
    data.outtext = text_buf;
    data.outtextlength = num;
    data.outtextlimit = num;
    
    memset(&param, 0, sizeof(param));
    param.base.size = sizeof(param);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &param.base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &param.base.buttonSwap);
    param.base.graphicsThread = 0x11;
    param.base.accessThread = 0x13;
    param.base.fontThread = 0x12;
    param.base.soundThread = 0x10;
    param.datacount = 1;
    param.data = &data;
	
	sceUtilityOskInitStart(&param);
	
	while(!running) {
		pgGuRender();
		
		switch(sceUtilityOskGetStatus()) {
			case PSP_UTILITY_DIALOG_NONE :
				running = 1;
				break;
			case PSP_UTILITY_DIALOG_VISIBLE :
				pgWaitVn(1);
				sceUtilityOskUpdate(2);
				break;
			case PSP_UTILITY_DIALOG_QUIT :
				sceUtilityOskShutdownStart();
				break;
			default:
				break;
		}
		flipScreen(0);
	}

    /* ucs => sjis */
    psp2chUCS2SJIS((unsigned char*)text, text_buf, num);
}

//==============================================================
// Simple IME�ɂ�镶�������
//--------------------------------------------------------------
// ���ۂ̏�����PSP�������̕�������͂ōs���Ă܂��B
//--------------------------------------------------------------

static int psp2chLInput(char *title, char *pretext, int type)
{
	char		file[FILE_PATH];
	int			ret;
	strInpHis	**inphis;
	SceCtrlData	pad;

	sprintf(file, "%s/%s", s2ch.cfg.logDir, searchWords);
	inphis = InputHisLoad( file );								// �������擾
	keyWords[0] = '\0';
	if(pretext != NULL){
		strncpy(keyWords, pretext, sizeof(keyWords)-1);
	}

	initGraphics();												// PSP�������p��惂�[�h
	flipScreen2();												// �f�t�H���g�ł̓y�[�W�ݒ�ɕs�s��������̂�
	sceCtrlPeekBufferPositive(&pad, 1);
	SIMEgetchar(pad);											// Simple IME�̏������͑΍�
	ret = InputName(title, 50, type, keyWords, keyWords, inphis);
	if (!ret){													// ����̏ꍇ�͗����Ɍ���o�^
		InputHisAdd(inphis, keyWords);
		InputHisSave(file, inphis);
	}
	pad.Buttons = 0;
	SIMEgetchar(pad);											// ����Simple IME���N���������ɂ����Ȃ�L�[���s�[�g���Ȃ��悤��
	InputHisFree(inphis);										// �������폜
	pgSetupGu();												// �t���[�Y�I���^�p��惂�[�h
	sceCtrlPeekBufferPositive(&s2ch.pad, 1);
	s2ch.oldPad = s2ch.pad;										// ���̌�̃L�[���͎��̃L�[���s�[�g�΍�

	return ret;
}

/****************
���̓_�C�A���O�\��
text1: �_�C�A���O�ɕ\�������^�C�g��
text2: OSK�ɕ\������^�C�g��
�߂�l 0=����, -1=�����
keyWords[]�ɕ����񂪕Ԃ����
*****************/
int psp2chInputDialog(const unsigned char *text1, char *text2, const char *pretext)
{
	const char *menuStr = "  �� : ���́@�@�@�~ : �߂�@�@�@�� : ����@�@�@�� : �œ��́@�@�@���� : ����";
    int ret = 0, fsize = 0, count = 0, i = 0, change = 1, index;
    SceUID fd;
    char file[FILE_PATH];
	strInpHis	**inphis;

	if (s2ch.cfg.inputType == 1){
		return psp2chLInput(text1, pretext, 1);					// OSK�Ń��C������
	} else if (s2ch.cfg.inputType == 2){
		return psp2chLInput(text1, pretext, 0);					// Simple IME�Ń��C������
	}

    // add searchHistory.dat���J��
    sprintf(file, "%s/%s", s2ch.cfg.logDir, searchWords);
	inphis = InputHisLoad( file );
    keyWords[0] = '\0';
    if(pretext != NULL)
    {
    	strncpy(keyWords, pretext, sizeof(keyWords)-1);
    }
    pgCreateTexture();
	pgPrintMenuBar(menuStr);
    s2ch.oldPad = s2ch.pad;
	index = -1;
    while (s2ch.running)
    {
    	if (change)
        {
        	change = 0;
        	pgFillvram(12, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
			pgSetDrawStart(140, 70, 0, 0);
			pgPrint((char*)text1, YELLOW, 0, SCR_WIDTH);
			pgEditBox(WHITE, 140, 85, 340, 101);
			pgSetDrawStart(142, 87, 0, 0);
			pgPrint(keyWords, BLACK, WHITE, 340);
		}
        if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
        {
            if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
            {
                s2ch.oldPad = s2ch.pad;
                if(s2ch.pad.Buttons & PSP_CTRL_CIRCLE)
                {
                    psp2chGets(text2, keyWords, 128, 1);
					if (strlen(keyWords)==0){
						if (pretext){
							strncpy(keyWords, pretext, sizeof(keyWords)-1);
						}
					}
                    i = -1;
                }
                // add ��������
				else if(s2ch.pad.Buttons & PSP_CTRL_UP)			// ���i���͗����j
				{
					if (index>-1) index--;
					index = InputHisGet( keyWords, 128, inphis, index );
					if (index<0){
						keyWords[0] = '\0';
					}
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_DOWN)		// ���i���͗����j
				{
					index++;
					index = InputHisGet( keyWords, 128, inphis, index );
				}
                else if(s2ch.pad.Buttons & PSP_CTRL_CROSS)
                {
                	ret = -1;
                    break;
                }
                else if(s2ch.pad.Buttons & PSP_CTRL_SQUARE)
                {
					InputHisAdd( inphis, keyWords );
					InputHisSave( file, inphis );
                    break;
                }
                else if(s2ch.pad.Buttons & PSP_CTRL_TRIANGLE)
				{
					psp2chInputKana(keyWords, 128);				// �œ��̓p���b�g
					pgPrintMenuBar(menuStr);					// �L�[�K�C�_���X�����ɖ߂�
				}
                change = 1;
            }
        }
		pgDrawTexture(-1);
		pgCopyMenuBar();
		flipScreen(0);
    }
    pgDeleteTexture();
	InputHisFree( inphis );
    return ret;
}

int psp2chFileRead(SceUID fd, void *data, SceSize size)
{
	SceInt64 ret;
	
	ret = sceIoRead(fd, data, size);
	return ret;
}

int psp2chFileWrite(SceUID fd, const void *data, SceSize size)
{
	SceInt64 ret;
	
	ret = sceIoWrite(fd, data, size);
	if (ret != size)
		psp2chErrorDialog(0, "file write failed");
	return ret;
}

int psp2chReadPad(SceCtrlData *pad)
{
	static SceCtrlData old;
	
	sceCtrlPeekBufferPositive(pad, 1);
	if (old.Buttons != pad->Buttons) {
		old = *pad;
		return 1;
	}
	pad->Buttons = 0;
	return 0;
}
