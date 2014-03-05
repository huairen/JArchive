#ifndef __FILELISTVIEW_H__
#define __FILELISTVIEW_H__

#include "JWin/JListView.h"
#include <vector>

struct archive;

class CFileListView : public JListView
{
	struct FileInfo
	{
		char szName[64];
		int nFileSize;
		int nCompSize;
		bool bDataComplete;
		time_t FileTime;

		std::vector<FileInfo> Chlids;
	};

public:
	CFileListView();
	~CFileListView();

	virtual bool OnContextMenu(HWND hChildWnd, POINTS pos);
	virtual bool OnDropFiles(HDROP hDrop);
	virtual bool OnCommand(WORD wNotifyCode, WORD wID, HWND hwndCtl);

	bool OpenArchive(const char* pFileName);
	void CloseArchive();
	void LookItem(int nIndex);
	void SetCompression(int nCompr) { m_nCompression = nCompr; }
	int GetCompression() { return m_nCompression; }

protected:
	// update file list
	void RefreshFileList();
	FileInfo& PushToFileList(FileInfo& Item, char* pFileName);
	bool RemoveFileNode(FileInfo& Item, int nIndex);

	void ShowFileList(const char* pPath);
	void ShowFileInfo(FileInfo& Item);

	bool GetCurrentPath(char* pPath, UInt32 nSize);
	bool GetCurrentFileName(int nIndex, char* pPath, UInt32 nSize);
	int GetCurrentSelected();

	void ExtractFile(const char* pArcFile);
	void ExtractFolder(FileInfo *pItem, const char* pCurrPath, const char *pSavePath);
	void ExtractSelect();
	
	const char* ConvertSize(UInt32 nSize);
	const char* ConvertTime(const time_t* pFileTime);
	void ErrotTip();

private:
	archive *m_pArchive;
	char m_szArchiveName[MAX_PATH];

	FileInfo m_FileList;
	std::vector<FileInfo*> m_FileStack;

	int m_nCompression;
};

#endif
