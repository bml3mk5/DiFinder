/// @file bootselbox.cpp
///
/// @brief ブート種類選択ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "bootselbox.h"
#include <wx/string.h>
#include <wx/listbox.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include "../diskimg/diskimage.h"
#include "../diskimg/bootparam.h"
#include "../utils.h"


// Attach Event
BEGIN_EVENT_TABLE(BootSelBox, wxDialog)
	EVT_LISTBOX(IDC_LIST_BOOT, BootSelBox::OnBootChanged)
	EVT_BUTTON(wxID_OK, BootSelBox::OnOK)
END_EVENT_TABLE()

BootSelBox::BootSelBox(wxWindow* parent, wxWindowID id, DiskImageFile *file, int show_flags)
	: wxDialog(parent, id, _("Select Bootstrap Type"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);

	comBoot = new wxListBox(this, IDC_LIST_BOOT, wxDefaultPosition, wxDefaultSize);
	szrAll->Add(comBoot, flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK | wxCANCEL);
	szrAll->Add(szrButtons, flags);

	for(size_t i=0; i<gBootTemplates.Count(); i++) {
		const BootParam *param = gBootTemplates.ItemPtr(i);
		wxString item = param->GetDescription();
		comBoot->Append(item);
	}

	SetSizerAndFit(szrAll);
}

int BootSelBox::ShowModal()
{
	return wxDialog::ShowModal();
}

void BootSelBox::OnOK(wxCommandEvent& event)
{
	if (Validate() && TransferDataFromWindow()) {
		if (IsModal()) {
			EndModal(wxID_OK);
		} else {
			SetReturnCode(wxID_OK);
			this->Show(false);
		}
	}
}

void BootSelBox::OnBootChanged(wxCommandEvent& event)
{
	int num = event.GetSelection();
	if (num == wxNOT_FOUND) return;

	ChangeBootParam(num);
}

void BootSelBox::ChangeBootParam(int sel)
{
	const BootParam *param = gBootTemplates.ItemPtr(sel);
	if (!param) return;
}

const BootParam *BootSelBox::GetBootParam() const
{
	const BootParam *match = NULL;

	int num = comBoot->GetSelection();
	if (num == wxNOT_FOUND) return match;

	match = gBootTemplates.ItemPtr(num);

	return match;
}
