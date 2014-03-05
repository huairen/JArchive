#include "filename_handle.h"
#include "archive_type.h"

void fill_filename( struct archive *arc, char *string_table, int table_size )
{
	struct file_entry *entry;
	ar_uint i;

	for (i=0; i<arc->file_table_size; i++)
	{
		if(table_size <= 0)
			break;

		entry = arc->file_table + i;
		if(entry->flags & JFILE_FLAG_EXIST)
		{
			entry->filename = _strdup(string_table);
			string_table += strlen(entry->filename) + 1;
		}
	}
}

int is_directory(const char *path)
{
	struct stat statbuff;
	if(-1 == stat(path, &statbuff))
		return 0;

	return (statbuff.st_mode & S_IFDIR);
}

const char* get_filename( const char *path )
{
	size_t i = strlen(path) - 1;
	for (; i >= 0; i--)
	{
		if((path[i] == '/') || (path[i] == '\\'))
			return &path[i + 1];
	}
	return path;
}

struct filename_node * get_file_list( const char *path, int *count, struct filename_node *file_node /*= NULL*/ )
{
	struct _finddata_t file_info;

	intptr_t find_handle = _findfirst(path, &file_info);
	if (find_handle == -1)
		return file_node;

	do 
	{
		if (file_info.attrib &  _A_SUBDIR)
		{
			char sub_puth[256];
			size_t len;

			if (file_info.attrib & _A_SYSTEM)
				continue;

			// skip . and .. directories
			if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0)
				continue;

			len = strlen(path) - 1;

			strncpy_s(sub_puth, 256, path, len);
			strcat_s(sub_puth, 256, file_info.name);
			strcat_s(sub_puth, 256, "\\*");
			file_node = get_file_list(sub_puth, count, file_node);
		}      
		else
		{
			struct filename_node *new_node;
			size_t len;

			// make sure it is the kind of file we're looking for
			if (file_info.attrib & _A_SYSTEM)
				continue;

			// add it to the list
			new_node = (struct filename_node*)malloc(sizeof(struct filename_node));
			len = strlen(path) - 1;

			strncpy_s(new_node->name, 256, path, len);
			strcat_s(new_node->name, 256, file_info.name);
			new_node->next = file_node;
			file_node = new_node;

			if(count != NULL)
				(*count) ++;
		}

	}while(_findnext(find_handle, &file_info) == 0);

	_findclose(find_handle);
	return file_node;
}

const char* get_relative_path( const char *src_path, const char *compare_path )
{
	if(is_directory(compare_path))
	{
		const char *name = get_filename(compare_path);
		if(name[0] != 0)
			return src_path + (name - compare_path);
	}

	return get_filename(src_path);
}
