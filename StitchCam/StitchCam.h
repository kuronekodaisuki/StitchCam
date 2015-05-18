
// StitchCam.h : PROJECT_NAME アプリケーションのメイン ヘッダー ファイルです。
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH に対してこのファイルをインクルードする前に 'stdafx.h' をインクルードしてください"
#endif

#include "resource.h"		// メイン シンボル


// CStitchCamApp:
// このクラスの実装については、StitchCam.cpp を参照してください。
//

class CStitchCamApp : public CWinApp
{
public:
	CStitchCamApp();

// オーバーライド
public:
	virtual BOOL InitInstance();

// 実装

	DECLARE_MESSAGE_MAP()
};

extern CStitchCamApp theApp;