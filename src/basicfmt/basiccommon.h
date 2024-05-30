/// @file basiccommon.h
///
/// @brief disk basic common
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef BASICCOMMON_H
#define BASICCOMMON_H


#include "../common.h"
#include <wx/string.h>
#include <wx/dynarray.h>


//////////////////////////////////////////////////////////////////////

#define FILENAME_BUFSIZE	(32)
#define FILEEXT_BUFSIZE		(4)

//////////////////////////////////////////////////////////////////////

/// @brief 共通属性フラグ
enum en_file_type_mask {
	FILE_TYPE_BASIC_MASK	 = 0x000001,
	FILE_TYPE_DATA_MASK		 = 0x000002,
	FILE_TYPE_MACHINE_MASK	 = 0x000004,
	FILE_TYPE_ASCII_MASK	 = 0x000008,
	FILE_TYPE_BINARY_MASK	 = 0x000010,
	FILE_TYPE_RANDOM_MASK	 = 0x000020,
	FILE_TYPE_ENCRYPTED_MASK = 0x000040,
	FILE_TYPE_READWRITE_MASK = 0x000080,
	FILE_TYPE_READONLY_MASK	 = 0x000100,
	FILE_TYPE_HIDDEN_MASK	 = 0x000200,
	FILE_TYPE_SYSTEM_MASK	 = 0x000400,
	FILE_TYPE_VOLUME_MASK	 = 0x000800,
	FILE_TYPE_DIRECTORY_MASK = 0x001000,
	FILE_TYPE_ARCHIVE_MASK	 = 0x002000,
	FILE_TYPE_LIBRARY_MASK	 = 0x004000,
	FILE_TYPE_NONSHARE_MASK	 = 0x008000,
	FILE_TYPE_UNDELETE_MASK	 = 0x010000,
	FILE_TYPE_WRITEONLY_MASK = 0x020000,
	FILE_TYPE_TEMPORARY_MASK = 0x040000,
	FILE_TYPE_INTEGER_MASK	 = 0x080000,
	FILE_TYPE_HARDLINK_MASK	 = 0x100000,
	FILE_TYPE_SOFTLINK_MASK	 = 0x200000,

	FILE_TYPE_EXTENSION_MASK = FILE_TYPE_BASIC_MASK
		| FILE_TYPE_DATA_MASK
		| FILE_TYPE_MACHINE_MASK
		| FILE_TYPE_ASCII_MASK
		| FILE_TYPE_BINARY_MASK
		| FILE_TYPE_RANDOM_MASK
		| FILE_TYPE_INTEGER_MASK,
};
/// @brief 共通属性フラグ位置
enum en_file_type_pos {
	FILE_TYPE_BASIC_POS		= 0,
	FILE_TYPE_DATA_POS		= 1,
	FILE_TYPE_MACHINE_POS	= 2,
	FILE_TYPE_ASCII_POS		= 3,
	FILE_TYPE_BINARY_POS	= 4,
	FILE_TYPE_RANDOM_POS	= 5,
	FILE_TYPE_ENCRYPTED_POS = 6,
	FILE_TYPE_READWRITE_POS = 7,
	FILE_TYPE_READONLY_POS	= 8,
	FILE_TYPE_HIDDEN_POS	= 9,
	FILE_TYPE_SYSTEM_POS	= 10,
	FILE_TYPE_VOLUME_POS	= 11,
	FILE_TYPE_DIRECTORY_POS = 12,
	FILE_TYPE_ARCHIVE_POS	= 13,
	FILE_TYPE_LIBRARY_POS	= 14,
	FILE_TYPE_NONSHARE_POS	= 15,
	FILE_TYPE_UNDELETE_POS	= 16,
	FILE_TYPE_WRITEONLY_POS	= 17,
	FILE_TYPE_TEMPORARY_POS	= 18,
	FILE_TYPE_INTEGER_POS	= 19,
	FILE_TYPE_HARDLINK_POS	= 20,
	FILE_TYPE_SOFTLINK_POS	= 21,
};

//////////////////////////////////////////////////////////////////////
//
// ディレクトリ
//
//////////////////////////////////////////////////////////////////////

#pragma pack(1)
/// @brief ディレクトリエントリ MS-DOS FAT (32bytes)
typedef struct st_directory_ms_dos {
	wxUint8 name[8];
	wxUint8 ext[3];
	wxUint8 type;
	wxUint8 ntres;
	wxUint8  ctime_tenth;
	wxUint16 ctime;
	wxUint16 cdate;
	wxUint16 adate;
	wxUint16 start_group_hi;
	wxUint16 wtime;
	wxUint16 wdate;
	wxUint16 start_group;
	wxUint32 file_size;
} directory_msdos_t;

/// @brief ディレクトリエントリ MS-DOS LFN (32bytes)
typedef struct st_directory_ms_lfn {
	wxUint8 order;
	wxUint8 name[10];
	wxUint8 type;
	wxUint8 type2;
	wxUint8 chksum;
	wxUint8 name2[12];
	wxUint16 dummy_group;
	wxUint8 name3[4];
} directory_ms_lfn_t;

/// @brief ディレクトリエントリ Human68K (MS-DOS compatible) (32bytes)
typedef struct st_directory_hu68k {
	wxUint8 name[8];
	wxUint8 ext[3];
	wxUint8 type;
	wxUint8 name2[10];
	wxUint16 wtime;
	wxUint16 wdate;
	wxUint16 start_group;
	wxUint32 file_size;
} directory_hu68k_t;

/// @brief ディレクトリエントリ MS-DOS compatible (32bytes)
typedef union un_directory_ms {
	directory_msdos_t  msdos;
	directory_ms_lfn_t mslfn;
	directory_hu68k_t  hu68k;
} directory_ms_t;

/// @brief OS-9 LSN
typedef struct st_os9_lsn {
	wxUint8 h;
	wxUint8 m;
	wxUint8 l;
} os9_lsn_t;

#define GET_OS9_LSN(lsn) (((wxUint32)(lsn.h) << 16) | ((wxUint32)(lsn.m) << 8) | lsn.l)
#define SET_OS9_LSN(lsn, val) { \
	lsn.h = (((val) & 0xff0000) >> 16); \
	lsn.m = (((val) & 0xff00) >> 8); \
	lsn.l = ((val) & 0xff); \
}

/// @brief OS-9 Segment
typedef struct st_os9_segment {
	os9_lsn_t LSN;
	wxUint16  SIZ;
} os9_segment_t;

/// @brief OS-9 Date Format
typedef struct st_os9_date {
	wxUint8 yy;
	wxUint8 mm;
	wxUint8 dd;
	wxUint8 hh;
	wxUint8 mi;
} os9_date_t;

/// @brief OS-9 Created Date
typedef struct st_os9_cdate {
	wxUint8 yy;
	wxUint8 mm;
	wxUint8 dd;
} os9_cdate_t;

/// @brief ディレクトリエントリ OS-9 (32bytes)
typedef struct st_directory_os9 {
	wxUint8		DE_NAM[28];
	wxUint8		DE_Reserved;
	os9_lsn_t	DE_LSN;	// link to FD
} directory_os9_t;

/// @brief OS-9 File Descriptor
typedef struct st_directory_os9_fd {
	wxUint8		FD_ATT;	// attr
	wxUint16	FD_OWN;	// owner id
	os9_date_t	FD_DAT;	// date
	wxUint8		FD_LNK;	// link count
	wxUint32	FD_SIZ;	// in bytes
	os9_cdate_t	FD_DCR;	// created date
	os9_segment_t FD_SEG[48];	// 5*48=240
} directory_os9_fd_t;

/// @brief ディレクトリエントリ サイズに注意！
typedef union un_directory {
	wxUint8				name[16];
	directory_msdos_t	msdos;
	directory_ms_lfn_t	mslfn;
	directory_os9_t		os9;
	directory_hu68k_t	hu68k;
} directory_t;
#pragma pack()

/// @brief DISK BASIC種類 番号
enum DiskBasicFormatType {
	FORMAT_TYPE_UNKNOWN	= -1,
	FORMAT_TYPE_MSDOS	= 3,
	FORMAT_TYPE_OS9		= 9,
	FORMAT_TYPE_HU68K	= 14,
	FORMAT_TYPE_MACHFS	= 41,
};

//////////////////////////////////////////////////////////////////////

/// @brief 名前と値 定数リスト用
typedef struct st_name_value {
	const char *name;
	int        value;

	/// @brief 名前が一致するか
	static int IndexOf(const struct st_name_value *list, const wxString &str);
	/// @brief 値が一致するか
	static int IndexOf(const struct st_name_value *list, int val);
} name_value_t;

//////////////////////////////////////////////////////////////////////

/// @brief 値と値 定数リスト用
typedef struct st_value_value {
	int com_value;
	int ori_value;
} value_value_t;

//////////////////////////////////////////////////////////////////////

/// @brief ファイルプロパティでファイル名変更した時に渡す値
class DiskBasicFileName
{
private:
	wxString	name;		///< ファイル名
	int			optional;	///< 拡張属性 ファイル名が同じでも、この属性が異なれば違うファイルとして扱う

public:
	DiskBasicFileName();
	DiskBasicFileName(const wxString &n_name, int n_optional = 0);
	~DiskBasicFileName();

	/// @brief ファイル名
	const wxString &GetName() const { return name; }
	/// @brief ファイル名
	wxString &GetName() { return name; }
	/// @brief ファイル名
	void SetName(const wxString &val) { name = val; }
	/// @brief 拡張属性 ファイル名が同じでも、この属性が異なれば違うファイルとして扱う
	int GetOptional() const { return optional; }
	/// @brief 拡張属性 ファイル名が同じでも、この属性が異なれば違うファイルとして扱う
	void SetOptional(int val) { optional = val; }
};

//////////////////////////////////////////////////////////////////////

/// @brief 属性保存クラス
class DiskBasicFileType
{
private:
	DiskBasicFormatType format;	///< DISK BASIC種類
	int type;					///< 共通属性 enum #en_file_type_mask の値の組み合わせ
	int origin[3];				///< 本来の属性

public:
	DiskBasicFileType();
	DiskBasicFileType(DiskBasicFormatType n_format, int n_type, int n_origin0 = 0, int n_origin1 = 0, int n_origin2 = 0);
	~DiskBasicFileType();

	/// @brief DISK BASIC種類
	DiskBasicFormatType GetFormat() const { return format; }
	/// @brief DISK BASIC種類
	void SetFormat(DiskBasicFormatType val) { format = val; }
	/// @brief 共通属性 enum #en_file_type_mask の値の組み合わせ
	int GetType() const { return type; }
	/// @brief 共通属性 enum #en_file_type_mask の値の組み合わせ
	void SetType(int val) { type = val; }
	/// @brief 本来の属性
	int GetOrigin(int idx = 0) const { return origin[idx]; }
	/// @brief 本来の属性
	void SetOrigin(int val) { origin[0] = val; }
	/// @brief 本来の属性
	void SetOrigin(int idx, int val) { origin[idx] = val; }

	/// @brief 共通属性が一致するか
	bool MatchType(int mask, int value) const;
	/// @brief 共通属性が一致しないか
	bool UnmatchType(int mask, int value) const;

	/// @brief 共通属性がアスキー属性か
	bool IsAscii() const;
	/// @brief 共通属性がボリューム属性か
	bool IsVolume() const;
	/// @brief 共通属性がディレクトリ属性か
	bool IsDirectory() const;
};

//////////////////////////////////////////////////////////////////////

/// @brief グループ番号に対応する機種依存データを保持
///
/// @sa DiskBasicGroupItem
class DiskBasicGroupUserData
{
public:
	DiskBasicGroupUserData() {}
	virtual ~DiskBasicGroupUserData() {}
	virtual DiskBasicGroupUserData *Clone() const { return new DiskBasicGroupUserData(*this); }
};

//////////////////////////////////////////////////////////////////////

/// @brief グループ番号に対応するパラメータを保持
///
/// @sa DiskBasicGroups
class DiskBasicGroupItem
{
private:
	wxUint32 m_group;			///< グループ番号
	wxUint32 m_next;			///< 次のグループ番号
//	int m_track;				///< トラック番号
//	int m_side;					///< サイド番号
	int m_sector_start;			///< グループ内の開始セクタ番号
	int m_sector_end;			///< グループ内の終了セクタ番号
//	int m_div_num;				///< １グループがセクタ内に複数ある時の分割位置
//	int m_div_nums;				///< １グループがセクタ内に複数ある時の分割数
	DiskBasicGroupUserData *p_user_data;	///< 機種依存データ
public:
	DiskBasicGroupItem();
	DiskBasicGroupItem(const DiskBasicGroupItem &);
//	DiskBasicGroupItem(wxUint32 n_group, wxUint32 n_next, int n_start, int n_end, int n_div = 0, int n_divs = 1, DiskBasicGroupUserData *n_user = NULL);
	DiskBasicGroupItem(wxUint32 n_group, wxUint32 n_next, int n_start, int n_end, DiskBasicGroupUserData *n_user = NULL);
	DiskBasicGroupItem(wxUint32 n_group, wxUint32 n_next, int n_start, DiskBasicGroupUserData *n_user = NULL);
	~DiskBasicGroupItem();
	/// @brief 代入
	DiskBasicGroupItem &operator=(const DiskBasicGroupItem &);
//	/// @brief データセット
//	void Set(wxUint32 n_group, wxUint32 n_next, int n_start, int n_end, int n_div = 0, int n_divs = 1, DiskBasicGroupUserData *n_user = NULL);
	/// @brief データセット
	void Set(wxUint32 n_group, wxUint32 n_next, int n_start, int n_end, DiskBasicGroupUserData *n_user = NULL);
	/// @brief データセット
	void Set(wxUint32 n_group, wxUint32 n_next, int n_start, DiskBasicGroupUserData *n_user = NULL);
	/// @brief グループ番号を返す
	wxUint32 GetGroup() const { return m_group; }
	/// @brief 次のグループ番号を返す
	wxUint32 GetNextGroup() const { return m_next; }
	/// @brief グループ内の開始セクタ番号を返す
	int GetSectorStart() const { return m_sector_start; }
	/// @brief グループ内の終了セクタ番号を返す
	int GetSectorEnd() const { return m_sector_end; }
	/// @brief 次のグループ番号を設定
	void SetNextGroup(wxUint32 val) { m_next = val; }
	/// @brief グループ内の開始セクタ番号を設定
	void SetSectorStart(int val) { m_sector_start = val; }
	/// @brief グループ内の終了セクタ番号を設定
	void SetSectorEnd(int val) { m_sector_end = val; }
	/// @brief 比較
	static int Compare(DiskBasicGroupItem **item1, DiskBasicGroupItem **item2);
};

/// @class DiskBasicGroupItems
///
/// @brief グループ番号 DiskBasicGroupItem のリスト
WX_DECLARE_OBJARRAY(DiskBasicGroupItem, DiskBasicGroupItems);

//////////////////////////////////////////////////////////////////////

/// @brief グループ番号のリストを保持
///
/// ディスク内ファイルのチェインをこのリストに保持する
///
/// @sa DiskBasicGroupItem , DiskBasicDirItem
class DiskBasicGroups
{
private:
	DiskBasicGroupItems	items;			///< グループ番号のリスト
	int					nums;			///< グループ数
	size_t				size;			///< グループ内の占有サイズ
	size_t				size_per_group;	///< １グループのサイズ

public:
	DiskBasicGroups();
	~DiskBasicGroups() {}

//	/// @brief 追加
//	void	Add(wxUint32 n_group, wxUint32 n_next, int n_start, int n_end, int n_div = 0, int n_divs = 1, DiskBasicGroupUserData *n_user = NULL);
	/// @brief 追加
	void	Add(wxUint32 n_group, wxUint32 n_next, int n_start, int n_end, DiskBasicGroupUserData *n_user = NULL);
	/// @brief 追加
	void	Add(wxUint32 n_group, wxUint32 n_next, int n_start, DiskBasicGroupUserData *n_user = NULL);
	/// @brief 追加
	void	Add(const DiskBasicGroupItem &n_item);
	/// @brief 追加
	void	Add(const DiskBasicGroups &n_items);
	/// @brief リストをクリア
	void	Empty();
	/// @brief リストの数を返す
	size_t	Count() const;
	/// @brief リストの最後を返す
	DiskBasicGroupItem &Last() const;
	/// @brief リストアイテムを返す
	DiskBasicGroupItem &Item(size_t idx) const;
	/// @brief リストアイテムを返す
	DiskBasicGroupItem *ItemPtr(size_t idx) const;
	/// @brief リストを返す
	const DiskBasicGroupItems &GetItems() const { return items; }

	/// @brief グループ数を返す
	int		GetNums() const { return nums; }
	/// @brief 占有サイズを返す
	size_t	GetSize() const { return size; }
	/// @brief １グループのサイズを返す
	size_t	GetSizePerGroup() const { return size_per_group; }

	/// @brief グループ数を設定
	void	SetNums(int val) { nums = val; }
	/// @brief 占有サイズを設定
	void	SetSize(size_t val) { size = val; }
	/// @brief １グループのサイズを設定
	void	SetSizePerGroup(size_t val) { size_per_group = val; }

	/// @brief グループ数を足す
	int		AddNums(int val);
	/// @brief 占有サイズを足す
	int		AddSize(int val);

	/// @brief グループ番号でソート
	void SortItems();
};

//////////////////////////////////////////////////////////////////////

/// @brief 汎用リスト用アイテム
///
/// @sa KeyValArray
class KeyValItem
{
public:
	wxString m_key;
	wxUint8 *m_value;
	size_t   m_size;
	enum en_type {
		TYPE_UNKNOWN = 0,
		TYPE_INTEGER,
		TYPE_UINT8,
		TYPE_UINT16,
		TYPE_UINT32,
		TYPE_STRING,
		TYPE_BOOL
	} m_type;
public:
	KeyValItem();
	KeyValItem(const wxString &key, int val);
	KeyValItem(const wxString &key, wxUint8 val, bool invert = false);
	KeyValItem(const wxString &key, wxUint16 val, bool big_endien = false, bool invert = false);
	KeyValItem(const wxString &key, wxUint32 val, bool big_endien = false, bool invert = false);
	KeyValItem(const wxString &key, const void *val, size_t size, bool invert = false);
	KeyValItem(const wxString &key, bool val);
	~KeyValItem();

	void Clear();
	void Set(const wxString &key, int val);
	void Set(const wxString &key, wxUint8 val, bool invert = false);
	void Set(const wxString &key, wxUint16 val, bool big_endien = false, bool invert = false);
	void Set(const wxString &key, wxUint32 val, bool big_endien = false, bool invert = false);
	void Set(const wxString &key, const void *val, size_t size, bool invert = false);
	void Set(const wxString &key, bool val);

	wxString GetValueString() const;

	const wxString &Key() const { return m_key; }

	static int Compare(KeyValItem **item1, KeyValItem **item2);
};

WX_DEFINE_ARRAY(KeyValItem *, ArrayOfKeyValItem);

/// @brief 汎用リスト KeyValItem の配列
class KeyValArray : public ArrayOfKeyValItem
{
private:
	void DeleteAll();

public:
	KeyValArray();
	~KeyValArray();

	void Clear();
	void Empty();
	void Add(const wxString &key, int val);
	void Add(const wxString &key, wxUint8 val, bool invert = false);
	void Add(const wxString &key, wxUint16 val, bool big_endien = false, bool invert = false);
	void Add(const wxString &key, wxUint32 val, bool big_endien = false, bool invert = false);
	void Add(const wxString &key, const void *val, size_t size, bool invert = false);
	void Add(const wxString &key, bool val);
};

#endif /* BASICCOMMON_H */
