
// OpenCVStitch2013Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "OpenCVStitch2013.h"
#include "OpenCVStitch2013Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define TIMER_STITCH	1
#define TIMER_PREVIEW	2

const float WIDTH = 640.0;
const float HEIGHT = 480.0;
const float FPS = 30.0;
const char STITCHED[] = "Stitched";

// COpenCVStitch2013Dlg dialog



COpenCVStitch2013Dlg::COpenCVStitch2013Dlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COpenCVStitch2013Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COpenCVStitch2013Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CAM1, m_cam1);
	DDX_Control(pDX, IDC_CAM2, m_cam2);
	DDX_Control(pDX, IDC_CAM3, m_cam3);
	DDX_Control(pDX, IDC_CAM4, m_cam4);
	DDX_Control(pDX, IDC_ONESHOT, m_oneshot);
	DDX_Control(pDX, IDOK, m_start);
	DDX_Control(pDX, IDC_STATUS, m_status);
}

BEGIN_MESSAGE_MAP(COpenCVStitch2013Dlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_PREVIEW, &COpenCVStitch2013Dlg::OnBnClickedPreview)
	ON_BN_CLICKED(IDC_ONESHOT, &COpenCVStitch2013Dlg::OnBnClickedOneshot)
	ON_BN_CLICKED(IDOK, &COpenCVStitch2013Dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_STOP, &COpenCVStitch2013Dlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_CAM1, &COpenCVStitch2013Dlg::OnBnClickedCam1)
	ON_BN_CLICKED(IDC_CAM2, &COpenCVStitch2013Dlg::OnBnClickedCam2)
	ON_BN_CLICKED(IDC_CAM3, &COpenCVStitch2013Dlg::OnBnClickedCam3)
	ON_BN_CLICKED(IDC_CAM4, &COpenCVStitch2013Dlg::OnBnClickedCam4)
END_MESSAGE_MAP()

CWnd *pDlg;

// enum camera callback
static void callback(void *pItem, char *pDeviceName)
{
	TCHAR buffer[80] = L"";
	size_t wlen;
	int *idcCam = (int*)pItem;
	if (*idcCam <= IDC_CAM4) {
		CButton *item = (CButton*)pDlg->GetDlgItem(*idcCam);

		item->EnableWindow();
		item->SetCheck(BST_CHECKED);
		item->ShowWindow(SW_SHOWNORMAL);
		mbstowcs_s(&wlen, buffer, pDeviceName, 80);
		// Buffaloのカメラの行末にCR,LFが入っているので、これを除去
		for (wlen--; 0 < wlen; wlen--) {
			if (buffer[wlen] == 13 || buffer[wlen] == 10)
				buffer[wlen] = 0;
		}
		item->SetWindowText(buffer);
	}
	(*idcCam)++;
}

// COpenCVStitch2013Dlg message handlers

BOOL COpenCVStitch2013Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	stitcher = cv::MyStitcher::createDefault(true);

	int count;
	if ((count = gpu::getCudaEnabledDeviceCount()) > 0)
	{
		stitcher = MyStitcher::createDefault(true);	// CUDAが使用できる場合
		stitcher.setBlender(new detail::MyBlender());
		stitcher.setExposureCompensator(new detail::MyCompensator(true));
		//stitcher.setSeamFinder(new detail::MySeamFinder(detail::MySeamFinder::COLOR));
		stitcher.setSeamFinder(new detail::DpSeamFinder());
		//stitcher.setSeamFinder(new detail::NoSeamFinder());

		CString buffer;
		buffer.Format(L"%d CUDA device detected.", count);
		m_status.SetWindowText(buffer);
		//TRACE(buffer + "\n");
	}
	else
	{
		stitcher = MyStitcher::createDefault();	// CUDAが使用できない場合
		stitcher.setBlender(new detail::MyBlender());
		stitcher.setExposureCompensator(new detail::MyCompensator());
		//stitcher.setWarper(new MyCylindricalWarperGpu());
		m_status.SetWindowText(L"No CUDA device detected.");
	}
	pDlg = this;
	int idcCam = IDC_CAM1;
	count = WebCam::EnumDevices((PENUMDEVICE)&callback, &idcCam);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void COpenCVStitch2013Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR COpenCVStitch2013Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 平均値を算出して表示
void COpenCVStitch2013Dlg::UpdateValue(double v)
{
	CString buffer;
	double sum = 0;
	value[iValue] = v;
	iValue = (iValue + 1) % AVERAGE_SIZE;

	for (int i = 0; i < AVERAGE_SIZE; i++)
	{
		sum += value[i];
	}
	buffer.Format(L"%.2fmsec", sum / AVERAGE_SIZE);
	m_status.SetWindowText(buffer);
}


// カメラ開始
void COpenCVStitch2013Dlg::DoOpen()
{
	//if (fileOpened)
	//{
	//	return;
	//}
	camera.clear();
	for (int id = IDC_CAM1; id <= IDC_CAM4; id++)
	{
		CButton *item = (CButton*)GetDlgItem(id);
		if (item->GetCheck() == BST_CHECKED)
		{
			WCHAR	wname[80];
			char name[80];// , buffer[100];
			size_t len;
			bool res;
			item->GetWindowText(wname, 80);
			wcstombs_s(&len, name, 80, wname, 80);

			WebCam cam = WebCam();
			res = cam.open(id - IDC_CAM1);
			if (res) {
				CString buffer;
				res = cam.set(CV_CAP_PROP_FRAME_WIDTH, WIDTH);
				res = cam.set(CV_CAP_PROP_FRAME_HEIGHT, HEIGHT);
				buffer.Format(L"%d %s", id - IDC_CAM1 + 1, name);
				res = cam.set(CV_CAP_PROP_FPS, FPS);
				res = cam.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
				cam.SetName(name);
				camera.push_back(cam);
			}
		}
	}
}

void COpenCVStitch2013Dlg::DoPreview()
{
	for (size_t i = 0; i < camera.size(); i++)
	{
		Mat image;
		camera[i] >> image;
		if (image.empty() == false) {
			imshow(camera[i].GetName(), image);
		}
	}
}

void COpenCVStitch2013Dlg::DoStitch()
{
	Mat stitched;
	vector<Mat> images;

	for (size_t i = 0; i < camera.size(); i++)
	{
		Mat image;
		camera[i] >> image;

		imshow(camera[i].GetName(), image);
		images.push_back(image);
	}
	if (2 <= camera.size())
	{
		int64 t = getTickCount();
		if (MyStitcher::Status::OK == stitcher.composePanorama(images, stitched)) {
			UpdateValue((getTickCount() - t) / getTickFrequency() * 1000);
			imshow(STITCHED, stitched);
			//if (writer.isOpened())
			//{
			//	writer << stitched;
			//}
		}
		else {
			MessageBox(L"内部処理でのエラー");
		}
		images.clear();
	}
	else
	{
		MessageBox(L"カメラが少ないためキャリブレートできません");
	}
}

void COpenCVStitch2013Dlg::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case TIMER_STITCH:
		DoStitch();
		break;

	case TIMER_PREVIEW:
		DoPreview();
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void COpenCVStitch2013Dlg::OnBnClickedPreview()
{
	DoOpen();
	pTimer = SetTimer(TIMER_PREVIEW, 30, NULL);	// 30msec
}


void COpenCVStitch2013Dlg::OnBnClickedOneshot()
{
	DoOpen();
	Mat stitched;
	vector<Mat> images;
	// 
	for (size_t i = 0; i < camera.size(); i++)
	{
		Mat image;
		camera[i] >> image;
		if (image.empty() == false) {
			imshow(camera[i].GetName(), image);
			images.push_back(image);
		}
	}
	if (2 <= camera.size())
	{
		stitcher.estimateTransform(images);
		if (MyStitcher::Status::OK == stitcher.composePanorama(images, stitched)) {
			//imageSize = Size(stitched.cols, stitched.rows);
			imshow(STITCHED, stitched);
			m_start.EnableWindow();
		}
		else {
			MessageBox(L"内部処理でのエラー", L"キャリブレート");
		}
	}
	else
	{
		MessageBox(L"カメラが少ないためキャリブレートできません");
	}
}


void COpenCVStitch2013Dlg::OnBnClickedOk()
{
	if (2 <= camera.size())
	{
		/*
		if (m_save.GetCheck() == BST_CHECKED)
		{
			static TCHAR BASED_CODE szFilter[] = _T("AVI (*.avi)|*.avi|All Files (*.*)|*.*||");

			CFileDialog dlg(FALSE, L"avi", L"save.avi", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
			if (dlg.DoModal() == IDOK)
			{
				char name[80];
				size_t len;
				int fourcc = CV_FOURCC('I', 'Y', 'U', 'V');	//CV_FOURCC('M','J','P','G');	// Motion JPEG

				CString filename = dlg.GetFileName();
				wcstombs_s(&len, name, 80, (LPCWSTR)filename, 80);
				//if (writer.open(name, fourcc, FPS / 2, imageSize) != true)
				//{
				//	MessageBox(L"Cant save");
				//}
			}
			else
			{
				return;
			}
		}
		*/
		pTimer = SetTimer(TIMER_STITCH, 30, NULL);	// 30msec
	}
	else
	{
		MessageBox(L"カメラが少ないためステッチできません");
	}
}


void COpenCVStitch2013Dlg::OnBnClickedStop()
{
	KillTimer(pTimer);
	destroyAllWindows();
	for (size_t i = 0; i < camera.size(); i++)
	{
		//	camera[i].release();
	}
	//if (writer.isOpened())
	//{
	//	writer.release();
	//}
}


void COpenCVStitch2013Dlg::OnBnClickedCam1()
{
	m_start.EnableWindow(FALSE);
}


void COpenCVStitch2013Dlg::OnBnClickedCam2()
{
	m_start.EnableWindow(FALSE);
}


void COpenCVStitch2013Dlg::OnBnClickedCam3()
{
	m_start.EnableWindow(FALSE);
}


void COpenCVStitch2013Dlg::OnBnClickedCam4()
{
	m_start.EnableWindow(FALSE);
}
