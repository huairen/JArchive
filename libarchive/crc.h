#ifndef _CRC_H_
#define _CRC_H_
#include "archive_type.h"

#define CRC_INIT_VAL 0xFFFFFFFF
#define CRC_GET_DIGEST(crc) ((crc) ^ CRC_INIT_VAL)

ar_uint crc_append(ar_uint crc_value, const void *data, int size);
ar_uint crc_calc(const void *data, int size);

#endif