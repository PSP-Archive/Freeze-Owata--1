/*
* $Id: psp2chImageView.c 150 2008-08-29 05:55:20Z bird_may_nike $
*/

#include "psp2ch.h"
#include <pspgu.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <setjmp.h> /* setjmp()/longjmp() */
#include <jpeglib.h>
#include <png.h>
#include <gif_lib.h>
#include "pg.h"
#include "psp2chImageView.h"
#include "psp2chImageReader.h" /* add for read from memory */
#include "charConv.h"

#define EX_MEMORRY (0x0a000000)
#define LIMIT_WIDTH (1024)		// 画像の最大表示WIDTH
#define PNG_BYTES_TO_CHECK (8)	// PNGの先頭ヘッダサイズ
#define GIF_FRAME_MAX (100)		// アニメーションGIFで扱える最大画像数

extern S_2CH s2ch; //psp2ch.c
extern unsigned int list[BUF_WIDTH*BUF_HEIGHT]; // pg.c
extern void* framebuffer; // pg.c
extern int preLine; // psp2chRes.c

typedef struct {
	int		startX;
	int		startY;
	int		width2;
	int		height2;
	double	thumb;
} strPicView;

// prototype
METHODDEF(void) my_error_exit(j_common_ptr cinfo);
static void myPngError(png_structp png_ptr,png_const_charp message);
static void blt(void *src, TEX *tex, int sw, int sh, int dw, int dh);

/* jpegエラールーチン */
METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	my_error_ptr err = (my_error_ptr)cinfo->err;
	(*cinfo->err->format_message) (cinfo, buffer);
	//psp2chErrorDialog(0, "%s", buffer);
	longjmp(err->setjmp_buffer, 1);
}

/* PNGエラールーチン */
static void myPngError(png_structp png_ptr,png_const_charp message)
{
	//psp2chErrorDialog(0, "%s", message);
	longjmp(png_ptr->jmpbuf, 1);
}

/*****************************
	矩形範囲を拡大縮小
*****************************/
#define SLICE_SIZE 64
static void blt(void *src, TEX *tex, int sw, int sh, int dw, int dh)
{
	int i, j, dy;
	struct Vertex *vertices;
	int* p = src;

	dy = BUF_HEIGHT * dw / sw;

	sceGuStart(GU_DIRECT, list);
	sceGuDrawBufferList(GU_PSM_8888, framebuffer, BUF_WIDTH);
	sceGuScissor(0, 0, SCR_WIDTH, SCR_HEIGHT);
	sceGuClearColor(GU_RGBA(128,128,128,255));
	sceGuClear(GU_COLOR_BUFFER_BIT);
	sceGuTexMode(GU_PSM_8888, 0, 0, GU_FALSE);
	sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
	sceGuTexWrap(GU_CLAMP, GU_CLAMP);
	if (sw == dw)
		sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	else
		sceGuTexFilter(GU_LINEAR, GU_LINEAR);
	i = 0;
	/* textureをSLICE_SIZE * BUF_HEIGHTに切り取ってループ */
	while (sh > BUF_HEIGHT)
	{
		for (j = 0; (j + SLICE_SIZE) < sw; j = j + SLICE_SIZE)
		{
			sceGuTexImage(0, tex->w, tex->h, tex->tb, p + j);
			vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));
			vertices[0].u = 0;
			vertices[0].v = 0;
			vertices[0].x = j * dw / sw;
			vertices[0].y = dy * i;
			vertices[1].u = SLICE_SIZE;
			vertices[1].v = BUF_HEIGHT;
			vertices[1].x = (j + SLICE_SIZE) * dw / sw;
			vertices[1].y = dy * (i + 1);
			sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_4444 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, vertices);
		}
		if (j < sw)
		{
			sceGuTexImage(0, tex->w, tex->h, tex->tb, p + j);
			vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));
			vertices[0].u = 0;
			vertices[0].v = 0;
			vertices[0].x = j * dw / sw;
			vertices[0].y = dy * i;
			vertices[1].u = sw - j;
			vertices[1].v = BUF_HEIGHT;
			vertices[1].x = dw;
			vertices[1].y = dy * (i + 1);
			sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_4444 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, vertices);
		}
		sh -= BUF_HEIGHT;
		p += tex->tb * BUF_HEIGHT;
		i++;
	}
	for (j = 0; (j + SLICE_SIZE) < sw; j = j + SLICE_SIZE)
	{
		sceGuTexImage(0, tex->w, tex->h, tex->tb, p + j);
		vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));
		vertices[0].u = 0;
		vertices[0].v = 0;
		vertices[0].x = j * dw / sw;
		vertices[0].y = dy * i;
		vertices[1].u = SLICE_SIZE;
		vertices[1].v = sh;
		vertices[1].x = (j + SLICE_SIZE) * dw / sw;
		vertices[1].y = dh;
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_4444 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, vertices);
	}
	if (j < sw)
	{
		sceGuTexImage(0, tex->w, tex->h, tex->tb, p + j);
		vertices = (struct Vertex *)sceGuGetMemory(2 * sizeof(struct Vertex));
		vertices[0].u = 0;
		vertices[0].v = 0;
		vertices[0].x = j * dw / sw;
		vertices[0].y = dy * i;
		vertices[1].u = sw - j;
		vertices[1].v = sh;
		vertices[1].x = dw;
		vertices[1].y = dh;
		sceGuDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_4444 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, 2, NULL, vertices);
	}
	sceGuFinish();
	sceGuSync(GU_SYNC_FINISH, GU_SYNC_WHAT_DONE);
}

/*****************************
メニュー処理
RGBAデータをVRAMに転送
*****************************/
static int psp2chImageViewer(int* img[], int width, int height, int *menu, int *thumbFlag, int shift, char *info)
{
	int startX, startY, width2, height2;
	double thumb;
	int padX, padY;
	int ret = COMPLETE;
	double thumbW, thumbH;
	int imgWH, imgHW, sx, sy;
	TEX tex;
	int **imgBuf = NULL;
	int* tmpbuf = NULL;
	int originalWidth = width;

	// Gu転送のため1行を16バイト境界にそろえる
	tex.tb = (width + 15) & 0xFFFFFFF0;
	tex.w = BUF_WIDTH;
	tex.h = BUF_HEIGHT;

	// 幅の大きい画像は縮小する
	if (originalWidth > LIMIT_WIDTH)
	{
		int i, j, k;
		int mip = originalWidth >> 10;
		mip++;
		
		i =  height / mip;
		imgBuf = memalign(16, sizeof(int *) * i);
		if (!imgBuf)
		{
			return MEM_ERR;
		}
		tmpbuf = memalign(16, sizeof(int) * LIMIT_WIDTH * i);
		if (!tmpbuf)
		{
			free(imgBuf);
			return MEM_ERR;
		}
		for (j = 0; j < i; j++)
		{
			imgBuf[j] = tmpbuf + (LIMIT_WIDTH * j);
			for (k = 0; k < width; k += mip)
			{
				imgBuf[j][k / mip] = img[j * mip][k];
			}
		}
		width /= mip;
		height /= mip;
		tex.tb = LIMIT_WIDTH;
		img = imgBuf;
	}
	thumbW = (double)width / SCR_WIDTH;		// 画面幅に合わせるための拡大縮小率
	imgWH = height / thumbW;				// 画面幅に合わせたときの画像高さ
	thumbH = (double)height / SCR_HEIGHT;	// 画面高さにあわせるための拡大縮小率
	imgHW = width / thumbH;					// 画面高さにあわせたときの画像幅

	startX = 0;
	startY = 0;
	switch (*thumbFlag){
	case 0:
		thumb = 1.0;
		width2 = width;
		height2 = height;
		break;
	case 1:
		thumb = thumbW;
		width2 = SCR_WIDTH;
		height2 = imgWH;
		break;
	case 2:
		thumb = thumbH;
		width2 = imgHW;
		height2 = SCR_HEIGHT;
		break;
	default:
		if (width / height >= 1.78) {
			thumb = thumbW;
			width2 = SCR_WIDTH;
			height2 = imgWH;
			*thumbFlag = 1;
		}
		else {
			thumb = thumbH;
			width2 = imgHW;
			height2 = SCR_HEIGHT;
			*thumbFlag = 2;
		}
	}

	// 画像表示メインループ
	while (s2ch.running)
	{
		if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
		{
			if (shift){
				pgPrintMenuBar(" ○ : 拡大縮小  × : 戻る  △ : メニューオン・オフ  □ : 戻る  ＬＲ : 画像送り");
			} else {
				pgPrintMenuBar(" ○ : 拡大縮小  × : 画像を保存して戻る  △ : メニューオン・オフ  □ : 画像を削除して戻る");
			}
			if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
			{
				s2ch.oldPad = s2ch.pad;
				if(s2ch.pad.Buttons & PSP_CTRL_CIRCLE)			// 拡大率
				{
					(*thumbFlag)++;
					if (*thumbFlag > 2)
					{
						*thumbFlag = 0;
					}
					if (*thumbFlag == 1)
					{
						thumb = thumbW;
						width2 = SCR_WIDTH;
						height2 = imgWH;
					}
					else if (*thumbFlag == 2)
					{
						thumb = thumbH;
						width2 = imgHW;
						height2 = SCR_HEIGHT;
					}
					else
					{
						thumb = 1.0;
						width2 = width;
						height2 = height;
					}
					startX = 0;
					startY = 0;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_CROSS)		// 表示終了
				{
					ret = SAVE_ERR;
					break;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_TRIANGLE)	// メニューON/OFF
				{
					(*menu) ^= 0x01;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_SELECT)		// プロパティ
				{
					(*menu) ^= 0x02;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_SQUARE)		// 削除
				{
					break;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_RTRIGGER && shift){	// 画像送り用
					break;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_LTRIGGER && shift){	// 画像送り用
					break;
				}
			}
			if(s2ch.pad.Buttons & PSP_CTRL_UP)
			{
				startY -= 2;
			}
			if(s2ch.pad.Buttons & PSP_CTRL_DOWN)
			{
				startY += 2;
			}
			if(s2ch.pad.Buttons & PSP_CTRL_LEFT)
			{
				startX -= 2;
			}
			if(s2ch.pad.Buttons & PSP_CTRL_RIGHT)
			{
				startX += 2;
			}
			padX = s2ch.pad.Lx - 127;
			padY = s2ch.pad.Ly - 127;
			if ((padX < -s2ch.cfg.padCutoff) || (padX > s2ch.cfg.padCutoff))
			{
				startX += (padX)/4;
			}
			if ((padY < -s2ch.cfg.padCutoff) || (padY > s2ch.cfg.padCutoff))
			{
				startY += (padY)/4;
			}
		}
		// width : 元画像幅
		// height : 元画像高さ
		// sx.sy : 元画像における表示開始位置
		// width2 : 表示画像幅
		// height2 : 表示画像高さ
		// startX.startY : 表示画像における表示開始位置
		if (startX >= width2 - SCR_WIDTH)
		{
			startX = width2 - SCR_WIDTH;
		}
		if (startY >= height2 - SCR_HEIGHT)
		{
			startY = height2 - SCR_HEIGHT;
		}
		if (startX < 0)
		{
			startX = 0;
		}
		if (startY < 0)
		{
			startY = 0;
		}
		sx = thumb * startX;
		sy = thumb * startY;
		if (originalWidth > LIMIT_WIDTH)
		{
			blt(img[sy] + sx, &tex, width - sx, height - sy, width2 - startX, height2 - startY);
		}
		else
		{
			blt(img[sy] + sx, &tex, width - sx, height - sy, width2 - startX, height2 - startY);
		}
		if ((*menu) & 0x01)
		{
			pgCopyMenuBar();
		}
		if (((*menu) & 0x02) && info){
			char *str,msg[256],buf[32];
			strcpy( msg, info );
			sprintf( buf, "\n倍率 %f", 1/thumb );
			strcat( msg, buf );
			str = msg;
			pgFillvram(THREAD_INDEX + 14, 0, 0, 6*25, 12*7.5, 2);
			pgSetDrawStart(10, 6, 0, 0);
			while ((str = pgPrint(str, WHITE, WHITE, SCR_WIDTH))){
				pgSetDrawStart(10, -1, 0, LINE_PITCH);
			}
			pgCopy(0,0);										// プロパティ
		}
		flipScreen(0);
	}

	free(tmpbuf);
	free(imgBuf);
	return ret;
}

/*****************************
jpegファイルを読み込んで32ビットRGBAに変換
*****************************/
int psp2chImageViewJpeg(const char* data, unsigned long length, int *menu, int *thumbFlag, int shift, char *info)
{
	JSAMPARRAY img = NULL;
	JSAMPROW buf = NULL, imgbuf = NULL;
	unsigned long width, height, i;
	int ret = COMPLETE;
	struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;	/* add */
	cinfo.err = jpeg_std_error(&jerr.pub);

	if ((u8)data[0] != 0xFF || (u8)data[1] != 0xD8)
	{
		return HEAD_ERR;
	}
	/* add */
	jerr.pub.error_exit = my_error_exit;
	if(setjmp(jerr.setjmp_buffer)) /* error時のジャンプ先 */
	{
		ret = LIB_ERR;
		goto jpeg_Error;
	}
	// 伸張開始
	jpeg_create_decompress(&cinfo);
	jpeg_memory_src(&cinfo, (void*)data, length);
	jpeg_read_header(&cinfo, TRUE);
	if (info){
		sprintf(info, "ファイルサイズ %dKB\n画像サイズ X=%d Y=%d\nカラー種別 %d", length/1024, cinfo.image_width, cinfo.image_height, cinfo.out_color_components);
	}
	// 大きすぎる場合にこちらで縮小(メモリのため)
	while ((cinfo.image_width / cinfo.scale_denom) > LIMIT_WIDTH)
	{
		cinfo.scale_denom += cinfo.scale_denom;
	}
	jpeg_calc_output_dimensions(&cinfo);
	jpeg_start_decompress(&cinfo);
	// Gu転送のため1行を16バイト境界にそろえる
	width = (cinfo.output_width + 15) & 0xFFFFFFF0;
	height = cinfo.output_height;
	img = memalign(16, sizeof(JSAMPROW) * height);
	if (!img)
	{
		ret = MEM_ERR;
		goto jpeg_Error;
	}
	buf = memalign(16, sizeof(JSAMPLE) * cinfo.output_components * width);
	if (!buf)
	{
		ret = MEM_ERR;
		goto jpeg_Error;
	}
	imgbuf = memalign(16, sizeof(JSAMPLE) * 4 * width * height);
	if (!imgbuf)
	{
		ret = MEM_ERR;
		goto jpeg_Error;
	}
	for (i = 0; i < height; i++ )
	{
		img[i] = &imgbuf[i * width * 4];
	}
	// RGBカラーをRGBAに
	if (cinfo.out_color_components == 3)
	{
		while(cinfo.output_scanline < cinfo.output_height)
		{
			jpeg_read_scanlines(&cinfo, &buf, 1);
			for (i = 0; i < cinfo.output_width; i++)
			{
				img[cinfo.output_scanline-1][i * 4 + 0] = buf[i * 3 + 0];
				img[cinfo.output_scanline-1][i * 4 + 1] = buf[i * 3 + 1];
				img[cinfo.output_scanline-1][i * 4 + 2] = buf[i * 3 + 2];
				img[cinfo.output_scanline-1][i * 4 + 3] = 0xFF;
			}
		}
	}
	// グレースケールをRGBAに
	else if (cinfo.out_color_components == 1)
	{
		while(cinfo.output_scanline < cinfo.output_height)
		{
			jpeg_read_scanlines(&cinfo, &buf, 1);
			for (i = 0; i < cinfo.output_width; i++)
			{
				img[cinfo.output_scanline-1][i * 4 + 0] = buf[i];
				img[cinfo.output_scanline-1][i * 4 + 1] = buf[i];
				img[cinfo.output_scanline-1][i * 4 + 2] = buf[i];
				img[cinfo.output_scanline-1][i * 4 + 3] = 0xFF;
			}
		}
	}
	else
	{
		ret = SUP_ERR;
		goto jpeg_Error;
	}
	jpeg_finish_decompress(&cinfo);
	// 表示
	ret = psp2chImageViewer((int**)img, cinfo.output_width, height, menu, thumbFlag, shift, info);
jpeg_Error:
	// 終了処理
	jpeg_destroy_decompress(&cinfo);
	if(imgbuf)
		free(imgbuf);
	if(buf)
		free(buf);
	if(img)
		free(img);
	preLine = -2;
	return ret;
}

/*****************************
PNGファイルを読み込んで32ビットRGBAに変換
*****************************/
int psp2chImageViewPng(const char* data, unsigned long length, int *menu, int *thumbFlag, int shift, char *info)
{
	my_img_buffer png_buf;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
	unsigned long width, height, width2, i;
	int bit_depth, color_type, interlace_type, ret = COMPLETE;
	png_bytepp img = NULL;
	png_bytep imgbuf = NULL;

	png_buf.data = (unsigned char*)data;
	png_buf.length = length;
	// PNGチェック
	if (png_sig_cmp((png_bytep)png_buf.data, 0, PNG_BYTES_TO_CHECK) != 0)
	{
		return HEAD_ERR;
	}
	png_buf.offset = PNG_BYTES_TO_CHECK;
	png_buf.data += PNG_BYTES_TO_CHECK;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	end_info = png_create_info_struct(png_ptr);
	//エラーハンドラ
	if (setjmp(png_ptr->jmpbuf))
	{
		ret = LIB_ERR;
		goto png_Error;
	}
	// IO変更
	png_set_read_fn(png_ptr, (void*)&png_buf, (png_rw_ptr)png_memread_func);
	png_set_error_fn(png_ptr, NULL, myPngError, NULL);
	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
	if (info){
		sprintf(info, "ファイルサイズ %dKB\n画像サイズ X=%d Y=%d\nカラー種別 %X\nビット数 %d\nインターレース %d", length/1024, width, height, color_type, bit_depth, interlace_type);
	}
	//パレット系->RGB系に拡張
	if (color_type == PNG_COLOR_TYPE_PALETTE ||
		(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) ||
		png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	{
		png_set_expand(png_ptr);
	}
	//16ビット->8ビットに落とす
	if (bit_depth > 8)
	{
		png_set_strip_16(png_ptr);
	}
	//グレースケール->RGBに拡張
	if (color_type==PNG_COLOR_TYPE_GRAY || color_type==PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(png_ptr);
	}
	if (color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type!=PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_add_alpha(png_ptr, 0xFFFF, PNG_FILLER_AFTER);
	}
	png_read_update_info(png_ptr, info_ptr);
	// Gu転送のため1行を16バイト境界にそろえる
	width2 = (width + 15) & 0xFFFFFFF0;
	img = memalign(16, height * sizeof(png_bytep));
	if (!img)
	{
		ret = MEM_ERR;
		goto png_Error;
	}
	// change 取得サイズを変更 暫定処置
	imgbuf = memalign(16, sizeof(unsigned char) * 4 * width2 * height);
	if (!imgbuf)
	{
		ret = MEM_ERR;
		goto png_Error;
	}
	for (i = 0; i < height; i++)
	{
		img[i] = &imgbuf[i * width2 * 4];
	}
	png_read_image(png_ptr, img);
	png_read_end(png_ptr, end_info);
	ret = psp2chImageViewer((int**)img, (int)width, (int)height, menu, thumbFlag, shift, info);
png_Error:
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	if(imgbuf)
		free(imgbuf);
	if(img)
		free(img);
	preLine = -2;
	return ret;
}

/*****************************
BMPファイルを読み込んで32ビットRGBAに変換
*****************************/
int psp2chImageViewBmp(char* data, unsigned long length,int *menu, int *thumbFlag, int shift, char *info)
{
	char* bmp_data;
	int i, j, y, width, height, len, ret = COMPLETE;
	BITMAPFILEHEADER bf;
	BITMAPINFOHEADER bi;
	unsigned char **img = NULL, *imgbuf = NULL, *buf = NULL;
	bmp_data = data;
	memcpy(&bf, bmp_data, sizeof(BITMAPFILEHEADER));
	bmp_data += sizeof(BITMAPFILEHEADER);
	// BITMAP 認識文字 "BM"
	if (memcmp(bf.bfType, "BM", 2) != 0)
	{
		return HEAD_ERR;
	}
	memcpy(&bi, bmp_data, sizeof(BITMAPINFOHEADER));
	bmp_data += sizeof(BITMAPINFOHEADER);
	if (info){
		sprintf(info, "ファイルサイズ %dKB\n画像サイズ X=%d Y=%d\nカラー種別 %d\n圧縮 %d", length/1024, bi.biWidth, bi.biHeight, bi.biBitCount, bi.biCompression);
	}
	// 非圧縮のみ
	if (bi.biCompression)
	{
		return SUP_ERR;
	}
	if (bi.biHeight < 0)
	{
		height = -bi.biHeight;
	}
	else
	{
		height = bi.biHeight;
	}
	// Gu転送のため1行を16バイト境界にそろえる
	width = (bi.biWidth + 15) & 0xFFFFFFF0;
	len = bi.biWidth * bi.biBitCount / 8;
	len += (4 - (len & 3)) & 3;
	img = memalign(16, sizeof(unsigned char*) * height);
	if (!img)
	{
		ret = MEM_ERR;
		goto bmp_Error;
	}
	buf = memalign(16, sizeof(unsigned char) * len);
	if (!buf)
	{
		ret = MEM_ERR;
		goto bmp_Error;
	};
	imgbuf = memalign(16, sizeof(unsigned char) * 4 * width * height);
	if (!imgbuf)
	{
		ret = MEM_ERR;
		goto bmp_Error;
	}
	for (i = 0; i < height; i++ )
	{
		img[i] = &imgbuf[i * width * 4];
	}
	bmp_data = data + bf.bfOffBits;
	// 24ビットBMPをRGBAに
	if (bi.biBitCount == 24)
	{
		for (j = 0; j < height; j++)
		{
			if (bi.biHeight < 0)
			{
				y = j;
			}
			else
			{
				y = height - j - 1;
			}
			memcpy(buf, bmp_data, len);
			bmp_data += len;
			for (i = 0; i < bi.biWidth; i++)
			{
				img[y][i * 4 + 0] = buf[i * 3 + 2];
				img[y][i * 4 + 1] = buf[i * 3 + 1];
				img[y][i * 4 + 2] = buf[i * 3 + 0];
				img[y][i * 4 + 3] = 0xFF;
			}
		}
	}
	// 32ビットBMPをRGBAに
	else if (bi.biBitCount == 32)
	{
		for (j = 0; j <height; j++)
		{
			if (bi.biHeight < 0)
			{
				y = j;
			}
			else
			{
				y = height - j - 1;
			}
			memcpy(buf, bmp_data, len);
			bmp_data += len;
			for (i = 0; i < bi.biWidth; i++)
			{
				img[y][i * 4 + 0] = buf[i * 4 + 2];
				img[y][i * 4 + 1] = buf[i * 4 + 1];
				img[y][i * 4 + 2] = buf[i * 4 + 0];
				img[y][i * 4 + 3] = 0xFF;
			}
		}
	}
	// 未対応
	else
	{
		ret = SUP_ERR;
		goto bmp_Error;
	}
	ret = psp2chImageViewer((int**)img, bi.biWidth, height, menu, thumbFlag, shift, info);
bmp_Error:
	if(imgbuf)
		free(imgbuf);
	if(buf)
		free(buf);
	if(img)
		free(img);
	preLine = -2;
	return ret;
}

/*****************************
RGBAデータをVRAMに転送
GIF専用
*****************************/

static int psp2chImageViewer2(int* img[], int width, int height, int *menu, int *thumbFlag, int *reset, int shift, strPicView *pv,char *info)
{
//	static int startX, startY, width2, height2;
//	static double thumb;
	int padX, padY;
	int rMenu = 0, ret = 10;
	double thumbW, thumbH;
	int imgWH, imgHW, sx, sy;
	TEX tex;
	int **imgBuf = NULL;
	int* tmpbuf = NULL;
	int originalWidth = width;

	// Gu転送のため1行を16バイト境界にそろえる
	tex.tb = (width + 15) & 0xFFFFFFF0;
	tex.w = BUF_WIDTH;
	tex.h = BUF_HEIGHT;
	// 幅の大きい画像は縮小する
	if (originalWidth > LIMIT_WIDTH)
	{
		int i, j, k;
		int mip = originalWidth >> 10;
		mip++;
		
		i =  height / mip;
		imgBuf = memalign(16, sizeof(int *) * i);
		if (!imgBuf)
		{
			return MEM_ERR;
		}
		tmpbuf = memalign(16, sizeof(int) * LIMIT_WIDTH * i);
		if (!tmpbuf)
		{
			free(imgBuf);
			return MEM_ERR;
		}
		for (j = 0; j < i; j++)
		{
			imgBuf[j] = tmpbuf + (LIMIT_WIDTH * j);
			for (k = 0; k < width; k += mip)
			{
				imgBuf[j][k / mip] = img[j * mip][k];
			}
		}
		width /= mip;
		height /= mip;
		tex.tb = LIMIT_WIDTH;
		img = imgBuf;
	}
//	width2 = width;
//	height2 = height;
//	thumb = 1.0;
	thumbW = (double)width / SCR_WIDTH;		// 画面幅に合わせるための拡大縮小率
	imgWH = height / thumbW;				// 画面幅に合わせたときの画像高さ
	thumbH = (double)height / SCR_HEIGHT;	// 画面高さにあわせるための拡大縮小率
	imgHW = width / thumbH;					// 画面高さにあわせたときの画像幅
	// 内部変数のリセット
	if (*reset){
		*reset = 0;
		pv->startX = 0;
		pv->startY = 0;
		switch (*thumbFlag){
		case 0:
			pv->thumb = 1.0;
			pv->width2 = width;
			pv->height2 = height;
			break;
		case 1:
			pv->thumb = thumbW;
			pv->width2 = SCR_WIDTH;
			pv->height2 = imgWH;
			break;
		case 2:
			pv->thumb = thumbH;
			pv->width2 = imgHW;
			pv->height2 = SCR_HEIGHT;
			break;
		default:
			if (width / height >= 1.78) {
				pv->thumb = thumbW;
				pv->width2 = SCR_WIDTH;
				pv->height2 = imgWH;
				*thumbFlag = 1;
			}
			else {
				pv->thumb = thumbH;
				pv->width2 = imgHW;
				pv->height2 = SCR_HEIGHT;
				*thumbFlag = 2;
			}
		}
	}

	if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
	{
		if (shift){
			pgPrintMenuBar(" ○ : 拡大縮小  × : 戻る  △ : メニューオン・オフ  □ : 戻る  ＬＲ : 画像送り");
		} else {
			pgPrintMenuBar(" ○ : 拡大縮小  × : 画像を保存して戻る  △ : メニューオン・オフ  □ : 画像を削除して戻る");
		}
		if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
		{
			s2ch.oldPad = s2ch.pad;
			if(s2ch.pad.Buttons & PSP_CTRL_CIRCLE)
			{
				(*thumbFlag)++;
				if (*thumbFlag > 2)
				{
					*thumbFlag = 0;
				}
				if (*thumbFlag == 1)
				{
					pv->thumb = thumbW;
					pv->width2 = SCR_WIDTH;
					pv->height2 = imgWH;
				}
				else if (*thumbFlag == 2)
				{
					pv->thumb = thumbH;
					pv->width2 = imgHW;
					pv->height2 = SCR_HEIGHT;
				}
				else
				{
					pv->thumb = 1.0;
					pv->width2 = width;
					pv->height2 = height;
				}
				pv->startX = 0;
				pv->startY = 0;
			}
			else if(s2ch.pad.Buttons & PSP_CTRL_CROSS)
			{
				ret = SAVE_ERR;
			}
			else if(s2ch.pad.Buttons & PSP_CTRL_TRIANGLE)
			{
				(*menu) ^= 0x01;
			}
			else if(s2ch.pad.Buttons & PSP_CTRL_SELECT)
			{
				(*menu) ^= 0x02;
			}
			else if(s2ch.pad.Buttons & PSP_CTRL_SQUARE)
			{
				ret = COMPLETE;
			}
			else if(s2ch.pad.Buttons & PSP_CTRL_RTRIGGER && shift){	// 画像送り用
				ret = COMPLETE;
			}
			else if(s2ch.pad.Buttons & PSP_CTRL_LTRIGGER && shift){	// 画像送り用
				ret = COMPLETE;
			}
		}
		if(s2ch.pad.Buttons & PSP_CTRL_UP)
		{
			pv->startY -= 2;
		}
		if(s2ch.pad.Buttons & PSP_CTRL_DOWN)
		{
			pv->startY += 2;
		}
		if(s2ch.pad.Buttons & PSP_CTRL_LEFT)
		{
			pv->startX -= 2;
		}
		if(s2ch.pad.Buttons & PSP_CTRL_RIGHT)
		{
			pv->startX += 2;
		}
		padX = s2ch.pad.Lx - 127;
		padY = s2ch.pad.Ly - 127;
		if ((padX < -s2ch.cfg.padCutoff) || (padX > s2ch.cfg.padCutoff))
		{
			pv->startX += (padX)/4;
		}
		if ((padY < -s2ch.cfg.padCutoff) || (padY > s2ch.cfg.padCutoff))
		{
			pv->startY += (padY)/4;
		}
	}
	// width : 元画像幅
	// height : 元画像高さ
	// sx.sy : 元画像における表示開始位置
	// width2 : 表示画像幅
	// height2 : 表示画像高さ
	// startX.startY : 表示画像における表示開始位置
	if (pv->startX >= pv->width2 - SCR_WIDTH)
	{
		pv->startX = pv->width2 - SCR_WIDTH;
	}
	if (pv->startY >= pv->height2 - SCR_HEIGHT)
	{
		pv->startY = pv->height2 - SCR_HEIGHT;
	}
	if (pv->startX < 0)
	{
		pv->startX = 0;
	}
	if (pv->startY < 0)
	{
		pv->startY = 0;
	}
	sx = pv->thumb * pv->startX;
	sy = pv->thumb * pv->startY;
	if (originalWidth > LIMIT_WIDTH)
	{
		blt(img[sy] + sx, &tex, width - sx, height - sy, pv->width2 - pv->startX, pv->height2 - pv->startY);
	}
	else
	{
		blt(img[sy] + sx, &tex, width - sx, height - sy, pv->width2 - pv->startX, pv->height2 - pv->startY);
	}
	if ((*menu) & 0x01)
	{
		pgCopyMenuBar();
	}
	if (((*menu) & 0x02) && info){
		char *str,msg[256],buf[32];
		strcpy( msg, info );
		sprintf( buf, "\n倍率 %f", 1/pv->thumb );
		strcat( msg, buf );
		str = msg;
		pgFillvram(THREAD_INDEX + 14, 0, 0, 6*25, 12*7.5, 2);
		pgSetDrawStart(10, 6, 0, 0);
		while ((str = pgPrint(str, WHITE, WHITE, SCR_WIDTH))){
			pgSetDrawStart(10, -1, 0, LINE_PITCH);
		}
		pgCopy(0,0);										// プロパティ
	}
	flipScreen(0);

	free(tmpbuf);
	free(imgBuf);
	return ret;
}

/*****************************
GIFファイルを読み込んで32ビットRGBAに変換
*****************************/
int psp2chImageViewGif(const char* data, unsigned long length, int *menu, int *thumbFlag, int shift, char *info)
{
	int InterlacedOffset[] = { 0, 4, 2, 1 }; /* The way Interlaced image should. */
	int InterlacedJumps[] = { 8, 8, 4, 2 };	/* be read - offsets and jumps... */
	int i, j, Size, Row, Col, Width, Height, Count, ExtCode, width2, ret = COMPLETE;
	int reset, err = 0, cormap = 0;
	int frameCnt, imgCount, colorCount, SHeight, SWidth;
	int transparent = -1, delaytime, ImageSize;
	unsigned int	nextTime, oldClock;
	my_img_buffer	gif_buf;
	GifFileType		*GifFile;
	GifRowType		*ScreenBuffer = NULL, ImgBuf = NULL, GifRow;
	GifRecordType	RecordType;
	GifByteType		*Extension;
	GifColorType	*ColorMapEntry;
	ColorMapObject	*ColorMap;
	unsigned char	**img = NULL, *buf = NULL, *BufferP;
	strPicView		pv;
	struct strGif{												// GIFの各フレーム情報を記録するイメージバッファ
		int				transparent;							// 透過色
		int				delaytime;								// フレーム間遅延時間
		GifRowType		ImgBuf;									// パレットイメージ
		GifColorType	*Colors;								// パレットテーブル
	} *gif = NULL;

	gif_buf.data = (unsigned char*)data;
	gif_buf.length = length;
	gif_buf.offset = 0;
	if ((GifFile = DGifOpen(&gif_buf, gif_data_read_func)) == NULL)
	{
		return LIB_ERR;
	}
	colorCount = 0;
	do {														// 簡易型GIF構造解析（DGifSlurp()ではメモリを消費しすぎるので）
		if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR){
			ret = LIB_ERR;
			err = -1;
			goto gif_Error;
		}
		switch (RecordType){
		case IMAGE_DESC_RECORD_TYPE:
			if (DGifGetImageDesc(GifFile) == GIF_ERROR){
				ret = LIB_ERR;
				err = -1;
				goto gif_Error;
			}
			ImageSize = GifFile->Image.Width;
			ScreenBuffer = (GifRowType *)malloc((size_t)ImageSize * sizeof(GifPixelType));
			if (ScreenBuffer == NULL) {
				ret = MEM_ERR;
				err = -1;
				goto gif_Error;
			}
			for (i=0; i<GifFile->Image.Height ;i++){			// これをやらないと次の画像に行けない…遅くなるからやりたくないのに
				if (DGifGetLine(GifFile, (char*)ScreenBuffer, ImageSize) == GIF_ERROR){
					free(ScreenBuffer);
					ret = LIB_ERR;
					err = -1;
					goto gif_Error;
				}
			}
			free(ScreenBuffer);
			ScreenBuffer = NULL;
			if (GifFile->Image.ColorMap) cormap = 1;
			ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap : GifFile->SColorMap);
			if (colorCount<ColorMap->ColorCount){
				colorCount = ColorMap->ColorCount;				// 使用パレット数
			}
			FreeMapObject(GifFile->Image.ColorMap);				//
			GifFile->Image.ColorMap = NULL;						// ローカルカラーマップによるメモリリーク対策
			break;
		case EXTENSION_RECORD_TYPE:
			if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR){
				ret = LIB_ERR;
				err = -1;
				goto gif_Error;
			}
			while (Extension != NULL) {
				if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR){
					ret = LIB_ERR;
					err = -1;
					goto gif_Error;
				}
			}
			break;
		case TERMINATE_RECORD_TYPE:
			break;
		default:
			break;
		}
	} while (RecordType != TERMINATE_RECORD_TYPE);
	imgCount = GifFile->ImageCount;								// 画像数（これを取得するためにどれほど苦労したことか(泣 ）
	if (info){
		sprintf(info, "ファイルサイズ %dKB\n画像サイズ X=%d Y=%d\nカラー数 %d\nColorMap %d\n画像数 %d",
		              length/1024, GifFile->Image.Width, GifFile->Image.Height, colorCount, cormap, imgCount);
	}
	DGifCloseFile(GifFile);
	gif_buf.data = (unsigned char*)data;
	gif_buf.length = length;
	gif_buf.offset = 0;
	if ((GifFile = DGifOpen(&gif_buf, gif_data_read_func)) == NULL)
	{
		ret = LIB_ERR;
		err = -1;
		goto gif_Error;
	}
	SWidth = GifFile->SWidth;
	SHeight = GifFile->SHeight;
	Size = GifFile->SWidth * sizeof(GifPixelType);/* Size in bytes one row.*/
	// Gu転送のため1行を16バイト境界にそろえる
	width2 = (GifFile->SWidth + 15) & 0xFFFFFFF0;
	ScreenBuffer = memalign(16, GifFile->SHeight * sizeof(GifRowType *));	// パレットイメージの各ライン先頭位置
	if (!ScreenBuffer)
	{
		ret = MEM_ERR;
		err = -1;
		goto gif_Error;
	}
	ImgBuf = memalign(16, Size * GifFile->SHeight);				// パレットイメージの実体
	if (!ImgBuf)
	{
		ret = MEM_ERR;
		err = -1;
		goto gif_Error;
	}
	img = memalign(16, sizeof(unsigned char *) * GifFile->SHeight);	// 画面イメージの各ライン先頭位置
	if (!img)
	{
		ret = MEM_ERR;
		err = -1;
		goto gif_Error;
	}
	buf = memalign(16, 4 * width2 * GifFile->SHeight);			// 画面イメージの実体
	if (!buf)
	{
		ret = MEM_ERR;
		err = -1;
		goto gif_Error;
	}
	for (i = 0; i < GifFile->SHeight; i++)
	{
		ScreenBuffer[i] = &ImgBuf[i * Size];					// パレットイメージのライン位置と実体との関係を接続
		img[i] = &buf[i * 4 * width2];							// 画面イメージのライン位置と実体との関係を接続
	}
	for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
	{
		ScreenBuffer[0][i] = GifFile->SBackGroundColor;			// １行目を背景色で塗りつぶす
	}
	for (i = 1; i < GifFile->SHeight; i++)						// １行目を２行目以降にコピーしてパレットイメージを背景色に
	{
		memcpy(ScreenBuffer[i], ScreenBuffer[0], Size);
	}
	if (imgCount!=0){											// 複数の画像が含まれている場合
		gif = malloc(sizeof(struct strGif) * imgCount);
		err = 0;
		if (gif){
			for (i=0; i<imgCount ;i++){							// 先にバッファ領域を確保しておく
				gif[i].ImgBuf = memalign(16, Size * GifFile->SHeight);	// パレットイメージの保存用領域
				if (gif[i].ImgBuf == NULL){
					err = -1;
					break;
				}
				gif[i].Colors = malloc(colorCount * sizeof(GifColorType));
				if (gif[i].Colors == NULL){
					free(gif[i].ImgBuf);
					err = -1;
					break;
				}
			}
			if (err){											// メモリが足りない
				for (i=i-1; i>0 ;i--){
					free(gif[i].Colors);
					free(gif[i].ImgBuf);
				}
				free(gif);
				gif = NULL;
			}
		}
	}
	err = 0;

	/* Scan the content of the GIF file and load the image(s) in: */
	reset = 1;													// 画像表示位置のリセット指定
	frameCnt = 0;
	do {
		do {
			if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR)
			{
				ret = LIB_ERR;
				err = -1;
				goto gif_Error;
			}
			switch (RecordType)
			{
			case IMAGE_DESC_RECORD_TYPE:
				if (frameCnt>imgCount){							// 予定より画像が多い！
					ret = LIB_ERR;
					err = -1;
					goto gif_Error;
				}
				if (DGifGetImageDesc(GifFile) == GIF_ERROR)
				{
					ret = LIB_ERR;
					err = -1;
					goto gif_Error;
				}
				Row = GifFile->Image.Top; /* Image Position relative to Screen. */
				Col = GifFile->Image.Left;
				Width = GifFile->Image.Width;
				Height = GifFile->Image.Height;
				if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
				   GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight)
				{
					ret = SUP_ERR;
					err = -1;
					goto gif_Error;
				}
				if (GifFile->Image.Interlace) {
					/* Need to perform 4 passes on the images: */
					for (Count = i = 0; i < 4; i++)
					{
						for (j = Row + InterlacedOffset[i]; j < Row + Height; j += InterlacedJumps[i])
						{
							if (DGifGetLine(GifFile, &ScreenBuffer[j][Col], Width) == GIF_ERROR)
							{
								ret = LIB_ERR;
								err = -1;
								goto gif_Error;
							}
						}
					}
				}
				else {
					for (i = 0; i < Height; i++)
					{
						if (DGifGetLine(GifFile, &ScreenBuffer[Row++][Col], Width) == GIF_ERROR)
						{
							ret = LIB_ERR;
							err = -1;
							goto gif_Error;
						}
					}
				}
				ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap : GifFile->SColorMap);
				if (ColorMap == NULL)
				{
					ret = SUP_ERR;
					err = -1;
					goto gif_Error;
				}
				if (gif){										// イメージバッファが確保されているなら
					memcpy(gif[frameCnt].ImgBuf, ImgBuf, Size * GifFile->SHeight);									// パレットイメージを保存
					memcpy(gif[frameCnt].Colors, ColorMap->Colors, ColorMap->ColorCount * sizeof(GifColorType));	// パレットテーブルを保存
					frameCnt++;
				} else {										// イメージバッファが確保されていないので画像の表示を行う
					for (i = 0; i < SHeight; i++) {				// パレットイメージをベタ画像へ
						GifRow = ScreenBuffer[i];
						BufferP = img[i];
						for (j = 0; j < SWidth; j++) {
							if (GifRow[j] != transparent)
							{
								ColorMapEntry = &ColorMap->Colors[GifRow[j]];
								*BufferP++ = ColorMapEntry->Red;
								*BufferP++ = ColorMapEntry->Green;
								*BufferP++ = ColorMapEntry->Blue;
								*BufferP++ = 0xFF;
							}
							else
								BufferP += 4;
						}
					}
					do{											// 指定されたフレーム時間になるまで待機
						oldClock = clock();
						ret = psp2chImageViewer2((int**)img, SWidth, SHeight, menu, thumbFlag, &reset, shift, &pv, info);
						if (ret!=10){							// キー入力により中断
							err = -1;
							goto gif_Error;
						}
						if (oldClock>clock()) break;			// クロックカウントがオーバーフローした場合の対策
					}while (clock()<nextTime && s2ch.running);
				}
				FreeMapObject(GifFile->Image.ColorMap);			//
				GifFile->Image.ColorMap = NULL;					// ローカルカラーマップによるメモリリーク対策
				break;
			case EXTENSION_RECORD_TYPE:
				/* Skip any extension blocks in file: */
				if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR)
				{
					ret = LIB_ERR;
					err = -1;
					goto gif_Error;
				}
				if (ExtCode == GRAPHICS_EXT_FUNC_CODE)
				{
					int flag = Extension[1];
					transparent = (flag & 0x01) ? Extension[4] : -1;	// 透過色
					delaytime = Extension[2] + Extension[3] * 256;		// 遅延時間
					nextTime = clock() + delaytime * 10*1000;			// 次のフレーム作画時間
					if (frameCnt>imgCount){								// 予定より画像が多い！
						ret = LIB_ERR;
						err = -1;
						goto gif_Error;
					}
					if (gif){
						gif[frameCnt].transparent = transparent;
						gif[frameCnt].delaytime = delaytime;
					}
				}
				while (Extension != NULL)
				{
					if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR)
					{
						ret = LIB_ERR;
						err = -1;
						goto gif_Error;
					}
				}
				break;
			case TERMINATE_RECORD_TYPE:
				break;
			default:			/* Should be traps by DGifGetRecordType. */
				break;
			}
		} while (RecordType != TERMINATE_RECORD_TYPE);
		if (gif==NULL && imgCount>1){
			DGifCloseFile(GifFile);
			gif_buf.data = (unsigned char*)data;
			gif_buf.length = length;
			gif_buf.offset = 0;
			if ((GifFile = DGifOpen(&gif_buf, gif_data_read_func)) == NULL){
				ret = LIB_ERR;
				err = -1;
			}
		}
	} while (s2ch.running && !err && gif==NULL && imgCount>1);

gif_Error:
	DGifCloseFile(GifFile);

	if (err==0){												// エラーが起きていないなら
		frameCnt = 0;
		if (imgCount==1){										// 画像が１枚のとき
			for (i = 0; i < SHeight; i++){						// パレットイメージをベタ画像へ
				GifRow = &gif[frameCnt].ImgBuf[i*Size];
				BufferP = img[i];
				for (j = 0; j < SWidth; j++) {
					if (GifRow[j] != gif[frameCnt].transparent)
					{
						ColorMapEntry = &gif[frameCnt].Colors[GifRow[j]];
						*BufferP++ = ColorMapEntry->Red;
						*BufferP++ = ColorMapEntry->Green;
						*BufferP++ = ColorMapEntry->Blue;
						*BufferP++ = 0xFF;
					}
					else
						BufferP += 4;
				}
			}
			ret = psp2chImageViewer((int**)img, SWidth, SHeight, menu, thumbFlag, shift, info);

		} else if (gif){										// 画像が複数でイメージバッファが確保されてるなら
			do{													// 画像表示ループ
				for (i = 0; i < SHeight; i++){					// パレットイメージをベタ画像へ
					GifRow = &gif[frameCnt].ImgBuf[i*Size];
					BufferP = img[i];
					for (j = 0; j < SWidth; j++) {
						if (GifRow[j] != gif[frameCnt].transparent)
						{
							ColorMapEntry = &gif[frameCnt].Colors[GifRow[j]];
							*BufferP++ = ColorMapEntry->Red;
							*BufferP++ = ColorMapEntry->Green;
							*BufferP++ = ColorMapEntry->Blue;
							*BufferP++ = 0xFF;
						}
						else
							BufferP += 4;
					}
				}
				nextTime = clock() + gif[frameCnt].delaytime * 10*1000;	// 次のフレームを表示する時刻
				do{												// フレーム作画時間までループ
					oldClock = clock();
					ret = psp2chImageViewer2((int**)img, SWidth, SHeight, menu, thumbFlag, &reset, shift, &pv, info);
					if (ret!=10) break;							// キー入力により中断
					if (oldClock>clock()) break;				// クロックカウントがオーバーフローした場合の対策
				}while (clock()<nextTime && s2ch.running);
				frameCnt++;
				if (frameCnt>=imgCount) frameCnt = 0;
			}while (s2ch.running && ret==10);
		}
	}

	if (gif){													// イメージバッファを開放
		for (i=imgCount-1; i>=0 ;i--){
			free(gif[i].ImgBuf);
			free(gif[i].Colors);
		}
		free(gif);
	}
	if(buf)
		free(buf);
	if(img)
		free(img);
	if(ImgBuf)
		free(ImgBuf);
	if(ScreenBuffer)
		free(ScreenBuffer);
	preLine = -2;
	return ret;
}

int saveImage(const char* path, const char* image, unsigned long length)
{
	SceUID fd = sceIoOpen(path, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, FILE_PARMISSION);
	if (fd < 0)
	{
		psp2chNormalError(FILE_OPEN_ERR, path);
		return -1;
	}
	psp2chFileWrite(fd, image, length);
	sceIoClose(fd);
	return 0;
}

int psp2chMenuImageView(char* file, int size, int *menu, int *thumbFlag, int type)
{
	char msg[512],info[512];
	int ret = 0;
	
	pgCreateTexture();
	pgFillvram(0, 0, 0, SCR_WIDTH, SCR_HEIGHT, 2);
	pgFillvram(THREAD_INDEX + 14, 0, 0, SCR_WIDTH, 14, 2);
	pgSetDrawStart(10, 0, 0, 0);
	strcpy(msg, "展開中...(");
	if (s2ch.cfg.hbl){
		psp2chUTF82Sjis(info, file);
	} else {
		strcpy(info, file);
	}
	strcat(msg, info);
	strcat(msg, ")");
	pgPrint(msg, WHITE, WHITE, SCR_WIDTH);
	pgCopy(0,0);												// ロード中メッセージ
	flipScreen(0);
	pgFillvram(0, 0, 0, SCR_WIDTH, 14, 2);

	SceUID fd = sceIoOpen(file, PSP_O_RDONLY, FILE_PARMISSION);
	if (fd < 0)
	{
		pgDeleteTexture();
		psp2chNormalError(FILE_OPEN_ERR, file);
		return 0;
	}
	char *image = (char*)malloc(sizeof(char) * (size + 1));
	psp2chFileRead(fd, image, size);
	sceIoClose(fd);
	info[0] = '\0';
	switch(type)
	{
		case 0:
			ret = psp2chImageViewJpeg(image, size, menu, thumbFlag, 1, info);
			break;
		case 1:
			ret = psp2chImageViewPng(image, size, menu, thumbFlag, 1, info);
			break;
		case 2:
			ret = psp2chImageViewGif(image, size, menu, thumbFlag, 1, info);
			break;
		case 3:
			ret = psp2chImageViewBmp(image, size, menu, thumbFlag, 1, info);
			break;
	}
	pgDeleteTexture();
	if (ret < 0){
		strcpy(msg,"decompress error\n");
		switch (ret){
		case LIB_ERR:
			strcat(msg, "  画像フォーマットが不正です");
			break;
		case MEM_ERR:
			strcat(msg, "  メモリを確保できませんでした");
			break;
		case SUP_ERR:
			strcat(msg, "  サポートしていない画像形式です");
			break;
		case FRM_ERR:
			strcat(msg, "  画像数が多すぎます");
			break;
		}
		strcat(msg,"\n\n");
		strcat(msg,info);
		psp2chErrorDialog(0, msg);
	}
	free(image);
	return 0;
}
