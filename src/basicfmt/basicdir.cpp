/// @file basicdir.cpp
///
/// @brief disk basic directory
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "basicdir.h"
#include "basicdiritem_msdos.h"
#include "basicdiritem_os9.h"
#include "basicdiritem_hu68k.h"
#include "basicfmt.h"
#include "basictype.h"
#include "../charcodes.h"


//
//
//
DiskBasicDir::DiskBasicDir(DiskBasic *basic)
{
	this->basic = basic;
	this->fat = basic->GetFat();

	this->root = NULL;

	this->format_type = NULL;
	this->current_item = NULL;
}
DiskBasicDir::~DiskBasicDir()
{
	delete root;
}
/// ディレクトリアイテムを新規に作成
DiskBasicDirItem *DiskBasicDir::NewItem()
{
	DiskBasicDirItem *item = NULL;

	int num = FORMAT_TYPE_UNKNOWN;
	if (format_type) num = format_type->GetTypeNumber();

	switch(num) {
	case FORMAT_TYPE_MSDOS:
		item = new DiskBasicDirItemVFAT(basic);
		break;
	case FORMAT_TYPE_OS9:
		item = new DiskBasicDirItemOS9(basic);
		break;
	case FORMAT_TYPE_HU68K:
		item = new DiskBasicDirItemHU68K(basic);
		break;
	default:
		wxFAIL_MSG(wxT("Unknown type is defined in basic_type.xml."));
		break;
	}
	if (item) {
		item->ClearData();
	}
	return item;
}
#if 0
/// ディレクトリアイテムを新規に作成してアサインする
/// @param [in]  n_block_num  パーティション内のセクタ番号
/// @param [in]  n_position   セクタ内の位置
/// @param [in]  n_data       セクタ内のバッファ
DiskBasicDirItem *DiskBasicDir::NewItem(int n_block_num, int n_position, wxUint8 *n_data)
{
	DiskBasicDirItem *item = NULL;

	int num = FORMAT_TYPE_UNKNOWN;
	if (format_type) num = format_type->GetTypeNumber();

	switch(num) {
	case FORMAT_TYPE_MSDOS:
		item = new DiskBasicDirItemVFAT(basic, n_block_num, n_position, n_data);
		break;
	case FORMAT_TYPE_OS9:
		item = new DiskBasicDirItemOS9(basic, n_block_num, n_position, n_data);
		break;
	case FORMAT_TYPE_HU68K:
		item = new DiskBasicDirItemHU68K(basic, n_block_num, n_position, n_data);
		break;
	default:
		wxFAIL_MSG(wxT("Unknown type is defined in basic_type.xml."));
		break;
	}
	return item;
}
/// ディレクトリアイテムを新規に作成してアサインする
/// @param [in]  n_num        通し番号
/// @param [in]  n_gitem      トラック番号などのデータ
/// @param [in]  n_block_num  パーティション内のセクタ番号
/// @param [in]  n_position   セクタ内での位置
/// @param [in]  n_data       セクタ内のバッファ
/// @param [in]  n_next       次のセクタ
/// @param [out] n_unuse      未使用か
DiskBasicDirItem *DiskBasicDir::NewItem(int n_num, const DiskBasicGroupItem *n_gitem, int n_block_num, int n_position, wxUint8 *n_data, const int *n_next, bool &n_unuse)
{
	DiskBasicDirItem *item = NULL;

	int num = FORMAT_TYPE_UNKNOWN;
	if (format_type) num = format_type->GetTypeNumber();

	switch(num) {
	case FORMAT_TYPE_MSDOS:
		item = new DiskBasicDirItemVFAT(basic, n_num, n_gitem, n_block_num, n_position, n_data, n_next, n_unuse);
		break;
	case FORMAT_TYPE_OS9:
		item = new DiskBasicDirItemOS9(basic, n_num, n_gitem, n_block_num, n_position, n_data, n_next, n_unuse);
		break;
	case FORMAT_TYPE_HU68K:
		item = new DiskBasicDirItemHU68K(basic, n_num, n_gitem, n_block_num, n_position, n_data, n_next, n_unuse);
		break;
	default:
		wxFAIL_MSG(wxT("Unknown type is defined in basic_type.xml."));
		break;
	}
	return item;
}
#endif
/// ディレクトリアイテムを新規に作成してアサインする
/// @param [in]  n_block_num  パーティション内のセクタ番号
/// @param [in]  n_position   セクタ内の位置
DiskBasicDirItem *DiskBasicDir::NewItem(int n_block_num, int n_position)
{
	DiskBasicDirItem *item = NULL;

	int num = FORMAT_TYPE_UNKNOWN;
	if (format_type) num = format_type->GetTypeNumber();

	switch(num) {
	case FORMAT_TYPE_MSDOS:
		item = new DiskBasicDirItemVFAT(basic, n_block_num, n_position);
		break;
	case FORMAT_TYPE_OS9:
		item = new DiskBasicDirItemOS9(basic, n_block_num, n_position);
		break;
	case FORMAT_TYPE_HU68K:
		item = new DiskBasicDirItemHU68K(basic, n_block_num, n_position);
		break;
	default:
		wxFAIL_MSG(wxT("Unknown type is defined in basic_type.xml."));
		break;
	}
	return item;
}
/// ディレクトリアイテムを新規に作成してアサインする
/// @param [in]  n_num        通し番号
/// @param [in]  n_gitem      トラック番号などのデータ
/// @param [in]  n_block_num  パーティション内のセクタ番号
/// @param [in]  n_position   セクタ内での位置
/// @param [in]  n_next       次のセクタ
/// @param [out] n_unuse      未使用か
DiskBasicDirItem *DiskBasicDir::NewItem(int n_num, const DiskBasicGroupItem *n_gitem, int n_block_num, int n_position, const int *n_next, bool &n_unuse)
{
	DiskBasicDirItem *item = NULL;

	int num = FORMAT_TYPE_UNKNOWN;
	if (format_type) num = format_type->GetTypeNumber();

	switch(num) {
	case FORMAT_TYPE_MSDOS:
		item = new DiskBasicDirItemVFAT(basic, n_num, n_gitem, n_block_num, n_position, n_next, n_unuse);
		break;
	case FORMAT_TYPE_OS9:
		item = new DiskBasicDirItemOS9(basic, n_num, n_gitem, n_block_num, n_position, n_next, n_unuse);
		break;
	case FORMAT_TYPE_HU68K:
		item = new DiskBasicDirItemHU68K(basic, n_num, n_gitem, n_block_num, n_position, n_next, n_unuse);
		break;
	default:
		wxFAIL_MSG(wxT("Unknown type is defined in basic_type.xml."));
		break;
	}
	return item;
}
/// ルートディレクトリのアイテムを返す
DiskBasicDirItem *DiskBasicDir::GetRootItem() const
{
	return root;
}
/// ルートディレクトリの一覧を返す
DiskBasicDirItems *DiskBasicDir::GetRootItems(DiskBasicDirItem **dir_item)
{
	DiskBasicDirItem *item = GetRootItem();
	if (!item) return NULL;
	if (dir_item) *dir_item = item;
	return item->GetChildren();
}
/// カレントディレクトリのアイテムを返す
DiskBasicDirItem *DiskBasicDir::GetCurrentItem() const
{
	return current_item;
}
/// カレントディレクトリの一覧を返す
DiskBasicDirItems *DiskBasicDir::GetCurrentItems(DiskBasicDirItem **dir_item)
{
	DiskBasicDirItem *item = GetCurrentItem();
	if (!item) return NULL;
	if (dir_item) *dir_item = item;
	return item->GetChildren();
}
/// カレントディレクトリの全ディレクトリアイテムをクリア
void DiskBasicDir::EmptyCurrent()
{
	DiskBasicDirItem *item = GetCurrentItem();
	if (item) item->EmptyChildren();
}
/// ルートをカレントにする
void DiskBasicDir::SetCurrentAsRoot()
{
	current_item = root;
}
/// 親ディレクトリのアイテムを返す
DiskBasicDirItem *DiskBasicDir::GetParentItem() const
{
	DiskBasicDirItem *item = NULL;
	if (current_item) {
		item = current_item->GetParent();
	}
	return item;
}

/// ディレクトリアイテムのポインタを返す
/// @param [in] idx インデックス
DiskBasicDirItem *DiskBasicDir::ItemPtr(size_t idx)
{
	DiskBasicDirItems *items = GetCurrentItems();
	if (!items || idx >= items->Count()) return NULL;
	return items->Item(idx);
}
/// カレントディレクトリ内で未使用のディレクトリアイテムを返す
/// @param [in,out] pitem     ファイル名、属性を持っている仮ディレクトリアイテム
/// @param [out]    next_item 未使用アイテムの次位置にあるアイテム
/// @return NULL:空きなし
DiskBasicDirItem *DiskBasicDir::GetEmptyItemOnCurrent(DiskBasicDirItem *pitem, DiskBasicDirItem **next_item)
{
	return GetEmptyItem(GetCurrentItem(), GetCurrentItems(), pitem, next_item);
}

/// ルートディレクトリ内で未使用のディレクトリアイテムを返す
/// @param [in,out] pitem     ファイル名、属性を持っている仮ディレクトリアイテム
/// @param [out]    next_item 未使用アイテムの次位置にあるアイテム
/// @return NULL:空きなし
DiskBasicDirItem *DiskBasicDir::GetEmptyItemOnRoot(DiskBasicDirItem *pitem, DiskBasicDirItem **next_item)
{
	return GetEmptyItem(GetRootItem(), GetRootItems(), pitem, next_item);
}

/// 未使用のディレクトリアイテムを返す
/// @param [in,out] parent    ディレクトリ
/// @param [in,out] items     ディレクトリアイテム一覧
/// @param [in,out] pitem     ファイル名、属性を持っている仮ディレクトリアイテム
/// @param [out]    next_item 未使用アイテムの次位置にあるアイテム
/// @return NULL:空きなし
DiskBasicDirItem *DiskBasicDir::GetEmptyItem(DiskBasicDirItem *parent, DiskBasicDirItems *items, DiskBasicDirItem *pitem, DiskBasicDirItem **next_item)
{
	DiskBasicType *type = basic->GetType();
	return type->GetEmptyDirectoryItem(parent, items, pitem, next_item);
}

/// 現在のディレクトリ内に同じファイル名が既に存在するか
/// @param [in]  filename     ファイル名
/// @param [in]  icase        大文字小文字を区別しないか(case insensitive)
/// @param [in]  exclude_item 検索対象から除くアイテム
/// @param [out] next_item    一致したアイテムの次位置にあるアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindFile(const DiskBasicFileName &filename, bool icase, DiskBasicDirItem *exclude_item, DiskBasicDirItem **next_item)
{
	return FindFile(GetCurrentItem(), filename, icase, exclude_item, next_item);
}

/// 指定したディレクトリ内に同じファイル名が既に存在するか
/// @param [in]  dir_item     検索対象のディレクトリアイテム
/// @param [in]  filename     ファイル名
/// @param [in]  icase        大文字小文字を区別しないか(case insensitive)
/// @param [in]  exclude_item 検索対象から除くアイテム
/// @param [out] next_item    一致したアイテムの次位置にあるアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindFile(const DiskBasicDirItem *dir_item, const DiskBasicFileName &filename, bool icase, DiskBasicDirItem *exclude_item, DiskBasicDirItem **next_item)
{
	DiskBasicDirItem *match_item = NULL;
	const DiskBasicDirItems *items = dir_item->GetChildren();
	if (items) {
		for(size_t pos = 0; pos <items->Count(); pos++) {
			DiskBasicDirItem *item = items->Item(pos);
			if (item != exclude_item && item->IsSameFileName(filename, icase)) {
				match_item = item;
				if (next_item) {
					pos++;
					if (pos < items->Count()) {
						*next_item = items->Item(pos);
					} else {
						*next_item = NULL;
					}
				}
				break;
			}
		}
	}
	return match_item;
}

/// 現在のディレクトリ内に同じファイル名が既に存在するか
/// @param [in]  target_item  検索対象アイテム
/// @param [in]  icase        大文字小文字を区別しないか(case insensitive)
/// @param [in]  exclude_item 検索対象から除くアイテム
/// @param [out] next_item    一致したアイテムの次位置にあるアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindFile(const DiskBasicDirItem *target_item, bool icase, DiskBasicDirItem *exclude_item, DiskBasicDirItem **next_item)
{
	return FindFile(GetCurrentItem(), target_item, icase, exclude_item, next_item);
}

/// 指定したディレクトリ内に同じファイル名が既に存在するか
/// @param [in]  dir_item     検索対象のディレクトリアイテム
/// @param [in]  target_item  検索対象アイテム
/// @param [in]  icase        大文字小文字を区別しないか(case insensitive)
/// @param [in]  exclude_item 検索対象から除くアイテム
/// @param [out] next_item    一致したアイテムの次位置にあるアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindFile(const DiskBasicDirItem *dir_item, const DiskBasicDirItem *target_item, bool icase, DiskBasicDirItem *exclude_item, DiskBasicDirItem **next_item)
{
	DiskBasicDirItem *match_item = NULL;
	const DiskBasicDirItems *items = dir_item->GetChildren();
	if (items) {
		for(size_t pos = 0; pos <items->Count(); pos++) {
			DiskBasicDirItem *item = items->Item(pos);
			if (item != exclude_item && item->IsSameFileName(target_item, icase)) {
				match_item = item;
				if (next_item) {
					pos++;
					if (pos < items->Count()) {
						*next_item = items->Item(pos);
					} else {
						*next_item = NULL;
					}
				}
				break;
			}
		}
	}
	return match_item;
}

/// 現在のディレクトリ内に同じファイル名(拡張子除く)が既に存在するか
/// @param [in]  name         ファイル名
/// @param [in]  icase        大文字小文字を区別しないか(case insensitive)
/// @param [in]  exclude_item 検索対象から除くアイテム
/// @param [out] next_item    一致したアイテムの次位置にあるアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindName(const wxString &name, bool icase, DiskBasicDirItem *exclude_item, DiskBasicDirItem **next_item)
{
	return FindName(GetCurrentItem(), name, icase, exclude_item, next_item);
}

/// 指定したディレクトリ内に同じファイル名(拡張子除く)が既に存在するか
/// @param [in]  dir_item     検索対象のディレクトリアイテム
/// @param [in]  name         ファイル名
/// @param [in]  icase        大文字小文字を区別しないか(case insensitive)
/// @param [in]  exclude_item 検索対象から除くアイテム
/// @param [out] next_item    一致したアイテムの次位置にあるアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindName(const DiskBasicDirItem *dir_item, const wxString &name, bool icase, DiskBasicDirItem *exclude_item, DiskBasicDirItem **next_item)
{
	DiskBasicDirItem *match_item = NULL;
	const DiskBasicDirItems *items = dir_item->GetChildren();
	if (items) {
		for(size_t pos = 0; pos <items->Count(); pos++) {
			DiskBasicDirItem *item = items->Item(pos);
			if (item != exclude_item && item->IsSameName(name, icase)) {
				match_item = item;
				if (next_item) {
					pos++;
					if (pos < items->Count()) {
						*next_item = items->Item(pos);
					} else {
						*next_item = NULL;
					}
				}
				break;
			}
		}
	}
	return match_item;
}

/// 現在のディレクトリ内の属性に一致するファイルを検索
/// @param [in]  file_type 検索対象の属性
/// @param [in]  mask      検索対象外にするビットマスク
/// @param [in]  prev_item 前回一致したアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindFileByAttr(int file_type, int mask, DiskBasicDirItem *prev_item)
{
	return FindFileByAttr(GetCurrentItem(), file_type, mask, prev_item);
}

/// ルートディレクトリ内の属性に一致するファイルを検索
/// @param [in]  file_type 検索対象の属性
/// @param [in]  mask      検索対象外にするビットマスク
/// @param [in]  prev_item 前回一致したアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindFileByAttrOnRoot(int file_type, int mask, DiskBasicDirItem *prev_item)
{
	return FindFileByAttr(GetRootItem(), file_type, mask, prev_item);
}

/// 指定したディレクトリ内の属性に一致するファイルを検索
/// @param [in]  dir_item  検索対象のディレクトリアイテム
/// @param [in]  file_type 検索対象の属性
/// @param [in]  mask      検索対象外にするビットマスク
/// @param [in]  prev_item 前回一致したアイテム
/// @return NULL: ない
DiskBasicDirItem *DiskBasicDir::FindFileByAttr(const DiskBasicDirItem *dir_item, int file_type, int mask, DiskBasicDirItem *prev_item)
{
	if (!dir_item) return NULL;

	DiskBasicDirItem *match_item = NULL;
	size_t start = 0;
	const DiskBasicDirItems *items = dir_item->GetChildren();
	if (items) {
		if (prev_item) {
			start = (size_t)items->Index(prev_item);
			start++;
		}
		for(size_t pos = start; pos <items->Count(); pos++) {
			DiskBasicDirItem *item = items->Item(pos);
			if (item->GetFileAttr().MatchType(mask, file_type & mask)) {
				match_item = item;
				break;
			}
		}
	}
	return match_item;
}

/// ルートディレクトリのチェック
/// @param [in] type          DISK BASIC 種類
/// @param [in] start_sector  開始セクタ番号
/// @param [in] end_sector    終了セクタ番号
/// @param [in] is_formatting フォーマット中か
double DiskBasicDir::CheckRoot(DiskBasicType *type, int start_sector, int end_sector, bool is_formatting)
{
	DiskBasicGroups root_groups;
	return type->CheckRootDirectory(start_sector, end_sector, root_groups, is_formatting);
}

/// ルートディレクトリをアサイン
/// @param [in] type         DISK BASIC 種類
/// @param [in] start_sector 開始セクタ番号
/// @param [in] end_sector   終了セクタ番号
bool DiskBasicDir::AssignRoot(DiskBasicType *type, int start_sector, int end_sector)
{
	DiskBasicGroups root_groups;
	delete root;
	root = NewItem();
	bool valid = type->AssignRootDirectory(start_sector, end_sector, root_groups, root);
	if (valid) {
		// グループを設定
		root->SetGroups(root_groups);
		// ディレクトリツリー確定
		root->ValidDirectory(true);
		// グループを保持
		current_item = root;
	}
	return valid;
}

/// ルートディレクトリをアサイン
/// @param [in] type        DISK BASIC 種類
bool DiskBasicDir::AssignRoot(DiskBasicType *type)
{
	DiskBasicGroups root_groups;
	delete root;
	root = NewItem();
	bool valid = type->AssignDirectory(true, root_groups, root);
	if (valid) {
		// グループを設定
		root->SetGroups(root_groups);
		// ディレクトリツリー確定
		root->ValidDirectory(true);
		// グループを保持
		current_item = root;
	}
	return valid;
}

/// ルートディレクトリをリリース
/// @param [in] type        DISK BASIC 種類
bool DiskBasicDir::ReleaseRoot(DiskBasicType *type)
{
	delete root;
	root = NULL;
	return true;
}

/// ディレクトリのチェック
/// @param [in] type        DISK BASIC 種類
/// @param [in] group_items グループのリスト
/// @return <0.0 エラー
double DiskBasicDir::Check(DiskBasicType *type, DiskBasicGroups &group_items)
{
	return type->CheckDirectory(false, group_items);
}

/// ディレクトリをアサイン
/// @param [in] type        DISK BASIC 種類
/// @param [in] group_items グループのリスト
/// @param [in] dir_item    ディレクトリのアイテム
bool DiskBasicDir::Assign(DiskBasicType *type, DiskBasicGroups &group_items, DiskBasicDirItem *dir_item)
{
	bool valid = type->AssignDirectory(false, group_items, dir_item);
	if (valid) {
		// ディレクトリツリー確定
		dir_item->ValidDirectory(true);
	}
	return valid;
}

/// ディレクトリをアサイン
bool DiskBasicDir::Assign(DiskBasicType *type, DiskBasicDirItem *dir_item)
{
	DiskBasicGroups group_items;
	dir_item->GetAllGroups(group_items);
	return Assign(type, group_items, dir_item);
}

/// ディレクトリをアサイン
bool DiskBasicDir::Assign(DiskBasicDirItem *dir_item)
{
	bool valid = true;
	DiskBasicType *type = basic->GetType();

	if (!type->IsRootDirectory(dir_item->GetStartGroup(0))) {
		// サブディレクトリのアイテムをアサイン
		if (dir_item->GetChildren() == NULL) {
			DiskBasicGroups groups;
			dir_item->GetAllGroups(groups);

			if (Check(type, groups) < 0.0) {
				return false;
			}
			valid = Assign(type, groups, dir_item);
		}
	}
	return valid;
}

/// ルートディレクトリを初期化
void DiskBasicDir::ClearRoot()
{
	Fill(basic->GetDirStartSector(), basic->GetDirEndSector(), basic->GetFillCodeOnDir());
}

/// ルートディレクトリ領域を指定コードで埋める
/// @param [in] start_sector 開始セクタ番号
/// @param [in] end_sector   終了セクタ番号
/// @param [in] code         埋めるコード
void DiskBasicDir::Fill(int start_sector, int end_sector, wxUint8 code)
{
	for(int sec_pos = start_sector; sec_pos <= end_sector; sec_pos++) {
		DiskImageSector *sector = basic->GetSector(sec_pos);
		if (!sector) {
			break;
		}
		sector->Fill(code);
	}
}

/// ディレクトリを移動する
/// @param [in,out] dst_item 移動先のディレクトリのアイテム
bool DiskBasicDir::Change(DiskBasicDirItem * &dst_item)
{
	DiskBasicType *type = basic->GetType();

	if (type->IsRootDirectory(dst_item->GetStartGroup(0))) {
		// ルートディレクトリに移動
		dst_item = root;
		current_item = root;

	} else {
		// サブディレクトリに移動
		if (dst_item->GetChildren() == NULL) {
			DiskBasicGroups groups;
			dst_item->GetAllGroups(groups);

			if (Check(type, groups) < 0.0) {
				return false;
			}
			Assign(type, groups, dst_item);
		}
		// ".",".."を実際のディレクトリアイテムにするため
		// 親ディレクトリと同じ場合はそのアイテムにする
		DiskBasicDirItem *pitem = dst_item;
		for(int i=0; i<2; i++) {
			pitem = pitem->GetParent();
			if (!pitem) break;

			if (dst_item->GetStartGroup(0) == pitem->GetStartGroup(0)) {
				dst_item = pitem;
				break;
			} 
		}
		current_item = dst_item;
	}
	return true;
}

/// ディレクトリの拡張ができるか
bool DiskBasicDir::CanExpand()
{
	DiskBasicType *type = basic->GetType();
	return GetParentItem() != NULL ? type->CanExpandDirectory() : type->CanExpandRootDirectory();
}

/// ディレクトリを拡張する
bool DiskBasicDir::Expand()
{
	DiskBasicType *type = basic->GetType();

	bool valid = false;
	if (current_item != NULL) {
		valid = basic->ExpandDirectory(current_item);
		if (valid) {
			// ディレクトリエリアを読み直す
			EmptyCurrent();
			if (GetParentItem() != NULL) {
				Assign(type, current_item);
			} else {
				AssignRoot(type, basic->GetDirStartSector(), basic->GetDirEndSector());
			}
		}
	}
	return valid;
}

/// ディレクトリの占有サイズを計算する
int DiskBasicDir::CalcSize()
{
	int size = 0;
	DiskBasicDirItems *items = GetCurrentItems();
	if (items) {
		int count = (int)items->Count();
		if (count == 0) {
			return 0;
		}
		int data_size = (int)items->Item(0)->GetDataSize();
		size = data_size * count;
		for(int i = (count-1); i >= 0; i--) {
			DiskBasicDirItem *item = items->Item(i);
			if (item->IsUsed()) {
				break;
			}
			size -= data_size;
		}
	}
	return size;
}
