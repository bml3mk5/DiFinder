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
#include "bootparam.h"
#include "diskresult.h"
#include "../utils.h"

//
//
//
DiskPlainParser::DiskPlainParser(DiskImageFile *file, short mod_flags, DiskResult *result)
	: DiskImageParser(file, mod_flags, result)
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

#pragma pack(1)
struct st_pc98_partition_h {
	wxUint8 boot;
	wxUint8 syss;
	wxUint16 unknown1;
	wxUint8 ipl_r;
	wxUint8 ipl_h;
	wxUint16 ipl_c;
	wxUint8 start_r;
	wxUint8 start_h;
	wxUint16 start_c;
	wxUint8 end_r;
	wxUint8 end_h;
	wxUint16 end_c;
	wxUint8 sig[16];
};

struct st_human68k_partition_1 {
	wxUint8 sig[8];
	wxUint8 boot_flags;
	wxUint8 start_block[3];
	wxUint32 block_size;
};

struct st_human68k_partition_h {
	wxUint8 sig[4];
	wxUint32 free_start;	// 未使用領域開始ブロック
	wxUint32 block_size;
	wxUint32 limit_size;
};

struct st_x68k_scsi_h {
	wxUint8 sig[8];
	wxUint16 sector_size;	// BE
	wxUint32 max_block;
	wxUint16 unknown;
	wxUint8 reserved[0x3f0];
};
#pragma pack()

/// ディスク1ファイルの構造の解析
int DiskPlainParser::ParseFile(wxInputStream &istream, const DiskParam *disk_param)
{
	// MBRを解析
	wxUint8 ipl[0x4000];

	// 先頭から1KBが読めるか
	size_t len = istream.Read(ipl, 0x400).LastRead();
	if (len < 0x400) {
		p_result->SetError(DiskResult::ERRV_DISK_TOO_SMALL, 0);
		return p_result->GetValid();
	}
	// のこりを読む
	memset(&ipl[0x400], 0, sizeof(ipl) - 0x400);
	len = istream.Read(&ipl[0x400], sizeof(ipl) - 0x400).LastRead();

	// MBR領域を検索
	wxArrayDouble valid_ratios;
	for(size_t i=0; i<gBootTemplates.Count(); i++) {
		const BootParam *boot_param = &gBootTemplates.Item(i);
		const BootKeywords *keywords = &boot_param->GetKeywords();
		int valid = 0;
		int count = 0;
		for(size_t n = 0; n<keywords->Count(); n++) {
			const BootKeyword *keyword = &keywords->Item(n);
			int pos = keyword->GetPos();
			wxCharBuffer key = keyword->GetKeyword().To8BitData();
			bool match = false;
			if (pos >= 0 && pos < (int)(sizeof(ipl) - key.length())) {
				// 比較位置指定あり
				count++;
				if (memcmp(&ipl[pos], key.data(), key.length()) == 0) {
					// match keyword
					valid++;
					match = true;
				}
			}
			if (!match && key.length() >= 3) {
				// 比較位置指定なし 総当たり
				count++;
				for (pos = 0; pos < (int)(sizeof(ipl) - (int)key.length()); pos++) {
					if (memcmp(&ipl[pos], key.data(), key.length()) == 0) {
						// match keyword
						valid++;
						match = true;
						break;
					}
				}
			}
		}
		if (count > 0) {
			valid_ratios.Add((double)valid/count);
		} else {
			valid_ratios.Add(0.0);
		}
	}

	// 尤もらしいもの
	int match_pos = -1;
	double max_valid_ratio = -1.0;
	for(size_t i=0; i<gBootTemplates.Count(); i++) {
		double val = valid_ratios.Item(i);
		if (val >= 0.1 && max_valid_ratio < val) {
			match_pos = (int)i;
			max_valid_ratio = val;
		}
	}

	if (match_pos < 0) {
		// 一致するものがない
		p_result->SetError(DiskResult::ERR_NO_BOOTSTRAP);
		return p_result->GetValid();
	}

	const BootParam *match_boot = &gBootTemplates.Item(match_pos);
	p_file->SetDescription(match_boot->GetDescription());

	switch(match_boot->GetBootType()) {
	case 1:
		// maybe PC-98x1
		{
			// パーティション情報
			wxUint32 addr = (wxUint32)disk_param->GetSectorSize();
			if (addr < 256) addr = 512;

			int disk_number = 0;
			wxUint32 total_block = 0;
			wxUint32 start_block = 0;
			wxUint32 block_size = 0;
//			wxUint32 tracks = (wxUint32)disk_param->GetTracksPerSide();
			wxUint32 sides = (wxUint32)disk_param->GetSidesPerDisk();
			wxUint32 sectors = (wxUint32)disk_param->GetSectorsPerTrack();
			do {
				struct st_pc98_partition_h *p = (struct st_pc98_partition_h *)&ipl[addr];
				if (p->boot == 0 && p->syss == 0) {
					break;
				}

				start_block = sectors * (sides * wxUINT16_SWAP_ON_BE(p->ipl_c) + p->ipl_h) + p->ipl_r;
				if (start_block == 0) {
					break;
				}

				block_size = sectors * (sides * wxUINT16_SWAP_ON_BE(p->end_c) + p->end_h) + p->end_r;
				total_block = block_size;

				block_size -= start_block;

				DiskImageDisk *disk = ParseDisk(istream, disk_number, start_block, block_size, disk_param);
				if (disk) {
					disk->SetName(p->sig, sizeof(p->sig));
					disk->SetDescription(p->sig, sizeof(p->sig));
					disk->SetBasicTypes(match_boot->GetBasicTypes());
				}

				disk_number++;
				addr+=(wxUint32)sizeof(struct st_pc98_partition_h);
			} while (start_block != 0);

			p_file->SetBlockSize(total_block);

			if (disk_number <= 0) {
				// パーティション情報がない
				p_result->SetError(DiskResult::ERR_NO_PARTITION);
			}
		}
		break;
	case 2:
		{
			// maybe OS-9/X68000 v2.4 one partition
			wxUint32 block_size = disk_param->GetTracksPerSide() * disk_param->GetSidesPerDisk() * disk_param->GetSectorsPerTrack();
			DiskImageDisk *disk = ParseDisk(istream, 0, 0, block_size, disk_param);
			if (disk) {
				disk->SetName(&ipl[2], 11);
				disk->SetDescription(&ipl[2], 11);
				disk->SetBasicTypes(match_boot->GetBasicTypes());
			}
		}
		break;
	case 4:
		{
			// maybe OS-9/X68000 v2.4 one partition
			int start_block = 0x400 / disk_param->GetSectorSize();
			wxUint32 block_size = disk_param->GetTracksPerSide() * disk_param->GetSidesPerDisk() * disk_param->GetSectorsPerTrack();
			DiskImageDisk *disk = ParseDisk(istream, 0, start_block, block_size, disk_param);
			if (disk) {
				disk->SetName(&ipl[0x402], 11);
				disk->SetDescription(&ipl[0x402], 11);
				disk->SetBasicTypes(match_boot->GetBasicTypes());
			}
		}
		break;
	case 3:
		// maybe Human68k
		{
			// パーティション情報
			struct st_human68k_partition_h *h = (struct st_human68k_partition_h *)&ipl[0x400];
			p_file->SetBlockSize(wxUINT32_SWAP_ON_LE(h->block_size));

			int disk_number = 0;
			wxUint32 start_block = 0;
			wxUint32 block_size = 0;
			wxUint32 addr = 0x410;
			do {
				struct st_human68k_partition_1 *p = (struct st_human68k_partition_1 *)&ipl[addr];
				start_block = (wxUint32)p->start_block[0] << 16 | (wxUint32)p->start_block[1] << 8 | p->start_block[2];
				if (start_block == 0) {
					break;
				}
				block_size = wxUINT32_SWAP_ON_LE(p->block_size);

				DiskImageDisk *disk = ParseDisk(istream, disk_number, start_block, block_size, disk_param);
				if (disk) {
					disk->SetName(p->sig, sizeof(p->sig));
					disk->SetDescription(p->sig, sizeof(p->sig));
					disk->SetBasicTypes(match_boot->GetBasicTypes());
				}

				disk_number++;
				addr+=(wxUint32)sizeof(struct st_human68k_partition_1);
			} while (start_block != 0);

			if (disk_number <= 0) {
				// パーティション情報がない
				p_result->SetError(DiskResult::ERR_NO_PARTITION);
			}
		}
		break;
	case 5:
		// maybe Human68k SCSI
		{
			// ヘッダ
//			struct st_x68k_scsi_h *h0 = (struct st_x68k_scsi_h *)&ipl[0];
			// パーティション情報
			struct st_human68k_partition_h *h = (struct st_human68k_partition_h *)&ipl[0x800];
			p_file->SetBlockSize(wxUINT32_SWAP_ON_LE(h->block_size));

			int disk_number = 0;
			wxUint32 start_block = 0;
			wxUint32 block_size = 0;
			wxUint32 addr = 0x810;
			wxUint32 sector_mag = 1024 / disk_param->GetSectorSize();
			do {
				struct st_human68k_partition_1 *p = (struct st_human68k_partition_1 *)&ipl[addr];
				start_block = (wxUint32)p->start_block[0] << 16 | (wxUint32)p->start_block[1] << 8 | p->start_block[2];
				if (start_block == 0) {
					break;
				}
				start_block *= sector_mag;

				block_size = wxUINT32_SWAP_ON_LE(p->block_size);
				block_size *= sector_mag;

				DiskImageDisk *disk = ParseDisk(istream, disk_number, start_block, block_size, disk_param);
				if (disk) {
					disk->SetName(p->sig, sizeof(p->sig));
					disk->SetDescription(p->sig, sizeof(p->sig));
					disk->SetBasicTypes(match_boot->GetBasicTypes());
				}

				disk_number++;
				addr+=(wxUint32)sizeof(struct st_human68k_partition_1);
			} while (start_block != 0);

			if (disk_number <= 0) {
				// パーティション情報がない
				p_result->SetError(DiskResult::ERR_NO_PARTITION);
			}
		}
		break;
	default:
		break;
	}

	p_file->SetDiskParam(*disk_param);

	return p_result->GetValid();
}

/// ベタファイルを解析
/// @param [in] istream    解析対象データ
/// @param [in] disk_param ディスクパラメータ
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskPlainParser::Parse(wxInputStream &istream, const DiskParam *disk_param)
{
	/// パラメータ
	if (!disk_param) {
		p_result->SetError(DiskResult::ERRV_INVALID_DISK, 0);
		return p_result->GetValid();
	}

	ParseFile(istream, disk_param);

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
				if (desided_all_sectors <= ival) {
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
		// ディスクサイズで割り切れる値を候補にする
		for(int sides = 8; sides >= 2; sides -= 2) {
			ival = desided_all_sectors % sides;
			if (ival == 0) {
				desided_sides = sides;
				desided_all_sectors = desided_all_sectors / sides;
				break;
			}
		}
		// セクタ数で割る
		for(int s = 33; s >= 2; s--) {
			ival = desided_all_sectors % s;
			if (ival == 0) {
				desided_tracks = desided_all_sectors / s;
				desided_sectors = s;
				break;
			}
		}
		// トラック数で割る
		const int ctracks[] = { 100, 0 };
		if (ival != 0) {
			for(int t = 0; ctracks[t] != 0; t++) {
				ival = desided_all_sectors % ctracks[t];
				if (ival == 0) {
					desided_tracks = ctracks[t];
					desided_sectors = desided_all_sectors / ctracks[t];
					break;
				}
			}
		}
		if (ival != 0) {
			for(int s = 1; s <= 32; s++) {
				if ((desided_all_sectors / s) < 10000) {
					desided_tracks = desided_all_sectors / s;
					desided_sectors = s;
					break;
				}
			}
		}
	}

	disk_param.SetDiskParam(
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
