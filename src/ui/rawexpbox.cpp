/// @file rawexpbox.cpp
///
/// @brief Rawエクスポート＆インポートダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "rawexpbox.h"
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/valtext.h>
#include <wx/msgdlg.h>
#include "../diskimg/diskimage.h"


// Attach Event
BEGIN_EVENT_TABLE(RawExpBox, wxDialog)
	EVT_BUTTON(wxID_OK, RawExpBox::OnOK)
END_EVENT_TABLE()

RawExpBox::RawExpBox(wxWindow* parent, wxWindowID id, const wxString &caption, DiskImageFile *file
	, int start_track_num, int start_side_num, int start_sector_num
	, int end_track_num, int end_side_num, int end_sector_num
)	: wxDialog(parent, id, caption, wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
	p_file = file;

	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);
	long style = 0;
	wxSize size(32,-1);
	wxTextValidator validate(wxFILTER_EMPTY | wxFILTER_DIGITS);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer *hbox;
	for(int i=0; i<2; i++) {
		hbox = new wxBoxSizer(wxHORIZONTAL);
		hbox->Add(new wxStaticText(this, wxID_ANY, i == 0 ? _("Start Sector") : _("End Sector")), flags);
		hbox->Add(new wxStaticText(this, wxID_ANY, wxT("  ")), flags);
		hbox->Add(new wxStaticText(this, wxID_ANY, _("Track")), flags);
		txtTrack[i] = new wxTextCtrl(this, IDC_TEXT_TRACK_ST + i, wxEmptyString, wxDefaultPosition, size, style, validate);
		txtTrack[i]->SetMaxLength(2);
		txtTrack[i]->SetValue(wxString::Format(wxT("%d"), i == 0 || end_track_num < 0 ? start_track_num : end_track_num));
		hbox->Add(txtTrack[i], 0);
		hbox->Add(new wxStaticText(this, wxID_ANY, wxT("  ")), flags);
		hbox->Add(new wxStaticText(this, wxID_ANY, _("Side")), flags);
		txtSide[i] = new wxTextCtrl(this, IDC_TEXT_TRACK_ST + i, wxEmptyString, wxDefaultPosition, size, style, validate);
		txtSide[i]->SetMaxLength(1);
		txtSide[i]->SetValue(wxString::Format(wxT("%d"), (i == 0 || end_side_num < 0 ? start_side_num : end_side_num)));
		hbox->Add(txtSide[i], 0);
		hbox->Add(new wxStaticText(this, wxID_ANY, wxT("  ")), flags);
		hbox->Add(new wxStaticText(this, wxID_ANY, _("Sector")), flags);
		txtSector[i] = new wxTextCtrl(this, IDC_TEXT_TRACK_ST + i, wxEmptyString, wxDefaultPosition, size, style, validate);
		txtSector[i]->SetMaxLength(2);
		txtSector[i]->SetValue(wxString::Format(wxT("%d"), i == 0 || end_sector_num < 0 ? start_sector_num : end_sector_num));
		hbox->Add(txtSector[i], 0);

		szrAll->Add(hbox, flags);
	}

	wxSizer *szrButtons = CreateButtonSizer(wxOK | wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

int RawExpBox::ShowModal()
{
	return wxDialog::ShowModal();
}

void RawExpBox::OnOK(wxCommandEvent& event)
{
	if (Validate() && TransferDataFromWindow() && ValidateParam()) {
		if (IsModal()) {
			EndModal(wxID_OK);
		} else {
			SetReturnCode(wxID_OK);
			this->Show(false);
		}
	}
}

bool RawExpBox::ValidateParam()
{
	return true;
}

int RawExpBox::GetTrackNumber(int num) const
{
	long val = 0;
	wxString str = txtTrack[num]->GetValue();
	str.ToLong(&val);
	return (int)val;
}

int RawExpBox::GetSideNumber(int num) const
{
	long val = 0;
	wxString str = txtSide[num]->GetValue();
	str.ToLong(&val);
	return (int)val;
}

int RawExpBox::GetSectorNumber(int num) const
{
	long val = 0;
	wxString str = txtSector[num]->GetValue();
	str.ToLong(&val);
	return (int)val;
}
