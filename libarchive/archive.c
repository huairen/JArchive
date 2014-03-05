#include "archive.h"
#include "archive_type.h"
#include "filename_handle.h"
#include "file_table.h"
#include "file_io.h"
#include "thread.h"

struct archive *archive_create( const char *filename, int stream_flag )
{
	FILE *stream;
	struct archive * arc = NULL;
	int write_share = (stream_flag & JARCHIVE_STREAM_FALG_WRITE_SHARE) ? _SH_DENYNO : _SH_DENYWR;
	
	stream = _fsopen(filename, "wb+", write_share);
	if(stream == NULL)
		return NULL;

	arc = (struct archive *)malloc(sizeof(struct archive));
	if(arc == 0)
	{
		fclose(stream);
		return NULL;
	}

	memset(arc, 0, sizeof(struct archive));
	arc->stream = stream;

	return arc;
}

struct archive * archive_open( const char* filename, int stream_flag )
{
	struct archive *arc = NULL;
	struct archive_header header;
	FILE *in_stream;
	char *string_table = NULL;
	char *access = (stream_flag & JARCHIVE_STREAM_FALG_READ_ONLY) ? "rb" : "rb+";
	int write_share = (stream_flag & JARCHIVE_STREAM_FALG_WRITE_SHARE) ? _SH_DENYNO : _SH_DENYWR;
	
	in_stream = _fsopen(filename, access, write_share);
	if(in_stream == NULL)
		return NULL;

	fread(&header, sizeof(header), 1, in_stream);

	if((header.id != ID_ARCHIVE) || (header.version > ARCHIVE_VERSION))
		goto error_out;

	arc = (struct archive *)malloc(sizeof(struct archive));

	memset(arc, 0, sizeof(struct archive));
	arc->hash_table_size = header.hash_table_size;
	arc->file_table_size = header.file_table_size;
	arc->stream = in_stream;

	arc->hash_table = (struct hash_entry*)malloc(header.hash_table_size * sizeof(struct hash_entry));
	if(arc->hash_table == NULL)
		goto error_out;

	arc->file_table = (struct file_entry*)malloc(header.file_table_size * sizeof(struct file_entry));
	if(arc->file_table == NULL)
		goto error_out;

	string_table = (char*)malloc(header.string_table_size);
	if(string_table == NULL)
		goto error_out;

	_fseeki64(in_stream, header.table_offset, SEEK_SET);
	fread(arc->hash_table, sizeof(struct hash_entry), header.hash_table_size, in_stream);
	fread(arc->file_table, sizeof(struct file_entry), header.file_table_size, in_stream);
	fread(string_table, sizeof(char), header.string_table_size, in_stream);
	
	fill_filename(arc, string_table, header.string_table_size);
	free(string_table);

	return arc;

error_out:
	free(string_table);
	fclose(in_stream);

	if(arc)
	{
		free(arc->hash_table);
		free(arc->file_table);
		free(arc);
	}
	
	return NULL;
}

void archive_close( struct archive *arc )
{
	ar_uint i;

	if(arc == NULL) return;
	if(arc->data_update)
		flush_archive(arc);

	destroy_lock(arc);

	for (i = 0; i < arc->file_table_size; ++i)
		free((void*)(arc->file_table[i].filename));

	fclose(arc->stream);
	free(arc->file_table);
	free(arc->hash_table);
	free(arc);
}

int archive_add_file( struct archive *arc, const char *filename, const char *path, int compression )
{
	struct filename_node *filename_list = NULL;
	struct filename_node *filename_work = NULL;
	FILE *in_stream;
	struct stat statbuf;
	struct archive_file *file;
	const char *curr_filename = filename;
	int error = 0;

	struct add_file_progress progress;
	progress.current = 0;
	progress.total = 1;
	progress.user_data = arc->user_data;

	//如果是文件夹， 获取所有文件
	if(is_directory(filename))
	{
		int count = 0;
		char find_path[256];
		sprintf_s(find_path, 256, "%s\\*", filename);

		filename_work = filename_list = get_file_list(find_path, &count, NULL);
		curr_filename = filename_list ? filename_list->name : NULL;
		set_file_table_size(arc, arc->file_table_size + count);

		progress.total = count;
	}
	
	while(curr_filename != NULL)
	{
		if(fopen_s(&in_stream, curr_filename, "rb") != 0)
			error = 1;

		if(error == 0)
		{
			char szFileName[256];
			const char *relative_path = get_relative_path(curr_filename, filename);
			if(path && *path)
			{
				sprintf_s(szFileName, 256, "%s\\", path);
				strcat_s(szFileName, 256, relative_path);
				relative_path = szFileName;
			}

			stat(curr_filename, &statbuf);
			file = archive_file_create(arc, relative_path, statbuf.st_size, compression);
			if(file == NULL)
				error = 1;
			else
				file->entry->file_time = statbuf.st_mtime;
		}

		if(error == 0)
		{
#define BUFFER_SIZE 4096
			char buffer[BUFFER_SIZE];
			int data_size = statbuf.st_size;
			int curr_size;

			progress.file_name = curr_filename;

			while(data_size > 0)
			{
				curr_size = data_size;
				if(curr_size > BUFFER_SIZE)
					curr_size = BUFFER_SIZE;

				if(arc->progress_stop)
				{
					error = 1;
					break;
				}

				if(curr_size != fread(buffer, 1, curr_size, in_stream))
				{
					error = 1;
					break;
				}

				if(curr_size != archive_file_write(file, buffer, curr_size))
				{
					error = 1;
					break;
				}

				data_size -= curr_size;

				if(arc->progress_func)
				{
					progress.curr_per = 1.0f - ((float)data_size / statbuf.st_size);
					arc->progress_func((void*)&progress);
				}
			}
		}
		
		if(error == 0)
		{
			progress.current++;
			arc->data_update = 1;
			filename_work = filename_work ? filename_work->next : NULL;
			curr_filename = filename_work ? filename_work->name : NULL;
		}
		else
		{
			if(file->entry != NULL)
				clear_file_entry(file->arc, file->entry);
			curr_filename = NULL;
		}

		archive_file_close(file);
		file = NULL;

		fclose(in_stream);
	}

	//释放内存
	while(filename_list != NULL)
	{
		filename_work = filename_list;
		filename_list = filename_list->next;
		free(filename_work);
	}

	flush_archive(arc);
	archive_progress_stop(arc);

	return (error == 0);
}

int archive_remove_file( struct archive *arc, const char *filename )
{
	struct file_entry *entry = get_file_entry(arc, filename);
	if(entry == NULL)
		return 0;

	clear_file_entry(arc, entry);
	arc->data_update = 1;
	return 1;
}

int archive_extract_file( struct archive *arc, const char *filename, const char *save_path )
{
	FILE *out_stream;
	char  buffer[0x1000];
	ar_uint file_size;
	int curr_size;

	struct archive_file *file = archive_file_open(arc, filename);
	if(file == NULL)
		return 0;

	if(fopen_s(&out_stream, save_path, "wb") != 0)
	{
		archive_file_close(file);
		return 0;
	}

	file_size = file->entry->file_size;
	while(file_size > 0)
	{
		curr_size = archive_file_read(file, buffer, sizeof(buffer));

		if(curr_size == 0)
			break;

		if(curr_size != fwrite(buffer, 1, curr_size, out_stream))
			break;

		file_size -= curr_size;
	}

	archive_file_close(file);
	fclose(out_stream);

	return (file_size == 0);
}

int archive_extract_describe( struct archive *arc, const char *save_path )
{
	FILE *out_stream;
	struct archive_header header;
	ar_uint64 offset;
	ar_uint *old_flags;
	ar_uint i;
	int string_size = 0;

	if(fopen_s(&out_stream, save_path, "wb") != 0)
		return 0;

	old_flags = (ar_uint *)malloc(arc->file_table_size * sizeof(ar_uint));
	if(old_flags == NULL)
	{
		fclose(out_stream);
		return 0;
	}

	for (i = 0; i < arc->file_table_size; i++)
	{
		old_flags[i] = arc->file_table[i].flags;
		arc->file_table[i].flags |= JFILE_FLAG_INVAILD_DATA;
	}
	
	offset = find_last_pos(arc);

	_fseeki64(out_stream, sizeof(struct archive_header), SEEK_SET);
	fwrite(arc->hash_table, sizeof(struct hash_entry), arc->hash_table_size, out_stream);
	fwrite(arc->file_table, sizeof(struct file_entry), arc->file_table_size, out_stream);

	for (i = 0; i < arc->file_table_size; ++i)
	{
		struct file_entry* entry = arc->file_table + i;
		if(entry->flags & JFILE_FLAG_EXIST)
		{
			int len = strlen(entry->filename);
			string_size += fwrite(entry->filename, 1, len + 1, out_stream);
		}
	}

	for (i = 0; i < arc->file_table_size; i++)
		arc->file_table[i].flags = old_flags[i];

	free(old_flags);

	header.id = ID_ARCHIVE_INFO;
	header.version = ARCHIVE_VERSION;
	header.table_offset = offset;
	header.hash_table_size = arc->hash_table_size;
	header.file_table_size = arc->file_table_size;
	header.string_table_size = string_size;

	_fseeki64(out_stream, 0, SEEK_SET);
	fwrite(&header, sizeof(struct archive_header), 1, out_stream);

	fclose(out_stream);
	return 1;
}

int archive_create_by_describe( const char *filename, struct archive_file *desc_file )
{
	char  buffer[0x1000];
	struct archive_header header;
	FILE *stream = NULL;
	ar_uint table_size;
	ar_uint curr_size;

	archive_file_read(desc_file, &header, sizeof(header));
	if(header.id != ID_ARCHIVE_INFO)
		return 0;

	if(fopen_s(&stream, filename, "wb") != 0)
		return 0;

	header.id = ID_ARCHIVE;
	fwrite(&header, 1, sizeof(header), stream);

	_fseeki64(stream, header.table_offset, SEEK_SET);

	table_size = desc_file->entry->file_size;
	table_size -= sizeof(header);

	while(table_size > 0)
	{
		curr_size = get_min(sizeof(buffer), table_size);

		if(archive_file_read(desc_file, buffer, curr_size) != curr_size)
			break;

		fwrite(buffer, 1, curr_size, stream);

		table_size -= curr_size;
	}

	fclose(stream);
	return (table_size == 0);
}

void archive_multi_thread( struct archive *arc, int multi_thread )
{
	destroy_lock(arc);
	if(multi_thread)
		create_lock(arc);
}