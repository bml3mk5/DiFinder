/// @file bootselbox.h
///
/// @brief ブート種類選択ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef BOOTSELBOX_H
#define BOOTSELBOX_H

#include "../common.h"
#include <wx/dialog.h>
#include <wx/dynarray.h>

class wxListBox;
class wxTextCtrl;
class DiskImageFile;
class BootParam;

/// ブート種類選択ボックス
class BootSelBox : public wxDialog
{
private:
	wxListBox *comBoot;

public:
	BootSelBox(wxWindow* parent, wxWindowID id, DiskImageFile *file, int show_flags);

	enum {
		IDC_LIST_BOOT = 1,
	};

	/// @name functions
	//@{
	int ShowModal();
	bool ValidateAllParam();

	void ChangeBootParam(int sel);
	//@}

	// event procedures
	void OnBootChanged(wxCommandEvent& event);
	void OnOK(wxCommandEvent& event);

	// properties
	const BootParam *GetBootParam() const;

	wxDECLARE_EVENT_TABLE();
};

#endif /* BOOTSELBOX_H */

