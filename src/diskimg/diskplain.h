/// @file diskplain.h
///
/// @brief ベタディスクイメージ入出力
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISKPLAIN_H
#define DISKPLAIN_H

#include "../common.h"
#include <wx/string.h>
#include <wx/filename.h>
//#include <wx/mstream.h>
#include <wx/dynarray.h>
#include <wx/hashmap.h>
#include "diskimage.h"
#include "diskparam.h"
#include "diskresult.h"
#include "../utils.h"

#define USE_SECTOR_BLOCK_CACHE

class wxFileStream;

class DiskWriteOptions;
class DiskPlainDisk;

// ----------------------------------------------------------------------

/// セクタデータへのヘッダ部分を渡すクラス
class DiskPlainSectorHeader : public DiskImageSectorHeader
{
};

// ----------------------------------------------------------------------

/// セクタデータへのポインタを保持するクラス
class DiskPlainSector : public DiskImageSector
{
protected:
	wxUint8 *m_data;		///< sector data
	bool m_own_make;		///< allocated buffer by myself
	int m_size;				///< data size
	wxFileOffset m_offset;	///< seek offset in file

	wxUint32 m_crc_32;

	DiskPlainSector(const DiskPlainSector &src) {}
	DiskPlainSector &operator=(const DiskPlainSector &src) { return *this; }

public:
	DiskPlainSector();
	DiskPlainSector(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	virtual ~DiskPlainSector();

	/// セクタの情報を設定
	virtual void Set(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	/// セクタの情報を設定(データはポインタを保持する)
	virtual void SetPtr(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	/// セクタの情報をクリア
	virtual void Clear();

	/// セクタのデータを置き換える
	bool	Replace(DiskImageSector *src_sector);
	/// セクタのデータを埋める
	bool	Fill(wxUint8 code, int len = -1, int start = 0); 
	/// セクタのデータを上書き
	bool	Copy(const void *buf, int len, int start = 0); 
	/// セクタのデータに指定したバイト列があるか
	int		Find(const void *buf, size_t len); 
	/// 指定位置のセクタデータを返す
	wxUint8	Get(int pos) const;
	/// 指定位置のセクタデータを返す
	wxUint16 Get16(int pos, bool big_endian = false) const;
	/// 指定位置にセクタデータを設定
	void	Set(int pos, wxUint8 val);
	/// 指定位置のセクタデータを設定
	void	Set16(int pos, wxUint16 val, bool big_endian = false);

	/// 同じセクタか
	bool	IsSameSector(int sector_number, bool deleted_mark = false);
	/// セクタサイズを返す
	int		GetSectorSize() const;
	/// セクタサイズを設定
	void	SetSectorSize(int val);
	/// セクタサイズ（バッファのサイズ）を返す
	int		GetSectorBufferSize() const;
	/// セクタサイズ（ヘッダ＋バッファのサイズ）を返す
	int		GetSize() const;
	/// セクタデータへのポインタを返す
	virtual wxUint8 *GetSectorBuffer();
	/// セクタデータへのポインタを返す
	virtual wxUint8 *GetSectorBuffer(int offset);

	/// ファイルオフセットを返す
	wxFileOffset GetFileOffset() const { return m_offset; }

	/// 変更されているか
	bool	IsModified() const;
	/// 変更済みをクリア
	void	ClearModify();

	/// セクタ番号の比較
	static int Compare(DiskPlainSector *item1, DiskPlainSector *item2);

	/// アクセス時間を更新
	virtual void UpdateAccessTime() {}
	/// アクセス時間
	virtual time_t GetAccessTime() const { return 0; }
	/// アクセス時間
	static time_t GetTimeNow();

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

#ifndef USE_SECTOR_BLOCK_CACHE

/// セクタデータへのポインタを保持するクラス（キャッシュ関連）
class DiskPlainSectorForCache : public DiskPlainSector
{
private:
	time_t m_accs_time;		///< access time	
	int m_refs_count;	///< lock in cache

public:
	DiskPlainSectorForCache();
	DiskPlainSectorForCache(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	virtual ~DiskPlainSectorForCache();

	/// セクタの情報を設定
	void Set(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	/// セクタの情報を設定(データはポインタを保持する)
	void SetPtr(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	/// セクタの情報をクリア
	void Clear();

	/// アクセス時間を更新
	void UpdateAccessTime();
	/// アクセス時間
	time_t GetAccessTime() const { return m_accs_time; }

	/// リファレンス数
	int GetRefs() const { return m_refs_count; }
	/// リファレンス数をクリア
	void ClearRefs() { m_refs_count = 0; }
	/// リファレンス数を増やす
	void IncRefs() { m_refs_count++; }
	/// リファレンス数を減らす
	void DecRefs() { m_refs_count--; }

	/// セクタ番号の比較
	static int Compare(DiskPlainSectorForCache *item1, DiskPlainSectorForCache *item2);
	/// アクセス時間の比較
	static int CompareAccessTime(DiskPlainSectorForCache *item1, DiskPlainSectorForCache *item2);
};

#else

class DiskPlainSectorBlock;

/// セクタデータへのポインタを保持するクラス（キャッシュ関連）
class DiskPlainSectorForBlock : public DiskPlainSector
{
private:
	DiskPlainSectorBlock *p_block;

public:
	DiskPlainSectorForBlock();
	DiskPlainSectorForBlock(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	virtual ~DiskPlainSectorForBlock();

	/// ブロックキャッシュを設定
	void SetSectorBlock(DiskPlainSectorBlock *block) { p_block = block; }

	/// セクタの情報を設定
	void Set(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	/// セクタの情報を設定(データはポインタを保持する)
	void SetPtr(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset);
	/// セクタの情報をクリア
	void Clear();

	/// セクタデータへのポインタを返す
	wxUint8 *GetSectorBuffer();
	/// セクタデータへのポインタを返す
	wxUint8 *GetSectorBuffer(int offset);

	/// アクセス時間を更新
	void UpdateAccessTime();
	/// アクセス時間
	time_t GetAccessTime() const;

	/// セクタ番号の比較
	static int Compare(DiskPlainSectorForBlock *item1, DiskPlainSectorForBlock *item2);
	/// アクセス時間の比較
	static int CompareAccessTime(DiskPlainSectorForBlock *item1, DiskPlainSectorForBlock *item2);
};

#endif // USE_SECTOR_BLOCK_CACHE

// ----------------------------------------------------------------------

/// １ディスクのヘッダを渡すクラス
class DiskPlainDiskHeader : public DiskImageDiskHeader
{
public:
	DiskPlainDiskHeader();
};

// ----------------------------------------------------------------------

class DiskPlainFile;
class DiskBasicParam;
class DiskBasic;
class DiskBasics;

/// １パーティションへのポインタを保持するクラス
class DiskPlainDisk : public DiskImageDisk
{
private:
	wxUint32 m_start_block;	///< 開始セクタ番号
	wxUint32 m_block_size;	///< number of sectors (セクタ数)
	wxUint32 m_disk_size;	///< in bytes

	wxString m_name;		///< パーティション名
	wxString m_desc;		///< 説明

	DiskBasics *p_basics;	///< ファイルシステム候補

	DiskPlainDisk() : DiskImageDisk() {}
	DiskPlainDisk(const DiskPlainDisk &src) : DiskImageDisk(src) {}
	DiskPlainDisk &operator=(const DiskPlainDisk &src) { return *this; }

public:
	DiskPlainDisk(DiskPlainFile *file, int n_num, wxUint32 start_block, wxUint32 block_size);
	DiskPlainDisk(DiskPlainFile *file, int n_num, const wxString &n_name);
	DiskPlainDisk(DiskPlainFile *file, int n_num, DiskImageDiskHeader &n_header);
	~DiskPlainDisk();

	/// ディスクの内容を置き換える
	int		Replace(DiskImageDisk *src_disk, int src_side_number);
	/// ディスクサイズ計算（ディスクヘッダ分を除く）
	size_t	CalcSizeWithoutHeader();

	/// ヘッダをコピー
	bool	CopyHeaderTo(DiskImageDiskHeader &dst);

	/// ディスク名を返す
	wxString GetName(bool real = false) const;
	/// ディスク名を設定
	void	SetName(const wxString &val);
	/// ディスク名を設定
	void	SetName(const wxUint8 *buf, size_t len);

	/// 指定セクタを返す
	DiskImageSector *GetSector(int sector_pos);
	/// キャッシュを更新する
	void	RefreshCache(int sector_pos);

	/// 書き込み禁止かどうかを返す
	bool IsWriteProtected() const;
	/// 書き込み禁止かどうかを設定
	void SetWriteProtect(bool val);

	/// ディスクサイズ（ヘッダサイズ含む）
	wxUint32 GetSize() const;
	/// ディスクサイズ（ヘッダサイズ含む）を設定
	void	SetSize(wxUint32 val);
	/// ディスクサイズ（ヘッダサイズを除く）
	wxUint32 GetSizeWithoutHeader() const;
	/// ディスクサイズ（ヘッダサイズを除く）を設定
	void	SetSizeWithoutHeader(wxUint32 val);
	/// ヘッダサイズを返す
	wxUint32 GetHeaderSize() const;

	/// 開始セクタ番号を返す
	wxUint32 GetStartSectorNumber() const;
	/// 開始セクタ番号を設定
	void	SetStartSectorNumber(wxUint32 val);
	/// 開始セクタ番号(文字列)を返す
	wxString GetStartSectorNumberStr(int show_type) const;
	/// セクタ数を返す
	wxUint32 GetNumberOfSectors() const;
	/// セクタ数を設定
	void	SetNumberOfSectors(wxUint32 val);
	/// セクタ数(文字列)を返す
	wxString GetNumberOfSectorsStr(int show_type) const;

	/// ディスクの内容を初期化する(0パディング)
	bool	Initialize();

	/// ディスクの説明
	wxString GetDescription() const { return m_desc; }
	/// ファイルの説明詳細
	wxString GetDescriptionDetails() const;
	/// ディスクの説明
	void	SetDescription(const wxString &desc) { m_desc = desc; }
	/// ディスクの説明
	void	SetDescription(const wxUint8 *buf, size_t len);

	/// 変更されているか
	bool	IsModified();
	/// 変更済みをクリア
	void	ClearModify();

	/// データをすべて出力	
	void	Flush();

	/// パラメータ変更フラグを設定
	void	SetParamChanged(bool val) {}
	/// パラメータ変更フラグを返す
	bool	GetParamChanged() const { return false; }

	/// DISK BASIC領域を確保
	void	AllocDiskBasics();
	/// DISK BASICを返す
	DiskBasic *GetDiskBasic(int idx);
	/// DISK BASICを返す
	DiskBasics *GetDiskBasics() { return p_basics; }
	/// DISK BASICをクリア
	void	ClearDiskBasics();
	/// キャラクターコードマップ設定
	void	SetCharCode(const wxString &name);

	// セクタ位置からトラック、サイド、セクタ番号を得る(オフセットを考慮)
	bool	GetNumberFromBlockNum(int block_num, int &track_num, int &side_num, int &sector_num) const;

	/// ディスク番号を比較
	static int Compare(DiskPlainDisk *item1, DiskPlainDisk *item2);
};

// ----------------------------------------------------------------------

#ifndef USE_SECTOR_BLOCK_CACHE

WX_DEFINE_SORTED_ARRAY(DiskPlainSectorForCache *, DiskPlainSectors);

class DiskPlainSectorCache;

/// セクタデータを一時的に保持するクラス
class DiskPlainSectorCache
{
private:
	DiskPlainFile *p_parent;
	DiskPlainSectors *p_cache;

	wxUint32 m_start_offset;	///< ファイルの開始オフセット

	size_t m_limit_count;	///< キャッシュできる最大アイテム数
	size_t m_shrink_count;	///< サイズを縮小する際のアイテム数

	int m_cache_overflowed;	///< キャッシュがいっぱいになった回数

	Utils::TempData m_temp;

	DiskPlainSectorForCache *FindBySectorPosR(int depth, int sector_pos, size_t index, size_t st, size_t ed);

	DiskPlainSectorCache() {}
	DiskPlainSectorCache(const DiskPlainSectorCache &) {}
	DiskPlainSectorCache &operator=(const DiskPlainSectorCache &) { return *this; }

public:
	DiskPlainSectorCache(DiskPlainFile *parent, wxUint32 start_offset);
	~DiskPlainSectorCache();
	/// セクタデータをキャッシュに追加する
	DiskPlainSector *AddSector(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset); 
	/// アクセスのないセクタデータをキャッシュから削除する
	void RemoveByAccessTime();
	/// セクタデータを削除する
	void RemoveSector(DiskPlainSectorForCache *item);
	/// 指定したセクタ番号のセクタデータがキャッシュにあるか
	DiskPlainSectorForCache *FindBySectorPos(int sector_pos);
	/// セクタデータを得る
	DiskImageSector *GetSector(int sector_pos);
	/// キャッシュをクリア
	void ClearCache(wxUint32 start, wxUint32 size);
	/// セクタのリファレンス数をクリア
	void ClearRefs(wxUint32 start, wxUint32 size);
	/// 変更されているか
	bool IsModified();
	/// 範囲内のセクタが変更されているか
	bool IsModified(wxUint32 start, wxUint32 size);
	/// 変更をクリア
	void ClearModify();
	/// 範囲内のセクタ変更をクリア
	void ClearModify(wxUint32 start, wxUint32 size);
	/// データをすべて出力
	void Flush();
	/// 範囲内のデータを出力
	void Flush(wxUint32 start, wxUint32 size);
	/// ステータスメッセージ
	void GetStatusMessage(wxString &str) const;
};

#else

// ----------------------------------------------------------------------

/// セクタデータをまとめたクラス
class DiskPlainSectorBlock
{
public:
	enum enFlags {
		NumOfSecs = 128
	};
private:
	DiskPlainSectorForBlock m_cache[NumOfSecs];
	int m_seq_num;	///< 通し番号
	Utils::TempData *p_data;	///< cached data in file
	wxFileOffset m_offset;	///< seek offset in file
	time_t m_accs_time;		///< access time
	int m_refs_count;	///< lock in cache
public:
	DiskPlainSectorBlock();
	DiskPlainSectorBlock(int seq_num);
	DiskPlainSectorBlock(int seq_num, DiskImageSectorHeader &header, const wxUint8 *data, size_t size, int sector_pos, size_t sector_size, wxFileOffset offset);
	~DiskPlainSectorBlock();

	/// セクタを返す
	DiskPlainSector *GetSector(int sector_pos);
	/// 通し番号を返す
	int GetNumber() const { return m_seq_num; }
	/// ファイルオフセットを返す
	wxFileOffset GetFileOffset() const { return m_offset; }
	/// バッファを返す
	wxUint8 *GetBufferData() const;
	/// バッファサイズを返す
	size_t GetBufferSize() const;
	/// 変更されているか
	bool IsModified() const;
	/// 変更をクリア
	void ClearModify();

	/// アクセス時間を更新
	void UpdateAccessTime();
	/// アクセス時間
	time_t GetAccessTime() const { return m_accs_time; }

	/// リファレンス数
	int GetRefs() const { return m_refs_count; }
	/// リファレンス数をクリア
	void ClearRefs() { m_refs_count = 0; }
	/// リファレンス数を増やす
	void IncRefs() { m_refs_count++; }
	/// リファレンス数を減らす
	void DecRefs() { m_refs_count--; }

	/// セクタ番号の比較
	static int Compare(DiskPlainSectorBlock *item1, DiskPlainSectorBlock *item2);
	/// アクセス時間の比較
	static int CompareAccessTime(DiskPlainSectorBlock *item1, DiskPlainSectorBlock *item2);
};

// ----------------------------------------------------------------------

WX_DEFINE_SORTED_ARRAY(DiskPlainSectorBlock *, DiskPlainSectorBlocks);

/// セクタデータを一時的に保持するクラス
class DiskPlainSectorBlockCache
{
private:
	DiskPlainFile *p_parent;
	DiskPlainSectorBlocks *p_cache;

	wxUint32 m_start_offset;	///< ファイルの開始オフセット

	size_t m_limit_count;	///< キャッシュできる最大アイテム数
	size_t m_shrink_count;	///< サイズを縮小する際のアイテム数

	int m_cache_overflowed;	///< キャッシュがいっぱいになった回数

	Utils::TempData m_temp;

	DiskPlainSectorBlock *FindBySectorPosR(int depth, int seq_num, size_t index, size_t st, size_t ed);

	DiskPlainSectorBlockCache() {}
	DiskPlainSectorBlockCache(const DiskPlainSectorBlockCache &) {}
	DiskPlainSectorBlockCache &operator=(const DiskPlainSectorBlockCache &) { return *this; }

public:
	DiskPlainSectorBlockCache(DiskPlainFile *parent, wxUint32 start_offset);
	~DiskPlainSectorBlockCache();
	/// セクタブロックをキャッシュに追加する
	DiskPlainSectorBlock *AddSectorBlock(int sector_pos); 
	/// アクセスのないセクタデータをキャッシュから削除する
	void RemoveByAccessTime();
	/// セクタブロックを削除する
	void RemoveSectorBlock(DiskPlainSectorBlock *item);
	/// 指定したセクタ番号のセクタデータがキャッシュにあるか
	DiskPlainSector *FindBySectorPos(int sector_pos);
	/// セクタデータを得る
	DiskImageSector *GetSector(int sector_pos);
	/// キャッシュをクリア
	void ClearCacheAll();
	/// キャッシュをクリア
	void ClearCache(int start, int size);
	/// セクタのリファレンス数をクリア
	void ClearRefs(int start, int size);
	/// 変更されているか
	bool IsModified();
	/// 範囲内のセクタが変更されているか
	bool IsModified(int start, int size);
	/// 変更をクリア
	void ClearModify();
	/// 範囲内のセクタ変更をクリア
	void ClearModify(int start, int size);
	/// データをすべて出力
	void Flush();
	/// 範囲内のデータを出力
	void Flush(int start, int size);
	/// ステータスメッセージ
	void GetStatusMessage(wxString &str) const;
};

#endif // USE_SECTOR_BLOCK_CACHE

// ----------------------------------------------------------------------

/// ディスクイメージへのポインタを保持するクラス
class DiskPlainFile : public DiskImageFile
{
private:
	wxFileStream *p_stream;		///< ディスクイメージファイル
	wxUint32 m_start_offset;	///< ファイル開始位置までのオフセット in bytes
	wxUint32 m_block_size;		///< ディスクイメージブロック数
	bool m_write_protected;		///< 書き込み禁止フラグ
	DiskImageDisks *p_disks;	///< パーティション情報
#ifdef USE_SECTOR_BLOCK_CACHE
	DiskPlainSectorBlockCache *p_cache;	///< セクタブロックキャッシュ
#else
	DiskPlainSectorCache *p_cache;	///< セクタキャッシュ
#endif
	wxString m_desc;			///< 説明

	DiskPlainFile(const DiskPlainFile &src) : DiskImageFile() {}

public:
	DiskPlainFile();
	~DiskPlainFile();

	/// インスタンス作成
	DiskImageDisk *NewImageDisk(int disk_number, wxUint32 start_block, wxUint32 block_size);
	/// インスタンス作成
	DiskImageDisk *NewImageDisk(int disk_number, DiskImageDiskHeader &n_header);

	void SetFileStream(wxFileStream *stream);
	wxFileStream *GetFileStream();

	size_t Add(DiskImageDisk *newdisk, short mod_flags);
	void Clear();
	size_t Count() const;
	bool Delete(size_t idx);

	void ClearDisks();
	DiskImageDisks *GetDisks() { return p_disks; }
	const DiskImageDisks *GetDisks() const { return p_disks; }
	DiskImageDisk  *GetDisk(size_t idx);

	/// ファイル開始位置までのオフセット in bytes
	void SetStartOffset(wxUint32 val) { m_start_offset = val; }
	/// ファイル開始位置までのオフセット in bytes
	wxUint32 GetStartOffset() const { return m_start_offset; }

	/// ディスクイメージブロック数
	void SetBlockSize(wxUint32 val) { m_block_size = val; }
	/// ディスクイメージブロック数
	wxUint32 GetBlockSize() const { return m_block_size; }

	/// 指定セクタを返す
	DiskImageSector *GetSector(int sector_pos);
	/// キャッシュを更新する
	void RefreshCache(int sector_pos);
	/// キャッシュをクリア ファイル更新もしない
	void ClearCacheAll();
	/// キャッシュをクリアする
	void ClearCache(wxUint32 start, wxUint32 size);
	/// キャッシュのリファレンス数ををクリアする
	void ClearCacheRefs(wxUint32 start, wxUint32 size);

	/// 変更されているか
	bool IsModified();
	/// 変更をクリア
	void ClearModify();
	/// 範囲内のセクタが変更されているか
	bool IsModified(wxUint32 start, wxUint32 size);
	/// 範囲内のセクタ変更をクリア
	void ClearModify(wxUint32 start, wxUint32 size);

	/// データをすべて出力	
	void Flush();
	/// 範囲内のデータを出力
	void Flush(wxUint32 start, wxUint32 size);

	/// 書き込み禁止かどうかを返す
	bool IsWriteProtected() const;
	/// 書き込み禁止かどうかを設定
	void SetWriteProtect(bool val);

	/// ファイルストリームを返す
	wxFileStream *GetStream() { return p_stream; }

	/// ファイルの説明
	wxString GetDescription() const { return m_desc; }
	/// ファイルの説明詳細
	wxString GetDescriptionDetails() const;
	/// ファイルの説明
	void	SetDescription(const wxString &desc);

	/// ステータスメッセージ
	void GetStatusMessage(wxString &str) const;
};

/// ベタディスクイメージ入出力
class DiskPlain : public DiskImage
{
private:
//	void NewFile(const wxString &newpath);
//	void ClearFile();

#ifdef DISKPLAIN_USE_MEMORY_INPUT_STREAM
	wxMemoryInputStream *stream;
	bool OpenStream(wxInputStream &src);
	void CloseStream();
#endif

public:
	DiskPlain();
	~DiskPlain();

	/// インスタンス作成
	DiskImageFile *NewImageFile();

	/// 新規作成
	int Create(const wxString &diskname, const DiskParam &param, bool write_protect, const wxString &basic_hint);
	/// ファイルを開く
	int Open(const wxString &filepath, const wxString &file_format, const DiskParam &param_hint);
	/// ファイルを開く前のチェック
	int Check(const wxString &filepath, wxString &file_format, DiskParamPtrs &params, DiskParam &manual_param);
	/// 既に開いているファイルを開きなおす
	int ReOpen(const BootParam &boot_param);
	/// 閉じる
	void Close();
	/// ファイルを開いているか
	bool IsOpened() const;
	/// ストリームの内容をファイルに保存できるか
	int CanSave(const wxString &filepath, const wxString &file_format);
	/// ストリームの内容をファイルに保存
	int Save(const wxString &filepath, const wxString &file_format, const DiskWriteOptions &options);
	/// ストリームの内容をファイルに保存
	int SaveDisk(int disk_number, const wxString &filepath, const wxString &file_format, const DiskWriteOptions &options);
	/// ディスクを削除
	bool Delete(int disk_number);
	/// 置換元のディスクを解析
	int ParseForReplace(int disk_number, const wxString &filepath, const wxString &file_format, const DiskParam &param_hint, DiskImageFile &src_file, DiskImageDisk* &tag_disk);
	/// ファイルでディスクを置換
	int ReplaceDisk(int disk_number, DiskImageDisk *src_disk, int src_side_number, DiskImageDisk *tag_disk);

	/// DISK BASICが一致するか
	bool MatchDiskBasic(const DiskBasic *target);
	/// DISK BASICの解析状態をクリア
	void ClearDiskBasicParseAndAssign(int disk_number, int side_number);
	/// キャラクターコードマップ設定
	void SetCharCode(const wxString &name);
};

#endif /* DISKPLAIN_H */
