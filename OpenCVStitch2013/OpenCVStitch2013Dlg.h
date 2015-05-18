
// OpenCVStitch2013Dlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "..\MyStitcher.h"
#include "..\WebCam.h"

using namespace cv;

#define AVERAGE_SIZE	10

// COpenCVStitch2013Dlg dialog
class COpenCVStitch2013Dlg : public CDialogEx
{
// Construction
public:
	COpenCVStitch2013Dlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_OPENCVSTITCH2013_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
	MyStitcher stitcher;
	vector<WebCam> camera;
	UINT_PTR pTimer;
	double value[AVERAGE_SIZE];
	int iValue;

	void UpdateValue(double value);
	void DoOpen();
	void DoStitch();
	void DoPreview();

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnTimer(UINT_PTR nIDEvent);
public:
	CButton m_cam1;
	CButton m_cam2;
	CButton m_cam3;
	CButton m_cam4;
	CButton m_oneshot;
	CButton m_start;
	CStatic m_status;
	afx_msg void OnBnClickedPreview();
	afx_msg void OnBnClickedOneshot();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedCam1();
	afx_msg void OnBnClickedCam2();
	afx_msg void OnBnClickedCam3();
	afx_msg void OnBnClickedCam4();
};
