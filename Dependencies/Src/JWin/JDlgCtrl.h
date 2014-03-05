#ifndef __DLGCTRL_H__
#define __DLGCTRL_H__

#include "JWinCtrl.h"

enum DlgItemFlags {
	NONE_FIT	= 0x000,
	LEFT_FIT	= 0x001,
	HMID_FIT	= 0x002,
	RIGHT_FIT	= 0x004,
	TOP_FIT		= 0x008,
	VMID_FIT	= 0x010,
	BOTTOM_FIT	= 0x020,
	FIT_SKIP	= 0x800,
	X_FIT		= LEFT_FIT|RIGHT_FIT,
	Y_FIT		= TOP_FIT|BOTTOM_FIT,
	XY_FIT		= X_FIT|Y_FIT,
};

class JDlgCtrl : public JWinCtrl
{
public:
	JDlgCtrl(int ResID);
	~JDlgCtrl(void);

	bool Create(JWinCtrl* pParent = NULL, HINSTANCE hInst = NULL);
	void EndDialog(int nResult);
	int Exec(JWinCtrl* pParent = NULL);
	virtual	bool PreProcMsg(MSG *msg);
	virtual LRESULT WinProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	bool m_bModalFlg;
	int m_nResID;
};

#endif