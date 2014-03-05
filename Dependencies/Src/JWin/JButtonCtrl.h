#ifndef __BUTTONCTRL_H__
#define __BUTTONCTRL_H__
#include "JWinCtrl.h"

class JButtonCtrl : public JWinCtrl
{
public:
	JButtonCtrl();
	~JButtonCtrl();

	bool Create(JWinCtrl* pParent, int x, int y, int nWidth, int nHeight, const char* pTitle = NULL, int nStyle = 0);

	bool LoadBitmap(int nResID, HINSTANCE hInst=NULL);

	virtual bool OnEraseBkgnd(HDC hDC);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItem);

	virtual bool OnMouseMove(UINT fwKeys, POINTS pos);
	virtual bool OnMouseHover();
	virtual bool OnMouseLeave();

protected:
	HBITMAP m_hBmp;
	bool m_bTrack;
	int m_nWidth;
	int m_nHeight;
};
#endif
