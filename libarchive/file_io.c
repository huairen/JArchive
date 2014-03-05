#include "file_io.h"
#include "file_table.h"
#include "compression.h"
#include "crc.h"
#include "thread.h"


int allocate_sector_buffer( struct archive_file *file )
{
	file->sector_size = 4096;
	file->file_sector = (ar_byte*)malloc(file->sector_size);

	return file->file_sector ? 1 : 0;
}

int allocate_sector_offsets( struct archive_file* file, int load_file )
{
	struct file_entry *entry;
	int offset_len;

	entry = file->entry;

	file->sector_count = ((entry->file_size - 1) / file->sector_size) + 1;
	offset_len = (file->sector_count + 1) * sizeof(*(file->sector_offsets));

	file->sector_offsets = (ar_uint *)malloc(offset_len);
	if(file->sector_offsets == NULL)
		return 0;

	if(load_file)
	{
		ar_uint i;
		ar_uint error;

		read_data(file->arc, entry->offset, file->sector_offsets, offset_len);

		error = (file->sector_offsets[0] != offset_len);


		for (i = 0; (i < file->sector_count) && !error; i++)
		{
			ar_uint dwOffset0 = file->sector_offsets[i];
			ar_uint dwOffset1 = file->sector_offsets[i+1];

			error |= (dwOffset1 <= dwOffset0);
			error |= ((dwOffset1 - dwOffset0) > file->entry->comp_size);
		}

		if(error)
		{
			free(file->sector_offsets);
			file->sector_offsets = 0;
			return 0;
		}
	}
	else
	{
		memset(file->sector_offsets, 0, offset_len);
		file->sector_offsets[0] = offset_len;
	}

	return 1;
}

int read_sector_data( struct archive_file *file )
{
	struct file_entry *entry = file->entry;
	FILE *stream = file->arc->stream;
	ar_uint sector_index;
	ar_uint data_offset;
	ar_uint data_size;

	if(file->file_sector == NULL && !allocate_sector_buffer(file))
		return 0;

	sector_index = file->file_pos / file->sector_size;

	if(entry->flags & JFILE_FLAG_COMRESSED)
	{
		ar_byte *compress_data;
		ar_uint raw_size;
		int result;

		if(file->sector_offsets == NULL && !allocate_sector_offsets(file, 1))
			return 0;

		data_offset = file->sector_offsets[sector_index];
		data_size = file->sector_offsets[sector_index + 1] - data_offset;

		compress_data = (ar_byte*)malloc(data_size);
		if(compress_data == NULL)
			return 0;

		read_data(file->arc, entry->offset + data_offset, compress_data, data_size);

		if(file->sector_size * (sector_index + 1) > entry->file_size)
			raw_size = entry->file_size - (file->sector_size * sector_index);
		else
			raw_size = file->sector_size;

		result = uncompress_data(file->file_sector, &raw_size, compress_data, data_size);
		free(compress_data);

		if(!result)
			return 0;
	}
	else
	{
		data_offset = sector_index * file->sector_size;
		data_size = get_min(file->sector_size, entry->file_size - file->file_pos);

		read_data(file->arc, entry->offset + data_offset, file->file_sector, data_size);
	}
			
	file->sector_index = sector_index;
	return 1;
}

int write_sector_data( struct archive_file *file )
{
	struct file_entry *entry;
	FILE *stream;
	ar_uint64 offset;
	ar_uint write_size;
	ar_uint bytes_write;
	ar_uint crc32;
	
	entry = file->entry;
	stream = file->arc->stream;

	write_size = file->file_pos % file->sector_size;
	if(write_size == 0)
		write_size = file->sector_size;

	offset = entry->offset + entry->comp_size;
	crc32 = crc_append(entry->crc32, file->file_sector, write_size);

	if(entry->flags & JFILE_FLAG_COMRESSED)
	{
		ar_uint comp_size;
		ar_byte *comp_data;
		ar_uint return_size;

		if(file->sector_offsets == NULL)
		{
			if(!allocate_sector_offsets(file, 0))
				return 0;

			file->entry->comp_size += file->sector_offsets[0];
			offset += file->sector_offsets[0];
		}

		comp_size = file->sector_size + 0x100;
		comp_data = (ar_byte*)malloc(comp_size);
		if(comp_data == NULL)
			return 0;

		if(!compress_data(comp_data, &comp_size, file->file_sector, write_size, file->compression))
			return 0;

		return_size = write_data(file->arc, offset, comp_data, comp_size);
		free(comp_data);

		if(return_size != comp_size)
			return 0;

		entry->comp_size += comp_size;
		if(file->sector_offsets != NULL)
			file->sector_offsets[file->sector_index + 1] = file->sector_offsets[file->sector_index] + comp_size;
	}
	else
	{
		//如果无压缩，直接把扇区数据写入
		bytes_write = write_data(file->arc, offset, file->file_sector, write_size);
		if(write_size != bytes_write)
			return 0;

		entry->comp_size += write_size;
	}

	entry->crc32 = crc32;

	if(file->file_pos >= entry->file_size)
	{
		entry->crc32 = CRC_GET_DIGEST(entry->crc32);

		if(entry->flags & JFILE_FLAG_COMRESSED)
		{
			if(file->sector_offsets != NULL)
			{
				write_size = file->sector_offsets[0];
				bytes_write = write_data(file->arc, entry->offset, file->sector_offsets, write_size);
				if(write_size != bytes_write)
					return 0;
			}

			entry->comp_size += write_raw_data_verify(file->arc, entry->offset, entry->comp_size);
		}
	}

	return 1;
}

int read_data(struct archive *arc, ar_uint64 offset, void *data, int size )
{
	int bytes_read;

	lock(arc);
	_fseeki64(arc->stream, offset, SEEK_SET);
	bytes_read = fread(data, 1, size, arc->stream);
	unlock(arc);

	return bytes_read;
}

int write_data(struct archive *arc, ar_uint64 offset, void *data, int size )
{
	int bytes_write;

	lock(arc);
	_fseeki64(arc->stream, offset, SEEK_SET);
	bytes_write = fwrite(data, 1, size, arc->stream);
	unlock(arc);

	return bytes_write;
}

int flush_archive( struct archive *arc )
{
	struct archive_header header;
	ar_uint64 offset;
	ar_uint string_size = 0;
	ar_uint i;

	if(arc->file_table_size == 0)
		return 1;

	offset = find_last_pos(arc);

	header.id = ID_ARCHIVE;
	header.version = ARCHIVE_VERSION;
	header.table_offset = offset;
	header.hash_table_size = arc->hash_table_size;
	header.file_table_size = arc->file_table_size;

	offset += write_data(arc, offset, arc->hash_table, sizeof(struct hash_entry) * arc->hash_table_size);
	offset += write_data(arc, offset, arc->file_table, sizeof(struct file_entry) * arc->file_table_size);

	for (i = 0; i < arc->file_table_size; ++i)
	{
		struct file_entry* entry = arc->file_table + i;
		if(entry->flags & JFILE_FLAG_EXIST)
		{
			int len = strlen(entry->filename);
			int bytes_write = write_data(arc, offset, (void*)entry->filename, len + 1);

			offset += bytes_write;
			string_size += bytes_write;
		}
	}

	header.string_table_size = string_size;

	write_data(arc, 0, &header, sizeof(struct archive_header));
	arc->data_update = 0;

	return 1;
}

int save_file_flag( struct archive *arc, struct file_entry *entry )
{
	ar_uint64 offset;

	if(arc->data_update)
		return flush_archive(arc);

	offset = find_last_pos(arc);
	offset += (arc->hash_table_size * sizeof(struct hash_entry));
	offset += ((entry - arc->file_table) * sizeof(struct file_entry));
	offset += ((ar_uint)(&(entry->flags)) - (ar_uint)entry);

	return write_data(arc, offset, &(entry->flags), sizeof(entry->flags));
}

ar_uint write_raw_data_verify( struct archive *arc, ar_uint64 raw_data_offset, ar_uint raw_data_size )
{
	ar_uint crc32 = CRC_INIT_VAL;
	ar_uint read_size = raw_data_size;
	ar_byte buffer[4096];
	ar_uint curr_size;
	ar_uint64 curr_pos = raw_data_offset;

	while(read_size != 0)
	{
		curr_size = get_min(sizeof(buffer), read_size);
		if(read_data(arc, curr_pos, buffer, curr_size)!= curr_size)
			break;

		crc32 = crc_append(crc32, buffer, curr_size);
		read_size -= curr_size;
		curr_pos += curr_size;
	}

	if(read_size > 0)
		return 0;

	crc32 = CRC_GET_DIGEST(crc32);
	write_data(arc, curr_pos, &crc32, sizeof(crc32));

	return JFILE_RAW_VERIFY_SIZE;
}

ar_uint get_raw_size(struct file_entry *entry)
{
	if(entry->flags & JFILE_FLAG_COMRESSED)
		return entry->comp_size - JFILE_RAW_VERIFY_SIZE;
	return entry->comp_size;
}

ar_uint get_min(ar_uint a, ar_uint b)
{
	return (a < b) ? a : b;
}
