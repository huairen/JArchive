#ifndef __PROGRESSDLG_H__
#define __PROGRESSDLG_H__
#include "JWin/JDlgCtrl.h"
#include <vector>
#include <string>

struct archive;

class ProgressDlg : public JDlgCtrl
{
	typedef std::vector<std::string> StringList;
public:
	ProgressDlg(void);
	~ProgressDlg(void);

	virtual bool OnCreate(LPARAM lParam);
	virtual bool OnClose();
	virtual bool OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);
	virtual bool OnAppEvent(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void Cancel();
	void SetArchiveHandle(archive *pArc);
	void SetCompression(int nCompression)
	{ m_nCompression = nCompression; }

	void AddToDir(const char* pDir);
	void AddFile(const char* pFileName);

	bool OnAddFiles();

	static DWORD WINAPI AddFileThread(void* progDlg);

private:
	archive *m_pArc;
	char m_szPath[MAX_PATH];
	StringList m_FileNames;
	int m_nCompression;

	HANDLE m_hThread;
};

inline void ProgressDlg::SetArchiveHandle( archive *pArc )
{
	m_pArc = pArc;
}

inline void ProgressDlg::AddToDir( const char* pDir )
{
	strcpy_s(m_szPath, MAX_PATH, pDir);
}

inline void ProgressDlg::AddFile( const char* pFileName )
{
	m_FileNames.push_back(pFileName);
}

#endif
