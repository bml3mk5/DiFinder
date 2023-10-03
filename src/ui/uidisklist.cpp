/// @file uidisklist.cpp
///
/// @brief ディスクリスト
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "uidisklist.h"
#include <wx/imaglist.h>
#include "mymenu.h"
#include "../main.h"
#include "uidiskattr.h"
#include "uifilelist.h"
#include "diskparambox.h"
#include "basicselbox.h"
#include "../basicfmt/basicparam.h"
#include "../basicfmt/basicfmt.h"
#include "../basicfmt/basicdiritem.h"
#include "../utils.h"


// icon
extern const char *hdd_16_xpm[];
extern const char *hd_part_16_xpm[];
extern const char *foldericon_open_xpm[];
extern const char *foldericon_close_xpm[];


const char **icons_for_tree[] = {
	hd_part_16_xpm,
	hdd_16_xpm,
	foldericon_close_xpm,
	foldericon_open_xpm,
	NULL
};
enum en_icons_for_tree {
	ICON_FOR_TREE_NONE = -1,
	ICON_FOR_TREE_SINGLE = 0,
	ICON_FOR_TREE_ROOT,
	ICON_FOR_TREE_CLOSE,
	ICON_FOR_TREE_OPEN,
};

//////////////////////////////////////////////////////////////////////
//
//
//
/// ツリー構造を保存する
/// @param [in] n_disknum   : ディスク番号(0から) ルートは-1
/// @param [in] n_typenum   : ディスク内にAB面があるとき-2、ディレクトリがあるとき-4、その他-1
/// @param [in] n_sidenum   : ディスク内にAB面があるときサイド番号、それ以外は-1
/// @param [in] n_pos       : ディレクトリ位置
/// @param [in] n_editable  : ディスク名の編集が可能か
/// @param [in] n_ditem     : DISK BASIC ディレクトリアイテム
UiDiskPositionData::UiDiskPositionData(int n_disknum, int n_typenum, int n_sidenum, int n_pos, bool n_editable, DiskBasicDirItem *n_ditem)
#ifndef USE_TREE_CTRL_ON_DISK_LIST
	: wxClientData()
#else
	: wxTreeItemData()
#endif
{
	disknum = n_disknum;
	typenum = n_typenum;
	sidenum = n_sidenum;
	pos = n_pos;
	editable = n_editable;
	ditem = n_ditem;
	shown = false;
}
UiDiskPositionData::~UiDiskPositionData()
{
}

//////////////////////////////////////////////////////////////////////

#ifndef USE_TREE_CTRL_ON_DISK_LIST
//
//
//
UiDiskTreeStoreModel::UiDiskTreeStoreModel(UiDiskFrame *parentframe)
	: wxDataViewTreeStore()
{
	frame = parentframe;
}
/// ディスク名編集できるか
bool UiDiskTreeStoreModel::IsEnabled(const wxDataViewItem &item, unsigned int col) const
{
#if defined(__WXOSX__)
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(item);
	if (!cd) return false;
	return cd->GetEditable();
#else
	return true;
#endif
}
/// ディスク名を変更した
bool UiDiskTreeStoreModel::SetValue(const wxVariant &variant, const wxDataViewItem &item, unsigned int col)
{
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(item);
	if (!cd || cd->GetDiskNumber() < 0 || cd->GetEditable() != true) return false;

	wxDataViewIconText data;
	data << variant;
	if (frame->GetDiskImage().SetDiskName(cd->GetDiskNumber(), data.GetText())) {
		SetItemText(item, frame->GetDiskImage().GetDiskName(cd->GetDiskNumber()));
	}
	return true;
}
#endif

//////////////////////////////////////////////////////////////////////
//
// ツリーコントロール
//
UiDiskTreeCtrl::UiDiskTreeCtrl(wxWindow *parentwindow, wxWindowID id)
#ifndef USE_TREE_CTRL_ON_DISK_LIST
	: MyCDTreeCtrl(parentwindow, id)
#else
	: MyCTreeCtrl(parentwindow, id)
#endif
{
}

//////////////////////////////////////////////////////////////////////
//
// Left Panel
//
// Attach Event
wxBEGIN_EVENT_TABLE(UiDiskList, UiDiskTreeCtrl)
#ifndef USE_TREE_CTRL_ON_DISK_LIST
	EVT_DATAVIEW_ITEM_CONTEXT_MENU(wxID_ANY, UiDiskList::OnContextMenu)

	EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, UiDiskList::OnSelectionChanged)

	EVT_DATAVIEW_ITEM_EXPANDING(wxID_ANY, UiDiskList::OnItemExpanding)
#else
	EVT_TREE_ITEM_MENU(wxID_ANY, UiDiskList::OnContextMenu)
	EVT_TREE_SEL_CHANGED(wxID_ANY, UiDiskList::OnSelectionChanged)
	EVT_TREE_ITEM_EXPANDING(wxID_ANY, UiDiskList::OnItemExpanding)
	EVT_TREE_BEGIN_LABEL_EDIT(wxID_ANY, UiDiskList::OnStartEditing)
	EVT_TREE_END_LABEL_EDIT(wxID_ANY, UiDiskList::OnEditingDone)
#endif

	EVT_MENU(IDM_DELETE_DIRECTORY, UiDiskList::OnDeleteDirectory)

	EVT_MENU(IDM_PROPERTY_FILE, UiDiskList::OnPropertyFile)
	EVT_MENU(IDM_PROPERTY_DISK, UiDiskList::OnPropertyDisk)
	EVT_MENU(IDM_PROPERTY_BASIC, UiDiskList::OnPropertyBasic)
wxEND_EVENT_TABLE()

UiDiskList::UiDiskList(UiDiskFrame *parentframe, wxWindow *parentwindow)
       : UiDiskTreeCtrl(parentwindow, wxID_ANY)
{
	parent   = parentwindow;
	frame    = parentframe;

	initialized = false;

	AssignTreeIcons( icons_for_tree );

#ifndef USE_TREE_CTRL_ON_DISK_LIST
	UiDiskTreeStoreModel *model = new UiDiskTreeStoreModel(parentframe);
	AssociateModel(model);
	model->DecRef();
#endif

	wxFont font;
	frame->GetDefaultListFont(font);
	SetFont(font);

	// fit size on parent window
    SetSize(parentwindow->GetClientSize());

	// popup menu
	MakePopupMenu();

	// key
	Bind(wxEVT_CHAR, &UiDiskList::OnChar, this);

	ClearFileName();

	initialized = true;
}

UiDiskList::~UiDiskList()
{
	// save ini file
	delete menuPopup;
}

/// 右クリック選択
/// @param[in] event リストイベント
void UiDiskList::OnContextMenu(UiDiskListEvent& event)
{
	UiDiskListItem item = event.GetItem();
	SelectTreeNode(item);
	ShowPopupMenu();
}

/// ツリーアイテム選択
/// @param[in] event リストイベント
void UiDiskList::OnSelectionChanged(UiDiskListEvent& event)
{
	UiDiskListItem item = event.GetItem();
	ChangeSelection(item);
}

/// アイテム展開
/// @param[in] event リストイベント
void UiDiskList::OnItemExpanding(UiDiskListEvent& event)
{
	UiDiskListItem item = event.GetItem();
	ExpandItemNode(item);
}

/// アイテム編集開始
/// @note wxDataViewTreeCtrl OSXでは発生しない
/// @param[in] event リストイベント
void UiDiskList::OnStartEditing(UiDiskListEvent& event)
{
	UiDiskListItem item = event.GetItem();
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(item);
	if (!cd || !cd->GetEditable()) {
		event.Veto();
	}
}

/// アイテム編集終了
/// @param[in] event リストイベント
void UiDiskList::OnEditingDone(UiDiskListEvent& event)
{
#ifdef USE_TREE_CTRL_ON_DISK_LIST
	if (event.IsEditCancelled()) return;
	UiDiskListItem item = event.GetItem();
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(item);
	if (!cd || cd->GetDiskNumber() < 0 || cd->GetEditable() != true) return;

	if (frame->GetDiskImage().SetDiskName(cd->GetDiskNumber(), event.GetLabel())) {
		SetItemText(item, frame->GetDiskImage().GetDiskName(cd->GetDiskNumber()));
	}
#endif
}

/// ディレクトリを削除選択
/// @param[in] event コマンドイベント
void UiDiskList::OnDeleteDirectory(wxCommandEvent& WXUNUSED(event))
{
	DeleteDirectory();
}

/// プロパティ選択
/// @param[in] event コマンドイベント
void UiDiskList::OnPropertyFile(wxCommandEvent& WXUNUSED(event))
{
	frame->ShowFileAttr();
}

/// プロパティ選択
/// @param[in] event コマンドイベント
void UiDiskList::OnPropertyDisk(wxCommandEvent& WXUNUSED(event))
{
	ShowDiskAttr();
}

/// BASIC情報選択
/// @param[in] event コマンドイベント
void UiDiskList::OnPropertyBasic(wxCommandEvent& WXUNUSED(event))
{
	frame->ShowBasicAttr();
}

/// キー入力
/// @param[in] event キーイベント
void UiDiskList::OnChar(wxKeyEvent& event)
{
	switch(event.GetKeyCode()) {
	case WXK_RETURN:
		frame->ShowFileAttr();
		break;
	case WXK_DELETE:
		SelectDeleting();
		break;
	default:
		event.Skip();
		break;
	}
}

/// ポップアップメニュー作成
void UiDiskList::MakePopupMenu()
{
	menuPopup = new wxMenu;
	menuPopup->Append(IDM_DELETE_DIRECTORY, _("De&lete Directory...") );
	menuPopup->AppendSeparator();
	menuPopup->Append(IDM_PROPERTY_FILE, _("Disk Information..."));
	menuPopup->Append(IDM_PROPERTY_DISK, _("Partition Information..."));
	menuPopup->Append(IDM_PROPERTY_BASIC, _("BASIC Information..."));
}

/// ポップアップメニュー表示
void UiDiskList::ShowPopupMenu()
{
	if (!menuPopup) return;

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(GetSelection());

	bool opened = (cd != NULL);

	opened = (opened && (selected_disk != NULL));
	menuPopup->Enable(IDM_PROPERTY_DISK, opened);

	opened = (opened && disk_selecting);
	menuPopup->Enable(IDM_PROPERTY_BASIC, opened && frame->CanUseBasicDisk());

	opened = (opened && cd->GetTypeNumber() == CD_TYPENUM_NODE_DIR);
	menuPopup->Enable(IDM_DELETE_DIRECTORY, opened);

	PopupMenu(menuPopup);
}

/// ディレクトリ/ディスク削除選択
void UiDiskList::SelectDeleting()
{
	UiDiskListItem node = GetSelection();
	if (!node.IsOk()) return;

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);
	if (!cd) return;

	if (cd->GetTypeNumber() == CD_TYPENUM_NODE_DIR) {
		// ディレクトリ
		frame->DeleteDirectory(selected_disk, cd->GetSideNumber(), cd->GetDiskBasicDirItem());
	}
}

/// フォーカスのあるディレクトリを削除
/// @return true:OK false:Error
bool UiDiskList::DeleteDirectory()
{
	UiDiskListItem node = GetSelection();
	if (!node.IsOk()) return false;

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);
	if (!cd) return false;

	if (cd->GetTypeNumber() == CD_TYPENUM_NODE_DIR) {
		// ディレクトリ
		return frame->DeleteDirectory(selected_disk, cd->GetSideNumber(), cd->GetDiskBasicDirItem());
	} else {
		return false;
	}
}

/// 再選択
/// @param [in] newparam BASICパラメータ 通常NULL BASICを変更した際に設定する 
void UiDiskList::ReSelect(const DiskBasicParam *newparam)
{
	UiDiskListItem item = GetSelection();
	ChangeSelection(item, newparam);
}

/// ツリーを選択
/// @param [in] node     選択したノード
/// @param [in] newparam BASICパラメータ 通常NULL BASICを変更した際に設定する 
void UiDiskList::ChangeSelection(UiDiskListItem &node, const DiskBasicParam *newparam)
{
	SetDataOnItemNode(node, NODE_SELECTED, newparam);
}

/// ディスクを指定して選択状態にする
/// @param [in] disk_number ディスク番号
/// @param [in] side_number サイド番号 両面なら-1
void UiDiskList::ChangeSelection(int disk_number, int side_number)
{
	UiDiskListItem match_node = FindNodeByDiskAndSideNumber(root_node, disk_number, side_number);
	if (match_node.IsOk()) {
		SelectTreeNode(match_node);
		ChangeSelection(match_node);
	}
}

/// ツリーを展開
/// @param [in] node     選択したノード
void UiDiskList::ExpandItemNode(UiDiskListItem &node)
{
	if (!node.IsOk()) return;

	UiDiskListItem sel_node = GetSelection();

	if (sel_node.GetID() != node.GetID()) {
		SetDataOnItemNode(node, NODE_EXPANDED);
	}
}

/// 指定ノードにデータを設定する
/// @param [in] node     選択したノード
/// @param [in] flag     NODE_SELECT / NODE_EXPANDED
/// @param [in] newparam BASICパラメータ 通常NULL BASICを変更した際に設定する 
void UiDiskList::SetDataOnItemNode(const UiDiskListItem &node, SetDataOnItemNodeFlags flag, const DiskBasicParam *newparam)
{
	if (!initialized) return;

	if (!node.IsOk()) {
		// rootアイテムを選択したらファイル一覧をクリア
		selected_disk = NULL;
		// 全パネルのデータをクリアする
		frame->ClearAllData();
		return;
	}

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);

	if (flag == NODE_SELECTED) disk_selecting = false;

	DiskImage *image = &frame->GetDiskImage();
	DiskImageFile *file = image->GetFile();

	if (cd == NULL || (TreeNodeHasChildren(node) && cd->GetDiskNumber() < 0)) {
		// rootアイテムを選択したらファイル一覧をクリア
		selected_disk = NULL;
		// 全パネルのデータをクリアする
		frame->ClearAllData();
		frame->SetDataOnFile(file);
		return;
	}

	DiskImageDisk *disk = image->GetDisk(cd->GetDiskNumber());
	if (!disk) {
		return;
	}

	// ディスク属性をセット
	if (flag == NODE_SELECTED) {
		selected_disk = disk;
		frame->SetDiskAttrData(disk);
	}

	int typenum = cd->GetTypeNumber();
	int sidenum = cd->GetSideNumber();
	// サイドA,Bがある場合
	if ((typenum == CD_TYPENUM_NODE_AB || typenum == CD_TYPENUM_NODE_BOTH) && sidenum < 0) {
		// RAWパネルだけデータをセットする
		frame->ClearAllAndSetRawData(disk, -2);
		if (flag == NODE_SELECTED) Expand(node);
		return;
	}

	// ディスク１枚 or ディレクトリを選択
	if (flag == NODE_SELECTED) disk_selecting = true;

	// サブディレクトリの場合
	if (typenum == CD_TYPENUM_NODE_DIR) {
		frame->AttachDiskBasicOnFileList(disk, sidenum);
		if (flag == NODE_SELECTED) {
			frame->ChangeDirectory(disk, sidenum, cd->GetDiskBasicDirItem(), true);
		} else {
			frame->AssignDirectory(disk, sidenum, cd->GetDiskBasicDirItem());
		}
		return;
	}

	// 右パネルにファイル名一覧を設定
	if (ParseDiskBasic(node, cd, disk, sidenum, newparam)) {
		frame->SetDataOnDisk(disk, sidenum, flag == NODE_SELECTED);
		RefreshRootDirectoryNode(disk, node);
	}
}

/// DISK BASICをアタッチ＆解析
/// @param [in] node       選択したノード
/// @param [in] cd         ノードの位置情報
/// @param [in] newdisk    ディスクイメージ
/// @param [in] newsidenum サイド番号 片面選択時0/1 両面なら-1
/// @param [in] newparam   BASICパラメータ 通常NULL BASICを変更した際に設定する
/// @return 片面のみ使用するBASICの場合false -> さらに下位ノードで解析
bool UiDiskList::ParseDiskBasic(const UiDiskListItem &node, UiDiskPositionData *cd, DiskImageDisk *newdisk, int newsidenum, const DiskBasicParam *newparam)
{
	DiskBasic *newbasic = newdisk->GetDiskBasic(newsidenum);

	newbasic->SetCharCode(frame->GetCharCode());

	// BASICモードのときはディスクを解析
	if (frame->GetSelectedMode() == 0) {
		bool valid = false;
		// ディスクをDISK BASICとして解析
		valid = (newbasic->ParseDisk(newdisk, newsidenum, newparam, false) == 0);

		// ルートディレクトリをセット
		if (valid) {
			valid = newbasic->AssignRootDirectory();
		}

		// エラーメッセージ
		if (!valid) {
			newbasic->ShowErrorMessage();
		}
	}

	return true;
}

/// サブキャプション
/// 表裏を入れ替えるようなディスクの場合
/// @param[in] type        タイプ
/// @param[in] side_number サイド番号
/// @param[out] caption    キャプション
void UiDiskList::SubCaption(int type, int side_number, wxString &caption) const
{
	if (side_number < 0) return;

	caption = Utils::GetSideStr(side_number, type != CD_TYPENUM_NODE_AB);
}

/// 選択しているディスクの子供を削除
void UiDiskList::DeleteChildrenOnSelectedDisk()
{
	UiDiskListItem node = GetSelection();
	if (!node.IsOk()) return;

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);
	if (!cd) return;

	int disk_number = cd->GetDiskNumber();
	node = FindNodeByDiskNumber(root_node, disk_number); 
	if (!node.IsOk()) return;

	cd = (UiDiskPositionData *)GetItemData(node);

	Collapse(node);
	DeleteChildren(node);
	cd->SetTypeNumber(CD_TYPENUM_NODE);
	cd->Shown(false);
	SelectTreeNode(node);
}

/// 選択しているディスクのルートを初期化＆再選択
/// @param [in] newparam BASICパラメータ 通常NULL BASICを変更した際に設定する 
void UiDiskList::RefreshSelectedDisk(const DiskBasicParam *newparam)
{
	// 子供を削除
	DeleteChildrenOnSelectedDisk();
	// 再選択
	ReSelect(newparam);
}

/// 選択しているサイドを再選択
/// @param [in] newparam BASICパラメータ 通常NULL BASICを変更した際に設定する 
void UiDiskList::RefreshSelectedSide(const DiskBasicParam *newparam)
{
	// 再選択
	ReSelect(newparam);
}

/// ファイル名をリストにセット
void UiDiskList::SetFileName()
{
	SetFileName(frame->GetFileName());
}

/// ファイル名をリストにセット
/// @param [in] filename ファイル名
void UiDiskList::SetFileName(const wxString &filename)
{
	DiskImage *image = &frame->GetDiskImage();
	if (!image) return;
	DiskImageDisks *disks = image->GetDisks();
	if (!disks) return;

	DeleteAllItems();

	UiDiskListItem node = AddRootTreeNode(filename, ICON_FOR_TREE_ROOT, ICON_FOR_TREE_NONE
		, new UiDiskPositionData(CD_DISKNUM_ROOT, CD_TYPENUM_NODE, -1, 0, false));
	root_node = node;

	for(size_t i=0; i<disks->Count(); i++) {
		DiskImageDisk *diskn = disks->Item(i);
		// ディスク１つ
		AddTreeContainer(node, diskn->GetName(), ICON_FOR_TREE_SINGLE, ICON_FOR_TREE_NONE
			, new UiDiskPositionData((int)i, CD_TYPENUM_NODE, CD_TYPENUM_NODE, CD_TYPENUM_NODE, true));
	}

	Expand(node);
	SelectTreeNode(node);
}

/// リストをクリア
void UiDiskList::ClearFileName()
{
	DeleteAllItems();

	UiDiskListItem node = AddRootTreeNode( _("(none)") );
	root_node = node;

	Expand(node);

	selected_disk = NULL;
	disk_selecting = false;
}

/// ファイルパスをリストにセット
/// @param [in] filename ファイル名
void UiDiskList::SetFilePath(const wxString &filename)
{
	if (!filename.IsEmpty()) {
		SetItemText(root_node, filename);
	} else {
		SetItemText(root_node, _("(none)"));
	}
}

/// ディレクトリアイテムと一致するノードをさがす
/// @attention 再帰的に呼ばれる。 This function is called recursively.
/// @param [in] node        ノード
/// @param [in] disk_number ディスク番号
/// @param [in] tag_item    対象ディレクトリアイテム
/// @param [in] depth       深さ
/// @return 一致したノード
UiDiskListItem UiDiskList::FindNodeByDirItem(const UiDiskListItem &node, int disk_number, DiskBasicDirItem *tag_item, int depth)
{
	UiDiskListItem match_node;

	if (depth < 100 && TreeNodeHasChildren(node)) {
		UiDiskTreeIdVal cookie;
		UiDiskListItem child_node = GetFirstChild(node, cookie);
		while(child_node.IsOk() && !match_node.IsOk()) {
			UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(child_node);
			if (cd) {
				// ディスク番号を比較
				if (cd->GetDiskNumber() == disk_number) {
					// ディレクトリアイテムのポインタを比較
					if (cd->GetDiskBasicDirItem() == tag_item) {
						match_node = child_node;
						break;
					} else if (TreeNodeHasChildren(child_node)) {
						// 再帰的に探す
						match_node = FindNodeByDirItem(child_node, disk_number, tag_item, depth + 1);
					}
				}
			}
			child_node = GetNextChild(node, cookie);
		}
	}
	return match_node;
}

/// ディスク番号と一致するノードをさがす
/// @param [in] node        ノード
/// @param [in] disk_number ディスク番号
/// @param [in] depth       深さ
/// @return 一致したノード
UiDiskListItem UiDiskList::FindNodeByDiskNumber(const UiDiskListItem &node, int disk_number, int depth)
{
	UiDiskListItem match_node;

	if (depth < 100 && TreeNodeHasChildren(node)) {
		UiDiskTreeIdVal cookie;
		UiDiskListItem child_node = GetFirstChild(node, cookie);
		while(child_node.IsOk() && !match_node.IsOk()) {
			UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(child_node);
			if (cd) {
				// ディスク番号を比較
				if (cd->GetDiskNumber() == disk_number) {
					match_node = child_node;
					break;
				}
			}
			child_node = GetNextChild(node, cookie);
		}
	}
	return match_node;
}

/// ディスク番号と一致するノードをさがす
/// @attention 再帰的に呼ばれる。 This function is called recursively.
/// @param [in] node        ノード
/// @param [in] disk_number ディスク番号
/// @param [in] side_number サイド番号 両面なら-1
/// @param [in] depth       深さ
/// @return 一致したノード
UiDiskListItem UiDiskList::FindNodeByDiskAndSideNumber(const UiDiskListItem &node, int disk_number, int side_number, int depth)
{
	UiDiskListItem match_node;

	if (depth < 100 && TreeNodeHasChildren(node)) {
		UiDiskTreeIdVal cookie;
		UiDiskListItem child_node = GetFirstChild(node, cookie);
		while(child_node.IsOk() && !match_node.IsOk()) {
			UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(child_node);
			if (cd) {
				// ディスク番号を比較
				if (cd->GetDiskNumber() == disk_number) {
					if (cd->GetSideNumber() == side_number) {
						match_node = child_node;
						break;
					} else if (TreeNodeHasChildren(child_node)) {
						// 再帰的に探す
						match_node = FindNodeByDiskAndSideNumber(child_node, disk_number, side_number, depth + 1);
					}
				}
			}
			child_node = GetNextChild(node, cookie);
		}
	}
	return match_node;
}

/// ルートディレクトリを更新
/// @param [in] disk        ディスク
/// @param [in] side_number サイド番号
void UiDiskList::RefreshRootDirectoryNode(DiskImageDisk *disk, int side_number)
{
	UiDiskListItem node = FindNodeByDiskAndSideNumber(root_node, disk->GetNumber(), side_number);
	if (!node.IsOk()) return;

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);
	DiskBasic *basic = disk->GetDiskBasic(side_number);
	if (!basic) return;
	DiskBasicDirItem *root_item = basic->GetRootDirectory();
	if (!root_item) return;
	RefreshDirectorySub(disk, node, cd, root_item);
}

/// ルートディレクトリを更新
/// @param [in] disk      ディスク
/// @param [in] node      ノード
void UiDiskList::RefreshRootDirectoryNode(DiskImageDisk *disk, const UiDiskListItem &node)
{
	if (!node.IsOk()) return;

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);
	DiskBasic *basic = disk->GetDiskBasic(cd->GetSideNumber());
	if (!basic) return;
	DiskBasicDirItem *root_item = basic->GetRootDirectory();
	cd->SetDiskBasicDirItem(root_item);
	if (!root_item) return;
	RefreshDirectorySub(disk, node, cd, root_item);
}

/// ディレクトリノードを更新
/// @param [in] disk     ディスク
/// @param [in] dir_item 対象ディレクトリアイテム
void UiDiskList::RefreshDirectoryNode(DiskImageDisk *disk, DiskBasicDirItem *dir_item)
{
	UiDiskListItem node = FindNodeByDirItem(root_node, disk->GetNumber(), dir_item);
	if (!node.IsOk()) return;

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);

	RefreshDirectorySub(disk, node, cd, dir_item);
}

/// 全てのディレクトリを更新
/// @param [in] disk        ディスク
/// @param [in] side_number サイド番号
void UiDiskList::RefreshAllDirectoryNodes(DiskImageDisk *disk, int side_number)
{
	if (!disk) return;
	DiskBasic *basic = disk->GetDiskBasic(side_number);
	if (!basic) return;
	DiskBasicDirItem *dir_item = basic->GetCurrentDirectory();
	DiskBasicDirItem *root_item = basic->GetRootDirectory();
	if (root_item == dir_item) {
		root_item->ValidDirectory(false);
		// ルートツリーを更新
		frame->RefreshRootDirectoryNodeOnDiskList(disk, side_number);
		root_item->ValidDirectory(true);
	} else {
		dir_item->ValidDirectory(false);
		// ディレクトリツリーを更新
		frame->RefreshDirectoryNodeOnDiskList(disk, dir_item);
		dir_item->ValidDirectory(true);
	}
}

/// ディレクトリノードを選択
/// @param [in] disk     ディスク
/// @param [in] dir_item 対象ディレクトリアイテム
void UiDiskList::SelectDirectoryNode(DiskImageDisk *disk, DiskBasicDirItem *dir_item)
{
	UiDiskListItem node = FindNodeByDirItem(root_node, disk->GetNumber(), dir_item);
	if (!node.IsOk()) return;

	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);

	RefreshDirectorySub(disk, node, cd, dir_item);

	SelectTreeNode(node);
}

/// ディレクトリノードを削除
/// @param [in] disk      ディスク
/// @param [in] dir_item  対象ディレクトリアイテム
void UiDiskList::DeleteDirectoryNode(DiskImageDisk *disk, DiskBasicDirItem *dir_item)
{
	UiDiskListItem node = FindNodeByDirItem(root_node, disk->GetNumber(), dir_item);
	if (!node.IsOk()) return;

	UiDiskListItem parent_node = GetParentTreeNode(node);

	DeleteTreeNode(node);

	if (!parent_node.IsOk()) return;

	SelectTreeNode(parent_node);
	ChangeSelection(parent_node);
}

/// ディレクトリノードを一括削除
/// @param [in] disk      ディスク
/// @param [in] dir_items 対象ディレクトリアイテム
void UiDiskList::DeleteDirectoryNodes(DiskImageDisk *disk, DiskBasicDirItems &dir_items)
{
	for(size_t i=0; i<dir_items.Count(); i++) {
		DiskBasicDirItem *dir_item = dir_items.Item(i);

		UiDiskListItem node = FindNodeByDirItem(root_node, disk->GetNumber(), dir_item);
		if (!node.IsOk()) continue;

		DeleteTreeNode(node);
	}
}

/// ディレクトリを更新
/// @param [in] disk      ディスク
/// @param [in] node      ノード
/// @param [in] cd        ノード情報
/// @param [in] dir_item  対象ディレクトリアイテム
void UiDiskList::RefreshDirectorySub(DiskImageDisk *disk, const UiDiskListItem &node, UiDiskPositionData *cd, DiskBasicDirItem *dir_item)
{
	if (dir_item->IsValidDirectory() && cd->IsShown()) return;

	bool expanded = IsExpanded(node);

	DeleteChildren(node);

	// ディレクトリ一覧を設定
	DiskBasicDirItems *dir_items = dir_item->GetChildren();
	if (dir_items) {
		for(size_t i=0; i<dir_items->Count(); i++) {
			DiskBasicDirItem *ditem = dir_items->Item(i);
			if (!ditem->IsUsedAndVisible()) continue;

			if (ditem->IsDirectory()) {
				AppendDirectory(node, ditem);
			}
		}
	}

	if (expanded) Expand(node);

	cd->Shown(true);
}

/// ディレクトリを追加
/// @param [in] parent    親ノード
/// @param [in] dir_item  対象ディレクトリアイテム
UiDiskListItem UiDiskList::AppendDirectory(const UiDiskListItem &parent, DiskBasicDirItem *dir_item)
{
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(parent);
	int disk_number = cd->GetDiskNumber();
	int side_number = cd->GetSideNumber();
	wxString name = dir_item->GetFileNameStr();
	if (!dir_item->IsVisibleOnTree()) return UiDiskListItem();

	int pos = GetTreeChildCount(parent);
	UiDiskListItem newitem = AddTreeContainer(parent, name, ICON_FOR_TREE_CLOSE, ICON_FOR_TREE_OPEN
		,new UiDiskPositionData(disk_number, CD_TYPENUM_NODE_DIR, side_number, pos, false, dir_item));

	return newitem;
}

/// ツリービューのディレクトリ名を再設定（キャラクターコードを変更した時）
/// @param [in] disk      ディスク
void UiDiskList::RefreshDirectoryName(DiskImageDisk *disk)
{
	if (!disk) return;

	RefreshDirectoryName(root_node, disk->GetNumber());
}

/// ツリービューのディレクトリ名を再設定
/// @attention 再帰的に呼ばれる。 This function is called recursively.
/// @param [in] node        ノード
/// @param [in] disk_number ディスク番号
/// @param [in] depth       深さ
void UiDiskList::RefreshDirectoryName(const UiDiskListItem &node, int disk_number, int depth)
{
	if (depth < 100 && TreeNodeHasChildren(node)) {
		UiDiskTreeIdVal cookie;
		UiDiskListItem child_node = GetFirstChild(node, cookie);
		while(child_node.IsOk()) {
			UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(child_node);
			if (cd) {
				// ディスク番号を比較
				if (cd->GetDiskNumber() == disk_number) {
					if (cd->GetTypeNumber() == CD_TYPENUM_NODE_DIR) {
						// ディレクトリアイテムの名前を再設定
						DiskBasicDirItem *dir_item = cd->GetDiskBasicDirItem();
						if (dir_item) {
							SetItemText(child_node, dir_item->GetFileNameStr());
						}
					}
					// 再帰的に探す
					RefreshDirectoryName(child_node, disk_number, depth + 1);
				}
			}
			child_node = GetNextChild(node, cookie);
		}
	}
}

/// ディスク名を変更
void UiDiskList::RenameDisk()
{
	UiDiskListItem node = SetSelectedItemAtDiskImage();
	if (!node.IsOk()) return;
	EditTreeNode(node);
}

/// パーティション情報ダイアログ
void UiDiskList::ShowDiskAttr()
{
	if (!selected_disk) return;

	wxString str = selected_disk->GetDescription();
	wxMessageBox(str, _("Partition Information"));
}

/// 選択位置のディスクイメージ
UiDiskListItem UiDiskList::SetSelectedItemAtDiskImage()
{
	UiDiskListItem invalid;
	if (!selected_disk) return invalid;
	UiDiskListItem node = GetSelection();
	if (!node.IsOk()) return invalid;
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(node);
	if (!cd) return invalid;
	if (cd->GetPosition() >= 0) {
		// ディスク名は親アイテムになる
		node = GetParentTreeNode(node);
		if (!node.IsOk()) return invalid;
		UiDiskPositionData *pcd = (UiDiskPositionData *)GetItemData(node);
		if (pcd->GetDiskNumber() != cd->GetDiskNumber()) return invalid;
	}
	return node;
}

/// 選択位置のディスク名をセット
/// @param[in] val ディスク名
void UiDiskList::SetDiskName(const wxString &val)
{
	UiDiskListItem item = SetSelectedItemAtDiskImage();
	if (!item.IsOk()) return;
	SetItemText(item, val);
}

/// キャラクターコード変更
/// @param[in] name コード名
void UiDiskList::ChangeCharCode(const wxString &name)
{
	DiskImage *image = &frame->GetDiskImage();
	if (!image) return;
	DiskImageDisks *disks = image->GetDisks();
	if (!disks) return;
	for(size_t i=0; i<disks->Count(); i++) {
		RefreshDirectoryName(disks->Item(i));
	}
}

/// フォントをセット
/// @param[in] font フォント
void UiDiskList::SetListFont(const wxFont &font)
{
	SetFont(font);
	Refresh();
}

/// 選択しているディスクイメージのディスク番号を返す
int UiDiskList::GetSelectedDiskNumber()
{
	if (!selected_disk) return wxNOT_FOUND;
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(GetSelection());
	if (!cd) return wxNOT_FOUND;
	return cd->GetDiskNumber();
}
/// 選択しているディスクイメージのサイド番号を返す
int UiDiskList::GetSelectedDiskSide()
{
	if (!selected_disk) return wxNOT_FOUND;
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(GetSelection());
	if (!cd) return wxNOT_FOUND;
	return cd->GetPosition();
}
/// 選択しているディスクイメージのディスク番号とサイド番号を返す
/// @param[out] disk_number ディスク番号
/// @param[out] side_number サイド番号
void UiDiskList::GetSelectedDisk(int &disk_number, int &side_number)
{
	if (!selected_disk) return;
	UiDiskPositionData *cd = (UiDiskPositionData *)GetItemData(GetSelection());
	if (!cd) return;
	disk_number = cd->GetDiskNumber();
	side_number = cd->GetPosition();
}

/// ディスクイメージを選択しているか
bool UiDiskList::IsSelectedDiskImage()
{
	return (selected_disk != NULL);
}

/// ディスクを選択しているか
bool UiDiskList::IsSelectedDisk()
{
	return disk_selecting;
}

/// ディスクを選択しているか(AB面どちらか)
bool UiDiskList::IsSelectedDiskSide()
{
	return false;
}
