#include "verify.h"
#include "crc.h"
#include "file_io.h"

int archive_verify_file(struct archive_file *file, int flag)
{
	ar_uint crc32 = CRC_INIT_VAL;
	char  buffer[0x1000];
	ar_uint file_size;
	ar_uint curr_size;

	if(file->entry->flags & JFILE_FLAG_INVAILD_DATA)
		return 0;

	file->file_pos = 0;
	file_size = file->entry->file_size;

	while(file_size > 0)
	{
		curr_size = archive_file_read(file, buffer, sizeof(buffer));
		if(curr_size == 0)
			break;

		if(flag & JARCHIVE_VERIFY_FILE_CRC)
			crc32 = crc_append(crc32, buffer, curr_size);

		file_size -= curr_size;
	}

	if(file_size > 0)
		return 0;

	if(flag & JARCHIVE_VERIFY_FILE_CRC)
	{
		crc32 = CRC_GET_DIGEST(crc32);
		if(file->entry->crc32 == crc32)
			return 1;
	}

	return 0;
}

int verify_raw_data( struct archive *arc, struct file_entry *entry )
{
	ar_uint crc32 = CRC_INIT_VAL;
	FILE *stream = arc->stream;

	char  buffer[0x1000];
	ar_uint comp_size;
	ar_uint curr_size;
	ar_uint64 curr_pos;

	char verify_data[JFILE_RAW_VERIFY_SIZE];
	ar_uint verify_size = 0;
	ar_uint file_crc;
	
	comp_size = entry->comp_size;
	curr_pos = entry->offset;

	while(comp_size > 0)
	{
		curr_size = get_min(comp_size, sizeof(buffer));
		if(curr_size != read_data(arc, curr_pos, buffer, curr_size))
			break;

		comp_size -= curr_size;
		curr_pos += curr_size;

		if((entry->flags & JFILE_FLAG_COMRESSED) && (comp_size < JFILE_RAW_VERIFY_SIZE))
		{
			int copy_size = JFILE_RAW_VERIFY_SIZE - comp_size - verify_size;
			curr_size -= copy_size;

			memcpy(verify_data + verify_size, buffer + curr_size, copy_size);
			verify_size += copy_size;
		}

		if(curr_size > 0)
			crc32 = crc_append(crc32, buffer, curr_size);
	}

	if(comp_size > 0)
		return 0;

	if(entry->flags & JFILE_FLAG_COMRESSED)
		file_crc = *(ar_uint*)verify_data;
	else
		file_crc = entry->crc32;
	
	crc32 = CRC_GET_DIGEST(crc32);
	return (file_crc == crc32);
}
