/// @file diskimage.cpp
///
/// @brief ディスクイメージ入出力
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskimage.h"
#include <wx/wfstream.h>
#include <wx/xml/xml.h>
#include "diskparser.h"
#include "diskwriter.h"
#include "../basicfmt/basicparam.h"
#include "../basicfmt/basicfmt.h"


// ----------------------------------------------------------------------
//
//
//
void IntHashMapUtil::IncleaseValue(IntHashMap &hash_map, int key)
{
	hash_map[key]++;
}
int IntHashMapUtil::GetMaxKeyOnMaxValue(IntHashMap &hash_map)
{
	IntHashMap::iterator it;
	int key_result = 0;
	int key2 = 0;
	int val = 0;
	for(it = hash_map.begin(); it != hash_map.end(); it++) {
		if (val < it->second) {
			val = it->second;
			key_result = it->first;
		} else if (val == it->second) {
			key2 = it->first;
			if (key_result < key2) {
				key_result = key2;
			}
		}
	}
	return key_result;
}
int IntHashMapUtil::MaxValue(int src, int value)
{
	return (src > value ? src : value);
}
int IntHashMapUtil::MinValue(int src, int value)
{
	return (src < value ? src : value);
}

// ----------------------------------------------------------------------
//
//
//
DiskImageSector::DiskImageSector(int n_num)
{
	m_num = n_num;
}

DiskImageSector::~DiskImageSector()
{
}
/// ID Nからセクタサイズを計算
int DiskImageSector::ConvIDNToSecSize(wxUint8 n)
{
	int sec = 0;
	if (n <= 3) sec = gSectorSizes[n];
	return sec;
}
/// セクタサイズからID Nを計算
wxUint8 DiskImageSector::ConvSecSizeToIDN(int size)
{
	wxUint8 n = 1;
	for(int i=0; gSectorSizes[i] != 0; i++) {
		if (gSectorSizes[i] == size) {
			n = (wxUint8)i;
			break;
		}
	}
	return n;
}

// ----------------------------------------------------------------------
//
//
//
DiskImageDiskHeader::DiskImageDiskHeader()
{
	m_id = 0;
}
DiskImageDiskHeader::DiskImageDiskHeader(int id)
{
	m_id = id;
}
DiskImageDiskHeader::~DiskImageDiskHeader()
{
}
bool DiskImageDiskHeader::Copy(DiskImageDiskHeader &src)
{
	if (m_id != src.m_id) {
		return false;
	}
	return true;
}

// ----------------------------------------------------------------------
//
//
//
DiskImageDisk::DiskImageDisk(DiskImageFile *file, int n_num)
{
	parent = file;
	m_num = n_num;
}
DiskImageDisk::DiskImageDisk(DiskImageFile *file, int n_num, DiskImageDiskHeader &n_header)
{
	parent = file;
	m_num = n_num;
}

DiskImageDisk::~DiskImageDisk()
{
}

/// ディスク名を返す
/// @param[in] real 名称が空白の時そのまま返すか
/// @return ディスク名
wxString DiskImageDisk::GetName(bool real) const
{
	return _T("");
}

/// セクタサイズを返す
int DiskImageDisk::GetSectorSize() const
{
	return parent ? parent->GetSectorSize() : 0;
}

/// BASIC種類を設定
void DiskImageDisk::AddBasicType(const wxString &name)
{
	m_basic_types.Add(BasicParamName(name, 0));
}

/// DISK BASIC領域を確保
void DiskImageDisk::AllocDiskBasics()
{
}

/// DISK BASICを返す
DiskBasic *DiskImageDisk::GetDiskBasic(int idx)
{
	return NULL;
}

/// DISK BASICをクリア
void DiskImageDisk::ClearDiskBasics()
{
}

/// キャラクターコードマップ番号設定
void DiskImageDisk::SetCharCode(const wxString &name)
{
}

// セクタ位置からトラック、サイド、セクタ番号を得る(オフセットを考慮)
void DiskImageDisk::GetNumberFromBlockNum(int block_num, int &track_num, int &side_num, int &sector_num) const
{
}

// ----------------------------------------------------------------------
//
//
//
DiskImageFile::DiskImageFile()
	: DiskParam()
{
}

DiskImageFile::DiskImageFile(const DiskParam &disk_param)
	: DiskParam(disk_param)
{
}

DiskImageFile::~DiskImageFile()
{
}

size_t DiskImageFile::Add(DiskImageDisk *newdsk, short mod_flags)
{
	return 0;
}

void DiskImageFile::Clear()
{
}

size_t DiskImageFile::Count() const
{
	return 0;
}

bool DiskImageFile::Delete(size_t idx)
{
	return false;
}

/// ファイル名を返す
wxString DiskImageFile::GetName() const
{
	return m_filename.GetFullName();
}

/// パスを返す
wxString DiskImageFile::GetPath() const
{
	return m_filename.GetPath();
}

/// ファイルパスを返す
wxString DiskImageFile::GetFilePath() const
{
	return m_filename.GetFullPath();
}

/// ファイル名を設定
void DiskImageFile::SetFileName(const wxString &path)
{
	m_filename = wxFileName(path);
}

/// セクタ位置からトラック、サイド、セクタ番号を得る
void DiskImageFile::GetNumberFromSectorPos(int sector_pos, int &track_num, int &side_num, int &sector_num) const
{
	if (sector_pos < 0) {
		track_num = -1;
		side_num = -1;
		sector_num = -1;
	} else {
		track_num = sector_pos / GetSectorsPerTrack() / GetSidesPerDisk();
		side_num = (sector_pos / GetSectorsPerTrack()) % GetSidesPerDisk();
		sector_num = (sector_pos % GetSectorsPerTrack());
	}
}

/// トラック、サイド、セクタ番号からセクタ位置を得る
int DiskImageFile::GetSectorPosFromNumber(int track_num, int side_num, int sector_num) const
{
	int sector_pos = 0;
	if (track_num >= 0 && side_num >= 0 && sector_num >= 0) {
		sector_pos = track_num * GetSectorsPerTrack() * GetSidesPerDisk();
		sector_pos += side_num * GetSectorsPerTrack();
		sector_pos += sector_num;
	}
	return sector_pos;
}

// ======================================================================
//
//
//
DiskImage::DiskImage()
{
}

DiskImage::~DiskImage()
{
}

/// 新規作成
/// @param [in] diskname      ディスク名
/// @param [in] param         ディスクパラメータ
/// @param [in] write_protect 書き込み禁止
/// @param [in] basic_hint    DISK BASIC種類のヒント
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskImage::Create(const wxString &diskname, const DiskParam &param, bool write_protect, const wxString &basic_hint)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}

/// 追加で新規作成
/// @param [in] diskname      ディスク名
/// @param [in] param         ディスクパラメータ
/// @param [in] write_protect 書き込み禁止
/// @param [in] basic_hint    DISK BASIC種類のヒント
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskImage::Add(const wxString &diskname, const DiskParam &param, bool write_protect, const wxString &basic_hint)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}

/// ファイルを追加
/// @param [in] filepath    ファイルパス
/// @param [in] file_format ファイルの形式名("d88","plain"など)
/// @param [in] param_hint  ディスクパラメータヒント("plain"時のみ)
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskImage::Add(const wxString &filepath, const wxString &file_format, const DiskParam &param_hint)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}

/// ファイルを開く
/// @param [in] filepath    ファイルパス
/// @param [in] file_format ファイルの形式名("d88","plain"など)
/// @param [in] param_hint  ディスクパラメータヒント("plain"時のみ)
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskImage::Open(const wxString &filepath, const wxString &file_format, const DiskParam &param_hint)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}

/// ファイルを開く前のチェック
/// @param [in] filepath        ファイルパス
/// @param [in,out] file_format ファイルの形式名("d88","plain"など)
/// @param [out] params         ディスクパラメータの候補
/// @param [out] manual_param   候補がないときのパラメータヒント
/// @retval  0 問題なし
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskImage::Check(const wxString &filepath, wxString &file_format, DiskParamPtrs &params, DiskParam &manual_param)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}

/// 閉じる
void DiskImage::Close()
{
}

/// ファイルを開いているか
bool DiskImage::IsOpened() const
{
	return false;
}

/// ストリームの内容を上書き保存できるか
int DiskImage::CanSave()
{
	if (p_file->IsWriteProtected()) {
		m_result.SetError(DiskResult::ERR_WRITE_PROTECTED);
	}
	return m_result.GetValid();
}
/// ストリームの内容をファイルに保存できるか
/// @param[in] filepath    開いているファイルパス
/// @param[in] file_format 保存ファイルのフォーマット
int DiskImage::CanSave(const wxString &filepath, const wxString &file_format)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}
/// ストリームの内容を上書き保存
/// @param[in] options     保存時のオプション
/// @retval  0:正常
/// @retval -1:エラー
int DiskImage::Save(const DiskWriteOptions &options)
{
	if (p_file->IsWriteProtected()) {
		m_result.SetError(DiskResult::ERR_WRITE_PROTECTED);
		return m_result.GetValid();
	}
	p_file->Flush();
	return m_result.GetValid();
}
/// ストリームの内容をファイルに保存
/// @param[in] filepath    保存先ファイルパス
/// @param[in] file_format 保存ファイルのフォーマット
/// @param[in] options     保存時のオプション
/// @retval  0:正常
/// @retval -1:エラー
int DiskImage::Save(const wxString &filepath, const wxString &file_format, const DiskWriteOptions &options)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}
/// ストリームの内容をファイルに保存
/// @param[in] disk_number ディスク番号
/// @param[in] filepath    保存先ファイルパス
/// @param[in] file_format 保存ファイルのフォーマット
/// @param[in] options     保存時のオプション
/// @retval  0:正常
/// @retval -1:エラー
int DiskImage::SaveDisk(int disk_number, const wxString &filepath, const wxString &file_format, const DiskWriteOptions &options)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}

/// ディスクを削除
/// @param[in] disk_number ディスク番号
/// @return true
bool DiskImage::Delete(int disk_number)
{
	return false;
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
int DiskImage::ParseForReplace(int disk_number, const wxString &filepath, const wxString &file_format, const DiskParam &param_hint, DiskImageFile &src_file, DiskImageDisk* &tag_disk)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}
/// ファイルでディスクを置換
/// @param [in] disk_number     ディスク番号
/// @param [in] src_disk        ソースディスク
/// @param [in] src_side_number ソース側のサイド番号
/// @param [in] tag_disk        ターゲットディスク
/// @retval  0 正常
/// @retval -1 エラーあり
/// @retval  1 警告あり
int DiskImage::ReplaceDisk(int disk_number, DiskImageDisk *src_disk, int src_side_number, DiskImageDisk *tag_disk)
{
	m_result.SetError(DiskResult::ERR_UNSUPPORTED);
	return -1;
}

/// ディスク名を設定
bool DiskImage::SetDiskName(size_t disk_number, const wxString &newname)
{
	DiskImageDisk *disk = GetDisk(disk_number);
	if (!disk) return false;

	if (disk->GetName() != newname) {
		disk->SetName(newname);
		return true;
	}
	return false;
}

/// ディスク名を返す
wxString DiskImage::GetDiskName(size_t disk_number, bool real) const
{
	const DiskImageDisk *disk = GetDisk(disk_number);
	if (!disk) return wxT("");
	return disk->GetName(real);
}

/// ファイル構造体を作成
void DiskImage::NewFile(const wxString &filepath)
{
	if (p_file) {
		delete p_file;
	}
	p_file = NewImageFile();
	SetFileName(filepath);
}

/// ファイル構造体をクリア
void DiskImage::ClearFile()
{
	delete p_file;
	p_file = NULL;
}
/// ディスクを変更したか
bool DiskImage::IsModified()
{
	bool modified = false;
	if (p_file) {
		modified = p_file->IsModified();
	}
	return modified;
}
/// ディスク枚数
size_t DiskImage::CountDisks() const
{
	if (!p_file) return 0;
	return p_file->Count();
}

/// ディスク一覧を返す
DiskImageDisks *DiskImage::GetDisks()
{
	if (!p_file) return NULL;
	return p_file->GetDisks();
}
/// 指定した位置のディスクを返す
DiskImageDisk *DiskImage::GetDisk(size_t index)
{
	if (!p_file) return NULL;
	return p_file->GetDisk(index);
}
/// 指定した位置のディスクを返す
const DiskImageDisk *DiskImage::GetDisk(size_t index) const
{
	if (!p_file) return NULL;
	return p_file->GetDisk(index);
}

/// ファイル名を返す
wxString DiskImage::GetName() const
{
	if (!p_file) return wxT("");
	return p_file->GetName();
}

/// パスを返す
wxString DiskImage::GetPath() const
{
	if (!p_file) return wxT("");
	return p_file->GetPath();
}

/// ファイルパスを返す
wxString DiskImage::GetFilePath() const
{
	if (!p_file) return wxT("");
	return p_file->GetFilePath();
}

/// ファイル名を設定
void DiskImage::SetFileName(const wxString &path)
{
	if (!p_file) return;
	p_file->SetFileName(path);
}

/// DISK BASICが一致するか
bool DiskImage::MatchDiskBasic(const DiskBasic *target)
{
	return false;
}

/// DISK BASICの解析状態をクリア
void DiskImage::ClearDiskBasicParseAndAssign(int disk_number, int side_number)
{
}

/// キャラクターコードマップ番号設定
void DiskImage::SetCharCode(const wxString &name)
{
}

/// ステータスメッセージ
void DiskImage::GetStatusMessage(wxString &str) const
{
	if (!p_file) return;
	p_file->GetStatusMessage(str);
}

/// エラーメッセージ
const wxArrayString &DiskImage::GetErrorMessage(int maxrow)
{
	return m_result.GetMessages(maxrow);
}

/// エラーメッセージを表示
void  DiskImage::ShowErrorMessage()
{
	m_result.Show();
}

/// エラー警告メッセージを表示
int DiskImage::ShowErrWarnMessage()
{
	return ResultInfo::ShowErrWarnMessage(m_result.GetValid(), m_result.GetMessages(-1));
}
