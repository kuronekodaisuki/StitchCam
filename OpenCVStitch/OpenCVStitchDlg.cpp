
// OpenCVStitchDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "OpenCVStitch.h"
#include "OpenCVStitchDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace cv;

extern "C"
{
#include <cuda_runtime_api.h>
#pragma comment(lib, "cudart.lib")
}

#ifdef _DEBUG
//#pragma comment(lib, "opencv_calib3d2410d.lib")
#pragma comment(lib, "opencv_core2410d.lib")
//#pragma comment(lib, "opencv_features2d2410d.lib")
//#pragma comment(lib, "opencv_flann2410d.lib")
#pragma comment(lib, "opencv_gpu2410d.lib")
#pragma comment(lib, "opencv_highgui2410d.lib")
#pragma comment(lib, "opencv_imgproc2410d.lib")
//#pragma comment(lib, "opencv_legacy2410d.lib")
//#pragma comment(lib, "opencv_ml2410d.lib")
//#pragma comment(lib, "opencv_nonfree2410d.lib")
//#pragma comment(lib, "opencv_objdetect2410d.lib")
//#pragma comment(lib, "opencv_ocl2410d.lib")
#pragma comment(lib, "opencv_stitching2410d.lib")
//#pragma comment(lib, "opencv_video2410d.lib")
#else
//#pragma comment(lib, "opencv_calib3d2410.lib")
#pragma comment(lib, "opencv_core2410.lib")
#pragma comment(lib, "opencv_features2d2410.lib")
//#pragma comment(lib, "opencv_flann2410.lib")
#pragma comment(lib, "opencv_gpu2410.lib")
#pragma comment(lib, "opencv_highgui2410.lib")
#pragma comment(lib, "opencv_imgproc2410.lib")
//#pragma comment(lib, "opencv_legacy2410.lib")
//#pragma comment(lib, "opencv_ml2410.lib")
//#pragma comment(lib, "opencv_nonfree2410.lib")
//#pragma comment(lib, "opencv_objdetect2410.lib")
//#pragma comment(lib, "opencv_ocl2410.lib")
#pragma comment(lib, "opencv_stitching2410.lib")
//#pragma comment(lib, "opencv_video2410.lib")
#endif


// CStitchCamDlg ダイアログ
#define TIMER_STITCH	1
#define TIMER_PREVIEW	2

const float WIDTH = 640.0;
const float HEIGHT = 480.0;
const float FPS = 30.0;
const char STITCHED[] = "Stitched";

// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// COpenCVStitchDlg ダイアログ




COpenCVStitchDlg::COpenCVStitchDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COpenCVStitchDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COpenCVStitchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CAM1, m_cam1);
	DDX_Control(pDX, IDC_CAM2, m_cam2);
	DDX_Control(pDX, IDC_CAM3, m_cam3);
	DDX_Control(pDX, IDC_CAM4, m_cam4);
	DDX_Control(pDX, IDC_WIDE, m_qvga);
	DDX_Control(pDX, IDC_SAVE, m_save);
}

BEGIN_MESSAGE_MAP(COpenCVStitchDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_PREVIEW, &COpenCVStitchDlg::OnBnClickedPreview)
	ON_BN_CLICKED(IDC_CARIBRATE, &COpenCVStitchDlg::OnBnClickedCaribrate)
	ON_BN_CLICKED(IDC_STOP, &COpenCVStitchDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_START, &COpenCVStitchDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_FILE, &COpenCVStitchDlg::OnBnClickedFile)
END_MESSAGE_MAP()

CWnd *pDlg;

// enum camera callback
static void callback(void *pItem, char *pDeviceName)
{
	char buffer[80] = "";
	size_t wlen;
	int *idcCam = (int*)pItem;
	if (*idcCam <= IDC_CAM4) {
		CButton *item = (CButton*)pDlg->GetDlgItem(*idcCam);

		item->EnableWindow();
		item->SetCheck(BST_CHECKED);
		item->ShowWindow(SW_SHOWNORMAL);
		//mbstowcs_s(&wlen, buffer, pDeviceName, 80);
		strcpy(buffer, pDeviceName);
		// Buffaloのカメラの行末にCR,LFが入っているので、これを除去
		for (wlen = strlen(buffer) - 1; 0 < wlen; wlen--) {
			if (buffer[wlen] == 13 || buffer[wlen] == 10)
				buffer[wlen] = 0;
		}
		item->SetWindowText(buffer);
	}
	(*idcCam)++;
}

// COpenCVStitchDlg メッセージ ハンドラー

BOOL COpenCVStitchDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。
	int count = 0;
	if (cudaSuccess == cudaGetDeviceCount(&count) && 0 < count)
	{
		stitcher = StitchImage::createDefault(true);	// CUDAが使用できる場合
		CWnd *pStatus = GetDlgItem(IDC_STATUS);
		char buffer[80];
		sprintf(buffer, "%d CUDA device detected.", count);
		pStatus->SetWindowText(buffer);
	}
	else
	{
		stitcher = StitchImage::createDefault(false);	// CUDAが使用できない場合
		CWnd *pStatus = GetDlgItem(IDC_STATUS);
		pStatus->SetWindowText("No CUDA device detected.");
	}
	pDlg = this;
	int idcCam = IDC_CAM1;
	count = WebCam::EnumDevices((PENUMDEVICE)&callback, &idcCam);

	fileOpened = false;

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void COpenCVStitchDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void COpenCVStitchDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR COpenCVStitchDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void COpenCVStitchDlg::OnBnClickedPreview()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
}


void COpenCVStitchDlg::OnBnClickedCaribrate()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
}


void COpenCVStitchDlg::OnBnClickedStart()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
}


void COpenCVStitchDlg::OnBnClickedStop()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
}


void COpenCVStitchDlg::OnBnClickedFile()
{
	// TODO: ここにコントロール通知ハンドラー コードを追加します。
}
