
#include "psp2ch.h"
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <psputility.h>
#include <psputility_htmlviewer.h>
#include "pg.h"
#include "psphtmlviewer.h"
#include "utf8.h"

#define LARGE_MEM_SIZE (40 * 1024 * 1024)						// �g�p���郁�����̍ő�l
#define MEM_SIZE (2 * 1024 * 1024)								// �g�p���郁�����̍ŏ��l
#define MEM_STEP (0.1 * 1024 * 1024)							// �������𒲂ׂ�Ԋu�i���������������E�ɋ߂Â��邯�ǎ��Ԃ�������j

extern S_2CH s2ch;

void pspShowBrowser(char *url, char *dldir)
{
    int done=0,mem;
    pspUtilityHtmlViewerParam html;
    SceSize html_size = sizeof(html);

    memset(&html, 0, html_size);
    html.base.size = html_size;
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE,&html.base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN, &html.base.buttonSwap);
    html.base.graphicsThread = 0x11;
    html.base.accessThread = 0x13;
    html.base.fontThread = 0x12;
    html.base.soundThread = 0x10;

	for (mem = LARGE_MEM_SIZE; mem>MEM_SIZE ;mem-=MEM_STEP){	// �m�ۂł��郁�����̍ő�l�𒲂ׂ�
		html.memaddr = memalign(32, mem);
		free(html.memaddr);
		if (html.memaddr) break;								// �������̎擾���ł�����m��
	}
	if (mem == MEM_SIZE){										// �������̊m�ۂɎ��s
		char	buf[64];
		sprintf(buf, "%dbyte �m�ۂł��܂���ł���", mem);
		psp2chNormalError(MEM_ALLC_ERR, buf);
		return;
	}
	html.memsize = mem;											// �����_�Ŋm�ۂł��郁�����̍ő�
	html.unknown1 = 0;
	html.unknown2 = 0;
	html.initialurl = url;
	html.numtabs = s2ch.cfg.browserTabs;
	if (html.numtabs<1) html.numtabs = 1;
	html.interfacemode = PSP_UTILITY_HTMLVIEWER_INTERFACEMODE_FULL;
	html.options = PSP_UTILITY_HTMLVIEWER_DISABLE_STARTUP_LIMITS | PSP_UTILITY_HTMLVIEWER_ENABLE_FLASH;
	// WITHOUT 'ms0:' on the paths
	html.dldirname = dldir;
	html.cookiemode = PSP_UTILITY_HTMLVIEWER_COOKIEMODE_DEFAULT;
	html.unknown3 = 0;
	html.homeurl = url;
	html.textsize = PSP_UTILITY_HTMLVIEWER_TEXTSIZE_NORMAL;
	html.displaymode = PSP_UTILITY_HTMLVIEWER_DISPLAYMODE_SMART_FIT;
	html.connectmode = PSP_UTILITY_HTMLVIEWER_CONNECTMODE_MANUAL_ALL;
	html.disconnectmode = s2ch.cfg.browserDisconnect;
	
	html.memaddr = memalign(32, html.memsize);
	if (!html.memaddr)											// ���ʂȂ炠�肦�Ȃ�
	{
		psp2chErrorDialog(0, "PSP�����u���E�U�p���[�N�������擾�Ɏ��s");
		return;
	}
    if (sceUtilityHtmlViewerInitStart(&html) < 0)
	{
		done = 1;
		psp2chErrorDialog(0, "PSP�����u���E�U�̋N���Ɏ��s���܂���");
	}

    while(!done)
    {
        pgGuRender();

        switch(sceUtilityHtmlViewerGetStatus())
        {
            case PSP_UTILITY_DIALOG_NONE:
            	done = 1;
                break;
            case PSP_UTILITY_DIALOG_INIT :
                break;
            case PSP_UTILITY_DIALOG_VISIBLE:
                sceUtilityHtmlViewerUpdate(1);
                break;
            case PSP_UTILITY_DIALOG_QUIT:
                sceUtilityHtmlViewerShutdownStart();
                break;
            case PSP_UTILITY_DIALOG_FINISHED:
                break;
        }
        flipScreen(1);
    }
	free(html.memaddr);
	return;
}
