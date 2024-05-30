/// @file diskthdparser.cpp
///
/// @brief THDディスクイメージパーサ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskthdparser.h"
#include <wx/stream.h>
#include "diskimage.h"
#include "diskparser.h"
#include "fileparam.h"
#include "diskresult.h"


#pragma pack(1)
/// THD形式ヘッダ 256bytes
typedef struct st_thd_dsk_header {
	wxUint16 tracks;
	wxUint8  reserved1[254];
} thd_dsk_header_t;
#pragma pack()

//
//
//
DiskTHDParser::DiskTHDParser(DiskImageFile *file, short mod_flags, DiskResult *result)
	: DiskPlainParser(file, mod_flags, result)
{
}

DiskTHDParser::~DiskTHDParser()
{
}

/// HDIファイルを解析
/// @param [in] istream    解析対象データ
/// @param [in] disk_param ディスクパラメータ
/// @param [in] boot_param ブートストラップ種類(nullable)
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskTHDParser::Parse(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param)
{
	if (!disk_param) {
		p_result->SetError(DiskResult::ERRV_INVALID_DISK, 0);
		return p_result->GetValid();
	}

	istream.SeekI(0);

	thd_dsk_header_t header;
	size_t len = istream.Read(&header, sizeof(header)).LastRead();
	if (len != sizeof(header)) {
		p_result->SetError(DiskResult::ERRV_DISK_TOO_SMALL, 0);
		return p_result->GetValid();
	}

	int skip_size = (int)sizeof(thd_dsk_header_t);
	istream.SeekI(skip_size);
	p_file->SetStartOffset(skip_size);

	return DiskPlainParser::Parse(istream, disk_param, boot_param);
}

/// チェック
/// @param [in] istream       解析対象データ
/// @param [in] disk_hints    ディスクパラメータヒント("2D"など)
/// @param [in] disk_param    ディスクパラメータ disk_hints指定時はNullable
/// @param [out] disk_params  ディスクパラメータの候補
/// @param [out] manual_param 候補がないときのパラメータヒント
/// @retval 1 選択ダイアログ表示
/// @retval 0 正常（候補が複数ある時はダイアログ表示）
int DiskTHDParser::Check(wxInputStream &istream, const DiskTypeHints *disk_hints, const DiskParam *disk_param, DiskParamPtrs &disk_params, DiskParam &manual_param)
{
	istream.SeekI(0);

	thd_dsk_header_t header;
	size_t len = istream.Read(&header, sizeof(header)).LastRead();
	if (len < sizeof(header)) {
		// too short
		p_result->SetError(DiskResult::ERRV_DISK_TOO_SMALL, 0);
		return p_result->GetValid();
	}

	int tracks_per_side = (int)wxUINT16_SWAP_ON_BE(header.tracks);
	if (tracks_per_side < 32 || tracks_per_side > 99999) {
		// invalid
		p_result->SetError(DiskResult::ERRV_TRACKS_HEADER, 0, tracks_per_side);
		return p_result->GetValid();
	}
	int sector_size = 256;
	int sectors_per_track = 33;
	int sides_per_disk = 8;

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
			0,
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
