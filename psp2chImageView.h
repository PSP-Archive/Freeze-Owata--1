/*
* $Id: psp2chImageView.h 147 2008-08-27 05:08:44Z bird_may_nike $
*/

#ifndef __PSP2CH_IMAGE_VIEW__
#define __PSP2CH_IMAGE_VIEW__

typedef struct tagBITMAPFILEHEADER {
    unsigned char  bfType[2];
    unsigned long  bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned long  bfOffBits;
} __attribute__ ((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
    unsigned long  biSize;
    long           biWidth;
    long           biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned long  biCompression;
    unsigned long  biSizeImage;
    long           biXPixPerMeter;
    long           biYPixPerMeter;
    unsigned long  biClrUsed;
    unsigned long  biClrImporant;
} __attribute__ ((packed)) BITMAPINFOHEADER;

// add
enum RET_TYPE
{
	COMPLETE,
	SAVE_ERR,
	HEAD_ERR = -0x8000000,
	LIB_ERR,
	MEM_ERR,
	SUP_ERR,
	FRM_ERR
};

int psp2chImageViewJpeg(const char* data, unsigned long length, int *menu, int *thumbFlag, int shift, char *info);
int psp2chImageViewPng(const char* data, unsigned long length, int *menu, int *thumbFlag, int shift, char *info);
int psp2chImageViewBmp(char* data, unsigned long length,int *menu, int *thumbFlag, int shift, char *info);
int psp2chImageViewGif(const char* data, unsigned long length, int *menu, int *thumbFlag, int shift, char *info);
int saveImage(const char* path, const char* image, unsigned long length);
int psp2chMenuImageView(char* file, int size, int *menu, int *thumbFlag, int type);

#endif
