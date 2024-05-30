/// @file partitionbox.h
///
/// @brief パーティション情報ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef PARTITIONBOX_H
#define PARTITIONBOX_H

#include "../common.h"
#include <wx/dialog.h>

class wxSizer;
class wxTextCtrl;
class wxListCtrl;
class UiDiskProcess;
class DiskImageFile;
class DiskImageDisk;

/// パーティション情報ボックス
class PartitionBox : public wxDialog
{
private:
	bool m_initialized;
	wxTextCtrl *txtIPL;
	wxListCtrl *lstPartition;
	wxSize m_oldsize;

	wxSizer *szrButtons;

public:
	PartitionBox(UiDiskProcess *frame, wxWindow* parent, wxWindowID id, const DiskImageFile *file, const DiskImageDisk *current_disk, int show_flags);

	enum {
		IDC_TEXT_IPL = 1,
		IDC_LIST_PARTITION,
	};

	enum en_show_flags {
		SHOW_ATTR_NONE = 0x00
	};

	/// @name functions
	//@{
	int ShowModal();
	//@}

	// event procedures
	void OnOK(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);

	// properties

	wxDECLARE_EVENT_TABLE();
};

#endif /* PARTITIONBOX_H */

