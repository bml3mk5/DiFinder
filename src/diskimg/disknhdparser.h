/// @file disknhdparser.h
///
/// @brief NHDディスクイメージパーサ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISKNHD_PARSER_H
#define DISKNHD_PARSER_H

#include "../common.h"
#include "diskplainparser.h"


class wxInputStream;
class wxArrayString;
class DiskImageTrack;
class DiskImageDisk;
class DiskImageFile;
class DiskParser;
class DiskParam;
class DiskParamPtrs;
class DiskResult;
class FileParam;
class DiskTypeHints;
class BootParam;

/// NHDディスクパーサー
class DiskNHDParser : public DiskPlainParser
{
private:
//	void CalcParamFromSize(int disk_size, DiskParam &disk_param);

public:
	DiskNHDParser(DiskImageFile *file, short mod_flags, DiskResult *result);
	~DiskNHDParser();

	/// チェック
	int Check(wxInputStream &istream, const DiskTypeHints *disk_hints, const DiskParam *disk_param, DiskParamPtrs &disk_params, DiskParam &manual_param);
	/// 解析
	int Parse(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param);
};

#endif /* DISKNHD_PARSER_H */
