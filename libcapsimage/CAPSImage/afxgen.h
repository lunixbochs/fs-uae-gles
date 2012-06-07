#ifndef AFXGEN_H
#define AFXGEN_H

#ifdef AMIGA
#include <exec/types.h>
#else
#include <stdint.h>
#endif // AMIGA
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

#ifndef __cdecl
#define __cdecl
#endif
#define _lrotl(x,n) (((x) << (n)) | ((x) >> (sizeof(x)*8-(n))))
#define _lrotr(x,n) (((x) >> (n)) | ((x) << (sizeof(x)*8-(n))))

#define TRUE        1
#define FALSE       0

#ifdef AMIGA
typedef LONG        DWORD;
typedef ULONG       UINT;
#else
typedef int16_t     BOOL;
typedef int16_t     WORD;
typedef int32_t     LONG;
typedef int32_t     DWORD;
typedef uint32_t    UINT;
#endif // AMIGA

typedef const char *LPCSTR;
typedef const char *LPCTSTR;

typedef struct _SYSTEMTIME {
	WORD wYear;
	WORD wMonth;
	WORD wDayOfWeek;
	WORD wDay;
	WORD wHour;
	WORD wMinute;
	WORD wSecond;
	WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;

extern "C"
void GetLocalTime(LPSYSTEMTIME lpSystemTime);

// array of 32-bit doublewords
class CDWordArray
{
public:
	CDWordArray();
	
	int GetSize() const;
	void SetSize(int nNewSize, int nGrowBy = -1);
	
	void RemoveAll();
	
	void SetAtGrow(int nIndex, DWORD newElement);
	
	DWORD& operator[](int nIndex);
	
protected:
	DWORD* m_pData;
	int m_nSize;
	int m_nMaxSize;
	int m_nGrowBy;
	
public:
	~CDWordArray();
};

inline CDWordArray::CDWordArray()
{
	m_pData = NULL;
	m_nSize = 0;
	m_nMaxSize = 0;
	m_nGrowBy = 1;
}

// get array size
inline int CDWordArray::GetSize() const
{
	return m_nSize;
}

// get or set array element
inline DWORD& CDWordArray::operator[](int nIndex)
{
	return m_pData[nIndex];
}

inline CDWordArray::~CDWordArray()
{
	RemoveAll();
}

// unbuffered file services
class CFile
{
public:
	enum OpenFlags {
		modeRead =		0x0000,
		modeReadWrite =		0x0002,
		shareDenyWrite =	0x0020,
		modeCreate =		0x1000
	};
	
	enum SeekPosition {
		begin =		0x0,
		current =	0x1
	};
	
	enum {
		hFileNull = -1
	};
	
	CFile();
	
	UINT m_hFile;
	
	virtual DWORD GetPosition() const;
	
	virtual BOOL Open(LPCTSTR lpszFileName, UINT nOpenFlags);
	
	virtual LONG Seek(LONG lOff, UINT nFrom);
	virtual DWORD GetLength() const;
	
	virtual UINT Read(void* lpBuf, UINT nCount);
	virtual void Write(const void* lpBuf, UINT nCount);
	
	virtual void Close();
	
public:
	virtual ~CFile();
};

inline CFile::CFile()
{
	m_hFile = (UINT)hFileNull;
}

// array of void pointers
class CPtrArray
{
public:
	CPtrArray();
	
	int GetSize() const;
	void SetSize(int nNewSize, int nGrowBy = -1);
	
	void SetAtGrow(int nIndex, void* newElement);
	
	void*& operator[](int nIndex);
	
protected:
	void** m_pData;
	int m_nSize;
	int m_nMaxSize;
	int m_nGrowBy;
	
public:
	~CPtrArray();
};

inline CPtrArray::CPtrArray()
{
	m_pData = NULL;
	m_nSize = 0;
	m_nMaxSize = 0;
	m_nGrowBy = 1;
}

// get array size
inline int CPtrArray::GetSize() const
{
	return m_nSize;
}

// get or set array element
inline void*& CPtrArray::operator[](int nIndex)
{
	return m_pData[nIndex];
}

inline CPtrArray::~CPtrArray()
{
	delete [] m_pData;
}

template<class BASE_CLASS, class TYPE>
class CTypedPtrArray : public BASE_CLASS
{
public:
	void SetAtGrow(int nIndex, TYPE newElement)
	{ BASE_CLASS::SetAtGrow(nIndex, newElement); }
	
	TYPE& operator[](int nIndex)
	{ return (TYPE&)BASE_CLASS::operator[](nIndex); }
};

#endif // AFXGEN_H
