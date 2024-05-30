/// @file partitionbox.cpp
///
/// @brief パーティション情報ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "partitionbox.h"
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/listctrl.h>
#include <wx/sizer.h>
#include "uimainprocess.h"
#include "../diskimg/diskimage.h"
#include "../utils.h"

// Attach Event
BEGIN_EVENT_TABLE(PartitionBox, wxDialog)
	EVT_BUTTON(wxID_OK, PartitionBox::OnOK)
	EVT_SIZE(PartitionBox::OnSize)
END_EVENT_TABLE()

PartitionBox::PartitionBox(UiDiskProcess *frame, wxWindow* parent, wxWindowID id, const DiskImageFile *file, const DiskImageDisk *current_disk, int show_flags)
	: wxDialog(parent, id, _("Partition Information"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER | wxCLOSE_BOX)
{
	m_initialized = false;

	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrAll = new wxBoxSizer(wxVERTICAL);

	txtIPL = new wxTextCtrl(this, IDC_TEXT_IPL, file->GetDescriptionDetails(), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	szrAll->Add(txtIPL, flags);

	lstPartition = new wxListCtrl(this, IDC_LIST_PARTITION, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	szrAll->Add(lstPartition, flags);

	int w = 0;
	int cw;
	cw =  16; lstPartition->InsertColumn(0, wxT(""), wxLIST_FORMAT_LEFT, cw); w += cw;
	cw =  32; lstPartition->InsertColumn(1, wxT("#"), wxLIST_FORMAT_RIGHT, cw); w += cw;
	cw = 128; lstPartition->InsertColumn(2, _("Description"), wxLIST_FORMAT_LEFT, cw); w += cw;
	cw =  96; lstPartition->InsertColumn(3, _("Start Sector"), wxLIST_FORMAT_RIGHT, cw); w += cw;
	cw =  96; lstPartition->InsertColumn(4, _("Start Position"), wxLIST_FORMAT_LEFT, cw); w += cw;
	cw =  96; lstPartition->InsertColumn(5, _("Size (Sectors)"), wxLIST_FORMAT_RIGHT, cw); w += cw;
	cw =  96; lstPartition->InsertColumn(6, _("Size (Bytes)"), wxLIST_FORMAT_RIGHT, cw); w += cw;

	lstPartition->SetSizeHints(w, 128);

	const DiskImageDisks *disks = file->GetDisks();
	if (disks) {
		for(int n = 0; n < (int)disks->Count(); n++) {
			const DiskImageDisk *disk = disks->Item(n);
			wxString str;
			if (current_disk && disk == current_disk) {
				str = wxT("*");
			}
			lstPartition->InsertItem(n, str);
			lstPartition->SetItem(n, 1, wxString::Format(wxT("%d"), n));
			if (disk) {
				str = disk->GetDescription();
				if (str.IsEmpty()) str = _("(no name)");
				lstPartition->SetItem(n, 2, str);
				lstPartition->SetItem(n, 3, disk->GetStartSectorNumberStr(0));
				lstPartition->SetItem(n, 4, disk->GetStartSectorNumberStr(1));
				lstPartition->SetItem(n, 5, disk->GetNumberOfSectorsStr(0));
				lstPartition->SetItem(n, 6, disk->GetNumberOfSectorsStr(1));
			} else {
				lstPartition->SetItem(n, 2, _("(no information)"));
			}
		}
	}

	wxFont font;
	frame->GetDefaultListFont(font);
	lstPartition->SetFont(font);

	szrButtons = CreateButtonSizer(wxOK);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);

	m_oldsize = GetSize();

	m_initialized = true;
}

int PartitionBox::ShowModal()
{
	return wxDialog::ShowModal();
}

void PartitionBox::OnOK(wxCommandEvent& event)
{
	if (IsModal()) {
		EndModal(wxID_OK);
	} else {
		SetReturnCode(wxID_OK);
		this->Show(false);
	}
}

/// リサイズ
/// @param[in] event サイズイベント
void PartitionBox::OnSize(wxSizeEvent& event)
{
	if (!m_initialized) {
		event.Skip();
		return;
	}
	if (event.GetEventObject() != this) {
		event.Skip();
		return;
	}

	// Get the difference of the dialog size
	wxSize subsiz = event.GetSize() - m_oldsize;

	if (subsiz.GetWidth() == 0 && subsiz.GetHeight() == 0) {
		return;
	}

	// Fit the position of buttons to the bottom of the dialog
	wxPoint pos = szrButtons->GetPosition();
	wxSize sz = szrButtons->GetSize();
	pos.y += subsiz.y;
	sz.SetWidth(sz.GetWidth() + subsiz.x);
	szrButtons->SetDimension(pos, sz);

	// Resize the text to fit to the dialog width
	sz = txtIPL->GetSize();
	sz.SetWidth(sz.GetWidth() + subsiz.x);
	txtIPL->SetSize(sz);

	// Resize the list to fit to the dialog width and height
	sz = lstPartition->GetSize();
	sz.SetWidth(sz.GetWidth() + subsiz.x);
	sz.SetHeight(sz.GetHeight() + subsiz.y);
	lstPartition->SetSize(sz);

	m_oldsize = event.GetSize();
}
