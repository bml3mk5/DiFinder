/// @file fileparam.cpp
///
/// @brief ファイルパラメータ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "fileparam.h"
#include <wx/intl.h>
#include <wx/xml/xml.h>
#include "../utils.h"


FileTypes gFileTypes;


//////////////////////////////////////////////////////////////////////
//
// ファイル形式種類
//
FileFormat::FileFormat()
{
}
FileFormat::FileFormat(const wxString &name, bool writable, const wxString &desc)
{
	m_name = name;
	m_writable = writable;
	m_description = desc;
}

//////////////////////////////////////////////////////////////////////

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(FileFormats);

//////////////////////////////////////////////////////////////////////
//
// ディスク解析で用いるヒント
//
DiskTypeHint::DiskTypeHint()
{
	m_kind = 0;
}
DiskTypeHint::DiskTypeHint(const wxString &hint)
{
	m_hint = hint;
	m_kind = 0;
}
DiskTypeHint::DiskTypeHint(const wxString &hint, int kind)
{
	m_hint = hint;
	m_kind = kind;
}
/// セット
void DiskTypeHint::Set(const wxString &hint, int kind)
{
	m_hint = hint;
	m_kind = kind;
}

//////////////////////////////////////////////////////////////////////

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(DiskTypeHints);

//////////////////////////////////////////////////////////////////////
//
// ファイル形式パラメータ
//
FileParamFormat::FileParamFormat()
{
}
/// @param[in] type ファイル種類("d88","plain",...)
FileParamFormat::FileParamFormat(const wxString &type)
{
	m_type = type;
}
/// ヒントを追加
void FileParamFormat::AddHint(const wxString &val, int kind)
{
	m_hints.Add(DiskTypeHint(val, kind));
}

//////////////////////////////////////////////////////////////////////

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(FileParamFormats);

//////////////////////////////////////////////////////////////////////
//
// ファイルパラメータ
//
FileParam::FileParam()
{
	this->ClearFileParam();
}
FileParam::FileParam(const FileParam &src)
{
	this->SetFileParam(src);
}
/// @param[in] n_ext     拡張子
/// @param[in] n_formats フォーマットリスト
/// @param[in] n_desc    説明
FileParam::FileParam(const wxString &n_ext, const FileParamFormats &n_formats, const wxString &n_desc)
{
	this->SetFileParam(n_ext, n_formats, n_desc);
}
/// 代入
FileParam &FileParam::operator=(const FileParam &src)
{
	this->SetFileParam(src);
	return *this;
}
/// 設定
/// @param[in] src       元
void FileParam::SetFileParam(const FileParam &src)
{
	m_extension = src.m_extension;
	m_formats = src.m_formats;
	m_description = src.m_description;
}
/// 設定
/// @param[in] n_ext     拡張子
/// @param[in] n_formats フォーマットリスト
/// @param[in] n_desc    説明
void FileParam::SetFileParam(const wxString &n_ext, const FileParamFormats &n_formats, const wxString &n_desc)
{
	m_extension = n_ext;
	m_formats = n_formats;
	m_description = n_desc;

	// 拡張子は小文字にしておく
	m_extension = m_extension.Lower();
}
/// 初期化
void FileParam::ClearFileParam()
{
	m_extension.Empty();
	m_formats.Empty();
	m_description.Empty();
}

//////////////////////////////////////////////////////////////////////

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(FileParams);

//////////////////////////////////////////////////////////////////////

ExtensionItem::ExtensionItem()
{
	p_desc = NULL;
}

ExtensionItem::ExtensionItem(const wxString &ext, const wxString &desc)
{
	m_ext = ext;
	p_desc = &desc;
}

//////////////////////////////////////////////////////////////////////

WX_DEFINE_OBJARRAY(ExtensionItems);

//////////////////////////////////////////////////////////////////////
//
// ファイル種類
//
FileTypes::FileTypes() : TemplatesBase()
{
}

/// XMLファイルをロード
/// @param[in] data_path   : ファイルパス
/// @param[in] locale_name : ローケル(jaなど)
/// @param[out] errmsgs     エラーメッセージ
/// @return false:エラー
bool FileTypes::Load(const wxString &data_path, const wxString &locale_name, wxString &errmsgs)
{
	wxXmlDocument doc;

	if (!doc.Load(data_path + wxT("file_types.xml"))) return false;

	// start processing the XML file
	if (doc.GetRoot()->GetName() != "FileTypes") return false;

	wxXmlNode *item = doc.GetRoot()->GetChildren();
	while (item) {
		if (item->GetName() == "FileFormatType") {
			wxString name = item->GetAttribute("name");
			wxXmlNode *itemnode = item->GetChildren();
			bool writable = true;
			wxString desc, desc_locale;
			while (itemnode) {
				if (itemnode->GetName() == "Writable") {
					writable = Utils::ToBool(itemnode->GetNodeContent());
				} else if (itemnode->GetName() == "Description") {
					LoadDescription(itemnode, locale_name, desc, desc_locale);
				}
				itemnode = itemnode->GetNext();
			}
			if (!desc_locale.IsEmpty()) {
				desc = desc_locale;
			}
			FileFormat p(name, writable, desc);
			formats.Add(p);
		} else if (item->GetName() == "FileType") {
			wxString ext  = item->GetAttribute("ext");
			wxXmlNode *itemnode = item->GetChildren();
			FileParamFormats fmts;
			wxString desc, desc_locale, str;
			while (itemnode) {
				if (itemnode->GetName() == "Format") {
					wxString type = itemnode->GetAttribute("type");
					FileParamFormat fmt(type);
					wxXmlNode *citemnode = itemnode->GetChildren();
					while(citemnode) {
						if (citemnode->GetName() == "DiskTypeHint") {
							str = citemnode->GetNodeContent();
							str = str.Trim(false).Trim(true);
							if (!str.IsEmpty()) {
								wxString skind = citemnode->GetAttribute("kind");
								long kind = 0;
								if (!skind.IsEmpty()) {
									skind.ToLong(&kind);
								}
								fmt.AddHint(str, (int)kind);
							}
						}
						citemnode = citemnode->GetNext();
					}
					fmts.Add(fmt);
				} else if (itemnode->GetName() == "Description") {
					LoadDescription(itemnode, locale_name, desc, desc_locale);
				}
				itemnode = itemnode->GetNext();
			}
			if (!desc_locale.IsEmpty()) {
				desc = desc_locale;
			}
			FileParam p(ext, fmts, desc);
			types.Add(p);
		}
		item = item->GetNext();
	}

	MakeWildcard();

	return true;
}

/// ファイルダイアログ用の拡張子選択リストを作成する
void FileTypes::MakeWildcard()
{
	// for load
	wcard_for_load = _("Supported files");
	wcard_for_load += wxT("|");
	for(size_t i=0; i<types.Count(); i++) {
		const FileParam *param = &types.Item(i);
		wxString ext = wxT("*.");
		ext += param->GetExt().Lower();
		if (i>0) wcard_for_load += wxT(";");
		wcard_for_load += ext;
#if !defined(__WXMSW__)
		ext = ext.Upper();
		wcard_for_load += wxT(";");
		wcard_for_load += ext;
#endif
	}

#if defined(__WXMSW__)
	for(size_t i=0; i<types.Count(); i++) {
		const FileParam *param = &types.Item(i);
		wcard_for_load += wxT("|");
		wcard_for_load += param->GetDescription();
		wxString ext = wxT("*.");
		ext += param->GetExt().Lower();
		wcard_for_load += wxT("|");
		wcard_for_load += ext;
	}
#endif
	wcard_for_load += wxT("|");
	wcard_for_load += _("All files");
	wcard_for_load += wxT("|*.*");


	// for save
	wcard_for_save = wxT("");
}

/// 拡張子をさがす
/// @param[in] n_ext 拡張子(".d88"など)
/// @return FileFormat
FileParam *FileTypes::FindExt(const wxString &n_ext)
{
	FileParam *match = NULL;
	wxString ext = n_ext.Lower();
	for(size_t i=0; i<types.Count(); i++) {
		FileParam *item = &types[i];
		if (ext == item->GetExt()) {
			match = item;
			break;
		}
	}
	return match;
}

/// ディスクイメージフォーマット形式をさがす
/// @param[in] n_name 名前("d88"など)
/// @return FileFormat
FileFormat *FileTypes::FindFormat(const wxString &n_name)
{
	FileFormat *match = NULL;
	for(size_t i=0; i<formats.Count(); i++) {
		FileFormat *item = &formats[i];
		if (n_name == item->GetName()) {
			match = item;
			break;
		}
	}
	return match;
}

/// ファイル保存時の保存形式のフォーマットを返す
/// @param[in] index ダイアログ内にある拡張子リストの位置
/// @return FileFormat
FileFormat *FileTypes::GetFilterForSave(int index)
{
	FileFormat *match = NULL;
	if (index < 0 || index >= (int)exts_for_save.Count()) {
		return match;
	}
	return match;
}
