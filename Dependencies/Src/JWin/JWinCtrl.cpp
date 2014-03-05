#include "JWinCtrl.h"
#include "JApp.h"

JWinCtrl::JWinCtrl()
{
	m_hWnd = NULL;
	m_hAccel = NULL;
	m_pOldProc = NULL;
	m_pParent = NULL;
	m_szDefClassName[0] = 0;
}

JWinCtrl::~JWinCtrl(void)
{
	if(m_pOldProc != NULL)
	{
		::SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG_PTR)m_pOldProc);
		m_pOldProc = NULL;
	}
}

bool JWinCtrl::Create( LPCSTR className, LPCSTR title, DWORD style, JWinCtrl* pParent, HMENU hMenu)
{
	if(className == NULL)
		className = GetDefaultClass();

	m_pParent = pParent;

	JApp::GetApp()->AddControl(this);

	m_hWnd = ::CreateWindow(className,title, style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		m_pParent ? m_pParent->GetWnd() : NULL, hMenu, JApp::GetInstance(), NULL);

	if(m_hWnd != NULL)
		return true;

	JApp::GetApp()->AddControl(NULL);
	return false;
}

void JWinCtrl::Destroy()
{
	if(::IsWindow(m_hWnd))
	{
		::DestroyWindow(m_hWnd);
		m_hWnd = NULL;
	}
}

void JWinCtrl::Show( int mode )
{
	::ShowWindow(m_hWnd,mode);
	::UpdateWindow(m_hWnd);
}

void JWinCtrl::ShowWindow( int mode /*= SW_SHOWDEFAULT*/ )
{
	::ShowWindow(m_hWnd,mode);
}

void JWinCtrl::ShowChlid( int nID, int mode /*= SW_SHOWDEFAULT*/ )
{
	HWND hWnd = ::GetDlgItem(m_hWnd,nID);
	::ShowWindow(hWnd,mode);
}

bool JWinCtrl::EnableWindow( bool bEnable )
{
	return (TRUE == ::EnableWindow(m_hWnd,bEnable));
}

bool JWinCtrl::SetForegroundWindow()
{
	return (TRUE == ::SetForegroundWindow(m_hWnd));
}

void JWinCtrl::MoveToCenter()
{
	if(m_hWnd == NULL)
		return;

	HWND deskWnd = ::GetParent(m_hWnd);
	if(deskWnd == NULL)
		deskWnd = GetDesktopWindow();

	RECT deskRect,dlgRect;

	GetWindowRect(deskWnd,&deskRect);
	GetWindowRect(m_hWnd,&dlgRect);

	int dlgWidth = dlgRect.right - dlgRect.left;
	int dlgHeight = dlgRect.bottom - dlgRect.top;

	int dlgNewLeft = (deskRect.right-deskRect.left-dlgWidth)/2 + deskRect.left;
	int dlgNewTop = (deskRect.bottom-deskRect.top-dlgHeight)/2 + deskRect.top;

	SetPos(dlgNewLeft,dlgNewTop);
}

void JWinCtrl::SetPos( int x, int y )
{
	assert(::IsWindow(m_hWnd));
	::SetWindowPos(m_hWnd,NULL,x,y,0,0,SWP_NOZORDER|SWP_NOSIZE);	
}

void JWinCtrl::SetExtent( int nWidth, int nHeight )
{
	assert(::IsWindow(m_hWnd));
	::SetWindowPos(m_hWnd,NULL,0,0,nWidth,nHeight,SWP_NOZORDER|SWP_NOMOVE);	
}

void JWinCtrl::MoveWindow( int x, int y, int nWidth, int nHeight,bool bRepaint /*= TRUE*/ )
{
	assert(::IsWindow(m_hWnd));
	::MoveWindow(m_hWnd, x, y, nWidth, nHeight, bRepaint);
}

void JWinCtrl::MoveWindow( const RECT& rect, bool bRepaint /*= TRUE*/ )
{
	assert(::IsWindow(m_hWnd));
	::MoveWindow(m_hWnd, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, bRepaint);
}



bool JWinCtrl::PostMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return (TRUE == ::PostMessage(m_hWnd,uMsg,wParam,lParam));
}

LRESULT JWinCtrl::SendMessage( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return ::SendMessage(m_hWnd,uMsg,wParam,lParam);
}

LRESULT JWinCtrl::SendDlgItemMessage( int nIDDlgItem, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return ::SendDlgItemMessage(m_hWnd,nIDDlgItem,uMsg,wParam,lParam);
}


bool JWinCtrl::SetWindowText( const char* pText )
{
	return (TRUE == ::SetWindowText(m_hWnd,pText));
}

int JWinCtrl::GetWindowText( char* pText, int nSize )
{
	return ::GetWindowText(m_hWnd, pText, nSize);
}

bool JWinCtrl::SetDlgItemText( int nIDDlgItem, const char* pText )
{
	return (TRUE == ::SetDlgItemText(m_hWnd, nIDDlgItem, pText));
}

int JWinCtrl::GetDlgItemText( int nIDDlgItem, char* pText, int nSize )
{
	return ::GetDlgItemText(m_hWnd, nIDDlgItem, pText, nSize);
}


bool JWinCtrl::AddStyle( UINT nStyle )
{
	if(m_hWnd == NULL)
		return false;

	DWORD dwOldStyle = ::GetWindowLong(m_hWnd,GWL_STYLE);
	if((dwOldStyle & nStyle) == nStyle)
		return false;

	return (::SetWindowLong(m_hWnd, GWL_STYLE, dwOldStyle | nStyle) != 0) ? true : false;
}

bool JWinCtrl::RemoveStyle( UINT nStyle )
{
	if(m_hWnd == NULL)
		return false;

	DWORD dwOldStyle = ::GetWindowLong(m_hWnd,GWL_STYLE);
	if((dwOldStyle & nStyle) != nStyle)
		return false;

	return (::SetWindowLong(m_hWnd, GWL_STYLE, dwOldStyle & ~nStyle) != 0) ? true : false;
}

bool JWinCtrl::AddStyleEx( UINT nStyle )
{
	if(m_hWnd == NULL)
		return false;

	DWORD dwOldStyle = ::GetWindowLong(m_hWnd,GWL_EXSTYLE);
	if((dwOldStyle & nStyle) == nStyle)
		return false;

	return (::SetWindowLong(m_hWnd, GWL_EXSTYLE, dwOldStyle | nStyle) != 0) ? true : false;
}

bool JWinCtrl::RemoveStyleEx( UINT nStyle )
{
	if(m_hWnd == NULL)
		return false;

	DWORD dwOldStyle = ::GetWindowLong(m_hWnd,GWL_EXSTYLE);
	if((dwOldStyle & nStyle) != nStyle)
		return false;

	return (::SetWindowLong(m_hWnd, GWL_EXSTYLE, dwOldStyle & ~nStyle) != 0) ? true : false;
}



void JWinCtrl::DragAcceptFiles( bool bAccept )
{
	if(m_hWnd != NULL)
		::DragAcceptFiles(m_hWnd, bAccept);
}

bool JWinCtrl::SetIcon( int nResID )
{
	HICON hIcon = ::LoadIcon(JApp::GetInstance(), MAKEINTRESOURCE(nResID));
	return (0 != ::SetClassLong(m_hWnd, GCL_HICON, (LONG)hIcon));
}

int JWinCtrl::SetCtrlID( int nID )
{
	assert(::IsWindow(m_hWnd));
	return (int)::SetWindowLong(m_hWnd, GWL_ID, nID);
}

bool JWinCtrl::SetBackgroundBitmap( int nResID, bool bChangeSize /*= true*/ )
{
	HBITMAP hBmp = ::LoadBitmap(JApp::GetInstance(),MAKEINTRESOURCE(nResID));
	if(hBmp == NULL)
		return false;

	if(bChangeSize)
	{
		BITMAP bmp;
		::GetObject(hBmp, sizeof(BITMAP), &bmp);
		SetExtent(bmp.bmWidth,bmp.bmHeight);
	}
	
	HBRUSH hBr = ::CreatePatternBrush(hBmp);
	if(hBr == NULL)
		return false;

	return (0 != ::SetClassLong(m_hWnd, GCL_HBRBACKGROUND, (LONG)hBr));
}

void JWinCtrl::SetBitmapRgn( HBITMAP hBmp, COLORREF transClr)
{
	if(hBmp == NULL || m_hWnd == NULL)
		return;

	HDC hdc = ::CreateCompatibleDC(NULL);
	if(hdc == NULL)
		return;

	::SelectObject(hdc,hBmp);

	BITMAP bmp;
	::GetObject(hBmp, sizeof(BITMAP), &bmp);

	HRGN hFullRgn = ::CreateRectRgn(0,0,0,0);

	for (int y = 0; y <= bmp.bmHeight; ++y)
	{
		int nLeft = 0;
		for (int x = 0; x <= bmp.bmWidth; ++x)
		{
			if(::GetPixel(hdc,x,y) == transClr || x == bmp.bmWidth)
			{
				if(nLeft != x)
				{
					HRGN hLineRgn = ::CreateRectRgn(nLeft,y,x,y+1);
					if(hLineRgn != NULL)
						::CombineRgn(hFullRgn,hFullRgn,hLineRgn,RGN_OR);
					::DeleteObject(hLineRgn);
				}
				nLeft = x + 1;
			}			
		}
	}

	::SetWindowRgn(m_hWnd,hFullRgn,true);
	::DeleteObject(hFullRgn);
	::DeleteObject(hdc);
}

void JWinCtrl::SetBitmapRgn( int nResID, HINSTANCE hInst, COLORREF transClr )
{
	HBITMAP hBmp = ::LoadBitmap(hInst ? hInst : JApp::GetInstance(), MAKEINTRESOURCE(nResID));
	if(hBmp != NULL)
	{
		SetBitmapRgn(hBmp, transClr);
		::DeleteObject(hBmp);
	}
}

void JWinCtrl::SetWndColorKey( COLORREF clr )
{
	AddStyleEx(WS_EX_LAYERED);
	SetLayeredWindowAttributes(m_hWnd, clr, 0, LWA_COLORKEY);
}

void JWinCtrl::SetWndAlpha( Byte alpha )
{
	AddStyleEx(WS_EX_LAYERED);
	SetLayeredWindowAttributes(m_hWnd, 0, alpha, LWA_ALPHA);
}

void JWinCtrl::Invalidate( bool bErase )
{
	::InvalidateRect(m_hWnd,NULL,bErase);
}

void JWinCtrl::InvalidateRect( int x,int y, int nWidth, int nHeight, bool bErase /*= TRUE*/ )
{
	RECT rect;
	rect.left = x;
	rect.top = y;
	rect.right = x + nWidth;
	rect.bottom = y + nHeight;
	
	::InvalidateRect(m_hWnd,&rect,bErase);
}

bool JWinCtrl::AttachWnd( JWinCtrl* pParent,int nID )
{
	if(pParent == NULL)
		return false;

	m_pParent = pParent;
	m_hWnd = ::GetDlgItem(m_pParent->GetWnd(),nID);
	if(m_hWnd == NULL)
		return false;

	return ProcessMsg();
}

bool JWinCtrl::DetachWnd()
{
	if(m_pOldProc == NULL || m_hWnd == NULL)
		return false;

	::SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG_PTR)m_pOldProc);
	m_pOldProc = NULL;
	return true;
}

bool JWinCtrl::OnDrawItem( UINT ctlID, LPDRAWITEMSTRUCT lpDrawItem )
{
	if(lpDrawItem == NULL)
		return false;

	HWND hChildWnd = lpDrawItem->hwndItem;
	JWinCtrl* pCtrl = JApp::GetApp()->FindControl(hChildWnd);
	if(pCtrl != NULL)
	{
		pCtrl->DrawItem(lpDrawItem);
		return true;
	}

	return false;
}

bool JWinCtrl::ProcessMsg()
{
	if(!::IsWindow(m_hWnd) || m_pOldProc != NULL)
		return false;

	JApp::GetApp()->AddCtrlByWnd(this,m_hWnd);

	m_pOldProc = (WNDPROC)::SetWindowLong(m_hWnd,GWL_WNDPROC,(LONG_PTR)JApp::WinProc);
	return m_pOldProc ? true : false;
}

bool JWinCtrl::PreProcMsg( MSG *msg )
{
	if (m_hAccel)
		return	(TRUE == ::TranslateAccelerator(m_hWnd, m_hAccel, msg));

	return	false;
}

LRESULT JWinCtrl::WinProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	bool done = false;
	LRESULT result = 0;

	switch(uMsg)
	{
	case WM_CREATE:
		done = OnCreate(lParam);
		break;
	case WM_MOVE:
		done = OnMove(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_SIZE:
		done = OnSize((UINT)wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_NCHITTEST:
 		done = OnNcHitTest(MAKEPOINTS(lParam),&result);
		break;
	case WM_NOTIFY:
		result = done = OnNotify((UINT)wParam, (LPNMHDR)lParam);
		break;
	case WM_COMMAND:
		done = OnCommand(HIWORD(wParam), LOWORD(wParam), (HWND)lParam);
		break;
	case WM_SYSCOMMAND:
		done = OnSysCommand(wParam,MAKEPOINTS(lParam));
		break;
	case WM_CONTEXTMENU:
		result = done = OnContextMenu((HWND)wParam,MAKEPOINTS(lParam));
		break;
	case WM_DROPFILES:
		done = OnDropFiles((HDROP)wParam);
		break;
	case WM_TIMER:
		done = OnTimer(wParam, (TIMERPROC)lParam);
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCRBUTTONDBLCLK:
	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCLBUTTONDBLCLK:
		done = OnButton(uMsg, (int)wParam, MAKEPOINTS(lParam));
		break;
	case WM_MOUSEMOVE:
		done = OnMouseMove((UINT)wParam, MAKEPOINTS(lParam));
		break;
	case WM_MOUSEHOVER:
		done = OnMouseHover();
		break;
	case WM_MOUSELEAVE:
		done = OnMouseLeave();
		break;
	case WM_CTLCOLORBTN:
	case WM_CTLCOLORDLG:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	case WM_CTLCOLORMSGBOX:
	case WM_CTLCOLORSCROLLBAR:
	case WM_CTLCOLORSTATIC:
		done = OnCtlColor(uMsg, (HDC)wParam, (HWND)lParam, (HBRUSH *)&result);
		break;
	case WM_PAINT:
		done = OnPaint();
		break;
	case WM_ERASEBKGND:
		done = OnEraseBkgnd((HDC)wParam);
		break;
	case WM_DRAWITEM:
		result = done = OnDrawItem((UINT)wParam, (LPDRAWITEMSTRUCT)lParam);
		break;
	case WM_KILLFOCUS:
	case WM_SETFOCUS:
		done = OnFocus(uMsg, (HWND)wParam);
		break;
	case WM_CLOSE:
		done = OnClose();
		break;
	case WM_DESTROY:
		done = OnDestroy();
		break;
	case WM_NCDESTROY:
		if(!OnNCDestroy())
			DefWindowProc(uMsg,wParam,lParam);
		done = true;
		JApp::GetApp()->DelControl(this);
		m_hWnd = NULL;
		break;
	default:
		if (uMsg >= WM_APP && uMsg <= 0xBFFF)
			result = done = OnAppEvent(uMsg, wParam, lParam);
		else if (uMsg >= WM_USER && uMsg < WM_APP || uMsg >= 0xC000 && uMsg <= 0xFFFF)
			result = done = OnUserEvent(uMsg, wParam, lParam);
		break;
	}

	return done ? result : DefWindowProc(uMsg,wParam,lParam);
}

LRESULT JWinCtrl::DefWindowProc( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if(m_pOldProc != NULL)
		return ::CallWindowProc(m_pOldProc, m_hWnd, uMsg, wParam, lParam);
	else
		return ::DefWindowProc(m_hWnd,uMsg,wParam,lParam);
}

//------------------------------------------------------------------------------------------------------
const char* JWinCtrl::GetDefaultClass()
{
	if(m_szDefClassName[0] == 0)
	{
		WNDCLASS wc;

		memset(&wc, 0, sizeof(wc));
		wc.style			= (CS_BYTEALIGNCLIENT | CS_BYTEALIGNWINDOW | CS_DBLCLKS);
		wc.lpfnWndProc		= JApp::WinProc;
		wc.cbClsExtra 		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= JApp::GetInstance();
		wc.hIcon			= NULL;
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= NULL;
		wc.lpszMenuName		= NULL;
		wc.lpszClassName	= "DefClassName";

		if (::FindWindow("DefClassName", NULL) == NULL)
		{
			if (::RegisterClass(&wc) == 0)
				return NULL;
		}

		strcpy_s(m_szDefClassName,64,"DefClassName");
	}

	return m_szDefClassName;
}
