#include "archive.h"
#include "archive_type.h"
#include "file_table.h"
#include "file_io.h"
#include "crc.h"


struct archive_file * archive_file_create( struct archive* arc, const char *filename, int file_size, int compression )
{
	struct archive_file *file;
	struct file_entry *entry;

	entry = get_file_entry(arc, filename);
	if(entry != 0)
		clear_file_entry(arc, entry);

	entry = allocate_file_entry(arc, filename);
	if(entry == NULL)
		return 0;

	file = (struct archive_file *)malloc(sizeof(struct archive_file));
	if(file == 0)
	{
		clear_file_entry(arc, entry);
		return NULL;
	}

	memset(file, 0, sizeof(struct archive_file));
	file->entry = entry;
	file->arc = arc;
	file->file_pos = 0;
	file->compression = compression;

	if(compression == JARCHIVE_COMPRESS_NONE)
		entry->offset = find_free_space(arc, file_size);
	else
		entry->offset = find_last_pos(arc);

	entry->file_size = file_size;
	entry->file_time = 0;
	entry->comp_size = 0;
	entry->flags = JFILE_FLAG_EXIST | ((compression == JARCHIVE_COMPRESS_NONE) ? 0 : JFILE_FLAG_COMRESSED);
	entry->crc32 = CRC_INIT_VAL;

	return file;
}

struct archive_file *archive_file_open(struct archive* arc, const char *filename)
{
	struct archive_file *file;
	struct file_entry *entry;

	entry = get_file_entry(arc, filename);
	if(entry == NULL)
		return NULL;

	file = (struct archive_file*)malloc(sizeof(struct archive_file));
	if(file == NULL)
		return NULL;

	memset(file, 0, sizeof(struct archive_file));

	file->arc = arc;
	file->entry = entry;

	return file;
}

struct archive_file * archive_file_open_by_index( struct archive *arc, int file_index )
{
	struct archive_file *file;
	struct file_entry *entry;

	if((ar_uint)file_index >= arc->file_table_size)
		return NULL;

	entry = arc->file_table + file_index;
	if(entry == NULL)
		return NULL;

	file = (struct archive_file*)malloc(sizeof(struct archive_file));
	if(file == NULL)
		return NULL;

	memset(file, 0, sizeof(struct archive_file));

	file->arc = arc;
	file->entry = entry;

	return file;
}

struct archive_file *archive_file_clone(struct archive_file *file)
{
	struct archive_file *copy;

	if(file == NULL)
		return NULL;

	copy = (struct archive_file*)malloc(sizeof(struct archive_file));
	if(copy == NULL)
		return NULL;

	memset(copy, 0, sizeof(struct archive_file));

	copy->arc = file->arc;
	copy->entry = file->entry;

	return copy;
}

void archive_file_close(struct archive_file *file)
{
	if(file)
	{
		free(file->file_sector);
		free(file->sector_offsets);
		free(file);
	}
}

int archive_file_count( struct archive *arc )
{
	int count = 0;
	ar_uint i;

	for (i=0; i<arc->file_table_size; i++)
	{
		if(arc->file_table[i].flags & JFILE_FLAG_EXIST)
			count++;
	}

	return count;
}

int archive_file_seek(struct archive_file *file, ar_uint pos)
{
	file->file_pos = get_min(file->entry->file_size, pos);
	return 1;
}

int archive_file_read(struct archive_file *file, void *buffer, int buff_size)
{
	struct file_entry *entry = file->entry;
	ar_uint sector_index;
	ar_uint sector_pos;
	int read_size = 0;
	int curr_size;
	int result = 1;
	
	if(entry->flags & JFILE_FLAG_INVAILD_DATA || buff_size == 0)
		return 0;

	if(file->file_pos + buff_size > entry->file_size)
		buff_size = entry->file_size - file->file_pos;

	if(file->file_sector == NULL)
	{
		if(!allocate_sector_buffer(file))
			return 0;

		file->sector_index = 0xffffffff;
	}

	while(buff_size > 0)
	{
		sector_index = file->file_pos / file->sector_size;

		if(sector_index != file->sector_index)
			result = read_sector_data(file);

		if(!result)
			break;

		sector_pos = file->file_pos % file->sector_size;
		curr_size  = get_min(file->sector_size - sector_pos, buff_size);

		memcpy(buffer, file->file_sector + sector_pos, curr_size);

		file->file_pos += curr_size;
		read_size += curr_size;
		buff_size -= curr_size;
		(ar_byte*)buffer += curr_size;
	}

	return read_size;
}

int archive_file_write(struct archive_file *file, void *buffer, int buff_size)
{
	struct file_entry *entry = file->entry;
	FILE *stream = file->arc->stream;
	ar_uint sector_pos;
	ar_uint curr_size;
	ar_uint write_size = 0;
	int result = 1;

	if(file->file_sector == NULL && !allocate_sector_buffer(file))
		return 0;

	sector_pos = file->file_pos % file->sector_size;
	file->sector_index = file->file_pos / file->sector_size;

	while(buff_size > 0 && result)
	{
		curr_size = get_min(buff_size, file->sector_size - sector_pos);

		memcpy(file->file_sector + sector_pos, buffer, curr_size);

		sector_pos += curr_size;
		write_size += curr_size;
		buff_size -= curr_size;
		(ar_byte*)buffer += curr_size;

		file->file_pos += curr_size;

		if(sector_pos >= file->sector_size || file->file_pos >= entry->file_size)
			result = write_sector_data(file);
	}
	
	return write_size;
}

int archive_file_copy(struct archive_file *file, struct archive *dest_arc, const char *dest_path)
{
	struct file_entry *src_entry = file->entry;
	struct file_entry *dest_entry;
	struct archive_file *dest_file;

	char buffer[4096];
	ar_uint copy_size;
	ar_uint curr_size;
	ar_uint64 write_pos;
	ar_uint64 read_pos;

	if(src_entry->flags & JFILE_FLAG_INVAILD_DATA)
		return 0;

	dest_file = archive_file_create(dest_arc, dest_path, src_entry->file_size, 0);
	if(dest_file == NULL)
		return 0;

	dest_entry = dest_file->entry;
	dest_entry->file_time = src_entry->file_time;
	dest_entry->comp_size = src_entry->comp_size;
	dest_entry->flags = src_entry->flags;
	dest_entry->crc32 = src_entry->crc32;

	if(dest_entry->file_size != src_entry->comp_size)
		dest_entry->offset = find_free_space(dest_arc, src_entry->comp_size);

	read_pos = src_entry->offset;
	write_pos = dest_entry->offset;
	copy_size = dest_entry->comp_size;

	while(copy_size > 0)
	{
		curr_size = get_min(sizeof(buffer), copy_size);

		if(read_data(file->arc, read_pos, buffer, curr_size) != curr_size)
			break;

		if(write_data(dest_arc, write_pos, buffer, curr_size) != curr_size)
			break;

		read_pos += curr_size;
		write_pos += curr_size;
		copy_size -= curr_size;
	}

	if(copy_size > 0)
		dest_entry->flags |= JFILE_FLAG_INVAILD_DATA;

	archive_file_close(dest_file);
	flush_archive(dest_arc);

	return (copy_size == 0);
}

void archive_file_property(struct archive_file *file, int prop, void *buffer, int buff_size)
{
	const void *src_data;
	int src_size;
	ar_uint temp_data;

	switch (prop)
	{
	case JARCHIVE_FILE_SIZE:
		src_data = &(file->entry->file_size);
		src_size = sizeof(file->entry->file_size);
		break;
	case JARCHIVE_FILE_COMPRESS_SIZE:
		src_data = &(file->entry->comp_size);
		src_size = sizeof(file->entry->comp_size);
		break;
	case JARCHIVE_FILE_POSITION:
		src_data = &(file->file_pos);
		src_size = sizeof(file->file_pos);
		break;
	case JARCHIVE_FILE_NAME:
		src_data = file->entry->filename;
		src_size = strlen(file->entry->filename) + 1;
		break;
	case JARCHIVE_FILE_TIME:
		src_data = &(file->entry->file_time);
		src_size = sizeof(file->entry->file_time);
		break;
	case JARCHIVE_FILE_IS_COMPLETE:
		temp_data = !(file->entry->flags & JFILE_FLAG_INVAILD_DATA);
		src_data = &temp_data;
		src_size = 1;
		break;
	case JARCHIVE_FILE_INDEX:
		temp_data = (file->entry - file->arc->file_table);
		src_data = &temp_data;
		src_size = 4;
		break;
	}

	if(src_size > buff_size)
		src_size = buff_size;

	memcpy(buffer, src_data, src_size);
}

struct archive_file * archive_find_first( struct archive *arc )
{
	struct archive_file *file;
	struct file_entry *entry;
	struct file_entry *end_entry;

	end_entry = arc->file_table + arc->file_table_size;
	for(entry = arc->file_table; (entry != end_entry) && !(entry->flags & JFILE_FLAG_EXIST); entry++);

	if(entry == end_entry)
		return NULL;

	file = (struct archive_file*)malloc(sizeof(struct archive_file));
	if(file == NULL)
		return NULL;

	memset(file, 0, sizeof(struct archive_file));

	file->arc = arc;
	file->entry = entry;

	return file;
}

int archive_find_next( struct archive_file *file )
{
	struct file_entry *entry;
	struct file_entry *end_entry;

	end_entry = file->arc->file_table + file->arc->file_table_size;
	for(entry = file->entry+1; (entry != end_entry) && !(entry->flags & JFILE_FLAG_EXIST); entry++);

	if(entry == end_entry)
		return 0;

	file->file_pos = 0;
	file->entry = entry;
	file->sector_index = 0xffffffff;

	free(file->sector_offsets);
	file->sector_offsets = NULL;

	return 1;
}

void archive_progress_stop(struct archive *arc)
{
	arc->progress_stop = 1;
	arc->progress_func = NULL;
}

void archive_progress_start(struct archive *arc, PROGRESS_CALLBACK func, void *user_data)
{
	arc->progress_stop = 0;
	arc->progress_func = func;
	arc->user_data = user_data;
}