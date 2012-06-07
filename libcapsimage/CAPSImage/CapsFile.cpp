// CapsFile.cpp: implementation of the CCapsFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



CCapsFile::CCapsFile()
{
	filetype=ftInvalid;
	filesize=0;
	filepos=0;
	fileflag=0;
	filebuf=NULL;
}

CCapsFile::~CCapsFile()
{
	Close();
}

// open file
int CCapsFile::Open(PCAPSFILE pcf)
{
	int res=true;

	Close();

	if (!pcf)
		return res;

	if (pcf->flag & CFF_MEMMAP)
		res=OpenMemFile(pcf);
	else
		res=OpenDiskFile(pcf);

	return res;
}

// close file
int CCapsFile::Close()
{
	int res=true;

	switch (filetype) {
		case ftDiskFile:
			res=CloseDiskFile();
			break;

		case ftMemFile:
			res=CloseMemFile();
			break;
	}

	filetype=ftInvalid;
	filesize=0;
	filepos=0;
	fileflag=0;

	return res;
}

// read file chunk into memory
int CCapsFile::Read(PUBYTE buf, int size)
{
	int res=0;

	if (!buf || size<0)
		return res;

	switch (filetype) {
		case ftDiskFile:
			res=ReadDiskFile(buf, size);
			break;

		case ftMemFile:
			res=ReadMemFile(buf, size);
			break;
	}

	return res;
}

// write file
int CCapsFile::Write(PUBYTE buf, int size)
{
	int res=0;

	if (!buf || size<0 || !(fileflag&CFF_WRITE))
		return res;

	switch (filetype) {
		case ftDiskFile:
			res=WriteDiskFile(buf, size);
			break;

		case ftMemFile:
			res=WriteMemFile(buf, size);
			break;
	}

	return res;
}

// seek in file
int CCapsFile::Seek(int pos, int mode)
{
	int res=0;

	switch (filetype) {
		case ftDiskFile:
			res=SeekDiskFile(pos, mode);
			break;

		case ftMemFile:
			res=SeekMemFile(pos, mode);
			break;
	}

	return res;
}



int CCapsFile::OpenDiskFile(PCAPSFILE pcf)
{
	if (!pcf->name || !strlen(pcf->name))
		return true;

	UINT fm=(pcf->flag & CFF_WRITE) ? CFile::modeReadWrite : CFile::modeRead;
	if (pcf->flag & CFF_CREATE)
		fm |= CFile::modeCreate;

	if (!dfile.Open(pcf->name, fm|CFile::shareDenyWrite))
		return true;

	filesize=dfile.GetLength();
	fileflag=pcf->flag;
	filetype=ftDiskFile;

	return false;
}

int CCapsFile::CloseDiskFile()
{
	dfile.Close();
	return false;
}

int CCapsFile::ReadDiskFile(PUBYTE buf, int size)
{
	int res=dfile.Read(buf, size);
	filepos+=res;

	return res;
}

int CCapsFile::WriteDiskFile(PUBYTE buf, int size)
{
	dfile.Write(buf, size);
	filepos=dfile.GetPosition();

	return size;
}

int CCapsFile::SeekDiskFile(int pos, int mode)
{
	if (!mode)
		mode=CFile::current;
	else
		mode=CFile::begin;

	return filepos=dfile.Seek(pos, mode);
}



int CCapsFile::OpenMemFile(PCAPSFILE pcf)
{
	if (!pcf->memmap || pcf->size<0)
		return true;

	if (pcf->size) {
		if (pcf->flag & CFF_MEMREF)
			filebuf=pcf->memmap;
		else {
			filebuf=new UBYTE[pcf->size];
			memcpy(filebuf, pcf->memmap, pcf->size);
		}

		filesize=pcf->size;
	}

	fileflag=pcf->flag;
	filetype=ftMemFile;

	return false;
}

int CCapsFile::CloseMemFile()
{
	if (!(fileflag & CFF_MEMREF))
		delete [] filebuf;

	filebuf=NULL;

	return false;
}

int CCapsFile::ReadMemFile(PUBYTE buf, int size)
{
	if (filesize-filepos < size)
		size=filesize-filepos;

	if (size) {
		memcpy(buf, filebuf+filepos, size);
		filepos+=size;
	}

	return size;
}

int CCapsFile::WriteMemFile(PUBYTE buf, int size)
{
	return 0;
}

int CCapsFile::SeekMemFile(int pos, int mode)
{
	if (!mode)
		filepos+=pos;
	else
		filepos=pos;

	if (filepos < 0)
		filepos=0;
	else
		if (filepos > filesize)
			filepos=filesize;

	return filepos;
}

