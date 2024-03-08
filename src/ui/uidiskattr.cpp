/// @file uidiskattr.cpp
///
/// @brief ディスク属性
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "uidiskattr.h"
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include "../main.h"
#include "uimainframe.h"
#include "../diskimg/diskimage.h"
#include "diskparambox.h"
#include "../utils.h"


//
// 右パネルのディスク属性
//
#define TEXT_ATTR_SIZE 500

// Attach Event
wxBEGIN_EVENT_TABLE(UiDiskDiskAttr, wxPanel)
	EVT_SIZE(UiDiskDiskAttr::OnSize)
	EVT_CHECKBOX(IDC_CHK_WPROTECT, UiDiskDiskAttr::OnCheckWriteProtect)
wxEND_EVENT_TABLE()

UiDiskDiskAttr::UiDiskDiskAttr(UiDiskFrame *parentframe, wxWindow *parentwindow)
       : wxPanel(parentwindow, wxID_ANY, wxDefaultPosition, wxDefaultSize)
{
	parent   = parentwindow;
	frame    = parentframe;

	p_disk	 = NULL;

	wxSizerFlags flagsW = wxSizerFlags().Expand().Border(wxALL, 2);
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
	szrButtons = new wxBoxSizer(wxHORIZONTAL);
	wxSize size(TEXT_ATTR_SIZE, -1);

	txtAttr = new wxTextCtrl(this, IDC_TXT_ATTR, wxT(""), wxDefaultPosition, size, wxTE_READONLY | wxTE_LEFT);
	hbox->Add(txtAttr, flagsW);

	chkWprotect = new wxCheckBox(this, IDC_CHK_WPROTECT, _("Write Protect"));
	szrButtons->Add(chkWprotect, flagsW);

	hbox->Add(szrButtons);
	vbox->Add(hbox);

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
	wxSize size = event.GetSize();
	wxSize sizz = szrButtons->GetSize();
	if (sizz.x == 0) return;

	int pos_x = size.x - sizz.x;
	if (pos_x < 0) return;

	wxPoint bp;
	bp = chkWprotect->GetPosition();

	pos_x -= bp.x;

	wxSize tz = txtAttr->GetSize();
	tz.x += pos_x;
	if (tz.x < TEXT_ATTR_SIZE) return;

	txtAttr->SetSize(tz);

	wxSizerItemList *slist = &szrButtons->GetChildren();
	wxSizerItemList::iterator it;
	for(it = slist->begin(); it != slist->end(); it++) {
		wxSizerItem *item = *it;
		if (item->IsWindow()) {
			wxWindow *win = item->GetWindow();
			bp = win->GetPosition();
			bp.x += pos_x;
			win->SetPosition(bp);
		}
	}
}

/// 書き込み禁止チェックボックスを押した
void UiDiskDiskAttr::OnCheckWriteProtect(wxCommandEvent& event)
{
	if (!p_disk) return;
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

/// ディスクイメージ選択時の情報を設定
void UiDiskDiskAttr::SetAttr(DiskImageFile *n_file)
{
	if (!n_file) return;

	wxString desc = n_file->GetDescriptionDetails();

	SetAttrText(desc);
	SetWriteProtect(n_file->IsWriteProtected());
}
/// パーティション選択時の情報を設定
void UiDiskDiskAttr::SetAttr(DiskImageDisk *n_disk)
{
	p_disk = n_disk;
	if (!p_disk) return;

	wxString desc = p_disk->GetDescriptionDetails();

	SetAttrText(desc);

	DiskImageFile *file = p_disk->GetFile();
	SetWriteProtect(file->IsWriteProtected());
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
//	btnChange->Enable(false);
//	SetDiskDensity(-1);
	SetWriteProtect(false, false);
}
/// フォントを設定
void UiDiskDiskAttr::SetListFont(const wxFont &font)
{
	txtAttr->SetFont(font);
}
