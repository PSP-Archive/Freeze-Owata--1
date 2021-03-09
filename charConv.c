#include <stdio.h>
#include <string.h>
#include "cptbl.h"
#include "charConv.h"

#define IS_SJIS_ZENKAKU(x) ( (0x80 < x && x < 0xa0) || (0xdf < x && x < 0xf0) )

int sjis2ucs(int *uni, int sjis)
{
	static int cache = 0;
	int i, temp = sjis & 0xFF;
	if (cache)
	{
		temp = (temp << 8) | cache;
		cache = 0x00;
	}
	else if (IS_SJIS_ZENKAKU(temp))
	{
		cache = temp;
		*uni = -1;
		return 1;
	}
	for (i = 0; i < TOTAL_TBL; i++)
		if (map[i][0] == temp)
			break;
	if (i == TOTAL_TBL)
	{
		*uni = 0;
		return -1;
	}
	*uni = map[i][1];
	return 0;
}

int ucs2sjis(int *sjis, int uni)
{
	int i;
	for (i = 0; i < TOTAL_TBL; i++)
		if (map[i][1] == uni)
			break;
	if (i == TOTAL_TBL)
	{
		*sjis = 0;
		return -1;
	}
	*sjis = map[i][0];
	return 0;
}

void psp2chSJIS2UCS(unsigned short *dst, const unsigned char *src, unsigned int num)
{
	int sjis, len = 0;
	
    while (*src && (unsigned int)len < num)
    {
    	sjis2ucs(&sjis, *src++);
        if (sjis == -1)
        {
            continue;
        }
        *dst++ = (unsigned short)sjis;
        len++;
    }
    *dst = '\0';
}

void psp2chUCS2SJIS(unsigned char *dst, const unsigned short *src, unsigned int num)
{
	unsigned int sjis, len = 0;
	
    while (*src)
    {
    	if (ucs2sjis((int*)&sjis, (int)(*src++))) {
			sjis = 0xA181;										// 変換できない文字は'■'
		}
        if (sjis < 0x100)
        {
            *dst++ = sjis;
            len++;
        }
        else
        {
            if (len >= num - 2)
            {
                break;
            }
            *dst++ = sjis & 0xFF;
            *dst++ = (sjis>>8) & 0xFF;
            len += 2;
        }
        if (len >= num - 1)
        {
            break;
        }
    }
    *dst = '\0';
}

/**********************
文字列のエンコード変換
Shift JISからEUC-JPへ
**********************/
void psp2chSjisToEuc(char* dst, char* src)
{
    unsigned char hi, lo;

    while (*src)
    {
        // 半角カナ
        if ((unsigned char)*src > 0xA0 && (unsigned char)*src < 0xE0)
        {
            *dst++ = 0x8E;
            *dst++ = *src++;
        }
        // 2バイト文字
        else if ((unsigned char)*src > 0x7F)
        {
            hi = *src++;
            lo = *src++;
            hi -= (hi <= 0x9f) ? 0x71 : 0xb1;
            hi <<= 1;
            hi++;
            if (lo > 0x7f)
                lo--;
            if (lo >= 0x9e) {
                lo -= 0x7d;
                hi++;
            }
            else {
                lo -= 0x1f;
            }
            *dst++ = hi | 0x80;
            *dst++ = lo | 0x80;
        }
        // ASCII文字
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/**********************
文字列のエンコード変換
EUC-JPからShift JISへ
**********************/
void psp2chEucToSjis(char* dst, char* src)
{
    unsigned char hi, lo;

    while (*src)
    {
    	// X212(存在しないため"？"へ変換
    	if ((unsigned char)*src == 0x8F)
    	{
    		src += 3;
    		*dst++ = 0x81;
    		*dst++ = 0x48;
    	}
        // 半角カナ
        else if ((unsigned char)*src == 0x8E)
        {
            src++;
            *dst++ = *src++;
        }
        // 2バイト文字
        else if ((unsigned char)*src > 0x7F)
        {
            hi = *src++ & 0x7F;
            lo = *src++ & 0x7F;
            hi -= 0x21;
            if (hi & 1)
            {
                lo += 0x7E;
            }
            else
            {
                lo += 0x1F;
                if (lo>= 0x7F)
                {
                    lo++;
                }
            }
            hi >>= 1;
            hi += (hi <= 0x1E) ? 0x81 : 0xC1;
            *dst++ = hi;
            *dst++ = lo;
        }
        // ASCII文字
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

int psp2chUCS2UTF8(unsigned char *dst, const unsigned short *src)
{
	int len = 0;
	while (*src)
	{
		if (*src <= 0x7F)
		{
			if (dst != NULL)
				*dst++ = *src;
			len++;
		}
		else if (*src >= 0x80 && *src <= 0x07FF)
		{
			if (dst != NULL)
			{
				*dst++ = ((*src >> 6) & 0x1F) | 0xC0;
				*dst++ = (*src & 0x3F) | 0x80;
			}
			len += 2;
		}
		else
		{
			if (dst != NULL)
			{
				*dst++ = (*src >> 12) | 0xE0;
				*dst++ = ((*src >> 6) & 0x3F) | 0x80;
				*dst++ = (*src & 0x3F) | 0x80;
			}
			len += 3;
		}
		src++;
	}
	if (dst != NULL)
		*dst = '\0';
	return len;
}

/*********************
src文字列をURLエンコードしてdst文字列に格納　
dstの容量に注意(srcの3倍あればOK)
*********************/
void psp2chUrlEncode(char* dst, char* src)
{
    char buf[4];

    while (*src)
    {
        if ((*src >= 0x30 && *src <= 0x39) || // '0'-'9'
            (*src >= 0x41 && *src <= 0x59) || // 'A'-'Z'
            (*src >= 0x61 && *src <= 0x79) || // 'a'-'z'
            *src == '-' || *src == '.' || *src == '_')
        {
            *dst++ = *src++;
        }
        else if (*src == ' ')
        {
            *dst++ = '+';
            src++;
        }
        else
        {
            sprintf(buf, "%02X", *(unsigned char*)src++);
            *dst++ = '%';
            *dst++ = buf[0];
            *dst++ = buf[1];
        }
    }
    *dst = '\0';
}

//==============================================================
// UTF8→シフトJISへ文字コード変換
//--------------------------------------------------------------

void psp2chUTF82Sjis(char *dst,char *str)
{
	int		sjis, uni;

	while (*str){
		if ((unsigned char)*str <= 0x7F){						// 1バイトコード（ASCII）
			*dst++ = *str++;
		} else {
			if ((unsigned char)*str <= 0xDFU){					// 2バイトコード
				uni = ((str[0] & 0x001fu) << 6) | (str[1] & 0x003fu);
				str += 2;
			} else if ((unsigned char)*str <= 0xEFU){			// 3バイトコード
				uni = ((str[0] & 0x001fu) << 12) | ((str[1] & 0x003fu) << 6) | (str[2] & 0x003fu);
				str += 3;
			} else {
				str++;
				uni = -1;
			}
			if (ucs2sjis(&sjis, uni) == -1){
				sjis = 0x81A1;									// 変換できなかったら'■'
			}
			*dst++ = sjis & 0xFF;
			*dst++ = (sjis>>8) & 0xFF;
		}
	}
	*dst = '\0';
}
