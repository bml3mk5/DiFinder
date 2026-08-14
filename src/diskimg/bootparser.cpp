/// @file bootparser.cpp
///
/// @brief ブートストラップパーサー
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "bootparser.h"
#include <wx/stream.h>
#include <wx/regex.h>
#include <wx/variant.h>
#include "diskimage.h"
#include "diskparam.h"
#include "bootparam.h"
#include "diskresult.h"
#include "../logging.h"


// ブートストラップ構造
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

struct st_fmr_partition_t {
	wxUint8 sig[6];
	wxUint8 reserved1[4];
	wxUint32 block_size;	// LE
	wxUint8 reserved2[2];
	wxUint8 mo_flag;
	wxUint8 reserved3[15];
};

struct st_fmr_partition_1 {
	wxUint8 boot_flag;
	wxUint8 system_id;
	wxUint32 start_block;	// LE
	wxUint32 block_size;	// LE
	wxUint8 reserved[6];
	wxUint8 name[10];
	wxUint8 password[6];
	wxUint8 desc[16];
};

struct st_fmr_partition_h {
	wxUint8 sig[4];		// "IPL4"
	wxUint8 jmp[3];
	wxUint8 desc[20];	// "HD IPL V1.1 89/01/14."
	wxUint8 code[485];
	struct st_fmr_partition_t top;
	struct st_fmr_partition_1 p[10];
};

union st_pcat_chs_h {
	struct {
		wxUint8 h;
		wxUint8 c_s;	// bit7-6:cylinder 2bits  bit5-0:sector
		wxUint8 c;
	};
	wxUint8 b[3];
};

struct st_pcat_mbr_partition_h {
	wxUint8  boot_flag;
	union st_pcat_chs_h start_chs;	// CHS指定
	wxUint8  ident;					// 種類
	union st_pcat_chs_h last_chs;	// CHS指定
	wxUint32 start_lba;	// LBA指定(LE)
	wxUint32 blocks;	// LBA指定(LE)
};

struct st_pcat_mbr_h {
	wxUint8 loader[0x1be];
	struct st_pcat_mbr_partition_h p[4];
	wxUint16 signature;
};
#pragma pack()

/// コンストラクタ
BootParser::BootParser()
	: DiskImageParser()
{
}

/// コンストラクタ
BootParser::BootParser(DiskImageFile *file, short mod_flags, DiskResult *result)
	: DiskImageParser(file, mod_flags, result)
{
}

BootParser::~BootParser()
{
}

/// 指定したブートストラップ種類でバッファを解析する
/// @param [in] ipl_buf バッファ
/// @param [in] ipl_len バッファ長さ
/// @param [in] boot_param ブートストラップ種類
/// @return 0.0 - 1.0 尤もらしい:1.0
double BootParser::ParseBootKeywords(const wxUint8 *ipl_buf, size_t ipl_len, const BootParam &boot_param) const
{
	double valid_ratio = 0.0;
	const BootKeywords *keywords = &boot_param.GetKeywords();
	int valid = 0;
	int count = 0;
	for(size_t n = 0; n<keywords->Count(); n++) {
		const BootKeyword *keyword = &keywords->Item(n);
		double dweight = keyword->GetWeight();
		if (dweight < 0.1) continue;
		int weight = (int)(1.0 / dweight);
		switch(keyword->GetKeywordType()) {
		case BootKeyword::KString:
			count += weight;
			valid += ParseBootKeywordString(ipl_buf, ipl_len, *keyword);
			break;
		case BootKeyword::KRegex:
			count += weight;
			valid += ParseBootKeywordRegex(ipl_buf, ipl_len, *keyword);
			break;
		case BootKeyword::KArrayString:
			count += weight;
			valid += ParseBootKeywordArrayString(ipl_buf, ipl_len, *keyword);
			break;
		default:
			break;
		}

	}
	if (count > 0) {
		valid_ratio = (double)valid/count;
	}
	return valid_ratio;
}

/// バッファ内に指定したキーワードに一致するバイト列があるか
/// @param [in] ipl_buf バッファ
/// @param [in] ipl_len バッファ長さ
/// @param [in] keyword キーワード文字列
/// @return 一致する:1
int BootParser::ParseBootKeywordString(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const
{
	int st = keyword.GetStartPos();
	int ed = keyword.GetLastPos();
	wxCharBuffer key = keyword.GetKeyword().GetString().To8BitData();
	return ParseBootKeywordStringSub(ipl_buf, ipl_len, key, st, ed);
}

/// バッファ内に指定したキーワードリストのいずれかに一致するバイト列があるか
/// @param [in] ipl_buf バッファ
/// @param [in] ipl_len バッファ長さ
/// @param [in] keyword キーワードリスト
/// @return 一致する:1
int BootParser::ParseBootKeywordArrayString(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const
{
	int st = keyword.GetStartPos();
	int ed = keyword.GetLastPos();
	int valid = 0;
	wxArrayString arr = keyword.GetKeyword().GetArrayString();
	for(size_t n=0; n<arr.Count() && valid == 0; n++) {
		wxCharBuffer key = arr.Item(n).To8BitData();
		valid += ParseBootKeywordStringSub(ipl_buf, ipl_len, key, st, ed);
	}
	return valid;
}

/// バッファ内の指定した位置に指定したキーワードがあるか
/// @param [in] ipl_buf バッファ
/// @param [in] ipl_len バッファ長さ
/// @param [in] key キーワード文字列
/// @param [in] st バッファの比較開始位置 or -1(全体)
/// @param [in] ed バッファの比較終了位置 or -1(全体)
/// @return 一致する:1
int BootParser::ParseBootKeywordStringSub(const wxUint8 *ipl_buf, size_t ipl_len, const wxCharBuffer &key, int st, int ed) const
{
	int slen = (int)(ipl_len - key.length());
	ed = (ed >= 0 && ed < slen ? ed : slen);

	int valid = 0;
	if (st >= 0 && ed >= st) {
		// 比較位置指定あり
		for (int pos = st; pos <= ed; pos++) {
			if (memcmp(&ipl_buf[pos], key.data(), key.length()) == 0) {
				// match keyword
				valid++;
				break;
			}
		}
	} else if (key.length() >= 3) {
		// 比較位置指定なし 総当たり
		for (int pos = 0; pos < slen; pos++) {
			if (memcmp(&ipl_buf[pos], key.data(), key.length()) == 0) {
				// match keyword
				valid++;
				break;
			}
		}
	}
	return valid;
}

/// バッファ内に指定した正規表現に一致するバイト列があるか
/// @param [in] ipl_buf バッファ
/// @param [in] ipl_len バッファ長さ
/// @param [in] keyword キーワード(正規表現)
/// @return 一致する:1
int BootParser::ParseBootKeywordRegex(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const
{
	int st = keyword.GetStartPos();
	wxRegEx re(keyword.GetKeyword().GetString());

//	int slen = (int)ipl_len;

	int flags = 0;
	int valid = 0;
	if (st >= 0) {
		// 比較位置指定あり
		if (re.Matches((const wxChar *)&ipl_buf[st], flags, ipl_len - (size_t)st)) {
			// match keyword
			valid++;
		}
	} else {
		// 比較位置指定なし 総当たり
		if (re.Matches((const wxChar *)ipl_buf, flags, ipl_len)) {
			// match keyword
			valid++;
		}
	}
	return valid;
}

/// ブートストラップの解析
/// @param [in] istream    解析対象データ
/// @param [in] disk_param ディスクパラメータ
/// @param [in] boot_param ブートストラップ種類(nullable)
/// @note boot_param が NULL のときは、尤もらしいものを検索する
int BootParser::ParseBoot(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param)
{
	// MBRを解析
	wxUint8 ipl[0x4000];

	size_t ipl_len = ReadHead(istream, ipl, sizeof(ipl));
	if (!ipl_len) {
		return p_result->GetValid();
	}

	boot_param = ParseBootStrap(ipl, ipl_len, disk_param, boot_param);
	if (!boot_param) {
		return p_result->GetValid();
	}

	p_file->SetDescription(boot_param->GetDescription());

	// 決定したブートストラップを解析
	switch(boot_param->GetBootType()) {
	case BT_PC98_IPL:
		// maybe PC-98x1
		ParsePC98x1IPL(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	case BT_OS9_X68K_24_IPL:
		ParseOS9X68K24IPL(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	case BT_OS9_X68K_24_SCSI_IPL:
		ParseOS9X68K24SCSIIPL(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	case BT_HU68K_IPL:
		// maybe Human68k
		ParseHu68kIPL(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	case BT_HU68K_SCSI_IPL:
		// maybe Human68k SCSI
		ParseHu68kSCSIIPL(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	case BT_FMR_IPL:
		// maybe FMR / FM Towns
		ParseFMRIPL(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	case BT_MAC_HFS:
		// maybe MAC HFS volume
		ParseMacHFSVolume(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	case BT_SUPER_FD:
		// maybe super floppy type (FAT)
		ParseSuperFloppyVolume(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	case BT_PCAT_MBR:
		// maybe master boot record
		ParsePCATMBR(istream, disk_param, boot_param, ipl, sizeof(ipl));
		break;
	default:
		break;
	}

	p_file->SetDiskParam(*disk_param);

	return p_result->GetValid();
}

/// ディスク先頭の1KBを読む
/// @param [in] istream    解析対象データ
/// @param [out] ipl_buf   ディスク先頭データ
/// @param [in] ipl_size   ディスク先頭データのサイズ
size_t BootParser::ReadHead(wxInputStream &istream, wxUint8 *ipl_buf, size_t ipl_size)
{
	// 先頭から1KBが読めるか
	size_t len = istream.Read(ipl_buf, 0x400).LastRead();
	if (len < 0x400) {
		p_result->SetError(DiskResult::ERRV_DISK_TOO_SMALL, 0);
		return 0;
	}
	// のこりを読む
	memset(&ipl_buf[0x400], 0, ipl_size - 0x400);
	len += istream.Read(&ipl_buf[0x400], ipl_size - 0x400).LastRead();
	return len;
}

/// ディスク先頭にあるブートストラップを解析
/// @param [in] ipl_buf    ディスク先頭データ
/// @param [in] ipl_size   ディスク先頭データのサイズ
/// @param [in] disk_param ディスクパラメータ
/// @param [in] boot_param ブートストラップ種類(nullable)
/// @note boot_param が NULL のときは、尤もらしいものを検索する
/// @return ブートストラップ
const BootParam *BootParser::ParseBootStrap(const wxUint8 *ipl_buf, size_t ipl_size, const DiskParam *disk_param, const BootParam *boot_param)
{
	// MBR領域を検索
	size_t st;
	size_t ed; 
	if (boot_param) {
		st = (size_t)gBootTemplates.IndexOf(boot_param);
		ed = st + 1;
	} else {
		st = 0;
		ed = gBootTemplates.Count();
	}
	wxArrayDouble valid_ratios;
	for(size_t i=st; i<ed; i++) {
		const BootParam *param = &gBootTemplates.Item(i);
		myLog.SetInfo(wxString::Format(wxT("Parsing Boot Type #%d: "), (int)i) + param->GetBootTypeName());
		double valid_ratio = ParseBootKeywords(ipl_buf, ipl_size, *param);
		myLog.SetInfo(wxT("  Result => %.2f"), valid_ratio);
		valid_ratios.Add(valid_ratio);
	}

	if (boot_param) {
		if (valid_ratios.Item(0) < 0.1) {
			myLog.SetInfo(wxT("Invalid bootstrap"));
			p_result->SetWarn(DiskResult::ERR_INVALID_BOOTSTRAP);
		} else {
			myLog.SetInfo(wxT("Decided: #%d => %.2f"), (int)st, valid_ratios.Item(0));
		}
	} else {
		// 尤もらしいもの
		int match_pos = -1;
		double max_valid_ratio = -1.0;
		for(size_t i=st, n=0; i<ed; i++, n++) {
			double val = valid_ratios.Item(n);
			if (val >= 0.1 && max_valid_ratio < val) {
				match_pos = (int)i;
				max_valid_ratio = val;
			}
		}

		if (match_pos < 0) {
			// 一致するものがない
			myLog.SetInfo(wxT("No Bootstrap Decided"));
			p_result->SetWarn(DiskResult::ERR_NO_BOOTSTRAP);
			return NULL;
		} else {
			myLog.SetInfo(wxT("Decided: #%d => %.2f"), match_pos, max_valid_ratio);
		}

		boot_param = &gBootTemplates.Item(match_pos);
	}

	return boot_param;
}

/// PC-98x1 IPL 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParsePC98x1IPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
{
	// パーティション情報
	wxUint32 addr = (wxUint32)disk_param->GetSectorSize();
	if (addr < 256) addr = 512;

	int disk_number = 0;
	wxUint32 total_block = 0;
	wxUint32 start_block = 0;
	wxUint32 block_size = 0;
//	wxUint32 tracks = (wxUint32)disk_param->GetTracksPerSide();
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
			disk->SetBasicTypes(boot_param->GetBasicTypes());
		}

		disk_number++;
		addr+=(wxUint32)sizeof(struct st_pc98_partition_h);
	} while (start_block != 0 && disk_number < 10);

	p_file->SetBlockSize(total_block);

	if (disk_number <= 0) {
		// パーティション情報がない
		p_result->SetWarn(DiskResult::ERR_NO_PARTITION);
		return false;
	}
	return true;
}

/// OS-9/X68000 SASI IPL 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParseOS9X68K24IPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
{
	// maybe OS-9/X68000 v2.4 one partition
	wxUint32 block_size = disk_param->GetTracksPerSide() * disk_param->GetSidesPerDisk() * disk_param->GetSectorsPerTrack();
	DiskImageDisk *disk = ParseDisk(istream, 0, 0, block_size, disk_param);
	if (disk) {
		disk->SetName(&ipl[2], 11);
		disk->SetDescription(&ipl[2], 11);
		disk->SetBasicTypes(boot_param->GetBasicTypes());
		return true;
	}
	return false;
}

/// OS-9/X68000 SCSI IPL 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParseOS9X68K24SCSIIPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
{
	// maybe OS-9/X68000 v2.4 one partition
	int start_block = 0x400 / disk_param->GetSectorSize();
	wxUint32 block_size = disk_param->GetTracksPerSide() * disk_param->GetSidesPerDisk() * disk_param->GetSectorsPerTrack();
	DiskImageDisk *disk = ParseDisk(istream, 0, start_block, block_size, disk_param);
	if (disk) {
		disk->SetName(&ipl[0x402], 11);
		disk->SetDescription(&ipl[0x402], 11);
		disk->SetBasicTypes(boot_param->GetBasicTypes());
		return true;
	}
	return false;
}

/// Human68k SASI IPL 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParseHu68kIPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
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
			disk->SetBasicTypes(boot_param->GetBasicTypes());
		}

		disk_number++;
		addr+=(wxUint32)sizeof(struct st_human68k_partition_1);
	} while (start_block != 0);

	if (disk_number <= 0) {
		// パーティション情報がない
		p_result->SetWarn(DiskResult::ERR_NO_PARTITION);
		return false;
	}
	return true;
}

/// Human68k SCSI IPL 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParseHu68kSCSIIPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
{
	// ヘッダ
	// struct st_x68k_scsi_h *h0 = (struct st_x68k_scsi_h *)&ipl[0];
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
			disk->SetBasicTypes(boot_param->GetBasicTypes());
		}

		disk_number++;
		addr+=(wxUint32)sizeof(struct st_human68k_partition_1);
	} while (start_block != 0);

	if (disk_number <= 0) {
		// パーティション情報がない
		p_result->SetWarn(DiskResult::ERR_NO_PARTITION);
		return false;
	}
	return true;
}

/// FMR / FMTowns IPL 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParseFMRIPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
{
	// パーティション情報
	struct st_fmr_partition_h *h = (struct st_fmr_partition_h *)ipl;

	int disk_number = 0;
	wxUint32 start_block = 0;
	wxUint32 block_size = 0;
	do {
		struct st_fmr_partition_1 *p = &h->p[disk_number];
		if (p->system_id == 0) {
			break;
		}

		start_block = wxUINT32_SWAP_ON_BE(p->start_block);
		if (start_block == 0) {
			break;
		}
		block_size = wxUINT32_SWAP_ON_BE(p->block_size);
		if (block_size == 0) {
			break;
		}

		DiskImageDisk *disk = ParseDisk(istream, disk_number, start_block, block_size, disk_param);
		if (disk) {
			disk->SetName(p->desc, sizeof(p->desc));
			disk->SetDescription(p->desc, sizeof(p->desc));
			disk->SetBasicTypes(boot_param->GetBasicTypes());
		}

		disk_number++;
	} while (disk_number < 10);

	if (disk_number <= 0) {
		// パーティション情報がない
		p_result->SetWarn(DiskResult::ERR_NO_PARTITION);
		return false;
	}
	return true;
}

/// Mac HFS 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParseMacHFSVolume(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
{
	int disk_number = 0;
	wxUint32 start_block = 0;
	wxUint32 block_size = disk_param->CalcNumberOfBlocks();
	DiskImageDisk *disk = ParseDisk(istream, disk_number, start_block, block_size, disk_param);
	if (disk) {
		disk->SetName(_("HFS"));
		disk->SetDescription(_("HFS Volume"));
		disk->SetBasicTypes(boot_param->GetBasicTypes());
		return true;
	}
	return false;
}

/// スーパーフロッピー形式 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParseSuperFloppyVolume(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
{
	int disk_number = 0;
	wxUint32 start_block = 0;
	wxUint32 block_size = disk_param->CalcNumberOfBlocks();
	DiskImageDisk *disk = ParseDisk(istream, disk_number, start_block, block_size, disk_param);
	if (disk) {
		disk->SetName(_("FAT"));
		disk->SetDescription(_("FAT"));
		disk->SetBasicTypes(boot_param->GetBasicTypes());
		return true;
	}
	return false;
}

/// PC-AT MBR 解析
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ipl         ディスク先頭データ
/// @param [in] ipl_size    ディスク先頭データのサイズ
/// @return false:パーティションがない
bool BootParser::ParsePCATMBR(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size)
{
	wxUint64 disk_size = (wxUint64)istream.GetLength();
	// ヘッダ
	struct st_pcat_mbr_h *h = (struct st_pcat_mbr_h *)ipl;

	int disk_number = 0;
	wxUint32 start_block = 0;
	wxUint32 block_size = 0;
	for(int n=0; n<4; n++) {
		struct st_pcat_mbr_partition_h *p = &h->p[n];
		start_block = wxUINT32_SWAP_ON_BE(p->start_lba);
		if (start_block == 0) {
			start_block = ConvToLBA(disk_size, p->start_chs.b);
		}
		if (start_block == 0) {
			continue;
		}

		block_size = wxUINT32_SWAP_ON_BE(p->blocks);
		if (block_size == 0) {
			block_size = ConvToLBA(disk_size, p->last_chs.b);
			block_size -= start_block;
		}
		if (block_size == 0) {
			continue;
		}

		switch(p->ident) {
		case 0x05:
		case 0x15:
		case 0x0f:
		case 0x1f:
			// 拡張DOSパーティション
			disk_number = ParsePCATExtPartition(istream, n, disk_number, disk_param, boot_param, p->ident, start_block, block_size);
			break;
		default:
			// 基本パーティション
			disk_number = ParsePCATOnePartition(istream, disk_number, disk_param, boot_param, p->ident, start_block, block_size
				, wxString::Format(_("#%d Standard"), n));
			break;
		}
	}

	if (disk_number <= 0) {
		// パーティション情報がない
		p_result->SetWarn(DiskResult::ERR_NO_PARTITION);
		return false;
	}
	return true;
}

/// パーティション１つ解析(PC-AT)
/// @param [in] istream     解析対象データ
/// @param [in] disk_number ディスク番号
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ident       パーティション種類
/// @param [in] start_block 開始ブロック番号
/// @param [in] block_size  ブロックサイズ
/// @param [in] prefix      説明
/// @return 解析後の最大ディスク番号
int BootParser::ParsePCATOnePartition(wxInputStream &istream, int disk_number, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 ident, wxUint32 start_block, wxUint32 block_size, const wxString &prefix)
{
	DiskImageDisk *disk = ParseDisk(istream, disk_number, start_block, block_size, disk_param);
	if (disk) {
		wxString desc;
		switch(ident) {
		case 0x00:
			desc = _("Empty");
			break;
		case 0x05:
		case 0x15:
		case 0x0f:
		case 0x1f:
			desc = _("Extended DOS");
			break;
		case 0x01:
		case 0x11:
			desc = wxT("FAT12");
			break;
		case 0x04:
		case 0x14:
		case 0x06:
		case 0x16:
		case 0x0e:
		case 0x1e:
			desc = wxT("FAT16");
			break;
		case 0x0b:
		case 0x1b:
		case 0x0c:
		case 0x1c:
			desc = wxT("FAT32");
			break;
		case 0x07:
		case 0x17:
			desc = wxT("HPFS/NTFS/exFAT");
			break;
		case 0x82:
			desc = _("Linux swap");
			break;
		case 0x83:
		case 0x93:
			desc = wxT("Linux");
			break;
		case 0x85:
			desc = _("Extended Linux");
			break;
		case 0xa5:
			desc = _("FreeBSD");
			break;
		default:
			desc = _("Partition");
			break;
		}
		desc += wxString::Format(wxT(" (%02Xh)"), ident);
		desc = prefix + wxT(" (") + desc + wxT(")");

		disk->SetName(desc);
		disk->SetDescription(desc);
		disk->SetBasicTypes(boot_param->GetBasicTypes());
	}

	disk_number++;

	return disk_number;
}

/// 拡張パーティション内部のパーティション解析(PC-AT)
/// @param [in] istream     解析対象データ
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] num         MBRパーティション番号
/// @param [in] idx         拡張パーティション番号
/// @param [in,out] disk_number ディスク番号
/// @param [in] ident       パーティション種類
/// @param [in] start_block 開始ブロック番号
/// @param [in] next_block  次パーティションの開始ブロック番号
/// @return true:次パーティションあり false:最終パーティション
bool BootParser::ParsePCATEBR(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, int num, int idx, int &disk_number, wxUint32 start_block, wxUint32 &next_block)
{
	wxUint8 ebr[0x200];
	int sector_size = disk_param->GetSectorSize();

	istream.SeekI(((wxFileOffset)start_block + next_block) * sector_size);
	if (istream.Read(ebr, sizeof(ebr)).LastRead() < sizeof(ebr)) {
		return false;
	}
	struct st_pcat_mbr_h *h = (struct st_pcat_mbr_h *)ebr;
	// check ident
	if (h->signature != wxUINT32_SWAP_ON_BE(0xaa55)) {
		return false;
	}
	// partition info
	struct st_pcat_mbr_partition_h *p = &h->p[0];
	wxUint32 ext_start_block = wxUINT32_SWAP_ON_BE(p->start_lba);
	wxUint32 ext_block_size = wxUINT32_SWAP_ON_BE(p->blocks);

	ext_start_block += start_block;
	ext_start_block += next_block;
	disk_number = ParsePCATOnePartition(istream, disk_number, disk_param, boot_param, p->ident, ext_start_block, ext_block_size
		, wxString::Format(_("#%d Extended#%d"), num, idx));

	// next entry
	struct st_pcat_mbr_partition_h *next = &h->p[1];
	wxUint32 next_start_block = wxUINT32_SWAP_ON_BE(next->start_lba);
	if (next_start_block == 0) {
		// end of partition list
		return false;
	}
	wxUint32 next_block_size = wxUINT32_SWAP_ON_BE(next->blocks);
	if (next_block_size == 0) {
		// end of partition list
		return false;
	}

	next_block = next_start_block;
	return true;
}

/// 拡張パーティション解析(PC-AT)
/// @param [in] istream     解析対象データ
/// @param [in] num         MBRパーティション番号
/// @param [in] disk_number ディスク番号
/// @param [in] disk_param  ディスクパラメータ
/// @param [in] boot_param  ブートストラップ種類
/// @param [in] ident       パーティション種類
/// @param [in] start_block 開始ブロック番号
/// @param [in] block_size  ブロックサイズ
/// @return 解析後の最大ディスク番号
int BootParser::ParsePCATExtPartition(wxInputStream &istream, int num, int disk_number, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 ident, wxUint32 start_block, wxUint32 block_size)
{
	wxUint32 next_block = 0;
	bool keep = true;
	int idx = 0;
	while(keep && idx < 100) {
		keep = ParsePCATEBR(istream, disk_param, boot_param, num, idx, disk_number, start_block, next_block);
		if (!keep && idx == 0) {
			disk_number = ParsePCATOnePartition(istream, disk_number, disk_param, boot_param, ident, start_block, block_size
				, wxString::Format(_("#%d"), num));
		}
		idx++;
	}
	return disk_number;
}

wxUint32 BootParser::ConvToLBA(wxUint64 disk_size, const wxUint8 *chs)
{
	const union st_pcat_chs_h *p = (const union st_pcat_chs_h *)chs;
	wxUint32 c = (wxUint32)(p->c | ((wxUint32)(p->c_s & 0xc0) << 2));
	wxUint32 h = (wxUint32)p->h;
	wxUint32 s = (wxUint32)(p->c_s & 0x3f);
	wxUint32 lba = s - 1;
	if (disk_size > (wxUint64)4032 * 1024 * 1024) {
		lba += (((c * 255) + h) * 63);
	} else if (disk_size > 2016 * 1024 * 1024) {
		lba += (((c * 128) + h) * 63);
	} else if (disk_size > 1008 * 1024 * 1024) {
		lba += (((c * 64) + h) * 63);
	} else if (disk_size > 504 * 1024 * 1024) {
		lba += (((c * 32) + h) * 63);
	} else {
		lba += (((c * 16) + h) * 63);
	}
	return lba;
}
