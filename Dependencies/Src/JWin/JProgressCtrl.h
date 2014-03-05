#ifndef __PROGRESS_CTRL_H__
#define __PROGRESS_CTRL_H__

#include "JWinCtrl.h"

class JProgressCtrl : public JWinCtrl
{
public:
	bool Create(JWinCtrl* pParent, int x, int y, int nWidth, int nHeight, int nStyle = 0);

	void SetRange(short nLower, short nUpper);
	void SetRange32(int nLower, short nUpper);
	int GetRangeLow();
	int GetRangeHigh();

	int SetPos(int nPos);
	int GetPos();

	int StepIt();
	void SetStep(int nStepInc);
};


class JBitmapProgressCtrl : public JWinCtrl
{
public:
	JBitmapProgressCtrl();
	~JBitmapProgressCtrl();

	bool Create(JWinCtrl* pParent, int x, int y, int nResID, HINSTANCE hInst = NULL);

	virtual bool OnEraseBkgnd(HDC hDC);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItem);

	void SetPos(int nPos);
	inline int GetPos()
	{ return m_nPos; }


private:
	HBITMAP m_hBmp;
	int m_nPos;
	int m_nMinLimit;
	int m_nMaxLimit;
};

#endif