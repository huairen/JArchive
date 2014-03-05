#ifndef _FILE_IO_H_
#define _FILE_IO_H_

#include "archive_type.h"

int allocate_sector_buffer( struct archive_file *file );
int allocate_sector_offsets( struct archive_file* file, int load_file );

int read_sector_data( struct archive_file *file );
int write_sector_data( struct archive_file *file );

int read_data(struct archive *arc, ar_uint64 offset, void *data, int size);
int write_data(struct archive *arc, ar_uint64 offset, void *data, int size);

int flush_archive( struct archive *arc );
int save_file_flag( struct archive *arc, struct file_entry *entry );

ar_uint write_raw_data_verify(struct archive *arc, ar_uint64 raw_data_offset, ar_uint raw_data_size);
ar_uint get_raw_size(struct file_entry *entry);

ar_uint get_min(ar_uint a, ar_uint b);

#endif