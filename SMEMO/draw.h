//==============================================================
// 画面作画関連
// STEAR 2009
//--------------------------------------------------------------

#ifndef DRAW_H
#define DRAW_H

#define SCRBUF0		(char*)(0)
#define SCR_WIDTH	(480)										// 画面横ピクセル数
#define SCR_HEIGHT	(272)										// 画面縦ピクセル数

/*キャッシュなしVRAM  VRAM[y][x]としてアクセス*/
#define VRAM		((long(*)[512])(((char*)0x4000000)+0x40000000))

//----- 構造体 -----

typedef struct
{
	int		wx;
	int		wy;
	int		width;
	long	*data;
} DrawImg;

//----- プロトタイプ宣言 -----

void	HLine(int x,int y,int wx,long cor);
void	VLine(int x,int y,int wy,long cor);
void	Box(int x,int y,int wx,int wy,long corFrame);
void	Fill(int x,int y,int wx,int wy,long corIn);
void	BoxFill(int x,int y,int wx,int wy,long corFrame,long corIn);
void	XFill(int x,int y,int wx,int wy,long corIn);
void	CurveBox(int x,int y,int wx,int wy,int type,long corFrm,long corIn);
void	BoxClr(int x,int y,int xw,int yw);
void	ImgMove(int sx,int sy,int wx,int wy,int dx,int dy);
DrawImg	*DrawImgCreate(int wx,int wy);
void	DrawImgFree(DrawImg *image);
void	BoxCopy(DrawImg* image,int x,int y);
void	BoxPaste(DrawImg* image,int x,int y);
DrawImg	*ScreenCopy(void);
void	ScreenPaste(DrawImg* image);
int		pf_setIntraFont(void);
void	pf_endIntraFont(void);
int		pf_setMonaFont(int no,char *pathA,char *pathW);
void	pf_endMonaFont(void);
int		pf_fontType(int font);
float	pf_print(float x,int y,const char *str,long cor);
float	pf_print2(float x,int y,int font,const char *str,long cor);
float	GetStrWidth(int font,const char *str);
void	ChkText(int font,char *text,int *maxLen,int *line,int *wx);
void	DrawText(int x,int y,int font,char *text,long corStr);
void	DiaBox1(int x,int y,int font,char *text,long corStr,long corFrm,long corIn);
void	DiaBox2(int x,int y,int font,char *text,char *key,long corStr,long corKey,long corFrm,long corIn);
int		DialogYN(char *msg,long corFrm,long corIn);
void	DialogFrame(int x,int y,int wx,int wy,char *title,char *guide,long corFrm,long corIn);
void	Progressbar1(int x,int y,int font,char *key,long corBak,long corKey,long corFrm,long corIn);
void	Progressbar2(long pos,long max,long corBar);
void	VRollBar(int x,int y,int wy,long pos,long size,long max,int flag,long corFrm,long corIn,long corBar);
int		DrawVal(int x,int y,long val,int ztype,int ctype,long corStr);
void	DrawChar(int x,int y,int code,long corStr);
void	DrawChar2(int x,int y,int code,long corStr);
void	ScreenView(void);

#endif
