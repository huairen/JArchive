/* 7zStream.c -- 7z Stream functions
2010-03-11 : Igor Pavlov : Public domain */

#include <string.h>

#include "Types.h"

SRes SeqInStream_Read2(ISeqInStream *stream, void *buf, size_t size, SRes errorType)
{
  while (size != 0)
  {
    size_t processed = size;
    RINOK(stream->Read(stream, buf, &processed));
    if (processed == 0)
      return errorType;
    buf = (void *)((Byte *)buf + processed);
    size -= processed;
  }
  return SZ_OK;
}

SRes SeqInStream_Read(ISeqInStream *stream, void *buf, size_t size)
{
  return SeqInStream_Read2(stream, buf, size, SZ_ERROR_INPUT_EOF);
}

SRes SeqInStream_ReadByte(ISeqInStream *stream, Byte *buf)
{
  size_t processed = 1;
  RINOK(stream->Read(stream, buf, &processed));
  return (processed == 1) ? SZ_OK : SZ_ERROR_INPUT_EOF;
}

SRes SeqOutStream_Write2(ISeqOutStream *stream, void *buf, size_t size, SRes errorType)
{
	while (size != 0)
	{
		size_t processed = stream->Write(stream, buf, size);
		if (processed == 0)
			return errorType;
		buf = (void *)((Byte *)buf + processed);
		size -= processed;
	}
	return SZ_OK;
}

SRes SeqOutStream_Write(ISeqOutStream *stream, void *buf, size_t size)
{
	return SeqOutStream_Write2(stream, buf, size, SZ_ERROR_OUTPUT_EOF);
}

SRes SeqOutStream_WriteByte(ISeqOutStream *stream, Byte *buf)
{
	size_t processed = stream->Write(stream, buf, 1);
	return (processed == 1) ? SZ_OK : SZ_ERROR_INPUT_EOF;
}

size_t MemOutStream_Write(void *p, const void *buf, size_t size)
{
	CMemOutStream *pp = (CMemOutStream *)p;
	SizeT writeSize = size;
	errno_t re;

	if(writeSize > pp->size)
		writeSize = pp->size;

	re = memcpy_s(pp->data, pp->size, buf, writeSize);
	return re == 0 ? writeSize : 0;
}

void MemOutStream_Init( CMemOutStream *p )
{
	p->s.Write = MemOutStream_Write;
	p->data = 0;
	p->size = 0;
}

SRes LookInStream_SeekTo(ILookInStream *stream, UInt64 offset)
{
  Int64 t = offset;
  return stream->Seek(stream, &t, SZ_SEEK_SET);
}

SRes LookInStream_LookRead(ILookInStream *stream, void *buf, size_t *size)
{
  const void *lookBuf;
  if (*size == 0)
    return SZ_OK;
  RINOK(stream->Look(stream, &lookBuf, size));
  memcpy(buf, lookBuf, *size);
  return stream->Skip(stream, *size);
}

SRes LookInStream_LookReadByte(ILookInStream *stream, void *buf)
{
	size_t processed = 1;
	RINOK(LookInStream_LookRead(stream, buf, &processed));
	return (processed == 1) ? SZ_OK : SZ_ERROR_INPUT_EOF;
}

SRes LookInStream_Read2(ILookInStream *stream, void *buf, size_t size, SRes errorType)
{
  while (size != 0)
  {
    size_t processed = size;
    RINOK(stream->Read(stream, buf, &processed));
    if (processed == 0)
      return errorType;
    buf = (void *)((Byte *)buf + processed);
    size -= processed;
  }
  return SZ_OK;
}

SRes LookInStream_Read(ILookInStream *stream, void *buf, size_t size)
{
  return LookInStream_Read2(stream, buf, size, SZ_ERROR_INPUT_EOF);
}

SRes LookInStream_ReadByte(ILookInStream *stream, Byte *buf)
{
	size_t processed = 1;
	RINOK(stream->Read(stream, buf, &processed));
	return (processed == 1) ? SZ_OK : SZ_ERROR_INPUT_EOF;
}

static SRes LookToRead_Look_Lookahead(void *pp, const void **buf, size_t *size)
{
  SRes res = SZ_OK;
  CLookToRead *p = (CLookToRead *)pp;
  size_t size2 = p->size - p->pos;
  if (size2 == 0 && *size > 0)
  {
    p->pos = 0;
    size2 = LookToRead_BUF_SIZE;
    res = p->realStream->Read(p->realStream, p->buf, &size2);
    p->size = size2;
  }
  if (size2 < *size)
    *size = size2;
  *buf = p->buf + p->pos;
  return res;
}

static SRes LookToRead_Look_Exact(void *pp, const void **buf, size_t *size)
{
  SRes res = SZ_OK;
  CLookToRead *p = (CLookToRead *)pp;
  size_t size2 = p->size - p->pos;
  if (size2 == 0 && *size > 0)
  {
    p->pos = 0;
    if (*size > LookToRead_BUF_SIZE)
      *size = LookToRead_BUF_SIZE;
    res = p->realStream->Read(p->realStream, p->buf, size);
    size2 = p->size = *size;
  }
  if (size2 < *size)
    *size = size2;
  *buf = p->buf + p->pos;
  return res;
}

static SRes LookToRead_Skip(void *pp, size_t offset)
{
  CLookToRead *p = (CLookToRead *)pp;
  p->pos += offset;
  return SZ_OK;
}

static SRes LookToRead_Read(void *pp, void *buf, size_t *size)
{
  CLookToRead *p = (CLookToRead *)pp;
  size_t rem = p->size - p->pos;
  if (rem == 0)
    return p->realStream->Read(p->realStream, buf, size);
  if (rem > *size)
    rem = *size;
  memcpy(buf, p->buf + p->pos, rem);
  p->pos += rem;
  *size = rem;
  return SZ_OK;
}

static SRes LookToRead_Seek(void *pp, Int64 *pos, ESzSeek origin)
{
  CLookToRead *p = (CLookToRead *)pp;
  p->pos = p->size = 0;
  return p->realStream->Seek(p->realStream, pos, origin);
}

void LookToRead_CreateVTable(CLookToRead *p, int lookahead)
{
  p->s.Look = lookahead ?
      LookToRead_Look_Lookahead :
      LookToRead_Look_Exact;
  p->s.Skip = LookToRead_Skip;
  p->s.Read = LookToRead_Read;
  p->s.Seek = LookToRead_Seek;
}

void LookToRead_Init(CLookToRead *p)
{
  p->pos = p->size = 0;
}

static SRes BufferStream_Look_Lookahead(void *pp, const void **buf, size_t *size)
{
	CBufferStream* buffer = (CBufferStream*)pp;
	CLookToRead* look = (CLookToRead*)pp;
	UInt64 rem = buffer->size - buffer->offset;

	if(look->size == look->pos)
		RINOK(LookInStream_SeekTo(pp, buffer->startPos + buffer->offset));

	if(*size > rem)
		*size = (size_t)rem;
	RINOK(LookToRead_Look_Lookahead(pp, buf, size));

	return SZ_OK;
}

static SRes BufferStream_Look_Exact(void *pp, const void **buf, size_t* size)
{
	CBufferStream* buffer = (CBufferStream*)pp;
	CLookToRead* look = (CLookToRead*)pp;
	UInt64 rem = buffer->size - buffer->offset;

	if(look->size == look->pos)
		RINOK(LookInStream_SeekTo(pp, buffer->startPos + buffer->offset));

	if(*size > rem)
		*size = (size_t)rem;
	RINOK(LookToRead_Look_Exact(pp, buf, size));

	return SZ_OK;
}

static SRes BufferStream_Skip(void *pp, size_t offset)
{
	CBufferStream* buffer = (CBufferStream*)pp;
	CLookToRead* look = (CLookToRead*)pp;
	look->pos += offset;
	buffer->offset += offset;
	return SZ_OK;
}

static SRes BufferStream_Read(void *pp, void *buf, size_t* size)
{
	CBufferStream* buffer = (CBufferStream*)pp;
	CLookToRead* look = (CLookToRead*)pp;
	UInt64 rem = buffer->size - buffer->offset;

	if(look->size == look->pos)
		RINOK(LookInStream_SeekTo(pp, buffer->startPos + buffer->offset));

	if(*size > rem)
		*size = (size_t)rem;
	RINOK(LookToRead_Read(pp, buf, size));

	return SZ_OK;

// 	CBufferStream* buffer = (CBufferStream*)pp;
// 	size_t originalSize = *size;
// 	if (originalSize == 0)
// 		return 0;
// 
// 	*size = 0;
// 	do
// 	{
// 		size_t processed = originalSize;
// 		LookInStream_LookRead(pp,buf,&processed);
// 		if (processed == 0)
// 			break;
// 
// 		buf = (void *)((Byte *)buf + processed);
// 		originalSize -= processed;
// 		*size += processed;
// 		buffer->offset += processed;
// 	}
// 	while (originalSize > 0);
// 	return SZ_OK;
}

void BufferStream_CreateVTable(CBufferStream *p, int lookahead)
{
	p->s.s.Look = lookahead ? BufferStream_Look_Lookahead : BufferStream_Look_Exact;
	p->s.s.Skip = BufferStream_Skip;
	p->s.s.Read = BufferStream_Read;
	p->s.s.Seek = LookToRead_Seek;
}

static SRes SecToLook_Read(void *pp, void *buf, size_t *size)
{
  CSecToLook *p = (CSecToLook *)pp;
  return LookInStream_LookRead(p->realStream, buf, size);
}

void SecToLook_CreateVTable(CSecToLook *p)
{
  p->s.Read = SecToLook_Read;
}

static SRes SecToRead_Read(void *pp, void *buf, size_t *size)
{
  CSecToRead *p = (CSecToRead *)pp;
  return p->realStream->Read(p->realStream, buf, size);
}

void SecToRead_CreateVTable(CSecToRead *p)
{
  p->s.Read = SecToRead_Read;
}

SRes CompressCallback_Progress(void *p, UInt64 inSize, UInt64 outSize)
{
	CCompressCallback* pp = (CCompressCallback*)p;
	pp->inProcessedSize += inSize;
	pp->outProcessedSize += outSize;
	return SZ_OK;
}

Bool CompressCallback_IsStop(void *p)
{
	CCompressCallback* pp = (CCompressCallback*)p;
	return pp->closing;
}

void CompressCallback_Init( CCompressCallback* p, UInt64 unpackSize )
{
	p->progress.Progress = CompressCallback_Progress;
	p->progress.IsStop = CompressCallback_IsStop;
	p->unpackSize = unpackSize;
	p->packSize = 0;
	p->inProcessedSize = 0;
	p->outProcessedSize = 0;
	p->closing = False;
}