#ifndef _FILENAME_HANDLE_H_
#define _FILENAME_HANDLE_H_

struct archive;
struct filename_node
{
	char name[256];
	struct filename_node* next;
};

void fill_filename(struct archive *arc, char *string_table, int table_size);
int is_directory(const char *path);
const char* get_filename(const char *path);
struct filename_node *get_file_list(const char *path, int *count, struct filename_node *file_node);
const char* get_relative_path(const char *src_path, const char *compare_path);

#endif