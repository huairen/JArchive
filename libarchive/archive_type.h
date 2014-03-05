#ifndef _ARCHIVE_TYPE_H_
#define _ARCHIVE_TYPE_H_

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <io.h>
#include <assert.h>
#include <sys/stat.h>
#include <share.h>

#include <WinSock2.h>
#include <Windows.h>

#pragma comment(lib,"ws2_32.lib")

#include "archive.h"

#define ID_ARCHIVE						0x1A56504D
#define ID_ARCHIVE_INFO					0x1A56504E
#define ID_ARCHIVE_FILE                 0x46404c45
#define ARCHIVE_VERSION					1

#define TABLE_INDEX_NULL    0xFFFFFFFF
#define HASH_KEY_NULL       0
#define HASH_KEY_DELETED    0x80000000

#define JFILE_FLAG_ENCRYPTED        0x0001
#define JFILE_FLAG_COMRESSED        0x0002
#define JFILE_FLAG_DELETE_MARKER    0x0004
#define JFILE_FLAG_CRC              0x0008
#define JFILE_FLAG_INVAILD_DATA     0x0010
#define JFILE_FLAG_EXIST            0x8000

#define JFILE_RAW_VERIFY_SIZE    4

typedef unsigned char ar_byte;
typedef unsigned short ar_ushort;
typedef unsigned int ar_uint;
typedef unsigned __int64 ar_uint64;
typedef void* ar_handle;

struct archive_header
{
	ar_uint id;
	ar_uint version;
	ar_uint64 table_offset;
	ar_uint hash_table_size;
	ar_uint file_table_size;
	ar_uint string_table_size;
};

struct hash_entry
{
	ar_uint file_index;
	ar_uint64 key;
};

struct file_entry
{
	ar_uint64	 offset;
	ar_uint64	 file_time;
	ar_uint      hash_index;
	ar_uint		 file_size;
	ar_uint		 comp_size;
	ar_uint		 flags;
	ar_uint		 crc32;
	const char*  filename;
};

struct archive
{
	ar_uint hash_table_size;
	ar_uint file_table_size;

	struct hash_entry *hash_table;
	struct file_entry *file_table;

	short data_update;
	short progress_stop;

	PROGRESS_CALLBACK progress_func;
	void *user_data;

	void *thread_lock;

	FILE *stream;
};

struct archive_file
{
	struct archive *arc;
	struct file_entry *entry;
	ar_uint file_pos;
	ar_uint compression;

	//ÎÄ¼þÉÈÇø
	ar_uint *sector_offsets;
	ar_byte *file_sector;
	ar_uint sector_count;
	ar_uint sector_size;
	ar_uint sector_index;
};

#endif