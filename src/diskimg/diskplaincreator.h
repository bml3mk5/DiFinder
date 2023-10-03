/// @file diskplaincreator.h
///
/// @brief ベタディスクイメージ作成
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISKPLAIN_CREATOR_H
#define DISKPLAIN_CREATOR_H

#include "../common.h"
#include <wx/string.h>

class DiskParam;
class DiskImageFile;
class DiskResult;

/// ベタディスクの新規作成
class DiskPlainCreator
{
private:
	wxString m_diskname;
	const DiskParam *p_param;
	bool m_write_protect;
	DiskImageFile *p_file;
	DiskResult *p_result;

	/// ディスクデータの作成
	wxUint32 CreateFile();

public:
	DiskPlainCreator(const wxString &diskname, const DiskParam &param, bool write_protect, DiskImageFile *file, DiskResult &result);
	~DiskPlainCreator();

	/// ディスクイメージの新規作成
	int Create();
};

#endif /* DISKPLAIN_CREATOR_H */
