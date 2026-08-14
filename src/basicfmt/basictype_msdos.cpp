/// @file basictype_msdos.cpp
///
/// @brief disk basic type for MS-DOS
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "basictype_msdos.h"
#include "basicfmt.h"
#include "basicdir.h"
#include "basictemplate.h"


//
//
//
DiskBasicTypeMSDOS::DiskBasicTypeMSDOS(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicTypeFAT32(basic, fat, dir)
{
}

/// FAT位置をセット
/// @param [in] num グループ番号(0...)
/// @param [in] val 値
void DiskBasicTypeMSDOS::SetGroupNumber(wxUint32 num, wxUint32 val)
{
	switch (m_fat_type) {
	case FAT_TYPE_12:
		DiskBasicTypeFAT12::SetGroupNumber(num, val);
		break;
	case FAT_TYPE_16:
		DiskBasicTypeFAT16::SetGroupNumber(num, val);
		break;
	case FAT_TYPE_32:
		DiskBasicTypeFAT32::SetGroupNumber(num, val);
		break;
	default:
		break;
	}
}
/// FAT位置を返す
/// @param [in] num グループ番号(0...)
wxUint32 DiskBasicTypeMSDOS::GetGroupNumber(wxUint32 num) const
{
	switch (m_fat_type) {
	case FAT_TYPE_12:
		return DiskBasicTypeFAT12::GetGroupNumber(num);
	case FAT_TYPE_16:
		return DiskBasicTypeFAT16::GetGroupNumber(num);
	case FAT_TYPE_32:
		return DiskBasicTypeFAT32::GetGroupNumber(num);
	default:
		return 0;
	}
}

/// システムグループ番号を返す
wxUint32 DiskBasicTypeMSDOS::GetGroupSystemCode() const
{
	switch (m_fat_type) {
	case FAT_TYPE_12:
		return 0xff8;
	case FAT_TYPE_16:
		return 0xfff8;
	case FAT_TYPE_32:
		return 0x0ffffff8;
	default:
		return 0;
	}
}

/// 残りディスクサイズを計算
void DiskBasicTypeMSDOS::CalcDiskFreeSize(bool wrote)
{
	switch (m_fat_type) {
	case FAT_TYPE_12:
		DiskBasicTypeFAT12::CalcDiskFreeSize(wrote);
		break;
	case FAT_TYPE_16:
		DiskBasicTypeFAT16::CalcDiskFreeSize(wrote);
		break;
	case FAT_TYPE_32:
		DiskBasicTypeFAT32::CalcDiskFreeSize(wrote);
		break;
	default:
		break;
	}
}

/// FATエリアをチェック
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0       正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeMSDOS::CheckFat(bool is_formatting)
{
	switch (m_fat_type) {
	case FAT_TYPE_12:
		return DiskBasicTypeFAT12::CheckFat(is_formatting);
	case FAT_TYPE_16:
		return DiskBasicTypeFAT16::CheckFat(is_formatting);
	case FAT_TYPE_32:
		return DiskBasicTypeFAT32::CheckFat(is_formatting);
	default:
		return -1.0;
	}
}

/// ディスクから各パラメータを取得＆必要なパラメータを計算
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0       正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeMSDOS::ParseParamOnDisk(bool is_formatting)
{
	double valid_ratio = 1.0;

	if (!basic->GetVariousBoolParam(wxT("IgnoreParameter"))) {
		valid_ratio = ParseMSDOSParamOnDisk(basic->GetDisk(), is_formatting);
	}

	wxCharBuffer ipl = basic->GetVariousStringParam(wxT("IPLCompareString")).To8BitData();
	if (ipl.length() > 0) {
		DiskImageSector *sector = basic->GetSector(0);
		if (!sector) return -1.0;
		if (sector->Find(ipl.data(), ipl.length()) < 0) {
			valid_ratio = 0.0;
		}
	}

	return valid_ratio;
}

/// ディスクからMS-DOSパラメータを取得＆必要なパラメータを計算
/// @param [in] disk          ディスク
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0>      正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeMSDOS::ParseMSDOSParamOnDisk(DiskImageDisk *disk, bool is_formatting)
{
	int nums = 0;
	int valids = 0;

	DiskImageFile *file = disk->GetFile();

	// MS-DOS ディスク上のパラメータを読む
	DiskImageSector *sector = disk->GetSector(0);
	if (!sector) return -1.0;
	wxUint8 *datas = sector->GetSectorBuffer();
	if (!datas) return -1.0;
	fat_bpb_t *bpb = (fat_bpb_t *)datas;

	nums++;
	if (bpb->BPB_SecPerClus == 0) {
		// クラスタサイズ
		return -1.0;
	}
	valids++;

	// セクタサイズの違い
	int sector_size_on_os = wxUINT16_SWAP_ON_BE(bpb->BPB_BytsPerSec);
	int sector_size_on_disk = file->GetSectorSize();

	// セクタサイズは整数倍
	int sector_mag = sector_size_on_os / sector_size_on_disk;

	nums++;
	if (sector_mag == 1 || sector_mag == 2 || sector_mag == 4 || sector_mag == 8) {
		// セクタタサイズ
		valids++;
	}

	if (nums == valids) {
		// 1クラスタのセクタ数
		basic->SetSectorsPerGroup(sector_mag * bpb->BPB_SecPerClus);
		// 予約セクタ数
		basic->SetReservedSectors(sector_mag * wxUINT16_SWAP_ON_BE(bpb->BPB_RsvdSecCnt));
		// FATの数
		basic->SetNumberOfFats(bpb->BPB_NumFATs);
		// FAT1つのセクタ数
		basic->SetSectorsPerFat(sector_mag * wxUINT16_SWAP_ON_BE(bpb->BPB_FATSz16));
		// ディレクトリのエントリ数
		basic->SetDirEntryCount(wxUINT16_SWAP_ON_BE(bpb->BPB_RootEntCnt));
		// ディレクトリの最終セクタを計算
		basic->SetDirStartSector(-1);
		basic->SetDirEndSector(-1);
		basic->CalcDirStartEndSector(sector_size_on_disk);
		// メディアID
		basic->SetMediaId(bpb->BPB_Media);
	}

	if (basic->GetSectorsPerGroup() <= 0) {
		basic->SetSectorsPerGroup(1);
	}

	// FATタイプの決定
	m_fat_type = FAT_TYPE_12;

	wxUint32 data_sectors = sector_mag * wxUINT16_SWAP_ON_BE(bpb->BPB_TotSec16);
	if (data_sectors == 0) {
		data_sectors = sector_mag * wxUINT32_SWAP_ON_BE(bpb->BPB_TotSec32);
	}
	data_sectors = data_sectors - basic->GetDirEndSector() - 1;
	int max_grp = (int)(data_sectors / basic->GetSectorsPerGroup());
#if 0
	if (max_grp >= 65525) {
		m_fat_type = FAT_TYPE_32;	// FAT32
	} else
#endif
	if (max_grp >= 4086) {
		m_fat_type = FAT_TYPE_16;	// FAT16
	}

	// FAT32か？
	if (bpb->BPB_FATSz16 == 0) {
		fat32_bs_t *fat32_bs = (fat32_bs_t *)(datas + 36);
		if (fat32_bs->BPB_FatSz32 == 0) {
			return -1.0;
		}

		m_fat_type = FAT_TYPE_32;	// FAT32

		// FAT1つのセクタ数
		basic->SetSectorsPerFat(sector_mag * wxUINT32_SWAP_ON_BE(fat32_bs->BPB_FatSz32));
		// ディレクトリのエントリ数
		basic->SetDirEntryCount(0);
		// ルートディレクトリのあるクラスタ
		basic->SetVariousParam(wxT("DirStartCluster"), wxVariant((int)wxUINT32_SWAP_ON_BE(fat32_bs->BPB_RootClus)));
		// ディレクトリの最終セクタを再計算
		basic->SetDirStartSector(-1);
		basic->SetDirEndSector(-1);
		basic->CalcDirStartEndSector(sector_size_on_disk);
	}

	// 最終グループ番号を計算
	wxUint32 system_code = basic->GetGroupSystemCode();
	wxUint32 final_code = basic->GetGroupFinalCode();
	int max_grp_on_fat = 0;
	switch(m_fat_type) {
	case FAT_TYPE_12:
		max_grp_on_fat = basic->GetSectorsPerFat() * sector_size_on_disk * 2 / 3;	// FAT12で計算
		system_code &= 0xfff;
		final_code &= 0xfff;
		break;
	case FAT_TYPE_16:
		max_grp_on_fat = basic->GetSectorsPerFat() * sector_size_on_disk / 2;	// FAT16で計算
		system_code &= 0xffff;
		final_code &= 0xffff;
		break;
	case FAT_TYPE_32:
		max_grp_on_fat = basic->GetSectorsPerFat() * sector_size_on_disk;	// FAT32で計算
		break;
	default:
		break;
	}
	int max_grp_on_prm = disk->GetNumberOfSectors() / basic->GetSectorsPerGroup();

	max_grp = (max_grp < max_grp_on_fat ? max_grp : max_grp_on_fat);
	max_grp = (max_grp < max_grp_on_prm ? max_grp : max_grp_on_prm);
	basic->SetFatEndGroup(max_grp - 1);
	basic->SetGroupSystemCode(system_code);
	basic->SetGroupFinalCode(final_code);

	// テンプレートに一致するものがあるか
	const DiskBasicParam *param = gDiskBasicTemplates.FindType(basic->GetBasicCategoryName(), basic->GetBasicTypeName());
	if (param) {
		wxString str = param->GetBasicDescription();
		switch(m_fat_type) {
		case FAT_TYPE_12:
			str += wxT(" (FAT12)");
			break;
		case FAT_TYPE_16:
			str += wxT(" (FAT16)");
			break;
		case FAT_TYPE_32:
			str += wxT(" (FAT32)");
			break;
		default:
			break;
		}
		basic->SetBasicDescription(str);
	}

	double valid_ratio = 0.0;
	if (nums > 0) {
		valid_ratio = (double)valids/nums;
	}

//	// FAT32はサポート外
//	if (m_fat_type == FAT_TYPE_32) {
//		valid_ratio = -1.0;
//	}

	return valid_ratio;
}

/// ルートディレクトリのチェック
/// @param [in]     start_sector 開始セクタ番号
/// @param [in]     end_sector   終了セクタ番号
/// @param [out]    group_items  セクタリスト
/// @param [in]     is_formatting フォーマット中か
/// @return <0.0 エラー 1.0:正常
double DiskBasicTypeMSDOS::CheckRootDirectory(int start_sector, int end_sector, DiskBasicGroups &group_items, bool is_formatting)
{
	// フォーマット中はチェックしない
	if (is_formatting) return 1.0;

//	if (m_fat_type == FAT_TYPE_32) {
//		DiskBasicDirItem *root = basic->GetRootDirectory();
//		root->GetAllGroups(group_items);
//		return DiskBasicType::CheckDirectory(true, group_items);
//	} else {
		return DiskBasicType::CheckRootDirectory(start_sector, end_sector, group_items, is_formatting);
//	}
}

/// ルートディレクトリをアサイン
/// @param [in]     start_sector 開始セクタ番号
/// @param [in]     end_sector   終了セクタ番号
/// @param [out]    group_items  セクタリスト
/// @param [in,out] dir_item     ルートディレクトリアイテム
/// @return true / false
bool DiskBasicTypeMSDOS::AssignRootDirectory(int start_sector, int end_sector, DiskBasicGroups &group_items, DiskBasicDirItem *dir_item)
{
	if (m_fat_type == FAT_TYPE_32) {
		if (dir_item->GetStartGroup(0) == 0) {
			int val = basic->GetVariousIntegerParam(wxT("DirStartCluster"));
			dir_item->SetStartGroup(0, val);
		}
		dir_item->GetAllGroups(group_items);
		return DiskBasicType::AssignDirectory(true, group_items, dir_item);
	} else {
		return DiskBasicType::AssignRootDirectory(start_sector, end_sector, group_items, dir_item);
	}
}

/// セクタデータを埋めた後の個別処理
/// フォーマット IPLの書き込み
bool DiskBasicTypeMSDOS::AdditionalProcessOnFormatted(const DiskBasicIdentifiedData &data)
{
	if (!CreateBiosParameterBlock("\xeb\x3c\x90", "FAT12")) {
		return false;
	}

	// ボリュームラベルを設定
	const DiskBasicFormat *fmt = basic->GetFormatType();
	if (fmt->HasVolumeName()) {
		int dir_start = basic->GetReservedSectors() + basic->GetNumberOfFats() * basic->GetSectorsPerFat();
//		DiskImageSector *sec = basic->GetSector(dir_start);
//		DiskBasicDirItem *ditem = dir->NewItem(dir_start, 0, sec->GetSectorBuffer());
		DiskBasicDirItem *ditem = dir->NewItem(dir_start, 0);

		ditem->SetFileNamePlain(data.GetVolumeName());
		ditem->SetFileAttr(FORMAT_TYPE_UNKNOWN, FILE_TYPE_VOLUME_MASK);
		TM tm;
		tm.Now();
		ditem->SetFileModifyDateTime(tm);
		delete ditem;
	}

	return true;
}

/// BIOS Parameter Block を作成
bool DiskBasicTypeMSDOS::CreateBiosParameterBlock(const char *jmp, const char *name, wxUint8 **sec_buf)
{
	DiskImageDisk *disk = basic->GetDisk();
	DiskImageSector *sec = basic->GetSector(0);
	if (!sec) return false;
	wxUint8 *buf = sec->GetSectorBuffer();
	if (!buf) return false;

	if(sec_buf) *sec_buf = buf;

	sec->Fill(0);

	fat_bpb_t *hed = (fat_bpb_t *)buf;

	size_t len;

	wxCharBuffer s_jmp = basic->GetVariousStringParam(wxT("JumpBoot")).To8BitData();
	if (s_jmp.length() > 0) {
		jmp = s_jmp.data();
	}
	len = strlen(jmp) < sizeof(hed->BS_JmpBoot) ? strlen(jmp) : sizeof(hed->BS_JmpBoot);
	memcpy(hed->BS_JmpBoot, jmp, len);

	hed->BPB_BytsPerSec = wxUINT16_SWAP_ON_BE(disk->GetSectorSize());
	hed->BPB_SecPerClus = basic->GetSectorsPerGroup();
	hed->BPB_RsvdSecCnt = wxUINT16_SWAP_ON_BE(basic->GetReservedSectors());
	hed->BPB_NumFATs = basic->GetNumberOfFats();
	hed->BPB_RootEntCnt = wxUINT16_SWAP_ON_BE(basic->GetDirEntryCount());

	wxCharBuffer s_name = basic->GetVariousStringParam(wxT("OEMName")).To8BitData();
	if (s_name.length() > 0) {
		name = s_name.data();
	}
	// 上記パラメータ領域をまたがって設定可能にする
	len = strlen(name) < 16 ? strlen(name) : 16;
	memset(hed->BS_OEMName, 0x20, sizeof(hed->BS_OEMName));
	memcpy(hed->BS_OEMName, name, len);

	len = 0;
	hed->BPB_TotSec16 = wxUINT16_SWAP_ON_BE(len);
	hed->BPB_Media = basic->GetMediaId();
	hed->BPB_FATSz16 = wxUINT16_SWAP_ON_BE(basic->GetSectorsPerFat());

	// FATの先頭にメディアIDをセット
	SetGroupNumber(0, 0xffffff00 | basic->GetMediaId());
	SetGroupNumber(1, 0xffffffff);

	return true;
}

/// ルートディレクトリか
bool DiskBasicTypeMSDOS::IsRootDirectory(wxUint32 group_num)
{
	// オフセット未満だったらルート
	return (group_num <= 1);
}

/// サブディレクトリを作成する前にディレクトリ名を編集する
bool DiskBasicTypeMSDOS::RenameOnMakingDirectory(wxString &dir_name)
{
	// 空や"."で始まるディレクトリは作成不可
	if (dir_name.IsEmpty() || dir_name.Left(1) == wxT(".")) {
		return false;
	}
	return true;
}

/// サブディレクトリを作成した後の個別処理
void DiskBasicTypeMSDOS::AdditionalProcessOnMadeDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, const DiskBasicDirItem *parent_item)
{
	if (group_items.Count() <= 0) return;

//	// ファイルサイズをクリア
//	item->SetFileSize(0);

	// カレントと親ディレクトリのエントリを作成する
	DiskBasicGroupItem *gitem = &group_items.Item(0);

//	DiskImageSector *sector = basic->GetDisk()->GetSector(gitem->GetSectorStart());

//	wxUint8 *buf = sector->GetSectorBuffer();
//	DiskBasicDirItem *newitem = basic->CreateDirItem(gitem->GetSectorStart(), 0, buf);
	DiskBasicDirItem *newitem = basic->CreateDirItem(gitem->GetSectorStart(), 0);

	// カレント
	newitem->CopyData(item->GetData());
	newitem->SetFileNamePlain(wxT("."));
	newitem->SetFileAttr(FORMAT_TYPE_UNKNOWN, FILE_TYPE_DIRECTORY_MASK);

	// 親
//	buf += newitem->GetDataSize();
//	newitem->SetDataPtr(0, NULL, gitem->GetSectorStart(), 0, buf);
	newitem->SetDataPtr(1, NULL, gitem->GetSectorStart(), (int)newitem->GetDataSize());
	if (parent_item) {
		// 親がサブディレクトリ
		newitem->CopyData(parent_item->GetData());
	} else {
		// 親がルート
		newitem->CopyData(item->GetData());
		newitem->SetStartGroup(0, 0);
	}
	newitem->SetFileCreateDateTime(item->GetFileCreateDateTime());
	newitem->SetFileModifyDateTime(item->GetFileModifyDateTime());
	newitem->SetFileAccessDateTime(item->GetFileAccessDateTime());
	newitem->SetFileNamePlain(wxT(".."));
	newitem->SetFileAttr(FORMAT_TYPE_UNKNOWN, FILE_TYPE_DIRECTORY_MASK);

	delete newitem;
}

/// IPLや管理エリアの属性を得る
void DiskBasicTypeMSDOS::GetIdentifiedData(DiskBasicIdentifiedData &data) const
{
	// volume label
	DiskBasicDirItem *ditem = dir->FindFileByAttrOnRoot(FILE_TYPE_VOLUME_MASK, FILE_TYPE_VOLUME_MASK | FILE_TYPE_DIRECTORY_MASK);
	if (ditem && ditem->IsUsed()) {
		data.SetVolumeName(ditem->GetFileNameStr());
		data.SetVolumeNameMaxLength(ditem->GetFileNameStrSize());
	}
}

/// IPLや管理エリアの属性をセット
void DiskBasicTypeMSDOS::SetIdentifiedData(const DiskBasicIdentifiedData &data)
{
	const DiskBasicFormat *fmt = basic->GetFormatType();

	// volume label
	if (fmt->HasVolumeName()) {
		ModifyOrMakeVolumeLabel(data.GetVolumeName());
	}
}

/// ボリュームラベルを更新 なければ作成
bool DiskBasicTypeMSDOS::ModifyOrMakeVolumeLabel(const wxString &filename)
{
	DiskBasicDirItem *next_item;
	// ボリュームラベルがあるか
	DiskBasicDirItem *item = dir->FindFileByAttrOnRoot(FILE_TYPE_VOLUME_MASK, FILE_TYPE_VOLUME_MASK | FILE_TYPE_DIRECTORY_MASK);
	if (!item) {
		// 新しいディレクトリアイテムを確保
		if ((item = dir->GetEmptyItemOnRoot(NULL, &next_item)) == NULL) {
			// 確保できない時
			return false;
		} else {
			item->SetEndMark(next_item);
		}
		item->ClearData();
		item->SetFileAttr(FORMAT_TYPE_UNKNOWN, FILE_TYPE_VOLUME_MASK, 0);
	}
	item->SetFileNameStr(filename);
	item->Used(true);

	return true;
}
