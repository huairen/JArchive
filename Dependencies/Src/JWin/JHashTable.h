#ifndef __JHASH_TABLE_H__
#define __JHASH_TABLE_H__

#include "JTypes.h"

namespace DictHash
{
	UInt32 NextPrime(UInt32 nSize);
}

class JHashTable
{
	struct LinkNode
	{
		LinkNode* pNext;
		UInt32 nKey;
		void* pData;
	};

public:
	JHashTable();
	~JHashTable();

	void ReSize(int nSize);
	void Destroy();

	bool Insert(UInt32 nKey, void* pData);
	void Remove(UInt32 nKey);
	void* Find(UInt32 nKey);

private:
	int m_nCount;

	LinkNode** m_pTable;
	int m_nTableSize;
};

#endif // __JLIST_H__