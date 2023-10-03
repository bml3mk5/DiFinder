/// @file basictype_msdos.h
///
/// @brief disk basic type for Human68k
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef BASICTYPE_HU68K_H
#define BASICTYPE_HU68K_H

#include "../common.h"
#include "basiccommon.h"
#include "basictype_fat16.h"


#pragma pack(1)
typedef struct st_hu68k_bpb {
	wxUint8	 JmpBoot[2];	// $60 $1C/$20
	wxUint8  OEMName[16];
	wxUint16 BytsPerSec;	// BE
	wxUint8  SecPerClus;
	wxUint8  NumOfFATs;		// if b7=1 then FAT16 MS-DOS
	wxUint16 RsvdSecCnt;
	wxUint16 RootEntCnt;
	wxUint16 TotalSecs16;
	wxUint8  MediaID;
	wxUint8  SecsOnFAT;
	union {
		wxUint32 StartSec;		// start sector block on SASI HDD
		wxUint32 TotalSecs32;	// extended
	};
} hu68k_bpb_t;
#pragma pack()

/** @class DiskBasicTypeHU68K

@brief Human68kの処理

DiskBasicParam 固有のパラメータ
@li IPLString : IPL文字列
@li IPLCompareString : OS判定時に使用する
@li IgnoreParameter : セクタ１にあるパラメータを無視するか
@li MediaID : メディアID

*/
class DiskBasicTypeHU68K : public DiskBasicTypeFAT16BE
{
protected:
	int m_fat_type;	// FAT12 = 0 / 16 = 1

	DiskBasicTypeHU68K() : DiskBasicTypeFAT16BE() {}
	DiskBasicTypeHU68K(const DiskBasicType &src) : DiskBasicTypeFAT16BE(src) {}

	/// ボリュームラベルを更新 なければ作成
	bool			ModifyOrMakeVolumeLabel(const wxString &filename);

public:
	DiskBasicTypeHU68K(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir);

	/// @name access to FAT area
	//@{
	/// @brief FAT位置をセット
	virtual void		SetGroupNumber(wxUint32 num, wxUint32 val);
	/// @brief FAT位置を返す
	virtual wxUint32	GetGroupNumber(wxUint32 num) const;
	/// @brief システムグループ番号を返す
	virtual wxUint32	GetGroupSystemCode() const;
	//@}

	/// @name check / assign FAT area
	//@{
	/// @brief FATエリアをチェック
	virtual double 	CheckFat(bool is_formatting);
	/// @brief ディスクから各パラメータを取得＆必要なパラメータを計算
	virtual double	ParseParamOnDisk(bool is_formatting);
	/// @brief ディスクからHuman68kパラメータを取得
	double			ParseHU68KParamOnDisk(DiskImageDisk *disk, bool is_formatting);
	//@}

	/// @name disk size
	//@{
	/// @brief 残りディスクサイズを計算
	virtual void	CalcDiskFreeSize(bool wrote);
	//@}

	/// @name directory
	//@{
	/// @brief ルートディレクトリか
	virtual bool	IsRootDirectory(wxUint32 group_num);
	/// @brief サブディレクトリを作成できるか
	virtual bool	CanMakeDirectory() const { return true; }
	/// @brief サブディレクトリのサイズを拡張できるか
	virtual bool	CanExpandDirectory() const { return true; }
	/// @brief サブディレクトリを作成する前にディレクトリ名を編集する
	virtual bool	RenameOnMakingDirectory(wxString &dir_name);
	/// @brief サブディレクトリを作成した後の個別処理
	virtual void	AdditionalProcessOnMadeDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, const DiskBasicDirItem *parent_item);
	/// @brief ディレクトリ拡張後の個別処理
	virtual bool	AdditionalProcessOnExpandedDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, const DiskBasicDirItem *parent_item);
	//@}

	/// @name format
	//@{
	/// @brief セクタデータを埋めた後の個別処理
	virtual bool	AdditionalProcessOnFormatted(const DiskBasicIdentifiedData &data);
	/// @brief BIOS Parameter Block を作成
	bool			CreateBiosParameterBlock(const char *jmp, const char *name, wxUint8 **sec_buf = NULL);
	//@}

	/// @name property
	//@{
	/// @brief IPLや管理エリアの属性を得る
	virtual void	GetIdentifiedData(DiskBasicIdentifiedData &data) const;
	/// @brief IPLや管理エリアの属性をセット
	virtual void	SetIdentifiedData(const DiskBasicIdentifiedData &data);
	//@}
};

#endif /* _BASICTYPE_HU68K_H_ */
