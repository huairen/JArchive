#include "JEditCtrl.h"
#include "JApp.h"

bool JEditCtrl::Create( JWinCtrl* pParent, int x, int y, int nWidth, int nHeight, const char* pTitle /*= NULL*/, int nStyle /*= 0*/ )
{
	m_hWnd = ::CreateWindow("EDIT", pTitle, WS_CHILD | WS_VISIBLE | nStyle, x, y, nWidth, nHeight,
		pParent ? pParent->GetWnd() : NULL, NULL, JApp::GetInstance(), NULL);

	if(m_hWnd == NULL)
		return false;

	m_pParent = pParent;
	return true;
}
