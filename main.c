/*
* $Id: main.c 146 2008-08-25 08:10:39Z bird_may_nike $
*/

#include <pspdisplay.h>
#include <pspkernel.h>
#include <pspsdk.h>
#include <psppower.h>
//#include <pspprof.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "psp2ch.h"
#include "pg.h"
#include "psp2chAudio.h"
#include "psp2chNet.h"
#include "psp2chWlan.h"

#define OWATA_HEAP_SIZE (-7 * 1024)
//#define OWATA_HEAP_SIZE (-9 * 1024)							// BUILD_PRX = 1�ŃR���p�C������ꍇ

/* Define the module info section */
PSP_MODULE_INFO("2ch Browser for PSP", PSP_MODULE_USER, 0, 8);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(OWATA_HEAP_SIZE); // psphtmlviewer.c ��MEM_SIZE�Ƃ̌��ˍ�����
//PSP_MAIN_THREAD_STACK_SIZE_KB(1024);
//PSP_HEAP_SIZE_KB(11*1024);

extern S_2CH s2ch; // psp2ch.c
volatile int	gExitRequest = 0;								// PSP������
void SMEMOinit(void);											// PSP������
void SMEMOend(void);											// PSP������

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	gExitRequest = 1;											// PSP������
    s2ch.running = 0;
    sceKernelExitGame();
    return 0;
}

int power_callback(int unknown, int pwrflags, void *common)
{
    /* check for power switch and suspending as one is manual and the other automatic */
    if (pwrflags & PSP_POWER_CB_POWER_SWITCH || pwrflags & PSP_POWER_CB_SUSPENDING) {
    	psp2chAudioStop();
    	psp2chDisconnect();
    } else if (pwrflags & PSP_POWER_CB_RESUMING) {
    } else if (pwrflags & PSP_POWER_CB_RESUME_COMPLETE) {
    } else if (pwrflags & PSP_POWER_CB_STANDBY) {
    } else {
    }
    sceDisplayWaitVblankStart();

	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
    int cbid;
    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
	cbid = sceKernelCreateCallback("Power Callback", power_callback, NULL);
	scePowerRegisterCallback(0, cbid);
    sceKernelSleepThreadCB();

    return 0;
}

/* sleep thread */
int sleep_thread(SceSize args, void *argp)
{
	SceCtrlData	pad,pad2;
	int			count;

	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	count = 0;
	pad2.Buttons = 0;
	pad2.Lx = pad2.Ly = 128;

	while (1){
		sceCtrlReadBufferPositive(&pad, 1);
		if (pad.Buttons & PSP_CTRL_HOLD || pad.Buttons != pad2.Buttons || abs(pad.Lx-pad2.Lx)>32 || abs(pad.Ly-pad2.Ly)>32) {
			count = 0;											// �����{�^���������Ă�����J�E���^�����Z�b�g
		}
		pad2 = pad;
		if (s2ch.cfg.sleep) count++;							// �T�X�y���h���Ԃ�0�Ȃ�J�E���g���Ȃ�
		if (count > s2ch.cfg.sleep * 60 * 2) {					// �w�莞�Ԍo�߂�����T�X�y���h�V�[�P���X�N��
			count = 0;
			scePowerTick(0);									// �T�X�y���h����
			scePowerRequestSuspend();							// �T�X�y���h�v��
		}

		sceKernelDelayThread(500000);							// 0.5�b�҂�
	}

    return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void)
{
    int thid = 0;
    thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0x0FA0, 0, 0);
    if (thid >= 0)
    	sceKernelStartThread(thid, 0, 0);
	thid = sceKernelCreateThread("sleep_thread", sleep_thread, 32, 0x800, 0, NULL);
	if(thid >= 0){
		sceKernelStartThread(thid, 0, 0);
	}
    return thid;
}

int main(int argc, char *argv[])
{
	char* ch;

	strcpy(s2ch.cwDir, argv[0]);
	ch = strrchr(s2ch.cwDir, '/');
	*ch = '\0';
	s2ch.cfg.sleep = 0;											// �����Ȃ�T�X�y���h���Ȃ��悤�Ɉꉞ�c��
	SetupCallbacks();
	// �R���g���[���n�̐ݒ�
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	SMEMOinit();												// PSP������
	if (psp2chInit() != 0)										// �t�H���g���[�h�̊֌W���瑁�߂Ɏ��s
		goto END;
	pgFontLoad();												// �t�H���g���[�h�Ɏ��s����������ŋ����I��
	pgCreateTexture();
	pgSetupGu();

	if (psp2chNetInit() < 0)
		goto END;
	if (psp2chAudioInit() < 0)
		goto END;
//	if (psp2chInit() != 0)
//		goto END;

	psp2ch(); // main loop

	SMEMOend();													// PSP������
END:
	pgDeleteTexture();
	pgFontUnload();
	pgTermGu();
	psp2chTerm();
	psp2chAudioTerm();
	psp2chNetTerm();
	scePowerSetClockFrequency(222, 222, 111);
	//gprof_cleanup();
	sceKernelExitGame();
	return 0;
}
