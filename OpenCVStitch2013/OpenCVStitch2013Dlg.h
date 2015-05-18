
// OpenCVStitch2013Dlg.h : header file
//

#pragma once


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

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
