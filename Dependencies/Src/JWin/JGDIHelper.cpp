#include "JGDIHelper.h"

Gdiplus::Image* JGDIHelper::LoadImageFormResource( HINSTANCE hInst, int nResID )
{
	HRSRC hRsrc = ::FindResource (hInst, MAKEINTRESOURCE(nResID), RT_BITMAP);
	if (hRsrc == NULL)
		return NULL;

	// load resource into memory
	DWORD dwResSize = SizeofResource(hInst, hRsrc);
	BYTE* lpRsrc = (BYTE*)LoadResource(hInst, hRsrc);
	if (lpRsrc == NULL)
		return NULL;

	// Allocate global memory on which to create stream
	HGLOBAL hMem = GlobalAlloc(GMEM_FIXED, dwResSize);
	BYTE* pMem = (BYTE*)GlobalLock(hMem);

	memcpy(pMem,lpRsrc,dwResSize);

	IStream* pStream;
	CreateStreamOnHGlobal(pMem,FALSE,&pStream);

	// load from stream
	Gdiplus::Image* pImage = Gdiplus::Image::FromStream(pStream);

	GlobalUnlock(pMem);
	pStream->Release();
	FreeResource(lpRsrc);

	return pImage;
}

void JGDIHelper::DrawAlphaImage( Gdiplus::Image* pImage, HWND hWnd, HDC hDc, int x, int y )
{
	static BLENDFUNCTION blendFunc = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

	int nWidth, nHeight;

	nWidth = pImage->GetWidth();
	nHeight = pImage->GetHeight();

	HDC hMemoryDc = ::CreateCompatibleDC(hDc);
	HBITMAP hBitmap = ::CreateCompatibleBitmap(hDc, nWidth, nHeight);
	HGDIOBJ hOldObj = ::SelectObject(hMemoryDc, hBitmap);

	Gdiplus::Graphics graphics(hMemoryDc);
	graphics.DrawImage(pImage, x, y, nWidth, nHeight);

	DWORD dwExStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
	if((dwExStyle & WS_EX_LAYERED) != WS_EX_LAYERED)
		SetWindowLong(hWnd, GWL_EXSTYLE, (dwExStyle | WS_EX_LAYERED));

	POINT destPos = {x, y};
	SIZE destSize = {nWidth, nHeight};
	POINT srcPos = {0, 0};

	//窗体样式 GWL_EXSTYLE 要设置 WS_EX_LAYERED
	UpdateLayeredWindow(hWnd, hDc, &destPos, &destSize, hMemoryDc, &srcPos, 0, &blendFunc, ULW_ALPHA);

	graphics.ReleaseHDC(hMemoryDc);
	::SelectObject(hMemoryDc,hOldObj);
	::DeleteObject(hMemoryDc);
	::DeleteObject(hBitmap);
}
