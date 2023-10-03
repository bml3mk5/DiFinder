/// @file diskwriter.h
///
/// @brief ディスクライター
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISK_WRITER_H
#define DISK_WRITER_H

#include "../common.h"
#include <wx/string.h>
#include "../diskimg/diskimage.h"


class wxOutputStream;

// ----------------------------------------------------------------------

/// ディスクライト時のオプション
class DiskWriteOptions
{
protected:
	bool m_trim_unused_data;
public:
	DiskWriteOptions();
	DiskWriteOptions(
		bool n_trim_unused_data
	);
	virtual ~DiskWriteOptions();
	virtual bool IsTrimUnusedData() const { return m_trim_unused_data; }
};

// ----------------------------------------------------------------------

/// ディスクライター
class DiskWriter : public DiskWriteOptions
{
private:
	wxString		 m_file_path;
	DiskImage		*p_image;
	DiskResult		*p_result;

	DiskWriter();
	DiskWriter(const DiskWriter &src);

	// 拡張子をさがす
	int CanSaveDiskByExt(int disk_number);
	// 拡張子で保存形式を判定
	int SelectCanSaveDisk(const wxString &file_format, int disk_number);
	// 拡張子をさがす
	int SaveDiskByExt(int disk_number, bool &support);
	// 拡張子で保存形式を判定
	int SelectSaveDisk(const wxString &file_format, int disk_number, bool &support);

public:
	DiskWriter(DiskImage *image, const wxString &path, const DiskWriteOptions &options, DiskResult *result);
	DiskWriter(DiskImage *image, const wxString &path, DiskResult *result);
	~DiskWriter();

	/// 出力先を開く
	int Open(const wxString &path);
	/// 出力先がオープンしているか
	bool IsOk() const;

	/// ディスクイメージを保存できるか
	int CanSave(const wxString &file_format);
	/// ストリームの内容をファイルに保存できるか
	int CanSaveDisk(int disk_number, const wxString &file_format);
	/// ディスクイメージの保存
	int Save(const wxString &file_format);
	/// ストリームの内容をファイルに保存
	int SaveDisk(int disk_number, const wxString &file_format);
};

/// 形式ごとのディスクライター
class DiskImageWriter
{
protected:
	DiskWriter *p_dw;
	DiskResult *p_result;

public:
	DiskImageWriter(DiskWriter *dw_, DiskResult *result_);
	virtual ~DiskImageWriter();

	/// ストリームの内容をファイルに保存できるか
	virtual int ValidateDisk(DiskImage *image, int disk_number);
	/// ストリームの内容をファイルに保存
	virtual int SaveDisk(DiskImage *image, int disk_number);
};

#endif /* DISK_WRITER_H */
