#ifndef _COMPRESSION_H_
#define _COMPRESSION_H_

#include "archive_type.h"

int compress_data(ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size, int compression);
int uncompress_data(ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size);

int compress_lzma(ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size);
int uncompress_lzma(ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size);

int compress_zlib(ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size);
int uncompress_zlib(ar_byte *out_buff, ar_uint *out_size, ar_byte *in_buff, ar_uint in_size);

#endif