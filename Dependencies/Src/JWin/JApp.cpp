#include "JApp.h"
#include "JWinCtrl.h"

//使用Win7界面风格
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


JApp* JApp::sm_pApp = NULL;

JApp::JApp(HINSTANCE hInst, LPSTR lpCmdLine, int nCmdShow)
{
	m_hInst = hInst;
	m_pPreWnd = NULL;
	sm_pApp = this;
}

JApp::~JApp(void)
{
}

int JApp::Run()
{
	if(!InitWindow())
		return 0;

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if(PreProcMsg(&msg))
			continue;

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;
}

bool JApp::PreProcMsg( MSG *msg )
{
	for (HWND hWnd = msg->hwnd; hWnd != NULL; hWnd = ::GetParent(hWnd))
	{
		JWinCtrl* pWin = FindControl(hWnd);
		if(pWin != NULL)
			return pWin->PreProcMsg(msg);
	}

	return false;
}


HINSTANCE JApp::GetInstance()
{
	if(sm_pApp == NULL)
		return 0;
	
	return sm_pApp->m_hInst;
}

void JApp::AddCtrlByWnd( JWinCtrl* pCtrl, HWND hWnd )
{
	pCtrl->SetWnd(hWnd);
	m_CtrlHash.Insert((UInt32)hWnd, pCtrl);
}

void JApp::DelControl( JWinCtrl* pCtrl )
{
	m_CtrlHash.Remove((UInt32)pCtrl->GetWnd());
}


LRESULT CALLBACK JApp::WinProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	JApp* app = JApp::GetApp();
	JWinCtrl* ctrl = app->FindControl(hWnd);

	if(ctrl != NULL)
		return ctrl->WinProc(uMsg,wParam,lParam);

	if((ctrl = app->m_pPreWnd))
	{
		app->m_pPreWnd = NULL;
		app->AddCtrlByWnd(ctrl,hWnd);
		return ctrl->WinProc(uMsg,wParam,lParam);
	}

	return ::DefWindowProc(hWnd,uMsg,wParam,lParam);
}