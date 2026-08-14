/// @file basictype_fat32.h
///
/// @brief disk basic type
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef BASICTYPE_FAT32_H
#define BASICTYPE_FAT32_H

#include "../common.h"
#include "basiccommon.h"
#include "basictype_fat16.h"


/** @class DiskBasicTypeFAT32

@brief FAT32の処理

*/
class DiskBasicTypeFAT32 : public DiskBasicTypeFAT16
{
protected:
	DiskBasicTypeFAT32() : DiskBasicTypeFAT16() {}
	DiskBasicTypeFAT32(const DiskBasicType &src) : DiskBasicTypeFAT16(src) {}
public:
	DiskBasicTypeFAT32(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir);
	virtual ~DiskBasicTypeFAT32() {}

	/// @name access to FAT area
	//@{
	/// @brief FAT位置をセット
	virtual void		SetGroupNumber(wxUint32 num, wxUint32 val);
	/// @brief FAT位置を返す
	virtual wxUint32	GetGroupNumber(wxUint32 num) const;
	//@}

	/// @name check / assign FAT area
	//@{
	/// @brief FATエリアをチェック
	virtual double 	CheckFat(bool is_formatting);
	//@}

	/// @name disk size
	//@{
	/// @brief 残りディスクサイズを計算
	virtual void	CalcDiskFreeSize(bool wrote);
	//@}

	/// @name file size
	//@{
	//@}

	/// @name file chain
	//@{
	//@}

	/// @name directory
	//@{
	//@}

	/// @name format
	//@{
	//@}

	/// @name save / write
	//@{
	//@}
};

/** @class DiskBasicTypeFAT32BE

@brief FAT32の処理 (Big Endien)

*/
class DiskBasicTypeFAT32BE : public DiskBasicTypeFAT32
{
protected:
	DiskBasicTypeFAT32BE() : DiskBasicTypeFAT32() {}
	DiskBasicTypeFAT32BE(const DiskBasicType &src) : DiskBasicTypeFAT32(src) {}
public:
	DiskBasicTypeFAT32BE(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir);
	virtual ~DiskBasicTypeFAT32BE() {}

	/// @name access to FAT area
	//@{
	/// @brief FAT位置をセット
	virtual void		SetGroupNumber(wxUint32 num, wxUint32 val);
	/// @brief FAT位置を返す
	virtual wxUint32	GetGroupNumber(wxUint32 num) const;
	//@}
};

#endif /* BASICTYPE_FAT32_H */
