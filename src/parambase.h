/// @file parambase.h
///
/// @brief parameter template
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef PARAMBASE_H
#define PARAMBASE_H

#include "common.h"
#include <wx/string.h>
#include <wx/dynarray.h>
#include <wx/variant.h>
#include <wx/hashmap.h>


//////////////////////////////////////////////////////////////////////

/// @brief 特別な属性などを保持する
///
/// @sa MyAttributes
class MyAttribute
{
private:
	int idx;		///< インデックス
	int type;		///< 属性タイプ
	int value;		///< 属性値
	int mask;		///< マスク
	wxString name;	///< 名前
	wxString desc;	///< 説明

public:
	MyAttribute();
	MyAttribute(int n_idx, int n_type, int n_value, int n_mask, const wxString &n_name, const wxString &n_desc);
	/// @brief インデックス
	int GetIndex() const { return idx; }
	/// @brief 属性タイプ
	int GetType() const { return type; }
	/// @brief 属性値
	int GetValue() const { return value; }
	/// @brief マスク
	int GetMask() const { return mask; }
	/// @brief 名前
	const wxString &GetName() const { return name; }
	/// @brief 説明
	const wxString &GetDescription() const { return desc; }
};

//////////////////////////////////////////////////////////////////////

WX_DECLARE_OBJARRAY(MyAttribute, ArrayOfMyAttribute);

//////////////////////////////////////////////////////////////////////

/// @brief 特別な属性のリスト MyAttribute の配列
class MyAttributes : public ArrayOfMyAttribute
{
public:
	MyAttributes();
	/// @brief 属性タイプと値に一致するアイテムを返す
	const MyAttribute *Find(int type, int value) const;
	/// @brief 属性タイプと値に一致するアイテムを返す
	const MyAttribute *Find(int type, int mask, int value) const;
	/// @brief 属性タイプに一致するアイテムを返す
	const MyAttribute *FindType(int type, int mask) const;
	/// @brief 属性値に一致するアイテムを返す
	const MyAttribute *FindValue(int value) const;
	/// @brief 属性名に一致するアイテムを返す
	const MyAttribute *Find(int type, const wxString &name) const;
	/// @brief 属性名に一致するアイテムを返す
	const MyAttribute *Find(const wxString &name) const;
	/// @brief 属性名に一致するアイテムを返す 大文字でマッチング
	const MyAttribute *FindUpperCase(const wxString &name) const;
	/// @brief 属性名と属性タイプに一致するアイテムを返す 大文字でマッチング
	const MyAttribute *FindUpperCase(const wxString &name, int type, int mask) const;
	/// @brief 属性名、属性タイプ、属性値に一致するアイテムを返す 大文字でマッチング
	const MyAttribute *FindUpperCase(const wxString &name, int type, int mask, int value) const;
	/// @brief 属性値に一致するアイテムの位置を返す
	int					GetIndexByValue(int value) const;
	/// @brief 属性値に一致するアイテムの属性値を返す
	int					GetTypeByValue(int value) const;
	/// @brief 位置から属性タイプを返す
	int					GetTypeByIndex(int idx) const;
	/// @brief 位置から属性値を返す
	int					GetValueByIndex(int idx) const;
};

//////////////////////////////////////////////////////////////////////

class wxXmlNode;

/// @brief パラメータのテンプレートを提供する
class TemplatesBase
{
protected:
	/// @brief Descriptionエレメントをロード
	bool LoadDescription(const wxXmlNode *node, const wxString &locale_name, wxString &desc, wxString &desc_locale);

public:
	TemplatesBase();
	virtual ~TemplatesBase() {}

	/// @brief XMLファイル読み込み
	virtual bool Load(const wxString &data_path, const wxString &locale_name, wxString &errmsgs);
};

#endif /* PARAMBASE_H */
