/// @file basictype_fat32.cpp
///
/// @brief disk basic fat type
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "basictype_fat32.h"
#include "basicfmt.h"
#include "basicdiritem.h"


//
//
//
DiskBasicTypeFAT32::DiskBasicTypeFAT32(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicTypeFAT16(basic, fat, dir)
{
}

/// FATエリアをチェック
/// @param [in] is_formatting フォーマット中か
/// @retval 1.0       正常
/// @retval 0.0 - 1.0 警告あり
/// @retval <0.0      エラーあり
double DiskBasicTypeFAT32::CheckFat(bool is_formatting)
{
	// 重複チェック
	double valid_ratio = CheckFatDuplicated(is_formatting, 2, 0x1ffff);

	return valid_ratio;
}

/// 残りディスクサイズを計算
void DiskBasicTypeFAT32::CalcDiskFreeSize(bool wrote)
{
	CalcDiskFreeSizeBase(wrote, 2, 0x0ffffff8);
}

/// FAT位置をセット
/// @param [in] num グループ番号(0...)
/// @param [in] val 値
void DiskBasicTypeFAT32::SetGroupNumber(wxUint32 num, wxUint32 val)
{
	fat->GetDiskBasicFatArea()->SetData32LE(num, val);
}
/// FAT位置を返す
/// @param [in] num グループ番号(0...)
wxUint32 DiskBasicTypeFAT32::GetGroupNumber(wxUint32 num) const
{
	return fat->GetDiskBasicFatArea()->GetData32LE(0, num);
}

//
//
//
DiskBasicTypeFAT32BE::DiskBasicTypeFAT32BE(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir)
	: DiskBasicTypeFAT32(basic, fat, dir)
{
}

/// FAT位置をセット
/// @param [in] num グループ番号(0...)
/// @param [in] val 値
void DiskBasicTypeFAT32BE::SetGroupNumber(wxUint32 num, wxUint32 val)
{
	fat->GetDiskBasicFatArea()->SetData32BE(num, val);
}
/// FAT位置を返す
/// @param [in] num グループ番号(0...)
wxUint32 DiskBasicTypeFAT32BE::GetGroupNumber(wxUint32 num) const
{
	return fat->GetDiskBasicFatArea()->GetData32BE(0, num);
}
