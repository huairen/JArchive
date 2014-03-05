#ifndef _ARCHIVE_H_
#define _ARCHIVE_H_

#ifdef __cplusplus
extern "C" {
#endif

#define JARCHIVE_COMPRESS_NONE       0x00
#define JARCHIVE_COMPRESS_ZLIB       0x01
#define JARCHIVE_COMPRESS_LZMA       0x02

#define JARCHIVE_FILE_SIZE              1
#define JARCHIVE_FILE_COMPRESS_SIZE     2
#define JARCHIVE_FILE_POSITION          3
#define JARCHIVE_FILE_NAME              4
#define JARCHIVE_FILE_TIME              5
#define JARCHIVE_FILE_IS_COMPLETE       6
#define JARCHIVE_FILE_INDEX             7

#define JARCHIVE_STREAM_FALG_READ_ONLY   0x0001
#define JARCHIVE_STREAM_FALG_WRITE_SHARE 0x0002

#define JARCHIVE_VERIFY_FILE_CRC        0x0001
#define JARCHIVE_VERIFY_FILE_MD5        0x0002

struct archive;
struct archive_file;
struct archive_connect;

struct add_file_progress
{
	int total;      //总文件数量
	int current;    //当前加载文件索引
	float curr_per; //当前文件加载的百分比
	const char* file_name;
	void *user_data;
};

typedef void (*PROGRESS_CALLBACK)(void* progress_data);

//////////////////////////////////////////////////////////////////////////
//archvie operate
struct archive *archive_create(const char *filename, int stream_flag);
struct archive *archive_open(const char *filename, int stream_flag);
void archive_close(struct archive *arc);

int archive_add_file(struct archive *arc, const char *filename, const char *path, int compression);
int archive_remove_file(struct archive *arc, const char *filename);
int archive_extract_file(struct archive *arc, const char *filename, const char *save_path);
int archive_extract_describe(struct archive *arc, const char *save_path);
int archive_create_by_describe(const char *filename, struct archive_file *desc_file);

void archive_multi_thread(struct archive *arc, int multi_thread);

//////////////////////////////////////////////////////////////////////////
//file operate
struct archive_file *archive_file_create(struct archive *arc, const char *filename, int file_size, int compression);
struct archive_file *archive_file_open(struct archive *arc, const char *filename);
struct archive_file *archive_file_open_by_index(struct archive *arc, int file_index);
struct archive_file *archive_file_clone(struct archive_file *file);
void archive_file_close(struct archive_file *file);

int archive_file_count(struct archive *arc);

int archive_file_seek(struct archive_file *file, unsigned int pos);
int archive_file_read(struct archive_file *file, void *buffer, int buff_size);
int archive_file_write(struct archive_file *file, void *buffer, int buff_size);
int archive_file_copy(struct archive_file *file, struct archive *dest_arc, const char *dest_path);

void archive_file_property(struct archive_file *file, int prop, void *buffer, int buff_size);

struct archive_file *archive_find_first(struct archive *arc);
int archive_find_next(struct archive_file *file);

// @param flag to verify method(crc or md5 ...)
int archive_verify_file(struct archive_file *file, int flag);
int archive_verify_raw_data(struct archive_file *file);

//to stop all operate(addfile or downloadfile...)
void archive_progress_stop(struct archive *arc);
void archive_progress_start(struct archive *arc, PROGRESS_CALLBACK func, void* user_data);

// download
void archive_download_add_file(struct archive_connect *conn, struct archive_file* file, const char *resource);
struct archive_connect *archive_download_connect(const char *host, unsigned short port);
int archive_download_batch(struct archive_connect *conn, unsigned int *download_size);
int archive_download_file(struct archive_connect *conn, struct archive_file* file, const char *url);
void archive_download_close(struct archive_connect *conn);
void archive_download_interrupt(struct archive_connect *conn);

#ifdef __cplusplus
};
#endif

#endif