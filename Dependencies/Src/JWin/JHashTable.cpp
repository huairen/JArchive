#include "JHashTable.h"

namespace DictHash
{
	static UInt32 Primes[] = {
	53ul,         97ul,         193ul,       389ul,       769ul,
	1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
	49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
	1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
	50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
	1610612741ul, 3221225473ul, 4294967291ul
};

	UInt32 NextPrime( UInt32 nSize )
	{
		UInt32 nLen = sizeof(Primes) / sizeof(UInt32);
		for(UInt32 i=0; i<nLen; i++)
			if(Primes[i] > nSize)
				return Primes[i];

		return Primes[nLen-1];
	}
}

//-----------------------------------------------------------------------------------
JHashTable::JHashTable()
{
	m_pTable = 0;
	m_nTableSize = 0;
	m_nCount = 0;
}

JHashTable::~JHashTable()
{
	Destroy();
}

void JHashTable::ReSize( int nSize )
{
	UInt32 currentSize = m_nTableSize;
	m_nTableSize = DictHash::NextPrime(nSize);
	LinkNode** pNewTable = (LinkNode**)malloc(m_nTableSize * sizeof(LinkNode*));
	memset(pNewTable,0,m_nTableSize * sizeof(LinkNode*));

	for (UInt32 i = 0; i < currentSize; i++)
		for (LinkNode* pNode = m_pTable[i]; pNode; )
		{
			UInt32 nIndex = pNode->nKey % m_nTableSize;
			LinkNode** ppLink = &pNewTable[nIndex];
			LinkNode* tmp = pNode->pNext;
			pNode->pNext = *ppLink;
			*ppLink = pNode;
			pNode = tmp;
		}

		free(m_pTable);
		m_pTable = pNewTable;
}

void JHashTable::Destroy()
{
	for (int i = 0; i < m_nTableSize; i++)
		for (LinkNode* ptr = m_pTable[i]; ptr; ) 
		{
			LinkNode *tmp = ptr;
			ptr = ptr->pNext;
			free(tmp);
		}
		free(m_pTable);
		m_pTable = NULL;
}

bool JHashTable::Insert( UInt32 nKey, void* pData )
{
	if (m_nCount >= m_nTableSize)
		ReSize(m_nCount + 1);

	UInt32 nIndex = nKey % m_nTableSize;
	LinkNode** pWork = &m_pTable[nIndex];
	for (; *pWork; pWork = &((*pWork)->pNext))
		if ( (*pWork)->nKey == nKey )
			return false;

	m_nCount++;
	(*pWork) = (LinkNode*)malloc(sizeof(LinkNode));
	(*pWork)->pNext = NULL;
	(*pWork)->nKey = nKey;
	(*pWork)->pData = pData;

	return true;
}

void JHashTable::Remove( UInt32 nKey )
{
	if (m_pTable == NULL)
		return;

	UInt32 nIndex = nKey % m_nTableSize;
	LinkNode** pWork = &m_pTable[nIndex];
	for (; *pWork; pWork = &((*pWork)->pNext))
		if ( (*pWork)->nKey == nKey)
		{
			LinkNode* pTemp = (*pWork);
			(*pWork) = (*pWork)->pNext;
			free(pTemp);
			return;
		}
}

void* JHashTable::Find( UInt32 nKey )
{
	if(m_nTableSize > 0 && m_pTable != NULL)
	{
		UInt32 nIndex = nKey % m_nTableSize;
		LinkNode* pWork = m_pTable[nIndex];
		for (; pWork; pWork = pWork->pNext)
			if ( pWork->nKey == nKey)
				return pWork->pData;
	}

	return NULL;
}
