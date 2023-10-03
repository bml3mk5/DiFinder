/// @file diskimage.h
///
/// @brief ディスクイメージ入出力
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#include "../common.h"
#include <wx/string.h>
#include <wx/filename.h>
//#include <wx/mstream.h>
#include <wx/dynarray.h>
#include <wx/hashmap.h>
#include "diskparam.h"
#include "bootparam.h"
#include "diskresult.h"

class DiskWriteOptions;
class DiskImageDisk;

// ----------------------------------------------------------------------

WX_DECLARE_HASH_MAP( int, int, wxIntegerHash, wxIntegerEqual, IntHashMap );

/// ハッシュを扱うクラス
class IntHashMapUtil
{
public:
	static void IncleaseValue(IntHashMap &hash_map, int key);
	static int GetMaxKeyOnMaxValue(IntHashMap &hash_map);
	static int MaxValue(int src, int value);
	static int MinValue(int src, int value);
};

// ----------------------------------------------------------------------

/// セクタデータへのヘッダ部分を渡すクラス
class DiskImageSectorHeader
{
public:
	int m_id;
protected:
	DiskImageSectorHeader() { m_id = 0; }
public:
	DiskImageSectorHeader(int id) { m_id = id; }
	virtual ~DiskImageSectorHeader() {}
	virtual bool Copy(DiskImageSectorHeader &src) { return false; }
};

// ----------------------------------------------------------------------

/// セクタデータへのポインタを保持するクラス
class DiskImageSector
{
protected:
	int m_num;		///< sector number

	DiskImageSector() {}
	DiskImageSector(const DiskImageSector &src) {}
	DiskImageSector &operator=(const DiskImageSector &src) { return *this; }

public:
	DiskImageSector(int n_num);
	virtual ~DiskImageSector();

	/// セクタのデータを置き換える
	virtual bool	Replace(DiskImageSector *src_sector) { return false; }
	/// セクタのデータを埋める
	virtual bool	Fill(wxUint8 code, int len = -1, int start = 0) { return false; } 
	/// セクタのデータを上書き
	virtual bool	Copy(const void *buf, int len, int start = 0) { return false; } 
	/// セクタのデータに指定したバイト列があるか
	virtual int		Find(const void *buf, size_t len) { return -1; }
	/// 指定位置のセクタデータを返す
	virtual wxUint8	Get(int pos) const { return 0; }
	/// 指定位置のセクタデータを返す
	virtual wxUint16 Get16(int pos, bool big_endian = false) const { return 0; }
	/// 指定位置にセクタデータを設定
	virtual void	Set(int pos, wxUint8 val) {}
	/// 指定位置のセクタデータを設定
	virtual void	Set16(int pos, wxUint16 val, bool big_endian = false) {}

	/// ヘッダをコピー
	virtual bool	CopyHeaderTo(DiskImageSectorHeader &dst) { return false; }

	/// セクタ番号を返す
	virtual int		GetNumber() const { return m_num; }
	/// セクタ番号を設定
	virtual void	SetNumber(int val) { m_num = val; }
	/// 同じセクタか
	virtual bool	IsSameSector(int sector_number, bool deleted_mark = false) { return false; }
	/// セクタサイズを返す
	virtual int		GetSectorSize() const { return 0; }
	/// セクタサイズを設定
	virtual void	SetSectorSize(int val) {}
	/// セクタサイズ（バッファのサイズ）を返す
	virtual int		GetSectorBufferSize() const { return 0; }
	/// セクタサイズ（ヘッダ＋バッファのサイズ）を返す
	virtual int		GetSize() const = 0;
	/// セクタデータへのポインタを返す
	virtual wxUint8 *GetSectorBuffer() { return NULL; }
	/// セクタデータへのポインタを返す
	virtual wxUint8 *GetSectorBuffer(int offset) { return NULL; }

	/// 変更されているか
	virtual bool	IsModified() const { return false; }
	/// 変更済みをクリア
	virtual void	ClearModify() {}

	/// セクタ内容の比較
	static int Compare(DiskImageSector *item1, DiskImageSector *item2);
	/// セクタ番号の比較
	static int CompareIDR(DiskImageSector **item1, DiskImageSector **item2);
	/// ID Nからセクタサイズを計算
	static int ConvIDNToSecSize(wxUint8 n);
	/// セクタサイズからID Nを計算
	static wxUint8 ConvSecSizeToIDN(int size);

	/// リファレンス数
	virtual int GetRefs() const { return 0; }
	/// リファレンス数をクリア
	virtual void ClearRefs() {}
	/// リファレンス数を増やす
	virtual void IncRefs() {}
	/// リファレンス数を減らす
	virtual void DecRefs() {}
};

// ----------------------------------------------------------------------

/// １ディスクのヘッダを渡すクラス
class DiskImageDiskHeader
{
protected:
	int m_id;
private:
	DiskImageDiskHeader();
public:
	DiskImageDiskHeader(int id);
	virtual ~DiskImageDiskHeader();
	virtual bool Copy(DiskImageDiskHeader &src);
};

// ----------------------------------------------------------------------

class DiskImageFile;
class DiskBasicParam;
class DiskBasic;
class DiskBasics;

/// １ディスク（パーティション）情報を保持するクラス
class DiskImageDisk
{
protected:
	DiskImageFile *parent;
	int m_num;					///< disk number

	BasicParamNames m_basic_types;	///< BASIC種類（DiskBasicParamとのマッチングにも使用）

	DiskImageDisk() {}
	DiskImageDisk(const DiskImageDisk &src) {}
	DiskImageDisk &operator=(const DiskImageDisk &src) { return *this; }

public:
	DiskImageDisk(DiskImageFile *file, int n_num);
	DiskImageDisk(DiskImageFile *file, int n_num, DiskImageDiskHeader &n_header);
	virtual ~DiskImageDisk();

	/// ディスクの内容を置き換える
	virtual int		Replace(DiskImageDisk *src_disk, int src_side_number) { return 0; }
	/// ディスクサイズ計算（ディスクヘッダ分を除く）
	virtual size_t	CalcSizeWithoutHeader() { return 0; }

	/// ヘッダをコピー
	virtual bool	CopyHeaderTo(DiskImageDiskHeader &dst) { return false; }

	/// ディスク番号を返す
	virtual int		GetNumber() const { return m_num; }
	/// ディスク名を返す
	virtual wxString GetName(bool real = false) const;
	/// ディスク名を設定
	virtual void	SetName(const wxString &val) {}
	/// ディスク名を設定
	virtual void	SetName(const wxUint8 *buf, size_t len) {}

	/// ディスクファイルを返す
	virtual DiskImageFile   *GetFile() const { return parent; }
	/// セクタサイズを返す
	virtual int		GetSectorSize() const;
	/// 指定セクタを返す
	virtual DiskImageSector *GetSector(int block_num) { return NULL; }
	/// キャッシュを更新する
	virtual void	RefreshCache(int block_num) {}

	/// 書き込み禁止かどうかを返す
	virtual bool	IsWriteProtected() const { return true; }
	/// 書き込み禁止かどうかを設定
	virtual void	SetWriteProtect(bool val) {}

	/// ディスクサイズ（ヘッダサイズ含む）
	virtual wxUint32 GetSize() const { return 0; }
	/// ディスクサイズ（ヘッダサイズ含む）を設定
	virtual void	SetSize(wxUint32 val) {}
	/// ディスクサイズ（ヘッダサイズを除く）
	virtual wxUint32 GetSizeWithoutHeader() const { return 0; }
	/// ディスクサイズ（ヘッダサイズを除く）を設定
	virtual void	SetSizeWithoutHeader(wxUint32 val) {}
	/// ヘッダサイズを返す
	virtual wxUint32 GetHeaderSize() const { return 0; }

	/// 開始セクタ番号を返す
	virtual wxUint32 GetStartSectorNumber() const { return 0; }
	/// 開始セクタ番号を設定
	virtual void	SetStartSectorNumber(wxUint32 val) {}
	/// セクタ数を返す
	virtual wxUint32 GetNumberOfSectors() const { return 0; }
	/// セクタ数を設定
	virtual void	SetNumberOfSectors(wxUint32 val) {}

	/// ディスクの内容を初期化する(0パディング)
	virtual bool	Initialize() { return false; }

	/// ディスクの説明
	virtual wxString GetDescription() const { return wxT(""); }
	/// ディスクの説明
	virtual void	SetDescription(const wxString &desc) {}
	/// ディスクの説明
	virtual void	SetDescription(const wxUint8 *buf, size_t len) {}

	/// 変更されているか
	virtual bool	IsModified() { return false; }
	/// 変更済みをクリア
	virtual void	ClearModify() {}

	/// データをすべて出力	
	virtual void	Flush() {}

	/// パラメータ変更フラグを設定
	virtual void	SetParamChanged(bool val) {}
	/// パラメータ変更フラグを返す
	virtual bool	GetParamChanged() const { return false; }

	/// @brief BASIC種類を設定
	virtual void AddBasicType(const wxString &name);
	/// @brief BASIC種類を設定
	virtual void SetBasicTypes(const BasicParamNames &arr) { m_basic_types = arr; }
	/// @brief BASIC種類を返す
	virtual const BasicParamNames &GetBasicTypes() const { return m_basic_types; }

	/// DISK BASIC領域を確保
	virtual void	AllocDiskBasics();
	/// DISK BASICを返す
	virtual DiskBasic *GetDiskBasic(int idx);
	/// DISK BASICを返す
	virtual DiskBasics *GetDiskBasics() { return NULL; }
	/// DISK BASICをクリア
	virtual void	ClearDiskBasics();
	/// キャラクターコードマップ設定
	virtual void	SetCharCode(const wxString &name);

	// セクタ位置からトラック、サイド、セクタ番号を得る(オフセットを考慮)
	virtual void GetNumberFromBlockNum(int block_num, int &track_num, int &side_num, int &sector_num) const;
};

// ----------------------------------------------------------------------

WX_DEFINE_ARRAY(DiskImageDisk *, DiskImageDisks);

// ----------------------------------------------------------------------

/// ディスクイメージへのポインタを保持するクラス
class DiskImageFile : public DiskParam
{
protected:
	wxFileName m_filename;

	wxString m_basic_type_hint;	///< BASIC種類ヒント

	DiskImageFile(const DiskImageFile &src) : DiskParam() {}

public:
	DiskImageFile();
	DiskImageFile(const DiskParam &disk_param);
	virtual ~DiskImageFile();

	/// インスタンス作成
	virtual DiskImageDisk *NewImageDisk(int disk_number, wxUint32 start_block, wxUint32 block_size) = 0;
	/// インスタンス作成
	virtual DiskImageDisk *NewImageDisk(int disk_number, DiskImageDiskHeader &n_header) { return NULL; }

	/// 変更フラグ 追加したかどうか
	enum {
		MODIFY_NONE = 0,
		MODIFY_ADD = 1
	};

	virtual size_t Add(DiskImageDisk *newdsk, short mod_flags);
	virtual void Clear();
	virtual size_t Count() const;
	virtual bool Delete(size_t idx);

	virtual DiskImageDisks *GetDisks() { return NULL; }
	virtual DiskImageDisk  *GetDisk(size_t idx) = 0;

	virtual void SetStartOffset(wxUint32 val) {}
	virtual wxUint32 GetStartOffset() const { return 0; }

	virtual void SetBlockSize(wxUint32 val) {}
	virtual wxUint32 GetBlockSize() const { return 0; }

	/// 指定セクタを返す
	virtual DiskImageSector *GetSector(int sector_pos) { return NULL; }
	/// キャッシュを更新する
	virtual void RefreshCache(int sector_pos) {}
	/// キャッシュをクリアする
	void ClearCache(wxUint32 start, wxUint32 size) {}
	/// キャッシュのリファレンス数ををクリアする
	void ClearCacheRefs(wxUint32 start, wxUint32 size) {}

	virtual bool IsModified() { return false; }
	virtual void ClearModify() {}

	/// データをすべて出力	
	virtual void Flush() {}

	/// 書き込み禁止かどうかを返す
	virtual bool	IsWriteProtected() const { return true; }
	/// 書き込み禁止かどうかを設定
	virtual void	SetWriteProtect(bool val) {}

	/// ファイル名を返す
	virtual wxString GetName() const;
	/// パスを返す
	virtual wxString GetPath() const;
	/// ファイルパスを返す
	virtual wxString GetFilePath() const;

	/// ファイル名を設定
	virtual void SetFileName(const wxString &path);

	/// ファイルの説明
	virtual wxString GetDescription() const { return wxT(""); }
	/// ファイルの説明
	virtual void	SetDescription(const wxString &desc) {}

	virtual const wxString &GetBasicTypeHint() const { return m_basic_type_hint; }
	virtual void SetBasicTypeHint(const wxString &val) { m_basic_type_hint = val; };

	/// ステータスメッセージ
	virtual void GetStatusMessage(wxString &str) const {}

	/// セクタ位置からトラック、サイド、セクタ番号を得る
	virtual void GetNumberFromSectorPos(int sector_pos, int &track_num, int &side_num, int &sector_num) const;
	/// トラック、サイド、セクタ番号からセクタ位置を得る
	virtual int  GetSectorPosFromNumber(int track_num, int side_num, int sector_num) const;
	
};

// ----------------------------------------------------------------------

/// ディスクイメージ入出力
class DiskImage
{
protected:
	DiskImageFile *p_file;

	DiskResult m_result;

	virtual void NewFile(const wxString &newpath);
	virtual void ClearFile();

public:
	DiskImage();
	virtual ~DiskImage();

	/// インスタンス作成
	virtual DiskImageFile *NewImageFile() = 0;

	/// 新規作成
	virtual int Create(const wxString &diskname, const DiskParam &param, bool write_protect, const wxString &basic_hint);
	/// 追加で新規作成
	virtual int Add(const wxString &diskname, const DiskParam &param, bool write_protect, const wxString &basic_hint);
	/// ファイルを追加
	virtual int Add(const wxString &filepath, const wxString &file_format, const DiskParam &param_hint);
	/// ファイルを開く
	virtual int Open(const wxString &filepath, const wxString &file_format, const DiskParam &param_hint);
	/// ファイルを開く前のチェック
	virtual int Check(const wxString &filepath, wxString &file_format, DiskParamPtrs &params, DiskParam &manual_param);
	/// 閉じる
	virtual void Close();
	/// ファイルを開いているか
	virtual bool IsOpened() const;
	/// ストリームの内容を上書き保存できるか
	virtual int CanSave();
	/// ストリームの内容をファイルに保存できるか
	virtual int CanSave(const wxString &filepath, const wxString &file_format);
	/// ストリームの内容を上書き保存
	virtual int Save(const DiskWriteOptions &options);
	/// ストリームの内容をファイルに保存
	virtual int Save(const wxString &filepath, const wxString &file_format, const DiskWriteOptions &options);
	/// ストリームの内容をファイルに保存
	virtual int SaveDisk(int disk_number, const wxString &filepath, const wxString &file_format, const DiskWriteOptions &options);
	/// ディスクを削除
	virtual bool Delete(int disk_number);
	/// 置換元のディスクを解析
	virtual int ParseForReplace(int disk_number, const wxString &filepath, const wxString &file_format, const DiskParam &param_hint, DiskImageFile &src_file, DiskImageDisk* &tag_disk);
	/// ファイルでディスクを置換
	virtual int ReplaceDisk(int disk_number, DiskImageDisk *src_disk, int src_side_number, DiskImageDisk *tag_disk);

	/// ディスク名を設定
	virtual bool SetDiskName(size_t disk_number, const wxString &newname);
	/// ディスク名を返す
	virtual wxString GetDiskName(size_t disk_number, bool real = false) const;

	/// ディスクを変更したか
	virtual bool IsModified();

	/// ディスクファイルを返す
	virtual DiskImageFile		*GetFile() { return p_file; }
	/// ディスクファイルを返す
	virtual const DiskImageFile	*GetFile() const { return p_file; }
	/// ディスク枚数
	virtual size_t CountDisks() const;
	/// ディスク一覧を返す
	virtual DiskImageDisks *GetDisks();
	/// 指定した位置のディスクを返す
	virtual DiskImageDisk  *GetDisk(size_t index);
	/// 指定した位置のディスクを返す
	virtual const DiskImageDisk	*GetDisk(size_t index) const;

	/// ファイル名を返す
	virtual wxString GetName() const;
	/// パスを返す
	virtual wxString GetPath() const;
	/// ファイルパスを返す
	virtual wxString GetFilePath() const;

	/// ファイル名を設定
	virtual void SetFileName(const wxString &path);

	/// DISK BASICが一致するか
	virtual bool MatchDiskBasic(const DiskBasic *target);
	/// DISK BASICの解析状態をクリア
	virtual void ClearDiskBasicParseAndAssign(int disk_number, int side_number);
	/// キャラクターコードマップ設定
	virtual void SetCharCode(const wxString &name);

	/// ステータスメッセージ
	virtual void GetStatusMessage(wxString &str) const;

	/// エラーメッセージ
	virtual const wxArrayString &GetErrorMessage(int maxrow = 20);
	/// エラーメッセージを表示
	virtual void  ShowErrorMessage();
	/// エラー警告メッセージを表示
	virtual int   ShowErrWarnMessage();
};

#endif /* DISKIMAGE_H */
