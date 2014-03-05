#ifndef __LIST_VIEW_H__
#define __LIST_VIEW_H__

#include "JWinCtrl.h"
#include <CommCtrl.h>

class JListView : public JWinCtrl
{
public:
	JListView();
	~JListView();

	bool AddExtendStyle(UInt32 nStyle);
	void LoadSystemImageList();

	bool InsertColumn(UInt32 nIndex, char* pText, UInt32 nWidth);
	bool InsertItem(UInt32 nIndex, char* pText, int nImage = -1, LPARAM lParam = 0);
	bool SetSubItem(UInt32 nIndex, UInt32 nSubIndex, char* pText);
	bool GetItemText(UInt32 nIndex, UInt32 nSubIndex, char* pBuffer, UInt32 nSize);
	int GetSelectionMark();
	int GetFirstSelected();
	int GetNextSelected(int nIndex);
	void DeleteAllItems();
};

#endif