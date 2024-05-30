/// @file diskthdparser.h
///
/// @brief THDディスクイメージパーサ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISKTHD_PARSER_H
#define DISKTHD_PARSER_H

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

/// THDディスクパーサー
class DiskTHDParser : public DiskPlainParser
{
private:

public:
	DiskTHDParser(DiskImageFile *file, short mod_flags, DiskResult *result);
	~DiskTHDParser();

	/// チェック
	int Check(wxInputStream &istream, const DiskTypeHints *disk_hints, const DiskParam *disk_param, DiskParamPtrs &disk_params, DiskParam &manual_param);
	/// 解析
	int Parse(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param);
};

#endif /* DISKTHD_PARSER_H */
