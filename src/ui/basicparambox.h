﻿/// @file basicparambox.h
///
/// @brief BASIC情報ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef BASICPARAMBOX_H
#define BASICPARAMBOX_H

#include "../common.h"
#include <wx/dialog.h>
#include <wx/dynarray.h>
#include "basicselbox.h"
#include "../basicfmt/basicparam.h"


class wxListBox;
class wxTextCtrl;
class wxChoice;
class wxStaticText;
class wxListView;
class DiskBasic;
class DiskBasicParam;
class DiskImageDisk;

/// BASIC情報ボックス
class BasicParamBox : public wxDialog, public VolumeCtrl
{
private:
	DiskBasic *basic;
	DiskBasicParamPtrs params;
	int  m_show_flags;

	wxChoice	*comBasic;
	int  m_selected_basic;
	bool m_open_forcely;

public:
	BasicParamBox(wxWindow* parent, wxWindowID id, const wxString &caption, DiskImageDisk *disk, DiskBasic *basic, int show_flags);

	enum {
		IDC_TEXT_BASIC = 1,
		IDC_LIST_BASIC,
		IDC_VOLUME_CTRL,
		IDC_BUTTON_FORCE,
	};

	enum en_show_flags {
		BASIC_SELECTABLE = 0x01
	};

	/// @name functions
	//@{
	int ShowModal();
	void CommitData();
	//@}

	// event procedures
	void OnOK(wxCommandEvent& event);
	void OnOKForcely(wxCommandEvent& event);

	// properties
	bool IsChangedBasic() const;
	const DiskBasicParam *GetBasicParam() const;
	bool WillOpenForcely() const;

	wxDECLARE_EVENT_TABLE();
};

#endif /* BASICPARAMBOX_H */

