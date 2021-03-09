/*
* $Id$
*/

#include "psp2ch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pg.h"
	
#include "colorMap.h"

extern S_2CH s2ch; // psp2ch.c
extern const char* colorFile; // psp2chMenu.c
extern int cursorMode; // psp2chRes.c
extern int wide; // psp2chRes.c

#define setHex(A, B, C) \
    p = strstr(buf, (A));\
    if (p) {\
        p = strstr(p, "0x");\
        if(p){\
            p+=2;\
            (B) = strtol(p, NULL, 16);\
        }\
        else{(B) = (C);}\
    }\
    else{(B) = (C);}

#define setInt(A, B, C) \
    p = strstr(buf, (A));\
    if (p) {\
        p = strchr(p, '=');\
        if(p){\
            p++;\
            (B) = strtol(p, NULL, 10);\
        }\
        else{(B) = (C);}\
    }\
    else{(B) = (C);}

#define setString(A, B, C, D) \
    p = strstr(buf2, (A));\
    if (p) {\
        p = strchr(p, '"');\
        if(p){\
            p++;\
            for (i = 0; *p != '"'; i++) {\
                if (i >= (D)) break;\
                (B)[i] = *p++;\
            }\
            (B)[i] = '\0';\
        }\
        else{strcpy((B), (C));}\
    }\
    else{strcpy((B), (C));}\

/***********************************
config.ini��ǂݍ���
************************************/
void psp2chIniLoadConfig(void)
{
    SceUID fd;
    SceIoStat st;
    char path[FILE_PATH];
    char *buf, *buf2;
    char *p;
    int i, j;

    sprintf(path, "%s/config.ini", s2ch.cwDir);
    i = sceIoGetstat(path, &st);
    if (i >= 0)
    {
        buf = (char*)malloc(st.st_size + 1);
        if (buf)
        {
            fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
            if (fd >= 0)
            {
                psp2chFileRead(fd, buf, st.st_size);
                sceIoClose(fd);
                buf[st.st_size] = '\0';
                buf2 = buf;
                setInt("CFG_PAD_REVERSE", s2ch.cfg.padReverse, 1);
                setInt("CFG_PAD_ACCEL", s2ch.cfg.padAccel, 1);
                setInt("CFG_PAD_CUTOFF", s2ch.cfg.padCutoff, 35);
                setInt("CFG_FAV_SELECT", s2ch.cfg.favSelect, 0);
                setInt("CFG_IMAGE_VIEW", s2ch.cfg.imageView, 0);
				setInt("CFG_IMAGE_MENU", s2ch.cfg.picMenu, 1);			// add �摜�r�����[�������j���[�ݒ�
				if (s2ch.cfg.picMenu > 3) s2ch.cfg.picMenu = 3;
				setInt("CFG_IMAGE_THUMB", s2ch.cfg.thumb, 0);			// add �摜�r�����[�����{��
				if (s2ch.cfg.thumb > 2) s2ch.cfg.picMenu = 2;
                setInt("CFG_BROWSER_DISCONNECT", s2ch.cfg.browserDisconnect, 2);
                setInt("CFG_CURSOL_MODE", cursorMode, 1); // add
                setInt("CFG_SCREEN_MODE", wide, 0); // add
                setInt("CFG_LOG_DISPLAY", s2ch.cfg.logDisplay, 1);		// add �ߋ����O�\�����[�h
				setInt("CFG_RES_TIMECOUNT", s2ch.cfg.timecount, 1);		// add �������݌o�ߎ���
				setInt("CFG_LIST_IKIOI", s2ch.cfg.ikioi, 1);			// add �X���ꗗ�ł̐����l�\��
				setInt("CFG_FORM_OSK", s2ch.cfg.formOSK, 0);			// add �������ݎ���OSK��D�悷�邩
				setInt("CFG_BROWSER_TABS", s2ch.cfg.browserTabs, 1);	// add �u���E�U�̃^�u��
				setInt("CFG_ANCHOR_RANGE", s2ch.cfg.anchorRange, 20);	// add �A���J�[�̗L���ő�͈͎w�萔
				setInt("CFG_THREAD_SEARCH", s2ch.cfg.threadSearch, 0);	// add �X�����Ď��ɗ��Ă��X�����������邩
				setInt("CFG_THREAD_ROLL", s2ch.cfg.threadRoll, 0);		// add �X���ꗗ�̃J�[�\���s���X�N���[�������邩
				setInt("CFG_RES_TYPE", s2ch.cfg.resType, 0);			// add ���X���̕\������
				setInt("CFG_FILE_SORT", s2ch.cfg.fileSort, 1);			// add �t�@�C���[�Ńt�@�C���\�[�g���邩
				setInt("CFG_HBL", s2ch.cfg.hbl, 0);						// add HBL�œ�������
				setInt("CFG_RES_IDCOUNT", s2ch.cfg.idCount, 0);			// add �X���\����ID�̏o������\�����邩
				setInt("CFG_RES_NG", s2ch.cfg.abon, 0);					// add NG���X�����ځ[��\�����邩
				setInt("CFG_START_COLOR", s2ch.cfg.colorSet, 0);		// add �ŏ��ɃJ���[�I�����邩
				setInt("CFG_INPUT_TYPE", s2ch.cfg.inputType, 0);		// add ���̓_�C�A���O�̎��
				setInt("CFG_UPDATE_TIME", s2ch.cfg.updateTime, 180);	// add �����X���X�V����
				if (s2ch.cfg.updateTime < 30) s2ch.cfg.updateTime = 30;
				setInt("CFG_SCROLL_TIME", s2ch.cfg.rollTime, 10);		// add �����X�N���[������
				if (s2ch.cfg.rollTime < 1) s2ch.cfg.rollTime = 1;
				setInt("CFG_BGM_LOOP", s2ch.cfg.bgmLoop, 1);			// add ����BGM���[�v�ݒ�
				if (s2ch.cfg.bgmLoop > 3) s2ch.cfg.bgmLoop = 3;
				setInt("CFG_SLEEP_TIME", s2ch.cfg.sleep, 10);			// add �T�X�y���h����
				setInt("CFG_FIND_MAX", s2ch.cfg.findMax, 50);			// add �S�������̌���
				setInt("CFG_RES_PADSPEED", s2ch.cfg.padSpeed, 2);		// add ���X�\���ł̃p�b�h�ɂ��ő�X�N���[����
				if (s2ch.cfg.padSpeed > 4) s2ch.cfg.padSpeed = 4;
                setString("CFG_IMAGE_DIR", s2ch.cfg.imageDir, "ms0:/PICTURE/OWATA", FILE_PATH);
                setString("CFG_LOG_DIR", s2ch.cfg.logDir, "ms0:/PICTURE/log", FILE_PATH);
                setString("CFG_BBSMENU_ADDR", s2ch.cfg.bbsmenuAddr, "menu.2ch.net/bbsmenu.html", NET_PATH_LENGTH); // add
                setString("CFG_TEMPLATE_DIR", s2ch.cfg.templateDir, "", FILE_PATH); // add
                setString("CFG_ADD_COOKIE", s2ch.cfg.addCookie, "; tepo=don", 256);
				setString("CFG_GOOGLE_URL", s2ch.cfg.google, "http://www.google.com/hws/search?q=&client=sce-psp-jp", 256);	// add
				setString("CFG_UNZIP_DIR", s2ch.cfg.unzipDir, "ms0:/PICTURE/OWATA", FILE_PATH);								// add
				setString("CFG_UNRAR_DIR", s2ch.cfg.unrarDir, "ms0:/PICTURE/OWATA", FILE_PATH);								// add
				setString("CFG_BGM_DIR", s2ch.cfg.bgmDir, "ms0:/MUSIC", FILE_PATH);											// add
                s2ch.font.count = 0;
                while ((p = strstr(buf2, "CFG_FONT_SET")))
                {
                    s2ch.font.count++;
                    buf2 = p + 12;
                }
                if (s2ch.font.count == 0)
                {
                    s2ch.font.count = 1;
                }
                s2ch.font.set = (char**)malloc(sizeof(char*) * s2ch.font.count);
                buf2 = buf;
                if (s2ch.font.set == NULL)
                {
                    strcpy(s2ch.font.fileA, "monafontA.bin");
                    strcpy(s2ch.font.fileJ, "monafontJ.bin");
                    s2ch.font.height = 12;
                    s2ch.font.pitch = 13;
                    s2ch.font.count = 1;
                }
                else
                {
                    for (j = 0; j < s2ch.font.count; j++)
                    {
                        setString("CFG_FONT_SET", path, "���i�[	monafontA.bin	monafontJ.bin	12	13", FILE_PATH);
                        s2ch.font.set[j] = (char*)malloc(sizeof(char) * strlen(path));
                        strcpy(s2ch.font.set[j], path);
                        buf2 = p;
                    }
                }

                free(buf);
                return;
            }
            else
            {
                free(buf);
            }
        }
    }
    s2ch.cfg.padReverse = 1;		// �p�b�h�����̈ړ�����(1:�Ώۂ��ړ�,-1:���_���ړ�)
    s2ch.cfg.padAccel = 1;			// 0:2�i�K��, 1:8�i�K��
    s2ch.cfg.padCutoff = 35;		// �A�i���O�p�b�h�̃j���[�g���������ђl
    s2ch.cfg.favSelect = 0;			// ���C�ɓ���̃f�t�H���g�\�� 0:�X��, 1:��
	s2ch.cfg.imageView = 0;			// �����N�悪�摜�̂Ƃ�0�F�r���[���[�ŊJ���A1�F�u���E�U�ŊJ��
	s2ch.cfg.picMenu = 1;			// �摜�r�����[�������j���[�ݒ�
	s2ch.cfg.thumb = 0;				// �摜�r�����[�����{��
	s2ch.cfg.browserDisconnect = 2;	// �u���E�U�I�����ɐؒf 0�F����A1�F���Ȃ��A2�F�I��
	cursorMode = 1;					// add �X���{�����̃J�[�\�����[�h
    wide = 0;						// add �X���{�����̃X�N���[�����[�h
    s2ch.cfg.logDisplay = 1;		// add �ߋ����O�\�����[�h
	s2ch.cfg.timecount = 1;			// �������݌o�ߎ���
	s2ch.cfg.ikioi = 1;				// �X���ꗗ�ł̐����l�\��
	s2ch.cfg.formOSK = 0;			// �������ݎ���OSK��D�悷�邩
	s2ch.cfg.browserTabs = 1;		// �u���E�U�̃^�u��
	s2ch.cfg.anchorRange = 20;		// �A���J�[�̗L���ő�͈͎w�萔
	s2ch.cfg.threadSearch = 0;		// �X�����Ď��ɗ��Ă��X�����������邩
	s2ch.cfg.threadRoll = 0;		// �X���ꗗ�̃J�[�\���s���X�N���[�������邩
	s2ch.cfg.resType = 0;			// ���X���̕\������
	s2ch.cfg.fileSort = 1;			// �t�@�C���[�Ńt�@�C���\�[�g���邩
	s2ch.cfg.hbl = 0;				// HBL��œ�������
	s2ch.cfg.idCount = 0;			// �X���\����ID�̏o������\�����邩
	s2ch.cfg.abon = 0;				// NG���X�����ځ[��\�����邩
	s2ch.cfg.colorSet = 0;			// �ŏ��ɃJ���[�I�����邩
	s2ch.cfg.inputType = 0;			// ���̓_�C�A���O�̎��
	s2ch.cfg.updateTime = 180;		// �����X���X�V����
	s2ch.cfg.rollTime = 10;			// �����X�N���[������
	s2ch.cfg.bgmLoop = 1;			// ����BGM���[�v�ݒ�
	s2ch.cfg.sleep = 10;			// �T�X�y���h����
	s2ch.cfg.findMax = 50;			// �S�������̌���
	s2ch.cfg.padSpeed = 2;			// ���X�\���ł̃p�b�h�ɂ��ő�X�N���[����
    strcpy(s2ch.cfg.imageDir, "ms0:/PICTURE/OWATA");			// PICTURE�t�H���_�ɍ쐬����t�H���_��
    strcpy(s2ch.cfg.logDir, "ms0:/PICTURE/log");				// add ���O�ۑ��t�H���_
    strcpy(s2ch.cfg.bbsmenuAddr, "menu.2ch.net/bbsmenu.html");	// add ���X�g�̎擾��
    strcpy(s2ch.cfg.templateDir, "");							// add ��^�������t�H���_
    strcpy(s2ch.cfg.addCookie, "; tepo=don");					// add cookie�ɒǉ�����
	strcpy(s2ch.cfg.google, "http://www.google.com/hws/search?q=&client=sce-psp-jp"); // �������URL
	strcpy(s2ch.cfg.unzipDir, "ms0:/PICTURE/OWATA");			// zip�t�@�C���𓀃t�H���_
	strcpy(s2ch.cfg.unrarDir, "ms0:/PICTURE/OWATA");			// rar�t�@�C���𓀃t�H���_
	strcpy(s2ch.cfg.bgmDir, "ms0:/MUSIC");						// BGM�t�H���_
    strcpy(s2ch.font.fileA, "monafontA.bin");					// �V���O���o�C�g(ASCII, ���p�J�i)�t�H���g
    strcpy(s2ch.font.fileJ, "monafontJ.bin");					// �}���`�o�C�g�t�H���g
    s2ch.font.height = 12;
    s2ch.font.pitch = 13;
    s2ch.font.count = 1;
    s2ch.font.set = NULL;
}

/***********************************
�O���J���[�Z�b�g
�ݒ�t�@�C����ǂݍ���Ŋe�J���[���Z�b�g
************************************/
void psp2chIniSetColor(const char* file)
{
    SceUID fd;
    SceIoStat st;
    char path[FILE_PATH];
    char *buf;
    char *p;
    int ret;

	// colorPalette�Ƃ̑Ή�
    // ���X�{��
    s2ch.resHeaderColor.num = 16;
    s2ch.resHeaderColor.num2 = 29;
    s2ch.resHeaderColor.num3 = 30;
    s2ch.resHeaderColor.name1 = 17;
    s2ch.resHeaderColor.name2 = 18;
    s2ch.resHeaderColor.mail = 19;
    s2ch.resHeaderColor.date = 20;
    s2ch.resHeaderColor.id0 = 31;
    s2ch.resHeaderColor.id1 = 21;
    s2ch.resHeaderColor.id2 = 22;
    s2ch.resHeaderColor.id3 = 23;
    s2ch.resColor.text = 24;
    s2ch.resColor.bg = 25;
    s2ch.resColor.link = 26;
    s2ch.resBarColor.slider = 27;
    s2ch.resBarColor.bg = 28;
    // ���X�A���J�[�@
    s2ch.resAHeaderColor.num = 32;
    s2ch.resAHeaderColor.num2 = 45;
    s2ch.resAHeaderColor.num3 = 46;
    s2ch.resAHeaderColor.name1 = 33;
    s2ch.resAHeaderColor.name2 = 34;
    s2ch.resAHeaderColor.mail = 35;
    s2ch.resAHeaderColor.date = 36;
    s2ch.resAHeaderColor.id0 = 47;
    s2ch.resAHeaderColor.id1 = 37;
    s2ch.resAHeaderColor.id2 = 38;
    s2ch.resAHeaderColor.id3 = 39;
    s2ch.resAColor.text = 40;
    s2ch.resAColor.bg = 41;
    s2ch.resAColor.link = 42;
    s2ch.resABarColor.slider = 43;
    s2ch.resABarColor.bg = 44;
    // ���j���[�o�[�@
    s2ch.menuColor.text = 48;
    s2ch.menuColor.bg = 49;
    s2ch.menuColor.bat1 = 50;
    s2ch.menuColor.bat2 = 51;
    s2ch.menuColor.bat3 = 52;
    // �X���b�h�ꗗ
    s2ch.threadColor.num[0] = 64;
    s2ch.threadColor.category[0] = 65;
    s2ch.threadColor.text1[0] = 66;
    s2ch.threadColor.text2[0] = 67;
    s2ch.threadColor.bg[0] = 68;
    s2ch.threadColor.count1[0] = 69;
    s2ch.threadColor.count2[0] = 70;
    s2ch.threadColor.num[1] = 71;
    s2ch.threadColor.category[1] = 72;
    s2ch.threadColor.text1[1] = 73;
    s2ch.threadColor.text2[1] = 74;
    s2ch.threadColor.bg[1] = 75;
    s2ch.threadColor.count1[1] = 76;
    s2ch.threadColor.count2[1] = 77;
    // �J�e�S���[
    s2ch.cateOnColor.cate.text = 80;
    s2ch.cateOnColor.cate.bg = 81;
    s2ch.cateOnColor.cate.s_text = 82;
    s2ch.cateOnColor.cate.s_bg = 83;
    s2ch.cateOnColor.ita.text = 84;
    s2ch.cateOnColor.ita.bg = 85;
    s2ch.cateOnColor.ita.s_text = 86;
    s2ch.cateOnColor.ita.s_bg = 87;
    s2ch.cateOnColor.base = 88;
    // �ꗗ
    s2ch.cateOffColor.cate.text = 96;
    s2ch.cateOffColor.cate.bg = 97;
    s2ch.cateOffColor.cate.s_text = 98;
    s2ch.cateOffColor.cate.s_bg = 99;
    s2ch.cateOffColor.ita.text = 100;
    s2ch.cateOffColor.ita.bg = 101;
    s2ch.cateOffColor.ita.s_text = 102;
    s2ch.cateOffColor.ita.s_bg = 103;
    s2ch.cateOffColor.base = 104;
    // ���M�t�H�[��
    s2ch.formColor.ita = 112;
    s2ch.formColor.title = 113;
    s2ch.formColor.title_bg = 114;
    // ���j���[�E�B���h�E
    s2ch.menuWinColor.text = 128;
    s2ch.menuWinColor.bg = 129;
    s2ch.menuWinColor.s_text = 130;
    s2ch.menuWinColor.s_bg = 131;
    s2ch.menuWinColor.dir = 132;
    // �J�[�\��
    s2ch.cursorColor.arrow1 = 144;
    s2ch.cursorColor.arrow2 = 145;

    if (file == NULL)
    {
        file = colorFile;
    }
    sprintf(path, "%s/%s/%s", s2ch.cwDir, COLOR_DIR, file);
    ret = sceIoGetstat(path, &st);
    if (ret >= 0)
    {
        buf = (char*)malloc(st.st_size + 1);
        if (buf)
        {
            fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
            if (fd >= 0)
            {
                psp2chFileRead(fd, buf, st.st_size);
                sceIoClose(fd);
                buf[st.st_size] = '\0';
                setHex("RES_NUMBER", colorPalette[16], 0xF00F);
                setHex("RES_NAME_HEAD", colorPalette[17], 0xF000);
                setHex("RES_NAME_BODY", colorPalette[18], 0xF0C0);
                setHex("RES_MAIL", colorPalette[19], 0xF999);
                setHex("RES_DATE", colorPalette[20], 0xF000);
                setHex("RES_ID_HEAD_1", colorPalette[21], 0xFF00);
                setHex("RES_ID_HEAD_2", colorPalette[22], 0xF00F);
                setHex("RES_ID_BODY", colorPalette[23], 0xFF00);
                setHex("RES_TEXT", colorPalette[24], 0xF000);
                setHex("RES_BG", colorPalette[25], 0xFEEE);
                setHex("RES_LINK", colorPalette[26], 0xFF00);
                setHex("RES_BAR_SLIDER", colorPalette[27], 0xF0FF);
                setHex("RES_BAR_BG", colorPalette[28], 0xFFFC);
                setHex("RES_NUMBER2", colorPalette[29], 0xFF00);
                setHex("RES_NUMBER3", colorPalette[30], 0xF808);
                setHex("RES_ID_HEAD_0", colorPalette[31], 0xF000);

                setHex("RES_A_NUMBER", colorPalette[32], 0xF00F);
                setHex("RES_A_NAME_HEAD", colorPalette[33], 0xF000);
                setHex("RES_A_NAME_BODY", colorPalette[34], 0xF0C0);
                setHex("RES_A_MAIL", colorPalette[35], 0xF999);
                setHex("RES_A_DATE", colorPalette[36], 0xF000);
                setHex("RES_A_ID_HEAD_1", colorPalette[37], 0xFF00);
                setHex("RES_A_ID_HEAD_2", colorPalette[38], 0xF00F);
                setHex("RES_A_ID_BODY", colorPalette[39], 0xFF00);
                setHex("RES_A_TEXT", colorPalette[40], 0xF000);
                setHex("RES_A_BG", colorPalette[41], 0xFCFF);
                setHex("RES_A_LINK", colorPalette[42], 0xFF00);
                setHex("RES_A_BAR_SLIDER", colorPalette[43], 0xFCF0);
                setHex("RES_A_BAR_BG", colorPalette[44], 0xFFFC);
                setHex("RES_A_NUMBER2", colorPalette[45], 0xFF00);
                setHex("RES_A_NUMBER3", colorPalette[46], 0xF808);
                setHex("RES_A_ID_HEAD_0", colorPalette[47], 0xF000);

                setHex("MENU_TEXT", colorPalette[48], 0xFFFF);
                setHex("MENU_BG", colorPalette[49], 0xF000);
                setHex("MENU_BATTERY_1", colorPalette[50], 0xF0F0);
                setHex("MENU_BATTERY_2", colorPalette[51], 0xF0FF);
                setHex("MENU_BATTERY_3", colorPalette[52], 0xF00F);

                setHex("THREAD_NUMBER", colorPalette[64], 0xF00F);
                setHex("THREAD_CATEGORY", colorPalette[65], 0xF00F);
                setHex("THREAD_TEXT_1", colorPalette[66], 0xFF00);
                setHex("THREAD_TEXT_2", colorPalette[67], 0xF00F);
                setHex("THREAD_BG", colorPalette[68], 0xFCFC);
                setHex("THREAD_COUNT_1", colorPalette[69], 0xF000);
                setHex("THREAD_COUNT_2", colorPalette[70], 0xF00F);
                setHex("THREAD_SELECT_NUMBER", colorPalette[71], 0xF00F);
                setHex("THREAD_SELECT_CATEGORY", colorPalette[72], 0xF009);
                setHex("THREAD_SELECT_TEXT_1", colorPalette[73], 0xF900);
                setHex("THREAD_SELECT_TEXT_2", colorPalette[74], 0xF009);
                setHex("THREAD_SELECT_BG", colorPalette[75], 0xFCCC);
                setHex("THREAD_SELECT_COUNT_1", colorPalette[76], 0xF000);
                setHex("THREAD_SELECT_COUNT_2", colorPalette[77], 0xF00F);

                setHex("CATE_ON_TEXT", colorPalette[80], 0xF03C);
                setHex("CATE_ON_BG", colorPalette[81], 0xFFFF);
                setHex("CATE_ON_S_TEXT", colorPalette[82], 0xFFFF);
                setHex("CATE_ON_S_BG", colorPalette[83], 0xF03C);
                setHex("ITA_OFF_TEXT", colorPalette[84], 0xFF66);
                setHex("ITA_OFF_BG", colorPalette[85], 0xFCCC);
                setHex("ITA_OFF_S_TEXT", colorPalette[86], 0xFCCC);
                setHex("ITA_OFF_S_BG", colorPalette[87], 0xFF66);
                setHex("CATE_ON_BASE", colorPalette[88], 0xFFFF);

                setHex("CATE_OFF_TEXT", colorPalette[96], 0xF698);
                setHex("CATE_OFF_BG", colorPalette[97], 0xFCCC);
                setHex("CATE_OFF_S_TEXT", colorPalette[98], 0xFCCC);
                setHex("CATE_OFF_S_BG", colorPalette[99], 0xF698);
                setHex("ITA_ON_TEXT", colorPalette[100], 0xFF00);
                setHex("ITA_ON_BG", colorPalette[101], 0xFFFF);
                setHex("ITA_ON_S_TEXT", colorPalette[102], 0xFFFF);
                setHex("ITA_ON_S_BG", colorPalette[103], 0xFF00);
                setHex("CATE_OFF_BASE", colorPalette[104], 0xFFFF);

                setHex("FORM_ITA_TEXT", colorPalette[112], 0xFCCC);
                setHex("FORM_TITLE_TEXT", colorPalette[113], 0xFFFF);
                setHex("FORM_TITLE_BG", colorPalette[114], 0xF00F);

                setHex("MENU_WIN_TEXT", colorPalette[128], 0xFCCC);
                setHex("MENU_WIN_BG", colorPalette[129], 0xF000);
                setHex("MENU_WIN_S_TEXT", colorPalette[130], 0xFFFF);
                setHex("MENU_WIN_S_BG", colorPalette[131], 0xFF00);
                setHex("MENU_WIN_DIR", colorPalette[132], 0xF0FF);

                setHex("CURSOR_ARROW1", colorPalette[144], 0xF000);
                setHex("CURSOR_ARROW2", colorPalette[145], 0xFFFF);
            }
            free(buf);
        }
    }
    pgCLUTUpdate();
    return;
}

/********************
�{�^���̊��蓖��
button.ini��ǂݍ���Ń{�^���̐ݒ�
********************/
void psp2chIniSetButtons(void)
{
    SceUID fd;
    SceIoStat st;
    char path[FILE_PATH];
    char *buf;
    char *p;
    int ret;

    sprintf(path, "%s/button.ini", s2ch.cwDir);
    ret = sceIoGetstat(path, &st);
    if (ret >= 0)
    {
        buf = (char*)malloc(st.st_size + 1);
        if (buf)
        {
            fd = sceIoOpen(path, PSP_O_RDONLY, FILE_PARMISSION);
            if (fd >= 0)
            {
                psp2chFileRead(fd, buf, st.st_size);
                sceIoClose(fd);
                buf[st.st_size] = '\0';
                setInt("RES_FORM_H", s2ch.btnRes[0].form, 8192);
                setInt("RES_BACK_H", s2ch.btnRes[0].back, 16384);
                setInt("RES_RELOAD_H", s2ch.btnRes[0].reload, 4096);
                setInt("RES_DATDEL_H", s2ch.btnRes[0].datDel, 32768);
                setInt("RES_SHIFT_H", s2ch.btnRes[0].change, 512);
                setInt("RES_FORM_V", s2ch.btnRes[1].form, 8192);
                setInt("RES_BACK_V", s2ch.btnRes[1].back, 16384);
                setInt("RES_RELOAD_V", s2ch.btnRes[1].reload, 4096);
                setInt("RES_DATDEL_V", s2ch.btnRes[1].datDel, 32768);
                setInt("RES_SHIFT_V", s2ch.btnRes[1].change, 512);

                setInt("RES_TOP_H", s2ch.btnRes[0].s.top, 16);
                setInt("RES_END_H", s2ch.btnRes[0].s.end, 64);
                setInt("RES_ADDFAV_H", s2ch.btnRes[0].addFav, 8192);
                setInt("RES_DELFAV_H", s2ch.btnRes[0].delFav, 32768);
                setInt("RES_CURSOR_H", s2ch.btnRes[0].cursor, 4096);
                setInt("RES_WIDE_H", s2ch.btnRes[0].wide, 16384);
                setInt("RES_SEARCH_H", s2ch.btnRes[0].search, 256);
                setInt("RES_TOP_V", s2ch.btnRes[1].s.top, 32);
                setInt("RES_END_V", s2ch.btnRes[1].s.end, 128);
                setInt("RES_ADDFAV_V", s2ch.btnRes[1].addFav, 8192);
                setInt("RES_DELFAV_V", s2ch.btnRes[1].delFav, 32768);
                setInt("RES_CURSOR_V", s2ch.btnRes[1].cursor, 4096);
                setInt("RES_WIDE_V", s2ch.btnRes[1].wide, 16384);
                setInt("RES_SEARCH_V", s2ch.btnRes[1].search, 256);

                setInt("RES_NUMRES_H", s2ch.btnRes[0].resForm, 32768);
                setInt("RES_NUMBACK_H", s2ch.btnRes[0].resFBack, 16384);
                setInt("RES_NUMRESVIEW_H", s2ch.btnRes[0].resFRes, 8192);
                setInt("RES_NUMRES_V", s2ch.btnRes[1].resForm, 32768);
                setInt("RES_NUMBACK_V", s2ch.btnRes[1].resFBack, 16384);
                setInt("RES_NUMRESVIEW_V", s2ch.btnRes[1].resFRes, 8192);

                setInt("RES_IDVIEW_H", s2ch.btnRes[0].idView, 8192);
                setInt("RES_IDNG_H", s2ch.btnRes[0].idNG, 32768);
                setInt("RES_IDBACK_H", s2ch.btnRes[0].idBack, 16384);
                setInt("RES_IDVIEW_V", s2ch.btnRes[1].idView, 8192);
                setInt("RES_IDNG_V", s2ch.btnRes[1].idNG, 32768);
                setInt("RES_IDBACK_V", s2ch.btnRes[1].idBack, 16384);

                setInt("RES_RESVIEW_H", s2ch.btnRes[0].resView, 8192);
                setInt("RES_RESMOVE_H", s2ch.btnRes[0].resMove, 4096);
                setInt("RES_RESBACK_H", s2ch.btnRes[0].resBack, 16384);
                setInt("RES_RESVIEW_V", s2ch.btnRes[1].resView, 8192);
                setInt("RES_RESMOVE_V", s2ch.btnRes[1].resMove, 4096);
                setInt("RES_RESBACK_V", s2ch.btnRes[1].resBack, 16384);

                setInt("RES_URL_H", s2ch.btnRes[0].url, 8192);
                setInt("RES_URL_HTML_H", s2ch.btnRes[0].urlHtml, 32768);
                setInt("RES_URLBACK_H", s2ch.btnRes[0].urlBack, 16384);
                setInt("RES_URLDOWN_H", s2ch.btnRes[0].urlDown, 4096);
                setInt("RES_URL_V", s2ch.btnRes[1].url, 8192);
                setInt("RES_URL_HTML_V", s2ch.btnRes[1].urlHtml, 32768);
                setInt("RES_URLBACK_V", s2ch.btnRes[1].urlBack, 16384);
                setInt("RES_URLDOWN_V", s2ch.btnRes[1].urlDown, 4096);

                setInt("RES_UP_H", s2ch.btnRes[0].s.up, 16);
                setInt("RES_PAGEUP_H", s2ch.btnRes[0].s.pUp, 128);
                setInt("RES_DOWN_H", s2ch.btnRes[0].s.down, 64);
                setInt("RES_PAGEDOWN_H", s2ch.btnRes[0].s.pDown, 32);
                setInt("RES_UP_V", s2ch.btnRes[1].s.up, 32);
                setInt("RES_PAGEUP_V", s2ch.btnRes[1].s.pUp, 16);
                setInt("RES_DOWN_V", s2ch.btnRes[1].s.down, 128);
                setInt("RES_PAGEDOWN_V", s2ch.btnRes[1].s.pDown, 64);

                setInt("SCROLL_UP_H", s2ch.listB[0].up, 16);
                setInt("PAGE_UP_H", s2ch.listB[0].pUp, 128);
                setInt("SCROLL_DOWN_H", s2ch.listB[0].down, 64);
                setInt("PAGE_DOWN_H", s2ch.listB[0].pDown, 32);
                setInt("LIST_TOP_H", s2ch.listB[0].top, 16);
                setInt("LIST_END_H", s2ch.listB[0].end, 64);
                setInt("SCROLL_UP_V", s2ch.listB[1].up, 32);
                setInt("PAGE_UP_V", s2ch.listB[1].pUp, 16);
                setInt("SCROLL_DOWN_V", s2ch.listB[1].down, 128);
                setInt("PAGE_DOWN_V", s2ch.listB[1].pDown, 64);
                setInt("LIST_TOP_V", s2ch.listB[1].top, 32);
                setInt("LIST_END_V", s2ch.listB[1].end, 128);

                setInt("ITA_OK_H", s2ch.itaB[0].ok, 8192);
                setInt("ITA_ESC_H", s2ch.itaB[0].esc, 16384);
                setInt("ITA_MOVEFAV_H", s2ch.itaB[0].move, 32768);
                setInt("ITA_RELOAD_H", s2ch.itaB[0].reload, 4096);
                setInt("ITA_SHIFT_H", s2ch.itaB[0].shift, 512);
                setInt("ITA_OK_V", s2ch.itaB[1].ok, 256);
                setInt("ITA_ESC_V", s2ch.itaB[1].esc, 16384);
                setInt("ITA_MOVEFAV_V", s2ch.itaB[1].move, 32768);
                setInt("ITA_RELOAD_V", s2ch.itaB[1].reload, 4096);
                setInt("ITA_SHIFT_V", s2ch.itaB[1].shift, 512);

                setInt("ITA_ADDFAV_H", s2ch.itaB[0].addFav, 4096);
                setInt("ITA_2CHSEARCH_H", s2ch.itaB[0].search2ch, 32768);
                setInt("ITA_ADDFAV_V", s2ch.itaB[1].addFav, 4096);
                setInt("ITA_2CHSEARCH_V", s2ch.itaB[1].search2ch, 32768);

                setInt("THREAD_OK_H", s2ch.thB[0].ok, 8192);
                setInt("THREAD_ESC_H", s2ch.thB[0].esc, 16384);
                setInt("THREAD_MOVEFAV_H", s2ch.thB[0].move, 32768);
                setInt("THREAD_RELOAD_H", s2ch.thB[0].reload, 4096);
                setInt("THREAD_SHIFT_H", s2ch.thB[0].shift, 512);
                setInt("THREAD_OK_V", s2ch.thB[1].ok, 256);
                setInt("THREAD_ESC_V", s2ch.thB[1].esc, 16384);
                setInt("THREAD_MOVEFAV_V", s2ch.thB[1].move, 32768);
                setInt("THREAD_RELOAD_V", s2ch.thB[1].reload, 4096);
                setInt("THREAD_SHIFT_V", s2ch.thB[1].shift, 512);

                setInt("THREAD_SORT_H", s2ch.thB[0].sort, 8192);
                setInt("THREAD_SEARCH_H", s2ch.thB[0].search, 4096);
                setInt("THREAD_2CHSEARCH_H", s2ch.thB[0].search2ch, 32768);
                setInt("THREAD_SORT_V", s2ch.thB[1].sort, 8192);
                setInt("THREAD_SEARCH_V", s2ch.thB[1].search, 4096);
                setInt("THREAD_2CHSEARCH_V", s2ch.thB[1].search2ch, 32768);

                setInt("FAV_OK_H", s2ch.favB[0].ok, 8192);
                setInt("FAV_MOVEITS_H", s2ch.favB[0].move, 16384);
                setInt("FAV_CHANGE_H", s2ch.favB[0].change, 4096);
                setInt("FAV_DEL_H", s2ch.favB[0].del, 32768);
                setInt("FAV_SHIFT_H", s2ch.favB[0].shift, 512);
                setInt("FAV_OK_V", s2ch.favB[1].ok, 256);
                setInt("FAV_MOVEITS_V", s2ch.favB[1].move, 16384);
                setInt("FAV_CHANGE_V", s2ch.favB[1].change, 4096);
                setInt("FAV_DEL_V", s2ch.favB[1].del, 32768);
                setInt("FAV_SHIFT_V", s2ch.favB[1].shift, 512);

                setInt("FAV_SORT_H", s2ch.favB[0].sort, 8192);
                setInt("FAV_2CHSEARCH_H", s2ch.favB[0].search2ch, 32768);
                setInt("FAV_UPDATE_H", s2ch.favB[0].update, 4096);
                setInt("FAV_TRANS_H", s2ch.favB[0].trans, 256);
                setInt("FAV_SORT_V", s2ch.favB[1].sort, 8192);
                setInt("FAV_2CHSEARCH_V", s2ch.favB[1].search2ch, 32768);
                setInt("FAV_UPDATE_V", s2ch.favB[1].update, 4096);
                setInt("FAV_TRANS_V", s2ch.favB[1].trans, 8192);

                setInt("SEARCH_OK_H", s2ch.findB[0].ok, 8192);
                setInt("SEARCH_ESC_H", s2ch.findB[0].esc, 16384);
                setInt("SEARCH_ITA_H", s2ch.findB[0].ita, 32768);
                setInt("SEARCH_FAV_H", s2ch.findB[0].fav, 4096);
                setInt("SEARCH_SHIFT_H", s2ch.findB[0].shift, 512);
                setInt("SEARCH_OK_V", s2ch.findB[1].ok, 256);
                setInt("SEARCH_ESC_V", s2ch.findB[1].esc, 16384);
                setInt("SEARCH_ITA_V", s2ch.findB[1].ita, 32768);
                setInt("SEARCH_FAV_V", s2ch.findB[1].fav, 4096);
                setInt("SEARCH_SHIFT_V", s2ch.findB[1].shift, 512);

                setInt("SEARCH_2CHSEARCH_H", s2ch.findB[0].search2ch, 32768);
                setInt("SEARCH_2CHSEARCH_V", s2ch.findB[1].search2ch, 32768);

                setInt("MENUWIN_OK_H", s2ch.menuWin[0].ok, 8192);
                setInt("MENUWIN_ESC_H", s2ch.menuWin[0].esc, 16384);
                setInt("MENUWIN_OK_V", s2ch.menuWin[1].ok, 256);
                setInt("MENUWIN_ESC_V", s2ch.menuWin[1].esc, 16384);

                setInt("MENUNG_DEL_H", s2ch.menuNG[0].del, 8192);
                setInt("MENUNG_ESC_H", s2ch.menuNG[0].esc, 16384);
                setInt("MENUNG_DEL_V", s2ch.menuNG[1].del, 256);
                setInt("MENUNG_ESC_V", s2ch.menuNG[1].esc, 16384);

                free(buf);
                return;
            }
            else
            {
                free(buf);
            }
        }
    }
    s2ch.btnRes[0].form	= PSP_CTRL_CIRCLE;
    s2ch.btnRes[0].back	= PSP_CTRL_CROSS;
    s2ch.btnRes[0].reload	= PSP_CTRL_TRIANGLE;
    s2ch.btnRes[0].datDel	= PSP_CTRL_SQUARE;
    s2ch.btnRes[0].change	= PSP_CTRL_RTRIGGER;

    s2ch.btnRes[0].s.top	= PSP_CTRL_UP;
    s2ch.btnRes[0].s.end	= PSP_CTRL_DOWN;
    s2ch.btnRes[0].addFav	= PSP_CTRL_CIRCLE;
    s2ch.btnRes[0].delFav	= PSP_CTRL_SQUARE;
    s2ch.btnRes[0].cursor	= PSP_CTRL_TRIANGLE;
    s2ch.btnRes[0].wide	= PSP_CTRL_CROSS;
    s2ch.btnRes[0].search	= PSP_CTRL_LTRIGGER;

    s2ch.btnRes[0].resForm	= PSP_CTRL_SQUARE;
    s2ch.btnRes[0].resFBack	= PSP_CTRL_CROSS;
    s2ch.btnRes[0].resFRes	= PSP_CTRL_CIRCLE;

    s2ch.btnRes[0].idView	= PSP_CTRL_CIRCLE;
    s2ch.btnRes[0].idNG	= PSP_CTRL_SQUARE;
    s2ch.btnRes[0].idBack	= PSP_CTRL_CROSS;

    s2ch.btnRes[0].resView	= PSP_CTRL_CIRCLE;
    s2ch.btnRes[0].resMove	= PSP_CTRL_TRIANGLE;
    s2ch.btnRes[0].resBack	= PSP_CTRL_CROSS;

    s2ch.btnRes[0].url		= PSP_CTRL_CIRCLE;
    s2ch.btnRes[0].urlHtml	= PSP_CTRL_SQUARE;
    s2ch.btnRes[0].urlBack	= PSP_CTRL_CROSS;
    s2ch.btnRes[0].urlDown	= PSP_CTRL_TRIANGLE;

    s2ch.btnRes[0].s.up		= PSP_CTRL_UP;
    s2ch.btnRes[0].s.pUp		= PSP_CTRL_LEFT;
    s2ch.btnRes[0].s.down		= PSP_CTRL_DOWN;
    s2ch.btnRes[0].s.pDown	= PSP_CTRL_RIGHT;

    s2ch.btnRes[1].form	= PSP_CTRL_CIRCLE;
    s2ch.btnRes[1].back	= PSP_CTRL_CROSS;
    s2ch.btnRes[1].reload	= PSP_CTRL_TRIANGLE;
    s2ch.btnRes[1].datDel	= PSP_CTRL_SQUARE;
    s2ch.btnRes[1].change	= PSP_CTRL_RTRIGGER;

    s2ch.btnRes[1].s.top	= PSP_CTRL_RIGHT;
    s2ch.btnRes[1].s.end	= PSP_CTRL_LEFT;
    s2ch.btnRes[1].addFav	= PSP_CTRL_CIRCLE;
    s2ch.btnRes[1].delFav	= PSP_CTRL_SQUARE;
    s2ch.btnRes[1].cursor	= PSP_CTRL_TRIANGLE;
    s2ch.btnRes[1].wide	= PSP_CTRL_CROSS;
    s2ch.btnRes[1].search	= PSP_CTRL_LTRIGGER;

    s2ch.btnRes[1].resForm	= PSP_CTRL_SQUARE;
    s2ch.btnRes[1].resFBack	= PSP_CTRL_CROSS;
    s2ch.btnRes[1].resFRes	= PSP_CTRL_CIRCLE;

    s2ch.btnRes[1].idView	= PSP_CTRL_CIRCLE;
    s2ch.btnRes[1].idNG	= PSP_CTRL_SQUARE;
    s2ch.btnRes[1].idBack	= PSP_CTRL_CROSS;

    s2ch.btnRes[1].resView	= PSP_CTRL_CIRCLE;
    s2ch.btnRes[1].resMove	= PSP_CTRL_TRIANGLE;
    s2ch.btnRes[1].resBack	= PSP_CTRL_CROSS;

    s2ch.btnRes[1].url		= PSP_CTRL_CIRCLE;
    s2ch.btnRes[1].urlHtml	= PSP_CTRL_SQUARE;
    s2ch.btnRes[1].urlBack	= PSP_CTRL_CROSS;
    s2ch.btnRes[1].urlDown	= PSP_CTRL_TRIANGLE;

    s2ch.btnRes[1].s.up		= PSP_CTRL_RIGHT;
    s2ch.btnRes[1].s.pUp		= PSP_CTRL_UP;
    s2ch.btnRes[1].s.down		= PSP_CTRL_LEFT;
    s2ch.btnRes[1].s.pDown	= PSP_CTRL_DOWN;

    s2ch.listB[0].up		= PSP_CTRL_UP;
    s2ch.listB[0].pUp		= PSP_CTRL_LEFT;
    s2ch.listB[0].down		= PSP_CTRL_DOWN;
    s2ch.listB[0].pDown	= PSP_CTRL_RIGHT;
    s2ch.listB[0].top		= PSP_CTRL_UP;
    s2ch.listB[0].end		= PSP_CTRL_DOWN;
    s2ch.listB[1].up		= PSP_CTRL_RIGHT;
    s2ch.listB[1].pUp		= PSP_CTRL_UP;
    s2ch.listB[1].down		= PSP_CTRL_LEFT;
    s2ch.listB[1].pDown		= PSP_CTRL_DOWN;
    s2ch.listB[1].top		= PSP_CTRL_RIGHT;
    s2ch.listB[1].end		= PSP_CTRL_LEFT;

    s2ch.itaB[0].ok     = PSP_CTRL_CIRCLE;
    s2ch.itaB[0].esc    = PSP_CTRL_CROSS;
    s2ch.itaB[0].move   = PSP_CTRL_SQUARE;
    s2ch.itaB[0].reload = PSP_CTRL_TRIANGLE;
    s2ch.itaB[0].shift  = PSP_CTRL_RTRIGGER;
    s2ch.itaB[1].ok     = PSP_CTRL_LTRIGGER;
    s2ch.itaB[1].esc    = PSP_CTRL_CROSS;
    s2ch.itaB[1].move   = PSP_CTRL_SQUARE;
    s2ch.itaB[1].reload = PSP_CTRL_TRIANGLE;
    s2ch.itaB[1].shift  = PSP_CTRL_RTRIGGER;

    s2ch.itaB[0].addFav    = PSP_CTRL_TRIANGLE;
    s2ch.itaB[0].search2ch = PSP_CTRL_SQUARE;
    s2ch.itaB[1].addFav    = PSP_CTRL_TRIANGLE;
    s2ch.itaB[1].search2ch = PSP_CTRL_SQUARE;

    s2ch.thB[0].ok		= PSP_CTRL_CIRCLE;
    s2ch.thB[0].esc	= PSP_CTRL_CROSS;
    s2ch.thB[0].move	= PSP_CTRL_SQUARE;
    s2ch.thB[0].reload	= PSP_CTRL_TRIANGLE;
    s2ch.thB[0].shift	= PSP_CTRL_RTRIGGER;
    s2ch.thB[1].ok		= PSP_CTRL_LTRIGGER;
    s2ch.thB[1].esc	= PSP_CTRL_CROSS;
    s2ch.thB[1].move	= PSP_CTRL_SQUARE;
    s2ch.thB[1].reload	= PSP_CTRL_TRIANGLE;
    s2ch.thB[1].shift	= PSP_CTRL_RTRIGGER;

    s2ch.thB[0].sort = PSP_CTRL_CIRCLE;
    s2ch.thB[0].search = PSP_CTRL_TRIANGLE;
    s2ch.thB[0].search2ch = PSP_CTRL_SQUARE;
    s2ch.thB[1].sort = PSP_CTRL_CIRCLE;
    s2ch.thB[1].search = PSP_CTRL_TRIANGLE;
    s2ch.thB[1].search2ch = PSP_CTRL_SQUARE;

    s2ch.favB[0].ok     = PSP_CTRL_CIRCLE;
    s2ch.favB[0].move   = PSP_CTRL_CROSS;
    s2ch.favB[0].change = PSP_CTRL_TRIANGLE;
    s2ch.favB[0].del    = PSP_CTRL_SQUARE;
    s2ch.favB[0].shift  = PSP_CTRL_RTRIGGER;
    s2ch.favB[1].ok     = PSP_CTRL_LTRIGGER;
    s2ch.favB[1].move   = PSP_CTRL_CROSS;
    s2ch.favB[1].change = PSP_CTRL_TRIANGLE;
    s2ch.favB[1].del    = PSP_CTRL_SQUARE;
    s2ch.favB[1].shift  = PSP_CTRL_RTRIGGER;

    s2ch.favB[0].sort      = PSP_CTRL_CIRCLE;
    s2ch.favB[0].search2ch = PSP_CTRL_SQUARE;
    s2ch.favB[0].update    = PSP_CTRL_TRIANGLE;
    s2ch.favB[0].trans     = PSP_CTRL_LTRIGGER;
    s2ch.favB[1].sort      = PSP_CTRL_CIRCLE;
    s2ch.favB[1].search2ch = PSP_CTRL_SQUARE;
    s2ch.favB[1].update    = PSP_CTRL_TRIANGLE;
    s2ch.favB[1].trans     = PSP_CTRL_CIRCLE;

    s2ch.findB[0].ok = PSP_CTRL_CIRCLE;
    s2ch.findB[0].esc = PSP_CTRL_CROSS;
    s2ch.findB[0].ita = PSP_CTRL_SQUARE;
    s2ch.findB[0].fav = PSP_CTRL_TRIANGLE;
    s2ch.findB[0].shift = PSP_CTRL_RTRIGGER;
    s2ch.findB[1].ok = PSP_CTRL_CIRCLE;
    s2ch.findB[1].esc = PSP_CTRL_CROSS;
    s2ch.findB[1].ita = PSP_CTRL_SQUARE;
    s2ch.findB[1].fav = PSP_CTRL_TRIANGLE;
    s2ch.findB[1].shift = PSP_CTRL_RTRIGGER;

    s2ch.findB[0].search2ch = PSP_CTRL_SQUARE;
    s2ch.findB[1].search2ch = PSP_CTRL_SQUARE;

    s2ch.menuWin[0].ok = PSP_CTRL_CIRCLE;
    s2ch.menuWin[0].esc = PSP_CTRL_CROSS;
    s2ch.menuWin[1].ok = PSP_CTRL_RTRIGGER;
    s2ch.menuWin[1].esc = PSP_CTRL_CROSS;

    s2ch.menuNG[0].del = PSP_CTRL_CIRCLE;
    s2ch.menuNG[0].esc = PSP_CTRL_CROSS;
    s2ch.menuNG[1].del = PSP_CTRL_RTRIGGER;
    s2ch.menuNG[1].esc = PSP_CTRL_CROSS;
}
