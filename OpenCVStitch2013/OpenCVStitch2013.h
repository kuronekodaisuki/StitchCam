
// OpenCVStitch2013.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// COpenCVStitch2013App:
// See OpenCVStitch2013.cpp for the implementation of this class
//

class COpenCVStitch2013App : public CWinApp
{
public:
	COpenCVStitch2013App();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern COpenCVStitch2013App theApp;