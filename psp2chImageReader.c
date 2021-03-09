#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <png.h>
#include <gif_lib.h>
#include "psp2chImageReader.h"

// prototype
METHODDEF(void) memory_init_source ();
METHODDEF(boolean) memory_fill_input_buffer (j_decompress_ptr cinfo);
METHODDEF(void) memory_skip_input_data (j_decompress_ptr cinfo, long num_bytes);
METHODDEF(void) memory_term_source ();

// JPEG メモリリード関数
METHODDEF(void) memory_init_source ()
{
	return;
}

METHODDEF(boolean) memory_fill_input_buffer (j_decompress_ptr cinfo)
{
	memory_src_ptr src = (memory_src_ptr) cinfo->src;

	src->buffer[0] = (JOCTET) 0xFF;
	src->buffer[1] = (JOCTET) JPEG_EOI;
	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = 2;
	return TRUE;
}

METHODDEF(void) memory_skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	memory_src_ptr src = (memory_src_ptr) cinfo->src;

	if (num_bytes > 0)
	{
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

METHODDEF(void) memory_term_source ()
{
	return;
}

GLOBAL(void) jpeg_memory_src (j_decompress_ptr cinfo, void* data, unsigned long len)
{
	memory_src_ptr src;

	if (cinfo->src == NULL)
	{	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
		(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
		  sizeof(memory_source_mgr));
		src = (memory_src_ptr) cinfo->src;
		src->buffer = (JOCTET *)
		(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
		  len * sizeof(JOCTET));
	}

	src = (memory_src_ptr) cinfo->src;
	src->pub.init_source = memory_init_source;
	src->pub.fill_input_buffer = memory_fill_input_buffer;
	src->pub.skip_input_data = memory_skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = memory_term_source;
	src->pub.bytes_in_buffer = len;
	src->pub.next_input_byte = (JOCTET*)data;
}

// PNG メモリリード関数
void png_memread_func(png_structp png_ptr, png_bytep buf, png_size_t size)
{
	my_img_buffer *png_buf = (my_img_buffer *)png_get_io_ptr(png_ptr);
	if (png_buf->offset + size > png_buf->length)
	{
		png_error(png_ptr,"read error");
	}
	memcpy(buf, png_buf->data, size);
	png_buf->offset += size;
	png_buf->data += size;
}

// GIF メモリリード関数
int gif_data_read_func(GifFileType* GifFile, GifByteType* buf, int size)
{
	my_img_buffer *gif_buf = (my_img_buffer *) GifFile->UserData;
	if (gif_buf->offset + size > gif_buf->length)
	{
		return 0;
	}
	memcpy(buf, gif_buf->data, size);
	gif_buf->offset += size;
	gif_buf->data += size;
	return size;
}
