/// @file uidiskattr.cpp
///
/// @brief ディスク属性
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "uidiskattr.h"

#ifndef USE_CONSOLE

#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include "../main.h"
#include "uimainframe.h"
#include "../diskimg/diskimage.h"
#include "bootselbox.h"
#include "../utils.h"


//
// 右パネルのディスク属性
//
#define TEXT_ATTR_SIZE 500

// Attach Event
wxBEGIN_EVENT_TABLE(UiDiskDiskAttr, wxPanel)
	EVT_SIZE(UiDiskDiskAttr::OnSize)
	EVT_BUTTON(IDC_BTN_CHANGE, UiDiskDiskAttr::OnButtonChange)
	EVT_CHECKBOX(IDC_CHK_WPROTECT, UiDiskDiskAttr::OnCheckWriteProtect)
wxEND_EVENT_TABLE()

UiDiskDiskAttr::UiDiskDiskAttr(UiDiskFrame *parentframe, wxWindow *parentwindow)
       : wxPanel(parentwindow, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
	parent   = parentwindow;
	frame    = parentframe;

	p_file   = NULL;
	p_disk	 = NULL;

	int p2 = FromDIP(2);
	wxSizerFlags flagsW = wxSizerFlags().Expand().Border(wxALL, p2);
	wxSizerFlags flags_bt2 = wxSizerFlags().Expand().Border(wxBOTTOM | wxTOP, p2);
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrHed = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *szrBtn = new wxBoxSizer(wxHORIZONTAL);
	wxSize size(TEXT_ATTR_SIZE, -1);

	txtAttr = new wxTextCtrl(this, IDC_TXT_ATTR, wxT(""), wxDefaultPosition, size, wxTE_READONLY | wxTE_LEFT);
	szriTxt = szrHed->Add(txtAttr, flags_bt2);

	size.x = 60;
	btnChange = new wxButton(this, IDC_BTN_CHANGE, _("Change"), wxDefaultPosition, FromDIP(size));
	btnChange->Enable(false);
	szrBtn->Add(btnChange, flagsW);

	chkWprotect = new wxCheckBox(this, IDC_CHK_WPROTECT, _("Write Protect"));
	szrBtn->Add(chkWprotect, flagsW);

	szriBtn = szrHed->Add(szrBtn);
	vbox->Add(szrHed);

	wxFont font;
	frame->GetDefaultListFont(font);
	txtAttr->SetFont(font);

	vbox->SetSizeHints(this);

	SetSizerAndFit(vbox);
	Layout();

	ClearData();
}
UiDiskDiskAttr::~UiDiskDiskAttr()
{
}

/// サイズ変更
void UiDiskDiskAttr::OnSize(wxSizeEvent& event)
{
	if (!GetSizer()) {
		event.Skip();
		return;
	}

	wxSize szCli = GetClientSize();
	if (szCli.x < 32) return;

	// コントロールのサイズを計算
	wxSize szTxt = szriTxt->CalcMin();
	wxSize szBtn = szriBtn->CalcMin();

	// テキストエリアのサイズを変更
	szTxt.SetWidth(szCli.GetWidth() - szBtn.GetWidth());
	int text_attr_size = FromDIP(TEXT_ATTR_SIZE);
	if (szTxt.GetWidth() < text_attr_size) {
		// 最小サイズ
		szTxt.SetWidth(text_attr_size);
	}

	// コントロールの再配置
	wxPoint pt;
	szriTxt->SetDimension(pt, szTxt);
	pt.x += szTxt.GetWidth();
	szriBtn->SetDimension(pt, szBtn);
}

/// 変更ボタンを押した
void UiDiskDiskAttr::OnButtonChange(wxCommandEvent& event)
{
	// パラメータを選択するダイアログを表示
	ShowChangeBootParam();
}

/// 書き込み禁止チェックボックスを押した
void UiDiskDiskAttr::OnCheckWriteProtect(wxCommandEvent& event)
{
//	if (!p_disk) return;
	bool checked = event.IsChecked();
	DiskImage *image = &frame->GetDiskImage();
	if (checked && image->IsModified()) {
		// チェックを入れた時、データが変更されている場合は確認
		int rc = wxMessageBox(_("This file is modified. Do you want to save it?"), _("Modified"), wxYES_NO | wxICON_INFORMATION);
		if (rc == wxYES) {
			frame->SaveDataFile();
		}
	}
	image->GetFile()->SetWriteProtect(checked);
}

/// パラメータを選択するダイアログを表示
void UiDiskDiskAttr::ShowChangeBootParam()
{
	if (!p_file) return;

	BootSelBox dlg(this, wxID_ANY, p_file, 0);
	int sts = dlg.ShowModal();
	if (sts == wxID_OK) {
		// ファイルを開きなおす
		const BootParam *param = dlg.GetBootParam();
		if (!param) return;

		frame->ReOpenDataFile(*param);
	}
}

/// ディスクイメージ選択時の情報を設定
/// @param[in] n_file 新イメージファイル
void UiDiskDiskAttr::SetAttr(DiskImageFile *n_file)
{
	p_file = n_file;
	if (!p_file) return;

	wxString desc = p_file->GetDescriptionDetails();

	SetAttrText(desc);
	btnChange->Enable(true);
	SetWriteProtect(p_file->IsWriteProtected());
}

/// パーティション選択時の情報を設定
/// @param[in] n_disk 新パーティション
void UiDiskDiskAttr::SetAttr(DiskImageDisk *n_disk)
{
	p_disk = n_disk;
	if (!p_disk) return;

//	wxString desc = p_disk->GetDescriptionDetails();

//	SetAttrText(desc);

//	DiskImageFile *file = p_disk->GetFile();
//	SetWriteProtect(file->IsWriteProtected());
}

/// 情報を設定
void UiDiskDiskAttr::SetAttrText(const wxString &val)
{
	txtAttr->SetValue(val);
}

/// 書き込み禁止を設定
void UiDiskDiskAttr::SetWriteProtect(bool val, bool enable)
{
	chkWprotect->Enable(enable);
	chkWprotect->SetValue(val);
}

/// 書き込み禁止を返す
bool UiDiskDiskAttr::GetWriteProtect() const
{
	return chkWprotect->GetValue();
}

/// 情報をクリア
void UiDiskDiskAttr::ClearData()
{
	SetAttrText(wxEmptyString);
	btnChange->Enable(false);
	SetWriteProtect(false, false);
}

/// フォントを設定
void UiDiskDiskAttr::SetListFont(const wxFont &font)
{
	txtAttr->SetFont(font);
}

#endif /* !USE_CONSOLE */
