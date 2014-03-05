#include "JDlgCtrl.h"
#include "JApp.h"

JDlgCtrl::JDlgCtrl(int nResID)
{
	m_bModalFlg = false;
	m_nResID = nResID;
}

JDlgCtrl::~JDlgCtrl(void)
{
	EndDialog(0);
}

bool JDlgCtrl::Create( JWinCtrl* pParent, HINSTANCE hInst )
{
	JApp::GetApp()->AddControl(this);

	m_pParent = pParent;

	m_hWnd = ::CreateDialog(hInst ? hInst : JApp::GetInstance(), MAKEINTRESOURCE(m_nResID),
		m_pParent ? m_pParent->GetWnd() : NULL, (DLGPROC)JApp::WinProc);

	if(m_hWnd != NULL)
		return true;

	JApp::GetApp()->AddControl(NULL);
	return false;
}

void JDlgCtrl::EndDialog( int nResult )
{
	if(::IsWindow(m_hWnd))
	{
		if(m_bModalFlg)
			::EndDialog(m_hWnd,nResult);
		else
			::DestroyWindow(m_hWnd);
	}
}

int JDlgCtrl::Exec(JWinCtrl* pParent)
{
	JApp::GetApp()->AddControl(this);

	m_pParent = pParent;

	m_bModalFlg = true;
	int result = ::DialogBox(JApp::GetInstance(), MAKEINTRESOURCE(m_nResID),
		m_pParent ? m_pParent->GetWnd() : NULL, (DLGPROC)JApp::WinProc);
	m_bModalFlg = false;

	return result;
}

bool JDlgCtrl::PreProcMsg( MSG *msg )
{
	if (m_hAccel && ::TranslateAccelerator(m_hWnd, m_hAccel, msg))
		return true;

	if (!m_bModalFlg)
		return (TRUE == ::IsDialogMessage(m_hWnd, msg));

	return	false;
}

LRESULT JDlgCtrl::WinProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	LRESULT	result = 0;

	switch(uMsg)
	{
	case WM_INITDIALOG:
		return OnCreate(lParam);
	case WM_CLOSE:
		OnClose();
		return 0;
	case WM_DESTROY:
		OnDestroy();
		return 0;
	case WM_NCDESTROY:
		JApp::GetApp()->DelControl(this);
		m_hWnd = 0;
		return 0;
	case WM_NOTIFY:
		result = OnNotify((UINT)wParam, (LPNMHDR)lParam);
		::SetWindowLong(m_hWnd, DWL_MSGRESULT, result);
		return	result;
	case WM_SYSCOMMAND:
		OnSysCommand(wParam,MAKEPOINTS(lParam));
		return 0;
	case WM_COMMAND:
		OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
		return 0;
	case WM_CONTEXTMENU:
		result = OnContextMenu((HWND)wParam,MAKEPOINTS(lParam));
		::SetWindowLong(m_hWnd, DWL_MSGRESULT, result);
		return result;
	case WM_DROPFILES:
		OnDropFiles((HDROP)wParam);
		return 0;
	case WM_TIMER:
		OnTimer(wParam, (TIMERPROC)lParam);
		return	0;
	case WM_LBUTTONDOWN:
		OnButton(uMsg, (int)wParam, MAKEPOINTS(lParam));
		return 0;
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORSTATIC:
		OnCtlColor(uMsg, (HDC)wParam, (HWND)lParam, (HBRUSH *)&result);
		::SetWindowLong(m_hWnd,DWL_MSGRESULT, result);
		return	result;
	case WM_ERASEBKGND:
		return OnEraseBkgnd((HDC)wParam);
	case WM_PAINT:
		OnPaint();
		return 0;
	case WM_DRAWITEM:
		result = OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
		::SetWindowLong(m_hWnd,DWL_MSGRESULT, result);
		return 0;
	case WM_MOVE:
		OnMove(LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_SIZE:
		OnSize((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	default:
		if (uMsg >= WM_APP && uMsg <= 0xBFFF)
			result = OnAppEvent(uMsg, wParam, lParam);
		else if (uMsg >= WM_USER && uMsg < WM_APP || uMsg >= 0xC000 && uMsg <= 0xFFFF)
			result = OnUserEvent(uMsg, wParam, lParam);
		::SetWindowLong(m_hWnd,DWL_MSGRESULT,result);
		return result;
	}

	return false;
}
