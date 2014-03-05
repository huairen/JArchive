/* 7zFile.c -- File IO
2009-11-24 : Igor Pavlov : Public domain */

#include "7zFile.h"
#include "7zCrc.h"
#include <stdio.h>

#ifndef USE_WINDOWS_FILE

#ifndef UNDER_CE
#include <errno.h>
#endif

#else

/*
   ReadFile and WriteFile functions in Windows have BUG:
   If you Read or Write 64MB or more (probably min_failure_size = 64MB - 32KB + 1)
   from/to Network file, it returns ERROR_NO_SYSTEM_RESOURCES
   (Insufficient system resources exist to complete the requested service).
   Probably in some version of Windows there are problems with other sizes:
   for 32 MB (maybe also for 16 MB).
   And message can be "Network connection was lost"
*/

#define kChunkSizeMax (1 << 22)

#endif

void File_Construct(CSzFile *p)
{
  #ifdef USE_WINDOWS_FILE
  p->handle = INVALID_HANDLE_VALUE;
  #else
  p->file = NULL;
  #endif
}

#if !defined(UNDER_CE) || !defined(USE_WINDOWS_FILE)
static WRes File_Open(CSzFile *p, const char *name, int writeMode)
{
  #ifdef USE_WINDOWS_FILE
  p->handle = CreateFileA(name,
      writeMode ? GENERIC_WRITE : GENERIC_READ,
      FILE_SHARE_READ, NULL,
      writeMode ? CREATE_ALWAYS : OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL, NULL);
  return (p->handle != INVALID_HANDLE_VALUE) ? 0 : GetLastError();
  #else
  p->file = fopen(name, writeMode ? "wb+" : "rb");
  return (p->file != 0) ? 0 :
    #ifdef UNDER_CE
    2; /* ENOENT */
    #else
    errno;
    #endif
  #endif
}

WRes InFile_Open(CSzFile *p, const char *name) { return File_Open(p, name, 0); }
WRes OutFile_Open(CSzFile *p, const char *name) { return File_Open(p, name, 1); }
#endif

#ifdef USE_WINDOWS_FILE
static WRes File_OpenW(CSzFile *p, const WCHAR *name, int writeMode)
{
  p->handle = CreateFileW(name,
      writeMode ? GENERIC_WRITE : GENERIC_READ,
      FILE_SHARE_READ, NULL,
      writeMode ? CREATE_ALWAYS : OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL, NULL);
  return (p->handle != INVALID_HANDLE_VALUE) ? 0 : GetLastError();
}
WRes InFile_OpenW(CSzFile *p, const WCHAR *name) { return File_OpenW(p, name, 0); }
WRes OutFile_OpenW(CSzFile *p, const WCHAR *name) { return File_OpenW(p, name, 1); }
#endif

WRes File_Close(CSzFile *p)
{
  #ifdef USE_WINDOWS_FILE
  if (p->handle != INVALID_HANDLE_VALUE)
  {
    if (!CloseHandle(p->handle))
      return GetLastError();
    p->handle = INVALID_HANDLE_VALUE;
  }
  #else
  if (p->file != NULL)
  {
    int res = fclose(p->file);
    if (res != 0)
      return res;
    p->file = NULL;
  }
  #endif
  return 0;
}

WRes File_Read(CSzFile *p, void *data, size_t *size)
{
  size_t originalSize = *size;
  if (originalSize == 0)
    return 0;

  #ifdef USE_WINDOWS_FILE

  *size = 0;
  do
  {
    DWORD curSize = (originalSize > kChunkSizeMax) ? kChunkSizeMax : (DWORD)originalSize;
    DWORD processed = 0;
    BOOL res = ReadFile(p->handle, data, curSize, &processed, NULL);
    data = (void *)((Byte *)data + processed);
    originalSize -= processed;
    *size += processed;
    if (!res)
      return GetLastError();
    if (processed == 0)
      break;
  }
  while (originalSize > 0);
  return 0;

  #else
  
  *size = fread(data, 1, originalSize, p->file);
  if (*size == originalSize)
    return 0;
  return ferror(p->file);
  
  #endif
}

WRes File_Write(CSzFile *p, const void *data, size_t *size)
{
  size_t originalSize = *size;
  if (originalSize == 0)
    return 0;
  
  #ifdef USE_WINDOWS_FILE

  *size = 0;
  do
  {
    DWORD curSize = (originalSize > kChunkSizeMax) ? kChunkSizeMax : (DWORD)originalSize;
    DWORD processed = 0;
    BOOL res = WriteFile(p->handle, data, curSize, &processed, NULL);
    data = (void *)((Byte *)data + processed);
    originalSize -= processed;
    *size += processed;
    if (!res)
      return GetLastError();
    if (processed == 0)
      break;
  }
  while (originalSize > 0);
  return 0;

  #else

  *size = fwrite(data, 1, originalSize, p->file);
  if (*size == originalSize)
    return 0;
  return ferror(p->file);
  
  #endif
}

WRes File_Seek(CSzFile *p, Int64 *pos, ESzSeek origin)
{
  #ifdef USE_WINDOWS_FILE

  LARGE_INTEGER value;
  DWORD moveMethod;
  value.LowPart = (DWORD)*pos;
  value.HighPart = (LONG)((UInt64)*pos >> 16 >> 16); /* for case when UInt64 is 32-bit only */
  switch (origin)
  {
    case SZ_SEEK_SET: moveMethod = FILE_BEGIN; break;
    case SZ_SEEK_CUR: moveMethod = FILE_CURRENT; break;
    case SZ_SEEK_END: moveMethod = FILE_END; break;
    default: return ERROR_INVALID_PARAMETER;
  }
  value.LowPart = SetFilePointer(p->handle, value.LowPart, &value.HighPart, moveMethod);
  if (value.LowPart == 0xFFFFFFFF)
  {
    WRes res = GetLastError();
    if (res != NO_ERROR)
      return res;
  }
  *pos = ((Int64)value.HighPart << 32) | value.LowPart;
  return 0;

  #else
  
  int moveMethod;
  int res;
  switch (origin)
  {
    case SZ_SEEK_SET: moveMethod = SEEK_SET; break;
    case SZ_SEEK_CUR: moveMethod = SEEK_CUR; break;
    case SZ_SEEK_END: moveMethod = SEEK_END; break;
    default: return 1;
  }
  res = fseek(p->file, (long)*pos, moveMethod);
  *pos = ftell(p->file);
  return res;
  
  #endif
}

WRes File_GetLength(CSzFile *p, UInt64 *length)
{
  #ifdef USE_WINDOWS_FILE
  
  DWORD sizeHigh;
  DWORD sizeLow = GetFileSize(p->handle, &sizeHigh);
  if (sizeLow == 0xFFFFFFFF)
  {
    DWORD res = GetLastError();
    if (res != NO_ERROR)
      return res;
  }
  *length = (((UInt64)sizeHigh) << 32) + sizeLow;
  return 0;
  
  #else
  
  long pos = ftell(p->file);
  int res = fseek(p->file, 0, SEEK_END);
  *length = ftell(p->file);
  fseek(p->file, pos, SEEK_SET);
  return res;
  
  #endif
}


/* ---------- FileSeqInStream ---------- */

static SRes FileSeqInStream_Read(void *pp, void *buf, size_t *size)
{
  CFileSeqInStream *p = (CFileSeqInStream *)pp;
  return File_Read(&p->file, buf, size) == 0 ? SZ_OK : SZ_ERROR_READ;
}

void FileSeqInStream_CreateVTable(CFileSeqInStream *p)
{
  p->s.Read = FileSeqInStream_Read;
}


/* ---------- FileInStream ---------- */

static SRes FileInStream_Read(void *pp, void *buf, size_t *size)
{
  CFileInStream *p = (CFileInStream *)pp;
  return (File_Read(&p->file, buf, size) == 0) ? SZ_OK : SZ_ERROR_READ;
}

static SRes FileInStream_Seek(void *pp, Int64 *pos, ESzSeek origin)
{
  CFileInStream *p = (CFileInStream *)pp;
  return File_Seek(&p->file, pos, origin);
}

void FileInStream_CreateVTable(CFileInStream *p)
{
  p->s.Read = FileInStream_Read;
  p->s.Seek = FileInStream_Seek;
}


/* ---------- FileOutStream ---------- */

static size_t FileOutStream_Write(void *pp, const void *data, size_t size)
{
  CFileOutStream *p = (CFileOutStream *)pp;
  File_Write(&p->file, data, &size);
  return size;
}

void FileOutStream_CreateVTable(CFileOutStream *p)
{
  p->s.Write = FileOutStream_Write;
}

/* ---------- FolderOutStream ---------- */

static size_t FolderOutStream_Write2(CFolderOutStream *p, const void *data, size_t size)
{
	SizeT processedSize = 0;
	UInt16 *fullPath = NULL;
	SizeT pathLen = 0;
	SizeT curSize = 0;
	SRes res = SZ_OK;
	UInt32 i;
	SizeT dirLen = 0;
	
	if(p->outputDir)
		dirLen = wcslen(p->outputDir);

	while(size != 0)
	{
		if(p->file.handle == INVALID_HANDLE_VALUE)
		{
			UInt16 *filename;
			UInt32 index  = p->startIndex + p->currentIndex;
			const CSzFileItem *fileItem = p->ar->db.Files + index;
			SizeT len = SzArEx_GetFileNameUtf16(p->ar, index, NULL);
			if(len + dirLen > pathLen)
			{
				free(fullPath);
				pathLen = len + dirLen;
				fullPath = (UInt16*)malloc(pathLen * sizeof(fullPath[0]));
				if(fullPath == 0)
				{
					res = SZ_ERROR_MEM;
					break;
				}

				if(p->outputDir)
					wcscpy_s(fullPath,pathLen,p->outputDir);
			}

			filename = fullPath + dirLen;
			SzArEx_GetFileNameUtf16(p->ar, index, filename);
			for (i = 0; fullPath[i] != 0 && i<pathLen; i++)
				if (fullPath[i] == '/')
				{
					fullPath[i] = 0;
					CreateDirectoryW(fullPath,NULL);
					fullPath[i] = CHAR_PATH_SEPARATOR;
				}
				
			if (fileItem->IsDir)
			{
				CreateDirectoryW(fullPath,NULL);
				continue;
			}
			else if(OutFile_OpenW(&p->file, fullPath))
			{
				res = SZ_ERROR_FAIL;
				break;
			}

#ifdef _SZ_ALLOC_DEBUG
			wprintf(L"\nExtract: %s", fullPath);
#endif
			p->fileSize = fileItem->Size;
			p->checkCrc = fileItem->CrcDefined;
			p->crc = CRC_INIT_VAL;
		}

		curSize = size < p->fileSize ? size : (SizeT)p->fileSize;
		if(S_OK != File_Write(&p->file, data, &curSize))
		{
			File_Close(&p->file);
			res = SZ_ERROR_FAIL;
			break;
		}

		if(p->checkCrc)
			p->crc = CrcUpdate(p->crc,data,curSize);

		data = (const Byte*)data + curSize;
		size -= curSize;
		p->fileSize -= curSize;
		p->folderSize -= curSize;
		processedSize += curSize;

		if(p->fileSize == 0)
		{
			UInt32 index  = p->startIndex + p->currentIndex;
			const CSzFileItem *fileItem = p->ar->db.Files + index;

			p->currentIndex += 1;

			if(fileItem->MTimeDefined)
				SetFileTime(p->file.handle,NULL,NULL,(const FILETIME*)&fileItem->MTime);

			File_Close(&p->file);

			if(fileItem->AttribDefined)
				SetFileAttributesW(fullPath, fileItem->Attrib);

			if(fileItem->CrcDefined && CRC_GET_DIGEST(p->crc) != fileItem->Crc)
			{
				res = SZ_ERROR_CRC;
				break;
			}
		}
	}

	free(fullPath);
	return processedSize;
}

static size_t FolderOutStream_Write(void *pp, const void *data, size_t size)
{
	CFolderOutStream *p = (CFolderOutStream *)pp;
	UInt32 curSize = FOLDEROUTSTREAM_BUF_SIZE - p->bufPos;
	UInt32 processSize = (size < curSize) ? size : curSize;

	if(size > FOLDEROUTSTREAM_BUF_SIZE)
	{
		if(p->bufPos > 0)
			FolderOutStream_Write2(p, p->buf, p->bufPos);
		return FolderOutStream_Write2(p,data,size);
	}

	if(processSize > 0)
	{
		memcpy(p->buf + p->bufPos, data, processSize);
		data = (Byte*)data + processSize;
		p->bufPos += processSize;
		curSize -= processSize;
	}

	if(curSize == 0 || p->bufPos >= p->folderSize)
	{
		FolderOutStream_Write2(p,p->buf,p->bufPos);
		p->bufPos = 0;
	}

	if(size > processSize)
	{
		processSize = size - processSize;
		memcpy(p->buf + p->bufPos, data, processSize);
		p->bufPos += processSize;
	}

	return size;
}

void FolderOutStream_Init(CFolderOutStream *p, const CSzArEx* ar, UInt32 folderIndex, const UInt16* outputDir)
{
	p->s.Write = FolderOutStream_Write;
	p->ar = ar;
	p->startIndex = ar->FolderStartFileIndex[folderIndex];
	p->currentIndex = 0;
	p->folderSize = SzFolder_GetUnpackSize(ar->db.Folders + folderIndex);
	p->fileSize = 0;
	p->crc = CRC_INIT_VAL;
	p->checkCrc = False;
	p->outputDir = outputDir;
	p->bufPos = 0;
	File_Construct(&p->file);
}