/// @file diskcommon.h
///
/// @brief ディスク共通
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISK_COMMON_H
#define DISK_COMMON_H

#include "../common.h"
#include <wx/string.h>

//////////////////////////////////////////////////////////////////////

/// @brief 名前リストを保存 
class DiskParamName
{
protected:
	wxString	m_name;
	int			m_flags;

public:
	DiskParamName();
	DiskParamName(const wxString &name, int flags);
	virtual ~DiskParamName() {}

	/// @brief 名前を設定
	void SetName(const wxString &val) { m_name = val; }
	/// @brief 名前を返す
	const wxString &GetName() const { return m_name; }
	/// @brief フラグを設定
	void SetFlags(int val) { m_flags = val; }
	/// @brief フラグを返す
	int GetFlags() const { return m_flags; }
};

//////////////////////////////////////////////////////////////////////

/// @brief カテゴリ(メーカ毎、OS毎にまとめる)クラス
class DiskCategory
{
protected:
	wxString	m_type_name;
	wxString	m_description;

public:
	DiskCategory();
	DiskCategory(const wxString & n_type_name, const wxString & n_description);
	virtual ~DiskCategory() {}

	/// @brief DISK BASICカテゴリ名
	const wxString&		GetTypeName() const	{ return m_type_name; }
	/// @brief 説明
	const wxString& GetDescription()		{ return m_description; }
	/// @brief 説明
	void			SetDescription(const wxString &str) { m_description = str; }
};

//////////////////////////////////////////////////////////////////////

#endif /* DISK_COMMON_H */
