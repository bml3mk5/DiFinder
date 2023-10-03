/// @file bootparam.cpp
///
/// @brief ディスクパラメータ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "bootparam.h"
#include <wx/xml/xml.h>
#include <wx/translation.h>
#include "../logging.h"
#include "../utils.h"


BootTemplates gBootTemplates;

//////////////////////////////////////////////////////////////////////
//
// DISK BASIC 名前リストを保存
//
BasicParamName::BasicParamName()
	: DiskParamName()
{
}
BasicParamName::BasicParamName(const wxString &name, int flags)
	: DiskParamName(name, flags)
{
}

//////////////////////////////////////////////////////////////////////

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(BasicParamNames);

//////////////////////////////////////////////////////////////////////
//
// 比較用キーワードを保存 
//
BootKeyword::BootKeyword()
{
	m_pos = 0;
}
BootKeyword::BootKeyword(int pos, const wxString &keyword)
{
	m_pos = pos;
	m_key = keyword;
}

//////////////////////////////////////////////////////////////////////

WX_DEFINE_OBJARRAY(BootKeywords);

//////////////////////////////////////////////////////////////////////
//
// ディスクパラメータ
//
BootParam::BootParam()
{
	this->ClearBootParam();
}
/// @param[in] src 元
BootParam::BootParam(const BootParam &src)
{
	this->SetBootParam(src);
}
/// 代入
/// @param[in] src 元
BootParam &BootParam::operator=(const BootParam &src)
{
	this->SetBootParam(src);
	return *this;
}
/// 全パラメータを設定
/// @param[in] n_boot_type      種類
/// @param[in] n_boot_type_name 種類名
/// @param[in] n_category_name  カテゴリ名
/// @param[in] n_keywords;		比較用キーワード
/// @param[in] n_basic_types;	BASIC種類（DiskBasicParamとのマッチングにも使用）
/// @param[in] n_description;	説明
void BootParam::SetBootParam(int n_boot_type
	, const wxString &n_boot_type_name
	, const wxString &n_category_name
	, const BootKeywords &n_keywords
	, const BasicParamNames &n_basic_types
	, const wxString &n_description
) {
	boot_type = n_boot_type;
	boot_type_name = n_boot_type_name;
	category_name = n_category_name;
	keywords = n_keywords;
	basic_types = n_basic_types;
	description = n_description;
}
/// 設定
/// @param[in] src 元
void BootParam::SetBootParam(const BootParam &src)
{
	boot_type = src.boot_type;
	boot_type_name = src.boot_type_name;
	category_name = src.category_name;
	keywords = src.keywords;
	basic_types = src.basic_types;
	description = src.description;
}
/// 初期化
void BootParam::ClearBootParam()
{
	boot_type = 0;
	boot_type_name.Empty();
	category_name.Empty();
	keywords.Empty();
	basic_types.Empty();
	description.Empty();
}
/// DISK BASICをさがす
/// @param[in] type_name タイプ名
/// @param[in] flags     フラグ
/// @return 名前
const BasicParamName *BootParam::FindBasicType(const wxString &type_name, int flags) const
{
	const BasicParamName *match = NULL;
	for(size_t i=0; i<basic_types.Count(); i++) {
		const BasicParamName *item = &basic_types.Item(i);
		if (item->GetName() == type_name && (flags < 0 || item->GetFlags() == flags)) {
			match = item;
			break;
		}
	}
	return match;
}

//////////////////////////////////////////////////////////////////////

WX_DEFINE_OBJARRAY(BootParams);

//////////////////////////////////////////////////////////////////////
//
// ブートストラップのカテゴリ(メーカ毎にまとめる)クラス
//
BootCategory::BootCategory()
	: DiskCategory()
{
}
BootCategory::BootCategory(const wxString &n_type_name, const wxString &n_description)
	: DiskCategory(n_type_name, n_description)
{
}

//////////////////////////////////////////////////////////////////////

WX_DEFINE_OBJARRAY(BootCategories);

//////////////////////////////////////////////////////////////////////
//
// ディスクパラメータのテンプレートを提供する
//
BootTemplates::BootTemplates() : TemplatesBase()
{
}
/// XMLファイルから読み込み
/// @param[in] data_path   入力ファイルのあるパス
/// @param[in] locale_name ローケル名
/// @param[out] errmsgs    エラーメッセージ
/// @return true / false
bool BootTemplates::Load(const wxString &data_path, const wxString &locale_name, wxString &errmsgs)
{
	wxXmlDocument doc;

	if (!doc.Load(data_path + wxT("boot_types.xml"))) return false;

	// start processing the XML file
	if (doc.GetRoot()->GetName() != "Boots") return false;

	bool valid = true;
	wxXmlNode *item;
	item = doc.GetRoot()->GetChildren();
	while (item && valid) {
		if (item->GetName() == "BootTypes") {
			valid = LoadBootTypes(item, locale_name, errmsgs);
		} else if (item->GetName() == "BootCategories") {
			valid = LoadBootCategories(item, locale_name, errmsgs);
		}
		item = item->GetNext();
	}
	return valid;
}

/// BootTypesエレメントをロード
/// @param[in]  node        子ノード
/// @param[in] locale_name  ローケル名
/// @param[out] errmsgs     エラーメッセージ
/// @return true
bool BootTemplates::LoadBootTypes(const wxXmlNode *node, const wxString &locale_name, wxString &errmsgs)
{
	wxString desc, desc_locale;

	wxXmlNode *itemnode = node->GetChildren();
	while(itemnode) {
		wxString str = itemnode->GetNodeContent();
		if (itemnode->GetName() == "BootType") {
			BootParam p;

			wxString type_name = itemnode->GetAttribute("name");
			p.SetBootTypeName(type_name);
			int type_num = Utils::ToInt(itemnode->GetAttribute("type"));
			p.SetBootType(type_num);
			wxString category_name = itemnode->GetAttribute("category");
			p.SetCategoryName(category_name);

			wxXmlNode *citemnode = itemnode->GetChildren();
			while(citemnode) {
				if (citemnode->GetName() == "CompareKeywords") {
					BootKeywords keywords;
					if (!LoadKeywords(citemnode, keywords, errmsgs)) {
						return false;
					}
					p.SetKeywords(keywords);
				} else if (citemnode->GetName() == "DiskBasicTypes") {
					BasicParamNames basic_types;
					if (!LoadDiskBasicTypes(citemnode, basic_types, errmsgs)) {
						return false;
					}
					p.SetBasicTypes(basic_types);
				} else if (citemnode->GetName() == "Description") {
					LoadDescription(citemnode, locale_name, desc, desc_locale);
					p.SetDescription(desc);
				}
				citemnode = citemnode->GetNext();
			}
			if (!desc_locale.IsEmpty()) {
				desc = desc_locale;
			}
			if (FindType(type_name) == NULL) {
				params.Add(p);
			} else {
				errmsgs += wxT("\n");
				errmsgs += _("Duplicate type name in BootType : ");
				errmsgs += type_name;
				return false;
			}
		}
		itemnode = itemnode->GetNext();
	}
	return true;
}

/// BootCategoriesエレメントをロード
/// @param[in]  node        子ノード
/// @param[in] locale_name  ローケル名
/// @param[out] errmsgs     エラーメッセージ
/// @return true
bool BootTemplates::LoadBootCategories(const wxXmlNode *node, const wxString &locale_name, wxString &errmsgs)
{
	bool valid = true;
	wxXmlNode *item = node->GetChildren();
	while(item && valid) {
		if (item->GetName() == "BootCategory") {
			wxString type_name = item->GetAttribute("name");
			wxString desc, desc_locale;

			wxXmlNode *itemnode = item->GetChildren();
			while (itemnode) {
				if (itemnode->GetName() == "Description") {
					LoadDescription(itemnode, locale_name, desc, desc_locale);
				}
				itemnode = itemnode->GetNext();
			}
			if (!desc_locale.IsEmpty()) {
				desc = desc_locale;
			}
			BootCategory c(
				type_name,
				desc
			);
			if (FindCategory(type_name) == NULL) {
				categories.Add(c);
			} else {
				errmsgs += wxT("\n");
				errmsgs += _("Duplicate type name in BootCategory : ");
				errmsgs += type_name;
				valid = false;
				break;
			}
		}
		item = item->GetNext();
	}
	return valid;
}

/// CompareKeywordsエレメントをロード
/// @param[in]  node        子ノード
/// @param[out] keywords    ロードしたデータ
/// @param[out] errmsgs     エラーメッセージ
/// @return true
bool BootTemplates::LoadKeywords(const wxXmlNode *node, BootKeywords &keywords, wxString &errmsgs)
{
	wxXmlNode *citemnode = node->GetChildren();
	while(citemnode) {
		wxString str = citemnode->GetNodeContent();
		if (citemnode->GetName() == "String") {
			wxString spos;
			int pos = -1;
			if (citemnode->GetAttribute("pos", &spos)) {
				pos = Utils::ToInt(spos);
			}
			str = str.Trim(false).Trim(true);
			wxString key;
			Utils::DecodeEscape(str, key);
			BootKeyword p(pos, key);
			if (pos >= 0 && !str.IsEmpty()) {
				keywords.Add(p);
			}
		}
		citemnode = citemnode->GetNext();
	}
	return true;
}

/// DiskBasicTypesエレメントをロード
/// @param[in]  node        子ノード
/// @param[out] basic_types ロードしたデータ
/// @param[out] errmsgs     エラーメッセージ
/// @return true
bool BootTemplates::LoadDiskBasicTypes(const wxXmlNode *node, BasicParamNames &basic_types, wxString &errmsgs)
{
	wxXmlNode *citemnode = node->GetChildren();
	while(citemnode) {
		wxString str = citemnode->GetNodeContent();
		if (citemnode->GetName() == "Type") {
			BasicParamName p;
			str = str.Trim(false).Trim(true);
			if (!str.IsEmpty()) {
				p.SetName(str);
			}
			str = citemnode->GetAttribute("p");
			if (str.Lower() == "major") {
				p.SetFlags(1);
			} else if (str.Lower() == "minor") {
				p.SetFlags(2);
			}
			basic_types.Add(p);
		}
		citemnode = citemnode->GetNext();
	}
	return true;
}

/// タイプ名に一致するテンプレートの番号を返す
/// @param[in] n_type_name タイプ名
/// @return ディスクテンプレートの位置 / ないとき-1
int BootTemplates::IndexOf(const wxString &n_type_name) const
{
	int match = -1;
	for(size_t i=0; i<params.Count(); i++) {
		BootParam *item = &params[i];
		if (n_type_name == item->GetBootTypeName()) {
			match = (int)i;
			break;
		}
	}
	return match;
}

/// タイプ名に一致するテンプレートを返す
/// @param[in] n_type_name タイプ名
/// @return ディスクパラメータ or NULL
const BootParam *BootTemplates::FindType(const wxString &n_type_name) const
{
	BootParam *match = NULL;
	for(size_t i=0; i<params.Count(); i++) {
		BootParam *item = &params[i];
		if (n_type_name == item->GetBootTypeName()) {
			match = item;
			break;
		}
	}
	return match;
}
/// カテゴリ番号に一致するタイプ名リストを検索
/// @param [in]  n_category_index : カテゴリ番号
/// @param [out] n_type_names     : タイプ名リスト
/// @return リストの数
size_t BootTemplates::FindTypeNames(size_t n_category_index, wxArrayString &n_type_names) const
{
	return FindTypeNames(categories.Item(n_category_index).GetTypeName(), n_type_names);
}
/// カテゴリ名に一致するタイプ名リストを検索
/// @param [in]  n_category_name  : カテゴリ名
/// @param [out] n_type_names     : タイプ名リスト
/// @return リストの数
size_t BootTemplates::FindTypeNames(const wxString &n_category_name, wxArrayString &n_type_names) const
{
	n_type_names.Clear();
	for(size_t n=0; n<params.Count(); n++) {
		const BootParam *item = &params[n];
		if (item->GetCategoryName() == n_category_name) {
			n_type_names.Add(item->GetBootTypeName());
		}
	}
	return n_type_names.Count();
}

/// @brief タイプ名に一致するカテゴリを返す
/// @param[in] n_type_name タイプ名
/// @return ディスクパラメータ or NULL
const BootCategory *BootTemplates::FindCategory(const wxString &n_type_name) const
{
	BootCategory *match = NULL;
	for(size_t i=0; i<categories.Count(); i++) {
		BootCategory *item = &categories[i];
		if (n_type_name == item->GetTypeName()) {
			match = item;
			break;
		}
	}
	return match;
}
