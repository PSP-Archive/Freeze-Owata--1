

#include "psp2ch.h"
#include "pg.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "psp2chError.h"

extern S_2CH s2ch; // psp2ch.c

void psp2chNormalError(int type, const char* str)
{
	char buf[BUF_LENGTH];
	
	switch(type)
	{
		case FILE_STAT_ERR: sprintf(buf, "File Stat Err:\n%s", str); break;
		case FILE_OPEN_ERR: sprintf(buf, "File Open Err:\n%s", str); break;
		case DIR_MAKE_ERR: sprintf(buf, "Dir Make Err:\n%s", str); break;
		case MEM_ALLC_ERR: sprintf(buf, "Mem Alloc Err:\n%s", str); break;
		case DATA_PARSE_ERR: sprintf(buf, "DAT log Err:\n%s", str); break;
		case MOD_LOAD_ERR: sprintf(buf, "LOAD MODULE Err:\n%s", str); break;
		case NET_GET_ERR: sprintf(buf, "GET Err:\n%s", str); break;
		case NET_POST_ERR: sprintf(buf, "POST Err:\n%s", str); break;
		case NET_WLAN_ERR: sprintf(buf, "WLAN Err:\n%s", str); break;
	}
	psp2chErrorDialog(0, "%s", buf);
	return;
}

/****************
エラーダイアログ表示
buttonType 0-2:
             3:板リストに登録
             4:Be.txtの再構成
0:未定 1:YES 3:NO 4:板リストに登録/Be.txtの再構成
*****************/
int psp2chErrorDialog(const int buttonType, const char* fmt, ...)
{
    va_list list;
    char message[3000];
    char *str;
    int ret = 0;

	va_start(list, fmt);
    vsprintf(message, fmt, list);
    va_end(list);

	pgCreateTexture();
	pgFillvram(THREAD_INDEX + 14, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
	if (buttonType == 3){
		pgPrintMenuBar("  ○ 決定　　　× 戻る　　　△ 板リストに登録");	// psp2chRes.c内psp2chResJump()専用
	} else if (buttonType == 4){
		pgPrintMenuBar("  ○ 決定　　　× 戻る　　　△ Be.txtの再構\成");	// Beログイン用
	} else {
		pgPrintMenuBar("  ○ 決定　　　× 戻る");
	}

    s2ch.oldPad = s2ch.pad;
	while(s2ch.running) {
		pgSetDrawStart(10, 0, 0, 0);
		str = message;
    	while ((str = pgPrint(str, WHITE, BLUE, SCR_WIDTH)))
		{
			pgSetDrawStart(10, -1, 0, LINE_PITCH);
		}
		sceCtrlPeekBufferPositive(&s2ch.pad, 1);
		if (s2ch.pad.Buttons != s2ch.oldPad.Buttons) {
			s2ch.oldPad = s2ch.pad;
			if (s2ch.pad.Buttons & PSP_CTRL_CIRCLE) {
				ret = 1;
				break;
			}
			else if (s2ch.pad.Buttons & PSP_CTRL_CROSS) {
				ret = 3;
				break;
			}
			else if (s2ch.pad.Buttons & PSP_CTRL_TRIANGLE && buttonType >= 3) {
				ret = 4;
				break;
			}
		}
		pgDrawTexture(-1);
		pgCopyMenuBar();
		flipScreen(1);
	}
	
	pgDeleteTexture();
	
	return ret;
}
