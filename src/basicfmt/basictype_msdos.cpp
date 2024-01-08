/// @file basictype_msdos.cpp
///
/// @brief disk basic type for MS-DOS
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "basictype_msdos.h"
#include "basicfmt.h"
#include "basicdir.h"


//
//
//
DiskBasicTypeMSDOS::DiskBasicTypeMSDOS(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicTypeFAT16(basic, fat, dir)
{
	m_fat_type = 0; // FAT12 default
}

/// FAT位置をセット
/// @param [in] num グループ番号(0...)
/// @param [in] val 値
void DiskBasicTypeMSDOS::SetGroupNumber(wxUint32 num, wxUint32 val)
{
	switch (m_fat_type) {
	case 0:
		DiskBasicTypeFAT12::SetGroupNumber(num, val);
		break;
	default:
		DiskBasicTypeFAT16::SetGroupNumber(num, val);
		break;
	}
}
/// FAT位置を返す
/// @param [in] num グループ番号(0...)
wxUint32 DiskBasicTypeMSDOS::GetGroupNumber(wxUint32 num) const
{
	switch (m_fat_type) {
	case 0:
		return DiskBasicTypeFAT12::GetGroupNumber(num);
	default:
		return DiskBasicTypeFAT16::GetGroupNumber(num);
	}
}

/// システムグループ番号を返す
wxUint32 DiskBasicTypeMSDOS::GetGroupSystemCode() const
{
	switch (m_fat_type) {
	case 0:
		return 0xff8;
	default:
		return 0xfff8;
	}
}

/// 残りディスクサイズを計算
void DiskBasicTypeMSDOS::CalcDiskFreeSize(bool wrote)
{
	switch (m_fat_type) {
	case 0:
		DiskBasicTypeFAT12::CalcDiskFreeSize(wrote);
		break;
	default:
		DiskBasicTypeFAT16::CalcDiskFreeSize(wrote);
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
	case 0:
		return DiskBasicTypeFAT12::CheckFat(is_formatting);
	default:
		return DiskBasicTypeFAT16::CheckFat(is_formatting);
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
	if (sector_mag == 1 || sector_mag == 2 || sector_mag == 4) {
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

	// FATタイプの決定
	wxUint32 data_sectors = sector_mag * wxUINT16_SWAP_ON_BE(bpb->BPB_TotSec16);
	if (data_sectors == 0) {
		data_sectors = sector_mag * wxUINT32_SWAP_ON_BE(bpb->BPB_TotSec32);
	}
	data_sectors = data_sectors - basic->GetDirEndSector() - 1;
	int max_grp = (int)(data_sectors / basic->GetSectorsPerGroup());
#if 0
	if (max_grp >= 65525) {
		m_fat_type = 2;	// FAT32
	} else
#endif
	if (max_grp >= 4086) {
		m_fat_type = 1;	// FAT16
	}

	// 最終グループ番号を計算
	int max_grp_on_fat = 0;
	switch(m_fat_type) {
	case 0:
		max_grp_on_fat = basic->GetSectorsPerFat() * sector_size_on_disk * 2 / 3;	// FAT12で計算
		break;
	default:
		max_grp_on_fat = basic->GetSectorsPerFat() * sector_size_on_disk / 2;	// FAT16で計算
		break;
	}
	int max_grp_on_prm = disk->GetNumberOfSectors() / basic->GetSectorsPerGroup();

	max_grp = (max_grp < max_grp_on_fat ? max_grp : max_grp_on_fat);
	max_grp = (max_grp < max_grp_on_prm ? max_grp : max_grp_on_prm);
	basic->SetFatEndGroup(max_grp - 1);

	// テンプレートに一致するものがあるか
	const DiskBasicParam *param = gDiskBasicTemplates.FindType(basic->GetBasicCategoryName(), basic->GetBasicTypeName());
	if (param) {
		wxString str = param->GetBasicDescription();
		switch(m_fat_type) {
		case 0:
			str += wxT(" (FAT12)");
			break;
		default:
			str += wxT(" (FAT16)");
			break;
		}
		basic->SetBasicDescription(str);
	}

	double valid_ratio = 0.0;
	if (nums > 0) {
		valid_ratio = (double)valids/nums;
	}
	return valid_ratio;
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

	// ファイルサイズをクリア
	item->SetFileSize(0);

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
	newitem->SetDataPtr(1, NULL, gitem->GetSectorStart(), newitem->GetDataSize());
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
