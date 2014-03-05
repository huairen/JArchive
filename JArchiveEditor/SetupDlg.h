#ifndef __SETUPDLG_H__
#define __SETUPDLG_H__
#include "JWin/JDlgCtrl.h"


class SetupDlg : public JDlgCtrl
{
public:
	SetupDlg(void);
	~SetupDlg(void);

	virtual bool OnCreate(LPARAM lParam);
	virtual bool OnClose();
	virtual bool OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

	void SetInitCompression(int nComp)
	{ m_nCompression = nComp; }

	int GetCompression()
	{ return m_nCompression; }

private:
	int m_nCompression;
};

#endif