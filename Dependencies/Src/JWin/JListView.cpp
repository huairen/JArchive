#include "JListView.h"
#include "JApp.h"
#pragma comment(lib, "comctl32.lib")

JListView::JListView()
{

}

JListView::~JListView()
{

}

bool JListView::AddExtendStyle( UInt32 nStyle )
{
	UInt32 oldStyle = SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	if((oldStyle & nStyle) == nStyle)
		return true;

	return (SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, oldStyle | nStyle) != 0) ? true : false;
}

void JListView::LoadSystemImageList()
{
	HIMAGELIST hSmallIconList;
	SHFILEINFO sfi = {0};

	hSmallIconList = (HIMAGELIST)SHGetFileInfo("",
		FILE_ATTRIBUTE_NORMAL,
		&sfi, sizeof(sfi),
		SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	if(hSmallIconList != NULL)
		SendMessage(LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)hSmallIconList);
}

bool JListView::InsertColumn( UInt32 nIndex, char* pText, UInt32 nWidth )
{
	LVCOLUMN _lvcol;
	memset(&_lvcol, 0, sizeof(LVCOLUMN));
	_lvcol.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
	_lvcol.pszText = pText;
	_lvcol.cx = nWidth;
	_lvcol.iSubItem = nIndex;
	return (SendMessage(LVM_INSERTCOLUMN, nIndex, (LPARAM)&_lvcol) != -1);
}

bool JListView::InsertItem( UInt32 nIndex, char* pText, int nImage, LPARAM lParam )
{
	LVITEM _lvitem;
	memset(&_lvitem, 0, sizeof(LVITEM));
	_lvitem.mask = LVIF_TEXT | (lParam ? LVIF_PARAM : 0) | ((nImage >= 0) ? LVIF_IMAGE : 0);
	_lvitem.iItem = nIndex;
	_lvitem.pszText = pText;
	_lvitem.lParam = lParam;
	_lvitem.iImage = nImage;
	return (SendMessage(LVM_INSERTITEM, 0, (LPARAM)&_lvitem) != -1);
}

bool JListView::SetSubItem( UInt32 nIndex, UInt32 nSubIndex, char* pText )
{
	LVITEM _lvitem = {0};
	_lvitem.mask = LVIF_TEXT;
	_lvitem.iItem = nIndex;
	_lvitem.iSubItem = nSubIndex;
	_lvitem.pszText = pText;
	return (SendMessage(LVM_SETITEM, 0, (LPARAM)&_lvitem) != -1);
}

bool JListView::GetItemText( UInt32 nIndex, UInt32 nSubIndex, char* pBuffer, UInt32 nSize )
{
	LVITEM _lvitem = {0};
	_lvitem.mask = LVIF_TEXT;
	_lvitem.iItem = nIndex;
	_lvitem.iSubItem = nSubIndex;
	_lvitem.pszText = pBuffer;
	_lvitem.cchTextMax = nSize;
	return (SendMessage(LVM_GETITEM, 0, (LPARAM)&_lvitem) == TRUE);
}

int JListView::GetSelectionMark()
{
	return SendMessage(LVM_GETSELECTIONMARK,0,0);
}

void JListView::DeleteAllItems()
{
	SendMessage(LVM_DELETEALLITEMS, 0, 0);
}

int JListView::GetFirstSelected()
{
	return SendMessage(LVM_GETNEXTITEM, -1, MAKELPARAM(LVIS_SELECTED, 0));
}

int JListView::GetNextSelected( int nIndex )
{
	return SendMessage(LVM_GETNEXTITEM, nIndex, MAKELPARAM(LVIS_SELECTED, 0));
}
