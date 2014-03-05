#include "MainDlg.h"
#include "SetupDlg.h"

#include "resource.h"

CMainDlg::CMainDlg() : JDlgCtrl(IDD_MAINDLG)
{
}

bool CMainDlg::OnCreate( LPARAM lParam )
{
	m_FileListView.AttachWnd(this,IDC_FILE_LIST);
	m_FileListView.AddExtendStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_FileListView.LoadSystemImageList();
	m_FileListView.DragAcceptFiles(true);
	m_FileListView.InsertColumn(0,"文件名",200);
	m_FileListView.InsertColumn(1,"大小",90);
	m_FileListView.InsertColumn(2,"压缩后的大小",84);
	m_FileListView.InsertColumn(3,"日期时间",104);
	m_FileListView.InsertColumn(4,"数据完整性",104);

	SetIcon(ARCHIVE_ICON);

	char* pCmd = strchr(GetCommandLine() + 1, '"') + 2;
	if(*pCmd != 0)
	{
		pCmd += 1;
		int nLen = strlen(pCmd);

		char szFileName[MAX_PATH];
		strncpy_s(szFileName, pCmd, nLen - 1);
		m_FileListView.OpenArchive(szFileName);
	}

	return true;
}

bool CMainDlg::OnClose()
{
	EndDialog(0);
	return true;
}

bool CMainDlg::OnDestroy()
{
	PostQuitMessage(0);
	return true;
}

bool CMainDlg::OnNotify( UINT ctlID, NMHDR* pNmHdr )
{
	switch(ctlID)
	{
	case IDC_FILE_LIST:
		{
			if(pNmHdr->code == NM_DBLCLK)
				m_FileListView.LookItem(((LPNMITEMACTIVATE) pNmHdr)->iItem);
		}
		break;
	case IDC_SETUP_LINK:
		{
			if(pNmHdr->code == NM_CLICK)
			{
				SetupDlg dlg;
				dlg.SetInitCompression(m_FileListView.GetCompression());
				if(dlg.Exec(this) == 0)
				{
					m_FileListView.SetCompression(dlg.GetCompression());
				}
			}
		}
		break;
	}
	return false;
}
