// CapsFile.h: interface for the CCapsFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CAPSFILE_H__C3D2E643_FE8E_49FB_BD49_394831B222A4__INCLUDED_)
#define AFX_CAPSFILE_H__C3D2E643_FE8E_49FB_BD49_394831B222A4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



// generic file descriptor
struct CapsFile {
	LPCSTR name;
	PUBYTE memmap;
	UDWORD flag;
	int size;
};

typedef CapsFile *PCAPSFILE;

// file mode flags
#define CFF_WRITE  DF_0
#define CFF_MEMMAP DF_1
#define CFF_MEMREF DF_2
#define CFF_CREATE DF_3



// CAPS standard file handler
class CCapsFile
{
public:
	CCapsFile();
	virtual ~CCapsFile();
	int Open(PCAPSFILE pcf);
	int Close();
	int Read(PUBYTE buf, int size);
	int Write(PUBYTE buf, int size);
	int Seek(int pos, int mode);
	int IsOpen();
	int GetSize();
	int GetPosition();

protected:
	enum {
		ftInvalid,
		ftDiskFile,
		ftMemFile,
	};

	int OpenDiskFile(PCAPSFILE pcf);
	int CloseDiskFile();
	int ReadDiskFile(PUBYTE buf, int size);
	int WriteDiskFile(PUBYTE buf, int size);
	int SeekDiskFile(int pos, int mode);

	int OpenMemFile(PCAPSFILE pcf);
	int CloseMemFile();
	int ReadMemFile(PUBYTE buf, int size);
	int WriteMemFile(PUBYTE buf, int size);
	int SeekMemFile(int pos, int mode);

protected:
	int filetype;
	int filesize;
	int filepos;
	UDWORD fileflag;
	PUBYTE filebuf;
	CFile dfile;
};

typedef CCapsFile *PCCAPSFILE;



// check whether file is accessible
inline int CCapsFile::IsOpen()
{
	return filetype != ftInvalid;
}

// get file size
inline int CCapsFile::GetSize()
{
	return filesize;
}

// get current file position
inline int CCapsFile::GetPosition()
{
	return filepos;
}

#endif // !defined(AFX_CAPSFILE_H__C3D2E643_FE8E_49FB_BD49_394831B222A4__INCLUDED_)
