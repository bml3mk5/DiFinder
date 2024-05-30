/// @file bootparam.h
///
/// @brief ブートストラップ パラメータ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef BOOT_PARAMETER_H
#define BOOT_PARAMETER_H

#include "../common.h"
#include "../parambase.h"
#include <wx/string.h>
#include <wx/arrstr.h>
#include <wx/dynarray.h>
#include <wx/variant.h>
#include "diskcommon.h"

class wxXmlNode;

enum enBootTypes {
	BT_PC98_IPL				 = 1,
	BT_OS9_X68K_24_IPL		 = 2,
	BT_HU68K_IPL			 = 3,
	BT_OS9_X68K_24_SCSI_IPL	 = 4,
	BT_HU68K_SCSI_IPL		 = 5,
	BT_FMR_IPL				 = 6,
	BT_MAC_HFS				 = 7,
	BT_SUPER_FD				 = 11,
	BT_PCAT_MBR				 = 12,
};

//////////////////////////////////////////////////////////////////////

/// @brief BASIC 名前リストを保存 
class BasicParamName : public DiskParamName
{
public:
	BasicParamName();
	BasicParamName(const wxString &name, int flags);
};

//////////////////////////////////////////////////////////////////////

/// @class BasicParamNames
///
/// @brief BasicParamName のリスト
WX_DECLARE_OBJARRAY(BasicParamName, BasicParamNames);

//////////////////////////////////////////////////////////////////////

/// @brief 比較用キーワードを保存 
class BootKeyword
{
public:
	enum enKeywordTypes {
		KNull = 0,
		KString,
		KRegex,
		KArrayString
	};

private:
	int		 m_start_pos;
	int		 m_last_pos;
	double   m_weight;
	wxVariant m_key;
	enKeywordTypes m_type;
public:
	BootKeyword();
	~BootKeyword() {}

	void Set(int start_pos, int last_pos, double weight);

	int GetStartPos() const { return m_start_pos; }
	int GetLastPos() const { return m_last_pos; }
	double GetWeight() const { return m_weight; }
	const wxVariant &GetKeyword() const { return m_key; }
	enKeywordTypes GetKeywordType() const;

	void SetStartPos(int val) { m_start_pos = val; }
	void SetLastPos(int val) { m_last_pos = val; }
	void SetWeight(double val) { m_weight = val; }
	void SetKeywordString(const wxString &val);
	void SetKeywordRegex(const wxString &val);
	void SetKeywordArrayString(const wxArrayString &val);
};

//////////////////////////////////////////////////////////////////////

/// @class BootKeywords
///
/// @brief BootKeyword のリスト
WX_DECLARE_OBJARRAY(BootKeyword, BootKeywords);

//////////////////////////////////////////////////////////////////////

/// @brief ブートストラップパラメータ
class BootParam
{
protected:
	int boot_type;					///< 種類
	wxString boot_type_name;		///< 種類名
	wxString category_name;			///< カテゴリ名
	BootKeywords keywords;			///< 比較用キーワード
	BasicParamNames basic_types;	///< BASIC種類（DiskBasicParamとのマッチングにも使用）
	wxString description;			///< 説明

public:
	BootParam();
	BootParam(const BootParam &src);
	virtual ~BootParam() {}

	/// @brief 代入
	BootParam &operator=(const BootParam &src);
	/// @brief 設定
	void SetBootParam(const BootParam &src);
	/// @brief 全パラメータを設定
	void SetBootParam(int n_boot_type
		, const wxString &n_boot_type_name
		, const wxString &n_category_name
		, const BootKeywords &n_keywords
		, const BasicParamNames &n_basic_types
		, const wxString &n_description
	);

	/// @brief 初期化
	void ClearBootParam();
	/// @brief DISK BASICをさがす
	const BasicParamName *FindBasicType(const wxString &type_name, int flags = -1) const;

	/// @brief 種類を設定
	void SetBootType(int val) { boot_type = val; }
	/// @brief 種類名を設定
	void SetBootTypeName(const wxString &str) { boot_type_name = str; }
	/// @brief カテゴリ名を設定
	void SetCategoryName(const wxString &str) { category_name = str; }
	/// @brief キーワードを設定
	void SetKeywords(const BootKeywords &arr) { keywords = arr; }
	/// @brief BASIC種類を設定
	void SetBasicTypes(const BasicParamNames &arr) { basic_types = arr; }
	/// @brief 説明を設定
	void SetDescription(const wxString &str) { description = str; }

	/// @brief 種類を返す
	int GetBootType() const { return boot_type; }
	/// @brief 種類名を返す
	const wxString &GetBootTypeName() const { return boot_type_name; }
	/// @brief カテゴリ名を返す
	const wxString &GetCategoryName() const { return category_name; }
	/// @brief キーワードを返す
	const BootKeywords &GetKeywords() const { return keywords; }
	/// @brief BASIC種類を返す
	const BasicParamNames &GetBasicTypes() const { return basic_types; }
	/// @brief 説明を返す
	const wxString &GetDescription() const { return description; }
};

//////////////////////////////////////////////////////////////////////

/// @class BootParams
///
/// @brief BootParam のリスト
WX_DECLARE_OBJARRAY(BootParam, BootParams);

/// @class BootParamPtrs
///
/// @brief BootParam のポインタリスト
WX_DEFINE_ARRAY(const BootParam *, BootParamPtrs);

//////////////////////////////////////////////////////////////////////

/// @brief ブートストラップのカテゴリ(メーカ毎にまとめる)クラス
class BootCategory : public DiskCategory
{
public:
	BootCategory();
	BootCategory(const wxString & n_type_name, const wxString & n_description);
};

//////////////////////////////////////////////////////////////////////

/// @class BootCategories
///
/// @brief BootCategory のリスト
WX_DECLARE_OBJARRAY(BootCategory, BootCategories);

//////////////////////////////////////////////////////////////////////

/// @brief ブートストラップパラメータのテンプレートを提供する
class BootTemplates : public TemplatesBase
{
private:
	BootParams params;
	BootCategories categories;

public:
	BootTemplates();
	~BootTemplates() {}

	/// @brief XMLファイルから読み込み
	bool Load(const wxString &data_path, const wxString &locale_name, wxString &errmsgs);
	/// @brief BootTypesエレメントをロード
	bool LoadBootTypes(const wxXmlNode *node, const wxString &locale_name, wxString &errmsgs);
	/// @brief BootCategoryエレメントのロード
	bool LoadBootCategories(const wxXmlNode *node, const wxString &locale_name, wxString &errmsgs);
	/// @brief CompareKeywordsエレメントをロード
	bool LoadKeywords(const wxXmlNode *node, BootKeywords &keywords, wxString &errmsgs);
	/// @brief CompareKeywordsエレメントの属性をロード
	bool LoadKeywordAttrs(const wxXmlNode *node, BootKeyword &keyword);
	/// @brief CompareKeywordsエレメントのStringエレメントをロード
	bool LoadKeywordString(const wxXmlNode *node, BootKeywords &keywords);
	/// @brief CompareKeywordsエレメントのRegexエレメントをロード
	bool LoadKeywordRegex(const wxXmlNode *node, BootKeywords &keywords);
	/// @brief CompareKeywordsエレメントのArrayStringエレメントをロード
	bool LoadKeywordArrayString(const wxXmlNode *node, BootKeywords &keywords);
	/// @brief DiskBasicTypesエレメントをロード
	bool LoadDiskBasicTypes(const wxXmlNode *node, BasicParamNames &basic_types, wxString &errmsgs);

	/// @brief 一致するテンプレートの番号を返す
	int IndexOf(const BootParam *n_boot_param) const;
	/// @brief タイプ名に一致するテンプレートの番号を返す
	int IndexOf(const wxString &n_type_name) const;
	/// @brief タイプ名に一致するテンプレートを返す
	const BootParam *FindType(const wxString &n_type_name) const;
	/// @brief カテゴリ番号に一致するタイプ名リストを検索
	size_t FindTypeNames(size_t n_category_index, wxArrayString &n_type_names) const;
	/// @brief カテゴリ名に一致するタイプ名リストを検索
	size_t FindTypeNames(const wxString &n_category_name, wxArrayString &n_type_names) const;
	/// @brief タイプ名に一致するカテゴリを返す
	const BootCategory *FindCategory(const wxString &n_type_name) const;
	/// @brief テンプレートを返す
	const BootParam *ItemPtr(size_t index) const { return &params[index]; }
	/// @brief テンプレートを返す
	const BootParam &Item(size_t index) const { return params[index]; }
	/// @brief テンプレートの数を返す
	size_t Count() const { return params.Count(); }
	/// @brief タイプ名に一致するカテゴリを返す
	const BootCategories &GetCategories() const { return categories; }
};

extern BootTemplates gBootTemplates;

#endif /* BOOT_PARAMETER_H */
