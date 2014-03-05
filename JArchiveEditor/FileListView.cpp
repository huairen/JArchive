#include "FileListView.h"
#include <CommCtrl.h>
#include <ShlObj.h>
#include <time.h>
#include "resource.h"
#include "ProgressDlg.h"
#include "archive.h"
#include "JWin/JApp.h"

#define DIR_STR "<DIR>"

CFileListView::CFileListView(void)
{
	m_pArchive = NULL;
	m_nCompression = 0;
}

CFileListView::~CFileListView(void)
{
	if(m_pArchive != NULL)
		archive_close(m_pArchive);
}

bool CFileListView::OnContextMenu( HWND hChildWnd, POINTS pos )
{
	HMENU hMenu = ::LoadMenu(JApp::GetInstance(), MAKEINTRESOURCE(IDR_MAIN_MENU));
	HMENU hSubMenu = ::GetSubMenu(hMenu, 0);

	bool isSelect = (GetCurrentSelected() >= 0);
	::EnableMenuItem(hSubMenu, MENU_LOOK, isSelect ? MF_ENABLED : MF_DISABLED);
	::EnableMenuItem(hSubMenu, MENU_EXTRACT, isSelect ? MF_ENABLED : MF_DISABLED);
	::EnableMenuItem(hSubMenu, MENU_RENAME, isSelect ? MF_DISABLED : MF_DISABLED);
	::EnableMenuItem(hSubMenu, MENU_DELETE, isSelect ? MF_ENABLED : MF_DISABLED);

	bool isOpen = (m_pArchive != NULL);
	::EnableMenuItem(hSubMenu, MENU_EXTRACT_FILEINFO, isOpen ? MF_ENABLED : MF_DISABLED);
	::EnableMenuItem(hSubMenu, MENU_NEW, isOpen ? MF_DISABLED : MF_ENABLED);
	::EnableMenuItem(hSubMenu, MENU_OPEN, isOpen ? MF_DISABLED : MF_ENABLED);
	::EnableMenuItem(hSubMenu, MENU_CLOSE, isOpen ? MF_ENABLED : MF_DISABLED);

	::TrackPopupMenu(hSubMenu, TPM_RIGHTBUTTON, pos.x, pos.y, 0, m_hWnd, NULL);
	::DestroyMenu(hMenu);
	return true;
}

bool CFileListView::OnDropFiles( HDROP hDrop )
{
	char szFileName[MAX_PATH];
	char szPath[MAX_PATH] = {0};
	UInt32 nTotalCount;

	if(m_pArchive == NULL)
	{
		DragQueryFile(hDrop, 0, szFileName, MAX_PATH);
		OpenArchive(szFileName);
	}
	else
	{
		if(m_FileStack.size() > 1)
			GetCurrentPath(szPath,MAX_PATH);

		ProgressDlg progDlg;
		progDlg.SetArchiveHandle(m_pArchive);
		progDlg.SetCompression(m_nCompression);
		progDlg.AddToDir(szPath);

		nTotalCount = DragQueryFile(hDrop, -1, NULL, 0);
		for (UInt32 i = 0; i < nTotalCount; ++i)
		{
			DragQueryFile(hDrop, i, szFileName, MAX_PATH);
			progDlg.AddFile(szFileName);
		}

		progDlg.Exec(this);

		RefreshFileList();
		ShowFileList(szPath);
	}

	DragFinish(hDrop);

	return true;
}

bool CFileListView::OnCommand( WORD wNotifyCode, WORD wID, HWND hwndCtl )
{
	switch (wID)
	{
	case MENU_LOOK:
		LookItem(GetFirstSelected());
		break;
	case MENU_EXTRACT:
		ExtractSelect();
		break;
	case MENU_RENAME:
		break;
	case MENU_DELETE:
		{
			if(m_pArchive != NULL)
			{
				RemoveFileNode(*(m_FileStack.back()), GetCurrentSelected());
				ShowFileInfo(*(m_FileStack.back()));
			}
		}
		break;
	case MENU_NEW:
		{
			if(m_pArchive == NULL)
			{
				char szBuffer[MAX_PATH] = {0}; 
				OPENFILENAME ofn= {0}; 

				ofn.lStructSize = sizeof(ofn); 
				ofn.hwndOwner = m_hWnd; 
				ofn.lpstrFilter = "JArchive文件(*.arc)\0*.arc\0所有文件(*.*)\0*.*\0";
				ofn.nFilterIndex = 0; 
				ofn.lpstrInitialDir = NULL;
				ofn.lpstrFile = szBuffer;
				ofn.nMaxFile = MAX_PATH; 
				ofn.Flags = OFN_OVERWRITEPROMPT;

				if(GetSaveFileName(&ofn))
				{
					char* pExt = strrchr(szBuffer,'.');
					if(pExt == NULL)
						strcat_s(szBuffer, MAX_PATH, ".arc");

					m_pArchive = archive_create(szBuffer,0);
					if(m_pArchive != NULL && m_pParent != NULL)
					{
						char szTitle[MAX_PATH];
						sprintf_s(szTitle, MAX_PATH, "JArchiveEditor - %s",szBuffer);
						m_pParent->SetWindowText(szTitle);
						strcpy_s(m_szArchiveName, MAX_PATH, szBuffer);
					}
				}
			}
		}
		break;
	case MENU_OPEN:
		{
			if(m_pArchive == NULL)
			{
				char szBuffer[MAX_PATH] = {0}; 
				OPENFILENAME ofn= {0};

				ofn.lStructSize = sizeof(ofn); 
				ofn.hwndOwner = m_hWnd; 
				ofn.lpstrFilter = "JArchive文件(*.arc)\0*.arc\0所有文件(*.*)\0*.*\0";
				ofn.nFilterIndex = 0; 
				ofn.lpstrInitialDir = NULL;
				ofn.lpstrFile = szBuffer;
				ofn.nMaxFile = MAX_PATH; 
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

				if(GetOpenFileName(&ofn))
					OpenArchive(szBuffer);
			}
		}
		break;
	case MENU_CLOSE:
		CloseArchive();
		break;
	case MENU_EXTRACT_FILEINFO:
		{
			char szSavePath[MAX_PATH];
			OPENFILENAME ofn = {0};

			if(m_pArchive == NULL)
				break;

			char* pName = strrchr(m_szArchiveName, '\\');
			if(pName != NULL)
				strcpy_s(szSavePath, MAX_PATH, pName+1);
			else
				strcpy_s(szSavePath, MAX_PATH, m_szArchiveName);

			pName = strrchr(szSavePath, '.');
			if(pName != NULL)
				*pName = 0;

			ofn.lStructSize = sizeof(ofn); 
			ofn.hwndOwner = m_hWnd; 
			ofn.lpstrFilter = "JArchive文件信息(*.afi)\0*.afi\0所有文件(*.*)\0*.*\0";
			ofn.nFilterIndex = 0; 
			ofn.lpstrInitialDir = NULL;
			ofn.lpstrFile = szSavePath;
			ofn.nMaxFile = MAX_PATH; 
			ofn.Flags = OFN_OVERWRITEPROMPT;

			if(GetSaveFileName(&ofn))
			{
				char* pExt = strrchr(szSavePath,'.');
				if(pExt == NULL)
					strcat_s(szSavePath, MAX_PATH, ".afi");

				archive_extract_describe(m_pArchive, szSavePath);
			}
		}
		break;
	}
	return false;
}

bool CFileListView::OpenArchive( const char* pFileName )
{
	CloseArchive();

	m_pArchive = archive_open(pFileName,0);
	if(m_pArchive == NULL)
	{
		ErrotTip();
		return false;
	}

	if(m_pParent != NULL)
	{
		char szTitle[MAX_PATH];
		sprintf_s(szTitle, MAX_PATH, "JArchiveEditor - %s",pFileName);
		m_pParent->SetWindowText(szTitle);
		strcpy_s(m_szArchiveName, MAX_PATH, pFileName);
	}

	RefreshFileList();
	ShowFileList(NULL);
	return true;
}

void CFileListView::CloseArchive()
{
	if(m_pArchive != NULL)
	{
		DeleteAllItems();

		archive_close(m_pArchive);
		m_pArchive = NULL;

		m_FileStack.clear();
		m_FileList.Chlids.clear();

		if(m_pParent != NULL)
		{
			HWND hWnd = GetDlgItem(m_pParent->GetWnd(), IDC_STATUS_TEXT);
			if(IsWindow(hWnd))
				::SetWindowText(hWnd, "");

			m_pParent->SetWindowText("JArchiveEditor");
		}
	}
}

void CFileListView::LookItem( int nIndex )
{
	if(nIndex == -1)
		return;

	char szText[64];
	if(GetItemText(nIndex, 1, szText, 64))
	{
		if(strcmp(szText,DIR_STR) == 0)
		{
			if(GetItemText(nIndex, 0, szText, 64))
				ShowFileList(szText);
		}
	}
}

//--------------------------------------------------------------------------------
void CFileListView::RefreshFileList()
{
	archive_file *pFile;

	strcpy_s(m_FileList.szName, 64, "root");
	m_FileList.nFileSize = 0;
	m_FileList.nCompSize = 0;
	m_FileList.FileTime = 0;
	m_FileList.Chlids.clear();

	m_FileStack.clear();
	m_FileStack.push_back(&m_FileList);

	if(m_pArchive == NULL)
		return;

	char szFilename[MAX_PATH];

	pFile = archive_find_first(m_pArchive);
	while(pFile != NULL)
	{
		memset(szFilename, 0, MAX_PATH);
		archive_file_property(pFile, JARCHIVE_FILE_NAME, szFilename, MAX_PATH);
		FileInfo& item = PushToFileList(m_FileList, szFilename);

		archive_file_property(pFile, JARCHIVE_FILE_SIZE, &(item.nFileSize), 4);
		archive_file_property(pFile, JARCHIVE_FILE_COMPRESS_SIZE, &(item.nCompSize), 4);
		archive_file_property(pFile, JARCHIVE_FILE_IS_COMPLETE, &(item.bDataComplete), 1);
		archive_file_property(pFile, JARCHIVE_FILE_TIME, &(item.FileTime), 8);

		if( !archive_find_next(pFile) )
			break;
	}
	archive_file_close(pFile);
}

CFileListView::FileInfo& CFileListView::PushToFileList( FileInfo& Item, char* pFileName )
{
	UInt32 nLength;
	char* pDir = NULL;

	assert(pFileName);
	nLength = strlen(pFileName);
	for (UInt32 i=0; i<nLength; ++i)
	{
		if((pFileName[i] == '/') || (pFileName[i] == '\\'))
		{
			pDir = &pFileName[i];
			break;
		}
	}

	if(pDir != NULL)
	{
		*pDir = 0;

		for (UInt32 i = 0; i < Item.Chlids.size(); ++i)
		{
			if(_stricmp(Item.Chlids[i].szName, pFileName) == 0)
			{
				*pDir = '/';
				return PushToFileList(Item.Chlids[i], pDir + 1);
			}
		}

		CFileListView::FileInfo temp;
		strcpy_s(temp.szName, 64, pFileName);

		temp.nFileSize = 0;
		temp.nCompSize = 0;
		temp.FileTime = 0;

		Item.Chlids.push_back(temp);

		*pDir = '/';
		return PushToFileList(Item.Chlids.back(), pDir + 1);
	}

	CFileListView::FileInfo temp;
	strcpy_s(temp.szName, 64, pFileName);

	temp.nFileSize = 0;
	temp.nCompSize = 0;
	temp.FileTime = 0;

	Item.Chlids.push_back(temp);

	return Item.Chlids.back();
}

bool CFileListView::RemoveFileNode( FileInfo& Item, int nIndex )
{
	if(nIndex < 0 || nIndex >= (int)Item.Chlids.size())
		return false;

	FileInfo& ChildItem = Item.Chlids[nIndex];
	m_FileStack.push_back(&ChildItem);

	for (UInt32 i=0; i<ChildItem.Chlids.size();)
	{
		if(!RemoveFileNode(ChildItem, i))
			++i;
	}

	m_FileStack.pop_back();

	char szPath[MAX_PATH] = {0};
	if(GetCurrentFileName(nIndex, szPath, MAX_PATH))
	{
		if(ChildItem.nFileSize > 0)
			archive_remove_file(m_pArchive, szPath);
		Item.Chlids.erase(Item.Chlids.begin() + nIndex);

		return true;
	};

	return false;
}

void CFileListView::ShowFileList( const char* pPath )
{
	if(pPath == NULL || *pPath == 0)
	{
		ShowFileInfo(*(m_FileStack.back()));
	}
	else if(strcmp(pPath, "..") == 0)
	{
		m_FileStack.pop_back();
		ShowFileInfo(*(m_FileStack.back()));
	}
	else if(!m_FileStack.empty())
	{
		char szTemp[MAX_PATH];
		char *pStep, *pNext;
		FileInfo* pCurrFile;

		strcpy_s(szTemp, MAX_PATH, pPath);
		pStep = strtok_s(szTemp, "\\/", &pNext);

		if(_stricmp(pStep,"root") == 0)
			pCurrFile = &m_FileList;
		else
			pCurrFile = m_FileStack.back();

		while(pStep != NULL)
		{
			for (UInt32 i=0; i<pCurrFile->Chlids.size(); ++i)
			{
				if(strcmp(pCurrFile->Chlids[i].szName, pStep) == 0)
				{
					m_FileStack.push_back(&(pCurrFile->Chlids[i]));
					pCurrFile = m_FileStack.back();
					pStep = strtok_s(NULL, "\\/", &pNext);
					break;
				}
			}
		}

		ShowFileInfo(*pCurrFile);
	}

	if(m_pParent != NULL)
	{
		HWND hWnd = GetDlgItem(m_pParent->GetWnd(), IDC_STATUS_TEXT);
		if(IsWindow(hWnd))
		{
			char szPath[MAX_PATH];
			GetCurrentPath(szPath, MAX_PATH);
			::SetWindowText(hWnd, szPath);
		}
	}
}

void CFileListView::ShowFileInfo( FileInfo& Item )
{
	UInt32 nIndex = 0;

	DeleteAllItems();

	if(m_FileStack.size() > 1)
	{
		InsertItem(nIndex, "..",1);
		SetSubItem(nIndex, 1, DIR_STR);
		SetSubItem(nIndex, 2, DIR_STR);
		++nIndex;
	}

	for (UInt32 i = 0; i < Item.Chlids.size(); ++i,++nIndex)
	{
		FileInfo& info = Item.Chlids[i];
		InsertItem(nIndex, info.szName, info.nFileSize ? 0 : 1);

		if(info.nFileSize > 0)
		{
			SetSubItem(nIndex, 1, (char*)ConvertSize(info.nFileSize));
			SetSubItem(nIndex, 2, (char*)ConvertSize(info.nCompSize));
			SetSubItem(nIndex, 4, info.bDataComplete ? "Y" : "N");
		}
		else
		{
			SetSubItem(nIndex, 1, DIR_STR);
			SetSubItem(nIndex, 2, DIR_STR);
		}

		if(info.FileTime != 0)
			SetSubItem(nIndex, 3, (char*)ConvertTime(&(info.FileTime)));
	}
}

bool CFileListView::GetCurrentPath( char* pPath, UInt32 nSize )
{
	*pPath = 0;
	for (UInt32 i=1; i<m_FileStack.size(); ++i)
	{
		if(i > 1)
			strcat_s(pPath, nSize, "\\");
		strcat_s(pPath, nSize, m_FileStack[i]->szName);
	}

	return (m_FileStack.size() > 1);
}

bool CFileListView::GetCurrentFileName( int nIndex, char* pPath, UInt32 nSize )
{
	if(nIndex < 0 || nSize == 0)
		return false;

	if(GetCurrentPath(pPath, nSize))
		strcat_s(pPath, nSize, "\\");

	if(nIndex < (int)(m_FileStack.back()->Chlids.size()))
		strcat_s(pPath, nSize, m_FileStack.back()->Chlids[nIndex].szName);

	return true;
}

int CFileListView::GetCurrentSelected()
{
	int nIndex = GetFirstSelected();
	if(m_FileStack.size() > 1)
		nIndex--;

	return nIndex;
}

void CFileListView::ExtractFile(const char* pArcFile)
{
	char szSavePath[MAX_PATH];
	OPENFILENAME ofn = {0};

	const char* pName = strrchr(pArcFile, '\\');
	if(pName != NULL)
		strcpy_s(szSavePath, MAX_PATH, pName+1);
	else
		strcpy_s(szSavePath, MAX_PATH, pArcFile);

	ofn.lStructSize = sizeof(ofn); 
	ofn.hwndOwner = m_hWnd; 
	ofn.lpstrFilter = "所有文件(*.*)\0*.*\0";
	ofn.nFilterIndex = 0; 
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrFile = szSavePath;
	ofn.nMaxFile = MAX_PATH; 
	ofn.Flags = OFN_OVERWRITEPROMPT;

	if(GetSaveFileName(&ofn))
		archive_extract_file(m_pArchive, pArcFile, szSavePath);
}

void CFileListView::ExtractFolder( FileInfo *pItem, const char* pCurrPath, const char *pSavePath )
{
	FileInfo *pSubItem;
	char szCurrPath[MAX_PATH];
	char szSavePath[MAX_PATH];
	char szPath[MAX_PATH];

	szCurrPath[0] = 0;
	if(pCurrPath && *pCurrPath)
	{
		strcat_s(szCurrPath, MAX_PATH, pCurrPath);
		strcat_s(szCurrPath, MAX_PATH, "\\");
	}
	strcat_s(szCurrPath, MAX_PATH, pItem->szName);

	sprintf_s(szSavePath, "%s\\%s", pSavePath, szCurrPath);
	if(!::CreateDirectory(szSavePath, NULL))
		return;

	for (size_t i=0; i<pItem->Chlids.size(); i++)
	{
		pSubItem = &(pItem->Chlids[i]);

		if(pSubItem->nFileSize > 0)
		{
			if(GetCurrentPath(szPath, MAX_PATH))
				strcat_s(szPath, MAX_PATH, "\\");

			strcat_s(szPath, MAX_PATH, szCurrPath);
			strcat_s(szPath, MAX_PATH, "\\");
			strcat_s(szPath, MAX_PATH, pSubItem->szName);

			sprintf_s(szSavePath, "%s\\%s\\%s", pSavePath, szCurrPath, pSubItem->szName);

			archive_extract_file(m_pArchive, szPath, szSavePath);
		}
		else
			ExtractFolder(pSubItem, szCurrPath, pSavePath);
	}
}

void CFileListView::ExtractSelect()
{
	UInt32 nIndex;
	FileInfo *info = NULL;
	char szPath[MAX_PATH];

	if(m_pArchive == NULL)
		return;

	nIndex = GetCurrentSelected();
	if(nIndex < m_FileStack.back()->Chlids.size())
		info = &(m_FileStack.back()->Chlids[nIndex]);

	if(info != NULL)
	{
		if(info->nFileSize > 0)
		{
			GetCurrentFileName(nIndex, szPath, MAX_PATH);
			ExtractFile(szPath);
		}
		else
		{
			BROWSEINFO bi; 
			bi.hwndOwner      = m_hWnd;
			bi.pidlRoot       = NULL;
			bi.pszDisplayName = NULL; 
			bi.lpszTitle      = TEXT("请选择文件夹"); 
			bi.ulFlags        = BIF_RETURNONLYFSDIRS | BIF_STATUSTEXT;
			bi.lpfn           = NULL; 
			bi.lParam         = 0;
			bi.iImage         = 0; 

			LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
			if (pidl == NULL)
				return;

			if (SHGetPathFromIDList(pidl, szPath))
				ExtractFolder(info, NULL, szPath);
		}
	}
}

const char* CFileListView::ConvertSize( UInt32 nSize )
{
	static char szBuffer[1024];
	UInt32 nByte[4] = {0};
	char szByteStr[8];
	int i;

	nByte[3] = nSize;
	for (i=3; i >= 0; --i)
	{
		if(i > 0)
		{
			nByte[i-1] = nByte[i] / 1000;
			if(nByte[i-1] == 0)
				break;
		}
		nByte[i] %= 1000;
	}

	memset(szBuffer, 0, 1024);
	sprintf_s(szBuffer, 1024, "%d", nByte[i++]);

	for (; i<4; ++i)
	{
		strcat_s(szBuffer, ",");
		sprintf_s(szByteStr, 8, "%.3d", nByte[i]);
		strcat_s(szBuffer, szByteStr);
	}

	return szBuffer;
}

const char* CFileListView::ConvertTime( const time_t* pFileTime )
{
	static char szBuffer[1024];
	tm localTime;

	assert(pFileTime != NULL);

	localtime_s(&localTime, pFileTime);

	sprintf_s(szBuffer, 1024, "%d/%d/%d %.2d:%.2d",
		localTime.tm_year + 1900,
		localTime.tm_mon + 1,
		localTime.tm_mday,
		localTime.tm_hour,
		localTime.tm_min);

	return szBuffer;
}

void CFileListView::ErrotTip()
{
	char szError[256];
	int nError = GetLastError();
	switch(nError)
	{
	case ERROR_SHARING_VIOLATION:
		sprintf_s(szError, 256, "其它进程使用中，不用访问！");
		break;
	case ERROR_ACCESS_DENIED:
		sprintf_s(szError, 256, "文件访问受限！");
		break;
	case ERROR_BAD_FORMAT:
		sprintf_s(szError, 256, "文件格式错误！");
		break;
	default:
		sprintf_s(szError, 256, "操作失败！");
		break;
	}

	MessageBox(m_hWnd,szError,"错误", MB_OK);
}