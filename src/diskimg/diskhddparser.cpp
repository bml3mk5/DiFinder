/// @file diskhddparser.cpp
///
/// @brief HDDディスクイメージパーサ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskhddparser.h"
#include <wx/stream.h>
#include "diskimage.h"
#include "diskparser.h"
#include "fileparam.h"
#include "diskresult.h"


#pragma pack(1)
/// HDD形式ヘッダ
typedef struct st_hdd_dsk_header {
	wxUint8  sig[3];
	wxUint8  version[4];
	wxUint8  unused1;
	wxUint8  comment[128];
	wxUint8  reserved0[4];
	wxUint16 mb_size;
	wxUint16 sector_size;
	wxUint8  sectors_per_track;
	wxUint8  sides_per_disk;
	wxUint16 tracks_per_side;
	wxUint32 total_size;
	wxUint8  reserved1[0x44];
} hdd_dsk_header_t;
#pragma pack()

//
//
//
DiskHDDParser::DiskHDDParser(DiskImageFile *file, short mod_flags, DiskResult *result)
	: DiskPlainParser(file, mod_flags, result)
{
}

DiskHDDParser::~DiskHDDParser()
{
}

/// HDIファイルを解析
/// @param [in] istream    解析対象データ
/// @param [in] disk_param ディスクパラメータ
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskHDDParser::Parse(wxInputStream &istream, const DiskParam *disk_param)
{
	if (!disk_param) {
		p_result->SetError(DiskResult::ERRV_INVALID_DISK, 0);
		return p_result->GetValid();
	}

	istream.SeekI(0);

	hdd_dsk_header_t header;
	size_t len = istream.Read(&header, sizeof(header)).LastRead();
	if (len != sizeof(header)) {
		p_result->SetError(DiskResult::ERRV_DISK_TOO_SMALL, 0);
		return p_result->GetValid();
	}

	int skip_size = (int)sizeof(hdd_dsk_header_t);	// 220bytes
	istream.SeekI(skip_size);
	p_file->SetStartOffset(skip_size);

	return DiskPlainParser::Parse(istream, disk_param);
}

/// チェック
/// @param [in] istream       解析対象データ
/// @param [in] disk_hints    ディスクパラメータヒント("2D"など)
/// @param [in] disk_param    ディスクパラメータ disk_hints指定時はNullable
/// @param [out] disk_params  ディスクパラメータの候補
/// @param [out] manual_param 候補がないときのパラメータヒント
/// @retval 1 選択ダイアログ表示
/// @retval 0 正常（候補が複数ある時はダイアログ表示）
int DiskHDDParser::Check(wxInputStream &istream, const DiskTypeHints *disk_hints, const DiskParam *disk_param, DiskParamPtrs &disk_params, DiskParam &manual_param)
{
	istream.SeekI(0);

	hdd_dsk_header_t header;
	size_t len = istream.Read(&header, sizeof(header)).LastRead();
	if (len < sizeof(header)) {
		// too short
		p_result->SetError(DiskResult::ERRV_DISK_TOO_SMALL, 0);
		return p_result->GetValid();
	}
	if (memcmp(header.sig, "VHD", 3) != 0) {
		// invalid
		p_result->SetError(DiskResult::ERRV_INVALID_DISK, 0);
		return p_result->GetValid();
	}

	int sector_size = (int)wxUINT32_SWAP_ON_BE(header.sector_size);
	if (sector_size <= 0 || sector_size > 4096) {
		// invalid
		p_result->SetError(DiskResult::ERRV_SECTOR_SIZE_HEADER, 0, sector_size);
		return p_result->GetValid();
	}
	int sectors_per_track = (int)wxUINT32_SWAP_ON_BE(header.sectors_per_track);
	if (sectors_per_track <= 0) {
		// invalid
		p_result->SetError(DiskResult::ERRV_SECTORS_HEADER, 0, sectors_per_track);
		return p_result->GetValid();
	}
	int sides_per_disk = (int)wxUINT32_SWAP_ON_BE(header.sides_per_disk);
	if (sides_per_disk <= 0 || sides_per_disk > 32) {
		// invalid
		p_result->SetError(DiskResult::ERRV_SIDES_HEADER, 0, sides_per_disk);
		return p_result->GetValid();
	}
	int tracks_per_side = (int)wxUINT32_SWAP_ON_BE(header.tracks_per_side);
	if (tracks_per_side < 35 || tracks_per_side > 99999) {
		// invalid
		p_result->SetError(DiskResult::ERRV_TRACKS_HEADER, 0, tracks_per_side);
		return p_result->GetValid();
	}

	// ディスクテンプレートから探す
	DiskParam dummy;
	const DiskParam *param = gDiskTemplates.FindStrict(sides_per_disk, tracks_per_side, sectors_per_track, sector_size
		, 1, 0, 0, 0
		, dummy.GetSingles(), dummy.GetParticularTracks());
	if (param) {
		disk_params.Add(param);
	}

	// 候補がないとき、手動設定
	if (disk_params.Count() == 0) {
		manual_param.SetDiskParam(
			sides_per_disk,
			tracks_per_side,
			sectors_per_track,
			sector_size,
			0,
			1,
			dummy.GetSingles(),
			dummy.GetParticularTracks()
		);
		return 1;
	}

	return 0;
}
