﻿/// @file basictype_os9.cpp
///
/// @brief disk basic type for OS-9
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "basictype_os9.h"
#include <wx/stream.h>
#include "basicfmt.h"
#include "basicdir.h"
#include "basicdiritem_os9.h"
#include "../logging.h"


//
//
//
OS9AllocMap::OS9AllocMap()
	: DiskBasicBitMLMap()
{
	map_bytes = 0;
	map_start_lsn = 0;
	end_lsn = 0;
	sector_size = 0;
	secs_per_bit = 0;
}

OS9AllocMap::~OS9AllocMap()
{
}

/// Allocation Map を割当てる
/// @param[in] basic : Disk Basic パラメータ
/// @param[in] n_map_start_lsn : MapのあるLSN
/// @param[in] n_map_bytes : Mapで使用するバイト数
/// @return true / false セクタなし
bool OS9AllocMap::AllocMap(DiskBasic *basic, wxUint32 n_map_start_lsn, wxUint32 n_map_bytes)
{
	map_bytes = n_map_bytes;
	map_start_lsn = n_map_start_lsn;
	end_lsn = basic->GetFatEndGroup();
	DiskImageDisk *disk = basic->GetDisk();
	sector_size = (wxUint32)disk->GetFile()->GetSectorSize();
	secs_per_bit = basic->GetGroupWidth();

	if (secs_per_bit <= 0) secs_per_bit = 1;

	bool valid = true;
	wxUint32 bytes = 0;

	Empty();

	wxUint32 map_end_lsn = (end_lsn / sector_size / 8 / secs_per_bit) + 1;
	map_end_lsn += map_start_lsn;
	for(wxUint32 map_lsn = map_start_lsn; map_lsn <= map_end_lsn && map_lsn <= (map_start_lsn + 32) && bytes < map_bytes; map_lsn++) {
//		DiskImageSector *sector = basic->GetSectorFromGroup(map_lsn);
//		if (!sector) {
//			// error
//			valid = false;
//			break;
//		}
//		wxUint8 *buf = sector->GetSectorBuffer();
//		int size = sector->GetSectorSize();

		int block_num = 0;
		basic->CalcStartNumFromGroupNum(map_lsn, block_num);

		AddBuffer(disk, block_num, 0, (int)sector_size);

		bytes += sector_size;
	}
	return valid;
}

/// Mapを元にして使用状況を作成する
/// @param[out] fat : 使用状況
void OS9AllocMap::MakeAvailable(DiskBasicAvailabillity &fat)
{
	wxUint32 bits = 0;
	wxUint32 map_bits = map_bytes << 3;
	wxUint32 lsn = 0;

	for(size_t map_idx = 0; map_idx < Count() && bits < map_bits; map_idx++) {
//		wxUint8 *buf = Item(map_idx).GetBuffer();
		BitMLBuffer *bitbuf = &Item(map_idx);
		int bit_size = bitbuf->GetBitSize();

		for(int pos = 0; pos < bit_size && lsn <= end_lsn && bits < map_bits; pos++) {
			bool used = bitbuf->IsSet(pos);
			for(int i=0; i<secs_per_bit && lsn <= end_lsn; i++) {
				if (!used) {
					fat.Add(FAT_AVAIL_FREE, sector_size, 1);
				} else {
					fat.Add(FAT_AVAIL_USED, 0, 0);
				}
				lsn++;
			}
			bits++;
		}
	}
}

/// LSNをMapにセット
/// @param[in] lsn : LSN
/// @param[in] val : セット / リセット
void OS9AllocMap::SetLSN(wxUint32 lsn, bool val)
{
	lsn /= secs_per_bit;

	Modify(lsn, val);
}

/// LSNを使用しているか
/// @param[in] lsn : LSN
/// @return true 使用している
bool OS9AllocMap::IsUsedLSN(wxUint32 lsn) const
{
	lsn /= secs_per_bit;

	return IsSet(lsn);
}

/// 空いているLSNを得る
/// @return LSN or INVALID_GROUP_NUMBER
wxUint32 OS9AllocMap::FindEmpty() const
{
	wxUint32 match_lsn = INVALID_GROUP_NUMBER;
	wxUint32 bytes = 0;
	wxUint32 lsn = 0;
	for(size_t map_idx = 0; map_idx < Count() && bytes < map_bytes && match_lsn == INVALID_GROUP_NUMBER; map_idx++) {
		wxUint8 *buf = Item(map_idx).GetBuffer();
		int size = (int)Item(map_idx).GetSize();

		for(int pos = 0; pos < size && lsn <= end_lsn && bytes < map_bytes && match_lsn == INVALID_GROUP_NUMBER; pos++) {
			for(int bit = 0; bit < 8 && lsn <= end_lsn && bytes < map_bytes && match_lsn == INVALID_GROUP_NUMBER; bit++) {
				bool used = ((buf[pos] & (0x80 >> bit)) != 0);
				if (!used) {
					match_lsn = lsn;
					break;
				}
				lsn += secs_per_bit;
			}
			bytes++;
		}
	}
	return match_lsn;
}

//
//
//
DiskBasicTypeOS9::DiskBasicTypeOS9(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicType(basic, fat, dir)
{
	os9_ident = NULL;
	m_sector_div = 1;
	m_sector_divs = 1;
}

/// ディスクから各パラメータを取得＆必要なパラメータを計算
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0       正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeOS9::ParseParamOnDisk(bool is_formatting)
{
	if (is_formatting) return 1.0;

	double valid_ratio = 1.0;

	// ディスクのセクタサイズとの比較
	int sector_size_on_prm = basic->GetBasedSectorSize();
	int sector_size_on_dsk = basic->GetDisk()->GetSectorSize();
	if (sector_size_on_dsk >= sector_size_on_prm) {
		m_sector_div = 1;
		m_sector_divs = sector_size_on_dsk / sector_size_on_prm;
	} else {
		m_sector_div = sector_size_on_dsk / sector_size_on_prm;
		m_sector_divs = 1;
	}

	// Identの位置だけは256バイトを基準
	int managed_sector_pos = basic->GetManagedSectorNumber();
	int div_num = managed_sector_pos % m_sector_divs;
	int div_nums = m_sector_divs;
	managed_sector_pos /= div_nums;
	DiskImageSector *sector = basic->GetSector(managed_sector_pos);
	if (!sector) {
		return -1.0;
	}
	os9_ident = (os9_ident_t *)sector->GetSectorBuffer(div_num * sector_size_on_prm);
	if (!os9_ident) {
		return -1.0;
	}
	basic->SetManagedSectorNumber(managed_sector_pos);

	// total groups
	wxUint32 total_groups = GET_OS9_LSN(os9_ident->DD_TOT);
	myLog.SetInfo("OS9: DD_TOT: Total Sectors: %d", total_groups); 
	if (total_groups < 1) {
		return -1.0;
	}
	// 最終グループ番号
	basic->SetFatEndGroup(total_groups - 1);

	if (basic->GetReservedSectors() == 1) {
		// このセクタのIdentは仮でもう1つIdentが存在する
		// OS-9/X68000 V2.2 用
		bool match = false;
		for(int i=0; i<512; i++) {
			managed_sector_pos++;
			sector = basic->GetSector(managed_sector_pos);
			if (!sector) {
				break;
			}
			// 各セクタの先頭がTotal Sizeと同じになるか
			os9_lsn_t *tot = (os9_lsn_t *)sector->GetSectorBuffer();
			if (total_groups == GET_OS9_LSN((*tot))) {
				// このセクタが本来のIdent
				match = true;
				break;
			}
		}
		if (match) {
			os9_ident = (os9_ident_t *)sector->GetSectorBuffer();
			basic->SetManagedSectorNumber(managed_sector_pos);
		} else {
			return -1.0;
		}
	}

	int ival;
	DiskImageFile *file = basic->GetDisk()->GetFile();

	// sectors per track
	ival = wxUINT16_SWAP_ON_LE(os9_ident->DD_SPT);
	myLog.SetInfo("OS9: DD_SPT: Sectors per Track: %d", ival); 
	if (ival == 0) {
		return -1.0;
	}
	if (ival > file->GetSectorsPerTrack()) {
		myLog.SetInfo("OS9: %d > %d", ival, file->GetSectorsPerTrack());
		valid_ratio = 0.5;
	}

	// sectors per bit on bitmap table
	ival = wxUINT16_SWAP_ON_LE(os9_ident->DD_BIT);
	myLog.SetInfo("OS9: DD_BIT: Sectors per Bit on Bitmap: %d", ival); 
	if (ival == 0 || !Utils::IsPowerOfTwo(ival, 16)) {
		return -1.0;
	}
	basic->SetGroupWidth(ival);

	// disk format
	ival = os9_ident->DD_FMT;
	wxString sval;
	sval += ((ival & 1) ? "double side" : "single side");
	sval += ((ival & 2) ? ", double density" : ", single density");
	if (ival & 4) sval += (", double track (96/135TPI)");
	if (ival & 8) sval += (", quad track density (192TPI)");
	if (ival & 16) sval += (", octal track density (384TPI)");
	myLog.SetInfo("OS9: DD_FMT: 0x%x (%s)", ival, sval.t_str());

	// root directory
	wxUint32 dir_fd_lsn = GET_OS9_LSN(os9_ident->DD_DIR);
	myLog.SetInfo("OS9: DD_DIR: LSN on Root Directory: %u", dir_fd_lsn); 
	if (dir_fd_lsn > basic->GetFatEndGroup()) {
		return -1.0;
	}
	sector = basic->GetSectorFromGroup(dir_fd_lsn);
	if (!sector) {
		return -1.0;
	}
	directory_os9_fd_t *fdd = (directory_os9_fd_t *)sector->GetSectorBuffer();

	for(int i = 0; i < 48; i++) {
		wxUint32 start_lsn = GET_OS9_LSN(fdd->FD_SEG[i].LSN);
		wxUint32 end_lsn = wxUINT16_SWAP_ON_LE(fdd->FD_SEG[i].SIZ);	// block size (in sectors)
		if (start_lsn == 0 && end_lsn == 0) {
			break;
		}
		end_lsn += start_lsn;

		if (i == 0) basic->SetDirStartSector(start_lsn);
		basic->SetDirEndSector(end_lsn);
	}

	// Allocation Map
	wxUint32 map_lsn = wxUINT32_SWAP_ON_LE(os9_ident->DD_MapLSN);
	wxUint32 map_bytes = wxUINT16_SWAP_ON_LE(os9_ident->DD_MAP);
	myLog.SetInfo("OS9: DD_MapLSN: %u", map_lsn);
	if (map_lsn == 0) {
		map_lsn++;
	}
	if (!alloc_map.AllocMap(basic, map_lsn, map_bytes)) {
		return -1.0;
	}

	basic->SetSectorsPerFat(map_bytes / file->GetSectorSize() + 1);

	return valid_ratio;
}

/// Allocation Mapの開始位置を得る（ダイアログ用）
void DiskBasicTypeOS9::GetStartNumOnFat(int &block_num, int &sector_pos) const
{
	block_num = -1;
	sector_pos = -1;
	int map_lsn = (int)alloc_map.GetMapStartLSN();
	DiskImageSector *sector = basic->GetSector(map_lsn);
	if (sector) {
		block_num = map_lsn;
		sector_pos = sector->GetNumber();
	}
}

/// Allocation Mapの終了位置を得る（ダイアログ用）
void DiskBasicTypeOS9::GetEndNumOnFat(int &block_num, int &sector_pos) const
{
	int map_lsn = (int)alloc_map.GetMapStartLSN();
	map_lsn += ((int)alloc_map.GetMapBytes() / basic->GetDisk()->GetSectorSize());
	DiskImageSector *sector = basic->GetSector(map_lsn);
	if (sector) {
		block_num = map_lsn;
		sector_pos = sector->GetNumber();
	}
}

/// タイトル名（ダイアログ用）
wxString DiskBasicTypeOS9::GetTitleForFat() const
{
	return _("Allocation Map");
}

/// エリアをチェック
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0       正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeOS9::CheckFat(bool is_formatting)
{
	return 1.0;
}

/// ルートディレクトリをアサイン
/// @param [in]     start_sector 開始セクタ番号
/// @param [in]     end_sector   終了セクタ番号
/// @param [out]    group_items  セクタリスト
/// @param [in,out] dir_item     ルートディレクトリアイテム
bool DiskBasicTypeOS9::AssignRootDirectory(int start_sector, int end_sector, DiskBasicGroups &group_items, DiskBasicDirItem *dir_item)
{
	bool sts = DiskBasicType::AssignRootDirectory(start_sector, end_sector, group_items, dir_item);

	// FDセクタへのポインタをルートアイテムに設定
	DiskBasicDirItemOS9 *ditem = (DiskBasicDirItemOS9 *)dir_item;

	wxUint32 dir_fd_lsn = GET_OS9_LSN(os9_ident->DD_DIR);
	ditem->SetStartGroup(0, dir_fd_lsn);
	DiskBasicDirItemOS9FD *fd = &ditem->GetFD();

	int sec_pos = GetStartSectorFromGroup(dir_fd_lsn);
	fd->Set(basic, sec_pos, dir_fd_lsn);

	return sts;
}

//

/// ルートディレクトリのセクタリストを計算
/// @param [in] start_sector  ディレクトリ開始セクタ番号
/// @param [in] end_sector    ディレクトリ終了セクタ番号
/// @param [out] group_items  セクタリスト
bool DiskBasicTypeOS9::CalcGroupsOnRootDirectory(int start_sector, int end_sector, DiskBasicGroups &group_items)
{
	bool valid = true;

	// root directory
	wxUint32 dir_fd_lsn = GET_OS9_LSN(os9_ident->DD_DIR);
	DiskImageSector *sector = basic->GetSectorFromGroup(dir_fd_lsn);
	if (!sector) {
		return false;
	}
	directory_os9_fd_t *fdd = (directory_os9_fd_t *)sector->GetSectorBuffer();
	if (!fdd) {
		return false;
	}

	group_items.Empty();

	size_t dir_size = 0;
	for(int i = 0; i < 48 && valid; i++) {
		wxUint32 start_lsn = GET_OS9_LSN(fdd->FD_SEG[i].LSN);
		wxUint32 end_lsn = wxUINT16_SWAP_ON_LE(fdd->FD_SEG[i].SIZ);	// block size (in sectors)
		if (start_lsn == 0 && end_lsn == 0) {
			break;
		}
		end_lsn += start_lsn;

		for(wxUint32 lsn = start_lsn; lsn < end_lsn; lsn++) {
			int sec_pos = GetStartSectorFromGroup(lsn);
			sector = basic->GetSector(sec_pos);
			if (!sector) {
				valid = false;
				break;
			}
			group_items.Add(lsn, 0, sec_pos, sec_pos);
			dir_size += sector->GetSectorSize();
		}
	}
	group_items.SetSize(dir_size);

	return valid;
}

/// ディレクトリが空か
bool DiskBasicTypeOS9::IsEmptyDirectory(bool is_root, const DiskBasicGroups &group_items)
{
	bool valid = true;
	bool last = false;

	int index_number = 0;
//	DiskBasicDirItem *nitem = dir->NewItem(0, 0, NULL);
	DiskBasicDirItem *nitem = dir->NewItem(0, 0);
	for(size_t idx = 0; idx < group_items.Count(); idx++) {
		const DiskBasicGroupItem *gitem = group_items.ItemPtr(idx);
		for(int sec_pos = gitem->GetSectorStart(); sec_pos <= gitem->GetSectorEnd() && valid && !last; sec_pos++) {
			DiskImageSector *sector = basic->GetSector(sec_pos);
			if (!sector) {
				valid = false;
				break;
			}
//			wxUint8 *buffer = sector->GetSectorBuffer();
//			if (!buffer) {
//				valid = false;
//				break;
//			}

			int pos = 0;
			int size = sector->GetSectorSize();

			// ディレクトリにファイルがないかのチェック
			while(valid && !last && pos < size) {
//				nitem->SetDataPtr(index_number, gitem, sec_pos, pos, buffer);
				nitem->SetDataPtr(index_number, gitem, sec_pos, pos);
				// FDセクタを調べる
				wxUint32 start_nsl = nitem->GetStartGroup(0);
				int fd_sector_pos = GetStartSectorFromGroup(start_nsl);
				DiskImageSector *fd_sector = basic->GetSector(fd_sector_pos);
				if (fd_sector) {
					directory_os9_fd_t *fd_buf = (directory_os9_fd_t *)fd_sector->GetSectorBuffer();
					if (fd_buf) {
						DiskBasicDirItemOS9FD *fd = &((DiskBasicDirItemOS9 *)nitem)->GetFD();
						fd->Set(basic, fd_sector_pos, start_nsl);
						if (nitem->IsNormalFile()) {
							valid = !nitem->CheckUsed(last);
						}
					}
				}
				pos    += nitem->GetDataSize();
//				buffer += nitem->GetDataSize();
				index_number++;
			}
		}
	}
	delete nitem;
	return valid;
}

/// ディレクトリエリアのサイズに達したらアサイン終了するか
/// @param[in,out] pos         ディレクトリの位置
/// @param[in,out] size        ディレクトリのセクタサイズ
/// @param[in,out] size_remain ディレクトリの残りサイズ
/// @retval  0 : 終了しない
/// @retval  1 : 強制的に未使用とする アサインは継続
int DiskBasicTypeOS9::FinishAssigningDirectory(int &pos, int &size, int &size_remain) const
{
	// サイズに達したら以降は未使用とする
	return (size_remain <= 0 ? 1 : 0);
}

/// 使用可能なディスクサイズを得る
void DiskBasicTypeOS9::GetUsableDiskSize(int &disk_size, int &group_size) const
{
	group_size = basic->GetFatEndGroup() + 1;
	disk_size = group_size * basic->GetDisk()->GetSectorSize();
}

/// 残りディスクサイズを計算
void DiskBasicTypeOS9::CalcDiskFreeSize(bool wrote)
{
	fat_availability.Empty();

	// Allocation Mapを調べる
	alloc_map.MakeAvailable(fat_availability);

	// ディレクトリエントリのグループ
	const DiskBasicDirItems *items = dir->GetCurrentItems();
	if (items) {
		for(size_t idx = 0; idx < items->Count(); idx++) {
			DiskBasicDirItem *item = items->Item(idx);
			if (!item || !item->IsUsed()) continue;

			// グループ番号のマップを調べる
			size_t gcnt = item->GetGroupCount();
			if (gcnt > 0) {
				const DiskBasicGroupItem *gitem = item->GetGroup(gcnt - 1);
				wxUint32 gnum = gitem->GetGroup();
				if (gnum <= basic->GetFatEndGroup()) {
					fat_availability.Set(gnum, FAT_AVAIL_USED_LAST);
				}
			}
		}
	}
}

/// 使用状態を設定する
/// @param [in] num グループ番号(0...)
/// @param [in] val 1:使用中 0:空きにする
void DiskBasicTypeOS9::SetGroupNumber(wxUint32 num, wxUint32 val)
{
	alloc_map.SetLSN(num, val != 0);
}

/// グループ番号を得る
wxUint32 DiskBasicTypeOS9::GetGroupNumber(wxUint32 num) const
{
	return num;
}

/// FAT位置が使用されているか
/// @param [in] num グループ番号(0...)
bool DiskBasicTypeOS9::IsUsedGroupNumber(wxUint32 num)
{
	return alloc_map.IsUsedLSN(num);
}

/// 次のグループ番号を得る
wxUint32 DiskBasicTypeOS9::GetNextGroupNumber(wxUint32 num, int block_num)
{
	return INVALID_GROUP_NUMBER;
}

/// 空き位置を返す
/// @return INVALID_GROUP_NUMBER: 空きなし
wxUint32 DiskBasicTypeOS9::GetEmptyGroupNumber()
{
	// Allocation Mapを調べる
	return alloc_map.FindEmpty();
}

/// 次の空き位置を返す 未使用
/// @return INVALID_GROUP_NUMBER: 空きなし
wxUint32 DiskBasicTypeOS9::GetNextEmptyGroupNumber(wxUint32 curr_group)
{
	return INVALID_GROUP_NUMBER;
}

/// ファイルをセーブする前の準備を行う
/// @param [in]     istream   ストリームバッファ
/// @param [in,out] file_size 出力サイズ
/// @param [in,out] pitem     ファイル名、属性を持っているディレクトリアイテム
/// @param [in,out] nitem     確保したディレクトリアイテム
/// @param [in,out] errinfo   エラー情報
bool DiskBasicTypeOS9::PrepareToSaveFile(wxInputStream &istream, int &file_size, DiskBasicDirItem *pitem, DiskBasicDirItem *nitem, DiskBasicError &errinfo)
{
	// FDセクタを確保する
	wxUint32 lsn = GetEmptyGroupNumber();
	if (lsn == INVALID_GROUP_NUMBER) {
		return false;
	}
	int block_num = GetStartSectorFromGroup(lsn);
	DiskImageSector *sector = basic->GetSector(block_num);
	if (!sector) {
		return false;
	}
	wxUint8 *buf = sector->GetSectorBuffer();
	if (!buf) {
		return false;
	}
	// FDセクタをセット
	nitem->SetChainSector(block_num, lsn, buf, pitem);

	// 開始LSNを設定
	nitem->SetStartGroup(0, lsn);

	// セクタを予約
	SetGroupNumber(lsn, 1);

	return true;
}

/// データサイズ分のグループを確保する
/// @param [in]     fileunit_num ファイル番号
/// @param [in,out] item         ディレクトリアイテム
/// @param [in]     data_size    確保するデータサイズ（バイト）
/// @param [in]     flags        新規か追加か
/// @param [out]    group_items  確保したセクタリスト
/// @return >0:正常 -1:空きなし(開始グループ設定前) -2:空きなし(開始グループ設定後)
int DiskBasicTypeOS9::AllocateUnitGroups(int fileunit_num, DiskBasicDirItem *item, int data_size, AllocateGroupFlags flags, DiskBasicGroups &group_items)
{
	DiskBasicDirItemOS9 *ditem = (DiskBasicDirItemOS9 *)item;
	DiskBasicDirItemOS9FD *fd = &ditem->GetFD();

	int sector_size = basic->GetDisk()->GetSectorSize();
	int seg_idx = -1;
	if (flags == ALLOCATE_GROUPS_APPEND) {
		// 追加の場合、既にあるセグメントを計算
		seg_idx = 48;
		for(int idx = 0; idx < 48; idx++) {
			if (fd->GetLSN(idx) == 0 && fd->GetSIZ(idx) == 0) {
				seg_idx = idx - 1; 
				break;
			}
		}
		if (seg_idx >= 48) {
			// セグメントに空きなし
			return -1;
		}
	}
	int start_seg_idx = seg_idx + 1;

	int file_size = (int)fd->GetSIZ();
	data_size += file_size;

	// 新規作成でDD_BITが2以上のとき
	bool is_first_lsn = (flags == ALLOCATE_GROUPS_NEW && basic->GetGroupWidth() > 1);

	// データ用のセクタを確保する
	int rc = 0;
	wxUint32 lsn = 0;
	wxUint32 prev_lsn = 0;
	wxUint16 seg_cnt = 0;
	int limit = basic->GetFatEndGroup() + 1;
	while(file_size < data_size && limit >= 0 && rc == 0) {
		int start = 0;
		if (is_first_lsn) {
			// 新規作成でDD_BITが2以上のときはFDセクタの空きからデータを書き込んでいく
			lsn = fd->GetMyLSN() + 1;
			start++;
			is_first_lsn = false;
		} else {
			lsn = GetEmptyGroupNumber();
		}
		if (lsn == INVALID_GROUP_NUMBER) {
			// 空きなし？
			rc = -2;
			break;
		}
		if (prev_lsn == 0 || (prev_lsn + 1) != lsn) {
			// LSNが連続していない
			seg_idx++;
			if (seg_idx >= 48) {
				// セグメント限界
				rc = -2;
				break;
			}
			fd->SetLSN(seg_idx, lsn);
			seg_cnt = (wxUint16)(basic->GetGroupWidth() - start);
			fd->SetSIZ(seg_idx, seg_cnt);
		} else {
			// LSNが連続しているなら、同じセグメントでセクタ数を増やす
			seg_cnt += basic->GetGroupWidth();
			fd->SetSIZ(seg_idx, seg_cnt);
		}
		// セクタを予約
		SetGroupNumber(lsn, 1);
		// グループ追加
		for(int i=start; i<basic->GetGroupWidth(); i++) {
			basic->GetNumsFromGroup(lsn, 0, sector_size, 0, group_items);
			lsn++;
		} 
		// LSNを保持
		prev_lsn = lsn - 1;

		if (file_size + sector_size * (basic->GetGroupWidth() - start) > data_size) {
			file_size = data_size;
		} else {
			file_size += sector_size * (basic->GetGroupWidth() - start);
		}
		// ファイルサイズ
		fd->SetSIZ(file_size);

		limit--;
	}
	if (limit < 0) {
		rc = -2;
	}

	// エラーの場合、確保したエリアを開放
	if (rc < 0) {
		for(int idx = start_seg_idx; idx < 48; idx++) {
			wxUint32 seg_lsn = fd->GetLSN(idx);
			int seg_siz = fd->GetSIZ(idx);
			if (seg_lsn == 0 && seg_siz == 0) {
				break;
			}
			for(int siz = 0; siz < seg_siz; siz++) {
				// セクタを未使用にする
				if ((seg_lsn / basic->GetGroupWidth()) != (fd->GetMyLSN() / basic->GetGroupWidth())) SetGroupNumber(seg_lsn, 0);
				seg_lsn++;
			}
			fd->SetLSN(idx, 0);
			fd->SetSIZ(idx, 0);
		}
	}

	return rc;
}

/// グループ番号からセクタ番号を得る
int DiskBasicTypeOS9::GetStartSectorFromGroup(wxUint32 group_num)
{
	return group_num + basic->GetManagedSectorNumber();
}

/// グループ番号から最終セクタ番号を得る
int DiskBasicTypeOS9::GetEndSectorFromGroup(wxUint32 group_num, wxUint32 next_group, int sector_start, int sector_size, int remain_size)
{
	return group_num + basic->GetManagedSectorNumber();
}

/// データ領域の開始セクタを計算
int DiskBasicTypeOS9::CalcDataStartSectorPos()
{
	DiskImageFile *file = basic->GetDisk()->GetFile();
	return 1 * file->GetSectorsPerTrack() * file->GetSidesPerDisk();
//	return basic->GetManagedTrackNumber() * basic->GetSectorsPerTrackOnBasic() * basic->GetSidesPerDiskOnBasic();
}

/// ルートディレクトリか
bool DiskBasicTypeOS9::IsRootDirectory(wxUint32 group_num)
{
	return ((group_num + 1) <= (wxUint32)basic->GetDirStartSector());
}

/// サブディレクトリを作成する前の準備を行う
/// @param [in]  item    確保したディレクトリアイテム
bool DiskBasicTypeOS9::PrepareToMakeDirectory(DiskBasicDirItem *item)
{
	// FDセクタを確保する
	wxUint32 lsn = GetEmptyGroupNumber();
	if (lsn == INVALID_GROUP_NUMBER) {
		return false;
	}
	DiskBasicDirItemOS9 *ditem = (DiskBasicDirItemOS9 *)item;
	int block_num = GetStartSectorFromGroup(lsn);

	DiskBasicDirItemOS9FD *fd = &ditem->GetFD();
	fd->Set(basic, block_num, lsn);
	fd->Clear();

	// 開始LSNを設定
	ditem->SetStartGroup(0, lsn);
	// 日付を設定
	TM tm;
	wxDateTime::GetTmNow(tm);
	ditem->SetFileCreateDateTime(tm);
	ditem->SetFileModifyDateTime(tm);
	// セクタを予約
	SetGroupNumber(lsn, 1);

	return true;
}

/// サブディレクトリを作成した後の個別処理
void DiskBasicTypeOS9::AdditionalProcessOnMadeDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, const DiskBasicDirItem *parent_item)
{
	if (group_items.Count() <= 0) return;

	// ディレクトリ属性
	item->SetFileAttr(basic->GetFormatTypeNumber(), 0,
		FILETYPE_MASK_OS9_DIRECTORY |
		FILETYPE_MASK_OS9_PUBLIC_EXEC |
		FILETYPE_MASK_OS9_PUBLIC_WRITE |
		FILETYPE_MASK_OS9_PUBLIC_READ |
		FILETYPE_MASK_OS9_USER_EXEC |
		FILETYPE_MASK_OS9_USER_WRITE |
		FILETYPE_MASK_OS9_USER_READ
	);

	// ファイルサイズはエントリ２つ分
	item->SetFileSize((int)item->GetDataSize() * 2);

	// カレントと親ディレクトリのエントリを作成する
	DiskBasicGroupItem *gitem = &group_items.Item(0);

//	DiskImageSector *sector = basic->GetSector(gitem->GetSectorStart());

//	wxUint8 *buf = sector->GetSectorBuffer();
//	DiskBasicDirItem *newitem = basic->CreateDirItem(gitem->GetSectorStart(), 0, buf);
	DiskBasicDirItem *newitem = basic->CreateDirItem(gitem->GetSectorStart(), 0);

	// 親をつくる
	newitem->ClearData();
	if (parent_item) {
		// 親がサブディレクトリ
		newitem->SetStartGroup(0, parent_item->GetStartGroup(0));
	} else {
		// 親がルート
		wxUint32 dir_fd_lsn = GET_OS9_LSN(os9_ident->DD_DIR);
		newitem->SetStartGroup(0, dir_fd_lsn);
	}
	newitem->SetFileNamePlain(wxT(".."));

	// カレント
//	buf += newitem->GetDataSize();
//	newitem->SetDataPtr(0, NULL, gitem->GetSectorStart(), 0, buf);
	newitem->SetDataPtr(1, NULL, gitem->GetSectorStart(), (int)newitem->GetDataSize());

	newitem->ClearData();
	newitem->SetStartGroup(0, item->GetStartGroup(0));
	newitem->SetFileNamePlain(wxT("."));

	// ディレクトリサイズを更新
	int dir_size = dir->CalcSize();
	DiskBasicDirItem *dir_item = item->GetParent();
	if (dir_item) {
		dir_item->SetFileSize(dir_size);
	}

	delete newitem;
}

/// セクタデータを指定コードで埋める
void DiskBasicTypeOS9::FillSector(DiskImageSector *sector)
{
	sector->Fill(basic->GetFillCodeOnFormat());
}

/// セクタデータを埋めた後の個別処理
bool DiskBasicTypeOS9::AdditionalProcessOnFormatted(const DiskBasicIdentifiedData &data)
{
	// Ident
	DiskImageSector *sector = basic->GetSector(basic->GetManagedSectorNumber());
	if (!sector) return false;
	os9_ident = (os9_ident_t *)sector->GetSectorBuffer();
	if (!os9_ident) return false;

	DiskImageDisk *disk = basic->GetDisk();
	int total_lsn = disk->GetNumberOfSectors() - 1; 
	// 最終グループ番号
	basic->SetFatEndGroup(total_lsn - 1);

	int map_lsn = 1;
	int root_start_lsn = (basic->GetDirStartSector() - 1);
	int root_end_lsn = (basic->GetDirEndSector() - 1);
	int ival;

	//
	// OS9 Identifier をセット
	//

	sector->Fill(0);

	DiskImageFile *file = disk->GetFile();

	// total lsn
	SET_OS9_LSN(os9_ident->DD_TOT, total_lsn);
	// sectors per track
	os9_ident->DD_TKS = file->GetSectorsPerTrack();
	// allocation map length
	ival = (total_lsn + 7) / 8;
	os9_ident->DD_MAP = wxUINT16_SWAP_ON_LE(ival);
	// sectors per group
	ival = basic->GetGroupWidth();
	os9_ident->DD_BIT = wxUINT16_SWAP_ON_LE(ival);
	// rootdir lsn
	SET_OS9_LSN(os9_ident->DD_DIR, root_start_lsn);
	// owner id
	os9_ident->DD_OWN = 0;
	// disk attr
	os9_ident->DD_ATT = 0;
	// disk ident
	os9_ident->DD_DSK = wxUINT16_SWAP_ON_LE(0);

	// format, density, number of sides
	ival = ((file->GetSidesPerDisk() - 1) & 0x01);
	ival |= (file->HasSingleDensity() == 1 ? 0 : 0x02);
	os9_ident->DD_FMT = (wxUint8)ival;

	// sector per track
	ival = file->GetSectorsPerTrack();
	os9_ident->DD_SPT = wxUINT16_SWAP_ON_LE(ival);

	// bootstrap lsn
	SET_OS9_LSN(os9_ident->DD_BT, 0);
	// bootstrap size (in bytes)
	os9_ident->DD_BSZ = wxUINT16_SWAP_ON_LE(0);

	// creation date
	TM tm;
	wxDateTime::GetTmNow(tm);
	os9_ident->DD_DAT.yy = (tm.GetYear() % 100);
	os9_ident->DD_DAT.mm = (tm.GetMonth() + 1);
	os9_ident->DD_DAT.dd = (tm.GetDay());
	os9_ident->DD_DAT.hh = (tm.GetHour());
	os9_ident->DD_DAT.mi = (tm.GetMinute());

	// bitmap starting sector number
	os9_ident->DD_MapLSN = wxUINT32_SWAP_ON_LE(map_lsn - 1);

	// volume label
	SetIdentifiedData(data);

	// 固有パラメータ設定
	basic->AssignParameter();

	//
	// Allocation Mapを作成
	//

	for(int lsn = map_lsn; lsn < root_start_lsn; lsn++) {
		sector = basic->GetSectorFromGroup(lsn);
		if (!sector) return false;
		sector->Fill(0xff);
	}
	for(int lsn = root_end_lsn + 1; lsn < total_lsn; lsn++) {
		SetGroupNumber(lsn, 0);
	}

	//
	// ルートディレクトリを作成
	//

	int block_num = GetStartSectorFromGroup(root_start_lsn);
	sector = basic->GetSector(block_num);
	if (!sector) return false;

	DiskBasicDirItemOS9 *root_item = (DiskBasicDirItemOS9 *)dir->NewItem();
	DiskBasicDirItemOS9FD *root_fd = &root_item->GetFD();

	root_fd->Set(basic, block_num, root_start_lsn);
	root_fd->Clear();

	root_item->SetStartGroup(0, root_start_lsn);
	// 日付を設定
	root_item->SetFileCreateDateTime(tm);
	root_item->SetFileModifyDateTime(tm);
	// セグメント設定
	root_fd->SetLSN(0, root_start_lsn + 1);
	root_fd->SetSIZ(0, root_end_lsn - root_start_lsn);
	// リンクの数
	root_fd->SetLNK(2);
	// セクタを予約
	SetGroupNumber(root_start_lsn, 1);

	for(int lsn = root_start_lsn + 1; lsn <= root_end_lsn; lsn++) {
		sector = basic->GetSectorFromGroup(lsn);
		if (!sector) {
			continue;
		}
		sector->Fill(0);
	}
	DiskBasicGroups group_items;
	basic->GetNumsFromGroup(root_start_lsn + 1, 0, file->GetSectorSize(), file->GetSectorSize(), group_items);
	AdditionalProcessOnMadeDirectory(root_item, group_items, NULL);

	delete root_item;

	return true;
}

/// データの書き込み終了後の処理
void DiskBasicTypeOS9::AdditionalProcessOnSavedFile(DiskBasicDirItem *item)
{
	// ディレクトリサイズを更新
	int dir_size = dir->CalcSize();
	DiskBasicDirItem *dir_item = item->GetParent();
	if (!dir_item) {
		// Why?
		return;
	}
	dir_item->SetFileSize(dir_size);
}

/// FAT領域を削除する
void DiskBasicTypeOS9::DeleteGroupNumber(wxUint32 group_num)
{
	// 未使用にする
	SetGroupNumber(group_num, 0);
}

/// ファイル削除後の処理
bool DiskBasicTypeOS9::AdditionalProcessOnDeletedFile(DiskBasicDirItem *item)
{
	// FDセクタを未使用にする
	SetGroupNumber(item->GetStartGroup(0), 0);

	// ディレクトリサイズを更新
	int dir_size = dir->CalcSize();
	DiskBasicDirItem *dir_item = item->GetParent();
	if (!dir_item) {
		// Why?
		return true;
	}
	dir_item->SetFileSize(dir_size);

	return true;
}

/// IPLや管理エリアの属性を得る
void DiskBasicTypeOS9::GetIdentifiedData(DiskBasicIdentifiedData &data) const
{
	// volume label
	char buf[sizeof(os9_ident->DD_NAM) + 1];
	DiskBasicDirItemOS9::DecodeString(buf, sizeof(os9_ident->DD_NAM), os9_ident->DD_NAM, sizeof(os9_ident->DD_NAM));
	wxString vol(buf, sizeof(os9_ident->DD_NAM));
	data.SetVolumeName(vol);
	data.SetVolumeNameMaxLength(sizeof(os9_ident->DD_NAM));
}

/// IPLや管理エリアの属性をセット
void DiskBasicTypeOS9::SetIdentifiedData(const DiskBasicIdentifiedData &data)
{
	const DiskBasicFormat *fmt = basic->GetFormatType();

	// volume label
	if (fmt->HasVolumeName()) {
		wxCharBuffer vol = data.GetVolumeName().To8BitData();
		DiskBasicDirItemOS9::EncodeString(os9_ident->DD_NAM, sizeof(os9_ident->DD_NAM), vol, vol.length());
	}
}
