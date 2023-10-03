/// @file rawtrackbox.cpp
///
/// @brief Rawトラックダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "rawtrackbox.h"
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/numformatter.h>
#include <wx/valtext.h>
#include "../diskimg/diskimage.h"


// Attach Event
BEGIN_EVENT_TABLE(RawTrackBox, wxDialog)
//	EVT_BUTTON(wxID_OK, RawTrackBox::OnOK)
END_EVENT_TABLE()

RawTrackBox::RawTrackBox(wxWindow* parent, wxWindowID id, int num, wxUint32 offset, DiskImageDisk *disk)
	: wxDialog(parent, id, _("Track Information"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
//	wxTextValidator validate(wxFILTER_ALPHANUMERIC);

//	wxSizerFlags flagsr = wxSizerFlags().Align(wxALIGN_RIGHT);
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);
//	wxGridSizer *grid;
//	wxStaticText *lbl;
	wxString str;

	wxSizer *szrButtons = CreateButtonSizer(wxOK);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

int RawTrackBox::ShowModal()
{
	return wxDialog::ShowModal();
}
