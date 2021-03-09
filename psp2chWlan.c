/*
* $Id: psp2chNet.c 157 2008-09-16 23:13:56Z bird_may_nike $
*/

#include "psp2ch.h"
#include <psputility.h>
#include <psprtc.h>
#include <pspwlan.h>
#include <pspnet_apctl.h>
#include <stdio.h>
#include "psp2chNet.h"
#include "pg.h"
#include "utf8.h"
#include "libCat/Cat_Network.h"

static int connected = 0;

// �`�揈��
// �_�C�A���O�\���̂����̔w�i��`�悷��
static void draw_callback( void* pvUserData )
{
	pgGuRender();
}

// �X�V����
// �t���[���o�b�t�@�̃X���b�v�Ȃ�
static void screen_update_callback( void* pvUserData )
{
	flipScreen(0);
}

/*****************************
�w�肳�ꂽ�ݒ�ԍ��̃A�N�Z�X�|�C���g�ɐڑ����܂�
psp2chApConnect()����Ă΂�܂��B
*****************************/
static int connect_to_apctl(int config)
{
    int state, ret = -1;
    clock_t time = sceKernelLibcClock() + 1000000 * 8; // 8�b��
	
    if (sceNetApctlConnect(config) != 0)
    {
        psp2chNormalError(NET_WLAN_ERR, "connect");
        time = 0;
    }
    while (sceKernelLibcClock() < time && ret < 0)
    {
        if (sceNetApctlGetState(&state) != 0)
        {
            psp2chNormalError(NET_WLAN_ERR, "state");
            break;
        }
        if (state < 3)
        {
            pgPrintMenuBar("�@AP�ɐڑ���");
            //stateLast = state;
        }
        else if (state == 3)
        {
            pgPrintMenuBar("�@IP�A�h���X���擾��");
            //stateLast = state;
        }
        else if (state == 4)
        {
        	pgPrintMenuBar("�@�ڑ�����");
            ret = 0;  // connected with static IP
        }
		pgCopyMenuBar();
        flipScreen(0);
    }
    return ret;
}

/*****************************
PSP���̃A�N�Z�X�|�C���g�ݒ���������܂�
�ݒ肪2�ȏ゠��ΑI���_�C�A���O��\�����܂��B
�ݒ肪1�݂̂̏ꍇ�͂��̒l�Őڑ����܂��B
�ݒ肪�Ȃ��ꍇ��-1���Ԃ���܂��B
*****************************/
int psp2chApConnect(void)
{
    struct
    {
        int index;
        char name[128];
    } ap[2];
    int count = 0;
    int i;
	
	if (connected) {
		// IP�擾�ς�
		return 0;
	}
    if (sceWlanGetSwitchState() == 0) {
        pgPrintMenuBar("    LAN�̃X�C�b�`���I�t");
        pgCopyMenuBar();
        flipScreen(0);
        pgWaitVn(30);
        return -2;
    }
    if (sceWlanDevIsPowerOn() == 0) {
        pgPrintMenuBar("    LAN�̓d���������ĂȂ�");
        pgCopyMenuBar();
        flipScreen(0);
        pgWaitVn(30);
        return -2;
    }
    for (i = 1; i < 100; i++) // skip the 0th connection
    {
        if (sceUtilityCheckNetParam(i) != 0)
        	continue;  // more
        sceUtilityGetNetParam(i, 0, (netData*) ap[count].name);
        ap[count].index = i;
        count++;
        if (count > 1)
        	break;  // no more room
    }
    if (count == 1)
    {
        i = connect_to_apctl(ap[0].index);
    }
    else if (count > 1)
    {
        // �Ԃ�l < 0 : �G���[
        // �Ԃ�l = 0 : �ڑ�����
        // �Ԃ�l > 0 : �L�����Z�����ꂽ
        i = Cat_NetworkConnect( draw_callback, screen_update_callback, 0 );
        // �S��0�Ȃ̂��ȁH�H�H
    }
    // �ڑ�����
    if (i == 0)
    	connected = 1;
    return i;
}

inline int psp2chIsConnect(void)
{
	int state;
	if (sceNetApctlGetState(&state) < 0) {
    	pgPrintMenuBar("    LAN��Ԃ̎擾�s��");
    	pgCopyMenuBar();
    	flipScreen(0);
    	pgWaitVn(30);
    	connected = 0;
    }
    if (state == PSP_NET_APCTL_STATE_GOT_IP)
    	connected = 1;
    else
    	connected = 0;
	return connected;
}

inline void psp2chDisconnect(void)
{
	sceNetApctlDisconnect();
	connected = 0;
}
