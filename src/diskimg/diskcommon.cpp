/// @file diskcommon.cpp
///
/// @brief ディスク共通
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskcommon.h"

//////////////////////////////////////////////////////////////////////
//
// 名前リストを保存
//
DiskParamName::DiskParamName()
{
	m_flags = 0;
}
DiskParamName::DiskParamName(const wxString &name, int flags)
{
	m_name = name;
	m_flags = flags;
}

//////////////////////////////////////////////////////////////////////
//
//　カテゴリ(メーカ毎、OS毎にまとめる)クラス
//
DiskCategory::DiskCategory()
{
}
DiskCategory::DiskCategory(const wxString & n_type_name, const wxString & n_description)
{
	m_type_name = n_type_name;
	m_description = n_description;
}
