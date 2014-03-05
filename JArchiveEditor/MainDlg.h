#ifndef __MAINDLG_H__
#define __MAINDLG_H__

#include "JWin/JDlgCtrl.h"
#include "FileListView.h"

class CMainDlg : public JDlgCtrl
{
public:
	CMainDlg();
	virtual bool OnCreate(LPARAM lParam);
	virtual bool OnClose();
	virtual bool OnDestroy();
	virtual bool OnNotify(UINT ctlID, NMHDR* pNmHdr);

private:
	CFileListView m_FileListView;
};

#endif