/// @file diskparambox.h
///
/// @brief ディスクパラメータダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef DISKPARAMBOX_H
#define DISKPARAMBOX_H

#define DISKPARAMBOX_TRANS \
_("Required information entry is empty.") \
_("'%s' is invalid") \
_("Validation conflict") \
_("'%s' should only contain digits.") \
_("'%s' should only contain ASCII characters.")

#include "../common.h"
#include <wx/dialog.h>
#include <wx/arrstr.h>


class wxComboBox;
class wxChoice;
class wxTextCtrl;
class wxCheckBox;
class wxRadioButton;

class DiskParam;
class DiskParamPtrs;
class DiskImageFile;

/// ディスクパラメータボックス
class DiskParamBox : public wxDialog
{
public:
	enum OpeFlags {
		SELECT_DISK_TYPE = 1,
		ADD_NEW_DISK,
		CREATE_NEW_DISK,
		CHANGE_DISK_PARAM,
		SHOW_DISK_PARAM,
		REBUILD_TRACKS,
	};

	enum en_show_flags {
		SHOW_ALL			 = 0xffff,
		SHOW_CATEGORY		 = 0x0001,
		SHOW_TEMPLATE		 = 0x0002,
		SHOW_TEMPLATE_ALL	 = 0x0003,
		SHOW_DISKLABEL_ALL	 = 0x0010,
		SHOW_FILEBTN_ALL	 = 0x0020,
	};

private:
	wxChoice   *comCategory;
	wxChoice   *comTemplate;
	wxTextCtrl *txtTracks;
	wxTextCtrl *txtSides;
	wxTextCtrl *txtSectors;
	wxComboBox *comSecSize;
	wxTextCtrl *txtSecIntl;
	wxTextCtrl *txtDiskSize;

	wxTextCtrl *txtDiskName;
	wxButton   *btnFile;

	OpeFlags m_ope_flags;
	int m_show_flags;
	const DiskParamPtrs *p_disk_params;
	const DiskParam *p_manual_param;
	DiskImageFile *p_file;
	bool now_manual_setting;

	wxArrayString m_type_names;

	int FindTemplate(DiskImageFile *file);
	void SetParamFromTemplate(const DiskParam *item);
	void SetParamOfIndexFromGlobals(size_t index);
	void SetParamOfIndexFromParams(size_t index);
	void SetParamForManual();
	void SetParamToControl(const DiskParam *item);
	bool GetParamFromGlobals(DiskParam &param);
	bool GetParamFromParams(DiskParam &param);
	void GetParamForManual(DiskParam &param);

public:
	DiskParamBox(wxWindow* parent, wxWindowID id, OpeFlags ope_flags, int select_number, DiskImageFile *file, const DiskParamPtrs *params, const DiskParam *manual_param, int show_flags);

	enum {
		IDC_COMBO_CATEGORY = 1,
		IDC_COMBO_TEMPLATE,
		IDC_TEXT_TRACKS,
		IDC_TEXT_SIDES,
		IDC_TEXT_SECTORS,
		IDC_COMBO_SECSIZE,
		IDC_TEXT_INTERLEAVE,
		IDC_COMBO_NUMBSEC,
		IDC_TEXT_DISKSIZE,
		IDC_TEXT_DISKNAME,
		IDC_BTN_FILE,
	};

	/// @name functions
	//@{
	int ShowModal();
	bool ValidateAllParam();
	//@}

	// event procedures
	void OnCategoryChanged(wxCommandEvent& event);
	void OnTemplateChanged(wxCommandEvent& event);
	void OnParameterChanged(wxCommandEvent& event);
	void OnSelectFileButton(wxCommandEvent& event);
	void OnOK(wxCommandEvent& event);

	// properties
	void SetTemplateValues(bool all);
	void SetTemplateValuesFromGlobals(bool all);
	void SetTemplateValuesFromGlobalsSub(int flags);
	void SetTemplateValuesFromParams();
	void SetParamOfIndex(size_t index);
	void SetParamFromFile(const DiskImageFile *file);
	void CalcDiskSize();
	bool GetParam(DiskParam &param);
	bool GetParamToFile(DiskImageFile &file);
	wxString GetCategory() const;
	int GetTracksPerSide() const;
	int GetSidesPerDisk() const;
	int GetSectorsPerTrack() const;
	int GetSectorSize() const;
	int GetInterleave() const;
	wxString GetDiskName() const;

	wxDECLARE_EVENT_TABLE();
};

#endif /* DISKPARAMBOX_H */

