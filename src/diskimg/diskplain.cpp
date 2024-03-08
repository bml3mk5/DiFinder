/// @file diskplain.cpp
///
/// @brief ベタディスクイメージ入出力
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskplain.h"
#include <wx/wfstream.h>
#include <wx/numformatter.h>
#include <wx/xml/xml.h>
#include "diskparser.h"
#include "diskwriter.h"
#include "diskplaincreator.h"
#include "../basicfmt/basicparam.h"
#include "../basicfmt/basicfmt.h"
#include "../config.h"


#define DISK_IMAGE_HEADER_KIND 2

// ----------------------------------------------------------------------
//
//

DiskPlainSector::DiskPlainSector()
	: DiskImageSector(-1)
{
	m_data = NULL;
	m_own_make = false;
	m_size = 0;
	m_offset = 0;

	m_crc_32 = 0xffffffff;
}

/// ファイルから読み込み用
DiskPlainSector::DiskPlainSector(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
	: DiskImageSector(n_num)
{
	m_data = new wxUint8[n_size];
	m_own_make = true;
	m_size = n_size;
	if (n_data) {
		memcpy(m_data, n_data, n_size);
	} else {
		memset(m_data, 0, n_size);
	}
	m_offset = n_offset;

	m_crc_32 = Utils::CRC32(m_data, m_size);
}

DiskPlainSector::~DiskPlainSector()
{
	if (m_own_make) {
		delete [] m_data;
	}
}

/// セクタの情報をセット
void DiskPlainSector::Set(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
{
	SetNumber(n_num);

	if (m_data) {
		if (m_size < n_size) {
			if (m_own_make) {
				delete [] m_data;
			}
			m_data = new wxUint8[n_size];
		}
	} else {
		m_data = new wxUint8[n_size];
	}
	m_own_make = true;
	m_size = n_size;
	memcpy(m_data, n_data, n_size);
	m_offset = n_offset;

	m_crc_32 = Utils::CRC32(m_data, m_size);
}
/// セクタの情報を設定(データはポインタを保持する)
void DiskPlainSector::SetPtr(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
{
	SetNumber(n_num);

	if (m_data) {
		if (m_own_make) {
			delete [] m_data;
		}
	}
	m_data = n_data;
	m_own_make = false;
	m_size = n_size;
	m_offset = n_offset;

	m_crc_32 = Utils::CRC32(m_data, m_size);
}
/// セクタの情報をクリア
void DiskPlainSector::Clear()
{
	SetNumber(-1);

	m_offset = 0;
	m_crc_32 = 0xffffffff;
}

/// セクタのデータを置き換える
/// セクタサイズは変更しない
bool DiskPlainSector::Replace(DiskImageSector *src_sector)
{
	if (!m_data) {
		return false;
	}
	wxUint8 *src_data = src_sector->GetSectorBuffer();
	if (!src_data) {
		// データなし
		return false;
	}
	size_t sz = src_sector->GetSectorBufferSize() > GetSectorBufferSize() ? GetSectorBufferSize() : src_sector->GetSectorBufferSize();
	if (sz > 0) {
		memset(m_data, 0, m_size);
		memcpy(m_data, src_data, sz);
	}
	return true;
}

/// セクタのデータを埋める
/// @param[in] code : コード
/// @param[in] len : 長さ
/// @param[in] start : 開始位置
/// @return false : 失敗
bool DiskPlainSector::Fill(wxUint8 code, int len, int start)
{
	if (!m_data) {
		return false;
	}
	if (start < 0) {
		start = m_size - start;
	}
	if (start < 0 || start >= m_size) {
		return false;
	}

	if (len < 0) len = m_size - start;
	else if ((start + len) > m_size) len = m_size - start;

	memset(&m_data[start], code, len);

	return true;
}

/// セクタのデータを上書き
/// @param[in] buf : データ
/// @param[in] len : 長さ
/// @param[in] start : 開始位置
/// @return false : 失敗
bool DiskPlainSector::Copy(const void *buf, int len, int start)
{
	if (!m_data) {
		return false;
	}
	if (start < 0) {
		start = m_size - start;
	}
	if (len < 0 || start < 0 || start >= m_size) {
		return false;
	}

	if ((start + len) > m_size) len = m_size - start;

	memcpy(&m_data[start], buf, len);

	return true;
}

/// セクタのデータに指定したバイト列があるか
/// @param[in] buf : データ
/// @param[in] len : 長さ
/// @return -1:なし >=0:あり・その位置
int DiskPlainSector::Find(const void *buf, size_t len)
{
	if (!m_data) {
		return -1;
	}
	int match = -1;
	for(int pos = 0; pos < (m_size - (int)len); pos++) { 
		if (memcmp(&m_data[pos], buf, len) == 0) {
			match = pos;
			break;
		}
	}
	return match;
}

/// 指定位置のセクタデータを返す
/// @param[in] pos : バッファ内の位置　負を指定すると末尾からの位置となる
/// @return 値
wxUint8	DiskPlainSector::Get(int pos) const
{
	if (!m_data) {
		return 0;
	}
	if (pos < 0) {
		pos += m_size;
	}
	return m_data[pos];
}

/// 指定位置のセクタデータを返す
/// @param[in] pos : バッファ内の位置　負を指定すると末尾からの位置となる
/// @param[in] big_endian : ビッグエンディアンか
/// @return 値
wxUint16 DiskPlainSector::Get16(int pos, bool big_endian) const
{
	if (!m_data) {
		return 0;
	}
	if (pos < 0) {
		pos += m_size;
	}
	return big_endian ? ((wxUint16)m_data[pos] << 8 | m_data[pos+1]) : ((wxUint16)m_data[pos+1] << 8 | m_data[pos]);
}

/// 指定位置にセクタデータを設定
/// @param[in] pos : バッファ内の位置　負を指定すると末尾からの位置となる
/// @param[in] val : 値
void DiskPlainSector::Set(int pos, wxUint8 val)
{
	if (!m_data) {
		return;
	}
	if (pos < 0) {
		pos += m_size;
	}
	m_data[pos] = val;
}

/// 指定位置のセクタデータを設定
void DiskPlainSector::Set16(int pos, wxUint16 val, bool big_endian)
{
	if (!m_data) {
		return;
	}
	if (pos < 0) {
		pos += m_size;
	}
	if (big_endian) {
		m_data[pos] = (val >> 8) & 0xff;
		m_data[pos+1] = (val & 0xff);
	} else {
		m_data[pos] = (val & 0xff);
		m_data[pos+1] = (val >> 8) & 0xff;
	}
}

/// 変更されているか
bool DiskPlainSector::IsModified() const
{
	wxUint32 crc = Utils::CRC32(m_data, m_size);

	return (m_crc_32 != crc);
}

/// 変更済みをクリア
void DiskPlainSector::ClearModify()
{
	wxUint32 crc = Utils::CRC32(m_data, m_size);
	m_crc_32 = crc;
}

/// 同じセクタか
bool DiskPlainSector::IsSameSector(int sector_number, bool deleted_mark)
{
	return (sector_number == m_num);
}

/// セクタサイズ
int DiskPlainSector::GetSectorSize() const
{
	return m_size;
}
void DiskPlainSector::SetSectorSize(int val)
{
	m_size = val;
}

/// セクタサイズ（バッファのサイズ）を返す
int DiskPlainSector::GetSectorBufferSize() const
{
	return m_size;
}

/// セクタサイズ（ヘッダ＋バッファのサイズ）を返す
int DiskPlainSector::GetSize() const
{
	return GetSectorBufferSize();
}

/// セクタデータへのポインタを返す
wxUint8 *DiskPlainSector::GetSectorBuffer()
{
	return m_data;
}

/// セクタデータへのポインタを返す
wxUint8 *DiskPlainSector::GetSectorBuffer(int offset)
{
	return &m_data[offset];
}

int DiskPlainSector::Compare(DiskPlainSector *item1, DiskPlainSector *item2)
{
 	return (item1->m_num - item2->m_num);
}

/// アクセス時間
time_t DiskPlainSector::GetTimeNow()
{
	return wxDateTime::GetTimeNow();
}

// ----------------------------------------------------------------------

#ifndef USE_SECTOR_BLOCK_CACHE

DiskPlainSectorForCache::DiskPlainSectorForCache()
	: DiskPlainSector()
{
	m_refs_count = 0;
	UpdateAccessTime();
}

DiskPlainSectorForCache::DiskPlainSectorForCache(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
	: DiskPlainSector(n_num, n_header, n_data, n_size, n_offset)
{
	m_refs_count = 0;
	UpdateAccessTime();
}

DiskPlainSectorForCache::~DiskPlainSectorForCache()
{
}

/// セクタの情報をセット
void DiskPlainSectorForCache::Set(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
{
	DiskPlainSector::Set(n_num, n_header, n_data, n_size, n_offset);
	m_refs_count = 0;
	UpdateAccessTime();
}
/// セクタの情報を設定(データはポインタを保持する)
void DiskPlainSectorForCache::SetPtr(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
{
	DiskPlainSector::SetPtr(n_num, n_header, n_data, n_size, n_offset);
	m_refs_count = 0;
	UpdateAccessTime();
}
/// セクタの情報をクリア
void DiskPlainSectorForCache::Clear()
{
	DiskPlainSector::Clear();
	m_refs_count = 0;
}
/// アクセス時間を更新
void DiskPlainSectorForCache::UpdateAccessTime()
{
	m_accs_time = GetTimeNow();
}

int DiskPlainSectorForCache::Compare(DiskPlainSectorForCache *item1, DiskPlainSectorForCache *item2)
{
 	return (item1->m_num - item2->m_num);
}

int DiskPlainSectorForCache::CompareAccessTime(DiskPlainSectorForCache *item1, DiskPlainSectorForCache *item2)
{
    return (item1->m_accs_time - item2->m_accs_time) || (item2->m_num - item1->m_num);
}

#else

DiskPlainSectorForBlock::DiskPlainSectorForBlock()
	: DiskPlainSector()
{
	p_block = NULL;
}

DiskPlainSectorForBlock::DiskPlainSectorForBlock(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
	: DiskPlainSector(n_num, n_header, n_data, n_size, n_offset)
{
	p_block = NULL;
}

DiskPlainSectorForBlock::~DiskPlainSectorForBlock()
{
}

/// セクタの情報をセット
void DiskPlainSectorForBlock::Set(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
{
	DiskPlainSector::Set(n_num, n_header, n_data, n_size, n_offset);
	UpdateAccessTime();
}
/// セクタの情報を設定(データはポインタを保持する)
void DiskPlainSectorForBlock::SetPtr(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
{
	DiskPlainSector::SetPtr(n_num, n_header, n_data, n_size, n_offset);
	UpdateAccessTime();
}
/// セクタの情報をクリア
void DiskPlainSectorForBlock::Clear()
{
	DiskPlainSector::Clear();
}
/// セクタデータへのポインタを返す
wxUint8 *DiskPlainSectorForBlock::GetSectorBuffer()
{
	UpdateAccessTime();
	return DiskPlainSector::GetSectorBuffer();
}
/// セクタデータへのポインタを返す
wxUint8 *DiskPlainSectorForBlock::GetSectorBuffer(int offset)
{
	UpdateAccessTime();
	return DiskPlainSector::GetSectorBuffer(offset);
}
/// アクセス時間を更新
void DiskPlainSectorForBlock::UpdateAccessTime()
{
	if (!p_block) return;

	p_block->UpdateAccessTime();
}
/// アクセス時間
time_t DiskPlainSectorForBlock::GetAccessTime() const
{
	return p_block ? p_block->GetAccessTime() : 0;
}

int DiskPlainSectorForBlock::Compare(DiskPlainSectorForBlock *item1, DiskPlainSectorForBlock *item2)
{
 	return (item1->m_num - item2->m_num);
}

int DiskPlainSectorForBlock::CompareAccessTime(DiskPlainSectorForBlock *item1, DiskPlainSectorForBlock *item2)
{
    return (item1->GetAccessTime() - item2->GetAccessTime()) || (item2->m_num - item1->m_num);
}

#endif // USE_SECTOR_BLOCK_CACHE

// ----------------------------------------------------------------------
//
//
//
DiskPlainDiskHeader::DiskPlainDiskHeader()
	: DiskImageDiskHeader(DISK_IMAGE_HEADER_KIND)
{
}

// ----------------------------------------------------------------------
//
//
//
DiskPlainDisk::DiskPlainDisk(DiskPlainFile *file, int n_num, wxUint32 start_block, wxUint32 block_size)
	: DiskImageDisk(file, n_num)
{
	p_basics = new DiskBasics;
	m_start_block = start_block;
	m_block_size = block_size;
	m_disk_size = 0;
}

DiskPlainDisk::DiskPlainDisk(DiskPlainFile *file, int n_num, const wxString &n_name)
	: DiskImageDisk(file, n_num)
{
	p_basics = new DiskBasics;
	m_start_block = 0;
	m_block_size = 0;
	m_disk_size = 0;
}

/// @note n_header はnewで確保しておくこと
DiskPlainDisk::DiskPlainDisk(DiskPlainFile *file, int n_num, DiskImageDiskHeader &n_header)
	: DiskImageDisk(file, n_num)
{
	p_basics = new DiskBasics;
	m_start_block = 0;
	m_block_size = 0;
	m_disk_size = 0;
}

DiskPlainDisk::~DiskPlainDisk()
{
	delete p_basics;
}

/// ディスクの内容を置き換える
/// @param [in] src_disk : 置換元のディスクイメージ
/// @param [in] src_side_number : 置換元のディスクイメージのサイド番号
int DiskPlainDisk::Replace(DiskImageDisk *src_disk, int src_side_number)
{
	return -1;
}

/// ディスクサイズ計算（ディスクヘッダ分を除く）
size_t DiskPlainDisk::CalcSizeWithoutHeader()
{
	size_t new_size = 0;
	return new_size;
}

/// ヘッダをコピー
bool DiskPlainDisk::CopyHeaderTo(DiskImageDiskHeader &dst)
{
	return false;
}

/// 変更済みをクリア
void DiskPlainDisk::ClearModify()
{
	// このパーティション内でキャッシュを検索
	((DiskPlainFile *)parent)->ClearModify(m_start_block, m_block_size);
}

/// 変更されているか
bool DiskPlainDisk::IsModified()
{
	// このパーティション内でキャッシュを検索
	return ((DiskPlainFile *)parent)->IsModified(m_start_block, m_block_size);
}

/// データをすべて出力
void DiskPlainDisk::Flush()
{
	// このパーティション内でキャッシュを検索しデータをすべて出力
	((DiskPlainFile *)parent)->Flush(m_start_block, m_block_size);
}

/// ディスク名を返す
/// @param[in] real 名称が空白の時そのまま返すか
/// @return ディスク名
wxString DiskPlainDisk::GetName(bool real) const
{
	return m_name;
}

/// ディスク名を設定
/// @param[in] val ディスク名
void DiskPlainDisk::SetName(const wxString &val)
{
	m_name = val;
}

/// ディスク名を設定
/// @param[in] buf ディスク名
/// @param[in] len 長さ
void DiskPlainDisk::SetName(const wxUint8 *buf, size_t len)
{
	m_name = wxString(buf, len);
}

/// 指定セクタを返す
/// @param[in] block_num パーティション内でのセクタ位置
/// @return セクタ
DiskImageSector *DiskPlainDisk::GetSector(int block_num)
{
	if (!parent) return NULL;

	if ((wxUint32)block_num >= m_block_size) {
		// out of range
		return NULL;
	}

	int pos = m_start_block + block_num;

	return parent->GetSector(pos);
}

/// キャッシュを更新する
void DiskPlainDisk::RefreshCache(int block_num)
{
	if (!parent) return;

	if ((wxUint32)block_num >= m_block_size) {
		// out of range
		return;
	}

	int pos = m_start_block + block_num;

	parent->RefreshCache(pos);
}

/// 書き込み禁止かどうかを返す
/// @return true:書き込み禁止
bool DiskPlainDisk::IsWriteProtected() const
{
	return parent->IsWriteProtected();
}

/// 書き込み禁止かどうかを設定
/// @param[in] val 書き込み禁止ならtrue
void DiskPlainDisk::SetWriteProtect(bool val)
{
	parent->SetWriteProtect(val);
}

/// ディスクサイズ（ヘッダサイズ含む）
wxUint32 DiskPlainDisk::GetSize() const
{
	return m_disk_size;
}

/// ディスクサイズ（ヘッダサイズ含む）を設定
/// @param [in] val サイズ（ヘッダサイズ含む）
void DiskPlainDisk::SetSize(wxUint32 val)
{
	m_disk_size = val;
}

/// ディスクサイズ（ヘッダサイズを除く）
wxUint32 DiskPlainDisk::GetSizeWithoutHeader() const
{
	return m_disk_size;
}

/// @param [in] val サイズ（ヘッダサイズを除く）を設定
void DiskPlainDisk::SetSizeWithoutHeader(wxUint32 val)
{
	m_disk_size = val;
}

/// ヘッダサイズを返す
wxUint32 DiskPlainDisk::GetHeaderSize() const
{
	return 0;
}

/// 開始セクタ番号を返す
wxUint32 DiskPlainDisk::GetStartSectorNumber() const
{
	return m_start_block;
}

/// 開始セクタ番号を設定
void DiskPlainDisk::SetStartSectorNumber(wxUint32 val)
{
	m_start_block = val;
}

/// 開始セクタ番号(文字列)を返す
wxString DiskPlainDisk::GetStartSectorNumberStr(int show_type) const
{
	wxString str;
	int trk_num, sid_num, sec_num;
	switch(show_type) {
	case 1:
		parent->GetNumberFromSectorPos(m_start_block, trk_num, sid_num, sec_num);
		str = wxString::Format(_("(C:%d H:%d R:%d)"), trk_num, sid_num, sec_num); 
		break;
	default:
		str = wxNumberFormatter::ToString((long)m_start_block);
		break;
	}
	return str;
}

/// セクタ数を返す
wxUint32 DiskPlainDisk::GetNumberOfSectors() const
{
	return m_block_size;
}

/// セクタ数を設定
void DiskPlainDisk::SetNumberOfSectors(wxUint32 val)
{
	m_block_size = val;
}

/// セクタ数(文字列)を返す
wxString DiskPlainDisk::GetNumberOfSectorsStr(int show_type) const
{
	wxString str;
	long size;
	switch(show_type) {
	case 1:
		size = (long)m_block_size * GetSectorSize();
		break;
	default:
		size = (long)m_block_size;
		break;
	}
	str = wxNumberFormatter::ToString(size);
	return str;
}

/// ディスクの内容を初期化する(0パディング)
bool DiskPlainDisk::Initialize()
{
	bool rc = true;
	GetFile()->ClearCacheRefs(m_start_block, m_block_size);
	for(int block_num = 0; block_num < (int)m_block_size && rc; block_num++) {
		DiskImageSector *sector = GetSector(block_num);
		if (!sector) {
			rc = false;
			break;
		}
		rc = sector->Fill(0);
	}
	return rc;
}

/// ディスクの説明詳細
wxString DiskPlainDisk::GetDescriptionDetails() const
{
	wxString str = m_desc;

	str += wxT(" ");
	str += _("Start Sector:");
	str += wxT(" ");
	str += GetStartSectorNumberStr(0);
	str += wxT(" ");
	str += GetStartSectorNumberStr(1);
	str += wxT(" ");
	str += _("Number of Sectors:");
	str += wxT(" ");
	str += GetNumberOfSectorsStr(0);
	str += wxT(" (");
	str += GetNumberOfSectorsStr(1);
	str += _("bytes");
	str += wxT(")");

	return str;
}

/// ディスクの説明
void DiskPlainDisk::SetDescription(const wxUint8 *buf, size_t len)
{
	m_desc = wxString(buf, len);
}

/// ディスク番号を比較
int DiskPlainDisk::Compare(DiskPlainDisk *item1, DiskPlainDisk *item2)
{
    return (item1->m_num - item2->m_num);
}

/// DISK BASIC領域を確保
void DiskPlainDisk::AllocDiskBasics()
{
	p_basics->Add();
}

/// DISK BASICを返す
DiskBasic *DiskPlainDisk::GetDiskBasic(int idx)
{
	if (idx < 0) idx = 0;
	return p_basics->Item(idx);
}

/// DISK BASICをクリア
void DiskPlainDisk::ClearDiskBasics()
{
	if (!p_basics) return;
	for(size_t idx=0; idx<p_basics->Count(); idx++) {
		DiskBasic *basic = GetDiskBasic((int)idx);
		if (!basic) continue;

		basic->ClearParseAndAssign();
	}
}

/// キャラクターコードマップ番号設定
void DiskPlainDisk::SetCharCode(const wxString &name)
{
	if (!p_basics) return;
	for(size_t idx=0; idx<p_basics->Count(); idx++) {
		DiskBasic *basic = GetDiskBasic((int)idx);
		if (!basic) continue;

		basic->SetCharCode(name);
	}
}

// セクタ位置からトラック、サイド、セクタ番号を得る(オフセットを考慮)
void DiskPlainDisk::GetNumberFromBlockNum(int block_num, int &track_num, int &side_num, int &sector_num) const
{
	parent->GetNumberFromSectorPos(m_start_block + block_num, track_num, side_num, sector_num);
}

// ----------------------------------------------------------------------

#ifndef USE_SECTOR_BLOCK_CACHE

DiskPlainSectorCache::DiskPlainSectorCache(DiskPlainFile *parent, wxUint32 start_offset)
{
	p_parent = parent;
	m_start_offset = start_offset;
	p_cache = new DiskPlainSectors(DiskPlainSectorForCache::Compare);

	size_t sector_size = parent->GetSectorSize();
	if (sector_size == 0) sector_size = 256; 
	m_limit_count = (size_t)gConfig.GetCacheLimitSize() * 1024 * 1024 / sector_size;
	m_shrink_count = (size_t)gConfig.GetCacheShrinkSize() * 1024 * 1024 / sector_size;
	m_cache_overflowed = 0;
}
DiskPlainSectorCache::~DiskPlainSectorCache()
{
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
		delete itm;
	}
	delete p_cache;
}
/// セクタデータをキャッシュに追加する
DiskPlainSector *DiskPlainSectorCache::AddSector(int n_num, DiskImageSectorHeader &n_header, wxUint8 *n_data, int n_size, wxFileOffset n_offset)
{
	DiskPlainSectorForCache *sector = new DiskPlainSectorForCache(n_num, n_header, n_data, n_size, n_offset);
	if (p_cache->Count() >= m_limit_count) {
		// メッセージ
		if (!m_cache_overflowed) {
			wxMessageBox(_("The data cache is reached the limited size. The modified data will be overwritten to the disk image. Are you sure?"),
				_("Data Cache Overflow"), wxOK);
		}
		m_cache_overflowed++;
		// 古いキャッシュを消す
		RemoveByAccessTime();
	}

	p_cache->Add(sector);

	return sector;
}
/// アクセスのないセクタデータをキャッシュから削除する
void DiskPlainSectorCache::RemoveByAccessTime()
{
	DiskPlainSectors temp(DiskPlainSectorForCache::CompareAccessTime);

	size_t limit = p_cache->Count();
	size_t i = 0;
	for(; i<limit; i++) {
		temp.Add(p_cache->Item(i));
	}
	if (limit > 100) {
		limit -= 100;
	}

	i = 0;
	while (p_cache->Count() > m_shrink_count && i < limit) {
		DiskPlainSectorForCache *itm = temp.Item(i);
		i++;

		if (itm->GetRefs() > 0) continue;

		RemoveSector(itm);
	}
}
/// セクタデータを削除する
void DiskPlainSectorCache::RemoveSector(DiskPlainSectorForCache *item)
{
	if (!item) return;

	// データが更新されている場合はファイルにライト
	if (item->IsModified()) {
		wxFileStream *stream = p_parent->GetStream();

		stream->SeekO(item->GetFileOffset(), wxFromStart);
		stream->Write(item->GetSectorBuffer(), item->GetSectorSize()).LastWrite();
	}

	// キャッシュから消す
	p_cache->Remove(item);
	delete item;
}
/// 指定したセクタ番号のセクタデータがキャッシュにあるか
/// @param[in] sector_pos : セクタ通し番号
/// @return セクタデータ / NULL
DiskPlainSectorForCache *DiskPlainSectorCache::FindBySectorPos(int sector_pos)
{
	size_t cnt = p_cache->Count();
	if (cnt == 0) {
		return NULL;
	}
	size_t index = cnt / 2;
	return FindBySectorPosR(0, sector_pos, index, 0, cnt - 1);
}
/// 指定したセクタ番号のセクタデータがキャッシュにあるか（再帰）
DiskPlainSectorForCache *DiskPlainSectorCache::FindBySectorPosR(int depth, int sector_pos, size_t index, size_t st, size_t ed)
{
	if (depth > 64) {
		return NULL;
	}

	DiskPlainSectorForCache *itm = p_cache->Item(index);
	int num = itm->GetNumber();
	if (num > sector_pos) {
		// not match
		if (index == st) {
			return NULL;
		} else {
			ed = index - 1;
			index = (ed + 1 - st) / 2 + st;
			return FindBySectorPosR(depth + 1, sector_pos, index, st, ed);
		}
	} else if (num < sector_pos) {
		// not match
		if (index == ed) {
			return NULL;
		} else {
			st = index + 1;
			index = (ed + 1 - st) / 2 + st;
			return FindBySectorPosR(depth + 1, sector_pos, index, st, ed);
		}
	} else {
		// match
		return itm;

	}
}
/// セクタデータを得る
DiskImageSector *DiskPlainSectorCache::GetSector(int sector_pos)
{
	// ファイルから指定位置をリードする
	DiskImageSector *sector = FindBySectorPos(sector_pos);
	if (sector) {
		return sector;
	}
	// キャッシュにないときはファイルからリードする
	DiskPlainSectorHeader header;
	size_t size = p_parent->GetSectorSize();

	if (size > m_temp.GetBufferSize()) {
		// expand buffer
		m_temp.SetSize(size);
	}

	wxFileStream *stream = p_parent->GetStream();
	wxFileOffset offset = sector_pos * size + m_start_offset; 
	stream->SeekI(offset, wxFromStart);
	size_t len = stream->Read(m_temp.GetData(), size).LastRead();
	if (len == size) {
		sector = AddSector(sector_pos, header, m_temp.GetData(), (int)size, offset);
	} else {
		sector = AddSector(sector_pos, header, NULL, (int)size, offset);
	}

	return sector;
}
/// キャッシュをクリア
void DiskPlainSectorCache::ClearCache(wxUint32 start, wxUint32 size)
{
	wxUint32 end = start + size;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
		wxUint32 num = (wxUint32)itm->GetNumber();
		if (start > num || num >= end) continue;

		RemoveSector(itm);
	}
	m_cache_overflowed = 0;
}
/// セクタのリファレンス数をクリア
void DiskPlainSectorCache::ClearRefs(wxUint32 start, wxUint32 size)
{
	wxUint32 end = start + size;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
		wxUint32 num = (wxUint32)itm->GetNumber();
		if (start > num || num >= end) continue;

		itm->ClearRefs();
	}
}
/// 変更されているか
bool DiskPlainSectorCache::IsModified()
{
	bool modified = false;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
		if (itm->IsModified()) {
			modified = true;
			break;
		}
	}
	return modified;
}
/// 変更されているか
bool DiskPlainSectorCache::IsModified(wxUint32 start, wxUint32 size)
{
	wxUint32 end = start + size;
	bool modified = false;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
		wxUint32 num = (wxUint32)itm->GetNumber();
		if (start > num || num >= end) continue; 
		if (itm->IsModified()) {
			modified = true;
			break;
		}
	}
	return modified;
}
/// 変更をクリア
void DiskPlainSectorCache::ClearModify()
{
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
		itm->ClearModify();
	}
}
/// 変更をクリア
void DiskPlainSectorCache::ClearModify(wxUint32 start, wxUint32 size)
{
	wxUint32 end = start + size;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
		wxUint32 num = (wxUint32)itm->GetNumber();
		if (start > num || num >= end) continue; 
		itm->ClearModify();
	}
}
/// データをすべて出力
void DiskPlainSectorCache::Flush()
{
	if (p_parent->IsWriteProtected()) return;

	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
//		wxUint32 num = (wxUint32)itm->GetNumber();

		// データが更新されている場合はファイルにライト
		if (itm->IsModified()) {
			wxFileStream *stream = p_parent->GetStream();

			stream->SeekO(itm->GetFileOffset(), wxFromStart);
			stream->Write(itm->GetSectorBuffer(), itm->GetSectorSize()).LastWrite();

			itm->ClearModify();
		}
	}
}
/// 範囲内のデータを出力
void DiskPlainSectorCache::Flush(wxUint32 start, wxUint32 size)
{
	if (p_parent->IsWriteProtected()) return;

	wxUint32 end = start + size;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorForCache *itm = p_cache->Item(i);
		wxUint32 num = (wxUint32)itm->GetNumber();
		if (start > num || num >= end) continue; 

		// データが更新されている場合はファイルにライト
		if (itm->IsModified()) {
			wxFileStream *stream = p_parent->GetStream();

			stream->SeekO(itm->GetFileOffset(), wxFromStart);
			stream->Write(itm->GetSectorBuffer(), itm->GetSectorSize()).LastWrite();

			itm->ClearModify();
		}
	}
}
/// ステータスメッセージ
void DiskPlainSectorCache::GetStatusMessage(wxString &str) const
{
	str = wxString::Format(wxT("Cached: %d/%d"), (int)p_cache->Count(), (int)m_limit_count);
}

#else

// ----------------------------------------------------------------------

DiskPlainSectorBlock::DiskPlainSectorBlock()
{
	m_seq_num = 0;
	p_data = NULL;
	m_offset = 0;
	UpdateAccessTime();
	m_refs_count = 0;
}
DiskPlainSectorBlock::DiskPlainSectorBlock(int seq_num)
{
	m_seq_num = seq_num;
	p_data = NULL;
	m_offset = 0;
	UpdateAccessTime();
	m_refs_count = 0;
}
DiskPlainSectorBlock::DiskPlainSectorBlock(int seq_num, DiskImageSectorHeader &header, const wxUint8 *data, size_t size, int sector_pos, size_t sector_size, wxFileOffset offset)
{
	m_seq_num = seq_num;
	p_data = new Utils::TempData(size);
	p_data->SetData(data, size);
	m_offset = offset;
	UpdateAccessTime();
	m_refs_count = 0;

	// 各セクタへのポインタを設定
	size_t pos = 0;
	for(int i=0; i<NumOfSecs; i++) {
		if (pos < size) {
			m_cache[i].SetPtr(sector_pos, header, p_data->GetData(pos), (int)sector_size, offset);
		} else {
			// end of file
			m_cache[i].SetPtr(sector_pos, header, NULL, 0, 0);
		}
		m_cache[i].SetSectorBlock(this);
		pos += sector_size;
		offset += sector_size;
		sector_pos++;
	}
}
DiskPlainSectorBlock::~DiskPlainSectorBlock()
{
	delete p_data;
}
/// セクタを返す
DiskPlainSector *DiskPlainSectorBlock::GetSector(int sector_pos)
{
	return &m_cache[sector_pos % NumOfSecs];
}
/// バッファを返す
wxUint8 *DiskPlainSectorBlock::GetBufferData() const
{
	return p_data ? p_data->GetData() : NULL;
}
/// バッファサイズを返す
size_t DiskPlainSectorBlock::GetBufferSize() const
{
	return p_data ? p_data->GetSize() : 0;
}
/// 変更されているか
bool DiskPlainSectorBlock::IsModified() const
{
	bool modified = false;
	for(int i=0; !modified && i<NumOfSecs; i++) {
		modified |= m_cache[i].IsModified();
	}
	return modified;
}
/// 変更をクリア
void DiskPlainSectorBlock::ClearModify()
{
	for(int i=0; i<NumOfSecs; i++) {
		m_cache[i].ClearModify();
	}
}
/// アクセス時間を更新
void DiskPlainSectorBlock::UpdateAccessTime()
{
	m_accs_time = wxDateTime::GetTimeNow();
}

/// 番号を比較
int DiskPlainSectorBlock::Compare(DiskPlainSectorBlock *item1, DiskPlainSectorBlock *item2)
{
 	return (item1->m_seq_num - item2->m_seq_num);
}
/// アクセス日時を比較
int DiskPlainSectorBlock::CompareAccessTime(DiskPlainSectorBlock *item1, DiskPlainSectorBlock *item2)
{
    return (item1->m_accs_time - item2->m_accs_time) || (item2->m_seq_num - item1->m_seq_num);
}

//

DiskPlainSectorBlockCache::DiskPlainSectorBlockCache(DiskPlainFile *parent, wxUint32 start_offset)
{
	p_parent = parent;
	m_start_offset = start_offset;
	p_cache = new DiskPlainSectorBlocks(DiskPlainSectorBlock::Compare);

	size_t sector_size = parent->GetSectorSize();
	if (sector_size == 0) sector_size = 256;

	m_limit_count = (size_t)gConfig.GetCacheLimitSize() * 1024 * 1024 / sector_size / DiskPlainSectorBlock::NumOfSecs;
	m_shrink_count = (size_t)gConfig.GetCacheShrinkSize() * 1024 * 1024 / sector_size / DiskPlainSectorBlock::NumOfSecs;

	m_cache_overflowed = 0;
}
DiskPlainSectorBlockCache::~DiskPlainSectorBlockCache()
{
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);
		delete itm;
	}
	delete p_cache;
}
/// セクタブロックをキャッシュに追加する
DiskPlainSectorBlock *DiskPlainSectorBlockCache::AddSectorBlock(int sector_pos)
{
	// キャッシュがいっぱい
	if (p_cache->Count() >= m_limit_count) {
		// メッセージ
		if (!m_cache_overflowed) {
			wxMessageBox(_("The data cache is reached the limited size. The modified data will be overwritten to the disk image. Are you sure?"),
				_("Data Cache Overflow"), wxOK);
		}
		m_cache_overflowed++;
		// 古いキャッシュを消す
		RemoveByAccessTime();
	}

	// ファイルからリードする

	DiskPlainSectorHeader header;
	size_t sector_size = p_parent->GetSectorSize();
	size_t block_size = sector_size * DiskPlainSectorBlock::NumOfSecs;

	if (block_size > m_temp.GetBufferSize()) {
		// expand buffer
		m_temp.SetSize(block_size);
	}

	// ファイル読み込み
	int seq_num = sector_pos / DiskPlainSectorBlock::NumOfSecs;
	wxFileStream *stream = p_parent->GetStream();
	wxFileOffset offset = seq_num * block_size + m_start_offset; 
	stream->SeekI(offset, wxFromStart);
	size_t read_size = stream->Read(m_temp.GetData(), block_size).LastRead();

	// キャッシュに追加
	DiskPlainSectorBlock *block = new DiskPlainSectorBlock(seq_num, header, m_temp.GetData(), read_size, sector_pos, sector_size, offset);
	p_cache->Add(block);

	return block;
}
/// アクセスのないセクタデータをキャッシュから削除する
void DiskPlainSectorBlockCache::RemoveByAccessTime()
{
	DiskPlainSectorBlocks temp(DiskPlainSectorBlock::CompareAccessTime);

	size_t limit = p_cache->Count();
	size_t i = 0;
	for(; i<limit; i++) {
		temp.Add(p_cache->Item(i));
	}
	if (limit > 32) {
		limit -= 32;
	}

	i = 0;
	while (p_cache->Count() > m_shrink_count && i < limit) {
		DiskPlainSectorBlock *itm = temp.Item(i);
		i++;

		if (itm->GetRefs() > 0) continue;

		RemoveSectorBlock(itm);
	}
}
/// セクタブロックを削除する
void DiskPlainSectorBlockCache::RemoveSectorBlock(DiskPlainSectorBlock *item)
{
	if (!item) return;

	// データが更新されている場合はファイルにライト
	if (item->IsModified()) {
		wxFileStream *stream = p_parent->GetStream();

		stream->SeekO(item->GetFileOffset(), wxFromStart);
		stream->Write(item->GetBufferData(), item->GetBufferSize());
	}

	// キャッシュから消す
	p_cache->Remove(item);
	delete item;
}
/// 指定したセクタ番号のセクタデータがキャッシュにあるか
/// @param[in] sector_pos : セクタ通し番号
/// @return セクタデータ / NULL
DiskPlainSector *DiskPlainSectorBlockCache::FindBySectorPos(int sector_pos)
{
	size_t cnt = p_cache->Count();
	if (cnt == 0) {
		// ない
		return NULL;
	}
	int seq_num = sector_pos / DiskPlainSectorBlock::NumOfSecs;
	size_t index = cnt / 2;
	DiskPlainSectorBlock *block = FindBySectorPosR(0, seq_num, index, 0, cnt - 1);
	if (block) {
		return block->GetSector(sector_pos % DiskPlainSectorBlock::NumOfSecs);
	} else {
		// ない
		return NULL;
	}
}
/// 指定したセクタ番号のセクタデータがキャッシュにあるか（再帰）
DiskPlainSectorBlock *DiskPlainSectorBlockCache::FindBySectorPosR(int depth, int seq_num, size_t index, size_t st, size_t ed)
{
	if (depth > 64) {
		return NULL;
	}

	DiskPlainSectorBlock *itm = p_cache->Item(index);
	int num = itm->GetNumber();
	if (num > seq_num) {
		// not match
		if (index == st) {
			return NULL;
		} else {
			ed = index - 1;
			index = (ed + 1 - st) / 2 + st;
			return FindBySectorPosR(depth + 1, seq_num, index, st, ed);
		}
	} else if (num < seq_num) {
		// not match
		if (index == ed) {
			return NULL;
		} else {
			st = index + 1;
			index = (ed + 1 - st) / 2 + st;
			return FindBySectorPosR(depth + 1, seq_num, index, st, ed);
		}
	} else {
		// match
		return itm;

	}
}
/// セクタデータを得る
DiskImageSector *DiskPlainSectorBlockCache::GetSector(int sector_pos)
{
	// ファイルから指定位置をリードする
	DiskImageSector *sector = FindBySectorPos(sector_pos);
	if (sector) {
		return sector;
	}
	// キャッシュにないときはファイルからリードする
	DiskPlainSectorBlock *block = AddSectorBlock(sector_pos);
	sector = block->GetSector(sector_pos);
	return sector;
}
/// キャッシュをクリア
void DiskPlainSectorBlockCache::ClearCache(int start, int size)
{
	int end = start + size;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);
		int num = itm->GetNumber();
		if (start > num || num >= end) continue;

		RemoveSectorBlock(itm);
	}
	m_cache_overflowed = 0;
}
/// セクタのリファレンス数をクリア
void DiskPlainSectorBlockCache::ClearRefs(int start, int size)
{
	int end = start + size;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);
		int num = itm->GetNumber();
		if (start > num || num >= end) continue;

		itm->ClearRefs();
	}
}
/// 変更されているか
bool DiskPlainSectorBlockCache::IsModified()
{
	bool modified = false;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);
		if (itm->IsModified()) {
			modified = true;
			break;
		}
	}
	return modified;
}
/// 変更されているか
bool DiskPlainSectorBlockCache::IsModified(int start, int size)
{
	int end = start + size;
	bool modified = false;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);
		int num = itm->GetNumber();
		if (start > num || num >= end) continue; 
		if (itm->IsModified()) {
			modified = true;
			break;
		}
	}
	return modified;
}
/// 変更をクリア
void DiskPlainSectorBlockCache::ClearModify()
{
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);
		itm->ClearModify();
	}
}
/// 変更をクリア
void DiskPlainSectorBlockCache::ClearModify(int start, int size)
{
	int end = start + size;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);
		int num = itm->GetNumber();
		if (start > num || num >= end) continue; 
		itm->ClearModify();
	}
}
/// データをすべて出力
void DiskPlainSectorBlockCache::Flush()
{
	if (p_parent->IsWriteProtected()) return;

	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);

		// データが更新されている場合はファイルにライト
		if (itm->IsModified()) {
			wxFileStream *stream = p_parent->GetStream();

			stream->SeekO(itm->GetFileOffset(), wxFromStart);
			stream->Write(itm->GetBufferData(), itm->GetBufferSize());

			itm->ClearModify();
		}
	}
}
/// 範囲内のデータを出力
void DiskPlainSectorBlockCache::Flush(int start, int size)
{
	if (p_parent->IsWriteProtected()) return;

	int end = start + size;
	for(size_t i=0; i<p_cache->Count(); i++) {
		DiskPlainSectorBlock *itm = p_cache->Item(i);
		int num = itm->GetNumber();
		if (start > num || num >= end) continue; 

		// データが更新されている場合はファイルにライト
		if (itm->IsModified()) {
			wxFileStream *stream = p_parent->GetStream();

			stream->SeekO(itm->GetFileOffset(), wxFromStart);
			stream->Write(itm->GetBufferData(), itm->GetBufferSize());

			itm->ClearModify();
		}
	}
}
/// ステータスメッセージ
void DiskPlainSectorBlockCache::GetStatusMessage(wxString &str) const
{
	str = wxString::Format(wxT("Cached: %d/%d"), (int)p_cache->Count(), (int)m_limit_count);
}

#endif // USE_SECTOR_BLOCK_CACHE

// ----------------------------------------------------------------------

//
//
//
DiskPlainFile::DiskPlainFile()
	: DiskImageFile()
{
	p_stream = NULL;
	p_disks = NULL;
	m_start_offset = 0;
	m_block_size = 0;
	p_cache = NULL;
	m_write_protected = true;
}

DiskPlainFile::~DiskPlainFile()
{
	Clear();
}

/// インスタンス作成
DiskImageDisk *DiskPlainFile::NewImageDisk(int disk_number, wxUint32 start_block, wxUint32 block_size)
{
	return new DiskPlainDisk(this, disk_number, start_block, block_size);
}
/// インスタンス作成
DiskImageDisk *DiskPlainFile::NewImageDisk(int disk_number, DiskImageDiskHeader &n_header)
{
	return new DiskPlainDisk(this, disk_number, n_header);
}

void DiskPlainFile::SetFileStream(wxFileStream *stream)
{
	p_stream = stream;
}

size_t DiskPlainFile::Add(DiskImageDisk *newdisk, short mod_flags)
{
	if (!p_disks) p_disks = new DiskImageDisks;
	p_disks->Add(newdisk);
	return p_disks->Count();
}

void DiskPlainFile::Clear()
{
	if (p_disks) {
		for(size_t i=0; i<p_disks->Count(); i++) {
			DiskPlainDisk *p = (DiskPlainDisk *)p_disks->Item(i);
			delete p;
		}
		delete p_disks;
		p_disks = NULL;
	}
	if (p_stream) {
		delete p_stream;
		p_stream = NULL;
	}
	if (p_cache) {
		delete p_cache;
		p_cache = NULL;
	}
}

size_t DiskPlainFile::Count() const
{
	if (!p_disks) return 0;
	return p_disks->Count();
}

bool DiskPlainFile::Delete(size_t idx)
{
	DiskPlainDisk *disk = (DiskPlainDisk *)GetDisk(idx);
	if (!disk) return false;
	delete disk;
	p_disks->RemoveAt(idx);
	return true;
}

DiskImageDisk *DiskPlainFile::GetDisk(size_t idx)
{
	if (!p_disks) return NULL;
	if (idx >= p_disks->Count()) return NULL;
	return p_disks->Item(idx);
}

/// 指定セクタを返す
/// @param[in] sector_pos  セクタ位置（0からの通し番号）
/// @return セクタ
DiskImageSector *DiskPlainFile::GetSector(int sector_pos)
{
	if (!p_cache) {
#ifdef USE_SECTOR_BLOCK_CACHE
		p_cache = new DiskPlainSectorBlockCache(this, m_start_offset);
#else
		p_cache = new DiskPlainSectorCache(this, m_start_offset);
#endif
	}
	return p_cache->GetSector(sector_pos);
}

/// キャッシュを更新する
void DiskPlainFile::RefreshCache(int sector_pos)
{
	if (!p_cache) {
#ifdef USE_SECTOR_BLOCK_CACHE
		p_cache = new DiskPlainSectorBlockCache(this, m_start_offset);
#else
		p_cache = new DiskPlainSectorCache(this, m_start_offset);
#endif
	}
	p_cache->GetSector(sector_pos);
}
/// キャッシュをクリアする
void DiskPlainFile::ClearCache(wxUint32 start, wxUint32 size)
{
	if (!p_cache) return;
	return p_cache->ClearCache(start, size);
}
/// キャッシュのリファレンス数ををクリアする
void DiskPlainFile::ClearCacheRefs(wxUint32 start, wxUint32 size)
{
	if (!p_cache) return;
	return p_cache->ClearRefs(start, size);
}
/// 変更されているか
bool DiskPlainFile::IsModified()
{
	if (!p_cache) return false;
	return p_cache->IsModified();
}
/// 範囲内のセクタが変更されているか
bool DiskPlainFile::IsModified(wxUint32 start, wxUint32 size)
{
	if (!p_cache) return false;
	return p_cache->IsModified(start, size);
}
/// 変更をクリア
void DiskPlainFile::ClearModify()
{
	if (!p_cache) return;
	p_cache->ClearModify();
}
/// 範囲内のセクタ変更をクリア
void DiskPlainFile::ClearModify(wxUint32 start, wxUint32 size)
{
	if (!p_cache) return;
	p_cache->ClearModify(start, size);
}
/// データをすべて出力
void DiskPlainFile::Flush()
{
	if (!p_cache) return;
	p_cache->Flush();
}
/// 範囲内のデータを出力
void DiskPlainFile::Flush(wxUint32 start, wxUint32 size)
{
	if (!p_cache) return;
	p_cache->Flush(start, size);
}

/// 書き込み禁止かどうかを返す
/// @return true:書き込み禁止
bool DiskPlainFile::IsWriteProtected() const
{
	return m_write_protected;
}

/// 書き込み禁止かどうかを設定
/// @param[in] val 書き込み禁止ならtrue
void DiskPlainFile::SetWriteProtect(bool val)
{
	m_write_protected = val;
}

/// ファイルの説明詳細
wxString DiskPlainFile::GetDescriptionDetails() const
{
	wxString str = m_desc;
	str += wxT(" [");
	str += GetDiskParamDetails();
	str += wxT("]");
	return str;
}

/// ファイルの説明
void DiskPlainFile::SetDescription(const wxString &desc)
{
	m_desc = desc;
}

/// ステータスメッセージ
void DiskPlainFile::GetStatusMessage(wxString &str) const
{
	if (!p_cache) return;
	p_cache->GetStatusMessage(str);
}

// ----------------------------------------------------------------------

//
//
//
DiskPlain::DiskPlain()
	: DiskImage()
{
	p_file = NULL;
}

DiskPlain::~DiskPlain()
{
	ClearFile();
}

/// インスタンス作成
DiskImageFile *DiskPlain::NewImageFile()
{
	return new DiskPlainFile;
}

/// 新規作成
/// @param [in] diskname      ディスク名
/// @param [in] param         ディスクパラメータ
/// @param [in] write_protect 書き込み禁止
/// @param [in] basic_hint    DISK BASIC種類のヒント
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskPlain::Create(const wxString &diskname, const DiskParam &param, bool write_protect, const wxString &basic_hint)
{
	m_result.Clear();

	NewFile(diskname);
	p_file->SetBasicTypeHint(basic_hint);
	DiskPlainCreator cr(diskname, param, write_protect, p_file, m_result);
	int valid_disk = cr.Create();

	// エラーあり
	if (valid_disk < 0) {
		ClearFile();
		return valid_disk;
	}

	// ファイルを開く
	valid_disk = Open(diskname, wxEmptyString, param);

	return valid_disk;
}

/// ファイルを開く
/// @param [in] filepath    ファイルパス
/// @param [in] file_format ファイルの形式名("d88","plain"など)
/// @param [in] param_hint  ディスクパラメータヒント("plain"時のみ)
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskPlain::Open(const wxString &filepath, const wxString &file_format, const DiskParam &param_hint)
{
	m_result.Clear();

	// ファイルを開く
	wxFileStream *fstream = new wxFileStream(filepath);
	if (!fstream->IsOk()) {
		m_result.SetError(DiskResult::ERR_CANNOT_OPEN);
		delete fstream;
		return -1;
	}

	NewFile(filepath);
	((DiskPlainFile *)p_file)->SetFileStream(fstream);
	DiskParser ps(filepath, fstream, p_file, m_result);
	int valid_disk = ps.Parse(file_format, param_hint);

	// エラーあり
	if (valid_disk < 0) {
		ClearFile();
	}

	return valid_disk;
}

/// ファイルを開く前のチェック
/// @param [in] filepath        ファイルパス
/// @param [in,out] file_format ファイルの形式名("d88","plain"など)
/// @param [out] params         ディスクパラメータの候補
/// @param [out] manual_param   候補がないときのパラメータヒント
/// @retval  0 問題なし
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskPlain::Check(const wxString &filepath, wxString &file_format, DiskParamPtrs &params, DiskParam &manual_param)
{
	m_result.Clear();

	// ファイルを開く
	wxFileInputStream fstream(filepath);
	if (!fstream.IsOk()) {
		m_result.SetError(DiskResult::ERR_CANNOT_OPEN);
		return -1;
	}

	DiskParser ps(filepath, &fstream, p_file, m_result);
	return ps.Check(file_format, params, manual_param);
}

/// 閉じる
void DiskPlain::Close()
{
	ClearFile();
}

/// ファイルを開いているか
bool DiskPlain::IsOpened() const
{
	return (p_file != NULL);
}

/// ストリームの内容をファイルに保存できるか
/// @param[in] filepath    開いているファイルパス
/// @param[in] file_format 保存ファイルのフォーマット
int DiskPlain::CanSave(const wxString &filepath, const wxString &file_format)
{
	DiskWriter dw(this, filepath, &m_result);
	return dw.CanSave(file_format);
}
/// ストリームの内容をファイルに保存
/// @param[in] filepath    保存先ファイルパス
/// @param[in] file_format 保存ファイルのフォーマット
/// @param[in] options     保存時のオプション
/// @retval  0:正常
/// @retval -1:エラー
int DiskPlain::Save(const wxString &filepath, const wxString &file_format, const DiskWriteOptions &options)
{
	DiskWriter dw(this, filepath, options, &m_result);
	return dw.Save(file_format);
}
/// ストリームの内容をファイルに保存
/// @param[in] disk_number ディスク番号
/// @param[in] filepath    保存先ファイルパス
/// @param[in] file_format 保存ファイルのフォーマット
/// @param[in] options     保存時のオプション
/// @retval  0:正常
/// @retval -1:エラー
int DiskPlain::SaveDisk(int disk_number, const wxString &filepath, const wxString &file_format, const DiskWriteOptions &options)
{
	DiskWriter dw(this, filepath, options, &m_result);
	return dw.SaveDisk(disk_number, file_format);
}

/// ディスクを削除
/// @param[in] disk_number ディスク番号
/// @return true
bool DiskPlain::Delete(int disk_number)
{
	if (!p_file) return false;
	p_file->Delete((size_t)disk_number);
//	file->SetModify();
	return true;
}
/// 置換元のディスクを解析
/// @param [in] disk_number ディスク番号
/// @param [in] filepath    ファイルパス
/// @param [in] file_format ファイルの形式名("d88","plain"など)
/// @param [in] param_hint  ディスクパラメータヒント("plain"時のみ)
/// @param [out] src_file   ソースディスク
/// @param [out] tag_disk   ターゲットディスク
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskPlain::ParseForReplace(int disk_number, const wxString &filepath, const wxString &file_format, const DiskParam &param_hint, DiskImageFile &src_file, DiskImageDisk* &tag_disk)
{
	if (!p_file) return 0;

	m_result.Clear();

	wxFileInputStream fstream(filepath);
	if (!fstream.IsOk()) {
		m_result.SetError(DiskResult::ERR_CANNOT_OPEN);
		return -1;
	}

	DiskParser ps(filepath, &fstream, &src_file, m_result);
	int valid_disk = ps.Parse(file_format, param_hint);

	// エラーあり
	if (valid_disk < 0) {
		return valid_disk;
	}

	// ディスクを選択 
	tag_disk = p_file->GetDisk(disk_number);
	if (!tag_disk) {
		m_result.SetError(DiskResult::ERR_NO_DATA);
		return m_result.GetValid();
	}

	return 0;
}
/// ファイルでディスクを置換
/// @param [in] disk_number     ディスク番号
/// @param [in] src_disk        ソースディスク
/// @param [in] src_side_number ソース側のサイド番号
/// @param [in] tag_disk        ターゲットディスク
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskPlain::ReplaceDisk(int disk_number, DiskImageDisk *src_disk, int src_side_number, DiskImageDisk *tag_disk)
{
	if (!p_file) return 0;

	m_result.Clear();

	int valid_disk = tag_disk->Replace(src_disk, src_side_number);
	if (valid_disk != 0) {
		m_result.SetError(DiskResult::ERR_REPLACE);
	}

	return valid_disk;
}

/// DISK BASICが一致するか
bool DiskPlain::MatchDiskBasic(const DiskBasic *target)
{
	bool match = false;
	DiskImageDisks *disks = GetDisks();
	if (!disks) return false;
	for(size_t i = 0; i < disks->Count(); i++) {
		DiskImageDisk *disk = disks->Item(i);
		DiskBasics *basics = disk->GetDiskBasics();
		if (!basics) return false;
		for(size_t j = 0; j < basics->Count(); j++) {
			if (target == basics->Item(j)) {
				match = true;
				break;
			}
		}
	}
	return match;
}

/// DISK BASICの解析状態をクリア
void DiskPlain::ClearDiskBasicParseAndAssign(int disk_number, int side_number)
{
	DiskImageDisk *disk = GetDisk(disk_number);
	if (!disk) return;

	if (p_file) {
		p_file->SetBasicTypeHint(wxT(""));
	}

	DiskBasics *basics = disk->GetDiskBasics();
	if (!basics) return;
	basics->ClearParseAndAssign(side_number);
}

/// キャラクターコードマップ番号設定
void DiskPlain::SetCharCode(const wxString &name)
{
	DiskImageDisks *disks = GetDisks();
	if (!disks) return;
	for(size_t i = 0; i < disks->Count(); i++) {
		DiskImageDisk *disk = disks->Item(i);
		disk->SetCharCode(name);
	}
}
