#include "SetupDlg.h"
#include "resource.h"
#include "archive.h"

int g_nArchiveCompression[] = {
	JARCHIVE_COMPRESS_NONE,
	JARCHIVE_COMPRESS_ZLIB,
	JARCHIVE_COMPRESS_LZMA
};

SetupDlg::SetupDlg(void) : JDlgCtrl(IDD_SETUP_DLG)
{
	m_nCompression = 0;
}

SetupDlg::~SetupDlg(void)
{
}

bool SetupDlg::OnCreate( LPARAM lParam )
{
	::SendDlgItemMessage(m_hWnd, IDC_COMBO_COMPRESS, CB_ADDSTRING, 0, (LPARAM)"нч");
	::SendDlgItemMessage(m_hWnd, IDC_COMBO_COMPRESS, CB_ADDSTRING, 0, (LPARAM)"ZLIB");
	::SendDlgItemMessage(m_hWnd, IDC_COMBO_COMPRESS, CB_ADDSTRING, 0, (LPARAM)"LZMA");
	::SendDlgItemMessage(m_hWnd, IDC_COMBO_COMPRESS, CB_SETCURSEL, m_nCompression, 0);
	return true;
}

bool SetupDlg::OnClose()
{
	EndDialog(0);
	return true;
}

bool SetupDlg::OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCtl )
{
	switch(wID)
	{
	case IDOK:
		EndDialog(0);
		break;
	case IDCANCEL:
		EndDialog(1);
		break;
	case IDC_COMBO_COMPRESS:
		{
			if(wNotifyCode == CBN_SELCHANGE)
			{
				int nIndex = SendDlgItemMessage(IDC_COMBO_COMPRESS, CB_GETCURSEL, 0, 0);
				if(nIndex < sizeof(g_nArchiveCompression) / sizeof(int))
					m_nCompression = g_nArchiveCompression[nIndex];
			}
		}
		break;
	}

	return true;
}
