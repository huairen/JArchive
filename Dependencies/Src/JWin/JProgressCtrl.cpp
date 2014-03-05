#include "JProgressCtrl.h"
#include "JApp.h"
#include <commctrl.h>

bool JProgressCtrl::Create( JWinCtrl* pParent, int x, int y, int nWidth, int nHeight, int nStyle /*= 0*/ )
{
	m_hWnd = ::CreateWindow(PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | nStyle, x, y, nWidth, nHeight,
		pParent ? pParent->GetWnd() : NULL, NULL, JApp::GetInstance(), NULL);

	if(m_hWnd == NULL)
		return false;

	m_pParent = pParent;
	return true;
}

void JProgressCtrl::SetRange( short nLower, short nUpper )
{
	SendMessage(PBM_SETRANGE, 0, MAKELPARAM(nLower, nUpper)); 
}

void JProgressCtrl::SetRange32( int nLower, short nUpper )
{
// 	m_nLower = nLower;
// 	m_nUpper = m_nUpper;
	SendMessage(PBM_SETRANGE32, nLower, nUpper); 
}

int JProgressCtrl::GetRangeLow()
{
	return SendMessage(PBM_GETRANGE, TRUE, 0);
}

int JProgressCtrl::GetRangeHigh()
{
	return SendMessage(PBM_GETRANGE, FALSE, 0);
}

int JProgressCtrl::SetPos( int nPos )
{
// 	int nOldPos = m_nPos;
// 	m_nPos = nPos;
// 	Invalidate(false);
	
	return SendMessage(PBM_SETPOS, (WPARAM) nPos, 0);
}

int JProgressCtrl::GetPos()
{
	return SendMessage(PBM_GETPOS, 0, 0);
}

int JProgressCtrl::StepIt()
{
	return SendMessage(PBM_STEPIT, 0, 0);
}

void JProgressCtrl::SetStep( int nStepInc )
{
	SendMessage(PBM_SETSTEP, nStepInc, 0);
}


//-------------------------------------------------------------------------------------------------

JBitmapProgressCtrl::JBitmapProgressCtrl()
{
	m_hBmp = NULL;
	m_nPos = 0;
	m_nMinLimit = 0;
	m_nMaxLimit = 100;
}

JBitmapProgressCtrl::~JBitmapProgressCtrl()
{
	if(m_hBmp != NULL)
		::DeleteObject(m_hBmp);
}

bool JBitmapProgressCtrl::Create( JWinCtrl* pParent, int x, int y, int nResID, HINSTANCE hInst/*=NULL*/)
{
	m_hBmp = ::LoadBitmap(hInst ? hInst : JApp::GetInstance(),MAKEINTRESOURCE(nResID));
	if(m_hBmp == NULL)
		return false;

	BITMAP bit;
	::GetObject(m_hBmp,sizeof(BITMAP),&bit);

	m_hWnd = ::CreateWindow("STATIC", NULL, WS_CHILD | WS_VISIBLE | SS_OWNERDRAW, x, y, bit.bmWidth, bit.bmHeight/2,
		pParent ? pParent->GetWnd() : NULL, NULL, JApp::GetInstance(), NULL);

	if(m_hWnd == NULL)
		return false;

	m_pParent = pParent;
	return ProcessMsg();
}

bool JBitmapProgressCtrl::OnEraseBkgnd(HDC hDC)
{
	return true;
}

void JBitmapProgressCtrl::DrawItem( LPDRAWITEMSTRUCT lpDrawItem )
{
	if(m_hBmp == NULL || m_hWnd == NULL || lpDrawItem == NULL)
		return;

	int nWidth = lpDrawItem->rcItem.right - lpDrawItem->rcItem.left;
	int nHeight = lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top;

	float fPos = (float)(m_nPos - m_nMinLimit) / (float)(m_nMaxLimit - m_nMinLimit);
	int nLenght = (int)(fPos * nWidth);

	HDC hSrcDc = ::CreateCompatibleDC(NULL);
	HBITMAP	hOldBmp = (HBITMAP)::SelectObject(hSrcDc, m_hBmp);

	::BitBlt(lpDrawItem->hDC, 0, 0, nLenght, nHeight, hSrcDc, 0, nHeight, SRCCOPY);
	::BitBlt(lpDrawItem->hDC, nLenght, 0, nWidth - nLenght, nHeight, hSrcDc, nLenght, 0, SRCCOPY);

	::SelectObject(hSrcDc, hOldBmp);
	::DeleteObject(hSrcDc);
}

void JBitmapProgressCtrl::SetPos( int nPos )
{
	m_nPos = nPos;
	Invalidate();
}
