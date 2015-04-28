
// OpenCVStitchDlg.h : ヘッダー ファイル
//

#pragma once
#include "afxwin.h"

#include <opencv2/core/core.hpp>
#include <opencv2/stitching/stitcher.hpp>
#include <cuda_runtime_api.h>

#include "..\WebCam.h"
#include "StitchImage.h"

using namespace cv;


// COpenCVStitchDlg ダイアログ
class COpenCVStitchDlg : public CDialogEx
{
// コンストラクション
public:
	COpenCVStitchDlg(CWnd* pParent = NULL);	// 標準コンストラクター

// ダイアログ データ
	enum { IDD = IDD_OPENCVSTITCH_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート


// 実装
protected:
	HICON m_hIcon;

	StitchImage stitcher;	// Customised cv::Stitcher
	vector<WebCam> camera;
	VideoWriter writer;
	Size imageSize;
	UINT_PTR pTimer;
	bool fileOpened;

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CButton m_cam1;
	CButton m_cam2;
	CButton m_cam3;
	CButton m_cam4;
	CButton m_qvga;
	CButton m_save;
	afx_msg void OnBnClickedPreview();
	afx_msg void OnBnClickedCaribrate();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedStart();
	afx_msg void OnBnClickedFile();
};
