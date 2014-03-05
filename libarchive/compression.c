#include "compression.h"
#include "archive.h"
#include "LzmaLib/LzmaLib.h"
#include "zlib/zlib.h"

int compress_data( ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size, int compression )
{
	ar_byte *work = out_buff;
	int result;

	if(in_size == 0)
	{
		*out_size = 0;
		return 1;
	}

	*work++ = (ar_byte)compression;
	*out_size -= 1; //让接下来的压缩算法知道输出的空间已经被占用一字节了

	switch(compression)
	{
	case JARCHIVE_COMPRESS_LZMA:
		result = compress_lzma(work, out_size, in_buff, in_size);
		break;
	case JARCHIVE_COMPRESS_ZLIB:
		result = compress_zlib(work, out_size, in_buff, in_size);
		break;
	}

	*out_size += 1; //out_size是压缩算法输出的实际大小，要再加上刚刚compression占用的一字节

	return result;
}

int uncompress_data( ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size )
{
	int compress;
	int result;

	if(in_size == 0)
	{
		*out_size = 0;
		return 1;
	}

	compress = (int)*in_buff++;
	in_size -= 1;

	switch(compress)
	{
	case JARCHIVE_COMPRESS_LZMA:
		result = uncompress_lzma(out_buff, out_size, in_buff, in_size);
		break;
	case JARCHIVE_COMPRESS_ZLIB:
		result = uncompress_zlib(out_buff, out_size, in_buff, in_size);
		break;
	default:
		result = 0;
		break;
	}

	return result;
}

int compress_lzma( ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size )
{
	int result;
	size_t propSize = 5;

	*out_size -= LZMA_PROPS_SIZE;
	result = LzmaCompress(out_buff + LZMA_PROPS_SIZE, (size_t*)out_size, in_buff, in_size, out_buff, &propSize, 5, (1 << 24), 3, 0, 2, 32, 2);

	if(result == SZ_OK)
	{
		assert(propSize == LZMA_PROPS_SIZE);
		*out_size += propSize;
	}

	return result == SZ_OK;
}

int uncompress_lzma( ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size )
{
	int result;
	size_t data_size = in_size - LZMA_PROPS_SIZE;

	result = LzmaUncompress(out_buff, (size_t*)out_size, in_buff + LZMA_PROPS_SIZE, &data_size, in_buff, LZMA_PROPS_SIZE);

	return result == SZ_OK;
}

int compress_zlib( ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size )
{
	int result = compress(out_buff, out_size, in_buff, in_size);
	return result == Z_OK;
}

int uncompress_zlib( ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size )
{
	int result = uncompress(out_buff, out_size, in_buff, in_size);
	return result == Z_OK;
}
