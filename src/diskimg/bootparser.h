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
	double ParseBoot(const wxUint8 *ipl_buf, size_t ipl_len, const BootParam &boot_param) const;
	int ParseBootKeywordString(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const;
	int ParseBootKeywordArrayString(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const;
	int ParseBootKeywordStringSub(const wxUint8 *ipl_buf, size_t ipl_len, const wxCharBuffer &key, int st, int ed) const;
	int ParseBootKeywordRegex(const wxUint8 *ipl_buf, size_t ipl_len, const BootKeyword &keyword) const;

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
