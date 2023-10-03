/// @file parambase.cpp
///
/// @brief parameter templates
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "parambase.h"
#include "utils.h"
#include <wx/xml/xml.h>


//////////////////////////////////////////////////////////////////////
//
// 特別な属性などを保持する
//
MyAttribute::MyAttribute()
{
	idx = 0;
	type = 0;
	value = 0;
	mask = -1;
}
MyAttribute::MyAttribute(int n_idx, int n_type, int n_value, int n_mask, const wxString &n_name, const wxString &n_desc)
{
	idx = n_idx;
	type = n_type;
	value = n_value;
	mask = n_mask;
	name = n_name;
	desc = n_desc;

	if (name.IsEmpty()) name = wxT("???");
}

//////////////////////////////////////////////////////////////////////

#include <wx/arrimpl.cpp>
WX_DEFINE_OBJARRAY(ArrayOfMyAttribute);

//////////////////////////////////////////////////////////////////////
//
// 特別な属性のリスト MyAttribute の配列
//
MyAttributes::MyAttributes()
	: ArrayOfMyAttribute()
{
}
/// 属性タイプと値に一致するアイテムを返す
const MyAttribute *MyAttributes::Find(int type, int value) const
{
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if (attr->GetType() == type && attr->GetValue() == (value & attr->GetMask())) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性タイプと値に一致するアイテムを返す
const MyAttribute *MyAttributes::Find(int type, int mask, int value) const
{
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if ((attr->GetType() & mask) == (type & mask) && attr->GetValue() == (value & attr->GetMask())) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性タイプと値に一致するアイテムを返す
const MyAttribute *MyAttributes::FindType(int type, int mask) const
{
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if ((attr->GetType() & mask) == (type & mask)) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性値に一致するアイテムを返す
const MyAttribute *MyAttributes::FindValue(int value) const
{
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if (attr->GetValue() == (value & attr->GetMask())) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性名に一致するアイテムを返す
const MyAttribute *MyAttributes::Find(int type, const wxString &name) const
{
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if (attr->GetType() == type && attr->GetName() == name) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性名に一致するアイテムを返す
const MyAttribute *MyAttributes::Find(const wxString &name) const
{
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if (attr->GetName() == name) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性名に一致するアイテムを返す 大文字でマッチング
const MyAttribute *MyAttributes::FindUpperCase(const wxString &name) const
{
	wxString iname = name.Upper();
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if (attr->GetName().Upper() == iname) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性名と属性タイプに一致するアイテムを返す 大文字でマッチング
const MyAttribute *MyAttributes::FindUpperCase(const wxString &name, int type, int mask) const
{
	wxString iname = name.Upper();
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if (attr->GetName().Upper() == iname && (attr->GetType() & mask) == (type & mask)) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性名、属性タイプ、属性値に一致するアイテムを返す 大文字でマッチング
const MyAttribute *MyAttributes::FindUpperCase(const wxString &name, int type, int mask, int value) const
{
	wxString iname = name.Upper();
	const MyAttribute *match = NULL;
	for(size_t i=0; i<Count(); i++) {
		MyAttribute *attr = &Item(i);
		if (attr->GetName().Upper() == iname && (attr->GetType() & mask) == (type & mask) && attr->GetValue() == value) {
			match = attr;
			break;
		}
	}
	return match;
}
/// 属性値に一致するアイテムの位置を返す
int MyAttributes::GetIndexByValue(int value) const
{
	int idx = -1;
	const MyAttribute *match = FindValue(value);
	if (match) {
		idx = match->GetIndex();
	}
	return idx;
}
/// 属性値に一致するアイテムの属性タイプを返す
int MyAttributes::GetTypeByValue(int value) const
{
	int type = 0;
	const MyAttribute *match = FindValue(value);
	if (match) {
		type = match->GetType();
	}
	return type;
}
/// 位置から属性タイプを返す
int MyAttributes::GetTypeByIndex(int idx) const
{
	return idx < (int)Count() ? Item(idx).GetType() : 0;
}
/// 位置から属性値を返す
int MyAttributes::GetValueByIndex(int idx) const
{
	return idx < (int)Count() ? Item(idx).GetValue() : 0;
}

//////////////////////////////////////////////////////////////////////
//
// パラメータのテンプレートを提供する
//
TemplatesBase::TemplatesBase()
{
}

/// XMLファイル読み込み
/// @param[in]  data_path   XMLファイルがあるフォルダ
/// @param[in]  locale_name ローケル名
/// @param[out] errmsgs     エラーメッセージ
/// @return true/false
bool TemplatesBase::Load(const wxString &data_path, const wxString &locale_name, wxString &errmsgs)
{
	return false;
}

/// Descriptionエレメントをロード
/// @param[in]  node        子ノード
/// @param[in]  locale_name ローケル名
/// @param[out] desc        説明
/// @param[out] desc_locale 説明ローケル
/// @return true
bool TemplatesBase::LoadDescription(const wxXmlNode *node, const wxString &locale_name, wxString &desc, wxString &desc_locale)
{
	if (node->HasAttribute("lang")) {
		wxString lang = node->GetAttribute("lang");
		if (locale_name.Find(lang) != wxNOT_FOUND) {
			desc_locale = node->GetNodeContent();
		}
	} else {
		desc = node->GetNodeContent();
	}
	return true;
}

