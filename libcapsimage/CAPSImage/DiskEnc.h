// DiskEnc.h: interface for the CDiskEncoding class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DISKENC_H__DA46DC34_AC2C_4649_AD21_79EF7F573320__INCLUDED_)
#define AFX_DISKENC_H__DA46DC34_AC2C_4649_AD21_79EF7F573320__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// gcr mode id
enum {
	gcridNone,
	gcridCBM
};



// disk encoding functions
class CDiskEncoding
{
public:
	CDiskEncoding();
	virtual ~CDiskEncoding();
	static void InitFM();
	static void InitMFM(uint32_t mfmsize);
	static void InitGCRCBM(uint32_t *gcrtable, int gcrid);
	static void InitGCRAppleH();
	static void InitGCRApple5(uint32_t *gcrtable);
	static void InitGCRApple6(uint32_t *gcrtable);

protected:
	void Clear();

public:
	static uint32_t fminit; // fm table initialized
	static uint32_t *fmcode; // fm code table
	static uint32_t *fmdecode; // fm decode table
	static uint32_t mfminit; // mfm table initialized
	static uint32_t mfmcodebit; // mfm table number of bits to index the code table
	static uint32_t *mfmcode; // mfm code table
	static uint32_t *mfmdecode; // mfm decode table
	static int gcrinit; // gcr table initialized
	static uint32_t *gcrcode; // gcr code table
	static uint32_t *gcrdecode; // gcr decode table
	static int gcrahinit; // gcr apple header table initialized
	static uint32_t *gcrahcode; // gcr apple header code table
	static uint32_t *gcrahdecode; // gcr apple header decode table
	static int gcra5init; // gcr apple 5 bit table initialized
	static uint32_t *gcra5code; // gcr apple 5 bit code table
	static uint32_t *gcra5decode; // gcr apple 5 bit decode table
	static int gcra6init; // gcr apple 6 bit table initialized
	static uint32_t *gcra6code; // gcr apple 6 bit code table
	static uint32_t *gcra6decode; // gcr apple 6 bit decode table

	static uint32_t gcr_cbm[]; // cbm gcr table
	static uint32_t gcr_apple5[]; // apple 5 bit gcr table
	static uint32_t gcr_apple6[]; // apple 6 bit gcr table
};

typedef CDiskEncoding *PCDISKENCODING;

#endif // !defined(AFX_DISKENC_H__DA46DC34_AC2C_4649_AD21_79EF7F573320__INCLUDED_)
