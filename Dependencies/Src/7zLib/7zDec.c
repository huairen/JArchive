/* 7zDec.c -- Decoding from 7z folder
2010-11-02 : Igor Pavlov : Public domain */

#include <string.h>

/* #define _7ZIP_PPMD_SUPPPORT */

#include "7z.h"

#include "Bcj2.h"
#include "Bra.h"
#include "CpuArch.h"
#include "LzmaDec.h"
#include "Lzma2Dec.h"
#ifdef _7ZIP_PPMD_SUPPPORT
#include "Ppmd7.h"
#endif

#define k_Copy 0
#define k_LZMA2 0x21
#define k_LZMA  0x30101
#define k_BCJ   0x03030103
#define k_PPC   0x03030205
#define k_ARM   0x03030501
#define k_ARMT  0x03030701
#define k_SPARC 0x03030805
#define k_BCJ2  0x0303011B

#define  BUFF_SIZE (1<<17)

#ifdef _7ZIP_PPMD_SUPPPORT

#define k_PPMD 0x30401

typedef struct
{
	IByteIn p;
	const Byte *cur;
	const Byte *end;
	const Byte *begin;
	UInt64 processed;
	Bool extra;
	SRes res;
	ILookInStream *inStream;
} CByteInToLook;

static Byte ReadByte(void *pp)
{
	CByteInToLook *p = (CByteInToLook *)pp;
	if (p->cur != p->end)
		return *p->cur++;
	if (p->res == SZ_OK)
	{
		size_t size = p->cur - p->begin;
		p->processed += size;
		p->res = p->inStream->Skip(p->inStream, size);
		size = (1 << 25);
		p->res = p->inStream->Look(p->inStream, (const void **)&p->begin, &size);
		p->cur = p->begin;
		p->end = p->begin + size;
		if (size != 0)
			return *p->cur++;;
	}
	p->extra = True;
	return 0;
}

static SRes SzDecodePpmd(CSzCoderInfo *coder, UInt64 inSize, ILookInStream *inStream,
						 Byte *outBuffer, SizeT outSize, ISzAlloc *allocMain)
{
	CPpmd7 ppmd;
	CByteInToLook s;
	SRes res = SZ_OK;

	s.p.Read = ReadByte;
	s.inStream = inStream;
	s.begin = s.end = s.cur = NULL;
	s.extra = False;
	s.res = SZ_OK;
	s.processed = 0;

	if (coder->Props.size != 5)
		return SZ_ERROR_UNSUPPORTED;

	{
		unsigned order = coder->Props.data[0];
		UInt32 memSize = GetUi32(coder->Props.data + 1);
		if (order < PPMD7_MIN_ORDER ||
			order > PPMD7_MAX_ORDER ||
			memSize < PPMD7_MIN_MEM_SIZE ||
			memSize > PPMD7_MAX_MEM_SIZE)
			return SZ_ERROR_UNSUPPORTED;
		Ppmd7_Construct(&ppmd);
		if (!Ppmd7_Alloc(&ppmd, memSize, allocMain))
			return SZ_ERROR_MEM;
		Ppmd7_Init(&ppmd, order);
	}
	{
		CPpmd7z_RangeDec rc;
		Ppmd7z_RangeDec_CreateVTable(&rc);
		rc.Stream = &s.p;
		if (!Ppmd7z_RangeDec_Init(&rc))
			res = SZ_ERROR_DATA;
		else if (s.extra)
			res = (s.res != SZ_OK ? s.res : SZ_ERROR_DATA);
		else
		{
			SizeT i;
			for (i = 0; i < outSize; i++)
			{
				int sym = Ppmd7_DecodeSymbol(&ppmd, &rc.p);
				if (s.extra || sym < 0)
					break;
				outBuffer[i] = (Byte)sym;
			}
			if (i != outSize)
				res = (s.res != SZ_OK ? s.res : SZ_ERROR_DATA);
			else if (s.processed + (s.cur - s.begin) != inSize || !Ppmd7z_RangeDec_IsFinishedOK(&rc))
				res = SZ_ERROR_DATA;
		}
	}
	Ppmd7_Free(&ppmd, allocMain);
	return res;
}

#endif

//-----------------------------Coder---------------------------------


typedef struct 
{
	CSzCoderInfo *coder;

	UInt64 *packSizes;
	ILookInStream **inStreams;
	UInt32 numInStreams;

	UInt64 *unpackSize;
	ISeekInStream **outStreams;
	UInt32 numOutStreams;
} CCoder;

//-----------------------------Lzma-------------------------------------
typedef struct  
{
	ISeekInStream s;
	CCoder coder;
	CLzmaDec state;
} CLzmaCoder;

static SRes LzmaCoder_Read(void *pp, void *buf, size_t *size)
{
	CLzmaCoder* p = (CLzmaCoder*)pp;
	ILookInStream *inStream = p->coder.inStreams[0];
	UInt64 inSize = p->coder.packSizes[0];
	SRes res = SZ_OK;
	size_t originalSize = *size;
	if (originalSize == 0)
		return 0;

	*size = 0;
	do
	{
		Byte *inBuf = NULL;
		size_t inProcessed = (1 << 18);
		if (inProcessed > inSize)
			inProcessed = (size_t)inSize;
		res = inStream->Look(inStream, &inBuf, &inProcessed);
		if (res != SZ_OK || inProcessed == 0)
			break;

		{
			SizeT outProcessed = originalSize;
			ELzmaStatus status;
			res = LzmaDec_DecodeToBuf(&p->state, (Byte *)buf, &outProcessed, inBuf, &inProcessed, LZMA_FINISH_ANY, &status);

			buf = (void *)((Byte *)buf + outProcessed);
			originalSize -= outProcessed;
			*size += outProcessed;

			if (res != SZ_OK)
				break;

			res = inStream->Skip(inStream, inProcessed);
			if (res != SZ_OK)
				break;
		}
	}
	while (originalSize > 0);
	return res;
}

SRes LzmaCoder_Init(CLzmaCoder* p, CSzCoderInfo* coder, ISzAlloc *allocMain)
{
	p->s.Read = LzmaCoder_Read;
	p->s.Seek = NULL;

	LzmaDec_Construct(&p->state);
	RINOK(LzmaDec_Allocate(&p->state, coder->Props.data, (unsigned)coder->Props.size, allocMain));
	LzmaDec_Init(&p->state);

	return SZ_OK;
}

static SRes SzDecodeLzma(CSzCoderInfo *coder, UInt64 inSize, ILookInStream *inStream,
						 ISeqOutStream *outStream, SizeT outSize, ICompressProgress* progress, ISzAlloc *allocMain)
{
	CLzmaDec state;
	SRes res = SZ_OK;
	SizeT outSizeProcessed = 0;

	LzmaDec_Construct(&state);
	RINOK(LzmaDec_Allocate(&state, coder->Props.data, (unsigned)coder->Props.size, allocMain));
	LzmaDec_Init(&state);

	for (;;)
	{
		Byte *inBuf = NULL;
		SizeT curSize = state.dicBufSize - state.dicPos;
		SizeT rem = outSize - outSizeProcessed;
		SizeT lookahead = (1 << 18);
		if (lookahead > inSize)
			lookahead = (SizeT)inSize;
		res = inStream->Look((void *)inStream, (const void **)&inBuf, &lookahead);
		if (res != SZ_OK)
			break;

		if(rem <= curSize)
			curSize = rem;

		{
			Bool finished,stopDecoding;
			SizeT inProcessed = (SizeT)lookahead, dicPos = state.dicPos;
			ELzmaStatus status;
			res = LzmaDec_DecodeToDic(&state, dicPos + curSize, inBuf, &inProcessed, LZMA_FINISH_ANY, &status);
			lookahead -= inProcessed;
			inSize -= inProcessed;
			outSizeProcessed += (state.dicPos - dicPos);
			if (res != SZ_OK)
				break;

			if(progress != NULL)
			{
				progress->Progress(progress,inProcessed,state.dicPos - dicPos);
				if(progress->IsStop(progress))
				{
					res = SZ_ERROR_PROGRESS;
					break;
				}
			}

			finished = (inProcessed == 0 && dicPos == state.dicPos) ? True : False;
			stopDecoding = (outSizeProcessed >= outSize) ? True : False;

			if (state.dicPos == state.dicBufSize || finished || stopDecoding)
			{
				res = SeqOutStream_Write(outStream,state.dic,state.dicPos);
				if (res != SZ_OK)
					break;

				if (stopDecoding)
					break;

				if (finished)
				{ 
					res = (status == LZMA_STATUS_FINISHED_WITH_MARK ? SZ_OK : SZ_ERROR_DATA);
					break;
				}

				state.dicPos = 0;
			}
			res = inStream->Skip((void *)inStream, inProcessed);
			if (res != SZ_OK)
				break;
		}
	}

	LzmaDec_Free(&state, allocMain);
	return res;
}

//-----------------------------Lzma2-------------------------------------
typedef struct  
{
	ISeekInStream s;
	CCoder coder;
	CLzma2Dec state;
} CLzma2Coder;

static SRes Lzma2Coder_Read(void *pp, void *buf, size_t *size)
{
	CLzma2Coder* p = (CLzma2Coder*)pp;
	ILookInStream *inStream = p->coder.inStreams[0];
	UInt64 inSize = p->coder.packSizes[0];
	SRes res = SZ_OK;
	size_t originalSize = *size;
	if (originalSize == 0)
		return 0;

	*size = 0;
	do
	{
		Byte *inBuf = NULL;
		size_t inProcessed = (1 << 18);
		if (inProcessed > inSize)
			inProcessed = (size_t)inSize;
		res = inStream->Look((void *)inStream, (const void **)&inBuf, &inProcessed);
		if (res != SZ_OK || inProcessed == 0)
			break;

		{
			SizeT outProcessed = originalSize;
			ELzmaStatus status;
			res = Lzma2Dec_DecodeToBuf(&p->state, (Byte *)buf, &outProcessed, inBuf, &inProcessed, LZMA_FINISH_ANY, &status);

			buf = (void *)((Byte *)buf + outProcessed);
			originalSize -= outProcessed;
			*size += outProcessed;

			if (res != SZ_OK)
				break;

			res = inStream->Skip((void *)inStream, inProcessed);
			if (res != SZ_OK)
				break;
		}
	}
	while (originalSize > 0);
	return res;
}

SRes Lzma2Coder_Init(CLzma2Coder* p, CSzCoderInfo* coder, ISzAlloc *allocMain)
{
	p->s.Read = Lzma2Coder_Read;
	p->s.Seek = NULL;

	Lzma2Dec_Construct(&p->state);
	if (coder->Props.size != 1)
		return SZ_ERROR_DATA;
	RINOK(Lzma2Dec_Allocate(&p->state, coder->Props.data[0], allocMain));
	Lzma2Dec_Init(&p->state);

	return SZ_OK;
}

static SRes SzDecodeLzma2(CSzCoderInfo *coder, UInt64 inSize, ILookInStream *inStream,
						  ISeqOutStream *outStream, SizeT outSize, ICompressProgress* progress, ISzAlloc *allocMain)
{
	CLzma2Dec state;
	SRes res = SZ_OK;
	size_t outSizeProcessed = 0;

	Lzma2Dec_Construct(&state);
	if (coder->Props.size != 1)
		return SZ_ERROR_DATA;
	RINOK(Lzma2Dec_Allocate(&state, coder->Props.data[0], allocMain));
	Lzma2Dec_Init(&state);

	for (;;)
	{
		Byte *inBuf = NULL;
		SizeT curSize = state.decoder.dicBufSize - state.decoder.dicPos;
		SizeT rem = outSize - outSizeProcessed;
		SizeT lookahead = (1 << 18);
		if (lookahead > inSize)
			lookahead = (SizeT)inSize;
		res = inStream->Look((void *)inStream, (const void **)&inBuf, &lookahead);
		if (res != SZ_OK)
			break;

		if(rem <= curSize)
			curSize = rem;

		{
			Bool finished,stopDecoding;
			SizeT inProcessed = (SizeT)lookahead, dicPos = state.decoder.dicPos;
			ELzmaStatus status;
			res = Lzma2Dec_DecodeToDic(&state, dicPos + curSize, inBuf, &inProcessed, LZMA_FINISH_END, &status);
			lookahead -= inProcessed;
			inSize -= inProcessed;
			outSizeProcessed += (state.decoder.dicPos - dicPos);
			if (res != SZ_OK)
				break;

			if(progress != NULL)
			{
				progress->Progress(progress,inProcessed,state.decoder.dicPos - dicPos);
				if(progress->IsStop(progress))
				{
					res = SZ_ERROR_PROGRESS;
					break;
				}
			}

			finished = (inProcessed == 0 && dicPos == state.decoder.dicPos) ? True : False;
			stopDecoding = (outSizeProcessed >= outSize) ? True : False;

			if (state.decoder.dicPos == state.decoder.dicBufSize || finished || stopDecoding)
			{
				res = SeqOutStream_Write(outStream,state.decoder.dic,state.decoder.dicPos);
				if (res != SZ_OK)
					break;

				if (stopDecoding)
					break;

				if (finished)
				{ 
					res = (status == LZMA_STATUS_FINISHED_WITH_MARK ? SZ_OK : SZ_ERROR_DATA);
					break;
				}

				state.decoder.dicPos = 0;
			}

			res = inStream->Skip((void *)inStream, inProcessed);
			if (res != SZ_OK)
				break;
		}
	}

	Lzma2Dec_Free(&state, allocMain);
	return res;
}

static SRes SzDecodeCopy(UInt64 inSize, ILookInStream *inStream,  ISeqOutStream *outStream, ICompressProgress* progress)
{
	while (inSize > 0)
	{
		void *inBuf;
		size_t curSize = (1 << 18);
		if (curSize > inSize)
			curSize = (size_t)inSize;
		RINOK(inStream->Look((void *)inStream, (const void **)&inBuf, &curSize));
		if (curSize == 0)
			return SZ_ERROR_INPUT_EOF;
		SeqOutStream_Write(outStream, inBuf, curSize);
		inSize -= curSize;

		if(progress != NULL)
		{
			progress->Progress(progress,curSize,curSize);
			if(progress->IsStop(progress))
				return SZ_ERROR_PROGRESS;
		}

		RINOK(inStream->Skip((void *)inStream, curSize));
	}
	return SZ_OK;
}

static SRes SzDecodeBcj(ILookInStream *inStream, ISeqOutStream *outStream, SizeT outSize, ICompressProgress* progress)
{
	Byte buffer[BUFF_SIZE];
	UInt32 bufferPos = 0;
	SizeT processedSize = 0;
	UInt32 convertPos = 0;

	UInt32 state;
	x86_Convert_Init(state);

	while (processedSize < outSize)
	{
		SizeT curSize = BUFF_SIZE - bufferPos;
		UInt32 endPos;
		UInt32 writeSize = outSize - processedSize;
		UInt32 i;

		RINOK(inStream->Read(inStream, buffer+bufferPos, &curSize));

		endPos = bufferPos + curSize;

		bufferPos = x86_Convert(buffer, endPos, convertPos, &state, 0);
		convertPos += bufferPos;

		if(bufferPos > endPos)
		{
			for(; endPos<bufferPos; endPos++)
				buffer[endPos] = 0;

			bufferPos = x86_Convert(buffer, endPos, convertPos, &state, 0);
			convertPos += bufferPos;
		}

		if(bufferPos == 0)
		{
			if(endPos == 0)
				return SZ_OK;

			if(endPos < writeSize)
				writeSize = endPos;
			return SeqOutStream_Write(outStream, buffer, writeSize);
		}

		if(bufferPos < writeSize)
			writeSize = bufferPos;
		RINOK(SeqOutStream_Write(outStream, buffer, writeSize));
		processedSize += writeSize;

		if(progress != NULL)
		{
			progress->Progress(progress,processedSize,processedSize);
			if(progress->IsStop(progress))
				return SZ_ERROR_PROGRESS;
		}
		
		i = 0;
		while (bufferPos < endPos)
			buffer[i++] = buffer[bufferPos++];
		bufferPos = i;
	}
	return SZ_OK;
}

static Bool IS_MAIN_METHOD(UInt32 m)
{
	switch(m)
	{
	case k_Copy:
	case k_LZMA:
	case k_LZMA2:
#ifdef _7ZIP_PPMD_SUPPPORT
	case k_PPMD:
#endif
		return True;
	}
	return False;
}

static Bool IS_SUPPORTED_CODER(const CSzCoderInfo *c)
{
	return
		c->NumInStreams == 1 &&
		c->NumOutStreams == 1 &&
		c->MethodID <= (UInt32)0xFFFFFFFF &&
		IS_MAIN_METHOD((UInt32)c->MethodID);
}

#define IS_BCJ2(c) ((c)->MethodID == k_BCJ2 && (c)->NumInStreams == 4 && (c)->NumOutStreams == 1)

static SRes CheckSupportedFolder(const CSzFolder *f)
{
	if (f->NumCoders < 1 || f->NumCoders > 4)
		return SZ_ERROR_UNSUPPORTED;
	if (!IS_SUPPORTED_CODER(&f->Coders[0]))
		return SZ_ERROR_UNSUPPORTED;
	if (f->NumCoders == 1)
	{
		if (f->NumPackStreams != 1 || f->PackStreams[0] != 0 || f->NumBindPairs != 0)
			return SZ_ERROR_UNSUPPORTED;
		return SZ_OK;
	}
	if (f->NumCoders == 2)
	{
		CSzCoderInfo *c = &f->Coders[1];
		if (c->MethodID > (UInt32)0xFFFFFFFF ||
			c->NumInStreams != 1 ||
			c->NumOutStreams != 1 ||
			f->NumPackStreams != 1 ||
			f->PackStreams[0] != 0 ||
			f->NumBindPairs != 1 ||
			f->BindPairs[0].InIndex != 1 ||
			f->BindPairs[0].OutIndex != 0)
			return SZ_ERROR_UNSUPPORTED;
		switch ((UInt32)c->MethodID)
		{
		case k_BCJ:
		case k_ARM:
			break;
		default:
			return SZ_ERROR_UNSUPPORTED;
		}
		return SZ_OK;
	}
	if (f->NumCoders == 4)
	{
		if (!IS_SUPPORTED_CODER(&f->Coders[1]) ||
			!IS_SUPPORTED_CODER(&f->Coders[2]) ||
			!IS_BCJ2(&f->Coders[3]))
			return SZ_ERROR_UNSUPPORTED;
		if (f->NumPackStreams != 4 ||
			f->PackStreams[0] != 2 ||
			f->PackStreams[1] != 6 ||
			f->PackStreams[2] != 1 ||
			f->PackStreams[3] != 0 ||
			f->NumBindPairs != 3 ||
			f->BindPairs[0].InIndex != 5 || f->BindPairs[0].OutIndex != 0 ||
			f->BindPairs[1].InIndex != 4 || f->BindPairs[1].OutIndex != 1 ||
			f->BindPairs[2].InIndex != 3 || f->BindPairs[2].OutIndex != 2)
			return SZ_ERROR_UNSUPPORTED;
		return SZ_OK;
	}
	return SZ_ERROR_UNSUPPORTED;
}

static UInt64 GetSum(const UInt64 *values, UInt32 index)
{
	UInt64 sum = 0;
	UInt32 i;
	for (i = 0; i < index; i++)
		sum += values[i];
	return sum;
}

static CCoder* Coder_Create(CSzCoderInfo* coder, ISeekInStream** ppCoderStream, ISzAlloc* allocMain)
{
	CCoder *pCoder = 0;
	*ppCoderStream = 0;
	switch(coder->MethodID)
	{
	case k_LZMA:
		{
			CLzmaCoder* lzmaCoder = (CLzmaCoder*)IAlloc_Alloc(allocMain, sizeof(CLzmaCoder));
			LzmaCoder_Init(lzmaCoder,coder,allocMain);
			*ppCoderStream = &lzmaCoder->s;
			pCoder = &lzmaCoder->coder;
		}
		break;
	case k_LZMA2:
		{
			CLzma2Coder* lzma2Coder = (CLzma2Coder*)IAlloc_Alloc(allocMain, sizeof(CLzma2Coder));
			Lzma2Coder_Init(lzma2Coder,coder,allocMain);
			*ppCoderStream = &lzma2Coder->s;
			pCoder = &lzma2Coder->coder;
		}
		break;
	case k_BCJ:
	case k_BCJ2:
		pCoder = (CCoder*)IAlloc_Alloc(allocMain, sizeof(CCoder));
		break;
	}
	return pCoder;
}

static void Coder_Free(CCoder* pCoder, ISzAlloc* allocMain)
{
	void* pCoderPointer = pCoder;
	if(pCoder->outStreams && pCoder->outStreams[0])
	{
		if(pCoder->coder->MethodID == k_LZMA)
		{
			CLzmaCoder* pLzma = (CLzmaCoder*)pCoder->outStreams[0];
			LzmaDec_Free(&pLzma->state, allocMain);
			pCoderPointer = pLzma;
		}
		else if(pCoder->coder->MethodID == k_LZMA2)
		{
			CLzma2Coder* pLzma2 = (CLzma2Coder*)pCoder->outStreams[0];
			Lzma2Dec_Free(&pLzma2->state, allocMain);
			pCoderPointer = pLzma2;
		}
	}

	IAlloc_Free(allocMain,pCoder->packSizes);
	IAlloc_Free(allocMain,pCoder->inStreams);
	IAlloc_Free(allocMain,pCoder->unpackSize);
	IAlloc_Free(allocMain,pCoder->outStreams);
	IAlloc_Free(allocMain,pCoderPointer);
}

#define CASE_BRA_CONV(isa) case k_ ## isa: isa ## _Convert(outBuffer, outProcessedSize, 0, 0); break;

SRes SzFolder_Decode2(CCoder* pCoder, ISeqOutStream *outStream, size_t outSize, ICompressProgress* progress, ISzAlloc *allocMain)
{
	switch(pCoder->coder->MethodID)
	{
	case k_Copy:
		RINOK(SzDecodeCopy(pCoder->packSizes[0], pCoder->inStreams[0], outStream, progress));
		break;
	case k_LZMA:
		RINOK(SzDecodeLzma(pCoder->coder, pCoder->packSizes[0], pCoder->inStreams[0], outStream, outSize, progress, allocMain));
		break;
	case k_LZMA2:
		RINOK(SzDecodeLzma2(pCoder->coder, pCoder->packSizes[0], pCoder->inStreams[0], outStream, outSize, progress, allocMain));
		break;
	case k_BCJ:
		RINOK(SzDecodeBcj( pCoder->inStreams[0], outStream, outSize, progress));
		break;
	case k_BCJ2:
		RINOK(Bcj2_Decode( pCoder->inStreams, pCoder->packSizes, pCoder->numInStreams, outStream, outSize, progress));
		break;
	}
	return SZ_OK;
}

SRes SzFolder_Decode(const CSzFolder *folder, const UInt64 *packSizes,
					  ISeekInStream *inStream, UInt64 startPos,
					  ISeqOutStream *outStream, size_t outSize, ICompressProgress* progress, ISzAlloc *allocMain)
{
	CBufferStream *pPackStreams;
	CLookToRead *pBindStreams;
	CCoder **ppCoder;
	Int32 mainCoder;
	UInt32 inStreamIndex = 0, outStreamIndex = 0;
	UInt32 i,j;
	SRes res = SZ_OK;

	RINOK(CheckSupportedFolder(folder));

	mainCoder = SzFolder_FindMainCoder(folder);
	if(mainCoder < 0)
		return SZ_ERROR_FAIL;

	pPackStreams = (CBufferStream *)IAlloc_Alloc(allocMain,folder->NumPackStreams * sizeof(CBufferStream));
	for (i = 0; i < folder->NumPackStreams; i++)
	{
		BufferStream_CreateVTable(&pPackStreams[i], False);
		LookToRead_Init(&pPackStreams[i].s);
		pPackStreams[i].s.realStream = inStream;
		pPackStreams[i].startPos = startPos + GetSum(packSizes,i);
		pPackStreams[i].size = packSizes[i];
		pPackStreams[i].offset = 0;
	}

	ppCoder = (CCoder**)IAlloc_Alloc(allocMain, folder->NumCoders * sizeof(CCoder*));

	for (i = 0; i < folder->NumCoders; i++)
	{
		CSzCoderInfo *coder = &folder->Coders[i];
		ISeekInStream *pCurrCoderStream = 0;
		if(i == (UInt32)mainCoder)
			ppCoder[i] = (CCoder*)IAlloc_Alloc(allocMain, sizeof(CCoder));
		else
			ppCoder[i] = Coder_Create(coder,&pCurrCoderStream,allocMain);
		

		if(ppCoder[i] == 0)
		{
			res = SZ_ERROR_UNSUPPORTED;
			break;
		}

		ppCoder[i]->coder = coder;

		//get output size list
		ppCoder[i]->numOutStreams = coder->NumOutStreams;
		ppCoder[i]->unpackSize = (UInt64 *)IAlloc_Alloc(allocMain, coder->NumOutStreams * sizeof(UInt64));
		ppCoder[i]->outStreams = (ISeekInStream **)IAlloc_Alloc(allocMain, coder->NumOutStreams * sizeof(ISeekInStream*));

		for (j = 0; j < coder->NumOutStreams; j++, outStreamIndex++)
		{
			ppCoder[i]->unpackSize[j] = folder->UnpackSizes[outStreamIndex];
			ppCoder[i]->outStreams[j] = pCurrCoderStream;
		}

		//get input size list
		ppCoder[i]->numInStreams = coder->NumInStreams;
		ppCoder[i]->packSizes = (UInt64 *)IAlloc_Alloc(allocMain, coder->NumInStreams * sizeof(UInt64));
		ppCoder[i]->inStreams = (ILookInStream **)IAlloc_Alloc(allocMain, coder->NumInStreams * sizeof(ILookInStream *));

		for (j = 0; j < coder->NumInStreams; j++, inStreamIndex++)
		{
			Int32 bindPairIndex = SzFolder_FindBindPairForInStream(folder,inStreamIndex);
			if(bindPairIndex >= 0)
			{
				Int32 index = folder->BindPairs[bindPairIndex].OutIndex;
				ppCoder[i]->packSizes[j] = folder->UnpackSizes[index];
				ppCoder[i]->inStreams[j] = 0;
			}
			else
			{
				Int32 index = SzFolder_FindPackStreamsIndex(folder,inStreamIndex);
				ppCoder[i]->packSizes[j] = packSizes[index];
				ppCoder[i]->inStreams[j] = &pPackStreams[index].s.s;
			}
		}
	}

	//bind stream
	pBindStreams = (CLookToRead *)IAlloc_Alloc(allocMain,folder->NumBindPairs * sizeof(CLookToRead));
	for (i = 0; i < folder->NumBindPairs; i++)
	{
		CSzBindPair* bindPair = &folder->BindPairs[i];
		UInt32 inCoderIndex, inCoderStreamIndex;
		UInt32 outCoderIndex, outCoderStreamIndex;

		LookToRead_CreateVTable(&pBindStreams[i], True);
		LookToRead_Init(&pBindStreams[i]);

		SzFolder_FindInStream(folder,bindPair->InIndex,&inCoderIndex,&inCoderStreamIndex);
		SzFolder_FindOutStream(folder,bindPair->OutIndex,&outCoderIndex,&outCoderStreamIndex);

		pBindStreams[i].realStream = ppCoder[outCoderIndex]->outStreams[outCoderStreamIndex];
		ppCoder[inCoderIndex]->inStreams[inCoderStreamIndex] = &pBindStreams[i].s;
	}

	res = SzFolder_Decode2(ppCoder[mainCoder], outStream,outSize,progress,allocMain);

	for (i = 0; i < folder->NumCoders; i++)
		Coder_Free(ppCoder[i],allocMain);
	IAlloc_Free(allocMain,ppCoder);
	IAlloc_Free(allocMain,pPackStreams);
	IAlloc_Free(allocMain,pBindStreams);

	return res;
}