#include "stdafx.h"

#ifdef AMIGA
#include <exec/alerts.h>
#include <proto/exec.h>

extern APTR MemPool;

void* operator new(size_t sz)
{
	size_t *p;
	
	// malloc(0) is unpredictable, avoid it
	if (!sz)
		sz = 1;
	// allocate shareable public memory
	if (!(p = (size_t *)AllocPooled(MemPool, sz + sizeof(size_t))))
		Alert(AT_DeadEnd|AN_Unknown|AG_NoMemory);
	*p++ = sz;
	
	return p;
}

void operator delete(void *ptr)
{
	size_t sz = *--(size_t *&)ptr;

	FreePooled(MemPool, ptr, sz);
}

void* operator new[](size_t sz)
{
	return operator new(sz);
}

void operator delete[](void *ptr)
{
	return operator delete(ptr);
}
#endif // AMIGA

extern "C"
void GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
	time_t t = time(NULL);
	struct tm *tp = localtime(&t);
	
	lpSystemTime->wYear = tp->tm_year + 1900;
	lpSystemTime->wMonth = tp->tm_mon + 1;
	lpSystemTime->wDayOfWeek = tp->tm_wday;
	lpSystemTime->wDay = tp->tm_mday;
	lpSystemTime->wHour = tp->tm_hour;
	lpSystemTime->wMinute = tp->tm_min;
	lpSystemTime->wSecond = tp->tm_sec;
	// we don't have milliseconds in struct tm
	lpSystemTime->wMilliseconds = 0;
}

// set array size
void CDWordArray::SetSize(int nNewSize, int nGrowBy)
{
	m_nGrowBy = nGrowBy;
	
	if (nNewSize < m_nSize || nNewSize > m_nMaxSize)
	{
		DWORD *pNewData = NULL;
		
		if ((m_nMaxSize = nNewSize < m_nSize ? nNewSize : nNewSize > m_nSize + m_nGrowBy ? nNewSize : m_nSize + m_nGrowBy))
		{
			pNewData = new DWORD[m_nMaxSize];
			// if m_pData is NULL, m_nSize will be 0 and no bytes are copied
			memcpy(pNewData, m_pData, (nNewSize < m_nSize ? nNewSize : m_nSize) * sizeof(DWORD));
		}
		delete [] m_pData;
		m_pData = pNewData;
	}
	m_nSize = nNewSize;
}

// remove all array elements
void CDWordArray::RemoveAll()
{
	delete [] m_pData;
	m_pData = NULL;
	m_nSize = 0;
	m_nMaxSize = 0;
}

// set element and grow array if necessary
void CDWordArray::SetAtGrow(int nIndex, DWORD newElement)
{
	if (nIndex >= m_nSize)
		SetSize(nIndex + 1, m_nGrowBy);
	
	m_pData[nIndex] = newElement;
}

// get current file position
DWORD CFile::GetPosition() const
{
	return lseek(m_hFile, 0, SEEK_CUR);
}

// open file
BOOL CFile::Open(LPCTSTR lpszFileName, UINT nOpenFlags)
{
	int flags = nOpenFlags & modeReadWrite ? O_RDWR : O_RDONLY;
	
	if (nOpenFlags & modeCreate)
		flags |= O_CREAT|O_TRUNC;
	m_hFile = open(lpszFileName, flags, DEFFILEMODE);
	
	return m_hFile != hFileNull;
}

// seek in file
LONG CFile::Seek(LONG lOff, UINT nFrom)
{
	return lseek(m_hFile, lOff, nFrom == begin ? SEEK_SET : SEEK_CUR);
}

// get file length
DWORD CFile::GetLength() const
{
	const DWORD nPosition = GetPosition();
	const DWORD nLength = lseek(m_hFile, 0, SEEK_END);
	
	lseek(m_hFile, nPosition, SEEK_SET);
	
	return nLength;
}

// read file chunk into memory
UINT CFile::Read(void* lpBuf, UINT nCount)
{
	return read(m_hFile, lpBuf, nCount);
}

// write file
void CFile::Write(const void* lpBuf, UINT nCount)
{
	write(m_hFile, lpBuf, nCount);
}

// close file
void CFile::Close()
{
	if (m_hFile != hFileNull)
		close(m_hFile);
	
	m_hFile = (UINT)hFileNull;
}

CFile::~CFile()
{
	Close();
}

// set array size
void CPtrArray::SetSize(int nNewSize, int nGrowBy)
{
	m_nGrowBy = nGrowBy;
	
	if (nNewSize < m_nSize || nNewSize > m_nMaxSize)
	{
		void **pNewData = NULL;
		
		if ((m_nMaxSize = nNewSize < m_nSize ? nNewSize : nNewSize > m_nSize + m_nGrowBy ? nNewSize : m_nSize + m_nGrowBy))
		{
			pNewData = new void *[m_nMaxSize];
			// if m_pData is NULL, m_nSize will be 0 and no bytes are copied
			memcpy(pNewData, m_pData, (nNewSize < m_nSize ? nNewSize : m_nSize) * sizeof(void *));
		}
		delete [] m_pData;
		m_pData = pNewData;
	}
	m_nSize = nNewSize;
}

// set element and grow array if necessary
void CPtrArray::SetAtGrow(int nIndex, void* newElement)
{
	if (nIndex >= m_nSize)
		SetSize(nIndex + 1, m_nGrowBy);
	
	m_pData[nIndex] = newElement;
}

