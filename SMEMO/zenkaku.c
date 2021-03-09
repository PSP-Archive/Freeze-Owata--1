/*****************************************************************************
*zenkaku.c
*全角文字列表示サンプル
*****************************************************************************/
#include	<pspuser.h>
#include	<pspgu.h>
#include	<pspdisplay.h>
#include	"zenkaku.h"
/*****************************************************************************
*macros
*****************************************************************************/
/*キャッシュなしVRAM　VRAM[y][x]としてアクセス*/
#define	VRAM		((long(*)[BUF_WIDTH])(((char*)0x4000000)+0x40000000))
/*****************************************************************************
*prots
*****************************************************************************/
static	void	draw_char(int x,int y,int col,int chr);
static	void	draw_zenkaku(int x,int y,int hi,int lo,int col);
static	void	draw_hankaku(int x,int y,int ch,int col);
static	int	iskanji(int c);
static	int	chrwidth(int hi);
/*****************************************************************************
*文字をVRAMに転送
*****************************************************************************/
static	void	draw_char(int x,int y,int col,int chr)
{
	extern	const	char	shinonomefont[];
	int	dx, dy, bit, bitcnt;
	if(x > SCR_WIDTH-FONTWIDTH){
		return;
	}
	bitcnt = 0;
	bit = 0;
	chr *= 9;
	col &= 0xffffff;
	for(dy = 0 ; dy < FONTHEIGHT ; dy++) {
		for(dx = 0 ; dx < FONTWIDTH ; dx++) {
			bitcnt >>= 1;
			if(!bitcnt){
				bit = shinonomefont[chr++];
				bitcnt = 0x80;
			}
			if(bit & bitcnt){
				VRAM[y+dy][x+dx] = col;
			}
		}
	}
}
/*****************************************************************************
*半角文字の描画
*****************************************************************************/
static	void	draw_hankaku(int x,int y,int ch,int col)
{
	if(ch < 0x20){		//00-1f
		return;
	}else if(ch < 0x80){	//20-7f
		ch -= 0x20;
	}else if(ch < 0xa0){	//80-9f
		return;
	}else if(ch < 0xe0){	//a0-df
		ch -= 0x40;
	}else{			//e0-ff
		return;
	}
	draw_char(x,y,col,ch);
}
/*****************************************************************************
*全角文字の描画
*****************************************************************************/
static	void	draw_zenkaku(int x,int y,int hi,int lo,int col) {
	int	idx;
	static	const	int codetbl[] = {
/*80*/		-1, 0, 1, 2, 3,-1,-1, 4, 5, 6, 7, 8, 9,10,11,12,
/*90*/		13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,
/*e0*/		29,30,31,32,33,34,35,36,37,38,39,-1,-1,40,41,-1,
/*f0*/		-1,-1,-1,-1,-1,-1,-1,-1,42,43,44,45,46,47,-1,-1
	};
/*下位バイトのチェック*/
	if(lo < 0x40){
		return;
	}
	lo -= 0x40;
/*上位バイトからインデックスを得る*/
	idx = codetbl[hi & 0x3f];
/*表にない文字は無視*/
	if(idx == -1){
		return;
	}
/**/
	idx = (16*10)/*半角文字をスキップ*/ + idx*2*192/*codetbl１個あたり192文字*/;
	if(chrwidth(hi) == 1){	//２バイト半角
		idx += lo;
		draw_char(x,y,col,idx);
	}else{
		idx += lo*2;
		draw_char(x,y,col,idx);
		draw_char(x+FONTWIDTH,y,col,idx+1);
	}
}
/*****************************************************************************
*文字列の描画
*****************************************************************************/
void	mh_print(int x,int y,const char *str,int col)
{
	int	ch = 0,bef = 0;
	while((ch = (*str++ & 0xff)) != 0) {
		if (bef!=0) {
			draw_zenkaku(x, y, bef, ch, col);
			x += FONTWIDTH*chrwidth(bef);
			bef=0;
		} else {
			if(iskanji(ch)){
				bef = ch;
			} else {
				draw_hankaku(x,y,ch,col);
				x += FONTWIDTH;
			}
		}
		if(x > SCR_WIDTH-FONTWIDTH){	/*画面を越えさせない*/
			break;
		}
	}/*while*/
}
/*****************************************************************************
*全角一バイト目かどうか判定
*****************************************************************************/
static	int	iskanji(int c)
{
	c &= 0xff;
	return ((c >= 0x81) && (c < 0xa0)) || ((c >= 0xe0) && (c <= 0xfd));
}
/*****************************************************************************
*文字の描画幅を文字単位で返す
*****************************************************************************/
static	int	chrwidth(int hi)
{
	hi &= 0xff;
	if(hi == 0xfd){ //２バイト半角
		return 1;
	}
	return iskanji(hi) ? 2 : 1;
}
