//
// IPF DECODER LIBRARY Copyright (c) 2001-2011 by István Fábián; additional work by Christian Sauer,
// under exclusive licence to KryoFlux Products & Services Ltd., http://www.kryoflux.com
// For licensing options please see LICENCE.txt
//
// Version: 4.2
//
// CAPSImg.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "CAPSImg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

/////////////////////////////////////////////////////////////////////////////
// CCAPSImgApp

BEGIN_MESSAGE_MAP(CCAPSImgApp, CWinApp)
	//{{AFX_MSG_MAP(CCAPSImgApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCAPSImgApp construction

CCAPSImgApp::CCAPSImgApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCAPSImgApp object

CCAPSImgApp theApp;

BOOL CCAPSImgApp::InitInstance() 
{
	CAPSInit();
	return CWinApp::InitInstance();
}

int CCAPSImgApp::ExitInstance() 
{
	CAPSExit();
	return CWinApp::ExitInstance();
}
