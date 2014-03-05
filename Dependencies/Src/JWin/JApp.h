#ifndef __JAPP_H__
#define __JAPP_H__

#include "JHashTable.h"

class JWinCtrl;

class JApp
{
public:
	JApp(HINSTANCE hInst, LPSTR lpCmdLine, int nCmdShow);
	virtual ~JApp(void);

	virtual bool InitWindow() = 0;
	virtual int Run();
	virtual bool PreProcMsg(MSG *msg);

	void AddControl(JWinCtrl* pCtrl) { m_pPreWnd = pCtrl; }
	void AddCtrlByWnd(JWinCtrl* pCtrl, HWND hWnd);
	void DelControl(JWinCtrl* pCtrl);
	inline JWinCtrl* FindControl(HWND hWnd)
	{ return (JWinCtrl*)m_CtrlHash.Find((UInt32)hWnd); }

	static JApp* GetApp() { return sm_pApp; }
	static HINSTANCE GetInstance();
	static LRESULT CALLBACK WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	HINSTANCE m_hInst;
	JHashTable m_CtrlHash;
	JWinCtrl* m_pPreWnd;

	static JApp* sm_pApp;
};

#endif