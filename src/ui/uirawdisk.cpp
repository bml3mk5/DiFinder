/// @file uirawdisk.cpp
///
/// @brief ディスクID一覧
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "uirawdisk.h"
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/valnum.h>
#include <wx/slider.h>
#include <wx/button.h>
#include <wx/menu.h>
#include <wx/regex.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/dataobj.h>
#include <wx/clipbrd.h>
#include <wx/utils.h>
#include "mymenu.h"
#include "../main.h"
#include "rawtrackbox.h"
#include "rawsectorbox.h"
#include "rawparambox.h"
#include "rawexpbox.h"
#include "../utils.h"


//////////////////////////////////////////////////////////////////////

const struct st_list_columns gUiDiskRawSectorColumnDefs[] = {
	{ "Num",		wxTRANSLATE("Num"),				false,	40,	wxALIGN_RIGHT,	true },
	{ "IDC",		wxTRANSLATE("C"),				false,	40,	wxALIGN_RIGHT,	false },
	{ "IDH",		wxTRANSLATE("H"),				false,	40,	wxALIGN_RIGHT,	false },
	{ "IDR",		wxTRANSLATE("R"),				false,	40,	wxALIGN_RIGHT,	false },
	{ "SecNum",		wxTRANSLATE("Sector"),			false,	72,	wxALIGN_RIGHT,	false },
	{ "Sectors",	wxTRANSLATE("NumOfSectors"),	false,	72,	wxALIGN_RIGHT,	false },
	{ "Size",		wxTRANSLATE("Size"),			false,	72,	wxALIGN_RIGHT,	false },
	{ "Offset",		wxTRANSLATE("Offset"),			false,	72,	wxALIGN_RIGHT,	false },
	{ NULL,			NULL,							false,	 0,	wxALIGN_LEFT,	false }
};

//////////////////////////////////////////////////////////////////////
//
// 分割ウィンドウ
//
// Attach Event
wxBEGIN_EVENT_TABLE(UiDiskRawPanel, wxSplitterWindow)
wxEND_EVENT_TABLE()

UiDiskRawPanel::UiDiskRawPanel(UiDiskFrame *parentframe, wxWindow *parentwindow)
                : wxSplitterWindow(parentwindow, wxID_ANY,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_BORDER | wxSP_LIVE_UPDATE |
                                   wxCLIP_CHILDREN /* | wxSP_NO_XP_THEME */ )
{
	parent = parentwindow;
    frame = parentframe;

	invert_data = false;
	reverse_side = false;

	// fit size on parent window
	wxSize sz = parentwindow->GetClientSize();
	SetSize(sz);

	// resize right window when resize parent window.
	SetSashGravity(0.0);

	// control panel
	lpanel = new UiDiskRawTrack(frame, this);
	rpanel = new UiDiskRawSector(frame, this);
	SplitVertically(lpanel, rpanel, 236);

	SetMinimumPaneSize(10);
}

/// トラックリストにデータを設定する
void UiDiskRawPanel::SetTrackListData(DiskImageFile *file)
{
	lpanel->SetTracks(file);
	frame->UpdateMenuAndToolBarRawDisk(this);
}

/// トラックリストにデータを設定する
void UiDiskRawPanel::SetTrackListData(DiskImageDisk *disk)
{
	lpanel->SetTracks(disk);
	frame->UpdateMenuAndToolBarRawDisk(this);
}

/// トラックリストをクリアする
void UiDiskRawPanel::ClearTrackListData()
{
	lpanel->ClearTracks();
	frame->UpdateMenuAndToolBarRawDisk(this);
}

/// トラックリストを再描画する
void UiDiskRawPanel::RefreshTrackListData()
{
	lpanel->RefreshTracks();
}

/// トラックリストが存在するか
bool UiDiskRawPanel::TrackListExists() const
{
	return (lpanel->GetFile() != NULL);
}

/// トラックリストの選択行を返す
int UiDiskRawPanel::GetTrackListSelectedRow() const
{
	return 0; // (int)lpanel->GetListSelectedRow();
}

/// セクタリストにデータを設定する
void UiDiskRawPanel::SetSectorListData(DiskImageFile *file, int track_num, int side_num, int start_sector, int end_sector)
{
	rpanel->SetSectors(file, track_num, side_num, start_sector, end_sector);
	frame->UpdateMenuAndToolBarRawDisk(this);
}

/// セクタリストをクリアする
void UiDiskRawPanel::ClearSectorListData()
{
	rpanel->ClearSectors();
	frame->UpdateMenuAndToolBarRawDisk(this);
}

/// セクタリストの選択行を返す
int UiDiskRawPanel::GetSectorListSelectedRow() const
{
	return rpanel->GetListSelectedRow();
}

/// トラックリストとセクタリストを更新
void UiDiskRawPanel::RefreshAllData()
{
	lpanel->RefreshTracks();
	rpanel->RefreshSectors();
}

/// クリップボードヘコピー
bool UiDiskRawPanel::CopyToClipboard()
{
	wxWindow *fwin = wxWindow::FindFocus();
	if (fwin == lpanel) {
		return lpanel->CopyToClipboard();
	} else if (fwin == rpanel) {
		return rpanel->CopyToClipboard();
	}
	return true;
}

/// クリップボードからペースト
bool UiDiskRawPanel::PasteFromClipboard()
{
	// トラック側
	return lpanel->PasteFromClipboard();
}

/// エクスポートダイアログ表示
bool UiDiskRawPanel::ShowExportDataDialog()
{
	wxWindow *fwin = wxWindow::FindFocus();
	if (fwin == lpanel) {
		return lpanel->ShowExportTrackDialog();
	} else if (fwin == rpanel) {
		return rpanel->ShowExportDataFileDialog();
	}
	return true;
}

/// インポートダイアログ表示
bool UiDiskRawPanel::ShowImportDataDialog()
{
	// トラック側のダイアログを表示
	return lpanel->ShowImportTrackDialog();
}

/// データを削除ダイアログ表示
bool UiDiskRawPanel::ShowDeleteDataDialog()
{
	wxWindow *fwin = wxWindow::FindFocus();
	if (fwin == lpanel) {
	} else if (fwin == rpanel) {
		rpanel->DeleteSector();
	}
	return true;
}

/// トラックへインポートダイアログ（トラックの範囲指定）表示
bool UiDiskRawPanel::ShowImportTrackRangeDialog(const wxString &path, int st_trk, int st_sid, int st_sec)
{
	return lpanel->ShowImportTrackRangeDialog(path, st_trk, st_sid, st_sec);
}

/// セクタからエクスポートダイアログ表示
bool UiDiskRawPanel::ShowExportDataFileDialog()
{
	return rpanel->ShowExportDataFileDialog();
}

/// セクタへインポートダイアログ表示
bool UiDiskRawPanel::ShowImportDataFileDialog()
{
	return rpanel->ShowImportDataFileDialog();
}

/// トラック or セクタのプロパティダイアログ表示
bool UiDiskRawPanel::ShowRawDiskAttr()
{
	return true;
}

/// セクタのプロパティダイアログ表示
bool UiDiskRawPanel::ShowSectorAttr()
{
	return true;
}

/// ファイル名を生成
/// @param[in] st_c 開始トラック番号
/// @param[in] st_h 開始サイド番号
/// @param[in] st_r 開始セクタ番号
/// @param[in] ed_c 終了トラック番号
/// @param[in] ed_h 終了サイド番号
/// @param[in] ed_r 終了セクタ番号
/// @return ファイル名
wxString UiDiskRawPanel::MakeFileName(int st_c, int st_h, int st_r, int ed_c, int ed_h, int ed_r)
{
	return wxString::Format(wxT("%02d-%02d-%02d--%02d-%02d-%02d.bin"),
		st_c, st_h, st_r, ed_c, ed_h, ed_r
	);
}

/// フォントをセット
/// @param[in] font フォントデータ
void UiDiskRawPanel::SetListFont(const wxFont &font)
{
	lpanel->SetListFont(font);
	lpanel->Refresh();
	rpanel->SetFont(font);
	rpanel->Refresh();
}

/// 次のサイドへ
void UiDiskRawPanel::IncreaseSide()
{
	lpanel->IncreaseSide();
}

/// 前のサイドへ
void UiDiskRawPanel::DecreaseSide()
{
	lpanel->DecreaseSide();
}

//////////////////////////////////////////////////////////////////////
//
// スライダー付きテキスト
//
MySliderText::MySliderText()
	: wxControl()
{
	pText = NULL;
	pSlider = NULL;
}
MySliderText::MySliderText(wxWindow *parent, wxWindowID id, const wxPoint &pos, const wxSize &size)
	: wxControl(parent, id, pos, size, wxBORDER_NONE)
{
	wxBoxSizer *box = new wxBoxSizer(wxVERTICAL);
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 2);

	wxSize sz(size.GetX(), -1);
	pText = new wxTextCtrl(this, wxID_ANY, wxT("0"), wxDefaultPosition, sz, wxTE_PROCESS_ENTER | wxALIGN_RIGHT, wxIntegerValidator<int>());
	pText->Enable(false);
	box->Add(pText, flags);

	pSlider = new wxSlider(this, wxID_ANY, 0, 0, 1, wxDefaultPosition, sz);
	pSlider->Enable(false);
	box->Add(pSlider, flags);

	SetSizer(box);

	// event handling
	Bind(wxEVT_TEXT_ENTER, &MySliderText::OnEnterText, this, pText->GetId());
	Bind(wxEVT_SLIDER, &MySliderText::OnSlider, this, pSlider->GetId());


}
/// フォントをセット
bool MySliderText::SetFont(const wxFont &font)
{
	bool rc = true;
	rc = rc && wxControl::SetFont(font);
	rc = rc && pText->SetFont(font);
	rc = rc && pSlider->SetFont(font);
	return rc;
}
/// 範囲と値をセット
void MySliderText::SetRangeValue(int min_val, int max_val, int curr_val)
{
	if (min_val == max_val) {
		curr_val = min_val;
		pSlider->SetRange(min_val, max_val + 1);
		pText->Enable(false);
		pSlider->Enable(false);
	} else {
		pSlider->SetRange(min_val, max_val);
		pText->Enable(true);
		pSlider->Enable(true);
	}
	pText->SetValue(wxString::Format(wxT("%d"), curr_val));
	pSlider->SetValue(curr_val);
}
void MySliderText::SetValue(int val)
{
	pText->SetValue(wxString::Format(wxT("%d"), val));
	pSlider->SetValue(val);
}
int MySliderText::GetValue() const
{
	EnteredText();

	return pSlider->GetValue();
}
/// テキスト入力
void MySliderText::EnteredText() const
{
	int min_val = pSlider->GetMin();
	int max_val = pSlider->GetMax();

	long val = 0;
	if (!pText->GetValue().ToLong(&val)) {
		pText->SetValue(wxString::Format(wxT("%d"), min_val));
		pSlider->SetValue(min_val);
		return;
	}
	if (val >= (long)max_val) {
		val = (long)max_val;
	}
	if (val <= (long)min_val) {
		val = (long)min_val;
	}
	pSlider->SetValue((int)val);
}
/// テキスト入力
void MySliderText::OnEnterText(wxCommandEvent &event)
{
	EnteredText();
}
/// スライダー変更
void MySliderText::OnSlider(wxCommandEvent &event)
{
	int val = pSlider->GetValue();
	pText->SetValue(wxString::Format(wxT("%d"), val));
}

//////////////////////////////////////////////////////////////////////
//
// 左パネルのトラックリスト
//
// Attach Event
wxBEGIN_EVENT_TABLE(UiDiskRawTrack, wxControl)
	EVT_BUTTON(IDC_BTN_SUBMIT, UiDiskRawTrack::OnButtonSubmit)

	EVT_CHAR(UiDiskRawTrack::OnChar)

	EVT_MENU(IDM_PROPERTY_TRACK, UiDiskRawTrack::OnPropertyTrack)
wxEND_EVENT_TABLE()

UiDiskRawTrack::UiDiskRawTrack(UiDiskFrame *parentframe, UiDiskRawPanel *parentwindow)
       : wxPanel(parentwindow, wxID_ANY)
{
	parent   = parentwindow;
	frame    = parentframe;

	p_file = NULL;

	// controls
	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer *track_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Track Number"));
	szrAll->Add(track_box);

	wxSize sz(200, -1);
	txtTrack = new MySliderText(this, IDC_TXT_TRACK, wxDefaultPosition, sz);
	track_box->Add(txtTrack);

	wxStaticBoxSizer *side_box = new wxStaticBoxSizer(wxVERTICAL, this, _("Side Number"));
	szrAll->Add(side_box);

	txtSide = new MySliderText(this, IDC_TXT_SIDE, wxDefaultPosition, sz);
	side_box->Add(txtSide);

	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
	btnSubmit = new wxButton(this, IDC_BTN_SUBMIT, _("Submit"));
	hbox->Add(btnSubmit);
	szrAll->Add(hbox, wxSizerFlags().Centre().Border(wxALL, 4));

	wxFont font;
	frame->GetDefaultListFont(font);
	SetListFont(font);

	SetSizerAndFit(szrAll);

	// popup menu
	MakePopupMenu();
}

UiDiskRawTrack::~UiDiskRawTrack()
{
	delete menuPopup;
}

/// Submitボタン押下
void UiDiskRawTrack::OnButtonSubmit(wxCommandEvent& event)
{
	if (!p_file) return;

	SelectData();
}

/// トラックプロパティ選択
void UiDiskRawTrack::OnPropertyTrack(wxCommandEvent& event)
{
	ShowTrackAttr();
}

/// キー押下
void UiDiskRawTrack::OnChar(wxKeyEvent& event)
{
	switch(event.GetKeyCode()) {
	case WXK_CONTROL_C:
		// Ctrl + C クリップボードヘコピー
		CopyToClipboard();
		break;
	case WXK_CONTROL_V:
		// Ctrl + V クリップボードからペースト
		PasteFromClipboard();
		break;
	default:
		event.Skip();
		break;
	}
}

/// 選択
void UiDiskRawTrack::SelectData()
{
	if (!p_file) return;

	int trk = txtTrack->GetValue();
	int sid = txtSide->GetValue();

	int start_sector = (trk * p_file->GetSidesPerDisk() + sid) * p_file->GetSectorsPerTrack();
	int end_sector = start_sector + p_file->GetSectorsPerTrack() - 1;

	parent->SetSectorListData(p_file, trk, sid, start_sector, end_sector);
}

/// トラックリストをセット
void UiDiskRawTrack::SetTracks(DiskImageFile *newfile)
{
	if (!newfile) return;

	p_file = newfile;

	SetTracks(0);

	// セクタリストはクリア
	parent->ClearSectorListData();
}

/// トラックリストをセット
/// @param[in] newdisk    表示対象ディスク
void UiDiskRawTrack::SetTracks(DiskImageDisk *newdisk)
{
	if (!newdisk) return;

	p_file = newdisk->GetFile();

	SetTracks(newdisk->GetStartSectorNumber());

	// セクタリストはクリア
	parent->ClearSectorListData();
}

/// トラックリストを再セット
void UiDiskRawTrack::RefreshTracks()
{
	int trk_num = txtTrack->GetValue();
	int sid_num = txtSide->GetValue();

	SetTracks(trk_num, sid_num);
}

/// トラックリストを再セット
void UiDiskRawTrack::SetTracks(int start_sector)
{
	if (!p_file) return;

	int sides = p_file->GetSidesPerDisk();
	int secs = p_file->GetSectorsPerTrack();

	int trk_num = start_sector / secs / sides;
	int sid_num = (start_sector / secs) % sides;

	SetTracks(trk_num, sid_num);
}

/// トラックリストをセット
void UiDiskRawTrack::SetTracks(int track_num, int side_num)
{
	int tracks = p_file->GetTracksPerSide();
	int sides = p_file->GetSidesPerDisk();

	txtTrack->SetRangeValue(0, tracks - 1, track_num);
	txtSide->SetRangeValue(0, sides - 1, side_num);
}

/// トラックリストをクリア
void UiDiskRawTrack::ClearTracks()
{
	p_file = NULL;

	// セクタリストクリア
	parent->ClearSectorListData();
}

/// トラックリスト上のポップアップメニュー作成
void UiDiskRawTrack::MakePopupMenu()
{
	menuPopup = new MyMenu;
	MyMenu *sm = new MyMenu;
		sm->AppendCheckItem(IDM_INVERT_DATA,  _("Invert datas."));
		sm->AppendCheckItem(IDM_REVERSE_SIDE, _("Descend side number order."));
	menuPopup->AppendSubMenu(sm, _("Behavior When In/Out"));
	menuPopup->AppendSeparator();
	menuPopup->Append(IDM_EXPORT_TRACK, _("&Export Track..."));
	menuPopup->Append(IDM_IMPORT_TRACK, _("&Import..."));
	menuPopup->AppendSeparator();
	menuPopup->Append(IDM_PROPERTY_TRACK, _("&Property"));
}

/// トラックリスト上のポップアップメニュー表示
void UiDiskRawTrack::ShowPopupMenu()
{
	if (!menuPopup) return;
//	int num = 0;
//	wxUint32 offset = 0;

	menuPopup->Check(IDM_INVERT_DATA, parent->InvertData());
	menuPopup->Check(IDM_REVERSE_SIDE, parent->ReverseSide());

	bool opened = (p_file != NULL);
	menuPopup->Enable(IDM_EXPORT_TRACK, opened);
	menuPopup->Enable(IDM_IMPORT_TRACK, opened);

	PopupMenu(menuPopup);
}

/// ドラッグする 外部へドロップ場合
bool UiDiskRawTrack::DragDataSourceForExternal()
{
	wxString tmp_dir_name;
	wxFileDataObject file_object;
	bool sts = CreateFileObject(tmp_dir_name, file_object);
	if (sts) {
		// ファイルをドロップ
#ifdef __WXMSW__
		wxDropSource dragSource(file_object);
#else
		wxDropSource dragSource(file_object, frame);
#endif
		dragSource.DoDragDrop();
	}
	return sts;
}

// クリップボードへコピー
bool UiDiskRawTrack::CopyToClipboard()
{
	wxString tmp_dir_name;
	wxFileDataObject *file_object = new wxFileDataObject();
	bool sts = CreateFileObject(tmp_dir_name, *file_object);
	if (sts) {
		if (wxTheClipboard->Open())	{
		    // This data objects are held by the clipboard,
			// so do not delete them in the app.
			wxTheClipboard->SetData(file_object);
			wxTheClipboard->Close();
		}
	} else {
		delete file_object;
	}
	return sts;
}

/// ファイルをテンポラリディレクトリにエクスポートしファイルリストを作成する（DnD, クリップボード用）
bool UiDiskRawTrack::CreateFileObject(wxString &tmp_dir_name, wxFileDataObject &file_object)
{
	return false;
}

// ファイルリストを解放（DnD, クリップボード用）
void UiDiskRawTrack::ReleaseFileObject(const wxString &tmp_dir_name)
{
	UiDiskApp *app = &wxGetApp();

	// テンポラリディレクトリを削除
	app->RemoveTempDir(tmp_dir_name);
}

/// クリップボードからペースト
bool UiDiskRawTrack::PasteFromClipboard()
{
	// Read some text
	wxFileDataObject file_object;

	if (wxTheClipboard->Open()) {
		if (wxTheClipboard->IsSupported( wxDF_FILENAME )) {
			wxTheClipboard->GetData( file_object );
		}
		wxTheClipboard->Close();
	}

	wxArrayString file_names = file_object.GetFilenames();
	if (file_names.Count() != 1) return false;

	bool sts = true;
	for(size_t n = 0; n < file_names.Count(); n++) {
		sts &= ShowImportTrackRangeDialog(file_names.Item(n));
	}

	return sts;
}

/// トラックをエクスポート ダイアログ表示
bool UiDiskRawTrack::ShowExportTrackDialog()
{
	return false;
}

/// 指定したファイルにトラックデータをエクスポート
/// @param [in] path ファイルパス
/// @param [in] st_trk 開始トラック番号
/// @param [in] st_sid 開始サイド番号
/// @param [in] st_sec 開始セクタ番号
/// @param [in] ed_trk 終了トラック番号
/// @param [in] ed_sid 終了サイド番号
/// @param [in] ed_sec 終了セクタ番号
/// @return true:成功 / false:エラー
bool UiDiskRawTrack::ExportTrackDataFile(const wxString &path, int st_trk, int st_sid, int st_sec, int ed_trk, int ed_sid, int ed_sec)
{
	return false;
}

/// トラックにインポート ダイアログ表示
bool UiDiskRawTrack::ShowImportTrackDialog()
{
	if (!p_file) return false;

	wxString caption = _("Import data to track");

	UiDiskFileDialog fdlg(
		caption,
		frame->GetIniExportFilePath(),
		wxEmptyString,
		_("All files (*.*)|*.*"),
		wxFD_OPEN);
	int sts = fdlg.ShowModal();
	if (sts != wxID_OK) return false;

	wxString path = fdlg.GetPath();

	return ShowImportTrackRangeDialog(path);
}

/// 指定したファイルのファイル名にある数値から指定したトラックにインポートする
/// @param [in] path ファイルパス
/// @param [in] st_trk 開始トラック番号
/// @param [in] st_sid 開始サイド番号
/// @param [in] st_sec 開始セクタ番号
/// @return true:成功 / false:エラー
bool UiDiskRawTrack::ShowImportTrackRangeDialog(const wxString &path, int st_trk, int st_sid, int st_sec)
{
	int ed_trk = st_trk;
	int ed_sid = st_sid;
	int ed_sec = st_sec;

	wxString caption = _("Import data to the disk");

	wxRegEx re("([0-9][0-9])-([0-9][0-9])-([0-9][0-9])--([0-9][0-9])-([0-9][0-9])-([0-9][0-9])");
	if (re.Matches(path)) {
		wxString sval;
		long lval;
		sval = re.GetMatch(path, 1); sval.ToLong(&lval);
		st_trk = (int)lval;
		sval = re.GetMatch(path, 2); sval.ToLong(&lval);
		st_sid = (int)lval;
		sval = re.GetMatch(path, 3); sval.ToLong(&lval);
		st_sec = (int)lval;
		sval = re.GetMatch(path, 4); sval.ToLong(&lval);
		ed_trk = (int)lval;
		sval = re.GetMatch(path, 5); sval.ToLong(&lval);
		ed_sid = (int)lval;
		sval = re.GetMatch(path, 6); sval.ToLong(&lval);
		ed_sec = (int)lval;

	} else if (st_trk < 0) {
		return false;

	}

	RawExpBox dlg(this, wxID_ANY, caption, p_file
		, st_trk, st_sid, st_sec
		, ed_trk, ed_sid, ed_sec
	);
	int sts = dlg.ShowModal();
	if (sts != wxID_OK) {
		return false;
	}

	return ImportTrackDataFile(path
	, dlg.GetTrackNumber(0), dlg.GetSideNumber(0), dlg.GetSectorNumber(0)
	, dlg.GetTrackNumber(1), dlg.GetSideNumber(1), dlg.GetSectorNumber(1)
	);
}

/// 指定したファイルから指定した範囲にトラックデータをインポート
/// @param [in] path ファイルパス
/// @param [in] st_trk 開始トラック番号
/// @param [in] st_sid 開始サイド番号
/// @param [in] st_sec 開始セクタ番号
/// @param [in] ed_trk 終了トラック番号
/// @param [in] ed_sid 終了サイド番号
/// @param [in] ed_sec 終了セクタ番号
/// @return true:成功 / false:エラー
bool UiDiskRawTrack::ImportTrackDataFile(const wxString &path, int st_trk, int st_sid, int st_sec, int ed_trk, int ed_sid, int ed_sec)
{
	// エクスポート元パスを覚えておく
	frame->SetIniExportFilePath(path);

	if (!p_file) return false;

	wxFile infile(path, wxFile::read);
	if (!infile.IsOpened()) return false;

	int st_sec_pos = p_file->GetSectorPosFromNumber(st_trk, st_sid, st_sec);
	int ed_sec_pos = p_file->GetSectorPosFromNumber(ed_trk, ed_sid, ed_sec);

	for(int sec_pos = st_sec_pos; sec_pos <= ed_sec_pos; sec_pos++) {
		DiskImageSector *sector = p_file->GetSector(sec_pos);
		if (!sector) continue;

		wxUint8 *buf = sector->GetSectorBuffer();
		size_t bufsize = sector->GetSectorBufferSize();

		infile.Read((void *)buf, bufsize);
	}
	infile.Close();

	return true;
}

/// ディスク全体のIDを変更
/// @param [in] type_num  1:ID H  3:ID N
void UiDiskRawTrack::ModifyIDonDisk(int type_num)
{
}

/// ディスク上の密度を一括変更
void UiDiskRawTrack::ModifyDensityOnDisk()
{
}

/// トラック情報を表示
void UiDiskRawTrack::ShowTrackAttr()
{
}

/// トラックを追加
void UiDiskRawTrack::AppendTrack()
{
}

/// トラックを削除
void UiDiskRawTrack::DeleteTracks()
{
}

/// フォントをセット
void UiDiskRawTrack::SetListFont(const wxFont &font)
{
	wxPanel::SetFont(font);
	wxFont fontb = font;
	int pt = font.GetPointSize() * 2;
	if (pt > 18) pt = 18;
	fontb.SetPointSize(pt);

	txtTrack->SetFont(fontb);
	txtSide->SetFont(fontb);
	btnSubmit->SetFont(fontb);
}

/// 次のサイドへ
void UiDiskRawTrack::IncreaseSide()
{
	if (!p_file) return;

	int trk = txtTrack->GetValue();
	int sid = txtSide->GetValue();
	sid++;
	if (sid >= p_file->GetSidesPerDisk()) {
		trk++;
		sid = 0;
	}
	if (trk >= p_file->GetTracksPerSide()) {
		trk = p_file->GetTracksPerSide() - 1;
	}
	txtTrack->SetValue(trk);
	txtSide->SetValue(sid);

	SelectData();
}

/// 前のサイドへ
void UiDiskRawTrack::DecreaseSide()
{
	if (!p_file) return;

	int trk = txtTrack->GetValue();
	int sid = txtSide->GetValue();
	sid--;
	if (sid < 0) {
		trk--;
		sid = p_file->GetSidesPerDisk() - 1;
	}
	if (trk < 0) {
		trk = 0;
	}
	txtTrack->SetValue(trk);
	txtSide->SetValue(sid);

	SelectData();
}

//////////////////////////////////////////////////////////////////////
//
//
//
#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
UiDiskRawSectorListStoreModel::UiDiskRawSectorListStoreModel(wxWindow *parent)
{
	ctrl = (UiDiskRawSector *)parent;
}

bool UiDiskRawSectorListStoreModel::IsEnabledByRow(unsigned int row, unsigned int col) const
{
    return true;
}

int UiDiskRawSectorListStoreModel::Compare(const wxDataViewItem &item1, const wxDataViewItem &item2, unsigned int col, bool ascending) const
{
	DiskImageSectors *sectors = ctrl->GetSectors();
	if (!sectors) return 0;

	int idx = -1;
	if (!ctrl->FindColumn(col, &idx)) return 0;

	int cmp = 0;
	int i1 = (int)GetItemData(item1);
	int i2 = (int)GetItemData(item2);
	switch(idx) {
	case SECTORCOL_ID_R:
		cmp = UiDiskRawSectorListCtrl::CompareIDR(sectors, i1, i2, ascending ? 1 : -1);
		break;
	case SECTORCOL_NUM:
		cmp = UiDiskRawSectorListCtrl::CompareNum(sectors, i1, i2, ascending ? 1 : -1);
		break;
	default:
		break;
	}
	return cmp;
}
#endif

//////////////////////////////////////////////////////////////////////
//
// セクタリストコントロール
//
UiDiskRawSectorListCtrl::UiDiskRawSectorListCtrl(UiDiskFrame *parentframe, wxWindow *parent, UiDiskRawSector *sub, wxWindowID id)
#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
	: MyCDListCtrl(
		parentframe, parent, id,
		gUiDiskRawSectorColumnDefs,
		NULL,
		wxDV_MULTIPLE,
		new UiDiskRawSectorListStoreModel(sub)
	)
#else
	: MyCListCtrl(
		parentframe, parent, id,
		gUiDiskRawSectorColumnDefs,
		-1, -1,
		NULL,
		0
	)
#endif
{
#if 0
	AssignListIcons(icons_for_seclist);
#endif
}

/// リストデータを設定
void UiDiskRawSectorListCtrl::SetListData(int idc, int idh, int idr, int secnum, int secs, int siz, wxUint32 offset, int row, MyRawSectorListValue *values)
{
	values[SECTORCOL_NUM].Set(row, wxString::Format(wxT("%d"), row));
	values[SECTORCOL_ID_C].Set(row, wxString::Format(wxT("%d"), idc));
	values[SECTORCOL_ID_H].Set(row, wxString::Format(wxT("%d"), idh));
	values[SECTORCOL_ID_R].Set(row, wxString::Format(wxT("%d"), idr));
	values[SECTORCOL_SECNUM].Set(row, wxString::Format(wxT("%d"), secnum));
	values[SECTORCOL_SECTORS].Set(row, wxString::Format(wxT("%d"), secs));
	values[SECTORCOL_SIZE].Set(row, wxString::Format(wxT("%d"), siz));
	values[SECTORCOL_OFFSET].Set(row, wxString::Format(wxT("%x"), offset));
}

/// リストにデータを挿入
void UiDiskRawSectorListCtrl::InsertListData(int idc, int idh, int idr, int secnum, int secs, int siz, wxUint32 offset, int row, int idx)
{
	MyRawSectorListValue values[SECTORCOL_END];

	SetListData(idc, idh, idr, secnum, secs, siz, offset, row, values);

	InsertListItem(row, values, SECTORCOL_END, (wxUIntPtr)idx);
}

/// リストデータを更新
void UiDiskRawSectorListCtrl::UpdateListData(int idc, int idh, int idr, int secnum, int secs, int siz, wxUint32 offset, int row, int idx)
{
	MyRawSectorListValue values[SECTORCOL_END];

	SetListData(idc, idh, idr, secnum, secs, siz, offset, row, values);

	UpdateListItem(row, values, SECTORCOL_END, (wxUIntPtr)idx);
}

/// 選択位置のセクタ番号を得る
int UiDiskRawSectorListCtrl::GetListSectorNumber(int row)
{
	MyRawSectorListValue values[SECTORCOL_END];

	GetListItem(row, values, SECTORCOL_END);

	long val = 0;
	values[SECTORCOL_SECNUM].Get().ToLong(&val);
	return (int)val;
}

#ifdef USE_LIST_CTRL_ON_SECTOR_LIST
//
//
//

/// ソート用アイテム
struct st_sector_list_sort_exp {
	int (*cmpfunc)(int i1, int i2, int dir);
	int dir;
};

/// アイテムをソート
void UiDiskRawSectorListCtrl::SortDataItems(int col)
{
	struct st_sector_list_sort_exp exp;

	// ソート対象か
	int idx;
	bool match_col;
	exp.dir = SelectColumnSortDir(col, idx, match_col);

	// 番号かID Rの時のみソート
	if (col >= 0 && match_col) {
		// ソート
		switch(idx) {
		case SECTORCOL_ID_R:
			exp.cmpfunc = &CompareIDR;
			break;
		case SECTORCOL_NUM:
			exp.cmpfunc = &CompareNum;
			break;
		default:
			exp.cmpfunc = NULL;
			break;
		}
		SortItems(&Compare, (wxIntPtr)&exp);

		SetColumnSortIcon(idx);
	}
}

/// ソート用コールバック
int wxCALLBACK UiDiskRawSectorListCtrl::Compare(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortdata)
{
	struct st_sector_list_sort_exp *exp = (struct st_sector_list_sort_exp *)sortdata;

	int cmp = exp->cmpfunc != NULL ? exp->cmpfunc((int)item1, (int)item2, exp->dir) : 0;
	if (cmp == 0) cmp = ((int)item1 - (int)item2);
	return cmp;
}
#endif

/// ID Rでソート
int UiDiskRawSectorListCtrl::CompareIDR(int i1, int i2, int dir)
{
	return (i1 - i2) * dir;
}
int UiDiskRawSectorListCtrl::CompareNum(int i1, int i2, int dir)
{
	return (i1 - i2) * dir;
}

//////////////////////////////////////////////////////////////////////
//
// 右パネルのセクタリスト
//
// Attach Event
wxBEGIN_EVENT_TABLE(UiDiskRawSector, UiDiskRawSectorListCtrl)
#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
	EVT_DATAVIEW_ITEM_CONTEXT_MENU(wxID_ANY, UiDiskRawSector::OnItemContextMenu)
	EVT_DATAVIEW_SELECTION_CHANGED(wxID_ANY, UiDiskRawSector::OnSelectionChanged)
	EVT_DATAVIEW_ITEM_BEGIN_DRAG(wxID_ANY, UiDiskRawSector::OnBeginDrag)
#else
	EVT_CONTEXT_MENU(UiDiskRawSector::OnContextMenu)
	EVT_LIST_ITEM_SELECTED(wxID_ANY, UiDiskRawSector::OnSelectionChanged)
	EVT_LIST_COL_CLICK(wxID_ANY, UiDiskRawSector::OnColumnClick)
	EVT_LIST_BEGIN_DRAG(wxID_ANY, UiDiskRawSector::OnBeginDrag)
#endif
	EVT_CHAR(UiDiskRawSector::OnChar)

	EVT_MENU(IDM_EXPORT_FILE, UiDiskRawSector::OnExportFile)
	EVT_MENU(IDM_IMPORT_FILE, UiDiskRawSector::OnImportFile)

	EVT_MENU(IDM_EDIT_SECTOR, UiDiskRawSector::OnEditSector)

wxEND_EVENT_TABLE()

UiDiskRawSector::UiDiskRawSector(UiDiskFrame *parentframe, UiDiskRawPanel *parentwindow)
	: UiDiskRawSectorListCtrl(parentframe, parentwindow, this, wxID_ANY)
{
	m_initialized = false;
	parent   = parentwindow;
	frame    = parentframe;

	p_file = NULL;
	m_track_num = 0;
	m_side_num = 0;
	m_start_sector = 0;
	m_end_sector = 0;

	wxFont font;
	frame->GetDefaultListFont(font);
	SetFont(font);

	// popup menu
	MakePopupMenu();

	m_initialized = true;
}

UiDiskRawSector::~UiDiskRawSector()
{
	delete menuPopup;
}


/// セクタリスト選択
void UiDiskRawSector::OnSelectionChanged(MyRawSectorListEvent& event)
{
	if (!m_initialized) return;

	DiskImageSector *sector = GetSelectedSector();
	if (!sector) {
		// 非選択
		UnselectItem();
		return;
	}

	// 選択
	SelectItem(sector->GetNumber(), sector);
}

/// セクタリストからドラッグ開始
void UiDiskRawSector::OnBeginDrag(MyRawSectorListEvent& event)
{
	DragDataSourceForExternal();
}

/// セクタリスト右クリック
void UiDiskRawSector::OnItemContextMenu(MyRawSectorListEvent& event)
{
	ShowPopupMenu();
}

/// 右クリック
void UiDiskRawSector::OnContextMenu(wxContextMenuEvent& event)
{
	ShowPopupMenu();
}

/// セクタリスト カラムをクリック
void UiDiskRawSector::OnColumnClick(MyRawSectorListEvent& event)
{
//	int col = event.GetColumn();
}

/// セクタリスト エクスポート選択
void UiDiskRawSector::OnExportFile(wxCommandEvent& event)
{
	ShowExportDataFileDialog();
}

/// セクタリスト インポート選択
void UiDiskRawSector::OnImportFile(wxCommandEvent& event)
{
	ShowImportDataFileDialog();
}

/// セクタ編集選択
void UiDiskRawSector::OnEditSector(wxCommandEvent& event)
{
	EditSector();
}

/// セクタリスト上でキー押下
void UiDiskRawSector::OnChar(wxKeyEvent& event)
{
	switch(event.GetKeyCode()) {
	case WXK_CONTROL_C:
		// Ctrl + C クリップボードヘコピー
		CopyToClipboard();
		break;
	case WXK_CONTROL_V:
		// Ctrl + V クリップボードからペースト
		PasteFromClipboard();
		break;
	case WXK_LEFT:
		// Allow <- 前のレコードへ
		parent->DecreaseSide();
		break;
	case WXK_RIGHT:
		// Allow -> 次のレコードへ
		parent->IncreaseSide();
		break;
	default:
		event.Skip();
		break;
	}
}

/// ポップアップメニュー作成
void UiDiskRawSector::MakePopupMenu()
{
	menuPopup = new MyMenu;
	menuPopup->Append(IDM_EXPORT_FILE, _("&Export Sector..."));
	menuPopup->Append(IDM_IMPORT_FILE, _("&Import..."));
	menuPopup->AppendSeparator();
	menuPopup->Append(IDM_EDIT_SECTOR, _("Edit Current Sector"));
}

/// ポップアップメニュー表示
void UiDiskRawSector::ShowPopupMenu()
{
	if (!menuPopup) return;

	bool opened = (p_file != NULL);
	menuPopup->Enable(IDM_IMPORT_FILE, opened);

	int cnt = GetListSelectedItemCount();
	opened = (opened && (cnt > 0));
	menuPopup->Enable(IDM_EXPORT_FILE, opened);
	menuPopup->Enable(IDM_EDIT_SECTOR, opened && (cnt == 1));

	PopupMenu(menuPopup);
}

// セクタリスト選択
void UiDiskRawSector::SelectItem(int sector_pos, DiskImageSector *sector)
{
	// ダンプリストをセット
	frame->SetBinDumpData(sector_pos, sector->GetSectorBuffer(), sector->GetSectorSize());

	// メニューを更新
	frame->UpdateMenuAndToolBarRawDisk(parent);

}

// セクタリスト非選択
void UiDiskRawSector::UnselectItem()
{
	// ダンプリストをクリア
	frame->ClearBinDumpData();

	// メニューを更新
	frame->UpdateMenuAndToolBarRawDisk(parent);
}

/// セクタリストにデータをセット
void UiDiskRawSector::SetSectors(DiskImageFile *file, int track_num, int side_num, int start_sector, int end_sector)
{
	p_file = file;
	m_track_num = track_num;
	m_side_num = side_num;
	m_start_sector = start_sector;
	m_end_sector = end_sector;

	RefreshSectors();
}

/// セクタリストをリフレッシュ
void UiDiskRawSector::RefreshSectors()
{
	if (!p_file) return;

	int row = 0;
	int row_count = (int)GetItemCount();

	int siz = p_file->GetSectorSize();
	int secs = p_file->GetSectorsPerTrack();

	for (int sector_num = m_start_sector, i = 0; sector_num <= m_end_sector; sector_num++, i++) {
		wxUint32 offset = sector_num * siz + p_file->GetStartOffset();
		if (row < row_count) {
			UpdateListData(m_track_num, m_side_num, i, sector_num, secs, siz, offset, row, i);
		} else {
			InsertListData(m_track_num, m_side_num, i, sector_num, secs, siz, offset, row, i);
		}
		row++;
	}
#ifndef USE_LIST_CTRL_ON_SECTOR_LIST
	// 余分な行は消す
	for(int idx = row; idx < row_count; idx++) {
		DeleteItem((unsigned)row);
	}
#else
#ifndef USE_VIRTUAL_ON_LIST_CTRL
	// 余分な行は消す
	for(int idx = row; idx < row_count; idx++) {
		DeleteItem(row);
	}
#else
	SetItemCount(row);
#endif
#endif

#ifdef USE_LIST_CTRL_ON_SECTOR_LIST
#ifdef USE_VIRTUAL_ON_LIST_CTRL
	SetItemCount(row);
#endif
	// ソート
	SortDataItems(-1);
#endif

	// ダンプリストをクリア
	frame->ClearBinDumpData();

	// メニューを更新
	frame->UpdateMenuAndToolBarRawDisk(parent);

}

/// セクタリストをクリア
void UiDiskRawSector::ClearSectors()
{
}

/// 選択しているセクタを返す
DiskImageSector *UiDiskRawSector::GetSelectedSector(int *pos)
{
	if (!p_file) return NULL;

	int idx = GetListSelectedNum();
	if (idx == wxNOT_FOUND) return NULL;
	if (pos) *pos = idx;

	int sector_pos = m_start_sector + idx;
	DiskImageSector *sector = p_file->GetSector(sector_pos);
	return sector;
}

/// セクタを返す
DiskImageSector *UiDiskRawSector::GetSector(const MyRawSectorListItem &item)
{
	if (!p_file) return NULL;

	int idx = (int)GetListItemData(item);

	int sector_pos = m_start_sector + idx;
	DiskImageSector *sector = p_file->GetSector(sector_pos);
	return sector;
}

/// ドラッグする 外部へドロップ場合
bool UiDiskRawSector::DragDataSourceForExternal()
{
	wxString tmp_dir_name;
	wxFileDataObject file_object;
	bool sts = CreateFileObject(tmp_dir_name, file_object);
	if (sts) {
		// ファイルをドロップ
#ifdef __WXMSW__
		wxDropSource dragSource(file_object);
#else
		wxDropSource dragSource(file_object, frame);
#endif
		dragSource.DoDragDrop();
	}
	return sts;
}

// クリップボードへコピー
bool UiDiskRawSector::CopyToClipboard()
{
	wxString tmp_dir_name;
	wxFileDataObject *file_object = new wxFileDataObject();
	bool sts = CreateFileObject(tmp_dir_name, *file_object);
	if (sts) {
		if (wxTheClipboard->Open())	{
		    // This data objects are held by the clipboard,
			// so do not delete them in the app.
			wxTheClipboard->SetData(file_object);
			wxTheClipboard->Close();
		}
	} else {
		delete file_object;
	}
	return sts;
}

/// ファイルをテンポラリディレクトリにエクスポートしファイルリストを作成する（DnD, クリップボード用）
bool UiDiskRawSector::CreateFileObject(wxString &tmp_dir_name, wxFileDataObject &file_object)
{
	MyRawSectorListItems selected_items;
	int selected_count = GetListSelections(selected_items);
	if (selected_count <= 0) return false;

	UiDiskApp *app = &wxGetApp();

	// テンポラリディレクトリを作成
	if (!app->MakeTempDir(tmp_dir_name)) {
		return false;
	}

	int cnt = 0;
	for(int n = 0; n < selected_count; n++) {
		DiskImageSector *sector = GetSector(selected_items.Item(n));
		if (!sector) continue;

		int sector_num = sector->GetNumber() - m_start_sector;
		wxString filename = parent->MakeFileName(
			m_track_num, m_side_num, sector_num,
			m_track_num, m_side_num, sector_num);

		// ファイルパスを作成
		wxFileName file_path(tmp_dir_name, filename);

		bool sts = ExportDataFile(file_path.GetFullPath(), sector);
		if (sts) {
			// ファイルリストに追加
			file_object.AddFile(file_path.GetFullPath());
			cnt++;
		}
	}

	return (cnt > 0);
}

// ファイルリストを解放（DnD, クリップボード用）
void UiDiskRawSector::ReleaseFileObject(const wxString &tmp_dir_name)
{
	UiDiskApp *app = &wxGetApp();

	// テンポラリディレクトリを削除
	app->RemoveTempDir(tmp_dir_name);
}

/// クリップボードからペースト
bool UiDiskRawSector::PasteFromClipboard()
{
	return parent->PasteFromClipboard();
}

/// エクスポートダイアログ表示
bool UiDiskRawSector::ShowExportDataFileDialog()
{
	MyRawSectorListItems selected_items;
	int selected_count = GetListSelections(selected_items);

	bool sts = true;
	if (selected_count == 1) {
		// 単一行 指定
		DiskImageSector *sector = GetSector(selected_items.Item(0));
		if (!sector) return false;

		int sector_num = sector->GetNumber() - m_start_sector;
		wxString filename = parent->MakeFileName(
			m_track_num, m_side_num, sector_num,
			m_track_num, m_side_num, sector_num);

		UiDiskFileDialog dlg(
			_("Export data from sector"),
			frame->GetIniExportFilePath(),
			filename,
			_("All files (*.*)|*.*"),
			wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

		int rc = dlg.ShowModal();
		wxString path = dlg.GetPath();

		if (rc == wxID_OK) {
			sts = ExportDataFile(path, sector);
		}

	} else {
		// 複数行 指定
		UiDiskDirDialog dlg(
			_("Export each datas from selected sector"),
			frame->GetIniExportFilePath());

		int rc = dlg.ShowModal();
		if (rc != wxID_OK) {
			return false;
		}

		wxString dir_path = dlg.GetPath();
		for(int n = 0; n < selected_count; n++) {
			DiskImageSector *sector = GetSector(selected_items.Item(n));
			if (!sector) continue;

			// ファイルパスを作成
			int sector_num = sector->GetNumber() - m_start_sector;
			wxFileName file_path(dir_path, parent->MakeFileName(
				m_track_num, m_side_num, sector_num,
				m_track_num, m_side_num, sector_num));

			sts &= ExportDataFile(file_path.GetFullPath(), sector);
		}
	}

	return sts;
}

/// 指定したファイルにセクタのデータをエクスポート
bool UiDiskRawSector::ExportDataFile(const wxString &path, DiskImageSector *sector)
{
	// エクスポート元パスを覚えておく
	frame->SetIniExportFilePath(path);

	if (!sector) return false;

	size_t bufsize = sector->GetSectorBufferSize();
	wxUint8 *buf = sector->GetSectorBuffer();
	if (buf == NULL || bufsize <= 0) return false;

	wxFile outfile(path, wxFile::write);
	if (!outfile.IsOpened()) return false;

	Utils::TempData tbuf;
	tbuf.SetData(buf, bufsize, parent->InvertData());
	outfile.Write(tbuf.GetData(), tbuf.GetSize());

	return true;
}

/// インポートダイアログ表示
bool UiDiskRawSector::ShowImportDataFileDialog()
{
	UiDiskFileDialog dlg(
		_("Import data to sector"),
		frame->GetIniExportFilePath(),
		wxEmptyString,
		_("All files (*.*)|*.*"),
		wxFD_OPEN);

	int dlgsts = dlg.ShowModal();
	wxString path = dlg.GetPath();

	if (dlgsts != wxID_OK) {
		return false;
	}

	int st_trk = -1;
	int st_sid = 0;
	int st_sec = 0;

	return parent->ShowImportTrackRangeDialog(path, st_trk, st_sid, st_sec);
}

/// セクタを削除(ゼロクリア)
void UiDiskRawSector::DeleteSector()
{
	int pos = 0;
	DiskImageSector *sector = GetSelectedSector(&pos);
	if (!sector) return;

	int ans = wxYES;
	wxString msg = wxString::Format(_("Do you really want to clear current sector?"));
	ans = wxMessageBox(msg, _("Clear Sector"), wxYES_NO);
	if (ans == wxYES) {
		sector->Fill(0);

		// 画面更新
		parent->RefreshAllData();
	}
}

/// セクタを編集
void UiDiskRawSector::EditSector()
{
	int pos = 0;
	DiskImageSector *sector = GetSelectedSector(&pos);
	if (!sector) return;

	size_t bufsize = sector->GetSectorBufferSize();
	wxUint8 *buf = sector->GetSectorBuffer();
	if (buf == NULL || bufsize <= 0) {
		wxMessageBox(_("No sector data exists."), _("Edit Sector"), wxICON_ERROR | wxOK);
		return;
	}

	// データを反転して出力するか
	bool inverted = false;
	inverted = parent->InvertData();

	UiDiskApp *app = &wxGetApp();

	// テンポラリディレクトリを作成
	wxString tmp_dir_name;
	if (!app->MakeTempDir(tmp_dir_name)) {
		return;
	}

	// セクタデータを出力
	int trk = m_track_num;
	int sid = m_side_num;
	int sec = sector->GetNumber() - m_start_sector;
	wxString tmp_file_name;
	tmp_file_name = wxString::Format(wxT("sector_%d_%d_%d.dat"), trk, sid, sec);

	wxFileName tmp_path(tmp_dir_name, tmp_file_name);

	wxFile outfile(tmp_path.GetFullPath(), wxFile::write);
	if (!outfile.IsOpened()) return;
	if (inverted) mem_invert(buf, bufsize);
	outfile.Write((const void *)buf, bufsize);
	outfile.Close();
	if (inverted) mem_invert(buf, bufsize);

	// エディタを起動
	if (!frame->OpenFileWithEditor(EDITOR_TYPE_BINARY, tmp_path)) {
		// コマンド起動失敗
		return;
	}

	// ファイルを読み込む
	wxFile infile(tmp_path.GetFullPath(), wxFile::read);
	if (!infile.IsOpened()) return;
	if (inverted) mem_invert(buf, bufsize);
	infile.Read((void *)buf, bufsize);
	infile.Close();
	if (inverted) mem_invert(buf, bufsize);
}
