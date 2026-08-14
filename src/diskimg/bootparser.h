/// @file bootparser.h
///
/// @brief ブートストラップパーサ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef BOOT_PARSER_H
#define BOOT_PARSER_H

#include "../common.h"
#include "diskparser.h"

class wxCharBuffer;
class wxInputStream;
class DiskImageFile;
class DiskImageDisk;
class DiskResult;
class DiskParam;
class BootParam;
class BootKeywords;
class BootKeyword;

/// ブートストラップパーサー
class BootParser : public DiskImageParser
{
private:
	double ParseBootKeywords(const wxUint8 *ipl_buf, size_t ipl_len, const BootParam &boot_param) const;
	int ParseBootKeywordString(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const;
	int ParseBootKeywordArrayString(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const;
	int ParseBootKeywordStringSub(const wxUint8 *ipl_buf, size_t ipl_len, const wxCharBuffer &key, int st, int ed) const;
	int ParseBootKeywordRegex(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const;

	size_t ReadHead(wxInputStream &istream, wxUint8 *ipl_buf, size_t ipl_size);
	const BootParam *ParseBootStrap(const wxUint8 *ipl_buf, size_t ipl_size, const DiskParam *disk_param, const BootParam *boot_param);

	bool ParsePC98x1IPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	bool ParseOS9X68K24IPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	bool ParseOS9X68K24SCSIIPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	bool ParseHu68kIPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	bool ParseHu68kSCSIIPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	bool ParseFMRIPL(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	bool ParseMacHFSVolume(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	bool ParseSuperFloppyVolume(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	bool ParsePCATMBR(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 *ipl, size_t ipl_size);
	int ParsePCATOnePartition(wxInputStream &istream, int disk_number, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 ident, wxUint32 start_block, wxUint32 block_size, const wxString &prefix);
	bool ParsePCATEBR(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param, int num, int idx, int &disk_number, wxUint32 start_block, wxUint32 &next_block);
	int ParsePCATExtPartition(wxInputStream &istream, int num, int disk_number, const DiskParam *disk_param, const BootParam *boot_param, wxUint8 ident, wxUint32 start_block, wxUint32 block_size);

	static wxUint32 ConvToLBA(wxUint64 disk_size, const wxUint8 *chs);

protected:
	BootParser();

	int ParseBoot(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param);

	virtual DiskImageDisk *ParseDisk(wxInputStream &istream, int disk_number, wxUint32 start_block, wxUint32 block_size, const DiskParam *disk_param) = 0;

public:
	BootParser(DiskImageFile *file, short mod_flags, DiskResult *result);
	virtual ~BootParser();
};

#endif /* BOOT_PARSER_H */
