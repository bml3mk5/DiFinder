/// @file diskplainparser.h
///
/// @brief べたディスクイメージパーサ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISKPLAIN_PARSER_H
#define DISKPLAIN_PARSER_H

#include "../common.h"
#include "bootparser.h"

class wxInputStream;
class wxArrayString;
class DiskImageTrack;
class DiskImageDisk;
class DiskImageFile;
class DiskParam;
class DiskParamPtrs;
class DiskResult;
class FileParam;
class DiskTypeHints;
class BootParam;

/// べたディスクパーサー
class DiskPlainParser : public BootParser
{
protected:
	wxUint32 ParseSector(wxInputStream &istream, int disk_number, const DiskParam *disk_param, int block_number, int sector_size, bool single_density, bool is_dummy, DiskImageDisk *disk);
	DiskImageDisk *ParseDisk(wxInputStream &istream, int disk_number, wxUint32 start_block, wxUint32 block_size, const DiskParam *disk_param);

	void CalcParamFromSize(int disk_size, DiskParam &disk_param);

	DiskPlainParser();
	DiskPlainParser(const DiskPlainParser &src);

public:
	DiskPlainParser(DiskImageFile *file, short mod_flags, DiskResult *result);
	virtual ~DiskPlainParser();

	/// チェック
	virtual int Check(wxInputStream &istream, const DiskTypeHints *disk_hints, const DiskParam *disk_param, DiskParamPtrs &disk_params, DiskParam &manual_param);
	/// 解析
	virtual int Parse(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param);
};

#endif /* DISKPLAIN_PARSER_H */
