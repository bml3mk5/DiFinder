/// @file diskplainparser.cpp
///
/// @brief べたディスクパーサー
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskplainparser.h"
#include <wx/stream.h>
#include <wx/dynarray.h>
#include "diskimage.h"
#include "fileparam.h"
#include "diskparam.h"
#include "bootparam.h"
#include "diskresult.h"
#include "../logging.h"
#include "../utils.h"

//
//
//
DiskPlainParser::DiskPlainParser()
	: BootParser()
{
}

DiskPlainParser::DiskPlainParser(const DiskPlainParser &src)
	: BootParser(src)
{
}

DiskPlainParser::DiskPlainParser(DiskImageFile *file, short mod_flags, DiskResult *result)
	: BootParser(file, mod_flags, result)
{
}

DiskPlainParser::~DiskPlainParser()
{
}

/// セクタデータの解析
wxUint32 DiskPlainParser::ParseSector(wxInputStream &istream, int disk_number, const DiskParam *disk_param, int block_number, int sector_size, bool single_density, bool is_dummy, DiskImageDisk *disk)
{
	wxFileOffset cpos = istream.TellI();
	wxFileOffset npos = istream.SeekI(sector_size, wxFromCurrent);
	wxUint32 len = (wxUint32)(npos - cpos);
	if (npos == wxInvalidOffset) {
		len = 0;
	}
	return len;
}

/// ディスクパーティションの解析
DiskImageDisk *DiskPlainParser::ParseDisk(wxInputStream &istream, int disk_number, wxUint32 start_block, wxUint32 block_size, const DiskParam *disk_param)
{
	DiskImageDisk *disk = p_file->NewImageDisk(disk_number, start_block, block_size);

	wxUint32 offset = disk->GetHeaderSize();
	int sector_size = disk_param->GetSectorSize();

	offset += (wxUint32)(block_size * sector_size);

	disk->SetSize(offset);

	if (p_result->GetValid() >= 0) {
		// ディスクを追加
		disk->AllocDiskBasics();
		p_file->Add(disk, m_mod_flags);
	} else {
		delete disk;
		disk = NULL;
	}

	return disk;
}

/// ベタファイルを解析
/// @param [in] istream    解析対象データ
/// @param [in] disk_param ディスクパラメータ
/// @param [in] boot_param ブートストラップ種類(nullable)
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskPlainParser::Parse(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param)
{
	/// パラメータ
	if (!disk_param) {
		p_result->SetError(DiskResult::ERRV_INVALID_DISK, 0);
		return p_result->GetValid();
	}

	ParseBoot(istream, disk_param, boot_param);

	return p_result->GetValid();
}

/// チェック
/// @param [in] istream       解析対象データ
/// @param [in] disk_hints    ディスクパラメータヒント("2D"など)
/// @param [in] disk_param    ディスクパラメータ disk_hints指定時はNullable
/// @param [out] disk_params  ディスクパラメータの候補
/// @param [out] manual_param 候補がないときのパラメータヒント
/// @retval 1 選択ダイアログ表示
/// @retval 0 正常（候補が複数ある時はダイアログ表示）
int DiskPlainParser::Check(wxInputStream &istream, const DiskTypeHints *disk_hints, const DiskParam *disk_param, DiskParamPtrs &disk_params, DiskParam &manual_param)
{
	int rc = 0;
	int stream_size = (int)istream.GetLength();

	// パラメータで判断
	if (disk_param != NULL) {
		// 特定している
		disk_params.Add(disk_param);
		return rc;
	}

	if (disk_hints != NULL) {
		// パラメータヒントあり

		// 優先順位の高い候補
		for(size_t i=0; i<disk_hints->Count(); i++) {
			wxString hint = disk_hints->Item(i).GetHint();
			const DiskParam *param = gDiskTemplates.Find(hint);
			if (param) {
				int disk_size_hint = param->CalcDiskSize();
				if (stream_size == disk_size_hint) {
					// ファイルサイズが一致
					disk_params.Add(param);
				}
			}
		}
	}

	// ディスクテンプレート全体から探す
	for(int mag = 1; mag <= 2; mag++) {
		bool separator = (disk_params.Count() == 0);
		for(size_t i=0; i<gDiskTemplates.Count(); i++) {
			const DiskParam *param = &gDiskTemplates.Item(i);
			if (param) {
				// 同じ候補がある場合スキップ
				if (disk_params.Index(param) >= 0) {
					continue;
				}

				int disk_size_hint = param->CalcDiskSize();
				if (stream_size * mag == disk_size_hint) {
					if (!separator) {
						disk_params.Add(NULL);
						separator = true;
					}
					// ファイルサイズが一致
					disk_params.Add(param);
				}
			}
		}
	}

	// 候補がないとき、ディスクサイズからパラメータを計算
	if (disk_params.Count() == 0) {
		CalcParamFromSize(stream_size, manual_param);
	}

	// GUIで選択ダイアログを表示させる
	rc = 1;

	return rc;
}

/// ディスクサイズから尤もらしいパラメータを計算する
void DiskPlainParser::CalcParamFromSize(int disk_size, DiskParam &disk_param)
{
	// セクタサイズヒント
	const int sec_size_hints[] = {
		512,256,1024,0
	};
	// セクタ数ヒント
	const int secs256[] = {	33, 0 };
	const int secs512[] = {	17, 0 };
	const int secs1024[] = { 9, 0 };
	const int *secs_hint[] = {
		secs256,
		secs512,
		secs1024,
		NULL
	};

	// トラック数はディスクサイズで
	int max_tracks = 9999;
	int min_tracks = 100;

	int ival = 0;
	int desided_sec_size_idx = -1;
	int desided_all_sectors = 0;

	for(int sec_size_idx = 0; sec_size_hints[sec_size_idx] != 0; sec_size_idx++) {
		// セクタサイズで割る
		ival = disk_size % sec_size_hints[sec_size_idx];	// 余り
		if (ival == 0) {
			desided_sec_size_idx = sec_size_idx;
			desided_all_sectors = disk_size / sec_size_hints[sec_size_idx];
			break;
		}
	}
	if (desided_sec_size_idx < 0) {
		// セクタサイズ候補なし
		return;
	}

	int desided_tracks = 0;
	int desided_sides = 0;
	int desided_sectors = 0;
	bool desided = false;
	const int *sectors = secs_hint[desided_sec_size_idx];
	for(int sides = 8; sides >= 2 && !desided; sides--) {
		for(int tracks = min_tracks; tracks <= max_tracks && !desided; tracks++) {
			for(int ss = 0; sectors[ss] != 0 && !desided; ss++) {
				ival = (tracks * sides * sectors[ss]);
				if (desided_all_sectors == ival) {
					desided_tracks = tracks;
					desided_sides = sides;
					desided_sectors = sectors[ss];
					desided = true;
					break;
				}
			}
		}
	}

	// 候補がない
	if (!desided) {
		int all_secs = desided_all_sectors;
		// ディスクサイズで割り切れる値を候補にする
		for(int sides = 8; sides >= 2; sides -= 2) {
			ival = desided_all_sectors % sides;
			if (ival == 0) {
				desided_sides = sides;
				all_secs = all_secs / sides;
				break;
			}
		}
		// セクタ数で割る
		for(int s = 33; s >= 2; s--) {
			ival = all_secs % s;
			if (ival == 0) {
				desided_tracks = all_secs / s;
				desided_sectors = s;
				break;
			}
		}
		// トラック数で割る
		const int ctracks[] = { 100, 0 };
		if (ival != 0) {
			for(int t = 0; ctracks[t] != 0; t++) {
				ival = all_secs % ctracks[t];
				if (ival == 0) {
					desided_tracks = ctracks[t];
					desided_sectors = all_secs / ctracks[t];
					break;
				}
			}
		}
		// 割り切れない！
		if (ival != 0) {
#if 0
			for(int s = 1; s <= 32; s++) {
				if ((desided_all_sectors / s) < 10000) {
					desided_tracks = desided_all_sectors / s;
					desided_sectors = s;
					break;
				}
			}
#else
			desided_sides = 0;
			desided_tracks = 0;
			desided_sectors = 0;
#endif
		}
	}

	disk_param.SetDiskParam(
		desided_all_sectors,
		desided_sides,
		desided_tracks,
		desided_sectors,
		sec_size_hints[desided_sec_size_idx],
		0,
		1,
		DiskParticulars(),
		DiskParticulars()
	);
}
