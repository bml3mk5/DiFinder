/// @file diskwriter.cpp
///
/// @brief ディスクライター
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskwriter.h"
#include <wx/filename.h>
#include <wx/wfstream.h>
#include "diskimage.h"
#include "diskplainwriter.h"
#include "fileparam.h"
#include "diskresult.h"

//////////////////////////////////////////////////////////////////////
//
//
//
DiskWriteOptions::DiskWriteOptions()
{
	m_trim_unused_data = false;
}
DiskWriteOptions::DiskWriteOptions(
	bool n_trim_unused_data
) {
	m_trim_unused_data = n_trim_unused_data;
}
DiskWriteOptions::~DiskWriteOptions()
{
}

//////////////////////////////////////////////////////////////////////
//
//
//
DiskWriter::DiskWriter()
	: DiskWriteOptions()
{
	p_image = NULL;
	p_result = NULL;
}
DiskWriter::DiskWriter(const DiskWriter &src)
	: DiskWriteOptions(src)
{
}

/// @param [in,out] image ディスクイメージ
/// @param [in] path      ファイルパス
/// @param [in] options   出力時のオプション
/// @param [out] result   結果
DiskWriter::DiskWriter(DiskImage *image, const wxString &path, const DiskWriteOptions &options, DiskResult *result)
	: DiskWriteOptions(options)
{
	p_image = image;
	m_file_path = path;
	p_result = result;
	Open(path);
}

/// @param [in]  image    ディスクイメージ
/// @param [in] path      ファイルパス
/// @param [out] result   結果
DiskWriter::DiskWriter(DiskImage *image, const wxString &path, DiskResult *result)
	: DiskWriteOptions()
{
	p_image = image;
	m_file_path = path;
	p_result = result;
}

DiskWriter::~DiskWriter()
{
}

/// 出力先を開く
/// @param [in] path 出力先ファイルパス
/// @return 結果
int DiskWriter::Open(const wxString &path)
{
	return 0;
}

/// 出力先がオープンしているか
bool DiskWriter::IsOk() const
{
	return true;
}

/// 拡張子をさがす
/// @param [in] disk_number ディスク番号
int DiskWriter::CanSaveDiskByExt(int disk_number)
{
	int  rc = 0;

	// ファイル形式の指定がない場合
	wxFileName fpath(m_file_path);

	// 拡張子で判定
	wxString ext = fpath.GetExt();

	// サポートしているファイルか
	FileParam *fitem = gFileTypes.FindExt(ext);
	if (!fitem) {
		p_result->SetError(DiskResult::ERR_UNSUPPORTED);
		return p_result->GetValid();
	}

	// 指定した形式でファイル出力
	const FileParamFormats *formats = &fitem->GetFormats();
	for(size_t i=0; i<formats->Count(); i++) {
		const FileParamFormat *param_format = &formats->Item(i);
		rc = SelectCanSaveDisk(param_format->GetType(), disk_number);
		if (rc >= 0) {
			break;
		}
	}

	return rc;
}

/// 拡張子をさがす
/// @param [in] disk_number ディスク番号
/// @param [out] support    対応しているフォーマットならtrue
int DiskWriter::SaveDiskByExt(int disk_number, bool &support)
{
	int  rc = 0;

	// ファイル形式の指定がない場合
	wxFileName fpath(m_file_path);

	// 拡張子で判定
	wxString ext = fpath.GetExt();

	// サポートしているファイルか
	FileParam *fitem = gFileTypes.FindExt(ext);
	if (!fitem) {
		p_result->SetError(DiskResult::ERR_UNSUPPORTED);
		return p_result->GetValid();
	}

	// 指定した形式でファイル出力
	const FileParamFormats *formats = &fitem->GetFormats();
	for(size_t i=0; i<formats->Count(); i++) {
		const FileParamFormat *param_format = &formats->Item(i);
		rc = SelectSaveDisk(param_format->GetType(), disk_number, support);
		if (rc >= 0) {
			break;
		}
	}

	return rc;
}

/// ディスクイメージを保存できるか
/// @param [in] file_format ファイルフォーマット
/// @retval 0:できる
/// @retval 1:警告あり
int DiskWriter::CanSave(const wxString &file_format)
{
	return CanSaveDisk(-1, file_format);
}

/// ストリームの内容をファイルに保存できるか
/// @param [in] disk_number ディスク番号
/// @param [in] file_format ファイルフォーマット
/// @retval 0:できる
/// @retval 1:警告あり
int DiskWriter::CanSaveDisk(int disk_number, const wxString &file_format)
{
	int  rc = 0;
	if (file_format.IsEmpty()) {
		// ファイル形式の指定がない場合
		rc = CanSaveDiskByExt(disk_number);
	} else {
		// ファイル形式の指定あり
		rc = SelectCanSaveDisk(file_format, disk_number);
	}
	return rc;
}

/// ディスクイメージの保存
/// @param [in] file_format ファイルフォーマット
int DiskWriter::Save(const wxString &file_format)
{
	return SaveDisk(-1, file_format);
}

/// ストリームの内容をファイルに保存
/// @param [in] disk_number ディスク番号
/// @param [in] file_format ファイルフォーマット
int DiskWriter::SaveDisk(int disk_number, const wxString &file_format)
{
	int  rc = 0;
	bool support = false;

	if (!IsOk()) {
		p_result->SetError(DiskResult::ERR_CANNOT_SAVE);
		return p_result->GetValid();
	}

	if (file_format.IsEmpty()) {
		// ファイル形式の指定がない場合
		rc = SaveDiskByExt(disk_number, support);
	} else {
		// ファイル形式の指定あり
		rc = SelectSaveDisk(file_format, disk_number, support);
	}
	if (!support) {
		p_result->SetError(DiskResult::ERR_UNSUPPORTED);
		return p_result->GetValid();
	}
	return rc;
}

/// 拡張子で保存形式を判定＆保存できるか
/// @param [in] file_format ファイルフォーマット
/// @param [in] disk_number ディスク番号
int DiskWriter::SelectCanSaveDisk(const wxString &file_format, int disk_number)
{
	int rc = -1;
	if (file_format == wxT("hdi")) {
		// hdi形式
		DiskPlainWriter wr(this, p_result);
		rc = wr.ValidateDisk(p_image, disk_number);
	} else
	if (file_format == wxT("hdd")) {
		// hdd形式
		DiskPlainWriter wr(this, p_result);
		rc = wr.ValidateDisk(p_image, disk_number);
	} else
	if (file_format == wxT("nhd")) {
		// NHD形式 (T98 NEXT)
		DiskPlainWriter wr(this, p_result);
		rc = wr.ValidateDisk(p_image, disk_number);
	} else
	if (file_format == wxT("thd")) {
		// THD形式 (T98)
		DiskPlainWriter wr(this, p_result);
		rc = wr.ValidateDisk(p_image, disk_number);
	} else
	if (file_format == wxT("plain")) {
		// ベタ
		DiskPlainWriter wr(this, p_result);
		rc = wr.ValidateDisk(p_image, disk_number);
	}
	return rc;
}


/// 拡張子で保存形式を判定
/// @param [in] file_format ファイルフォーマット
/// @param [in] disk_number ディスク番号
/// @param [out] support    対応しているフォーマットならtrue
int DiskWriter::SelectSaveDisk(const wxString &file_format, int disk_number, bool &support)
{
	int rc = -1;
	if (file_format == wxT("hdi")) {
		// hdi形式
		DiskPlainWriter wr(this, p_result);
		rc = wr.SaveDisk(p_image, disk_number /*, m_ostream*/);
		support = true;
	} else
	if (file_format == wxT("hdd")) {
		// hdd形式
		DiskPlainWriter wr(this, p_result);
		rc = wr.SaveDisk(p_image, disk_number /*, m_ostream*/);
		support = true;
	} else
	if (file_format == wxT("nhd")) {
		// NHD形式 (T98 NEXT)
		DiskPlainWriter wr(this, p_result);
		rc = wr.SaveDisk(p_image, disk_number /*, m_ostream*/);
		support = true;
	} else
	if (file_format == wxT("thd")) {
		// THD形式 (T98)
		DiskPlainWriter wr(this, p_result);
		rc = wr.SaveDisk(p_image, disk_number /*, m_ostream*/);
		support = true;
	} else
	if (file_format == wxT("plain")) {
		// ベタ
		DiskPlainWriter wr(this, p_result);
		rc = wr.SaveDisk(p_image, disk_number /*, m_ostream*/);
		support = true;
	}
	if (support && rc >= 0) {
		// 保存したファイル名を持っておく
		p_image->SetFileName(m_file_path);
	}
	return rc;
}

//////////////////////////////////////////////////////////////////////
//
// 形式ごとの保存
//
DiskImageWriter::DiskImageWriter(DiskWriter *dw_, DiskResult *result_)
{
	p_dw = dw_;
	p_result = result_;
}

DiskImageWriter::~DiskImageWriter()
{
}

/// ストリームの内容をファイルに保存できるか
/// @param [in,out] image ディスクイメージ
/// @param [in]     disk_number ディスク番号(0-) / -1のときは全体
/// @retval  0 正常
int DiskImageWriter::ValidateDisk(DiskImage *image, int disk_number)
{
	p_result->Clear();

	return 0;
}

/// ストリームの内容をファイルに保存
/// @param [in,out] image ディスクイメージ
/// @param [in]     disk_number ディスク番号(0-) / -1のときは全体 
/// @retval  0 正常
int DiskImageWriter::SaveDisk(DiskImage *image, int disk_number)
{
	p_result->Clear();

	return 0;
}
