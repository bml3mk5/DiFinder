/// @file diskparser.cpp
///
/// @brief ディスクパーサー
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskparser.h"
#include "diskplainparser.h"
#include "diskhdiparser.h"
#include "diskhddparser.h"
#include "disknhdparser.h"
#include "diskthdparser.h"
#include "diskimage.h"
#include "fileparam.h"
#include "bootparam.h"
#include "diskresult.h"


/// コンストラクタ
/// @param [in]     filepath    解析するファイルのパス 
/// @param [in]     stream      上記ファイルのストリーム   
/// @param [in,out] file        既存のディスクイメージ
/// @param [out]    result      結果
DiskParser::DiskParser(const wxString &filepath, wxInputStream *stream, DiskImageFile *file, DiskResult &result)
{
	m_filepath = wxFileName(filepath);
	p_stream = stream;
	p_file = file;
	p_result = &result;
}

DiskParser::~DiskParser()
{
}

/// ディスクイメージを新たに解析する
/// @param [in] file_format      ファイルの形式名("d88","plain"など)
/// @param [in] param_hint       ディスクパラメータヒント("plain"時のみ)
/// @param [in] boot_param  ブートストラップ種類(nullable)
int DiskParser::Parse(const wxString &file_format, const DiskParam &param_hint, const BootParam *boot_param)
{
	return Parse(file_format, param_hint, DiskImageFile::MODIFY_NONE, boot_param);
}

/// 指定ディスクを解析してこれを既存のディスクイメージに追加する
/// @param [in] file_format      ファイルの形式名("d88","plain"など)
/// @param [in] param_hint       ディスクパラメータヒント("plain"時のみ)
/// @param [in] boot_param  ブートストラップ種類(nullable)
int DiskParser::ParseAdd(const wxString &file_format, const DiskParam &param_hint, const BootParam *boot_param)
{
	return Parse(file_format, param_hint, DiskImageFile::MODIFY_ADD, boot_param);
}

/// ディスクイメージの解析
/// @param [in] file_format ファイルの形式名("d88","plain"など)
/// @param [in] param_hint  ディスクパラメータヒント("plain"時のみ)
/// @param [in] mod_flags   オープン/追加 DiskImageFile::Add()
/// @param [in] boot_param  ブートストラップ種類(nullable)
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskParser::Parse(const wxString &file_format, const DiskParam &param_hint, short mod_flags, const BootParam *boot_param)
{
	bool support = false;
	int rc = -1;

	m_image_type.Empty();
	if (!file_format.IsEmpty()) {
		// ファイル形式の指定あり
		rc = SelectPerser(file_format, &param_hint, mod_flags, support, boot_param);
		if (rc >= 0) {
			m_image_type = file_format;
		}
	}
	if (!support) {
		p_result->SetError(DiskResult::ERR_UNSUPPORTED);
		return p_result->GetValid();
	}
	return rc;
}

/// ディスクイメージをチェック
/// @param [in,out] file_format  ファイルの形式名("d88","plain"など)
/// @param [out] disk_params     ディスクパラメータの候補
/// @param [out] manual_param    候補がないときのパラメータヒント
int DiskParser::Check(wxString &file_format, DiskParamPtrs &disk_params, DiskParam &manual_param)
{
	return Check(file_format, disk_params, manual_param, DiskImageFile::MODIFY_NONE);
}

/// ディスクイメージのチェック
/// @param [in,out] file_format  ファイルの形式名("d88","plain"など)
/// @param [out] disk_params     ディスクパラメータの候補
/// @param [out] manual_param    候補がないときのパラメータヒント
/// @param [in] mod_flags        オープン/追加 DiskImageFile::Add()
/// @retval  0 正常
/// @retval -1 エラーあり
int DiskParser::Check(wxString &file_format, DiskParamPtrs &disk_params, DiskParam &manual_param, short mod_flags)
{
	bool support = false;
	int rc = -1;

	if (file_format.IsEmpty()) {
		// ファイル形式の指定がない場合

		// 拡張子で判定
		wxString ext = m_filepath.GetExt();

		// サポートしているファイルか
		FileParam *fitem = gFileTypes.FindExt(ext);
		if (!fitem) {
			p_result->SetError(DiskResult::ERR_UNSUPPORTED);
			return p_result->GetValid();
		}

		// 指定形式で解析する
		const FileParamFormats *formats = &fitem->GetFormats();
		for(size_t i=0; i<formats->Count(); i++) {
			const FileParamFormat *param_format = &formats->Item(i);
			rc = SelectChecker(param_format->GetType(), &param_format->GetHints(), NULL, disk_params, manual_param, mod_flags, support);
			if (rc >= 0) {
				file_format = param_format->GetType();
				break;
			}
		}

	} else {
		// ファイル形式の指定あり

		rc = SelectChecker(file_format, NULL, NULL, disk_params, manual_param, mod_flags, support);

	}
	if (!support) {
		p_result->SetError(DiskResult::ERR_UNSUPPORTED);
		return p_result->GetValid();
	}
	return rc;
}

/// ファイルの解析方法を選択
/// @param [in] type             ファイルの形式名("d88","plain"など)
/// @param [in] disk_param       ディスクパラメータ("plain"時のみ)
/// @param [in] mod_flags        オープン/追加 DiskImageFile::Add()
/// @param [out] support         サポートしているファイルか
/// @param [in] boot_param       ブートストラップ種類(nullable)
/// @retval  1 警告
/// @retval  0 正常
/// @retval -1 エラー
int DiskParser::SelectPerser(const wxString &type, const DiskParam *disk_param, short mod_flags, bool &support, const BootParam *boot_param)
{
	int rc = -1;
	if (type == wxT("hdi")) {
		// HDI形式 (Anex86)
		DiskHDIParser ps(p_file, mod_flags, p_result);
		rc = ps.Parse(*p_stream, disk_param, boot_param);
		support = true;
	} else
	if (type == wxT("hdd")) {
		// HDD形式 (Virtual98)
		DiskHDDParser ps(p_file, mod_flags, p_result);
		rc = ps.Parse(*p_stream, disk_param, boot_param);
		support = true;
	} else
	if (type == wxT("nhd")) {
		// NHD形式 (T98 NEXT)
		DiskNHDParser ps(p_file, mod_flags, p_result);
		rc = ps.Parse(*p_stream, disk_param, boot_param);
		support = true;
	} else
	if (type == wxT("thd")) {
		// THD形式 (T98)
		DiskTHDParser ps(p_file, mod_flags, p_result);
		rc = ps.Parse(*p_stream, disk_param, boot_param);
		support = true;
	} else
	if (type == wxT("plain")) {
		// ベタ
		DiskPlainParser ps(p_file, mod_flags, p_result);
		rc = ps.Parse(*p_stream, disk_param, boot_param);
		support = true;
	}
	return rc;
}

/// ファイルのチェック方法を選択
/// @param [in] type             ファイルの形式名("d88","plain"など)
/// @param [in] disk_hints       ディスクパラメータヒント("plain"時のみ)
/// @param [in] disk_param       ディスクパラメータ("plain"時のみ)
/// @param [out] disk_params     ディスクパラメータの候補
/// @param [out] manual_param    候補がないときのパラメータヒント
/// @param [in] mod_flags        オープン/追加 DiskImageFile::Add()
/// @param [out] support         サポートしているファイルか
/// @retval  1 候補がないので改めてディスク種類を選択してもらう
/// @retval  0 候補あり正常
/// @retval -1 エラー終了
int DiskParser::SelectChecker(const wxString &type, const DiskTypeHints *disk_hints, const DiskParam *disk_param, DiskParamPtrs &disk_params, DiskParam &manual_param, short mod_flags, bool &support)
{
	int rc = -1;
	if (type == wxT("hdi")) {
		// HDI形式 (Anex86)
		DiskHDIParser ps(p_file, mod_flags, p_result);
		rc = ps.Check(*p_stream, disk_hints, disk_param, disk_params, manual_param);
		support = true;
	} else
	if (type == wxT("hdd")) {
		// HDD形式 (Virtual98)
		DiskHDDParser ps(p_file, mod_flags, p_result);
		rc = ps.Check(*p_stream, disk_hints, disk_param, disk_params, manual_param);
		support = true;
	} else
	if (type == wxT("nhd")) {
		// NHD形式 (T98 NEXT)
		DiskNHDParser ps(p_file, mod_flags, p_result);
		rc = ps.Check(*p_stream, disk_hints, disk_param, disk_params, manual_param);
		support = true;
	} else
	if (type == wxT("thd")) {
		// THD形式 (T98)
		DiskTHDParser ps(p_file, mod_flags, p_result);
		rc = ps.Check(*p_stream, disk_hints, disk_param, disk_params, manual_param);
		support = true;
	} else
	if (type == wxT("plain")) {
		// ベタ
		DiskPlainParser ps(p_file, mod_flags, p_result);
		rc = ps.Check(*p_stream, disk_hints, disk_param, disk_params, manual_param);
		support = true;
	}
	return rc;
}

// ----------------------------------------------------------------------
//
//
//
DiskImageParser::DiskImageParser(DiskImageFile *file, short mod_flags, DiskResult *result)
{
	p_file = file;
	m_mod_flags = mod_flags;
	p_result = result;
}

DiskImageParser::~DiskImageParser()
{
}

/// ファイルイメージを解析
/// @param [in] istream    解析対象データ
/// @param [in] disk_param ディスクパラメータ
/// @param [in] boot_param ブートストラップ種類(nullable)
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskImageParser::Parse(wxInputStream &istream, const DiskParam *disk_param, const BootParam *boot_param)
{
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
int DiskImageParser::Check(wxInputStream &istream, const DiskTypeHints *disk_hints, const DiskParam *disk_param, DiskParamPtrs &disk_params, DiskParam &manual_param)
{
	return p_result->GetValid();
}
