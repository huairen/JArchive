#include "file_table.h"
#include "file_io.h"

struct empty_data
{
	ar_uint64 start_pos;
	ar_uint64 end_pos;
	struct empty_data *prev;
	struct empty_data *next;
};

struct empty_data * empty_data_insert(struct empty_data *node, struct empty_data **list, struct empty_data **alloc_list_ptr)
{
	struct empty_data *new_data;
	if(alloc_list_ptr == NULL || *alloc_list_ptr == NULL)
	{
		new_data = (struct empty_data*)malloc(sizeof(struct empty_data));
	}
	else
	{
		new_data = *alloc_list_ptr;
		*alloc_list_ptr = (*alloc_list_ptr)->next;
	}

	if(new_data == NULL)
		return NULL;
	
	new_data->start_pos = 0;
	new_data->end_pos = 0;

	if(node)
	{
		new_data->prev = node->prev;
		new_data->next = node;

		if(node->prev)
			node->prev->next = new_data;
		node->prev = new_data;

		if(node == *list)
			*list = new_data;
	}
	
	return new_data;
}

struct empty_data *empty_data_push(struct empty_data **list, struct empty_data **alloc_list_ptr)
{
	struct empty_data *last_node;
	struct empty_data *new_data;

	for (last_node = *list; last_node&&last_node->next; last_node = last_node->next);

	if(alloc_list_ptr == NULL || *alloc_list_ptr == NULL)
	{
		new_data = (struct empty_data*)malloc(sizeof(struct empty_data));
	}
	else
	{
		new_data = *alloc_list_ptr;
		*alloc_list_ptr = (*alloc_list_ptr)->next;
	}

	if(new_data == NULL)
		return NULL;

	new_data->start_pos = 0;
	new_data->end_pos = 0;
	new_data->prev = NULL;
	new_data->next = NULL;

	if(last_node)
	{
		new_data->prev = last_node;
		last_node->next = new_data;
	}
	else
	{
		*list = new_data;
	}
	
	return new_data;
}

void empty_data_delete(struct empty_data *node, struct empty_data **list, struct empty_data **alloc_list_ptr)
{
	if(node == *list)
		*list = node->next;

	if(node->prev)
		node->prev->next = node->next;
	if(node->next)
		node->next->prev = node->prev;

	node->prev = NULL;
	node->next = *alloc_list_ptr;
	*alloc_list_ptr = node;
}

void empty_data_list_clear(struct empty_data *list)
{
	struct empty_data *temp;
	while (list != NULL)
	{
		temp = list;
		list = list->next;
		free(temp);
	}
}



//////////////////////////////////////////////////////////////////////////

struct file_entry * allocate_file_entry( struct archive *arc, const char* filename )
{
	struct file_entry *free_entry;

	free_entry = find_free_file_entry(arc);
	if (free_entry == 0)
	{
		ar_uint old_size = arc->file_table_size;
		set_file_table_size(arc, old_size + 16);
		free_entry = arc->file_table + old_size;
	}

	if (free_entry != NULL)
	{
		free_entry->filename = _strdup(filename);
		allocate_hash_entry(arc, free_entry);
	}

	return free_entry;
}

struct file_entry *find_free_file_entry( struct archive *arc )
{
	int index;
	int table_size;
	struct file_entry * file_table;
	struct file_entry * free_entry = NULL;

	table_size = arc->file_table_size;
	file_table = arc->file_table;
	if(file_table == NULL)
		return NULL;

	for (index = 0; index < table_size; ++index)
	{
		if((file_table[index].flags & JFILE_FLAG_EXIST) == 0)
		{
			free_entry = file_table + index;
			break;
		}			
	}

	if(free_entry != NULL)
		clear_file_entry(arc, free_entry);

	return free_entry;
}

void clear_file_entry( struct archive *arc, struct file_entry *entry )
{
	int index;
	struct hash_entry *hash_table;

	hash_table = arc->hash_table;

	if(entry->comp_size > 0)
	{
		index = entry->hash_index;
		if(hash_table[index].file_index == (entry - (arc->file_table)))
		{
			hash_table[index].key = HASH_KEY_DELETED;
			hash_table[index].file_index = TABLE_INDEX_NULL;
		}
	}

	if(entry->filename != NULL)
		free((void*)(entry->filename));

	memset(entry, 0, sizeof(*entry));
}

int allocate_hash_entry( struct archive *arc, struct file_entry *entry )
{
	ar_uint64 filename_hash;
	int index;
	int start_index;
	int free_index = TABLE_INDEX_NULL;
	int table_size;
	struct file_entry *file_table;
	struct hash_entry *hash_table;

	file_table = arc->file_table;
	hash_table = arc->hash_table;
	table_size = arc->hash_table_size;

	filename_hash  = hash_string(entry->filename);
	start_index = index = (int)(filename_hash % table_size);

	for (;;)
	{
		if((hash_table[index].key == HASH_KEY_NULL) || (hash_table[index].key == HASH_KEY_DELETED))
		{
			free_index = index;
			break;
		}

		index = (index + 1) % table_size;
		if(index == start_index)
			return TABLE_INDEX_NULL;
	}

	entry->hash_index = free_index;
	hash_table[free_index].key = filename_hash;
	hash_table[free_index].file_index = (entry - file_table);

	return free_index;
}

int set_file_table_size( struct archive *arc, ar_uint max_size )
{
	struct hash_entry *old_hash_table;
	struct file_entry *old_file_table;
	int old_hash_table_size;
	int old_file_table_size;
	int i;

	if(max_size <= arc->file_table_size)
		return 0;

	old_file_table_size = arc->file_table_size;
	old_file_table = arc->file_table;
	old_hash_table_size = arc->hash_table_size;
	old_hash_table = arc->hash_table;

	arc->file_table = 0;
	arc->hash_table = 0;

	arc->file_table_size = max_size;
	arc->file_table = (struct file_entry *)malloc(arc->file_table_size * sizeof(struct file_entry));
	if(arc->file_table == NULL)
		goto error_out;

	memset(arc->file_table, 0, arc->file_table_size * sizeof(struct file_entry));


	arc->hash_table_size = max_size * 4 / 3;
	arc->hash_table = (struct hash_entry *)malloc(arc->hash_table_size * sizeof(struct hash_entry));
	if(arc->hash_table == NULL)
		goto error_out;

	memset(arc->hash_table, 0, arc->hash_table_size * sizeof(struct hash_entry));

	memcpy(arc->file_table, old_file_table, old_file_table_size * sizeof(struct file_entry));
	for (i = 0; i < old_file_table_size; i++)
	{
		struct file_entry *entry = arc->file_table + i;
		if(allocate_hash_entry(arc, entry) == TABLE_INDEX_NULL)
			goto error_out;
	}

	free(old_file_table);
	free(old_hash_table);
	return 1;

error_out:
	free(arc->file_table);
	free(arc->hash_table);

	arc->file_table = old_file_table;
	arc->hash_table = old_hash_table;
	arc->file_table_size = old_file_table_size;
	arc->hash_table_size = old_hash_table_size;
	return 0;
}

ar_uint64 find_free_space( struct archive *arc, ar_uint file_size )
{
	struct empty_data *list = NULL, *alloc_list = NULL, *new_node;
	struct empty_data *work_ptr, *use_ptr = NULL;

	int error = 1;
	ar_uint i;
	ar_uint64 last_pos = sizeof(struct archive_header);
	
	for (i = 0; i < arc->file_table_size; ++i)
	{
		struct file_entry *entry = arc->file_table + i;
		if(entry->flags & JFILE_FLAG_EXIST)
		{
			ar_uint64 entry_start = entry->offset;
			ar_uint64 entry_end = entry->offset + entry->comp_size;
			// 如果当前文件跟上个文件位置不连续，要记录中间的无用空间
			if(entry_start > last_pos)
			{
				new_node = empty_data_push(&list, &alloc_list);
				if(new_node == NULL)
					goto error_out;
				
				new_node->start_pos = last_pos;
				new_node->end_pos = entry_start;
			}
			else if(entry_start < last_pos) // 如果当前文件在前面的无用空间里，要在无用空间列表里查找位置
			{
				for (work_ptr = list; work_ptr; work_ptr = work_ptr->next)
				{
					if(entry_start >= work_ptr->start_pos && entry_end <= work_ptr->end_pos)
					{
						ar_uint64 work_start_pos = work_ptr->start_pos;
						ar_uint64 work_end_pos = work_ptr->end_pos;

						if(entry_start > work_start_pos)
						{
							new_node = empty_data_insert(work_ptr, &list, &alloc_list);
							if(new_node == NULL)
								goto error_out;

							new_node->start_pos = work_start_pos;
							new_node->end_pos = entry_start;
						}

						if(entry_end < work_end_pos)
						{
							new_node = empty_data_insert(work_ptr, &list, &alloc_list);
							if(new_node == NULL)
								goto error_out;

							new_node->start_pos = entry_end;
							new_node->end_pos = work_end_pos;
						}

						empty_data_delete(work_ptr, &list, &alloc_list);
						break;
					}
				}
			}

			if(entry_end > last_pos)
				last_pos = entry_end;
		}
	}

	for (work_ptr = list; work_ptr; work_ptr = work_ptr->next)
	{
		ar_uint data_size = (ar_uint)(work_ptr->end_pos - work_ptr->start_pos);
		if(data_size == file_size)
		{
			use_ptr = work_ptr;
			break;
		}

		if(data_size > file_size)
		{
			ar_uint curr_size = 0xffffffff;
			if(use_ptr != NULL)
				curr_size = (ar_uint)(use_ptr->end_pos - use_ptr->start_pos);

			if(data_size < curr_size)
				use_ptr = work_ptr;
		}
	}

	if(use_ptr != NULL)
		last_pos = use_ptr->start_pos;

	error = 0; // 无错误，正常执行完

error_out:
	empty_data_list_clear(list);
	empty_data_list_clear(alloc_list);

	if(error)
		return find_last_pos(arc);
	return last_pos;
}

ar_uint64 find_last_pos( struct archive *arc )
{
	ar_uint i;
	ar_uint64 offset = sizeof(struct archive_header);
	for (i = 0; i < arc->file_table_size; i++)
	{
		ar_uint64 end_pos = arc->file_table[i].offset + arc->file_table[i].comp_size;
		if(end_pos > offset)
			offset = end_pos;
	}
	return offset;
}

int get_file_index( struct archive *arc, ar_uint64 key )
{
	ar_uint start_index;
	ar_uint index;
	ar_uint table_size;
	struct hash_entry *hash_table;

	hash_table = arc->hash_table;
	table_size = arc->hash_table_size;

	if(hash_table != NULL && table_size > 0)
	{
		start_index = index = (ar_uint)(key % table_size);

		while(hash_table[index].key != HASH_KEY_NULL)
		{
			if(hash_table[index].key == key)
				return hash_table[index].file_index;

			index = (index + 1) % table_size;
			if(index == start_index)
				break;
		}
	}

	return TABLE_INDEX_NULL;
}

struct file_entry * get_file_entry( struct archive *arc, const char *filename )
{
	ar_uint index;
	ar_uint64 key;

	if(arc->file_table == NULL)
		return NULL;

	key = hash_string(filename);
	index = get_file_index(arc, key);
	if(index >= arc->file_table_size)
		return NULL;

	return arc->file_table + index;
}
