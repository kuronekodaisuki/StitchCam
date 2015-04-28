
// OpenCVStitch.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// COpenCVStitchApp:
// このクラスの実装については、OpenCVStitch.cpp を参照してください。
//

class COpenCVStitchApp : public CWinApp
{
public:
	COpenCVStitchApp();

// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern COpenCVStitchApp theApp;