#ifndef __WINCTRL_H__
#define __WINCTRL_H__

#include "JTypes.h"

class JWinCtrl
{
public:
	JWinCtrl();
	virtual ~JWinCtrl(void);

	virtual bool Create(LPCSTR className=NULL, LPCSTR title=NULL,
		DWORD style=WS_OVERLAPPEDWINDOW, JWinCtrl* pParent=NULL, HMENU hMenu=NULL);
	void Destroy();

	void Show(int mode = SW_SHOWDEFAULT);
	void ShowWindow(int mode = SW_SHOWDEFAULT);
	void ShowChlid(int nID, int mode = SW_SHOWDEFAULT);
	bool EnableWindow(bool bEnable);
	bool SetForegroundWindow();

	void MoveToCenter();
	void SetPos(int x, int y);
	void SetExtent(int nWidth, int nHeight);
	void MoveWindow(int x, int y, int nWidth, int nHeight,bool bRepaint = true);
	void MoveWindow(const RECT& rect, bool bRepaint = true);

	bool PostMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT	SendMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT	SendDlgItemMessage(int nIDDlgItem, UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool SetWindowText(const char* pText);
	int GetWindowText(char* pText, int nSize);
	bool SetDlgItemText(int nIDDlgItem, const char* pText);
	int GetDlgItemText(int nIDDlgItem, char* pText, int nSize);

	bool AddStyle(UINT nStyle);
	bool RemoveStyle(UINT nStyle);
	bool AddStyleEx(UINT nStyle);
	bool RemoveStyleEx(UINT nStyle);

	void DragAcceptFiles(bool bAccept);
	bool SetIcon(int nResID);
	int SetCtrlID(int nID);
	bool SetBackgroundBitmap(int nResID, bool bChangeSize = true);
	void SetBitmapRgn(HBITMAP hBmp, COLORREF transClr);
	void SetBitmapRgn(int nResID, HINSTANCE hInst, COLORREF transClr);
	void SetWndColorKey(COLORREF clr);
	void SetWndAlpha(Byte alpha);
	void Invalidate(bool bErase = true);
	void InvalidateRect(int x,int y, int nWidth, int nHeight, bool bErase = true);

	bool AttachWnd(JWinCtrl* pParent,int nID);
	bool DetachWnd();
	bool ProcessMsg();

	inline void SetWnd(HWND hWnd) { m_hWnd = hWnd; }
	inline HWND GetWnd() { return m_hWnd; }

	//------------------------------------------------------------------------------------
	// 消息事件
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItem){};

	virtual bool OnCreate(LPARAM lParam) { return false; }
	virtual bool OnMove(int xpos, int ypos) { return false; }
	virtual bool OnSize(UINT nSizeType, WORD nWidth, WORD nHeight) { return false; }
	virtual bool OnNcHitTest(POINTS ptPos, LRESULT* result) { return false; }
	virtual bool OnNotify(UINT ctlID, NMHDR* pNmHdr) { return false; }
	virtual bool OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl) { return false; }
	virtual bool OnSysCommand(WPARAM uCmdType, POINTS pos) { return false; }
	virtual bool OnContextMenu(HWND childWnd, POINTS pos) { return false; }
	virtual bool OnDropFiles(HDROP hDrop) { return false; }
	virtual bool OnTimer(WPARAM timerID, TIMERPROC proc) { return false; }
	virtual bool OnButton(UINT uMsg, int nHitTest, POINTS pos) { return false; }
	virtual bool OnMouseMove(UINT fwKeys, POINTS pos) { return false; }
	virtual bool OnMouseHover() { return false; }
	virtual bool OnMouseLeave() { return false; }
	virtual bool OnClose() { return false; }
	virtual bool OnDestroy() { return false; }
	virtual bool OnNCDestroy() { return false; }
	virtual bool OnPaint() { return false; }
	virtual bool OnCtlColor(UINT uMsg, HDC hDcCtl, HWND hWndCtl, HBRUSH* result) { return false; }
	virtual bool OnEraseBkgnd(HDC hDC) { return false; }
	virtual bool OnDrawItem(UINT ctlID, LPDRAWITEMSTRUCT lpDrawItem);
	virtual bool OnFocus(UINT uMsg, HWND focusWnd) { return false; }
	virtual bool OnAppEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) { return false; }
	virtual bool OnUserEvent(UINT uMsg, WPARAM wParam, LPARAM lParam) { return false; }
	virtual	bool PreProcMsg(MSG *msg);
	virtual LRESULT WinProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual	LRESULT	DefWindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam);


protected:
	const char* GetDefaultClass();

protected:
	HWND m_hWnd;
	HACCEL m_hAccel;
	WNDPROC m_pOldProc;
	char m_szDefClassName[64];

	JWinCtrl* m_pParent;
};

#endif