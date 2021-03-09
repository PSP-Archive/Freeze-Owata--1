

#ifndef __CHAR_CONV_H__
#define __CHAR_CONV_H__

int sjis2ucs(int *uni, int sjis);
int ucs2sjis(int *sjis, int uni);
void psp2chSJIS2UCS(unsigned short *dst, const unsigned char *src, unsigned int num);
void psp2chUCS2SJIS(unsigned char *dst, const unsigned short *src, unsigned int num);
void psp2chSjisToEuc(char* dst, char* src);
void psp2chEucToSjis(char* dst, char* src);
int psp2chUCS2UTF8(unsigned char *dst, const unsigned short *src);
void psp2chUrlEncode(char* dst, char* src);
void psp2chUTF82Sjis(char *dst,char *str);

#endif
