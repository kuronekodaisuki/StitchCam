//
// Camera enumerate
//

#include "WebCam.h"

#ifdef _MSC_VER
extern "C"
{
#pragma comment(lib, "strmiids.lib")
}

#ifndef SAFE_RELEASE
#define SAFE_RELEASE( p ) if(p)p->Release()
#endif

int WebCam::EnumDevices(PENUMDEVICE pCallback, void* pItem)
{
	int nDevice = 0;
	HRESULT			hr				= S_OK;

	// System device enumerators
	ICreateDevEnum	*pDevEnum		= NULL;
	IEnumMoniker	*pEnum			= NULL;
	IMoniker		*pMoniker		= NULL;

	CoInitialize(NULL);

	// Create system device enumerator
	hr = CoCreateInstance( CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, reinterpret_cast<void**>(&pDevEnum) );
	if (FAILED(hr)) {
		return 0;
	}

	// create enumerator for video capture devices
	hr = pDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &pEnum, 0 );
	if (SUCCEEDED(hr)) 
	{
		IBindCtx* pbcfull = NULL;

		// Enumerate through the devices and print friendly name
		while ( (pEnum->Next( 1, &pMoniker, NULL ) == S_OK))
		{	
			VARIANT var;
			IPropertyBag *pPropertyBag;
			char szCameraName[200];

			// FrienlyName : The name of the device
			var.vt = VT_BSTR;

			// Bind to IPropertyBag
			pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropertyBag);
			
			// Reference: https://msdn.microsoft.com/en-us/library/windows/desktop/dd377566(v=vs.85).aspx
			hr = pPropertyBag->Read(L"Description", &var, 0);	// "FriendlyName", "DevicePath", "Description"
			if (FAILED(hr))
			{
				pPropertyBag->Read(L"FriendlyName", &var, 0);
			}
			WideCharToMultiByte(CP_ACP, 0, var.bstrVal, -1,szCameraName, sizeof(szCameraName), 0, 0);
			if (pCallback != NULL) {
				(*pCallback)(pItem, szCameraName);
			}
			VariantClear(&var);

			// Release resource
			pPropertyBag->Release();

			nDevice++;

			SAFE_RELEASE( pMoniker );
		}
	}
	return nDevice; // returns count of camera
}
#else	// Other plathome 
#include <stdio.h>
#include <string.h>
#include <dirent.h>	// POSIX directory 
int WebCam::EnumDevices(PENUMDEVICE pCallback, void* pItem)
{
	int count = 0;
	DIR *d;

	d = opendir("/dev");
	if (d != NULL)
	{
		struct dirent *dir;
		while ((dir = readdir(d)) != NULL)
		{
			if (strstr(dir->d_name, "video"))
			{
				printf("%s\n", dir->d_name);
				count++;
			}
		}
		closedir(d);
	}
	return count;
}
#endif

/*
* Open with specific mode
*/
#if 0
bool WebCam::open(const string& filename, int mode)
{
    if (isOpened()) release();
	cap = 0;
	 switch (mode)
	 {
#ifdef HAVE_VFW
	 case CV_CAP_VFW:
        cap = cvCreateFileCapture_VFW (filename);
		break;
#endif

#ifdef HAVE_MSMF
	 case CV_CAP_MSMF:
        cap = cvCreateFileCapture_MSMF (filename);
		break;
#endif


#if defined(HAVE_QUICKTIME) || defined(HAVE_QTKIT)
	 case CV_CAP_QT:
        cap = cvCreateFileCapture_QT (filename);
		break;
#endif

#ifdef HAVE_AVFOUNDATION
	 case CV_CAP_AVFOUNDATION:
        cap = cvCreateFileCapture_AVFoundation (filename);
		break;
#endif

#ifdef HAVE_OPENNI
	 case CV_CAP_OPENNI:
        cap = cvCreateFileCapture_OpenNI (filename);
		break;
#endif

	 default:
#ifdef HAVE_FFMPEG
        cap = cvCreateFileCapture_FFMPEG_proxy (filename);
#endif
	/*
    if (! result)
        result = cvCreateFileCapture_Images (filename);

	 }
	 */
	 }
    return isOpened();
}
#endif
