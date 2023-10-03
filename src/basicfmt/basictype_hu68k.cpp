/// @file basictype_msdos.cpp
///
/// @brief disk basic type for Human68k
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "basictype_hu68k.h"
#include "basicfmt.h"
#include "basicdir.h"


//
//
//
DiskBasicTypeHU68K::DiskBasicTypeHU68K(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicTypeFAT16BE(basic, fat, dir)
{
	m_fat_type = 0; // FAT12 default
}

/// FAT位置をセット
/// @param [in] num グループ番号(0...)
/// @param [in] val 値
void DiskBasicTypeHU68K::SetGroupNumber(wxUint32 num, wxUint32 val)
{
	switch (m_fat_type) {
	case 0:
		DiskBasicTypeFAT12::SetGroupNumber(num, val);
		break;
	default:
		DiskBasicTypeFAT16BE::SetGroupNumber(num, val);
		break;
	}
}
/// FAT位置を返す
/// @param [in] num グループ番号(0...)
wxUint32 DiskBasicTypeHU68K::GetGroupNumber(wxUint32 num) const
{
	switch (m_fat_type) {
	case 0:
		return DiskBasicTypeFAT12::GetGroupNumber(num);
	default:
		return DiskBasicTypeFAT16BE::GetGroupNumber(num);
	}
}

/// システムグループ番号を返す
wxUint32 DiskBasicTypeHU68K::GetGroupSystemCode() const
{
	switch (m_fat_type) {
	case 0:
		return 0xff8;
	default:
		return 0xfff8;
	}
}

/// 残りディスクサイズを計算
void DiskBasicTypeHU68K::CalcDiskFreeSize(bool wrote)
{
	switch (m_fat_type) {
	case 0:
		DiskBasicTypeFAT12::CalcDiskFreeSize(wrote);
		break;
	default:
		DiskBasicTypeFAT16BE::CalcDiskFreeSize(wrote);
		break;
	}
}

/// FATエリアをチェック
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0       正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeHU68K::CheckFat(bool is_formatting)
{
	switch (m_fat_type) {
	case 0:
		return DiskBasicTypeFAT12::CheckFat(is_formatting);
	default:
		return DiskBasicTypeFAT16BE::CheckFat(is_formatting);
	}
}

/// ディスクから各パラメータを取得＆必要なパラメータを計算
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0  正常
/// @retval <1.0 警告あり
/// @retval <0.0 エラーあり
double DiskBasicTypeHU68K::ParseParamOnDisk(bool is_formatting)
{
	if (is_formatting) return 0;

	double valid_ratio = 1.0;
	if (!basic->GetVariousBoolParam(wxT("IgnoreParameter"))) {
		valid_ratio = ParseHU68KParamOnDisk(basic->GetDisk(), is_formatting);
	}
	if (valid_ratio >= 0.0) {
		// セクタ０
		DiskImageSector *sector = basic->GetSector(0);
		if (!sector) {
			return -1.0;
		}

		// IPLに"X68IPL"が含まれるか
		// "Human"の文字列が含まれるか
		int found = -1;
		wxCharBuffer istr;
		for(int i=0; i<3; i++) {
			found = -1;
			switch(i) {
			case 0:
				istr = basic->GetVariousStringParam(wxT("IPLString")).To8BitData();
				break;
			case 1:
				istr = basic->GetVariousStringParam(wxT("IPLCompareString")).To8BitData();
				break;
			case 2:
				istr = wxCharBuffer("Human");
				break;
			}
			if (istr.length() > 0) {
				found = sector->Find(istr.data(), istr.length());
			}
			if (found >= 0) {
				valid_ratio = 1.0;
				break;
			}
		}
		if (found < 0) {
			valid_ratio = 0.1;
		}
	}

	return valid_ratio;
}

/// ディスクからHuman68kパラメータを取得＆必要なパラメータを計算
/// @param [in] disk          ディスク
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0>      正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeHU68K::ParseHU68KParamOnDisk(DiskImageDisk *disk, bool is_formatting)
{
	if (is_formatting) return 1.0;

	int nums = 0;
	int valids = 0;

	DiskImageFile *file = disk->GetFile();

	// MS-DOS ディスク上のパラメータを読む
	DiskImageSector *sector = disk->GetSector(0);
	if (!sector) return -1.0;
	wxUint8 *datas = sector->GetSectorBuffer();
	if (!datas) return -1.0;
	hu68k_bpb_t *bpb = (hu68k_bpb_t *)datas;

	nums++;
	if (bpb->SecPerClus == 0) {
		// クラスタサイズ
		return -1.0;
	}
	valids++;

	// セクタサイズ HDDは256, OS(2HD)は1024
	int sector_size_on_os = wxUINT16_SWAP_ON_LE(bpb->BytsPerSec);
	int sector_size_on_disk = file->GetSectorSize();

	// セクタサイズは整数倍
	int sector_mag = sector_size_on_os / sector_size_on_disk;

	nums++;
	if (sector_mag == 1 || sector_mag == 2 || sector_mag == 4) {
		// セクタタサイズ
		valids++;
	}

	if (nums == valids) {
		// 基準となるセクタサイズをディスク側に合わせる
		basic->SetBasedSectorSize(sector_size_on_disk);
		// 1クラスタのセクタ数
		basic->SetSectorsPerGroup(sector_mag * bpb->SecPerClus);
		// 予約セクタ数
		basic->SetReservedSectors(sector_mag * wxUINT16_SWAP_ON_LE(bpb->RsvdSecCnt));
		// FATの数
		basic->SetNumberOfFats(bpb->NumOfFATs);
		// FAT1つのセクタ数
		basic->SetSectorsPerFat(sector_mag * bpb->SecsOnFAT);
		// ディレクトリのエントリ数
		basic->SetDirEntryCount(wxUINT16_SWAP_ON_LE(bpb->RootEntCnt));
		// ディレクトリの最終セクタを計算
		basic->SetDirStartSector(-1);
		basic->SetDirEndSector(-1);
		basic->CalcDirStartEndSector(sector_size_on_disk);
		// メディアID
		basic->SetMediaId(bpb->MediaID);
	}

	// FATタイプの決定
	wxUint32 data_sectors = sector_mag * wxUINT16_SWAP_ON_LE(bpb->TotalSecs16);
	if (data_sectors == 0) {
		data_sectors = sector_mag * wxUINT32_SWAP_ON_LE(bpb->TotalSecs32);
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
			str += wxT(" (FAT16 BE)");
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
bool DiskBasicTypeHU68K::AdditionalProcessOnFormatted(const DiskBasicIdentifiedData &data)
{
	if (!CreateBiosParameterBlock("\x60\x20", "Hudson soft 2.00")) {
		return false;
	}

	// ボリュームラベルを設定
	int dir_start = basic->GetReservedSectors() + basic->GetNumberOfFats() * basic->GetSectorsPerFat();
	DiskImageSector *sec = basic->GetSector(dir_start);
//	DiskBasicDirItem *ditem = dir->NewItem(dir_start, 0, sec->GetSectorBuffer());
	DiskBasicDirItem *ditem = dir->NewItem(dir_start, 0);

	ditem->SetFileNameStr(data.GetVolumeName());
	ditem->SetFileAttr(FORMAT_TYPE_UNKNOWN, FILE_TYPE_VOLUME_MASK);
	TM tm;
	tm.Now();
	ditem->SetFileCreateDateTime(tm);

	delete ditem;

	return true;
}

/// BIOS Parameter Block を作成
bool DiskBasicTypeHU68K::CreateBiosParameterBlock(const char *jmp, const char *name, wxUint8 **sec_buf)
{
	DiskImageDisk *disk = basic->GetDisk();

	// 仮想セクタサイズ 1セクタ1024バイトとして計算
	int sector_size_on_os = 1024;
	int sector_size_on_disk = disk->GetSectorSize();

	// セクタサイズは整数倍
	int sector_mag = sector_size_on_os / sector_size_on_disk;

	// 予約セクタ数
	int reserved_sectors = basic->GetReservedSectors();
	reserved_sectors = reserved_sectors * basic->GetBasedSectorSize() / sector_size_on_os;

	// 1クラスタのセクタ数
	int sectors_per_group = basic->GetSectorsPerGroup();
	sectors_per_group = sectors_per_group * basic->GetBasedSectorSize() / sector_size_on_os;

	// パーティション内のセクタ数
	int number_of_sectors = disk->GetNumberOfSectors();
	number_of_sectors = number_of_sectors / sector_mag;

	// グループ数
	int number_of_groups = number_of_sectors / sectors_per_group;
	if (number_of_groups >= 4086) {
		m_fat_type = 1;
	}
	// FATのセクタ数
	int sectors_per_fat = basic->GetSectorsPerFat();
	if (sectors_per_fat <= 0) {
		switch(m_fat_type) {
		case 0:
			// FAT12
			sectors_per_fat = (number_of_groups + (sector_size_on_os * 2 / 3) - 1) * 3 / 2 / sector_size_on_os;
			break;
		default:
			// FAT16
			sectors_per_fat = (number_of_groups + (sector_size_on_os / 2) - 1) * 2 / sector_size_on_os;
			break;
		}
	} else {
		sectors_per_fat = sectors_per_fat * basic->GetBasedSectorSize() / sector_size_on_os;
	}

	int rootdir_start_sector = reserved_sectors + sectors_per_fat * basic->GetNumberOfFats();
	int sectors_on_rootdir = (basic->GetDirEntryCount() * 32 + sector_size_on_os - 1) / sector_size_on_os;
	if (sectors_on_rootdir <= 0) sectors_on_rootdir = 1;
	int rootdir_end_sector = rootdir_start_sector + sectors_on_rootdir - 1;

	// システム管理エリアをクリアする
	DiskImageSector *sec;
	for(int n = 0; n < (sector_mag * rootdir_end_sector + sector_mag); n++) {
		sec = basic->GetSector(n);
		if (!sec) continue;
		sec->Fill(basic->GetFillCodeOnFAT());
	}

	sec = basic->GetSector(0);
	if (!sec) return false;
	wxUint8 *buf = sec->GetSectorBuffer();
	if (!buf) return false;

	if(sec_buf) *sec_buf = buf;

	hu68k_bpb_t *hed = (hu68k_bpb_t *)buf;
	size_t len;

	wxCharBuffer s_jmp = basic->GetVariousStringParam(wxT("JumpBoot")).To8BitData();
	if (s_jmp.length() > 0) {
		jmp = s_jmp.data();
	}
	len = strlen(jmp) < sizeof(hed->JmpBoot) ? strlen(jmp) : sizeof(hed->JmpBoot);
	memcpy(hed->JmpBoot, jmp, len);

	hed->BytsPerSec = wxUINT16_SWAP_ON_LE(sector_size_on_os);
	hed->SecPerClus = (wxUint8)sectors_per_group;
	hed->RsvdSecCnt = wxUINT16_SWAP_ON_LE(reserved_sectors);
	hed->NumOfFATs = (wxUint8)basic->GetNumberOfFats();
	hed->RootEntCnt = wxUINT16_SWAP_ON_LE(basic->GetDirEntryCount());

	wxCharBuffer s_name = basic->GetVariousStringParam(wxT("OEMName")).To8BitData();
	if (s_name.length() > 0) {
		name = s_name.data();
	}
	// 上記パラメータ領域をまたがって設定可能にする
	len = strlen(name) < 16 ? strlen(name) : 16;
	memset(hed->OEMName, 0x20, sizeof(hed->OEMName));
	memcpy(hed->OEMName, name, len);

	hed->MediaID = basic->GetMediaId();

	hed->SecsOnFAT = (wxUint8)sectors_per_fat;

	hed->TotalSecs16 = wxUINT16_SWAP_ON_LE(number_of_sectors);

	hed->StartSec = wxUINT32_SWAP_ON_LE(disk->GetStartSectorNumber());

	// 再計算
	basic->SetReservedSectors(sector_mag * reserved_sectors);
	basic->SetSectorsPerFat(sector_mag * sectors_per_fat);
	basic->SetSectorsPerGroup(sector_mag * sectors_per_group);
	basic->SetDirStartSector(sector_mag * rootdir_start_sector);
	basic->SetDirEndSector(sector_mag * rootdir_end_sector + sector_mag - 1);

	// FATの先頭にメディアIDをセット
	fat->Assign(true);
	SetGroupNumber(0, 0xffffff00 | basic->GetMediaId());
	SetGroupNumber(1, 0xffffffff);

	return true;
}

/// ルートディレクトリか
bool DiskBasicTypeHU68K::IsRootDirectory(wxUint32 group_num)
{
	// オフセット未満だったらルート
	return (group_num <= 1);
}

/// サブディレクトリを作成する前にディレクトリ名を編集する
bool DiskBasicTypeHU68K::RenameOnMakingDirectory(wxString &dir_name)
{
	// 空や"."で始まるディレクトリは作成不可
	if (dir_name.IsEmpty() || dir_name.Left(1) == wxT(".")) {
		return false;
	}
	return true;
}

/// サブディレクトリを作成した後の個別処理
void DiskBasicTypeHU68K::AdditionalProcessOnMadeDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, const DiskBasicDirItem *parent_item)
{
	if (group_items.Count() <= 0) return;

	// ファイルサイズをクリア
	item->SetFileSize(0);

	// カレントと親ディレクトリのエントリを作成する
	DiskBasicGroupItem *gitem = &group_items.Item(0);

	DiskImageSector *sector = basic->GetDisk()->GetSector(gitem->GetSectorStart());

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
	newitem->SetDataPtr(0, NULL, gitem->GetSectorStart(), 0);
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

/// ディレクトリ拡張後の個別処理
bool DiskBasicTypeHU68K::AdditionalProcessOnExpandedDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, const DiskBasicDirItem *parent_item)
{
	// ファイルサイズをクリア
	item->SetFileSize(0);

	return true;
}

/// IPLや管理エリアの属性を得る
void DiskBasicTypeHU68K::GetIdentifiedData(DiskBasicIdentifiedData &data) const
{
	// volume label
	DiskBasicDirItem *ditem = dir->FindFileByAttrOnRoot(FILE_TYPE_VOLUME_MASK, FILE_TYPE_VOLUME_MASK | FILE_TYPE_DIRECTORY_MASK);
	if (ditem && ditem->IsUsed()) {
		data.SetVolumeName(ditem->GetFileNameStr());
		data.SetVolumeNameMaxLength(ditem->GetFileNameStrSize());
	}
}

/// IPLや管理エリアの属性をセット
void DiskBasicTypeHU68K::SetIdentifiedData(const DiskBasicIdentifiedData &data)
{
	const DiskBasicFormat *fmt = basic->GetFormatType();

	// volume label
	if (fmt->HasVolumeName()) {
		ModifyOrMakeVolumeLabel(data.GetVolumeName());
	}
}

/// ボリュームラベルを更新 なければ作成
bool DiskBasicTypeHU68K::ModifyOrMakeVolumeLabel(const wxString &filename)
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
