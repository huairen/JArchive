#ifndef __EDITCTRL_H__
#define __EDITCTRL_H__
#include "JWinCtrl.h"

class JEditCtrl : public JWinCtrl
{
public:
	bool Create(JWinCtrl* pParent, int x, int y, int nWidth, int nHeight, const char* pTitle = NULL, int nStyle = 0);
};

#endif
