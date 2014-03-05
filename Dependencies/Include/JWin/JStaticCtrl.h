#ifndef __STATICCTRL_H__
#define __STATICCTRL_H__
#include "JWinCtrl.h"

class JStaticCtrl : public JWinCtrl
{
public:
	enum Align
	{
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT,
	};

public:
	bool Create(JWinCtrl* pParent, int x, int y, int nWidth, int nHeight, const char* pTitle = NULL, int nStyle = 0);

	void AlignText(int nStyle);
	void SetTransparent(bool bValue = true);
	void EraseBkgnd();
};

#endif
