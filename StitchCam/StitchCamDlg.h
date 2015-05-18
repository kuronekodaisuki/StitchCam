
// StitchCamDlg.h : ヘッダー ファイル
//

#pragma once


// CStitchCamDlg ダイアログ
class CStitchCamDlg : public CDialogEx
{
// コンストラクション
public:
	CStitchCamDlg(CWnd* pParent = NULL);	// 標準コンストラクター

// ダイアログ データ
	enum { IDD = IDD_STITCHCAM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート


// 実装
protected:
	HICON m_hIcon;

	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
};
