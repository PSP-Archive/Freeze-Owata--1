
#ifndef __PSP2CHERROR_H__
#define __PSP2CHERROR_H__

enum NORMAL_ERR_TYPE
{
	FILE_STAT_ERR,
	FILE_OPEN_ERR,
	DIR_MAKE_ERR,
	MEM_ALLC_ERR,
	DATA_PARSE_ERR,
	MOD_LOAD_ERR,
	NET_GET_ERR,
	NET_POST_ERR,
	NET_WLAN_ERR
};

char err_msg[32];

void psp2chNormalError(int type, const char* str);
int psp2chErrorDialog(const int buttonType, const char* fmt, ...);

#endif