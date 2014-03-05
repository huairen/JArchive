#include "JButtonCtrl.h"
#include "JApp.h"

JButtonCtrl::JButtonCtrl()
{
	m_hBmp = NULL;
	m_bTrack = false;
}

JButtonCtrl::~JButtonCtrl()
{
	if(m_hBmp != NULL)
		::DeleteObject(m_hBmp);
}

bool JButtonCtrl::Create( JWinCtrl* pParent, int x, int y, int nWidth, int nHeight, const char* pTitle, int nStyle )
{
	m_hWnd = ::CreateWindow("BUTTON", pTitle, WS_CHILD | WS_VISIBLE | nStyle, x, y, nWidth, nHeight,
		pParent ? pParent->GetWnd() : NULL, NULL, JApp::GetInstance(), NULL);

	if(m_hWnd == NULL)
		return false;

	m_pParent = pParent;
	return true;
}

bool JButtonCtrl::LoadBitmap( int nResID, HINSTANCE hInst/*=NULL*/ )
{
	assert(::IsWindow(m_hWnd));

	if(m_hBmp != NULL)
	{
		::DeleteObject(m_hBmp);
		m_hBmp = NULL;
	}

	m_hBmp = ::LoadBitmap(hInst ? hInst : JApp::GetInstance(),MAKEINTRESOURCE(nResID));
	if(m_hBmp == NULL)
		return false;

	BITMAP bit;
	::GetObject(m_hBmp,sizeof(BITMAP),&bit);
	m_nWidth = bit.bmWidth;
	m_nHeight = bit.bmHeight/4;

	SetExtent(m_nWidth,m_nHeight);

 	AddStyle(BS_OWNERDRAW);

	return ProcessMsg();
}

bool JButtonCtrl::OnEraseBkgnd( HDC hDC )
{
	return true;
}

void JButtonCtrl::DrawItem( LPDRAWITEMSTRUCT lpDrawItem )
{
	if(m_hBmp == NULL || m_hWnd == NULL || lpDrawItem == NULL)
		return;

	HDC hSrcDc = ::CreateCompatibleDC(NULL);
	HBITMAP	hOldBmp = (HBITMAP)::SelectObject(hSrcDc, m_hBmp);

	UINT state = lpDrawItem->itemState;
	
	if(state & ODS_SELECTED)
		state = 2;
	else if(state & ODS_DISABLED)
		state = 3;
	else
		state = m_bTrack ? 1 : 0;

	::BitBlt(lpDrawItem->hDC, 0, 0, m_nWidth, m_nHeight, hSrcDc, 0, state * m_nHeight, SRCCOPY);
	::SelectObject(hSrcDc, hOldBmp);
	::DeleteObject(hSrcDc);
}

bool JButtonCtrl::OnMouseMove( UINT fwKeys, POINTS pos )
{
	if(m_hBmp != NULL && !m_bTrack)
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.hwndTrack = m_hWnd;
		tme.dwFlags = TME_LEAVE | TME_HOVER;
		tme.dwHoverTime = 10;
		m_bTrack = (TRUE == TrackMouseEvent(&tme));
	}

	return false;
}

bool JButtonCtrl::OnMouseHover()
{
	Invalidate();
	return false;
}

bool JButtonCtrl::OnMouseLeave()
{
	m_bTrack = false;
	Invalidate();
	return false;
}
