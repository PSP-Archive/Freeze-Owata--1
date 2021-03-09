/*
* $Id: pg.h 126 2008-06-24 08:52:25Z bird_may_nike $
*/

#ifndef __PG_H__
#define __PG_H__

#define BUF_WIDTH (512)
#define BUF_HEIGHT (512)
#define SCR_WIDTH (480)
#define SCR_HEIGHT (272)
#define FONT_HEIGHT (s2ch.font.height)
#define LINE_PITCH (s2ch.font.pitch)
#define DRAW_LINE_H (s2ch.font.lineH)
#define DRAW_LINE_V (s2ch.font.lineV)
#define FRAME_SIZE (BUF_WIDTH * SCR_HEIGHT)
#define ZBUF_SIZE (BUF_WIDTH * BUF_HEIGHT)
//#define RGB(r,g,b) (0xF000|(((b>>4) & 0xF)<<8)|(((g>>4) & 0xF)<<4)|((r>>4) & 0xF))
//#define RGB(r,g,b) (0x8000|(((b>>3) & 0x1F)<<10)|(((g>>3) & 0x1F)<<5)|((r>>3) & 0x1F))
#define WHITE 3
#define BLACK 4
#define RED 5
#define GREEN 6
#define BLUE 7
#define YELLOW 8
#define MAGENTA 9
#define CYAN 10
#define GRAY 11

#define RES_INDEX 16
#define RES_ANCHER_INDEX 32
#define MENUBAR_INDEX 48
#define THREAD_INDEX 64
#define CATEGORY_INDEX 80
#define ITA_INDEX 96
#define FORM_INDEX 112
#define MENU_WINDOW_INDEX 128
#define CURSOR_INDEX 144

#include "psp2ch.h"

typedef struct {
	unsigned char *frame;
	int tateFlag;
	int width;
	int height;
	int lineEnd;
	int pgCursorX;
	int pgCursorY;
	int viewX;
	int viewY;
} Window_Layer;

struct Vertex
{
	unsigned short u, v;
	unsigned char color;
	short x, y, z;
};

typedef struct rect_t
{
	short left;
	short top;
	short right;
	short bottom;
} RECT;

typedef struct tex
{
	int w;
	int h;
	int tb;
} TEX;

struct entityTag
{
	char* str;
	int len;
	int byte;
	char c1;
	char c2;
};

typedef struct {
	unsigned char color;
	unsigned char bgcolor;
	int width;
} S_PUTCHAR;

void pgCursorColorSet(void);
void pgErrorDialog(int type,char *path);						// PSPÉÅÉÇí†
int pgExtraFontInit(void);
void pgSetupGu(void);
void pgTermGu(void);
void pgFontLoad(void);
void pgFontUnload(void);
void psp2chSetFontParam(void);
void psp2chSetBarParam(void);
void psp2chSetScreenParam(int axis);
void pgCLUTUpdate(void);
void pgWaitVn(unsigned long count);
void pgFillvram(unsigned char color, int x1, int y1, unsigned int w, unsigned int h, int wide);
void pgPrintTitleBar(char* ita, char* title,int size);
void pgCopyTitleBar(void);
void pgPrintMenuBar(char* str);
void pgCopyMenuBar(void);
void pgEditBox(unsigned char color, int x1, int y1, int x2, int y2);
void pgWindowFrame(int x1, int y1, int x2, int y2);
void pgScrollbar(S_SCROLLBAR* bar, S_2CH_BAR_COLOR c);
void pgPadCursor(int x, int y);
void pgCopyWindow(int offset, int x, int y, int w, int h);
void pgCopy(int offsetX, int offsetY);
void pgPrintNumber(int num, int color,int bgcolor);
char* pgPrint(char *str, unsigned char color, unsigned char bgcolor, int width);
char* pgPrintHtml(char *str,S_2CH_RES_COLOR *c, int startX, int width,int drawLine);
char* pgCountHtml(char *str, int width, int specialchar);
void pgPrintOwata(void);
void flipScreen(int mode);
void pgReDraw(void);
void pgRewrite(void);
// Texture
int pgCreateTexture(void);
void pgDeleteTexture(void);
void pgDrawTexture(int num);
void pgSetDrawStart(int x, int y, int addX, int addY);
void pgGuRender(void);

#endif
