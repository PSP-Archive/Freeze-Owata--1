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
#define LIMIT_WIDTH (1024)		// �摜�̍ő�\��WIDTH
#define PNG_BYTES_TO_CHECK (8)	// PNG�̐擪�w�b�_�T�C�Y
#define GIF_FRAME_MAX (100)		// �A�j���[�V����GIF�ň�����ő�摜��

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

/* jpeg�G���[���[�`�� */
METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
	char buffer[JMSG_LENGTH_MAX];

	my_error_ptr err = (my_error_ptr)cinfo->err;
	(*cinfo->err->format_message) (cinfo, buffer);
	//psp2chErrorDialog(0, "%s", buffer);
	longjmp(err->setjmp_buffer, 1);
}

/* PNG�G���[���[�`�� */
static void myPngError(png_structp png_ptr,png_const_charp message)
{
	//psp2chErrorDialog(0, "%s", message);
	longjmp(png_ptr->jmpbuf, 1);
}

/*****************************
	��`�͈͂��g��k��
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
	/* texture��SLICE_SIZE * BUF_HEIGHT�ɐ؂����ă��[�v */
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
���j���[����
RGBA�f�[�^��VRAM�ɓ]��
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

	// Gu�]���̂���1�s��16�o�C�g���E�ɂ��낦��
	tex.tb = (width + 15) & 0xFFFFFFF0;
	tex.w = BUF_WIDTH;
	tex.h = BUF_HEIGHT;

	// ���̑傫���摜�͏k������
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
	thumbW = (double)width / SCR_WIDTH;		// ��ʕ��ɍ��킹�邽�߂̊g��k����
	imgWH = height / thumbW;				// ��ʕ��ɍ��킹���Ƃ��̉摜����
	thumbH = (double)height / SCR_HEIGHT;	// ��ʍ����ɂ��킹�邽�߂̊g��k����
	imgHW = width / thumbH;					// ��ʍ����ɂ��킹���Ƃ��̉摜��

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

	// �摜�\�����C�����[�v
	while (s2ch.running)
	{
		if(sceCtrlPeekBufferPositive(&s2ch.pad, 1))
		{
			if (shift){
				pgPrintMenuBar(" �� : �g��k��  �~ : �߂�  �� : ���j���[�I���E�I�t  �� : �߂�  �k�q : �摜����");
			} else {
				pgPrintMenuBar(" �� : �g��k��  �~ : �摜��ۑ����Ė߂�  �� : ���j���[�I���E�I�t  �� : �摜���폜���Ė߂�");
			}
			if (s2ch.pad.Buttons != s2ch.oldPad.Buttons)
			{
				s2ch.oldPad = s2ch.pad;
				if(s2ch.pad.Buttons & PSP_CTRL_CIRCLE)			// �g�嗦
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
				else if(s2ch.pad.Buttons & PSP_CTRL_CROSS)		// �\���I��
				{
					ret = SAVE_ERR;
					break;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_TRIANGLE)	// ���j���[ON/OFF
				{
					(*menu) ^= 0x01;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_SELECT)		// �v���p�e�B
				{
					(*menu) ^= 0x02;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_SQUARE)		// �폜
				{
					break;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_RTRIGGER && shift){	// �摜����p
					break;
				}
				else if(s2ch.pad.Buttons & PSP_CTRL_LTRIGGER && shift){	// �摜����p
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
		// width : ���摜��
		// height : ���摜����
		// sx.sy : ���摜�ɂ�����\���J�n�ʒu
		// width2 : �\���摜��
		// height2 : �\���摜����
		// startX.startY : �\���摜�ɂ�����\���J�n�ʒu
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
			sprintf( buf, "\n�{�� %f", 1/thumb );
			strcat( msg, buf );
			str = msg;
			pgFillvram(THREAD_INDEX + 14, 0, 0, 6*25, 12*7.5, 2);
			pgSetDrawStart(10, 6, 0, 0);
			while ((str = pgPrint(str, WHITE, WHITE, SCR_WIDTH))){
				pgSetDrawStart(10, -1, 0, LINE_PITCH);
			}
			pgCopy(0,0);										// �v���p�e�B
		}
		flipScreen(0);
	}

	free(tmpbuf);
	free(imgBuf);
	return ret;
}

/*****************************
jpeg�t�@�C����ǂݍ����32�r�b�gRGBA�ɕϊ�
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
	if(setjmp(jerr.setjmp_buffer)) /* error���̃W�����v�� */
	{
		ret = LIB_ERR;
		goto jpeg_Error;
	}
	// �L���J�n
	jpeg_create_decompress(&cinfo);
	jpeg_memory_src(&cinfo, (void*)data, length);
	jpeg_read_header(&cinfo, TRUE);
	if (info){
		sprintf(info, "�t�@�C���T�C�Y %dKB\n�摜�T�C�Y X=%d Y=%d\n�J���[��� %d", length/1024, cinfo.image_width, cinfo.image_height, cinfo.out_color_components);
	}
	// �傫������ꍇ�ɂ�����ŏk��(�������̂���)
	while ((cinfo.image_width / cinfo.scale_denom) > LIMIT_WIDTH)
	{
		cinfo.scale_denom += cinfo.scale_denom;
	}
	jpeg_calc_output_dimensions(&cinfo);
	jpeg_start_decompress(&cinfo);
	// Gu�]���̂���1�s��16�o�C�g���E�ɂ��낦��
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
	// RGB�J���[��RGBA��
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
	// �O���[�X�P�[����RGBA��
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
	// �\��
	ret = psp2chImageViewer((int**)img, cinfo.output_width, height, menu, thumbFlag, shift, info);
jpeg_Error:
	// �I������
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
PNG�t�@�C����ǂݍ����32�r�b�gRGBA�ɕϊ�
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
	// PNG�`�F�b�N
	if (png_sig_cmp((png_bytep)png_buf.data, 0, PNG_BYTES_TO_CHECK) != 0)
	{
		return HEAD_ERR;
	}
	png_buf.offset = PNG_BYTES_TO_CHECK;
	png_buf.data += PNG_BYTES_TO_CHECK;
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	info_ptr = png_create_info_struct(png_ptr);
	end_info = png_create_info_struct(png_ptr);
	//�G���[�n���h��
	if (setjmp(png_ptr->jmpbuf))
	{
		ret = LIB_ERR;
		goto png_Error;
	}
	// IO�ύX
	png_set_read_fn(png_ptr, (void*)&png_buf, (png_rw_ptr)png_memread_func);
	png_set_error_fn(png_ptr, NULL, myPngError, NULL);
	png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);
	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL, NULL);
	if (info){
		sprintf(info, "�t�@�C���T�C�Y %dKB\n�摜�T�C�Y X=%d Y=%d\n�J���[��� %X\n�r�b�g�� %d\n�C���^�[���[�X %d", length/1024, width, height, color_type, bit_depth, interlace_type);
	}
	//�p���b�g�n->RGB�n�Ɋg��
	if (color_type == PNG_COLOR_TYPE_PALETTE ||
		(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) ||
		png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
	{
		png_set_expand(png_ptr);
	}
	//16�r�b�g->8�r�b�g�ɗ��Ƃ�
	if (bit_depth > 8)
	{
		png_set_strip_16(png_ptr);
	}
	//�O���[�X�P�[��->RGB�Ɋg��
	if (color_type==PNG_COLOR_TYPE_GRAY || color_type==PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_gray_to_rgb(png_ptr);
	}
	if (color_type != PNG_COLOR_TYPE_RGB_ALPHA && color_type!=PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		png_set_add_alpha(png_ptr, 0xFFFF, PNG_FILLER_AFTER);
	}
	png_read_update_info(png_ptr, info_ptr);
	// Gu�]���̂���1�s��16�o�C�g���E�ɂ��낦��
	width2 = (width + 15) & 0xFFFFFFF0;
	img = memalign(16, height * sizeof(png_bytep));
	if (!img)
	{
		ret = MEM_ERR;
		goto png_Error;
	}
	// change �擾�T�C�Y��ύX �b�菈�u
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
BMP�t�@�C����ǂݍ����32�r�b�gRGBA�ɕϊ�
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
	// BITMAP �F������ "BM"
	if (memcmp(bf.bfType, "BM", 2) != 0)
	{
		return HEAD_ERR;
	}
	memcpy(&bi, bmp_data, sizeof(BITMAPINFOHEADER));
	bmp_data += sizeof(BITMAPINFOHEADER);
	if (info){
		sprintf(info, "�t�@�C���T�C�Y %dKB\n�摜�T�C�Y X=%d Y=%d\n�J���[��� %d\n���k %d", length/1024, bi.biWidth, bi.biHeight, bi.biBitCount, bi.biCompression);
	}
	// �񈳏k�̂�
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
	// Gu�]���̂���1�s��16�o�C�g���E�ɂ��낦��
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
	// 24�r�b�gBMP��RGBA��
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
	// 32�r�b�gBMP��RGBA��
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
	// ���Ή�
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
RGBA�f�[�^��VRAM�ɓ]��
GIF��p
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

	// Gu�]���̂���1�s��16�o�C�g���E�ɂ��낦��
	tex.tb = (width + 15) & 0xFFFFFFF0;
	tex.w = BUF_WIDTH;
	tex.h = BUF_HEIGHT;
	// ���̑傫���摜�͏k������
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
	thumbW = (double)width / SCR_WIDTH;		// ��ʕ��ɍ��킹�邽�߂̊g��k����
	imgWH = height / thumbW;				// ��ʕ��ɍ��킹���Ƃ��̉摜����
	thumbH = (double)height / SCR_HEIGHT;	// ��ʍ����ɂ��킹�邽�߂̊g��k����
	imgHW = width / thumbH;					// ��ʍ����ɂ��킹���Ƃ��̉摜��
	// �����ϐ��̃��Z�b�g
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
			pgPrintMenuBar(" �� : �g��k��  �~ : �߂�  �� : ���j���[�I���E�I�t  �� : �߂�  �k�q : �摜����");
		} else {
			pgPrintMenuBar(" �� : �g��k��  �~ : �摜��ۑ����Ė߂�  �� : ���j���[�I���E�I�t  �� : �摜���폜���Ė߂�");
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
			else if(s2ch.pad.Buttons & PSP_CTRL_RTRIGGER && shift){	// �摜����p
				ret = COMPLETE;
			}
			else if(s2ch.pad.Buttons & PSP_CTRL_LTRIGGER && shift){	// �摜����p
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
	// width : ���摜��
	// height : ���摜����
	// sx.sy : ���摜�ɂ�����\���J�n�ʒu
	// width2 : �\���摜��
	// height2 : �\���摜����
	// startX.startY : �\���摜�ɂ�����\���J�n�ʒu
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
		sprintf( buf, "\n�{�� %f", 1/pv->thumb );
		strcat( msg, buf );
		str = msg;
		pgFillvram(THREAD_INDEX + 14, 0, 0, 6*25, 12*7.5, 2);
		pgSetDrawStart(10, 6, 0, 0);
		while ((str = pgPrint(str, WHITE, WHITE, SCR_WIDTH))){
			pgSetDrawStart(10, -1, 0, LINE_PITCH);
		}
		pgCopy(0,0);										// �v���p�e�B
	}
	flipScreen(0);

	free(tmpbuf);
	free(imgBuf);
	return ret;
}

/*****************************
GIF�t�@�C����ǂݍ����32�r�b�gRGBA�ɕϊ�
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
	struct strGif{												// GIF�̊e�t���[�������L�^����C���[�W�o�b�t�@
		int				transparent;							// ���ߐF
		int				delaytime;								// �t���[���Ԓx������
		GifRowType		ImgBuf;									// �p���b�g�C���[�W
		GifColorType	*Colors;								// �p���b�g�e�[�u��
	} *gif = NULL;

	gif_buf.data = (unsigned char*)data;
	gif_buf.length = length;
	gif_buf.offset = 0;
	if ((GifFile = DGifOpen(&gif_buf, gif_data_read_func)) == NULL)
	{
		return LIB_ERR;
	}
	colorCount = 0;
	do {														// �ȈՌ^GIF�\����́iDGifSlurp()�ł̓����������������̂Łj
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
			for (i=0; i<GifFile->Image.Height ;i++){			// ��������Ȃ��Ǝ��̉摜�ɍs���Ȃ��c�x���Ȃ邩���肽���Ȃ��̂�
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
				colorCount = ColorMap->ColorCount;				// �g�p�p���b�g��
			}
			FreeMapObject(GifFile->Image.ColorMap);				//
			GifFile->Image.ColorMap = NULL;						// ���[�J���J���[�}�b�v�ɂ�郁�������[�N�΍�
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
	imgCount = GifFile->ImageCount;								// �摜���i������擾���邽�߂ɂǂ�قǋ�J�������Ƃ�(�� �j
	if (info){
		sprintf(info, "�t�@�C���T�C�Y %dKB\n�摜�T�C�Y X=%d Y=%d\n�J���[�� %d\nColorMap %d\n�摜�� %d",
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
	// Gu�]���̂���1�s��16�o�C�g���E�ɂ��낦��
	width2 = (GifFile->SWidth + 15) & 0xFFFFFFF0;
	ScreenBuffer = memalign(16, GifFile->SHeight * sizeof(GifRowType *));	// �p���b�g�C���[�W�̊e���C���擪�ʒu
	if (!ScreenBuffer)
	{
		ret = MEM_ERR;
		err = -1;
		goto gif_Error;
	}
	ImgBuf = memalign(16, Size * GifFile->SHeight);				// �p���b�g�C���[�W�̎���
	if (!ImgBuf)
	{
		ret = MEM_ERR;
		err = -1;
		goto gif_Error;
	}
	img = memalign(16, sizeof(unsigned char *) * GifFile->SHeight);	// ��ʃC���[�W�̊e���C���擪�ʒu
	if (!img)
	{
		ret = MEM_ERR;
		err = -1;
		goto gif_Error;
	}
	buf = memalign(16, 4 * width2 * GifFile->SHeight);			// ��ʃC���[�W�̎���
	if (!buf)
	{
		ret = MEM_ERR;
		err = -1;
		goto gif_Error;
	}
	for (i = 0; i < GifFile->SHeight; i++)
	{
		ScreenBuffer[i] = &ImgBuf[i * Size];					// �p���b�g�C���[�W�̃��C���ʒu�Ǝ��̂Ƃ̊֌W��ڑ�
		img[i] = &buf[i * 4 * width2];							// ��ʃC���[�W�̃��C���ʒu�Ǝ��̂Ƃ̊֌W��ڑ�
	}
	for (i = 0; i < GifFile->SWidth; i++)  /* Set its color to BackGround. */
	{
		ScreenBuffer[0][i] = GifFile->SBackGroundColor;			// �P�s�ڂ�w�i�F�œh��Ԃ�
	}
	for (i = 1; i < GifFile->SHeight; i++)						// �P�s�ڂ��Q�s�ڈȍ~�ɃR�s�[���ăp���b�g�C���[�W��w�i�F��
	{
		memcpy(ScreenBuffer[i], ScreenBuffer[0], Size);
	}
	if (imgCount!=0){											// �����̉摜���܂܂�Ă���ꍇ
		gif = malloc(sizeof(struct strGif) * imgCount);
		err = 0;
		if (gif){
			for (i=0; i<imgCount ;i++){							// ��Ƀo�b�t�@�̈���m�ۂ��Ă���
				gif[i].ImgBuf = memalign(16, Size * GifFile->SHeight);	// �p���b�g�C���[�W�̕ۑ��p�̈�
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
			if (err){											// ������������Ȃ�
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
	reset = 1;													// �摜�\���ʒu�̃��Z�b�g�w��
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
				if (frameCnt>imgCount){							// �\����摜�������I
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
				if (gif){										// �C���[�W�o�b�t�@���m�ۂ���Ă���Ȃ�
					memcpy(gif[frameCnt].ImgBuf, ImgBuf, Size * GifFile->SHeight);									// �p���b�g�C���[�W��ۑ�
					memcpy(gif[frameCnt].Colors, ColorMap->Colors, ColorMap->ColorCount * sizeof(GifColorType));	// �p���b�g�e�[�u����ۑ�
					frameCnt++;
				} else {										// �C���[�W�o�b�t�@���m�ۂ���Ă��Ȃ��̂ŉ摜�̕\�����s��
					for (i = 0; i < SHeight; i++) {				// �p���b�g�C���[�W���x�^�摜��
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
					do{											// �w�肳�ꂽ�t���[�����ԂɂȂ�܂őҋ@
						oldClock = clock();
						ret = psp2chImageViewer2((int**)img, SWidth, SHeight, menu, thumbFlag, &reset, shift, &pv, info);
						if (ret!=10){							// �L�[���͂ɂ�蒆�f
							err = -1;
							goto gif_Error;
						}
						if (oldClock>clock()) break;			// �N���b�N�J�E���g���I�[�o�[�t���[�����ꍇ�̑΍�
					}while (clock()<nextTime && s2ch.running);
				}
				FreeMapObject(GifFile->Image.ColorMap);			//
				GifFile->Image.ColorMap = NULL;					// ���[�J���J���[�}�b�v�ɂ�郁�������[�N�΍�
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
					transparent = (flag & 0x01) ? Extension[4] : -1;	// ���ߐF
					delaytime = Extension[2] + Extension[3] * 256;		// �x������
					nextTime = clock() + delaytime * 10*1000;			// ���̃t���[����掞��
					if (frameCnt>imgCount){								// �\����摜�������I
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

	if (err==0){												// �G���[���N���Ă��Ȃ��Ȃ�
		frameCnt = 0;
		if (imgCount==1){										// �摜���P���̂Ƃ�
			for (i = 0; i < SHeight; i++){						// �p���b�g�C���[�W���x�^�摜��
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

		} else if (gif){										// �摜�������ŃC���[�W�o�b�t�@���m�ۂ���Ă�Ȃ�
			do{													// �摜�\�����[�v
				for (i = 0; i < SHeight; i++){					// �p���b�g�C���[�W���x�^�摜��
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
				nextTime = clock() + gif[frameCnt].delaytime * 10*1000;	// ���̃t���[����\�����鎞��
				do{												// �t���[����掞�Ԃ܂Ń��[�v
					oldClock = clock();
					ret = psp2chImageViewer2((int**)img, SWidth, SHeight, menu, thumbFlag, &reset, shift, &pv, info);
					if (ret!=10) break;							// �L�[���͂ɂ�蒆�f
					if (oldClock>clock()) break;				// �N���b�N�J�E���g���I�[�o�[�t���[�����ꍇ�̑΍�
				}while (clock()<nextTime && s2ch.running);
				frameCnt++;
				if (frameCnt>=imgCount) frameCnt = 0;
			}while (s2ch.running && ret==10);
		}
	}

	if (gif){													// �C���[�W�o�b�t�@���J��
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
	strcpy(msg, "�W�J��...(");
	if (s2ch.cfg.hbl){
		psp2chUTF82Sjis(info, file);
	} else {
		strcpy(info, file);
	}
	strcat(msg, info);
	strcat(msg, ")");
	pgPrint(msg, WHITE, WHITE, SCR_WIDTH);
	pgCopy(0,0);												// ���[�h�����b�Z�[�W
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
			strcat(msg, "  �摜�t�H�[�}�b�g���s���ł�");
			break;
		case MEM_ERR:
			strcat(msg, "  ���������m�ۂł��܂���ł���");
			break;
		case SUP_ERR:
			strcat(msg, "  �T�|�[�g���Ă��Ȃ��摜�`���ł�");
			break;
		case FRM_ERR:
			strcat(msg, "  �摜�����������܂�");
			break;
		}
		strcat(msg,"\n\n");
		strcat(msg,info);
		psp2chErrorDialog(0, msg);
	}
	free(image);
	return 0;
}
