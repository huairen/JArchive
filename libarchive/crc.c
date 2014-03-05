#include "crc.h"

static ar_uint g_nCrc32Table[256];
static char g_bCrcTableValid = 0;

void crc_init()
{
	unsigned int val;
	int i,j;

	if(g_bCrcTableValid == 1)
		return;

	for(i = 0; i < 256; i++)
	{
		val = i;
		for(j = 0; j < 8; j++)
		{
			if(val & 0x01)
				val = 0xEDB88320 ^ (val >> 1);
			else
				val = val >> 1;
		}
		g_nCrc32Table[i] = val;
	}

	g_bCrcTableValid = 1;
}

ar_uint crc_append( ar_uint crc_value, const void *data, int size )
{
	char * buf;
	int i;

	if(g_bCrcTableValid == 0)
		crc_init();

	buf = (char*)data;
	for(i = 0; i < size; i++)
		crc_value = g_nCrc32Table[(crc_value ^ buf[i]) & 0xff] ^ (crc_value >> 8);

	return crc_value;
}

ar_uint crc_calc( const void *data, int size )
{
	return CRC_GET_DIGEST(crc_append(CRC_INIT_VAL, data, size));
}
