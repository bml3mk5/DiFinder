/// @file basicfmt.cpp
///
/// @brief disk basic
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "basicfmt.h"
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/numformatter.h>
#include <wx/datetime.h>
#include "basicfat.h"
#include "basicdir.h"
#include "basictype.h"
#include "basictype_msdos.h"
#include "basictype_os9.h"
#include "basictype_hu68k.h"
#include "../logging.h"
#include "../utils.h"


#ifdef DeleteFile
#undef DeleteFile
#endif
#ifdef GetCurrentDirectory
#undef GetCurrentDirectory
#endif

//#define DEBUG_DISK_FULL_TEST 1

//////////////////////////////////////////////////////////////////////
//
//
//
DiskBasicIdentifiedData::DiskBasicIdentifiedData()
{
	m_volume_name_maxlen = 0;
	m_volume_number = 0;
	m_volume_number_hexa = false;
}
DiskBasicIdentifiedData::DiskBasicIdentifiedData(const wxString &volume_name, int volume_number, const wxString &volume_date)
{
	m_volume_name = volume_name;
	m_volume_name_maxlen = 0;
	m_volume_number = volume_number;
	m_volume_number_hexa = false;
	m_volume_date = volume_date;
}

//////////////////////////////////////////////////////////////////////
//
//
//
DiskBasics::DiskBasics()
{
	basics = NULL;
}
DiskBasics::~DiskBasics()
{
	if (basics) {
		for(size_t idx = 0; idx < basics->Count(); idx++) {
			DiskBasic *p = basics->Item(idx);
			delete p;
		}
	}
	delete basics;
}
void DiskBasics::Add(DiskBasic *newitem)
{
	if (!basics) {
		basics = new ArrayOfDiskBasic;
	}
	if (!newitem) newitem = new DiskBasic;
	basics->Add(newitem);
}
void DiskBasics::Clear()
{
	if (basics) {
		for(size_t idx = 0; idx < basics->Count(); idx++) {
			DiskBasic *p = basics->Item(idx);
			delete p;
		}
		basics->Clear();
	}
}
void DiskBasics::Empty()
{
	if (basics) {
		for(size_t idx = 0; idx < basics->Count(); idx++) {
			DiskBasic *p = basics->Item(idx);
			delete p;
		}
		basics->Empty();
	}
}
DiskBasic *DiskBasics::Item(size_t idx)
{
	DiskBasic *item = NULL;
	if (basics && idx < basics->Count()) {
		item = basics->Item(idx);
	}
	return item;
}
size_t DiskBasics::Count()
{
	size_t count = 0;
	if (basics) {
		count = basics->Count();
	}
	return count;
}
void DiskBasics::RemoveAt(size_t idx)
{
	if (basics && idx < basics->Count()) {
		DiskBasic *p = basics->Item(idx);
		delete p;
		basics->RemoveAt(idx);
	}
}
void DiskBasics::ClearParseAndAssign(int idx)
{
	if (!basics) return;

	for(size_t i = 0; i < basics->Count(); i++) {
		DiskBasic *basic = basics->Item(i);
		if (idx < 0 || (int)i == idx) {
			basic->ClearParseAndAssign();
		}
	}
}

//////////////////////////////////////////////////////////////////////
//
//
//
DiskBasic::DiskBasic() : DiskBasicParam()
{
	disk = NULL;
	formatted = false;
	parsed = false;
	assigned = false;
	selected_side = -1;
	data_start_sector = 0;
	skipped_track = 0x7fff;
//	reverse_side = false;
//	inverted_data = false;
//	char_code = -1;

	fat = new DiskBasicFat(this);
	dir = new DiskBasicDir(this);
	type = NULL;
}

DiskBasic::~DiskBasic()
{
	delete type;
	delete dir;
	delete fat;
}

/// BASIC種類を設定
void DiskBasic::CreateType()
{
	delete type;
	type = NULL;

	const DiskBasicFormat *fmt = GetFormatType();
	if (!fmt) return;

	switch(fmt->GetTypeNumber()) {
	case FORMAT_TYPE_MSDOS:
		type = new DiskBasicTypeMSDOS(this, fat, dir);
		break;
	case FORMAT_TYPE_OS9:
		type = new DiskBasicTypeOS9(this, fat, dir);
		break;
	case FORMAT_TYPE_HU68K:
		type = new DiskBasicTypeHU68K(this, fat, dir);
		break;
	default:
		wxFAIL_MSG(wxT("Unknown type is defined in basic_type.xml."));
		break;
	}
}

/// 最も高い値のインデックスを返す
int DiskBasic::MaxRatio(wxArrayDouble &values)
{
	int idx = -1;
	double max_ratio = -1.0;
	for(size_t i=0; i<values.Count(); i++) {
		double ratio = values.Item(i);
		if (ratio > max_ratio) {
			idx = (int)i;
			max_ratio = ratio;
		}
	}
	return idx;
}

/// 指定したディスクがDISK BASICかを解析する
/// @param [in] newdisk       : 新しいディスク
/// @param [in] newside       : サイド番号 片面の場合のみ 両面なら -1
/// @param [in] match         : パラメータ 既に分かっている場合にセット 
/// @param [in] is_formatting : フォーマット実行時 true
/// @retval >0:ワーニング
/// @retval  0:正常
/// @retval <0:エラーあり
int DiskBasic::ParseDisk(DiskImageDisk *newdisk, int newside, const DiskBasicParam *match, bool is_formatting)
{
	errinfo.Clear();

	selected_side = newside;

	if (assigned) return 0;

	disk = newdisk;
	formatted = false;

	myLog.SetInfo("Parsing Disk #%d ...", newdisk->GetNumber());

	DiskImageFile *file = newdisk->GetFile();
	wxString hint = file->GetBasicTypeHint();
	BasicParamNames types = newdisk->GetBasicTypes();
	DiskBasicParamPtrs valid_params;
	wxArrayDouble      valid_ratios;

	double valid_ratio = 0.0;
	if (!match) {
		// DISK BASICかどうか
		bool support = false;
		match = gDiskBasicTemplates.FindType(hint, types);
		if (match) {
			support = true;
		}
		if (!support) {
			// DISK BASICとして使用不可
			Clear();
			errinfo.SetError(DiskBasicError::ERR_SUPPORTED);
			return errinfo.GetValid();
		}

		for(size_t n=0; n<types.Count(); n++) {
			match = gDiskBasicTemplates.FindType(hint, types.Item(n).GetName());
			if (match) {
				// フォーマットされているか？
				myLog.SetInfo("Parsing format: %s", match->GetBasicTypeName().t_str());
				valid_ratio = ParseFormattedDisk(newdisk, match, is_formatting);
				myLog.SetInfo("  Result => %.2f", valid_ratio);
				if (valid_ratio >= 0.0) {
					// 候補にする
					valid_params.Add(match);
					valid_ratios.Add(valid_ratio);
				}
			}
		}

		errinfo.Clear();
		if (valid_params.Count() > 0) {
			// それらしいものを候補とする
			int idx = MaxRatio(valid_ratios);
			if (idx < 0) idx = 0;
			match = valid_params.Item(idx);
			myLog.SetInfo("Decided format: %s", match->GetBasicTypeName().t_str());
			valid_ratio = ParseFormattedDisk(newdisk, match, is_formatting);
			myLog.SetInfo("  Result => %.2f", valid_ratio);
		}
	} else {
		// すでにフォーマット済み
		myLog.SetInfo("Known format: %s", match->GetBasicTypeName().t_str());
		valid_ratio = ParseFormattedDisk(newdisk, match, is_formatting);
		myLog.SetInfo("  Result => %.2f", valid_ratio);
	}
	if (valid_ratio >= 0.6) {
		errinfo.Clear();
		parsed = true;
	}
	if (!formatted) {
		errinfo.SetInfo(DiskBasicError::ERR_FORMATTED);
	}
	return errinfo.GetValid();
}

/// 指定のDISK BASICでフォーマットされているかを解析＆チェック
/// @param [in] newdisk       : 新しいディスク
/// @param [in] match         : DISK BASICのパラメータ
/// @param [in] is_formatting : フォーマット実行時true
/// @return <0.0      エラーあり
double DiskBasic::ParseFormattedDisk(DiskImageDisk *newdisk, const DiskBasicParam *match, bool is_formatting)
{
	double valid_ratio = 0.0; 

	SetBasicParam(*match);

	dir->SetFormatType(GetFormatType());

	int sector_size = newdisk->GetSectorSize();
	CalcDirStartEndSector(sector_size);
	CreateType();
	if (!type) {
		return -1.0;
	}

	AssignParameter();

	// 必要ならディスク上のパラメータを解析
	double prm_valid_ratio = type->ParseParamOnDisk(is_formatting);
	if (prm_valid_ratio < 0.0) {
		errinfo.SetError(DiskBasicError::ERR_IN_PARAMETER_AREA);
	} else if (prm_valid_ratio < 1.0) {		
		errinfo.SetInfo(DiskBasicError::ERR_INVALID_IN_PARAMETER_AREA);
	}
	valid_ratio += prm_valid_ratio;

	// FATのチェック
	double fat_valid_ratio = 0.0;
	if (valid_ratio >= 0.0) {
		fat_valid_ratio = AssignFat(is_formatting);
		if (!is_formatting && fat_valid_ratio < 0.0) {
			errinfo.SetInfo(DiskBasicError::ERR_IN_FAT_AREA);
		}
		valid_ratio += fat_valid_ratio;
	}

	// ディレクトリのチェック
	double dir_valid_ratio = 0.0;
	if (valid_ratio >= 0.0) {
		dir_valid_ratio = CheckRootDirectory(is_formatting);
		if (!is_formatting && dir_valid_ratio < 0.0) {
			errinfo.SetInfo(DiskBasicError::ERR_IN_DIRECTORY_AREA);
		}
		valid_ratio += dir_valid_ratio;
	}
	if ((prm_valid_ratio >= 0.0 && fat_valid_ratio >= 0.0 && dir_valid_ratio >= 0.0) || is_formatting) {
		// フォーマット完了
		formatted = true;
	}

	valid_ratio /= 3.0;

	return valid_ratio;
}

/// パラメータをクリア
void DiskBasic::Clear()
{
	disk = NULL;
	formatted = false;
	parsed = false;
	assigned = false;
	selected_side = -1;

	DiskBasicParam::ClearBasicParam();

	if (type) type->ClearDiskFreeSize();
}

/// DISKイメージの番号を返す
int DiskBasic::GetDiskNumber() const
{
	return disk ? disk->GetNumber() : -1;
}

/// 選択中のサイド文字列を返す
wxString DiskBasic::GetSelectedSideStr() const
{
	return Utils::GetSideStr(selected_side, false);
}

/// DISK BASICの説明を取得
const wxString &DiskBasic::GetDescriptionDetail()
{
	wxString desc = DiskBasicParam::GetBasicDescription();
	int free_size = type ? (int)type->GetFreeDiskSize() : -1;
	int free_groups = type ? (int)type->GetFreeGroupSize() : -1;
	if (!parsed) {
		desc += wxT(" ?");
	}
	desc += wxString::Format(_(" [Free:%sbytes(%sgroups)]")
		, free_size >= 0 ? wxNumberFormatter::ToString((long)free_size) : wxT("---")
		, free_groups >= 0 ? wxNumberFormatter::ToString((long)free_groups) : wxT("---")
	);
	desc_size = desc;
	return desc_size;
}

/// FATエリアの空き状況を取得
void DiskBasic::GetFatAvailability(wxUint32 *offset, const wxArrayInt **arr) const
{
	if (type) {
		type->GetFatAvailability(offset, arr);
	}
}

/// ディレクトリのアイテムを取得
DiskBasicDirItem *DiskBasic::GetDirItem(size_t pos)
{
	return dir->ItemPtr(pos);
}

/// DISK BASICで使用できる残りディスクサイズに足りるか
/// @param [in] size 指定サイズ
/// @return true 足りる / false 足りない
bool DiskBasic::HasFreeDiskSize(int size)
{
	bool enough = true;
	// ディスクに空きがあるか
	if (size > (int)disk->GetSizeWithoutHeader()) {
		errinfo.SetError(DiskBasicError::ERR_FILE_TOO_LARGE);
		enough = false;
	} else if (size > (int)type->GetFreeDiskSize()) {
		errinfo.SetError(DiskBasicError::ERR_NOT_ENOUGH_FREE);
		enough = false;
	} else if (!type->IsEnoughFileSize(size)) {
		errinfo.SetError(DiskBasicError::ERR_NOT_ENOUGH_FREE);
		enough = false;
	}
	return enough;
}

/// DISK BASICで使用できる残りディスクサイズを返す
int DiskBasic::GetFreeDiskSize() const
{
	return type->GetFreeDiskSize();
}

/// 固有のパラメータをセット
void DiskBasic::AssignParameter()
{
	data_start_sector = type->CalcDataStartSectorPos();
	skipped_track = type->CalcSkippedTrack();
}

/// 現在選択しているディスクのFAT領域をアサイン
/// @param [in]  is_formatting フォーマット中か
/// @retval 1.0  正常
/// @retval <1.0 警告あり
/// @retval <0.0 エラーあり
double DiskBasic::AssignFat(bool is_formatting)
{
	if (!disk) return -1;
	if (assigned) return 0;

	fat->Empty();

	// 固有のパラメータ
	AssignParameter();

	return fat->Assign(is_formatting);
}

/// 現在選択しているディスクのルートディレクトリ構造をチェック
/// @param [in] is_formatting フォーマット中か
/// @return <0.0 ディレクトリにエラーあり
double DiskBasic::CheckRootDirectory(bool is_formatting)
{
	if (!disk) return -1.0;
	if (assigned) return 1.0;

	return dir->CheckRoot(type, GetDirStartSector(), GetDirEndSector(), is_formatting);
}

/// 現在選択しているディスクのルートディレクトリをアサイン
/// @return true / false ディレクトリにエラーあり
bool DiskBasic::AssignRootDirectory()
{
	if (!disk) return false;

	bool valid = true;
	if (!assigned) {
		// ルートをアサインする
		valid = dir->AssignRoot(type, GetDirStartSector(), GetDirEndSector());
	} else {
		// 既にアサイン済み、ルートをカレントにする
		dir->SetCurrentAsRoot();
	}
	if (valid) {
		// 残りサイズ計算
		type->CalcDiskFreeSize(false);
	} else {
		type->ClearDiskFreeSize();
	}

	assigned = valid;

	return valid;
}

/// 現在選択しているディスクのFATとルートディレクトリをアサイン
/// @return true / false エラーあり
bool DiskBasic::AssignFatAndDirectory()
{
	bool valid = true;

	// fat area
	valid = (AssignFat(false) >= 0.0);

	// directory area
	valid = valid && AssignRootDirectory();

	return valid;
}

/// 解析済みをクリア
void DiskBasic::ClearParseAndAssign()
{
	parsed = false;
	assigned = false;
	dir->ReleaseRoot(type);
	dir->SetCurrentAsRoot();
}

/// ロードできるか
/// @param [in] item ディレクトリのアイテム
bool DiskBasic::IsLoadableFile(DiskBasicDirItem *item)
{
	if (!item || !item->IsLoadable() || !item->IsUsed()) {
		errinfo.SetError(DiskBasicError::ERRV_CANNOT_EXPORT, item->GetFileNameStr().wc_str());
		return false;
	}
	return true;
}

/// 指定したディレクトリ位置のファイルをロード
/// @param [in] item_number ディレクトリの位置
/// @param [in] dstpath 出力先パス
bool DiskBasic::LoadFile(int item_number, const wxString &dstpath)
{
	DiskBasicDirItem *item = dir->ItemPtr(item_number);
	return LoadFile(item, dstpath);
}

/// 指定したディレクトリアイテムのファイルをロード
/// @param [in] item ディレクトリのアイテム
/// @param [in] dstpath 出力先パス
bool DiskBasic::LoadFile(DiskBasicDirItem *item, const wxString &dstpath)
{
	wxFileOutputStream file(dstpath);
	if (!file.IsOk() || !file.GetFile()->IsOpened()) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_EXPORT);
		return false;
	}
	return LoadFile(item, file);
}

/// 指定したストリームにファイルをロード
/// @param [in]     item    ディレクトリのアイテム
/// @param [in,out] ostream 出力先ストリーム
bool DiskBasic::LoadFile(DiskBasicDirItem *item, wxOutputStream &ostream)
{
	// ディスクイメージからデータを取り出す
	wxMemoryOutputStream otemp;
	bool sts = LoadData(item, otemp);
	if (!sts) {
		return false;
	}
	if (otemp.GetLength() == 0) {
		return true;
	}
	// 必要なら取り出したデータ内容を変換・置換してファイルに出力
	wxMemoryInputStream itemp(otemp);
	sts = type->ConvertDataForLoad(item, itemp, ostream);
	return sts;
}

/// 指定したアイテムのファイルをベリファイ
/// @param [in] item ディレクトリのアイテム
/// @param [in] srcpath 比較するファイルのパス
/// @return 0:差異なし 1:差異あり -1:エラー
int DiskBasic::VerifyFile(DiskBasicDirItem *item, const wxString &srcpath)
{
	wxFileInputStream file(srcpath);
	if (!file.IsOk() || !file.GetFile()->IsOpened()) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_VERIFY);
		return -1;
	}
	// ファイルを必要なら変換
	wxMemoryOutputStream otemp;
	if (!type->ConvertDataForVerify(item, file, otemp)) {
		return -1;
	}
	// 変換した内容と内部ファイルとをベリファイ
	wxMemoryInputStream itemp(otemp);
	int sts = VerifyData(item, itemp);
	return sts;
}

/// 指定したストリームにファイルをロード
/// @param [in,out] item         ディレクトリアイテム 
/// @param [in,out] ostream      エクスポート時指定
/// @param [out]    outsize      実際に出力したサイズ(ostreamを指定した時のみ有効)
bool DiskBasic::LoadData(DiskBasicDirItem *item, wxOutputStream &ostream, size_t *outsize)
{
	int sts = 0;
	for(int fileunit_num = 0; sts == 0; fileunit_num++) {
		if (!item->IsValidFileUnit(fileunit_num)) {
			break;
		}
		sts = AccessUnitData(fileunit_num, item, NULL, &ostream, outsize);
	}
	return (sts == 0);
}


/// 指定したアイテムのファイルをベリファイ
/// @param [in,out] item         ディレクトリアイテム 
/// @param [in,out] istream      ベリファイ時指定 
/// @return 0:差異なし 1:差異あり -1:エラー
int DiskBasic::VerifyData(DiskBasicDirItem *item, wxInputStream &istream)
{
	int sts = 0;
	int file_offset = 0;

	istream.SeekI(0);
	for(int fileunit_num = 0; sts == 0; fileunit_num++) {
		int sizeremain = item->GetFileUnitSize(fileunit_num, istream, file_offset);
		if (sizeremain < 0) {
			break;
		}
		sts = AccessUnitData(fileunit_num, item, &istream, NULL);
		file_offset += sizeremain;
	}
	return sts;
}

/// ディスクデータにアクセス（ロード/ベリファイで使用）
/// @param [in]     fileunit_num ファイル番号
/// @param [in,out] item         ディレクトリアイテム 
/// @param [in,out] istream      ベリファイ時指定 
/// @param [in,out] ostream      エクスポート時指定
/// @param [out]    outsize      実際に出力したサイズ(ostreamを指定した時のみ有効)
/// @return 0:差異なし 1:差異あり -1:エラー
int DiskBasic::AccessUnitData(int fileunit_num, DiskBasicDirItem *item, wxInputStream *istream, wxOutputStream *ostream, size_t *outsize)
{
	if (!item) {
		errinfo.SetError(DiskBasicError::ERR_FILE_NOT_FOUND);
		return -1;
	}

	int sector_start = 0;
	int sector_end = 0;
	int rc = 0;

	wxFileOffset osize = 0;
	if (ostream) osize = ostream->TellO();

	// ファイルのトラック番号、サイド番号、セクタ番号を計算しリストにする
	DiskBasicGroups gitems;
	item->GetUnitGroups(fileunit_num, gitems);

	int remain = item->GetFileSize();
	if (remain == 0) {
		// ディレクトリエントリのファイルサイズが０ならグループ数から計算したサイズを得る
		remain = (int)gitems.GetSize();
	}

	// アクセス前に機種固有の処理を行う
	if (!type->PrepareToAccessFile(fileunit_num, item, istream, ostream, remain, gitems, errinfo)) {
		return -1;
	}

	int gidx_end = (int)gitems.Count() - 1;
	for(int gidx = 0; gidx <= gidx_end && remain > 0 && rc == 0; gidx++) {
		DiskBasicGroupItem *gitem = &gitems.Item(gidx);

		sector_start = gitem->GetSectorStart();
		sector_end = gitem->GetSectorEnd();

		for(int block_num = sector_start; block_num <= sector_end && remain > 0; block_num++) {
			DiskImageSector *sector = GetSector(block_num);
			if (!sector) {
				// セクタがない！
				errinfo.SetError(DiskBasicError::ERRV2_NO_SECTOR, gitem->GetGroup(), block_num);
				rc = -1;
				continue;
			}
			int bufsize = sector->GetSectorSize();
//			bufsize /= gitem->div_nums;
			wxUint8 *buf = sector->GetSectorBuffer();
//			buf += (bufsize * gitem->div_num);

			// データの読み込み
			bufsize = type->AccessFile(fileunit_num, item, istream, ostream, buf, bufsize, remain, block_num, sector_end);
			if (bufsize < 0) {
				if (bufsize == -2) {
					// セクタがおかしいぞ
					errinfo.SetError(DiskBasicError::ERRV3_INVALID_SECTOR, gitem->GetGroup(), block_num, bufsize);
					rc = -1;
				} else {
					// データが異なる
					errinfo.SetError(DiskBasicError::ERRV2_VERIFY_FILE, gitem->GetGroup(), block_num);
					rc = 1;
				}
				break;
			}
			remain -= bufsize;
		}
	}

	if (ostream) osize = ostream->TellO() - osize;

	if (outsize) *outsize += (size_t)osize;

	return rc;
}

/// 同じファイル名が既に存在して上書き可能か
/// @param [in]  filename     ファイル名
/// @param [in]  exclude_item 検索対象から除くアイテム
/// @param [out] next_item    一致したアイテムの次位置にあるアイテム
/// @retval  0  なし
/// @retval  1  あり 通常ファイル
/// @retval  -1 あり 上書き不可（ディレクトリ or ボリュームラベル）
int DiskBasic::IsFileNameDuplicated(const DiskBasicFileName &filename, DiskBasicDirItem *exclude_item, DiskBasicDirItem **next_item)
{
	DiskBasicDirItem *item = dir->FindFile(filename, IsCompareCaseInsense(), exclude_item, next_item);
	if (item == NULL) {
		return 0;
	}
	return (item->IsOverWritable() ? 1 : -1);
}

/// 同じファイル名が既に存在して上書き可能か
/// @param [in]  target_item  アイテム
/// @param [in]  exclude_item 検索対象から除くアイテム
/// @param [out] next_item    一致したアイテムの次位置にあるアイテム
/// @retval  0  なし
/// @retval  1  あり 通常ファイル
/// @retval  -1 あり 上書き不可（ディレクトリ or ボリュームラベル）
int DiskBasic::IsFileNameDuplicated(const DiskBasicDirItem *target_item, DiskBasicDirItem *exclude_item, DiskBasicDirItem **next_item)
{
	DiskBasicDirItem *item = dir->FindFile(target_item, IsCompareCaseInsense(), exclude_item, next_item);
	if (item == NULL) {
		return 0;
	}
	return (item->IsOverWritable() ? 1 : -1);
}

/// 書き込みできるか
bool DiskBasic::IsWritableIntoDisk()
{
	errinfo.Clear();
	// ディスク非選択
	if (!disk) {
		errinfo.SetError(DiskBasicError::ERR_UNSELECT_DISK);
		return false;
	}
	// 書き込み処理に対応しているか
	if (!type->SupportWriting()) {
		errinfo.SetError(DiskBasicError::ERR_WRITE_UNSUPPORTED);
		return false;
	}
	// ライトプロテクトかかっているか？
	if (disk->IsWriteProtected()) {
		errinfo.SetError(DiskBasicError::ERR_WRITE_PROTECTED);
		return false;
	}
	return true;
}

/// 指定ファイルのサイズでディスクに書き込めるかをチェック
/// @param [in]  srcpath    ファイルパス
/// @param [out] file_size  ファイルのサイズを返す
bool DiskBasic::CheckFile(const wxString &srcpath, int *file_size)
{
	// ディスクに書き込めるか
	if (!IsWritableIntoDisk()) {
		return false;
	}
	// 通常のファイルか
	wxFileName srcfile(srcpath);
	if (!srcfile.FileExists() && srcfile.DirExists()) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_IMPORT_DIRECTORY);
		return false;
	}

	wxFileInputStream infile(srcpath);
	// ファイル読めるか
	if (!infile.IsOk() || !infile.GetFile()->IsOpened()) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_IMPORT);
		return false;
	}

	int size = (int)infile.GetLength();
	if (file_size) *file_size = size;

#ifndef DEBUG_DISK_FULL_TEST
	// 空きがあるか
	if (!HasFreeDiskSize(size)) {
		return false;
	}
#endif

	return true;
}

/// 指定ファイルをディスクイメージにセーブ
/// @param [in]     srcpath  元ファイルのあるパス
/// @param [in,out] pitem    ファイル名、属性を持っているディレクトリアイテム
/// @param [out]    nitem    確保したディレクトリアイテム
/// @return false:エラーあり
bool DiskBasic::SaveFile(const wxString &srcpath, DiskBasicDirItem *pitem, DiskBasicDirItem **nitem)
{
	if (!IsWritableIntoDisk()) return false;

	wxFileInputStream infile(srcpath);
	// ファイル読めるか
	if (!infile.IsOk() || !infile.GetFile()->IsOpened()) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_IMPORT);
		return false;
	}

	// セーブ
	return SaveFile(infile, pitem, nitem);
}

/// バッファデータをディスクイメージにセーブ
/// @param [in]     buffer   データ
/// @param [in]     buflen   データサイズ
/// @param [in,out] pitem    ファイル名、属性を持っているディレクトリアイテム
/// @param [out]    nitem    確保したディレクトリアイテム
/// @return false:エラーあり
bool DiskBasic::SaveFile(const wxUint8 *buffer, size_t buflen, DiskBasicDirItem *pitem, DiskBasicDirItem **nitem)
{
	if (!IsWritableIntoDisk()) return false;

	wxMemoryInputStream indata(buffer, buflen);
	// データ読めるか
	if (!indata.IsOk()) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_IMPORT);
		return false;
	}

	// セーブ
	return SaveFile(indata, pitem, nitem);
}

/// ストリームデータをディスクイメージにセーブ
/// @param [in]     istream  ストリームバッファ
/// @param [in,out] pitem    ファイル名、属性を持っている仮ディレクトリアイテム
/// @param [out]    nitem    確保したディレクトリアイテム
/// @return false:エラーあり
bool DiskBasic::SaveFile(wxInputStream &istream, DiskBasicDirItem *pitem, DiskBasicDirItem **nitem)
{
	DiskBasicDirItem *next_item = NULL;
	DiskBasicDirItem *item = dir->FindFile(pitem, IsCompareCaseInsense(), NULL, &next_item);

	bool valid = true;

	if (item == NULL) {
		// 追加の場合
		// 新しいディレクトリアイテムを確保
		while((item = dir->GetEmptyItemOnCurrent(pitem, &next_item)) == NULL) {
			// 確保できない時
			// ディレクトリエリアを拡張する
			if (dir->CanExpand()) {
				valid = dir->Expand();
			} else {
				valid = false;
			}
			if (!valid) {
				// 拡張できない
				errinfo.SetError(DiskBasicError::ERR_DIRECTORY_FULL);
				return false;
			}
		}
		item->SetEndMark(next_item);
	} else {
		// すでにあるアイテムを削除する
		if (!this->DeleteFile(item, false)) {
			return false;
		}
	}
	if (nitem) *nitem = item;

	// ファイル名属性をクリア
	item->ClearData();
	// ファイル名属性を設定
	item->CopyItem(*pitem);

	// 入力ストリームのデータを変換する
	wxMemoryOutputStream otemp;
	if (!type->ConvertDataForSave(item, istream, otemp)) {
		// 削除する
		this->DeleteFile(item, false);
		return false;
	}

	int file_size = 0;
	DiskBasicGroups group_items;
	wxInputStream *itemp = NULL;

	if (otemp.GetLength() > 0) {
		itemp = new wxMemoryInputStream(otemp);
	} else {
		itemp = &istream;
	}
	do {
		valid = SaveData(*itemp, pitem, item, group_items, file_size);

		// ファイルサイズ
		item->SetFileSize(file_size);

		if (!valid) {
			// エラーの場合は消す
			this->DeleteFile(item, group_items);
			break;
		}
		// 変更された
		item->Refresh();
		item->SetModify();
		// グループ数を計算
		item->CalcFileSize();

		// ベリファイ
		int sts = VerifyData(item, *itemp);
		valid = (sts == 0);

		// 機種個別の処理を行う
		type->AdditionalProcessOnSavedFile(item);

		// 空きサイズを計算
		type->CalcDiskFreeSize(true);

	} while(0);

	if (otemp.GetLength() > 0) {
		delete itemp;
	}

	return valid;
}

/// ストリームデータをディスクイメージにセーブ
/// @param [in]     istream     ストリームバッファ
/// @param [in,out] pitem       ファイル名、属性を持っている仮ディレクトリアイテム
/// @param [in,out] item        確保したディレクトリアイテム
/// @param [out]    group_items グループリスト
/// @param [out]    file_size   セーブしたファイルのサイズ
bool DiskBasic::SaveData(wxInputStream &istream, DiskBasicDirItem *pitem, DiskBasicDirItem *item, DiskBasicGroups &group_items, int &file_size)
{
	bool valid = true;
	int file_offset = 0;

	for(int fileunit_num = 0; valid; fileunit_num++) {
		// 出力するファイル数から必要なサイズを得る
		int isize = item->GetFileUnitSize(fileunit_num, istream, file_offset);
		if (isize < 0) {
			// 終了
			break;
		}
		valid = SaveUnitData(fileunit_num, istream, isize, pitem, item, group_items, file_size);

		file_offset += isize;
	}

	return valid;
}

/// ストリームデータをディスクイメージにセーブ
/// @param [in]     fileunit_num  ファイル番号
/// @param [in]     istream       ストリームバッファ
/// @param [in]     isize         バッファ内のセーブ対象データサイズ
/// @param [in,out] pitem         ファイル名、属性を持っている仮ディレクトリアイテム
/// @param [in,out] item          確保したディレクトリアイテム
/// @param [out]    group_items   グループリスト
/// @param [out]    file_size     セーブしたファイルのサイズ
bool DiskBasic::SaveUnitData(int fileunit_num, wxInputStream &istream, int isize, DiskBasicDirItem *pitem, DiskBasicDirItem *item, DiskBasicGroups &group_items, int &file_size)
{
	// ファイルをセーブする前の準備を行う
	if (!type->PrepareToSaveFile(istream, isize, pitem, item, errinfo)) {
		return false;
	}

	// ファイルサイズを再計算する(終端コードが必要な場合など)
	isize = item->RecalcFileSizeOnSave(&istream, isize);
	
#ifndef DEBUG_DISK_FULL_TEST
	// ディスクに空きがあるか
	if (!HasFreeDiskSize(isize)) {
		return false;
	}
#endif

	// 必要なグループを確保
	DiskBasicGroups gitems;
	int rc = type->AllocateUnitGroups(fileunit_num, item, isize, ALLOCATE_GROUPS_NEW, gitems);
	group_items.Add(gitems);
	if (rc < 0) {
		// 空きなし
		errinfo.SetError(DiskBasicError::ERR_DISK_FULL);
		return false;
	}

	// セクタに書き込む
	int block_num = 0;
	wxUint32 group_num, next_group;
	int sector_start, sector_end;
	int seq_num = 0;
	for(int gidx = 0; gidx < (int)gitems.Count(); gidx++) {
		DiskBasicGroupItem *gitem = &gitems.Item(gidx);
		group_num = gitem->GetGroup();
		next_group = gitem->GetNextGroup();

		sector_start = gitem->GetSectorStart();
		sector_end = gitem->GetSectorEnd();

		for(block_num = sector_start; block_num <= sector_end; block_num++) {
			DiskImageSector *sector = GetSector(block_num);
			if (!sector) {
				errinfo.SetError(DiskBasicError::ERRV2_NO_SECTOR, gitem->GetGroup(), block_num);
				rc = -2;
				continue;
			}
			int bufsize = sector->GetSectorSize();
//			bufsize /= gitem->div_nums;
			wxUint8 *buf = sector->GetSectorBuffer();
//			buf += (bufsize * gitem->div_num);

			// ディスク内に書き込む
			int last_size = type->WriteFile(item, istream, buf, bufsize, isize, block_num, group_num, next_group, sector_end, seq_num);
			isize -= last_size;
			file_size += last_size;
			seq_num++;
		}
	}

	return (rc >= 0);
}

/// ファイルを削除できるか
bool DiskBasic::IsDeletableFiles()
{
	errinfo.Clear();
	if (!type || !type->SupportDeleting()) {
		errinfo.SetError(DiskBasicError::ERR_DELETE_UNSUPPORTED);
		return false;
	}
	if (disk->IsWriteProtected()) {
		errinfo.SetError(DiskBasicError::ERR_WRITE_PROTECTED);
		return false;
	}
	return true;
}

/// 指定したファイルを削除できるか
/// @param [in]  item        ディレクトリアイテム
/// @param [in]  clearmsg    エラーメッセージのバッファをクリアするか
/// @return -1:エラー継続不可 1:エラー継続可能
int DiskBasic::IsDeletableFile(DiskBasicDirItem *item, bool clearmsg)
{
	if (clearmsg) errinfo.Clear();

	if (disk->IsWriteProtected()) {
		errinfo.SetError(DiskBasicError::ERR_WRITE_PROTECTED);
		return -1;
	}

	if (!item) return 0;

	// 削除できるか
	if (!item->IsDeletable()) {
		errinfo.SetError(DiskBasicError::ERRV_CANNOT_DELETE, item->GetFileNameStr().wc_str());
		return 1;
	}
	return 0;
}

/// 指定したディレクトリが空か
/// @param [in]  item        ディレクトリアイテム
/// @param [out] group_items グループ番号一覧
/// @param [in]  clearmsg    エラーメッセージのバッファをクリアするか
bool DiskBasic::IsEmptyDirectory(DiskBasicDirItem *item, DiskBasicGroups &group_items, bool clearmsg)
{
	if (clearmsg) errinfo.Clear();

	// ファイルのトラック番号、サイド番号、セクタ番号を計算しリストにする
	item->GetAllGroups(group_items);

	if (item->IsDirectory()) {
		// ディレクトリの場合は空かどうかをチェック
		if (!type->IsEmptyDirectory(false, group_items)) {
			errinfo.SetError(DiskBasicError::ERRV_CANNOT_DELETE_DIRECTORY, item->GetFileNameStr().wc_str());
			return false;
		}
	}
	return true;
}

/// ファイルを削除
/// @param [in] item         ディレクトリアイテム
/// @param [in]  clearmsg    エラーメッセージのバッファをクリアするか
/// @return true 成功 / false 失敗
bool DiskBasic::DeleteFile(DiskBasicDirItem *item, bool clearmsg)
{
	if (!item) return false;

	if (clearmsg) errinfo.Clear();
	DiskBasicGroups group_items;
	item->GetAllGroups(group_items);
	return DeleteFile(item, group_items);
}

/// ファイルを削除
/// @param [in] item        ディレクトリアイテム
/// @param [in] group_items グループ番号一覧
/// @return true 成功 / false 失敗
bool DiskBasic::DeleteFile(DiskBasicDirItem *item, const DiskBasicGroups &group_items)
{
	if (!disk) return false;

	if (!item) {
		errinfo.SetError(DiskBasicError::ERR_FILE_NOT_FOUND);
		return false;
	}

	// FATエントリを削除
	type->DeleteGroups(group_items);

	// ディレクトリエントリを削除
	item->Delete();

	// 削除時の追加処理
	if (!type->AdditionalProcessOnDeletedFile(item)) {
		errinfo.SetError(DiskBasicError::ERRV_CANNOT_DELETE, item->GetFileNameStr().wc_str());
		return false;
	}

	// 子ディレクトリなどを削除
	item->EmptyChildren();

	item->Refresh();
	item->SetModify();

	// 空きサイズを計算
	type->CalcDiskFreeSize(true);

	// 必要ならアイテムも削除
	type->ReleaseDirectoryItem(item);

	return true;
}

/// ファイル名や属性を更新できるか
/// @param [in] item        ディレクトリアイテム
/// @param [in] showmsg     エラーメッセージをセットするか
/// @return true できる / false できない
bool DiskBasic::CanRenameFile(DiskBasicDirItem *item, bool showmsg)
{
	errinfo.Clear();
	if (!item) {
		if (showmsg) {
			errinfo.SetError(DiskBasicError::ERR_FILE_NOT_FOUND);
		}
		return false;
	}
	if (disk->IsWriteProtected()) {
		if (showmsg) {
			errinfo.SetError(DiskBasicError::ERR_WRITE_PROTECTED);
		}
		return false;
	}
	if (!item->IsFileNameEditable()) {
		if (showmsg) {
			wxString filename = item->GetFileNameStr();
			errinfo.SetError(DiskBasicError::ERRV_CANNOT_EDIT_NAME, filename.wc_str());
		}
		return false;
	}
	return true;
}

/// ファイル名を更新
/// @param [in] item        ディレクトリアイテム
/// @param [in] newname     ファイル名
/// @return true
bool DiskBasic::RenameFile(DiskBasicDirItem *item, const wxString &newname)
{
	// ファイル名更新
	if (item->IsFileNameEditable()) {
		item->SetFileNameStr(newname);
	}

	// 変更されたか
	item->Refresh();
	item->SetModify();

	// ファイル名を更新した後の個別処理
	type->AdditionalProcessOnRenamedFile(item);

	return true;
}

/// 属性を更新
/// @param [in] item        ディレクトリアイテム
/// @param [in] attr        属性値
bool DiskBasic::ChangeAttr(DiskBasicDirItem *item, DiskBasicDirItemAttr &attr)
{
	// 名前を更新
	if (attr.IsRenameable()) {
		item->SetOptionalName(attr.GetFileName().GetOptional());
		bool sts = RenameFile(item, attr.GetFileName().GetName());
		if (!sts) return sts;
	}
	// 属性更新
	if (!attr.DoesIgnoreFileAttr()) {
		item->SetFileAttr(attr.GetFileAttr());
	}
	// 開始アドレス更新
	if (attr.GetStartAddress() >= 0) {
		item->SetStartAddress(attr.GetStartAddress());
	}
	// 終了アドレス更新
	if (attr.GetEndAddress() >= 0) {
		item->SetEndAddress(attr.GetEndAddress());
	}
	// 実行アドレス更新
	if (attr.GetExecuteAddress() >= 0) {
		item->SetExecuteAddress(attr.GetExecuteAddress());
	}
	// 日時更新
	bool ignore_datetime = attr.DoesIgnoreDateTime();
	DiskBasicDirItem::enDateTime ignore_type = item->CanIgnoreDateTime();
	if (!(ignore_datetime && (ignore_type & DiskBasicDirItem::DATETIME_CREATE) != 0)) {
		item->SetFileCreateDateTime(attr.GetCreateDateTime());
	}
	if (!(ignore_datetime && (ignore_type & DiskBasicDirItem::DATETIME_MODIFY) != 0)) {
		item->SetFileModifyDateTime(attr.GetModifyDateTime());
	}
	if (!(ignore_datetime && (ignore_type & DiskBasicDirItem::DATETIME_ACCESS) != 0)) {
		item->SetFileAccessDateTime(attr.GetAccessDateTime());
	}
	// 他の属性
	item->SetOptionalAttr(attr);

	// 変更されたか
	item->Refresh();
	item->SetModify();

	// 属性変更後の個別処理
	type->AdditionalProcessOnChangedAttr(item);

	return true;
}

/// DISK BASIC用にフォーマットされているか
bool DiskBasic::IsFormatted() const
{
	return (disk != NULL && formatted);
}

/// DISK BASIC用にフォーマットできるか
bool DiskBasic::IsFormattable()
{
	errinfo.Clear();

	bool enable = (type != NULL);
	if (!enable) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_FORMAT);
		return enable;
	}
	enable = type->SupportFormatting();
	if (!enable) {
		errinfo.SetError(DiskBasicError::ERR_FORMAT_UNSUPPORTED);
	}

	return enable;
}

/// ディスクを論理フォーマット
/// @param [in] data 機種依存データ（ボリューム名など）
/// @retval >0:ワーニング
/// @retval  0:正常
/// @retval <0:エラーあり
int DiskBasic::FormatDisk(const DiskBasicIdentifiedData &data)
{
	errinfo.Clear();
	if (!disk) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_FORMAT);
		return errinfo.GetValid();
	}

	parsed = false;
	assigned = false;

	bool rc = true;
	// セクタを埋める
	for(int block_num = 0; block_num < (int)disk->GetNumberOfSectors(); block_num++) {
		DiskImageSector *sector = disk->GetSector(block_num);
		// コードで埋める
		type->FillSector(sector);
	}

	// 初期データをセット
	if (!type->AdditionalProcessOnFormatted(data)) {
		errinfo.SetError(DiskBasicError::ERR_FORMATTING);
		rc = false;
	}

	if (rc) {
		formatted = true;
	}
	return errinfo.GetValid();
}

/// ルートディレクトリを返す
DiskBasicDirItem *DiskBasic::GetRootDirectory()
{
	return dir->GetRootItem();
}
/// ルートディレクトリ内の一覧を返す
DiskBasicDirItems *DiskBasic::GetRootDirectoryItems(DiskBasicDirItem **dir_item)
{
	return dir->GetRootItems(dir_item);
}

/// カレントディレクトリを返す
DiskBasicDirItem *DiskBasic::GetCurrentDirectory()
{
	return dir->GetCurrentItem();
}
/// カレントディレクトリ内の一覧を返す
DiskBasicDirItems *DiskBasic::GetCurrentDirectoryItems(DiskBasicDirItem **dir_item)
{
	return dir->GetCurrentItems(dir_item);
}

/// ディレクトリをアサイン
/// @param [in] dir_item ディレクトリのアイテム
bool DiskBasic::AssignDirectory(DiskBasicDirItem *dir_item)
{
	if (!disk) return false;

	return dir->Assign(dir_item);
}

/// ディレクトリを変更
/// @param [in,out] dst_item 移動先ディレクトリのアイテム
bool DiskBasic::ChangeDirectory(DiskBasicDirItem * &dst_item)
{
	if (!disk) return false;

	bool valid = dir->Change(dst_item);
	if (valid) {
		// 残りサイズ計算
		type->CalcDiskFreeSize(false);
	}
	return valid;
}

/// サブディレクトリの作成できるか
bool DiskBasic::CanMakeDirectory() const
{
	return (type->CanMakeDirectory()) && (GetSubDirGroupSize() > 0);
}

/// サブディレクトリの作成
/// @param [in] filename  ディレクトリ名
/// @param [in] ignore_datetime 日時は設定しないか
/// @param [out] nitem    作成したディレクトリアイテム
/// @return 1:同じ名前がある -1:その他エラー
int DiskBasic::MakeDirectory(const wxString &filename, bool ignore_datetime, DiskBasicDirItem **nitem)
{
	if (!IsWritableIntoDisk()) {
		return -1;
	}

	if (!CanMakeDirectory()) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_MAKE_DIRECTORY);
		return -1;
	}

	DiskBasicFileName dir_name;
	dir_name.SetName(filename);
	// サブディレクトリを作成する前にディレクトリ名を編集する
	if (!type->RenameOnMakingDirectory(dir_name.GetName())) {
		errinfo.SetError(DiskBasicError::ERR_CANNOT_MAKE_DIRECTORY);
		return -1;
	}

	/// 同じファイル名があるか
	DiskBasicDirItem *next_item;
	DiskBasicDirItem *item = dir->FindFile(dir_name, IsCompareCaseInsense(), NULL, &next_item);
	if (item) {
		errinfo.SetError(DiskBasicError::ERR_FILE_ALREADY_EXIST);
		return 1;
	}

	// 仮アイテムを作成
	DiskBasicDirItem *pitem = dir->NewItem();
	// エントリをクリア
	pitem->ClearData();
	// ファイル名＆属性を設定
	pitem->SetFileNameStr(dir_name.GetName());
	pitem->SetFileAttr(FORMAT_TYPE_UNKNOWN, FILE_TYPE_DIRECTORY_MASK, 0);
	// 日時設定
	TM tm;
	DiskBasicDirItem::enDateTime ignore_type = pitem->CanIgnoreDateTime();
	tm.Now();
	if (!(ignore_datetime && (ignore_type & DiskBasicDirItem::DATETIME_CREATE) != 0)) {
		pitem->SetFileCreateDateTime(tm);
	}
	if (!(ignore_datetime && (ignore_type & DiskBasicDirItem::DATETIME_MODIFY) != 0)) {
		pitem->SetFileModifyDateTime(tm);
	}
	if (!(ignore_datetime && (ignore_type & DiskBasicDirItem::DATETIME_ACCESS) != 0)) {
		pitem->SetFileAccessDateTime(tm);
	}

	// 新しいディレクトリアイテムを確保
	while((item = dir->GetEmptyItemOnCurrent(pitem, &next_item)) == NULL) {
		// 確保できない時
		bool valid = false;
		// ディレクトリエリアを拡張する
		if (dir->CanExpand()) {
			valid = dir->Expand();
		}
		if (!valid) {
			// 拡張できない
			errinfo.SetError(DiskBasicError::ERR_DIRECTORY_FULL);
			return -1;
		}
	}
	item->SetEndMark(next_item);

	if (nitem) *nitem = item;


	// ファイル名属性をクリア
	item->ClearData();
	// ファイル名属性を設定
	item->CopyItem(*pitem);

	delete pitem;

	// ディレクトリを作成する前の準備を行う
	if (!type->PrepareToMakeDirectory(item)) {
		// 削除する
		this->DeleteFile(item, false);
		return -1;
	}

	int sizeremain = GetSectorsPerGroup() * disk->GetSectorSize() * GetSubDirGroupSize();
	// 空きがあるか
	if (sizeremain > GetFreeDiskSize()) {
		// 空きが足りない
		errinfo.SetError(DiskBasicError::ERR_DISK_FULL);
		// アイテムに削除マークを入れる
		this->DeleteFile(item, false);
		return -1;
	}

	int file_size = 0;

	int rc;

	// 必要なディスク領域を確保する
	DiskBasicGroups group_items;
	rc = type->AllocateGroups(item, sizeremain, ALLOCATE_GROUPS_NEW, group_items);
	if (rc < 0) {
		// 空きが足りない
		errinfo.SetError(DiskBasicError::ERR_DISK_FULL);
		// 確保した領域を削除
		this->DeleteFile(item, false);
		return -1;
	}

	// セクタに書き込む
	rc = type->InitializeSectorsAsDirectory(group_items, file_size, sizeremain, errinfo);

	// ファイルサイズ
	item->SetFileSize(file_size);

	if (rc < 0) {
		// エラーの場合は消す
		this->DeleteFile(item, false);
		return -1;
	}

	// 変更された
	item->Refresh();
	item->SetModify();

	// ディレクトリ作成後の個別処理
	type->AdditionalProcessOnMadeDirectory(item, group_items, dir->GetCurrentItem());

	// グループ数を計算
	item->CalcFileSize();
	// 空きサイズを計算
	type->CalcDiskFreeSize(true);

	return 0;
}

/// ディレクトリのサイズを拡張
/// @param [in] dir_item ディレクトリのエントリ
bool DiskBasic::ExpandDirectory(DiskBasicDirItem *dir_item)
{
	int rc;

	int sizeremain = GetSubDirGroupSize() * GetSectorsPerGroup() * disk->GetSectorSize();
	DiskBasicGroups group_items;
	rc = type->AllocateGroups(dir_item, sizeremain, ALLOCATE_GROUPS_APPEND, group_items);
	if (rc < 0) {
		// 空きが足りない
		errinfo.SetError(DiskBasicError::ERR_DISK_FULL);
		return false;
	}

	int file_size = dir_item->GetFileSize();

	// セクタに書き込む
	rc = type->InitializeSectorsAsDirectory(group_items, file_size, sizeremain, errinfo);
	if (rc < 0) {
		// エラーの場合
		return false;
	}

	// ファイルサイズを設定
	dir_item->SetFileSize(file_size);
	DiskBasicDirItem *cur_item = dir->FindName(wxT("."), IsCompareCaseInsense(), NULL, NULL);
	if (cur_item) {
		cur_item->SetFileSize(file_size);
	}

	// ディレクトリ拡張後の個別処理
	if (!type->AdditionalProcessOnExpandedDirectory(dir_item, group_items, dir->GetParentItem())) {
		// 空きが足りない
		errinfo.SetError(DiskBasicError::ERR_DIRECTORY_FULL);
		return false;
	}

	return true;
}

/// ディレクトリアイテムの作成
/// @attention 使用後はdeleteすること
DiskBasicDirItem *DiskBasic::CreateDirItem()
{
	return dir->NewItem();
}

#if 0
/// ディレクトリアイテムの作成
/// @attention 使用後はdeleteすること
/// @param [in] n_block_num セクタ番号
/// @param [in] n_position   セクタ内の位置
/// @param [in] n_data       ディレクトリデータのあるポインタ
/// @return ディレクトリアイテム
DiskBasicDirItem *DiskBasic::CreateDirItem(int n_block_num, int n_position, wxUint8 *n_data)
{
	return dir->NewItem(n_block_num, n_position, n_data);
}
#endif

/// ディレクトリアイテムの作成
/// @attention 使用後はdeleteすること
/// @param [in] n_block_num セクタ番号
/// @param [in] n_position   セクタ内の位置
/// @return ディレクトリアイテム
DiskBasicDirItem *DiskBasic::CreateDirItem(int n_block_num, int n_position)
{
	return dir->NewItem(n_block_num, n_position);
}

/// パーティション内のセクタ位置からセクタ返す
/// @param [in] block_num  パーティション内のセクタ位置
/// @return セクタデータ
DiskImageSector	*DiskBasic::GetSector(int block_num)
{
	return disk->GetSector(block_num);
}

#if 0
/// パーティション内のセクタ位置からセクタを返す
/// @param [in] block_num     パーティション内のセクタ位置
/// @param [out] div_num      分割番号
/// @param [out] div_nums     分割数
/// @return セクタデータ
DiskImageSector *DiskBasic::GetSector(int block_num, int *div_num, int *div_nums)
{
	type->GetNumFromSectorPos(block_num, div_num, div_nums);
	return disk->GetSector(block_num);
}
#endif

/// キャッシュを更新する
/// @param [in] block_num     パーティション内のセクタ位置
void DiskBasic::RefreshCache(int block_num)
{
	disk->RefreshCache(block_num);
}

/// グループ番号からセクタ番号を計算してリストに入れる
/// @note 管理エリアがあれば飛ばす、開始グループ番号のオフセット分を引く などの機種依存を考慮
/// @param [in] group_num     グループ番号
/// @param [in] next_group    次のグループ番号
/// @param [in] sector_size   セクタサイズ
/// @param [in] remain_size   残りデータサイズ
/// @param [out] items        トラック、サイド、セクタの各番号が入ったリスト
/// @param [out] end_sector   このグループの最終セクタ番号
/// @return false : グループ番号が範囲外
bool DiskBasic::GetNumsFromGroup(wxUint32 group_num, wxUint32 next_group, int sector_size, int remain_size, DiskBasicGroups &items, int *end_sector)
{
	int sector_start = type->GetStartSectorFromGroup(group_num);
	if (sector_start < 0) {
		return false;
	}

	int sector_end = type->GetEndSectorFromGroup(group_num, next_group, sector_start, sector_size, remain_size);

//	int div_num, div_nums;
	bool first = true;
//	DiskBasicGroupItem itm(group_num, next_group, -1, -1, sector_start, sector_start);
	DiskBasicGroupItem itm(group_num, next_group, sector_start, sector_start);
	for(int block_num = sector_start; block_num <= sector_end; block_num++) {
//		type->GetNumFromSectorPos(block_num, &div_num, &div_nums);
		if (itm.GetSectorEnd() + 1 != block_num) {
			if (!first) {
				items.Add(itm);
			}
			itm.SetSectorStart(block_num);
			first = false;
		}
		itm.SetSectorEnd(block_num);
//		itm.div_num = div_num;
//		itm.div_nums = div_nums;
	}

	items.Add(itm);

	if (end_sector) *end_sector = sector_end;

	return true;
}

/// グループ番号から開始セクタ番号(パーティション内とディスク全体)を計算(グループ計算用)
/// @note 管理エリアがあれば飛ばす、開始グループ番号のオフセット分を引く などの機種依存を考慮
/// @param [in] group_num     グループ番号
/// @param [out] start_block  開始セクタ番号 パーティション内基準
/// @param [out] block_num   開始セクタ番号 ディスク全体
/// @return false : グループ番号が範囲外
bool DiskBasic::CalcStartNumFromGroupNum(wxUint32 group_num, int &start_block, int *sector_pos)
{
	// 最大グループを超えている
	if (group_num > (wxUint32)GetFatEndGroup()) {
		return false;
	}

	start_block = type->GetStartSectorFromGroup(group_num);
	if (start_block < 0) {
		return false;
	}

	if (sector_pos) {
		*sector_pos = start_block + GetDisk()->GetStartSectorNumber();
	}

	return true;
}

#if 0
/// グループ番号から開始セクタ番号(パーティション内とディスク全体)を計算(グループ計算用)
/// @note 管理エリアがあれば飛ばす、開始グループ番号のオフセット分を引く などの機種依存を考慮
/// @param [in] group_num     グループ番号
/// @param [out] start_block  開始セクタ番号 パーティション内基準
/// @param [out] block_num   開始セクタ番号 ディスク全体
/// @param [out] div_num      分割番号
/// @param [out] div_nums     分割数
/// @return false : グループ番号が範囲外
bool DiskBasic::CalcStartNumFromGroupNum(wxUint32 group_num, int &start_block, int *sector_pos, int *div_num, int *div_nums)
{
	type->GetNumFromSectorPos(start_block, div_num, div_nums);
	return CalcStartNumFromGroupNum(group_num, start_block, sector_pos);
}
#endif

/// グループ番号から開始セクタを返す
/// @param [in]  group_num : グループ番号
DiskImageSector *DiskBasic::GetSectorFromGroup(wxUint32 group_num)
{
	int start_sector = type->GetStartSectorFromGroup(group_num);
	return GetSector(start_sector);
}

#if 0
/// グループ番号から開始セクタを返す
/// @param [in]  group_num : グループ番号
/// @param [out] div_num   : 分割番号
/// @param [out] div_nums  : 分割数
DiskImageSector *DiskBasic::GetSectorFromGroup(wxUint32 group_num, int *div_num, int *div_nums)
{
	int start_sector = type->GetStartSectorFromGroup(group_num);
	type->GetNumFromSectorPos(start_sector, div_num, div_nums);
	return GetSector(start_sector);
}
#endif

/// ディレクトリアイテムの位置から開始セクタを返す
DiskImageSector *DiskBasic::GetSectorFromPosition(size_t position, wxUint32 *start_group)
{
	DiskBasicDirItem *item = dir->ItemPtr(position);
	if (!item) return NULL;

	wxUint32 gnum = item->GetStartGroup(0);
	if (start_group) *start_group = gnum;
	return GetSectorFromGroup(gnum);
}

/// 開始セクタ番号を返す
/// @note DiskBasicParamを優先
int DiskBasic::GetSectorNumberBase() const
{
//	int val = GetSectorNumberBaseOnBasic();
	DiskImageFile *file = disk->GetFile();
	int val = file->GetSectorNumberBaseOnDisk();
	return val;
}

/// キャラクターコードの文字体系を設定
void DiskBasic::SetCharCode(const wxString &name)
{
	if (char_code == name) return;

	char_code = name;
	codes.SetMap(name);
}

/// エラーメッセージ
const wxArrayString &DiskBasic::GetErrorMessage(int maxrow)
{
	return errinfo.GetMessages(maxrow);
}
/// エラー有無
/// @return <0:エラー 0:正常 0>:ワーニング
int DiskBasic::GetErrorLevel(void) const
{
	return errinfo.GetValid();
}
/// エラーメッセージを表示
void DiskBasic::ShowErrorMessage()
{
	ResultInfo::ShowMessage(GetErrorLevel(), GetErrorMessage());
}
/// エラーメッセージをクリア
void DiskBasic::ClearErrorMessage()
{
	errinfo.Clear();
}

#if 0
/// 必要ならデータを反転する
wxUint8 DiskBasic::InvertUint8(wxUint8 val) const
{
	return IsDataInverted() ? val ^ 0xff : val;
}

/// 必要ならデータを反転する
wxUint16 DiskBasic::InvertUint16(wxUint16 val) const
{
	return IsDataInverted() ? val ^ 0xffff : val;
}

/// 必要ならデータを反転する＆エンディアンを考慮
wxUint16 DiskBasic::InvertAndOrderUint16(wxUint16 val) const
{
	val = IsDataInverted() ? val ^ 0xffff : val;
	return OrderUint16(val);
}

/// 必要ならデータを反転する
wxUint32 DiskBasic::InvertUint32(wxUint32 val) const
{
	return IsDataInverted() ? ~val : val;
}

/// 必要ならデータを反転する＆エンディアンを考慮
wxUint32 DiskBasic::InvertAndOrderUint32(wxUint32 val) const
{
	val = IsDataInverted() ? ~val : val;
	return OrderUint32(val);
}

/// 必要ならデータを反転する
void DiskBasic::InvertMem(void *val, size_t len) const
{
	if (IsDataInverted()) mem_invert(val, len);
}

/// 必要ならデータを反転する
void DiskBasic::InvertMem(const wxUint8 *src, size_t len, wxUint8 *dst) const
{
	memcpy(dst, src, len);
	if (IsDataInverted()) mem_invert(dst, len);
}
#endif

/// エンディアンを考慮した値を返す
wxUint16 DiskBasic::OrderUint16(wxUint16 val) const
{
	return IsBigEndian() ? wxUINT16_SWAP_ON_LE(val) : wxUINT16_SWAP_ON_BE(val); 
}

/// エンディアンを考慮した値を返す
wxUint32 DiskBasic::OrderUint32(wxUint32 val) const
{
	return IsBigEndian() ? wxUINT32_SWAP_ON_LE(val) : wxUINT32_SWAP_ON_BE(val); 
}

/// DISK BASIC種類番号を返す
DiskBasicFormatType DiskBasic::GetFormatTypeNumber() const
{
	return GetFormatType()->GetTypeNumber();
}
