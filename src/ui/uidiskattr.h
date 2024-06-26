﻿/// @file uidiskattr.h
///
/// @brief ディスク属性
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef UIDISKATTR_H
#define UIDISKATTR_H

#include "../common.h"
#include <wx/string.h>
#include <wx/panel.h>


class wxTextCtrl;
class wxButton;
class wxChoice;
class wxCheckBox;
class wxBoxSizer;

class UiDiskFrame;
class DiskImageFile;
class DiskImageDisk;

/// 右パネルの属性
class UiDiskDiskAttr : public wxPanel
{
private:
	wxWindow *parent;
	UiDiskFrame *frame;

	wxTextCtrl *txtAttr;
	wxButton   *btnChange;
	wxCheckBox *chkWprotect;
	wxBoxSizer *szrButtons;

	DiskImageFile *p_file;
	DiskImageDisk *p_disk;

public:
	UiDiskDiskAttr(UiDiskFrame *parentframe, wxWindow *parent);
	~UiDiskDiskAttr();

	void OnSize(wxSizeEvent& event);
	void OnButtonChange(wxCommandEvent& event);
	void OnCheckWriteProtect(wxCommandEvent& event);

	void ShowChangeBootParam();
	void SetAttr(DiskImageFile *n_file);
	void SetAttr(DiskImageDisk *n_disk);

	void SetAttrText(const wxString &val);
	int  GetDiskDensity() const;
	void SetWriteProtect(bool val, bool enable = true);
	bool GetWriteProtect() const;
	void ClearData();

	void SetListFont(const wxFont &font);

	enum {
		IDC_TXT_ATTR = 1,
		IDC_BTN_CHANGE,
//		IDC_COM_DENSITY,
		IDC_CHK_WPROTECT,
	};

	wxDECLARE_EVENT_TABLE();
};

#endif /* UIDISKATTR_H */

