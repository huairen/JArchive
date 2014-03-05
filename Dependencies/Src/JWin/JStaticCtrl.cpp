#include "JStaticCtrl.h"
#include "JApp.h"

bool JStaticCtrl::Create( JWinCtrl* pParent, int x, int y, int nWidth, int nHeight, const char* pTitle /*= NULL*/, int nStyle /*= 0*/ )
{
	m_hWnd = ::CreateWindow("STATIC", pTitle, WS_CHILD | WS_VISIBLE | nStyle, x, y, nWidth, nHeight,
		pParent ? pParent->GetWnd() : NULL, NULL, JApp::GetInstance(), NULL);

	if(m_hWnd == NULL)
		return false;

	m_pParent = pParent;
	return true;
}

void JStaticCtrl::AlignText( int nStyle )
{
	bool bResult;

	switch(nStyle)
	{
	case ALIGN_LEFT:
		bResult = AddStyle(SS_LEFT);
		break;
	case ALIGN_CENTER:
		bResult = AddStyle(SS_CENTER);
		break;
	case ALIGN_RIGHT:
		bResult = AddStyle(SS_RIGHT);
		break;
	}

	if(bResult)
		Invalidate();
}

void JStaticCtrl::SetTransparent(bool bValue)
{
	bool bResult;

	if(bValue)
		bResult = AddStyleEx(WS_EX_TRANSPARENT);
	else
		bResult = RemoveStyleEx(WS_EX_TRANSPARENT);

	if(bResult)
		Invalidate();
}

void JStaticCtrl::EraseBkgnd()
{
	HWND hWnd = ::GetParent(m_hWnd);
	if(hWnd != NULL)
	{
		RECT rc;
		::GetWindowRect(m_hWnd, &rc);

		POINT pos = {rc.left, rc.top};
		SIZE size = {rc.right-rc.left, rc.bottom-rc.top};
		::ScreenToClient(hWnd, &pos);

		rc.left = pos.x;
		rc.top = pos.y;
		rc.right = pos.x + size.cx;
		rc.bottom = pos.y + size.cy;

		::InvalidateRect(hWnd, &rc, TRUE);
	}
}
