/// @file basictype_msdos.h
///
/// @brief disk basic type for MS-DOS
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef BASICTYPE_MSDOS_H
#define BASICTYPE_MSDOS_H

#include "../common.h"
#include "basiccommon.h"
#include "basictype_fat12.h"
#include "basictype_fat16.h"
#include "basictype_fat32.h"

#pragma pack(1)
/// FAT BPB
typedef struct st_fat_bpb {
	wxUint8	 BS_JmpBoot[3];
	wxUint8  BS_OEMName[8];
	wxUint16 BPB_BytsPerSec;
	wxUint8  BPB_SecPerClus;
	wxUint16 BPB_RsvdSecCnt;
	wxUint8  BPB_NumFATs;
	wxUint16 BPB_RootEntCnt;
	wxUint16 BPB_TotSec16;
	wxUint8  BPB_Media;
	wxUint16 BPB_FATSz16;
	wxUint16 BPB_SecPerTrk;
	wxUint16 BPB_NumHeads;
	wxUint32 BPB_HiddSec;
	wxUint32 BPB_TotSec32;
} fat_bpb_t;

typedef struct st_fat16_bs {
	wxUint8  BS_DrvNum;
	wxUint8  BS_Reserved;
	wxUint8  BS_BootSig;
	wxUint32 BS_VolID;
	wxUint8  BS_VolLab[11];
	wxUint8  BS_FilSysType[8];
	wxUint8  BS_BootCode[448];
	wxUint16 BS_Sign;
} fat16_bs_t;

typedef struct st_fat32_bs {
	wxUint32 BPB_FatSz32;
	wxUint16 BPB_ExtFlags;
	wxUint16 BPB_FSVer;
	wxUint32 BPB_RootClus;
	wxUint16 BPB_FSInfo;
	wxUint16 BPB_BkBootSec;
	wxUint8  BPB_Reserved[12];
	wxUint8  BS_DrvNum;
	wxUint8  BS_Reserved;
	wxUint8  BS_BootSig;
	wxUint32 BS_VolID;
	wxUint8  BS_VolLab[11];
	wxUint8  BS_FilSysType[8];
	wxUint8  BS_BootCode[420];
	wxUint16 BS_Sign;
} fat32_bs_t;
#pragma pack()


/** @class DiskBasicTypeMSDOS

@brief MS-DOSの処理

DiskBasicParam 固有のパラメータ
@li MediaID : メディアID
@li IgnoreParameter : セクタ1のパラメータを無視するか

*/
class DiskBasicTypeMSDOS : public DiskBasicTypeFAT32
{
protected:
	DiskBasicTypeMSDOS() : DiskBasicTypeFAT32() {}
	DiskBasicTypeMSDOS(const DiskBasicType &src) : DiskBasicTypeFAT32(src) {}

	/// ボリュームラベルを更新 なければ作成
	bool			ModifyOrMakeVolumeLabel(const wxString &filename);

public:
	DiskBasicTypeMSDOS(DiskBasic *basic, DiskBasicFat *fat, DiskBasicDir *dir);

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
	/// @brief ディスクからMSDOSパラメータを取得
	double			ParseMSDOSParamOnDisk(DiskImageDisk *disk, bool is_formatting);
	//@}

	/// @name check / assign directory area
	//@{
	/// @brief ルートディレクトリのチェック
	virtual double	CheckRootDirectory(int start_sector, int end_sector, DiskBasicGroups &group_items, bool is_formatting);
	/// @brief ルートディレクトリをアサイン
	virtual bool	AssignRootDirectory(int start_sector, int end_sector, DiskBasicGroups &group_items, DiskBasicDirItem *dir_item);
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

#endif /* BASICTYPE_MSDOS_H */
