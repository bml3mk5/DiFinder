/// @file diskparambox.cpp
///
/// @brief ディスクパラメータダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskparambox.h"
#include "../basicfmt/basicparam.h"
#include "../diskimg/bootparam.h"
#include "../diskimg/diskimage.h"
#include "../diskimg/fileparam.h"

#ifndef USE_CONSOLE

#include <wx/numformatter.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/radiobut.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/valtext.h>
#include <wx/msgdlg.h>
#include "filedirbox.h"
#include "uimainframe.h"


// Attach Event
BEGIN_EVENT_TABLE(DiskParamBox, wxDialog)
	EVT_CHOICE(IDC_COMBO_CATEGORY, DiskParamBox::OnCategoryChanged)
	EVT_CHOICE(IDC_COMBO_TEMPLATE, DiskParamBox::OnTemplateChanged)
	EVT_TEXT(IDC_TEXT_TRACKS, DiskParamBox::OnParameterChanged)
	EVT_TEXT(IDC_TEXT_SIDES, DiskParamBox::OnParameterChanged)
	EVT_TEXT(IDC_TEXT_SECTORS, DiskParamBox::OnParameterChanged)
	EVT_BUTTON(IDC_BTN_FILE, DiskParamBox::OnSelectFileButton)
	EVT_BUTTON(wxID_OK, DiskParamBox::OnOK)
END_EVENT_TABLE()

/// @param[in] parent    親ウィンドウ
/// @param[in] id        ウィンドウID
/// @param[in] ope_flags どういう操作か
/// @param[in] select_number 選択番号
/// @param[in] file      ディスクイメージ
/// @param[in] params    ディスクパラメータ
/// @param[in] manual_param 手動設定でのパラメータ
/// @param[in] show_flags 表示フラグ
DiskParamBox::DiskParamBox(wxWindow* parent, wxWindowID id, OpeFlags ope_flags, int select_number, DiskImageFile *file, const DiskParamPtrs *params, const DiskParam *manual_param, int show_flags)
	: wxDialog(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
	int p4 = FromDIP(4);
	int p2 = FromDIP(2);
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, p4);
	wxSizerFlags flags_p2 = wxSizerFlags().Expand().Border(wxALL, p2);
	wxSize size;
	long style = 0;
	m_ope_flags = ope_flags;
	m_show_flags = show_flags;
	p_disk_params = params;
	p_manual_param = manual_param;
	p_file = file;
	now_manual_setting = false;
	wxTextValidator validigits(wxFILTER_EMPTY | wxFILTER_DIGITS);
	wxTextValidator valialpha(wxFILTER_ASCII);
	bool use_template = ((show_flags & SHOW_TEMPLATE_ALL) != 0);
	wxSize fsize = GetTextExtent(wxT("00"));
	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);

	//
	//
	//

	comCategory = NULL;
	comTemplate = NULL;
	wxBoxSizer *vbox;
	if (use_template) {
		vbox = new wxBoxSizer(wxVERTICAL);

		if ((show_flags & SHOW_CATEGORY) != 0) {
			vbox->Add(new wxStaticText(this, wxID_ANY, _("Category:")), flags);
			comCategory = new wxChoice(this, IDC_COMBO_CATEGORY, wxDefaultPosition, wxDefaultSize);

			const BootCategories *categories = &gBootTemplates.GetCategories();
			comCategory->Append(_("All"));
			for(size_t i=0; i < categories->Count(); i++) {
				BootCategory *item = &categories->Item(i);
				wxString str = item->GetDescription();
				comCategory->Append(str);
			}
			vbox->Add(comCategory, flags);
		}

		if ((show_flags & SHOW_TEMPLATE) != 0) {
			vbox->Add(new wxStaticText(this, wxID_ANY, _("Template:")), flags);
			comTemplate = new wxChoice(this, IDC_COMBO_TEMPLATE, wxDefaultPosition, wxDefaultSize);
			vbox->Add(comTemplate, flags);

			szrAll->Add(vbox, flags);
		}
	}

	//
	//
	//

	vbox = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer *hboxAll = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);

	hbox->Add(new wxStaticText(this, wxID_ANY, _("Sector Size")), flags);
	size.x = fsize.x * 4 + FromDIP(16); size.y = -1;
	comSecSize = new wxComboBox(this, IDC_COMBO_SECSIZE, wxEmptyString, wxDefaultPosition, size, 0, NULL, wxCB_DROPDOWN | wxCB_READONLY);
	for(int i=0; gSectorSizes[i] != 0; i++) {
		comSecSize->Append(wxString::Format(wxT("%d"), gSectorSizes[i]));
	}
	comSecSize->SetSelection(0);
	hbox->Add(comSecSize, 0);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("bytes")), flags);
//	hbox->AddSpacer(fsize.x);

	vbox->Add(hbox, flags);

	hboxAll->Add(vbox, flags);

	//

	vbox = new wxBoxSizer(wxVERTICAL);

	hbox = new wxBoxSizer(wxHORIZONTAL);

	size.x = fsize.x * 6; size.y = -1;
	txtTracks = new wxTextCtrl(this, IDC_TEXT_TRACKS, wxEmptyString, wxDefaultPosition, size, style, validigits);
	txtTracks->SetMaxLength(12);
	hbox->Add(txtTracks, 0);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Tracks/Side")), flags);
	hbox->AddSpacer(fsize.x);

	size.x = fsize.x * 3; size.y = -1;
	txtSides = new wxTextCtrl(this, IDC_TEXT_SIDES, wxEmptyString, wxDefaultPosition, size, style, validigits);
	txtSides->SetMaxLength(2);
	hbox->Add(txtSides, 0);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Side(s)/Disk")), flags);
	hbox->AddSpacer(fsize.x);

	txtSectors = new wxTextCtrl(this, IDC_TEXT_SECTORS, wxEmptyString, wxDefaultPosition, size, style, validigits);
	txtSectors->SetMaxLength(4);
	hbox->Add(txtSectors, 0);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Sectors/Track")), flags);

	txtSecIntl = NULL;
#if 0
	hbox->AddSpacer(fsize.x);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Interleave")), flags);
	txtSecIntl = new wxTextCtrl(this, IDC_TEXT_INTERLEAVE, wxEmptyString, wxDefaultPosition, size, style, validigits);
	txtSecIntl->SetMaxLength(2);
	hbox->Add(txtSecIntl, 0);
#endif

	vbox->Add(hbox, flags);

	//

	hbox = new wxBoxSizer(wxHORIZONTAL);

	size.x = fsize.x * 8; size.y = -1;
	hbox->Add(new wxStaticText(this, wxID_ANY, _("Number of Sectors:")), flags);
	txtBlocks = new wxTextCtrl(this, IDC_TEXT_BLOCKS, wxEmptyString, wxDefaultPosition, size, style, validigits);
	hbox->Add(txtBlocks, 0);

	vbox->Add(hbox, flags);

	hboxAll->Add(vbox, flags);

	szrAll->Add(hboxAll, flags);

	//

	hbox = new wxBoxSizer(wxHORIZONTAL);

	hbox->Add(new wxStaticText(this, wxID_ANY, _("Data Size")), flags);
	size.x = fsize.x * 12; size.y = -1;
	txtDiskSize = new wxTextCtrl(this, IDC_TEXT_DISKSIZE, wxEmptyString, wxDefaultPosition, size, wxTE_RIGHT | wxTE_READONLY);
	hbox->Add(txtDiskSize, 0);
	hbox->Add(new wxStaticText(this, wxID_ANY, _("bytes")), flags);

	szrAll->Add(hbox, flags);

	//
	//
	//

	txtDiskName = NULL;
	btnFile = NULL;

	if ((show_flags & SHOW_DISKLABEL_ALL) != 0) {
		hbox = new wxBoxSizer(wxHORIZONTAL);
		hbox->Add(new wxStaticText(this, wxID_ANY, _("Image File Path")), flags);
		size.x = fsize.x * 32; size.y = -1;
		txtDiskName = new wxTextCtrl(this, IDC_TEXT_DISKNAME, wxEmptyString, wxDefaultPosition, size, style);
		hbox->Add(txtDiskName, flags_p2);
		btnFile = new wxButton(this, IDC_BTN_FILE, _("File..."));
		hbox->Add(btnFile, flags);

		szrAll->Add(hbox, flags);
	}

	//
	//
	//

	switch(ope_flags) {
	case SELECT_DISK_TYPE:
		SetTitle(_("Select Disk Type"));
		break;
	case ADD_NEW_DISK:
		SetTitle(_("Add New Disk"));
		break;
	case CREATE_NEW_DISK:
		SetTitle(_("Create New Disk"));
		break;
	case CHANGE_DISK_PARAM:
		SetTitle(_("Change Disk Parameter"));
		break;
	case SHOW_DISK_PARAM:
		SetTitle(_("Disk Parameter"));
		break;
	case REBUILD_TRACKS:
		SetTitle(_("Rebuild Tracks"));
			break;
	default:
		SetTitle(_("Unknown"));
		break;
	}

	//
	//
	//

	wxSizer *szrButtons = CreateButtonSizer(wxOK | wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);

	//
	if (comCategory) comCategory->SetSelection(0);
	SetTemplateValues(true);

	//
	if (manual_param) {
		// 手動設定の初期値をセット
		SetParamToControl(manual_param);
	}
	if (file) {
		// 元ディスクのパラメータをセット
		SetParamFromFile(file);
	}
	if (use_template) {
		// テンプレートの初期選択肢
		int sel_num = 0;
		if (manual_param) {
			// 手動設定のときは一番下
			sel_num = (int)gDiskTemplates.Count();
		} else if (select_number >= 0) {
			sel_num = select_number;
		} else if (file) {
			sel_num = FindTemplate(file);
		}
		comTemplate->SetSelection(sel_num);
		SetParamOfIndex(sel_num);
	}

	bool ena = ((show_flags & SHOW_FILEBTN_ALL) != 0);
	if (txtDiskName) txtDiskName->Enable(ena);
	if (btnFile) btnFile->Enable(ena);
}

int DiskParamBox::FindTemplate(DiskImageFile *file)
{
	int idx = gDiskTemplates.IndexOf(file->GetDiskTypeName());
	if (idx < 0) {
		// 手動
		idx = (int)gDiskTemplates.Count();
	}
	return idx;
}

int DiskParamBox::ShowModal()
{
	return wxDialog::ShowModal();
}

void DiskParamBox::OnSelectFileButton(wxCommandEvent& event)
{
	UiDiskFrame *frame = (UiDiskFrame *)GetParent();
	UiDiskSaveFileDialog dlg(_("Save File"),
		frame->GetIniRecentPath(),
		wxEmptyString,
		gFileTypes.GetWildcardForSave());
	int rc = dlg.ShowModal();
	if (rc == wxID_OK) {
		if (txtDiskName) {
			txtDiskName->SetValue(dlg.GetPath());
		}
	}
}

void DiskParamBox::OnOK(wxCommandEvent& event)
{
	if (Validate() && TransferDataFromWindow() && ValidateAllParam()) {
		if (IsModal()) {
			EndModal(wxID_OK);
		} else {
			SetReturnCode(wxID_OK);
			this->Show(false);
		}
	}
}

/// Validate entered parameters
bool DiskParamBox::ValidateAllParam()
{
	int valid = 0;
	wxString msg;
//	int trk = GetTracksPerSide();
//	int sid = GetSidesPerDisk();
//	int sec = GetSectorsPerTrack();
//	int inl = GetInterleave();
	if (p_disk_params) {
		if (comTemplate) {
			int i = comTemplate->GetSelection();
			if (i < ((int)p_disk_params->Count() - 1)) {
				if (p_disk_params->Item(i) == NULL) {
					return false;
				}
			}
		}
	} else {
		if (comTemplate) {
			int i = comTemplate->GetSelection();
			int temp_pos = -1;
			if (i >= 0) temp_pos = (int)(intptr_t)comTemplate->GetClientData((wxUint32)i);
			if (temp_pos < 0) {
				return false;
			}
		}
	}

	if (m_show_flags & SHOW_FILEBTN_ALL) {
		if (txtDiskName->IsEmpty()) {
			msg  = _("The image file path is required.");
			valid = -1; // error
		}
	}
	if (valid > 0) {
		if (!msg.IsEmpty()) msg += wxT("\n\n");
		switch(m_ope_flags) {
		case ADD_NEW_DISK:
		case CREATE_NEW_DISK:
			msg += _("Are you sure to create a disk forcely?");
			break;
		case REBUILD_TRACKS:
			msg += _("Are you sure to create tracks forcely?");
			break;
		case CHANGE_DISK_PARAM:
			msg += _("Are you sure to change it forcely?");
			break;
		default:
			msg += _("Are you sure to read the disk forcely?");
			break;
		}
		int ans = wxMessageBox(msg, _("Invalid parameter"), wxYES_NO | wxICON_EXCLAMATION);
		valid = (ans == wxYES ? 0 : -1);
	} else if (valid < 0) {
		wxMessageBox(msg, _("Invalid parameter"), wxOK | wxICON_ERROR);
	}
	return (valid >= 0);
}

/// カテゴリを変更した
void DiskParamBox::OnCategoryChanged(wxCommandEvent& event)
{
	m_category_name.Empty();
	int pos = event.GetSelection();
	if (pos <= 0) {
		// All items
		m_boot_type_names.Clear();
	} else {
		// カテゴリと一致するブート種類名リストを得る
		if (gBootTemplates.FindTypeNames(pos - 1, m_boot_type_names) <= 0) {
			// 一致するものがないのでカテゴリ名を保持
			m_category_name = gBootTemplates.GetCategoryName(pos - 1);
		}
	}
	SetTemplateValues(pos <= 0);
}

void DiskParamBox::SetTemplateValues(bool all)
{
	if (p_disk_params != NULL && p_manual_param == NULL) SetTemplateValuesFromParams();
	else SetTemplateValuesFromGlobals(all);
}

/// ディスクテンプレートから候補をコンボリストに追加
/// @param[in] all 全候補
void DiskParamBox::SetTemplateValuesFromGlobals(bool all)
{
	if (!comTemplate) return;

	comTemplate->Clear();
	if (all) {
		SetTemplateValuesFromGlobalsSub(-1);
	} else {
		// 推奨データ候補
		SetTemplateValuesFromGlobalsSub(1);
		if (comTemplate->GetCount() > 0) {
			wxString str = wxT("----------");
			comTemplate->Append(str, (void *)-1);
		}
		// 一般データ候補
		SetTemplateValuesFromGlobalsSub(0);
		// minorな候補
		SetTemplateValuesFromGlobalsSub(2);
	}
	comTemplate->Append(_("Manual Setting"));
	comTemplate->SetSelection(0);

	SetParamOfIndex(0);
}

/// ディスクテンプレートからフラグに一致するものをコンボリストに追加
/// @param[in] flags -1:すべて  1:推奨データ  0:一般データ
void DiskParamBox::SetTemplateValuesFromGlobalsSub(int flags)
{
	bool list_all = (m_boot_type_names.Count() <= 0 && m_category_name.IsEmpty());
	if (list_all) {
		// 全てを候補にする
		for(size_t i=0; i < gDiskTemplates.Count(); i++) {
			const DiskParam *item = gDiskTemplates.ItemPtr(i);
			wxString str = item->GetDiskParamDetails();
			comTemplate->Append(str, (void *)i);
		}
	} else {
		// 一致するものを候補にする
		for(size_t i=0; i < gDiskTemplates.Count(); i++) {
			const DiskParam *item = gDiskTemplates.ItemPtr(i);
			// ブート種類が一致するか
			bool match = (item->MatchBootType(m_boot_type_names, flags) != wxNOT_FOUND);
			// カテゴリ名が設定されている場合
			if (!match && flags == 0 && !m_category_name.IsEmpty()) {
				match = (item->MatchCategory(m_category_name) != wxNOT_FOUND);
			}
			if (!match) {
				continue;
			}
			wxString str = item->GetDiskParamDetails();
			comTemplate->Append(str, (void *)i);
		}
	}
}

/// パラメータリストからコンボリストに追加
/// @note disk_params パラメータリスト
void DiskParamBox::SetTemplateValuesFromParams()
{
	if (!comTemplate || !p_disk_params) return;

	comTemplate->Clear();
	for(size_t i=0; i < p_disk_params->Count(); i++) {
		const DiskParam *item = p_disk_params->Item(i);
		wxString str;
		int num;
		if (item) {
			str = item->GetDiskParamDetails();
			num = (int)i;
		} else {
			// NULLのとき
			str = wxT("----------");
			num = -1;
		}
		comTemplate->Append(str, (void *)(intptr_t)num);
	}
	comTemplate->Append(_("Manual Setting"));
	comTemplate->SetSelection(0);

	SetParamOfIndex(0);
}

void DiskParamBox::OnTemplateChanged(wxCommandEvent& event)
{
	SetParamOfIndex(event.GetSelection());
}

void DiskParamBox::OnParameterChanged(wxCommandEvent& event)
{
	CalcDiskSize();
}

/// 指定位置のコントロールをセット
void DiskParamBox::SetParamOfIndex(size_t index)
{
	if (p_disk_params != NULL && p_manual_param == NULL) SetParamOfIndexFromParams(index);
	else SetParamOfIndexFromGlobals(index);
}

/// ディスクテンプレートから一致するディスクを得てコントロールにセット
void DiskParamBox::SetParamOfIndexFromGlobals(size_t index)
{
	if (index < (comTemplate->GetCount() - 1)) {
		int temp_pos = (int)(intptr_t)comTemplate->GetClientData((wxUint32)index);
		const DiskParam *item = NULL;
		if (temp_pos >= 0) item = gDiskTemplates.ItemPtr((size_t)temp_pos);
		if (item) SetParamFromTemplate(item);
	} else {
		SetParamForManual();
		if (p_manual_param) {
			SetParamToControl(p_manual_param);
		}
	}
	bool ena = ((m_show_flags & SHOW_FILEBTN_ALL) != 0);
	if (txtDiskName) txtDiskName->Enable(ena);
	if (btnFile) btnFile->Enable(ena);
}

/// パラメータリストから一致するディスクを得てコントロールにセット
void DiskParamBox::SetParamOfIndexFromParams(size_t index)
{
	if (index < (comTemplate->GetCount() - 1)) {
		const DiskParam *item = p_disk_params->Item(index);
		if (item) SetParamFromTemplate(item);
	} else {
		SetParamForManual();
		if (p_manual_param) {
			SetParamToControl(p_manual_param);
		}
	}
}

/// パラメータの情報を各コントロールに設定
void DiskParamBox::SetParamFromTemplate(const DiskParam *item)
{
	now_manual_setting = false;

	SetParamToControl(item);

	txtTracks->Enable(false);
	txtSides->Enable(false);
	txtSectors->Enable(false);
	comSecSize->Enable(false);
	if (txtSecIntl) txtSecIntl->Enable(false);
	if (txtBlocks) txtBlocks->Enable(false);
}

/// ディスクの情報を各コントロールに設定
void DiskParamBox::SetParamFromFile(const DiskImageFile *file)
{
	now_manual_setting = false;

	SetParamToControl(file);
	if (txtDiskName) {
		txtDiskName->SetValue(file->GetFilePath());
	}

	txtTracks->Enable(false);
	txtSides->Enable(false);
	txtSectors->Enable(false);
	comSecSize->Enable(false);
	if (txtSecIntl) txtSecIntl->Enable(false);
	if (txtBlocks) txtBlocks->Enable(false);
}

/// 手動設定を選んだ時の各コントロールを設定
void DiskParamBox::SetParamForManual()
{
	// manual
	txtTracks->Enable(true);
	txtSides->Enable(true);
	txtSectors->Enable(true);
	comSecSize->Enable(true);
	if (txtSecIntl) txtSecIntl->Enable(true);
	if (txtBlocks) txtBlocks->Enable(true);

	now_manual_setting = true;
}

/// 各コントロールにパラメータ値をセット
void DiskParamBox::SetParamToControl(const DiskParam *item)
{
	txtTracks->SetValue(wxString::Format(wxT("%d"), item->GetTracksPerSide()));
	txtSides->SetValue(wxString::Format(wxT("%d"), item->GetSidesPerDisk()));
	txtSectors->SetValue(wxString::Format(wxT("%d"), item->GetSectorsPerTrack()));
	comSecSize->SetValue(wxString::Format(wxT("%d"), item->GetSectorSize()));
	if (txtSecIntl) txtSecIntl->SetValue(wxString::Format(wxT("%d"), item->GetInterleave()));
	txtBlocks->SetValue(wxString::Format(wxT("%d"), item->CalcNumberOfBlocks()));

	txtDiskSize->SetValue(wxNumberFormatter::ToString(item->CalcDiskSize()));

//	int single_secs = 0;
//	int single_size = 0;
//	int single_pos = item->HasSingleDensity(&single_secs, &single_size);
}

/// パラメータを得る
bool DiskParamBox::GetParam(DiskParam &param)
{
	if (p_disk_params != NULL && p_manual_param == NULL) return GetParamFromParams(param);
	else return GetParamFromGlobals(param);
}

/// ディスクサイズを計算
void DiskParamBox::CalcDiskSize()
{
	if (!now_manual_setting) return;

	// 手動設定の時は計算する
	int snum = -1; // GetSingleNumber();
	int ntrks = GetTracksPerSide();
	int nsids = GetSidesPerDisk();
	int strks = 0;
	switch(snum) {
	case 3:
		// track0, both sides
		strks = nsids;
		break;
	case 2:
		// track0, side0
		strks = 1;
		break;
	case 1:
		// all tracks
		strks = ntrks * nsids;
		break;
	default:
		// no track
		break;
	}
	ntrks *= nsids;
	ntrks -= strks;

	int val = (ntrks * GetSectorSize() * GetSectorsPerTrack());

	txtDiskSize->SetValue(wxNumberFormatter::ToString((long)val));
}

/// ダイアログのパラメータを取得
/// @param [out] param
/// @return true: テンプレートから false:手動設定
bool DiskParamBox::GetParamFromGlobals(DiskParam &param)
{
	size_t index = 0;
	if (comTemplate && (index = comTemplate->GetSelection()) < (comTemplate->GetCount() - 1)) {
		int temp_pos = (int)(intptr_t)comTemplate->GetClientData((wxUint32)index);
		if (temp_pos >= 0) param = *gDiskTemplates.ItemPtr((size_t)temp_pos);
		return true;
	} else {
		// manual
		GetParamForManual(param);
		return false;
	}
}

/// ダイアログのパラメータを取得
/// @param [out] param
/// @return true: テンプレートから false:手動設定
bool DiskParamBox::GetParamFromParams(DiskParam &param)
{
	size_t index = 0;
	if (comTemplate && (index = comTemplate->GetSelection()) < (comTemplate->GetCount() - 1)) {
		param = *p_disk_params->Item(index);
		return true;
	} else {
		// manual
		GetParamForManual(param);
		return false;
	}
}

/// ダイアログのパラメータを取得（手動設定）
void DiskParamBox::GetParamForManual(DiskParam &param)
{
	// manual
	DiskParticulars sd;
	DiskParam dummy;
	BootParamNames boot_types;
	param.SetDiskParam(wxT(""),
		boot_types,
		GetNumberOfBlocks(),
		GetSidesPerDisk(),
		GetTracksPerSide(),
		GetSectorsPerTrack(),
		GetSectorSize(),
		0,
		0,
		GetInterleave(),
		0,
		0,
		dummy.IsVariableSectorsPerTrack(),
		sd,
		dummy.GetParticularTracks(),
		dummy.GetParticularSectors(),
		wxArrayString(),
		wxT(""),
		wxT("")
	);
}

/// パラメータをディスクにセット
bool DiskParamBox::GetParamToFile(DiskImageFile &file)
{
	return true;
}

/// 選択したカテゴリタイプを返す
wxString DiskParamBox::GetCategory() const
{
	wxString str;
	if (comCategory) {
		int num = comCategory->GetSelection();
		if (num > 0) {
			const BootCategories *categories = &gBootTemplates.GetCategories();
			str = categories->Item(num-1).GetTypeName();
		}
	}
	return str;
}

/// 選択したブートカテゴリタイプを返す
wxString DiskParamBox::GetBootCategory() const
{
	wxString str = GetCategory();
	if (str == m_category_name) {
		// ブート種類にないカテゴリはカテゴリ名を空にする
		str.Empty();
	}
	return str;
}

/// サイド当たりのトラック数を返す
int DiskParamBox::GetTracksPerSide() const
{
	long val = 0;
	txtTracks->GetValue().ToLong(&val);
	return (int)val;
}

/// サイド数を返す
int DiskParamBox::GetSidesPerDisk() const
{
	long val = 0;
	txtSides->GetValue().ToLong(&val);
	return (int)val;
}

/// トラック当たりのセクタ数を返す
int DiskParamBox::GetSectorsPerTrack() const
{
	long val = 0;
	txtSectors->GetValue().ToLong(&val);
	return (int)val;
}

/// セクタサイズを返す
int DiskParamBox::GetSectorSize() const
{
	int idx = comSecSize->GetSelection();
	return gSectorSizes[idx];
}

/// インターリーブを返す
int DiskParamBox::GetInterleave() const
{
	long val = 1;
	if (txtSecIntl) txtSecIntl->GetValue().ToLong(&val);
	return (int)val;
}

/// ブロック数を返す
int DiskParamBox::GetNumberOfBlocks() const
{
	long val = 0;
	if (txtBlocks) txtBlocks->GetValue().ToLong(&val);
	long chr = GetTracksPerSide() * GetSidesPerDisk() * GetSectorsPerTrack();
	if (chr > 0 && chr == val) {
		// トラック数などで指定している場合はブロック数０とする
		val = 0;
	} 
	return (int)val;
}

/// ディスク名を返す
wxString DiskParamBox::GetDiskName() const
{
	return txtDiskName ? txtDiskName->GetValue() : wxT("");
}

// ----------------------------------------------------------------------------

DiskParamDiskCreateBox::DiskParamDiskCreateBox(wxWindow* parent, wxWindowID id, int show_flags)
	: DiskParamBox(parent, id, DiskParamBox::CREATE_NEW_DISK, 0, NULL, NULL, NULL, show_flags)
{
}

DiskParamDiskAddBox::DiskParamDiskAddBox(wxWindow* parent, wxWindowID id, int select_number, int show_flags)
	: DiskParamBox(parent, id, DiskParamBox::ADD_NEW_DISK, select_number, NULL, NULL, NULL, show_flags)
{
}

DiskParamDiskSelectBox::DiskParamDiskSelectBox(wxWindow* parent, wxWindowID id, const DiskParamPtrs* params, const DiskParam* manual_param, int show_flags)
	: DiskParamBox(parent, id, DiskParamBox::SELECT_DISK_TYPE, 0, NULL, params, manual_param, show_flags)
{
}

#endif /* USE_CONSOLE */
