#ifndef _FILE_TABLE_H_
#define _FILE_TABLE_H_

#include "archive_type.h"

struct file_entry *allocate_file_entry(struct archive *arc, const char* filename);
struct file_entry *find_free_file_entry(struct archive *arc);
void clear_file_entry(struct archive *arc, struct file_entry *entry);
int allocate_hash_entry(struct archive *arc, struct file_entry *entry);

int set_file_table_size(struct archive *arc, ar_uint max_size);
ar_uint64 find_free_space(struct archive *arc, ar_uint file_size);
ar_uint64 find_last_pos(struct archive *arc);

ar_uint64 hash_string(const char *filename);
int get_file_index(struct archive *arc, ar_uint64 key);
struct file_entry *get_file_entry(struct archive *arc, const char *filename);

#endif