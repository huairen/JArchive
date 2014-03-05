/* Bcj2.c -- Converter for x86 code (BCJ2)
2008-10-04 : Igor Pavlov : Public domain */

#include "Bcj2.h"

#ifdef _LZMA_PROB32
#define CProb UInt32
#else
#define CProb UInt16
#endif

#define IsJcc(b0, b1) ((b0) == 0x0F && ((b1) & 0xF0) == 0x80)
#define IsJ(b0, b1) ((b1 & 0xFE) == 0xE8 || IsJcc(b0, b1))

#define kNumTopBits 24
#define kTopValue ((UInt32)1 << kNumTopBits)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)
#define kNumMoveBits 5

#define RC_READ_BYTE RINOK(LookInStream_LookReadByte(inStreams[3],&b));++buff3Pos
#define RC_TEST { if (buff3Pos == inSizes[3]) return SZ_ERROR_DATA; }
#define RC_INIT2 code = 0; range = 0xFFFFFFFF; \
  { int i; Byte b; for (i = 0; i < 5; i++) { RC_TEST; RC_READ_BYTE; code = (code << 8) | b; }}

#define NORMALIZE if (range < kTopValue) { Byte b; RC_TEST; range <<= 8; RC_READ_BYTE; code = (code << 8) | b; }

#define IF_BIT_0(p) ttt = *(p); bound = (range >> kNumBitModelTotalBits) * ttt; if (code < bound)
#define UPDATE_0(p) range = bound; *(p) = (CProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits)); NORMALIZE;
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (CProb)(ttt - (ttt >> kNumMoveBits)); NORMALIZE;

int Bcj2_Decode( ILookInStream** inStreams, UInt64* inSizes, UInt32 numInStrems,
				ISeqOutStream* outStream, SizeT outSize, ICompressProgress* progress )
{
	CProb p[256 + 2];
	SizeT inPos = 0, outPos = 0;

	UInt32 prevProcessSize = 0;
	UInt32 buff3Pos = 0;
	UInt32 range, code;
	Byte prevByte = 0;

	unsigned int i;
	for (i = 0; i < sizeof(p) / sizeof(p[0]); i++)
		p[i] = kBitModelTotal >> 1;

	if(numInStrems != 4)
		return SZ_ERROR_DATA;

	if (outSize == 0)
		return SZ_OK;

	RC_INIT2;

	for (;;)
	{
		Byte b;
		CProb *prob;
		UInt32 bound;
		UInt32 ttt;
		SizeT processedSize = outPos - prevProcessSize;

		SizeT limit = (SizeT)inSizes[0] - inPos;
		if (outSize - outPos < limit)
			limit = outSize - outPos;

		if (processedSize >= (1<<14) && progress != NULL)
		{
			progress->Progress(progress,0,processedSize);
			prevProcessSize = outPos;
			if(progress->IsStop(progress))
				return SZ_ERROR_PROGRESS;
		}

		while (limit != 0)
		{
			RINOK(LookInStream_LookReadByte(inStreams[0],&b));
			inPos++;

			RINOK(SeqOutStream_WriteByte(outStream,&b));
			outPos++;

			if (IsJ(prevByte, b))
				break;

			prevByte = b;
			limit--;
		}

		if (limit == 0 || outPos == outSize)
			break;

		if (b == 0xE8)
			prob = p + prevByte;
		else if (b == 0xE9)
			prob = p + 256;
		else
			prob = p + 257;

		IF_BIT_0(prob)
		{
			UPDATE_0(prob);
			prevByte = b;
		}
		else
		{
			UInt32 dest = 0;
			ILookInStream* s;
			Int32 i;

			UPDATE_1(prob);

			s = (b == 0xE8) ? inStreams[1] : inStreams[2];
			for (i = 0; i < 4; i++)
			{
				Byte b0;
				RINOK(LookInStream_LookReadByte(s,&b0));
				dest <<= 8;
				dest |= ((UInt32)b0);
			}

			dest -= ((UInt32)outPos + 4);

			b = (Byte)dest;
			SeqOutStream_WriteByte(outStream,&b);
			outPos++;
			if (outPos == outSize)
				break;

			b = (Byte)(dest >> 8);
			SeqOutStream_WriteByte(outStream,&b);
			outPos++;
			if (outPos == outSize)
				break;

			b = (Byte)(dest >> 16);
			SeqOutStream_WriteByte(outStream,&b);
			outPos++;
			if (outPos == outSize)
				break;

			b = prevByte = (Byte)(dest >> 24);
			SeqOutStream_WriteByte(outStream,&b);
			outPos++;
		}
	}
	return (outPos == outSize) ? SZ_OK : SZ_ERROR_DATA;
}
