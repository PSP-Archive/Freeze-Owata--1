#ifndef __PSP2CH_IMAGE_READER_H__
#define __PSP2CH_IMAGE_READER_H__

/* for jpeg read from memory */
typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */
	JOCTET * buffer;
	unsigned long buffer_length;
} memory_source_mgr;
typedef memory_source_mgr *memory_src_ptr;

/* for jpeg library error */
struct my_error_mgr
{
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct my_error_mgr * my_error_ptr;

/* for png and gif read from memory */
typedef struct my_img_buffer_ {
	unsigned char *data;
	unsigned long length;
	unsigned long offset;
} my_img_buffer;

GLOBAL(void) jpeg_memory_src (j_decompress_ptr cinfo, void* data, unsigned long len);
void png_memread_func(png_structp png_ptr, png_bytep buf, png_size_t size);
int gif_data_read_func(GifFileType* GifFile, GifByteType* buf, int size);

#endif