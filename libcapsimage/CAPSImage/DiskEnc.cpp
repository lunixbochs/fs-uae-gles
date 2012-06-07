#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// fm tables
uint32_t CDiskEncoding::fminit=0;
uint32_t *CDiskEncoding::fmcode=NULL;
uint32_t *CDiskEncoding::fmdecode=NULL;

// mfm tables
uint32_t CDiskEncoding::mfminit=0;
uint32_t CDiskEncoding::mfmcodebit=0;
uint32_t *CDiskEncoding::mfmcode=NULL;
uint32_t *CDiskEncoding::mfmdecode=NULL;

// gcr tables
int CDiskEncoding::gcrinit=gcridNone;
uint32_t *CDiskEncoding::gcrcode=NULL;
uint32_t *CDiskEncoding::gcrdecode=NULL;

// gcr apple header tables
int CDiskEncoding::gcrahinit=0;
uint32_t *CDiskEncoding::gcrahcode=NULL;
uint32_t *CDiskEncoding::gcrahdecode=NULL;

// gcr apple 5 bit tables
int CDiskEncoding::gcra5init=0;
uint32_t *CDiskEncoding::gcra5code=NULL;
uint32_t *CDiskEncoding::gcra5decode=NULL;

// gcr apple 6 bit tables
int CDiskEncoding::gcra6init=0;
uint32_t *CDiskEncoding::gcra6code=NULL;
uint32_t *CDiskEncoding::gcra6decode=NULL;

// GCR table used by CBM DOS
uint32_t CDiskEncoding::gcr_cbm[]= {
	0x0a,
	0x0b,
	0x12,
	0x13,
	0x0e,
	0x0f,
	0x16,
	0x17,
	0x09,
	0x19,
	0x1a,
	0x1b,
	0x0d,
	0x1d,
	0x1e,
	0x15
};

// 5 bit GCR table used by Apple DOS
uint32_t CDiskEncoding::gcr_apple5[]= {
	0xab, 0xad, 0xae, 0xaf, 0xb5, 0xb6, 0xb7, 0xba,
	0xbb, 0xbd, 0xbe, 0xbf, 0xd6, 0xd7, 0xda, 0xdb,
	0xdd, 0xde, 0xdf, 0xea, 0xeb, 0xed, 0xee, 0xef,
	0xf5, 0xf6, 0xf7, 0xfa, 0xfb, 0xfd, 0xfe, 0xff
};

// 6 bit GCR table used by Apple DOS
uint32_t CDiskEncoding::gcr_apple6[]= {
	0x96, 0x97, 0x9a, 0x9b, 0x9d, 0x9e, 0x9f, 0xa6,
	0xa7, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb2, 0xb3,
	0xb4, 0xb5, 0xb6, 0xb7, 0xb9, 0xba, 0xbb, 0xbc,
	0xbd, 0xbe, 0xbf, 0xcb, 0xcd, 0xce, 0xcf, 0xd3,
	0xd6, 0xd7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde,
	0xdf, 0xe5, 0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec,
	0xed, 0xee, 0xef, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
	0xf7, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};



CDiskEncoding::CDiskEncoding()
{
}

CDiskEncoding::~CDiskEncoding()
{
	delete [] fmcode;
	delete [] fmdecode;

	delete [] mfmcode;
	delete [] mfmdecode;

	delete [] gcrcode;
	delete [] gcrdecode;

	delete [] gcrahcode;
	delete [] gcrahdecode;

	delete [] gcra5code;
	delete [] gcra5decode;

	delete [] gcra6code;
	delete [] gcra6decode;

	Clear();
}

// reset variables
void CDiskEncoding::Clear()
{
	fminit=0;
	fmcode=NULL;
	fmdecode=NULL;

	mfminit=0;
	mfmcodebit=0;
	mfmcode=NULL;
	mfmdecode=NULL;

	gcrinit=gcridNone;
	gcrcode=NULL;
	gcrdecode=NULL;

	gcrahinit=0;
	gcrahcode=NULL;
	gcrahdecode=NULL;

	gcra5init=0;
	gcra5code=NULL;
	gcra5decode=NULL;

	gcra6init=0;
	gcra6code=NULL;
	gcra6decode=NULL;
}

// initialize FM tables
void CDiskEncoding::InitFM()
{
	// stop if correct tables are available
	if (fminit)
		return;

	// create tables
	if (!fmcode)
		fmcode=new uint32_t[256];

	if (!fmdecode)
		fmdecode=new uint32_t[65536];

	uint32_t sval;

	// create FM word code table
	for (sval=0x0000; sval < 256; sval++) {
		uint32_t code=0;

		// generate clock and data bits
		for (uint32_t bit=0x80; bit; bit>>=1) {
			code<<=2;

			// always set clock to 1, and add data
			code|=(sval & bit) ? 3 : 2;
		}

		// store encoding
		fmcode[sval]=code;
	}

	// create FM decode table
	for (sval=0x0000; sval < 0x10000; sval++) {
		uint32_t code=0;

		// all bits
		for (uint32_t bit=0x4000; bit; bit>>=2) {
			code<<=1;
			if (sval & bit)
				code|=1;
		}

		// store decoding, mark coding erros
		uint32_t cd=fmcode[code] & 0xffff;
		if (cd != sval)
			code|=DF_31;
		fmdecode[sval]=code;
	}

	fminit=1;
}

// initialize MFM tables
void CDiskEncoding::InitMFM(uint32_t mfmsize)
{
	// cancel if correct tables are available
	if (mfmsize && mfminit>=mfmsize)
		return;

	// clear tables
	delete [] mfmcode;
	mfmcode=NULL;
	delete [] mfmdecode;
	mfmdecode=NULL;

	mfminit=0;
	mfmcodebit=0;

	// stop if empty tables
	if (!mfmsize)
		return;

	// create tables
	mfmcode=new uint32_t[mfmsize];
	mfmdecode=new uint32_t[mfmsize];

	// code only ever uses 8 or 16 bit indexing
	mfmcodebit=(mfmsize > 0x100) ? 16 : 8;

	uint32_t sval;

	// create MFM word code table (last code bit assumed 0)
	for (sval=0x0000; sval < mfmsize; sval++) {
		uint32_t code=0;

		// all bits
		for (uint32_t bit=0x8000; bit; bit>>=1) {
			code<<=2;
			// bit 0 encoded as 00, if last bit is 1
			// bit 0 encoded as 10, if last bit is 0
			// bit 1 encoded as 01
			if (sval & bit)
				code|=1;
			else
				if (!(code & 4))
					code|=2;
		}

		// store encoding
		mfmcode[sval]=code;
	}

	// create MFM decode table, possible errors marked for analyser only
	if (mfmsize > 0x100) {
		for (sval=0x0000; sval < mfmsize; sval++) {
			uint32_t code=0;

			// all bits
			for (uint32_t bit=0x4000; bit; bit>>=2) {
				code<<=1;
				if (sval & bit)
					code|=1;
			}

			// store decoding, mark coding erros
			uint32_t cd=mfmcode[code]&0xffff;
			if (cd!=sval && (cd&0x7fff)!=sval)
				code|=DF_31;
			mfmdecode[sval]=code;
		}
	} else {
		for (sval=0x0000; sval < mfmsize; sval++) {
			uint32_t code=0;

			// all bits
			for (uint32_t bit=0x4000; bit; bit>>=2) {
				code<<=1;
				if (sval & bit)
					code|=1;
			}

			// store decoding
			mfmdecode[sval]=code;
		}
	}

	mfminit=mfmsize;
}

// initialize GCR tables
void CDiskEncoding::InitGCRCBM(uint32_t *gcrtable, int gcrid)
{
	// cancel if table initialized with the same gcr version
	if (gcrid == gcrinit)
		return;

	// create tables
	if (!gcrcode)
		gcrcode=new uint32_t[256];

	if (!gcrdecode)
		gcrdecode=new uint32_t[1024];

	uint32_t sval;

	// set GCR decode table to coding error as default
	for (sval=0; sval < 1024; sval++)
		gcrdecode[sval]=DF_31;

	// create GCR table
	for (sval=0; sval < 256; sval++) {
		uint32_t ghi=gcrtable[sval >> 4];
		uint32_t glo=gcrtable[sval & 0xf];
		uint32_t code=ghi << 5 | glo;
		gcrcode[sval]=code;
		gcrdecode[code]=sval;
	}

	gcrinit=gcrid;
}

// initialize GCR Apple Header tables
void CDiskEncoding::InitGCRAppleH()
{
	// stop if correct tables are available
	if (gcrahinit)
		return;

	// create tables
	if (!gcrahcode)
		gcrahcode=new uint32_t[256];

	if (!gcrahdecode)
		gcrahdecode=new uint32_t[65536];

	uint32_t sval;

	// create code table
	for (sval=0x0000; sval < 256; sval++) {
		// generate clock and data bits, like FM but bits are interleaved
		// C7C5C3C1 C6C4C2C0
		uint32_t ahi=(sval >> 1) | 0xaa;
		uint32_t alo=sval | 0xaa;
		uint32_t code=ahi << 8 | alo;

		// store encoding
		gcrahcode[sval]=code;
	}

	// create decode table
	for (sval=0x0000; sval < 0x10000; sval++) {
		uint32_t dhi=(sval >> 8) & 0x55;
		uint32_t dlo=sval & 0x55;
		uint32_t code=dhi << 1 | dlo;

		// store decoding, mark coding erros
		uint32_t cd=gcrahcode[code] & 0xffff;
		if (cd != sval)
			code|=DF_31;
		gcrahdecode[sval]=code;
	}

	gcrahinit=1;
}

// initialize GCR Apple 5 bit tables
void CDiskEncoding::InitGCRApple5(uint32_t *gcrtable)
{
	// stop if correct tables are available
	if (gcra5init)
		return;

	// create tables
	if (!gcra5code)
		gcra5code=new uint32_t[32];

	if (!gcra5decode)
		gcra5decode=new uint32_t[256];

	uint32_t sval;

	// set GCR decode table to coding error as default
	for (sval=0; sval < 256; sval++)
		gcra5decode[sval]=DF_31;

	// create GCR table
	for (sval=0; sval < 32; sval++) {
		uint32_t code=gcrtable[sval];
		gcra5code[sval]=code;
		gcra5decode[code]=sval;
	}

	gcra5init=1;
}

// initialize GCR Apple 6 bit tables
void CDiskEncoding::InitGCRApple6(uint32_t *gcrtable)
{
	// stop if correct tables are available
	if (gcra6init)
		return;

	// create tables
	if (!gcra6code)
		gcra6code=new uint32_t[64];

	if (!gcra6decode)
		gcra6decode=new uint32_t[256];

	uint32_t sval;

	// set GCR decode table to coding error as default
	for (sval=0; sval < 256; sval++)
		gcra6decode[sval]=DF_31;

	// create GCR table
	for (sval=0; sval < 64; sval++) {
		uint32_t code=gcrtable[sval];
		gcra6code[sval]=code;
		gcra6decode[code]=sval;
	}

	gcra6init=1;
}

