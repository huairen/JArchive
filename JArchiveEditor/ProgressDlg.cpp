#include "ProgressDlg.h"
#include <CommCtrl.h>
#include <stdio.h>
#include "resource.h"
#include "archive.h"

#define DEFAULT_TIME_ID 1001
#define WM_PROGRESS_FINISH      WM_APP + 1

void AddFileCallback(void* progress_data)
{
	add_file_progress *prog_info = (add_file_progress*)progress_data;
	ProgressDlg* pDlg = (ProgressDlg*)prog_info->user_data;

	static int last_index = -1;

	if(prog_info->file_name != NULL && prog_info->current != last_index)
	{
		last_index = prog_info->current;

		char szBuffer[260];
		sprintf_s(szBuffer, "[%d/%d]%s", prog_info->current+1, prog_info->total, prog_info->file_name);
		pDlg->SetDlgItemText(IDC_FILENAME, szBuffer);
	}

	pDlg->SendDlgItemMessage(IDC_FILE_PROGRESS, PBM_SETPOS, (WPARAM)(prog_info->curr_per * 100), 0);
}

ProgressDlg::ProgressDlg(void) : JDlgCtrl(IDD_PROGRESS_DLG)
{
	m_pArc = NULL;
	m_hThread = NULL;
	m_nCompression = 0;
}

ProgressDlg::~ProgressDlg(void)
{
}

bool ProgressDlg::OnCreate( LPARAM lParam )
{
	archive_progress_start(m_pArc, AddFileCallback, (void*)this);

	DWORD dwThreadID;
	m_hThread = ::CreateThread(NULL, 0, AddFileThread, this, 0, &dwThreadID);
	SendDlgItemMessage(IDC_FILE_PROGRESS, PBM_SETRANGE32, 0, 100);
	return true;
}

bool ProgressDlg::OnClose()
{
	Cancel();
	return true;
}

bool ProgressDlg::OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCtl )
{
	if(wID == IDC_CANCEL_BTN)
		Cancel();
	return true;
}

bool ProgressDlg::OnAppEvent( UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch(uMsg)
	{
	case WM_PROGRESS_FINISH:
		{
			::WaitForSingleObject(m_hThread, INFINITE);
			::CloseHandle(m_hThread);
			m_hThread = NULL;
			EndDialog(0);
		}
		break;
	}

	return true;
}

void ProgressDlg::Cancel()
{
	archive_progress_stop(m_pArc);
}

bool ProgressDlg::OnAddFiles()
{
	for (size_t i = 0; i < m_FileNames.size(); ++i)
	{
		if(!archive_add_file(m_pArc, m_FileNames[i].c_str(), m_szPath, m_nCompression))
			break;
	}

	return true;
}

DWORD WINAPI ProgressDlg::AddFileThread( void* progDlg )
{
	ProgressDlg* pDlg = (ProgressDlg*)progDlg;
	pDlg->OnAddFiles();
	pDlg->PostMessage(WM_PROGRESS_FINISH, 0, 0);
	return 0;
}
