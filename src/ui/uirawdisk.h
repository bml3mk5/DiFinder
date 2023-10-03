/// @file uirawdisk.h
///
/// @brief ディスクID一覧
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef UIRAWDISK_H
#define UIRAWDISK_H

#include "uicommon.h"
#include <wx/string.h>
#include <wx/splitter.h>
#include "../diskimg/diskimage.h"

#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
#include "uicdlistctrl.h"
#else
#include "uiclistctrl.h"
#endif

class MyMenu;
class wxFileDataObject;
class wxSlider;
class UiDiskFrame;
class UiDiskRawTrack;
class UiDiskRawSector;

//////////////////////////////////////////////////////////////////////

enum en_sector_list_columns {
	SECTORCOL_NUM = 0,
	SECTORCOL_ID_C,
	SECTORCOL_ID_H,
	SECTORCOL_ID_R,
	SECTORCOL_SECNUM,
//	SECTORCOL_DELETED,
//	SECTORCOL_SINGLE,
	SECTORCOL_SECTORS,
	SECTORCOL_SIZE,
//	SECTORCOL_STATUS,
	SECTORCOL_OFFSET,
#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
	SECTORCOL_DUMMY,
#endif
	SECTORCOL_END
};

extern const struct st_list_columns gUiDiskRawSectorColumnDefs[];

//////////////////////////////////////////////////////////////////////

/// 分割ウィンドウ
class UiDiskRawPanel : public wxSplitterWindow
{
private:
	wxWindow *parent;
	UiDiskFrame *frame;

	UiDiskRawTrack *lpanel;
	UiDiskRawSector *rpanel;

	bool invert_data;	///< インポート・エクスポート時にデータを反転するか
	bool reverse_side;	///< インポート・エクスポート時にサイド番号を降順で行うか

public:
	UiDiskRawPanel(UiDiskFrame *parentframe, wxWindow *parentwindow);

	UiDiskRawTrack *GetLPanel() { return lpanel; }
	UiDiskRawSector *GetRPanel() { return rpanel; }

	/// トラックリストにデータを設定する
	void SetTrackListData(DiskImageFile *file);
	/// トラックリストにデータを設定する
	void SetTrackListData(DiskImageDisk *disk);
	/// トラックリストをクリアする
	void ClearTrackListData();
	/// トラックリストを再描画する
	void RefreshTrackListData();
	/// トラックリストが存在するか
	bool TrackListExists() const;
	/// トラックリストの選択行を返す
	int GetTrackListSelectedRow() const;

	/// セクタリストにデータを設定する
	void SetSectorListData(DiskImageFile *file, int track_num, int side_num, int start_sector, int end_sector);
	/// セクタリストをクリアする
	void ClearSectorListData();
	/// セクタリストの選択行を返す
	int GetSectorListSelectedRow() const;

	/// トラックリストとセクタリストを更新
	void RefreshAllData();

	/// クリップボードヘコピー
	bool CopyToClipboard();
	/// クリップボードからペースト
	bool PasteFromClipboard();

	/// エクスポートダイアログ表示
	bool ShowExportDataDialog();
	/// インポートダイアログ表示
	bool ShowImportDataDialog();
	/// データを削除ダイアログ表示
	bool ShowDeleteDataDialog();

	/// トラックへインポートダイアログ（トラックの範囲指定）表示
	bool ShowImportTrackRangeDialog(const wxString &path, int st_trk = -1, int st_sid = 0, int st_sec = 1);

	/// セクタからエクスポートダイアログ表示
	bool ShowExportDataFileDialog();
	/// セクタへインポートダイアログ表示
	bool ShowImportDataFileDialog();
//	/// トラックのID一括変更
//	void ModifyIDonTrack(int type_num);
//	/// トラックの密度一括変更
//	void ModifyDensityOnTrack();
//	/// トラックのセクタ数一括変更
//	void ModifySectorsOnTrack();
//	/// トラックのセクタサイズ一括変更
//	void ModifySectorSizeOnTrack();

	/// トラック or セクタのプロパティダイアログ表示
	bool ShowRawDiskAttr();
	/// セクタのプロパティダイアログ表示
	bool ShowSectorAttr();

//	/// ファイル名
//	wxString MakeFileName(DiskImageSector *sector);
	/// ファイル名
	wxString MakeFileName(int st_c, int st_h, int st_r, int ed_c, int ed_h, int ed_r);

	/// フォントをセット
	void SetListFont(const wxFont &font);

	/// インポート・エクスポート時にデータを反転するか
	bool InvertData() const { return invert_data; }
	/// インポート・エクスポート時にデータを反転するか
	void InvertData(bool val) { invert_data = val; }
	/// インポート・エクスポート時にサイド番号を降順で行うか
	bool ReverseSide() const { return reverse_side; }
	/// インポート・エクスポート時にサイド番号を降順で行うか
	void ReverseSide(bool val) { reverse_side = val; }

	/// 次のサイドへ
	void IncreaseSide();
	/// 前のサイドへ
	void DecreaseSide();

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(UiDiskRawPanel);
};

//////////////////////////////////////////////////////////////////////

/// スライダー付きテキスト
class MySliderText : public wxControl
{
protected:
	wxTextCtrl *pText;
	wxSlider   *pSlider;

public:
	MySliderText();
	MySliderText(wxWindow *parent, wxWindowID id, const wxPoint &pos=wxDefaultPosition, const wxSize &size=wxDefaultSize);
	virtual ~MySliderText() {}

	virtual bool SetFont(const wxFont &font);

	void SetRangeValue(int min_val, int max_val, int curr_val);
	void SetValue(int val);
	int GetValue() const;
	void EnteredText() const;

	void OnEnterText(wxCommandEvent &event);
	void OnSlider(wxCommandEvent &event);
};


//////////////////////////////////////////////////////////////////////

/// 左パネルのトラックリスト
class UiDiskRawTrack : public wxPanel
{
private:
	UiDiskRawPanel *parent;
	UiDiskFrame *frame;

	DiskImageFile *p_file;
//	DiskImageDisk *disk;
//	int side_number;

	MySliderText *txtTrack;
	MySliderText *txtSide;
	wxButton *btnSubmit;

	MyMenu *menuPopup;

	/// ファイルリスト作成（DnD, クリップボード用）
	bool CreateFileObject(wxString &tmp_dir_name, wxFileDataObject &file_object);
	/// ファイルリストを解放（DnD, クリップボード用）
	void ReleaseFileObject(const wxString &tmp_dir_name);

public:
	UiDiskRawTrack(UiDiskFrame *parentframe, UiDiskRawPanel *parentwindow);
	~UiDiskRawTrack();

	/// @name event procedures
	//@{
//	/// トラックリストを選択
//	void OnListItemSelected(MyRawTrackListEvent& event);
//	/// トラックリストをダブルクリック
//	void OnListActivated(MyRawTrackListEvent& event);
//	/// トラックリスト右クリック
//	void OnListContextMenu(MyRawTrackListEvent& event);
//	/// トラックリスト右クリック
//	void OnContextMenu(wxContextMenuEvent& event);
//	/// カラムをクリック
//	void OnColumnClick(MyRawTrackListEvent &event);
	/// Submitボタン押下
	void OnButtonSubmit(wxCommandEvent& event);
//	/// トラックをエクスポート選択
//	void OnExportTrack(wxCommandEvent& event);
//	/// トラックにインポート選択
//	void OnImportTrack(wxCommandEvent& event);
//	/// データを反転するチェック選択
//	void OnChangeInvertData(wxCommandEvent& event);
//	/// サイドを逆転するチェック選択
//	void OnChangeReverseSide(wxCommandEvent& event);
//	/// トラックリストからドラッグ開始
//	void OnBeginDrag(MyRawTrackListEvent& event);
//	/// ディスク上のID一括変更選択
//	void OnModifyIDonDisk(wxCommandEvent& event);
//	/// トラックのID一括変更選択
//	void OnModifyIDonTrack(wxCommandEvent& event);
//	/// ディスク上の密度一括変更選択
//	void OnModifyDensityOnDisk(wxCommandEvent& event);
//	/// トラックの密度一括変更選択
//	void OnModifyDensityOnTrack(wxCommandEvent& event);
//	/// トラックのセクタ数を一括変更選択
//	void OnModifySectorsOnTrack(wxCommandEvent& event);
//	/// トラックのセクタサイズを一括変更
//	void OnModifySectorSizeOnTrack(wxCommandEvent& event);
//	/// 新規トラックを追加選択
//	void OnAppendTrack(wxCommandEvent& event);
//	/// 現在のトラック以下を削除選択
//	void OnDeleteTracksBelow(wxCommandEvent& event);
	/// トラックプロパティ選択
	void OnPropertyTrack(wxCommandEvent& event);
	/// キー押下
	void OnChar(wxKeyEvent& event);
	//@}

	/// 選択
	void SelectData();
	/// トラックリストをセット
	void SetTracks(DiskImageFile *newfile);
	/// トラックリストをセット
	void SetTracks(DiskImageDisk *newdisk);
	/// トラックリストをセット
	void SetTracks(int start_sector);
	/// トラックリストをセット
	void SetTracks(int track_num, int side_num);
	/// トラックリストを再セット
	void RefreshTracks();
	/// トラックリストをクリア
	void ClearTracks();

	/// トラックリスト上のポップアップメニュー作成
	void MakePopupMenu();
	/// トラックリスト上のポップアップメニュー表示
	void ShowPopupMenu();

	/// ファイルリストをドラッグ
	bool DragDataSourceForExternal();
	/// クリップボードへコピー
	bool CopyToClipboard();
	/// クリップボードからペースト
	bool PasteFromClipboard();

	/// トラックをエクスポート ダイアログ表示
	bool ShowExportTrackDialog();
	/// 指定したファイルにトラックデータをエクスポート
	bool ExportTrackDataFile(const wxString &path, int st_trk, int st_sid, int st_sec, int ed_trk, int ed_sid, int ed_sec);
	/// トラックにインポート ダイアログ表示
	bool ShowImportTrackDialog();
	/// 指定したファイルのファイル名にある数値から指定したトラックにインポートする
	bool ShowImportTrackRangeDialog(const wxString &path, int st_trk = -1, int st_sid = 0, int st_sec = 1);
	/// 指定したファイルから指定した範囲にトラックデータをインポート
	bool ImportTrackDataFile(const wxString &path, int st_trk, int st_sid, int st_sec, int ed_trk, int ed_sid, int ed_sec);
	/// ディスク全体のIDを変更
	void ModifyIDonDisk(int type_num);
	/// ディスク上の密度を一括変更
	void ModifyDensityOnDisk();
	/// トラック情報を表示
	void ShowTrackAttr();

	/// トラックを追加
	void AppendTrack();
	/// トラックを削除
	void DeleteTracks();

	/// ディスクイメージを返す
	DiskImageFile *GetFile() const { return p_file; }
//	/// ディスクを返す
//	DiskImageDisk *GetDisk() const { return disk; }
//	/// 選択行のトラックを返す
//	DiskImageTrack *GetSelectedTrack();
//	/// 指定行のトラックを返す
//	DiskImageTrack *GetTrack(const MyRawTrackListItem &row);
//	/// 最初のトラックを返す
//	DiskImageTrack *GetFirstTrack();
//	/// トラックのセクタ１を得る
//	bool GetFirstSectorOnTrack(DiskImageTrack **track, DiskImageSector **sector);
//	/// トラックの開始セクタ番号と終了セクタ番号を得る
//	bool GetFirstAndLastSectorNumOnTrack(const DiskImageTrack *track, int &start_sector, int &end_sector);

	/// フォントをセット
	void SetListFont(const wxFont &font);

	/// 次のサイドへ
	void IncreaseSide();
	/// 前のサイドへ
	void DecreaseSide();

	enum {
		IDM_MENU_CHECK = 1,
		IDM_INVERT_DATA,
		IDM_REVERSE_SIDE,
		IDM_EXPORT_TRACK,
		IDM_IMPORT_TRACK,
		IDM_MODIFY_ID_C_DISK,
		IDM_MODIFY_ID_H_DISK,
		IDM_MODIFY_ID_R_DISK,
		IDM_MODIFY_ID_N_DISK,
		IDM_MODIFY_DENSITY_DISK,
		IDM_MODIFY_ID_C_TRACK,
		IDM_MODIFY_ID_H_TRACK,
		IDM_MODIFY_ID_R_TRACK,
		IDM_MODIFY_ID_N_TRACK,
		IDM_MODIFY_DENSITY_TRACK,
		IDM_MODIFY_SECTORS_TRACK,
		IDM_MODIFY_SIZE_TRACK,
		IDM_APPEND_TRACK,
		IDM_DELETE_TRACKS_BELOW,
		IDM_PROPERTY_TRACK,

		IDC_TXT_TRACK,
		IDC_TXT_SIDE,
		IDC_BTN_SUBMIT,
	};

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(UiDiskRawTrack);
};

//////////////////////////////////////////////////////////////////////

#ifndef USE_LIST_CTRL_ON_SECTOR_LIST

class UiDiskRawSector;

/// セクタリストの挙動を設定
class UiDiskRawSectorListStoreModel : public wxDataViewListStore
{
private:
	UiDiskRawSector *ctrl;
public:
	UiDiskRawSectorListStoreModel(wxWindow *parent);

	void SetControl(UiDiskRawSector *n_ctrl) { ctrl = n_ctrl; }

	bool IsEnabledByRow(unsigned int row, unsigned int col) const;

	int  Compare(const wxDataViewItem &item1, const wxDataViewItem &item2, unsigned int col, bool ascending) const;
};

#define MyRawSectorListColumn	wxDataViewColumn*
#define MyRawSectorListItem		wxDataViewItem
#define MyRawSectorListItems	wxDataViewItemArray
#define MyRawSectorListEvent	wxDataViewEvent
#define MyRawSectorListValue	MyCDListValue
#else
#define MyRawSectorListColumn	long
#define MyRawSectorListItem		long
#define MyRawSectorListItems	wxArrayLong
#define MyRawSectorListEvent	wxListEvent
#define MyRawSectorListValue	MyCListValue
#endif

//////////////////////////////////////////////////////////////////////

/// セクタリストコントロール
#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
class UiDiskRawSectorListCtrl : public MyCDListCtrl
#else
class UiDiskRawSectorListCtrl : public MyCListCtrl
#endif
{
public:
	UiDiskRawSectorListCtrl(UiDiskFrame *parentframe, wxWindow *parent, UiDiskRawSector *sub, wxWindowID id);
	/// リストデータを設定
	void SetListData(int idc, int idh, int idr, int secnum, int secs, int siz, wxUint32 offset, int row, MyRawSectorListValue *values);
	/// リストにデータを挿入
	void InsertListData(int idc, int idh, int idr, int secnum, int secs, int siz, wxUint32 offset, int row, int idx);
	/// リストデータを更新
	void UpdateListData(int idc, int idh, int idr, int secnum, int secs, int siz, wxUint32 offset, int row, int idx);
	/// 選択位置のセクタ番号を得る
	int GetListSectorNumber(int row);

#ifdef USE_LIST_CTRL_ON_SECTOR_LIST
	/// アイテムをソート
	void SortDataItems(int col);
	/// ソート用コールバック
	static int wxCALLBACK Compare(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortdata);
#endif
	static int CompareIDR(int i1, int i2, int dir);
	static int CompareNum(int i1, int i2, int dir);
};

//////////////////////////////////////////////////////////////////////

/// 右パネルのセクタリスト
class UiDiskRawSector : public UiDiskRawSectorListCtrl
{
private:
	UiDiskRawPanel *parent;
	UiDiskFrame *frame;

	MyMenu *menuPopup;

	DiskImageFile *p_file;
	int m_track_num;
	int m_side_num;
	int m_start_sector;
	int m_end_sector;

	bool m_initialized;

	/// ファイルリスト作成（DnD, クリップボード用）
	bool CreateFileObject(wxString &tmp_dir_name, wxFileDataObject &file_object);
	/// ファイルリストを解放（DnD, クリップボード用）
	void ReleaseFileObject(const wxString &tmp_dir_name);

public:
	UiDiskRawSector(UiDiskFrame *parentframe, UiDiskRawPanel *parent);
	~UiDiskRawSector();

	/// @name event procedures
	//@{
	/// セクタリスト右クリック
	void OnItemContextMenu(MyRawSectorListEvent& event);
	/// 右クリック
	void OnContextMenu(wxContextMenuEvent& event);
//	/// セクタリスト ダブルクリック
//	void OnItemActivated(MyRawSectorListEvent& event);
	/// セクタリスト カラムをクリック
	void OnColumnClick(MyRawSectorListEvent& event);
	/// セクタリスト選択
	void OnSelectionChanged(MyRawSectorListEvent& event);
	/// セクタリストからドラッグ開始
	void OnBeginDrag(MyRawSectorListEvent& event);
	/// セクタリスト エクスポート選択
	void OnExportFile(wxCommandEvent& event);
	/// セクタリスト インポート選択
	void OnImportFile(wxCommandEvent& event);
//	/// データを反転するチェック選択
//	void OnChangeInvertData(wxCommandEvent& event);
//	/// トラック上のID一括変更選択
//	void OnModifyIDonTrack(wxCommandEvent& event);
//	/// トラック上の密度一括変更選択
//	void OnModifyDensityOnTrack(wxCommandEvent& event);
//	/// トラック上のセクタ数一括変更選択
//	void OnModifySectorsOnTrack(wxCommandEvent& event);
//	/// トラック上のセクタサイズ一括変更選択
//	void OnModifySectorSizeOnTrack(wxCommandEvent& event);
//	/// セクタ追加選択
//	void OnAppendSector(wxCommandEvent& event);
//	/// セクタ削除選択
//	void OnDeleteSector(wxCommandEvent& event);
//	/// トラック上のセクタ一括削除選択
//	void OnDeleteSectorsOnTrack(wxCommandEvent& event);
	/// セクタ編集選択
	void OnEditSector(wxCommandEvent& event);
//	/// セクタプロパティ選択
//	void OnPropertySector(wxCommandEvent& event);
	/// セクタリスト上でキー押下
	void OnChar(wxKeyEvent& event);
	//@}

	/// ポップアップメニュー作成
	void MakePopupMenu();
	/// ポップアップメニュー表示
	void ShowPopupMenu();

	// セクタリスト選択
	void SelectItem(int sector_pos, DiskImageSector *sector);
	// セクタリスト非選択
	void UnselectItem();

	/// セクタリストにデータをセット
	void SetSectors(DiskImageFile *file, int track_num, int side_num, int start_sector, int end_sector);
//	/// セクタリストを返す
//	DiskImageSectors *GetSectors() const;
	/// セクタリストをリフレッシュ
	void RefreshSectors();
	/// セクタリストをクリア
	void ClearSectors();
	/// 選択しているセクタを返す
	DiskImageSector *GetSelectedSector(int *pos = NULL);
	/// セクタを返す
	DiskImageSector *GetSector(const MyRawSectorListItem &item);

	/// ファイルリストをドラッグ
	bool DragDataSourceForExternal();
	/// クリップボードへコピー
	bool CopyToClipboard();
	/// クリップボードからペースト
	bool PasteFromClipboard();

	/// エクスポートダイアログ表示
	bool ShowExportDataFileDialog();
	/// 指定したファイルにセクタのデータをエクスポート
	bool ExportDataFile(const wxString &path, DiskImageSector *sector);
	/// インポートダイアログ表示
	bool ShowImportDataFileDialog();
//	/// セクタ情報プロパティダイアログ表示
//	bool ShowSectorAttr();

//	/// トラック上のIDを一括変更
//	void ModifyIDonTrack(int type_num);
//	/// トラック上の密度を一括変更
//	void ModifyDensityOnTrack();
//	/// トラック上のセクタ数を一括変更
//	void ModifySectorsOnTrack();
//	/// トラック上のセクタサイズを一括変更
//	void ModifySectorSizeOnTrack();

//	/// セクタを追加ダイアログを表示
//	void ShowAppendSectorDialog();
	/// セクタを削除(ゼロクリア)
	void DeleteSector();
//	/// トラック上のセクタを一括削除
//	void DeleteSectorsOnTrack();

	/// セクタを編集
	void EditSector();

	enum {
		IDM_MENU_CHECK = 1,
		IDM_INVERT_DATA,
		IDM_REVERSE_SIDE,
		IDM_EXPORT_FILE,
		IDM_IMPORT_FILE,
		IDM_MODIFY_ID_C_TRACK,
		IDM_MODIFY_ID_H_TRACK,
		IDM_MODIFY_ID_R_TRACK,
		IDM_MODIFY_ID_N_TRACK,
		IDM_MODIFY_DENSITY_TRACK,
		IDM_MODIFY_SECTORS_TRACK,
		IDM_MODIFY_SIZE_TRACK,
		IDM_APPEND_SECTOR,
		IDM_DELETE_SECTOR,
		IDM_DELETE_SECTORS_BELOW,
		IDM_EDIT_SECTOR,
		IDM_PROPERTY_SECTOR,
	};

	wxDECLARE_EVENT_TABLE();
};

#endif /* UIRAWDISK_H */

