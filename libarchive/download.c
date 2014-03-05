#include "archive_type.h"
#include "file_io.h"
#include "crc.h"
#include "verify.h"

static int init_sock = 0;

enum ParseState
{
	ParsingStatusLine,
	ParsingHeader,
	ParsingChunkHeader,
	ProcessingBody,
	ProcessingDone,
};

struct file_item
{
	struct file_entry *entry;
	struct file_item *next;
};

struct archive_block
{
	struct archive *arc;
	char url[256];
	struct file_item *file_list;
	struct archive_block *next;
};

struct archive_connect
{
	SOCKET sock;
	char host_name[64];
	float http_version;
	ar_uint response_status;
	char status_text[64];
	ar_uint64 content_length;
	char content_type[32];
	int chunked_encoding;
	int parse_state;
	volatile int download_interrupt;

	struct archive_block *download_list;
};

// 批量下载时，添加文件到队列
static void add_file_to_archive_block(struct archive_block *node, struct archive_file *file)
{
	struct file_item *work_ptr, *prev_ptr = NULL, *new_ptr;
	ar_uint64 end_pos = file->entry->offset + file->entry->comp_size;

	for (work_ptr = node->file_list; work_ptr; prev_ptr = work_ptr, work_ptr = work_ptr->next)
		if(end_pos < work_ptr->entry->offset)
			break;

	new_ptr = (struct file_item *)malloc(sizeof(struct file_item));
	new_ptr->entry = file->entry;
	new_ptr->next = work_ptr;

	if(prev_ptr)
		prev_ptr->next = new_ptr;
	else
		node->file_list = new_ptr;
}

//////////////////////////////////////////////////////////////////////////
// HTTP信息解析
static int process_line(struct archive_connect *conn, char *line)
{
	switch (conn->parse_state)
	{
	case ParsingStatusLine:
		{
			char *ptr;
			ptr = strchr(line, ' ');
			if(ptr == NULL)
				return 1;

			ptr = strchr(ptr + 1, ' ');
			if(ptr == NULL)
				return 1;

			sscanf_s(line, "HTTP/%f %d ", &conn->http_version, &conn->response_status);
			strcpy_s(conn->status_text, 64, ptr + 1);
			conn->parse_state = ParsingHeader;
		}
		break;
	case ParsingHeader:
		{
			if(!_stricmp(line, "transfer-encoding: chunked"))
				conn->chunked_encoding = 1;
			else if(!strncmp(line, "Content-Length",14))
				sscanf_s(line,"Content-Length: %d",&conn->content_length);
			else if(!strncmp(line, "Content-Type",12))
				strcpy_s(conn->content_type, 32, line + 14);

			if(line[0] == 0)
			{
				if(conn->chunked_encoding)
					conn->parse_state = ParsingChunkHeader;
				else
					conn->parse_state = ProcessingBody;
			}
		}
		break;
	case ParsingChunkHeader:
		break;
	}

	return (conn->parse_state == ProcessingBody);
}

static int parse_line(struct archive_connect *conn, char *buffer)
{
	size_t pos;
	char *line = buffer;

	while(line[0] != 0)
	{
		pos = strcspn(line, "\r\n");
		line[pos] = 0;
		process_line(conn, line);
		line = line + pos + 2;

		if(conn->parse_state == ProcessingBody)
			break;
	}

	return (line - buffer);
}

//////////////////////////////////////////////////////////////////////////
// HTTP　IO

// 检查SOCKET 是否连接完成
static int http_wait_writable(SOCKET s, int time_out)
{
	struct fd_set writefds;
	struct timeval time;

	FD_ZERO(&writefds);
	FD_SET(s, &writefds);

	time.tv_sec = time_out / 1000;
	time.tv_usec = (time_out % 1000) * 1000;

	if( select(s + 1, NULL, &writefds, NULL, &time) > 0)
		return 1;

	return 0;
}

static int http_send_request(SOCKET s, const char *host, const char *resource, ar_uint64 start_pos, ar_uint64 end_pos)
{
	char buffer[1024];
	sprintf_s(buffer, sizeof(buffer), "GET /%s HTTP/1.1\r\nHost:%s\r\nRange:bytes=%lld-%lld\r\n\r\n", resource, host, start_pos, end_pos);
	
	if(send(s, buffer, strlen(buffer), 0) == SOCKET_ERROR)
		return 0;

	return 1;
}

// 接收HTTP数据，自动解析HTTP信息
static int http_recv(struct archive_connect *conn, char *buffer, int buff_len, ar_uint *recv_size)
{
	int processed_size;
	int return_size = recv(conn->sock, buffer, buff_len, 0);
	if(return_size == SOCKET_ERROR)
	{
		int error = WSAGetLastError();
		if(error == WSAEWOULDBLOCK)
		{
			*recv_size = 0;
			return 1;
		}

		return 0;
	}

	if(conn->parse_state != ProcessingBody)
	{
		processed_size = parse_line(conn, buffer);
		if(processed_size < return_size)
		{
			return_size -= processed_size;
			memmove_s(buffer, buff_len, buffer + processed_size, return_size);
		}
	}

	*recv_size = return_size;
	return 1;
}

//////////////////////////////////////////////////////////////////////////
//
static int network_init()
{
	if(init_sock++ == 0)
	{
		WSADATA wsaData;
		if(WSAStartup(WINSOCK_VERSION, &wsaData) != 0)
			return 0;
	}

	return 1;
}

static void network_destory()
{
	if(--init_sock == 0)
		WSACleanup();
}

//---------------------------------------------------------------------------------------------
// 下载单个文件
int archive_download_file( struct archive_connect *conn, struct archive_file *file, const char *url )
{
	ar_uint crc32 = CRC_INIT_VAL;
	ar_uint file_crc32;

	char buffer[4096];
	ar_uint buff_pos;

	ar_uint64 start_pos;
	ar_uint64 end_pos;
	int raw_size;
	int result_success = 1;

	if(conn == NULL || conn->sock == INVALID_SOCKET)
		return 0;

	while(!http_wait_writable(conn->sock, 0))
		if(conn->download_interrupt) return 0;

	conn->parse_state = ParsingStatusLine;
	conn->chunked_encoding = 0;
	raw_size = file->entry->comp_size;
	start_pos = file->entry->offset;
	end_pos = start_pos + raw_size - 1;

	if(!http_send_request(conn->sock, conn->host_name, url, start_pos, end_pos))
		return 0;
	
	while(raw_size > 0 && !conn->download_interrupt)
	{
		ar_uint recv_size = get_min(sizeof(buffer), raw_size);
		if(!http_recv(conn, buffer, sizeof(buffer), &recv_size))
			break;

		if(recv_size > 0)
		{
			if(write_data(file->arc, start_pos, buffer, recv_size) != recv_size)
			{
				result_success = 0;
				break;
			}

			start_pos += recv_size;
			raw_size -= recv_size;

			buff_pos = recv_size;
			if(raw_size <= 0 && file->entry->flags & JFILE_FLAG_COMRESSED)
				buff_pos = recv_size - JFILE_RAW_VERIFY_SIZE;

			crc32 = crc_append(crc32, buffer, buff_pos);
		}
	}

	if(result_success)
	{
		if(file->entry->flags & JFILE_FLAG_COMRESSED)
			file_crc32 = *(ar_uint*)(buffer + buff_pos);
		else
			file_crc32 = file->entry->crc32;

		crc32 = CRC_GET_DIGEST(crc32);
		if(file_crc32 == crc32)
		{
			file->entry->flags &= ~JFILE_FLAG_INVAILD_DATA;
			save_file_flag(file->arc, file->entry);
		}
		else
		{
			result_success = 0;
		}
	}

	return result_success;
}

// 把文件加入下载队列
void archive_download_add_file( struct archive_connect *conn, struct archive_file* file, const char *resource )
{
	struct archive_block *work_ptr, *prev_ptr = NULL;

	for (work_ptr = conn->download_list; work_ptr; prev_ptr = work_ptr, work_ptr = work_ptr->next)
		if(work_ptr->arc == file->arc)
			break;

	if(work_ptr == NULL)
	{
		work_ptr = (struct archive_block *)malloc(sizeof(struct archive_block));
		work_ptr->arc = file->arc;
		work_ptr->file_list = NULL;
		work_ptr->next = NULL;
		strcpy_s(work_ptr->url, 256, resource);

		if(prev_ptr)
			prev_ptr->next = work_ptr;
		else
			conn->download_list = work_ptr;
	}
	
	add_file_to_archive_block(work_ptr, file);
}

//连接服务器
struct archive_connect *archive_download_connect( const char *host, unsigned short port )
{
	struct archive_connect *conn;
	struct sockaddr_in addrto;
	u_long notblock = 1;

	if(!network_init())
		return 0;
	
	conn = (struct archive_connect*)malloc(sizeof(struct archive_connect));
	if(conn == 0)
		return 0;

	memset(conn, 0, sizeof(struct archive_connect));
	conn->sock = INVALID_SOCKET;

	conn->sock = socket(AF_INET, SOCK_STREAM, 0);
	if(conn->sock == INVALID_SOCKET)
		goto error_out;

	//设置不阻塞
	if(ioctlsocket(conn->sock, FIONBIO, &notblock) != 0)
		goto error_out;

	memset(&addrto, 0, sizeof(addrto));
	addrto.sin_addr.s_addr = inet_addr(host);
	if(addrto.sin_addr.s_addr == INADDR_NONE)
		goto error_out;

	addrto.sin_family = AF_INET;
	addrto.sin_port = htons(port);

	if(connect(conn->sock, (struct sockaddr*)&addrto, sizeof(addrto)) == SOCKET_ERROR)
	{
		if(WSAGetLastError() != WSAEWOULDBLOCK)
			goto error_out;
	}

	strcpy_s(conn->host_name, 64, host);
	return conn;

error_out:
	if(conn->sock != INVALID_SOCKET)
		closesocket(conn->sock);
	free(conn);
	network_destory();
	return 0;
}

//开始批量下载， 下载结束释放队列
int archive_download_batch(struct archive_connect *conn, ar_uint *download_size)
{
	struct archive_block *work_arc, *temp_arc;
	struct file_item *work_file = NULL, *start_file;
	ar_uint64 start_pos = 0;
	ar_uint64 end_pos = 0;

	char buffer[4096];
	char verify_data[JFILE_RAW_VERIFY_SIZE];
	ar_uint verify_len;

	ar_uint crc32;
	int raw_size;
	int result_success = 1;

	if(conn == NULL || conn->sock == INVALID_SOCKET)
		return 0;

	// 等待连接成功
	while(!http_wait_writable(conn->sock, 0))
		if(conn->download_interrupt) return 0;
	
	for (work_arc = conn->download_list; work_arc && !conn->download_interrupt && result_success; work_arc = work_arc->next)
	{
		while (!conn->download_interrupt && result_success)
		{
			// 查找连续的数据块
			start_file = work_file ? work_file : work_arc->file_list;
			start_pos = start_file->entry->offset;
			end_pos = start_pos + start_file->entry->comp_size - 1;

			for (work_file = start_file->next; work_file && (work_file->entry->offset == end_pos+1); work_file = work_file->next)
				end_pos = work_file->entry->offset + work_file->entry->comp_size - 1;

			//将连续数据块 一起 请求下载
			if(!http_send_request(conn->sock, conn->host_name, work_arc->url, start_pos, end_pos))
			{
				result_success = 0;
				break;
			}
			
			//开始下载数据
			conn->parse_state = ParsingStatusLine;
			conn->chunked_encoding = 0;
			verify_len = 0;
			crc32 = CRC_INIT_VAL;
			raw_size = get_raw_size(start_file->entry);
			
			// 当前下载文件 还没到 下一段连续数据块
			while(start_file != work_file && !conn->download_interrupt && result_success)
			{
				ar_uint recv_size;
				ar_uint buff_pos = 0;
				int result;
				
				if(!http_recv(conn, buffer, sizeof(buffer), &recv_size))
				{
					result_success = 0;
					break;
				}

				if(recv_size == 0)
					continue;
				
				result = write_data(work_arc->arc, start_pos, buffer, recv_size);
				if(result != recv_size)
				{
					result_success = 0;
					break;
				}

				start_pos += recv_size;
				if(download_size)
					*download_size += recv_size;
				
				while(buff_pos < recv_size)
				{
					ar_uint file_crc32;
					int rem_size;
					
					if(raw_size > 0)
					{
						rem_size = get_min(recv_size - buff_pos, raw_size);
						crc32 = crc_append(crc32, buffer + buff_pos, rem_size);
						raw_size -= rem_size;
						buff_pos += rem_size;
					}

					// 如果文件数据验证信息计算完，再继续对比
					if(raw_size > 0)
						break;

					// 获取文件CRC,有压缩的话，压缩数据的CRC在文件尾
					if(start_file->entry->flags & JFILE_FLAG_COMRESSED)
					{
						//如果上一次接收的数据，没有把验证数据接收完，就继续接收
						if(verify_len != JFILE_RAW_VERIFY_SIZE)
						{
							ar_uint cpy_size = get_min(JFILE_RAW_VERIFY_SIZE - verify_len, recv_size - buff_pos);
							memcpy(verify_data + verify_len, buffer + buff_pos, cpy_size);
							buff_pos += cpy_size;
							verify_len += cpy_size;
						}

						if(verify_len < JFILE_RAW_VERIFY_SIZE)
							break;

						file_crc32 = *(ar_uint*)verify_data;
						verify_len = 0;
					}
					else
						file_crc32 = start_file->entry->crc32;

					//对比CRC
					crc32 = CRC_GET_DIGEST(crc32);
					if(file_crc32 == crc32)
					{
						ar_uint64 save_pos = _ftelli64(work_arc->arc->stream);
						start_file->entry->flags &= ~JFILE_FLAG_INVAILD_DATA;
						save_file_flag(work_arc->arc, start_file->entry);
						_fseeki64(work_arc->arc->stream, save_pos, SEEK_SET);
					}

					start_file = start_file->next;
					if(start_file != NULL)
					{
						crc32 = CRC_INIT_VAL;
						raw_size = get_raw_size(start_file->entry);
					}
				}
			}

			if(work_file == NULL)
				break;
		}
	}
	
	//当文件数据填充完，验证文件完整性
	for (work_arc = conn->download_list; work_arc;)
	{
		for (work_file = work_arc->file_list; work_file;)
		{
			start_file = work_file;
			work_file = work_file->next;
			free(start_file);
		}
		temp_arc = work_arc;
		work_arc = work_arc->next;

		archive_close(temp_arc->arc);
		free(temp_arc);
	}

	conn->download_list = NULL;
	return result_success;
}

//结束下载
void archive_download_close(struct archive_connect *conn)
{
	if(conn)
	{
		struct archive_block *work_arc, *temp_arc;
		struct file_item *work_file = NULL, *start_file;

		for (work_arc = conn->download_list; work_arc;)
		{
			for (work_file = work_arc->file_list; work_file;)
			{
				start_file = work_file;
				work_file = work_file->next;
				free(start_file);
			}

			temp_arc = work_arc;
			work_arc = work_arc->next;

			archive_close(temp_arc->arc);
			free(temp_arc);
		}

		closesocket(conn->sock);
		free(conn);
	}

	network_destory();
}

void archive_download_interrupt(struct archive_connect *conn)
{
	if(conn)
		conn->download_interrupt = 1;
}