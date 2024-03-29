﻿/// @file configbox.cpp
///
/// @brief 設定ダイアログ
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "configbox.h"
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include "../config.h"
#include "../main.h"
#include "../basicfmt/basicfmt.h"
#include "../basicfmt/basictype.h"
#include "../diskimg/diskimage.h"
#include "../utils.h"
#include "../version.h"


// Attach Event
BEGIN_EVENT_TABLE(ConfigBox, wxDialog)
	EVT_CHECKBOX(IDC_CHECK_TEMP_FOLDER, ConfigBox::OnCheckTempFolder)
	EVT_BUTTON(IDC_BUTTON_TEMP_FOLDER, ConfigBox::OnClickTempFolder)
	EVT_BUTTON(IDC_BUTTON_BINARY_EDITOR, ConfigBox::OnClickBinaryEditor)
	EVT_BUTTON(IDC_BUTTON_TEXT_EDITOR, ConfigBox::OnClickTextEditor)
	EVT_BUTTON(wxID_OK, ConfigBox::OnOK)
END_EVENT_TABLE()

ConfigBox::ConfigBox(wxWindow* parent, wxWindowID id, Config *ini)
	: wxDialog(parent, id, _("Configure"), wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX)
{
	this->ini = ini;

	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);
	wxSizerFlags flagsh = wxSizerFlags().Expand().Border(wxLEFT | wxRIGHT, 4);

	wxBoxSizer *szrAll;
	wxBoxSizer *szrPage;
	wxBoxSizer *szrH;
	wxStaticBoxSizer *bszr;
	wxButton *btn;

	wxNotebook *book = new wxNotebook(this, wxID_ANY);
	wxPanel *page;

	//
	// page 1 : 一般
	//
	page = new wxPanel(book);
	book->AddPage(page, _("General"));

	szrPage = new wxBoxSizer(wxVERTICAL);

	// 削除されたファイルをリストに表示するか

	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkShowDelFile = new wxCheckBox(page, IDC_CHECK_SHOW_DELFILE, _("Show deleted and hidden files on the file list."));
	chkShowDelFile->SetValue(ini->IsShownDeletedFile());
	szrH->Add(chkShowDelFile, flags);
	szrPage->Add(szrH, flags);

	// プロパティダイアログに内部ディレクトリ情報を表示する

	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkInterDirItem = new wxCheckBox(page, IDC_CHECK_INTER_DIR_ITEM, _("Show the internal directory information on the property dialog."));
	chkInterDirItem->SetValue(ini->DoesShowInterDirItem());
	szrH->Add(chkInterDirItem, flags);
	szrPage->Add(szrH, flags);

	// 一度に処理できるディレクトリの深さ

	szrH = new wxBoxSizer(wxHORIZONTAL);
	szrH->Add(new wxStaticText(page, wxID_ANY, _("The depth of subdirectories that can be processed per time:")), flags);
	spnDirDepth = new wxSpinCtrl(page, IDC_SPIN_DIR_DEPTH);
	spnDirDepth->SetRange(1, 100);
	spnDirDepth->SetValue(ini->GetDirDepth());
	szrH->Add(spnDirDepth, flags);
	szrPage->Add(szrH, flags);

	// キャッシュサイズ

	bszr = new wxStaticBoxSizer(new wxStaticBox(page, wxID_ANY, _("Data Cache")), wxVERTICAL);
	szrH = new wxBoxSizer(wxHORIZONTAL);
	szrH->Add(new wxStaticText(page, wxID_ANY, _("Limit size")), flags);
	spnCacheLimit = new wxSpinCtrl(page, IDC_SPIN_CACHE_LIMIT);
	spnCacheLimit->SetRange(10, 10000);
	spnCacheLimit->SetValue(ini->GetCacheLimitSize());
	szrH->Add(spnCacheLimit, flags);
	szrH->Add(new wxStaticText(page, wxID_ANY, _("MB")), flags);

	szrH->AddSpacer(16);

	szrH->Add(new wxStaticText(page, wxID_ANY, _("Shrink size")), flags);
	spnCacheShrink = new wxSpinCtrl(page, IDC_SPIN_CACHE_SHRINK);
	spnCacheShrink->SetRange(10, 10000);
	spnCacheShrink->SetValue(ini->GetCacheShrinkSize());
	szrH->Add(spnCacheShrink, flags);
	szrH->Add(new wxStaticText(page, wxID_ANY, _("MB")), flags);

	bszr->Add(szrH, flags);

	szrPage->Add(bszr, flags);

	// 言語

	wxArrayString langs;
	wxTranslations *t = wxTranslations::Get();
	if (t) {
		langs = t->GetAvailableTranslations(_T(APPLICATION_NAME));
	}
	langs.Insert(_("System Dependent"), 0); 
	langs.Insert(_("Unknown"), 1); 

	szrH = new wxBoxSizer(wxHORIZONTAL);
	szrH->Add(new wxStaticText(page, wxID_ANY, wxT("Language")), flags);
	comLanguage = new wxChoice(page, IDC_COMBO_LANGUAGE, wxDefaultPosition, wxDefaultSize, langs);
	int sel = 0;
	if (!ini->GetLanguage().IsEmpty()) {
		sel = langs.Index(ini->GetLanguage());
	}
	if (sel < 0) {
		sel = 1;
	}
	comLanguage->SetSelection(sel);
	szrH->Add(comLanguage, flags);
	szrPage->Add(szrH, flags);

	page->SetSizerAndFit(szrPage);

	//
	// page 2 : エクスポート
	//
	page = new wxPanel(book);
	book->AddPage(page, _("Export"));

	szrPage = new wxBoxSizer(wxVERTICAL);

	// 属性に適した拡張子を付加する

	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkAddExtExport = new wxCheckBox(page, IDC_CHECK_ADD_EXT_EXPORT, _("Add extension suitable for file attribute to filename."));
	chkAddExtExport->SetValue(ini->IsAddExtensionExport());
	szrH->Add(chkAddExtExport, flags);
	szrPage->Add(szrH, flags);

	// エクスポート時に現在日時を設定する

	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkDateExport = new wxCheckBox(page, IDC_CHECK_DATE_EXPORT, _("Set current date and time to exported file."));
	chkDateExport->SetValue(ini->IsSetCurrentDateExport());
	szrH->Add(chkDateExport, flags);
	szrPage->Add(szrH, flags);

	page->SetSizerAndFit(szrPage);

	//
	// page 3 : インポート
	//
	page = new wxPanel(book);
	book->AddPage(page, _("Import"));

	szrPage = new wxBoxSizer(wxVERTICAL);

	// 確認ダイアログを抑制する

	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkSuppImport = new wxCheckBox(page, IDC_CHECK_SUPP_IMPORT, _("Suppress confirmation dialog."));
	chkSuppImport->SetValue(ini->IsSkipImportDialog());
	szrH->Add(chkSuppImport, flags);
	szrPage->Add(szrH, flags);

	// 属性を決定できる時、ファイル名から拡張子をとり除く

	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkDecAttrImport = new wxCheckBox(page, IDC_CHECK_DEC_ATTR_IMPORT, _("Trim extension in filename when decided file attribute by extension."));
	chkDecAttrImport->SetValue(ini->IsDecideAttrImport());
	szrH->Add(chkDecAttrImport, flags);
	szrPage->Add(szrH, flags);

	// インポート時に現在日時を設定する

	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkDateImport = new wxCheckBox(page, IDC_CHECK_DATE_IMPORT, _("Set current date and time to importing file."));
	chkDateImport->SetValue(ini->IsSetCurrentDateImport());
	szrH->Add(chkDateImport, flags);
	szrPage->Add(szrH, flags);

	// インポートやプロパティ変更時に日時を無視する

	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkIgnoreDate = new wxCheckBox(page, IDC_CHECK_IGNORE_DATE, _("Ignore date and time when import or change property. (Supported system only)"));
	chkIgnoreDate->SetValue(ini->DoesIgnoreDateTime());
	szrH->Add(chkIgnoreDate, flags);
	szrPage->Add(szrH, flags);

	page->SetSizerAndFit(szrPage);

	//
	// page 4 : パス
	//
	page = new wxPanel(book);
	book->AddPage(page, _("Path"));

	szrPage = new wxBoxSizer(wxVERTICAL);

	// テンポラリフォルダのパス

	bszr = new wxStaticBoxSizer(new wxStaticBox(page, wxID_ANY, _("Path of the temporary folder")), wxVERTICAL);
	szrH = new wxBoxSizer(wxHORIZONTAL);
	chkTempFolder = new wxCheckBox(page, IDC_CHECK_TEMP_FOLDER, _("Use system setting."));
	szrH->Add(chkTempFolder, flags);
	bszr->Add(szrH, flagsh);

	szrH = new wxBoxSizer(wxHORIZONTAL);
	txtTempFolder = new wxTextCtrl(page, IDC_TEXT_TEMP_FOLDER, wxT(""), wxDefaultPosition, wxSize(320, -1));
	szrH->Add(txtTempFolder, flags);
	btnTempFolder = new wxButton(page, IDC_BUTTON_TEMP_FOLDER, _("Folder..."));
	szrH->Add(btnTempFolder, flags);
	bszr->Add(szrH, flags);
	szrPage->Add(bszr, flags);

	InitializeTempFolder();

	// バイナリエディタのパス

	bszr = new wxStaticBoxSizer(new wxStaticBox(page, wxID_ANY, _("Path of the binary editor")), wxVERTICAL);
	szrH = new wxBoxSizer(wxHORIZONTAL);
	txtBinaryEditor = new wxTextCtrl(page, IDC_TEXT_BINARY_EDITOR, ini->GetBinaryEditor(), wxDefaultPosition, wxSize(320, -1));
	szrH->Add(txtBinaryEditor, flags);
	btn = new wxButton(page, IDC_BUTTON_BINARY_EDITOR, _("File..."));
	szrH->Add(btn, flags);
	bszr->Add(szrH, flags);
	szrPage->Add(bszr, flags);

	// テキストエディタのパス

	bszr = new wxStaticBoxSizer(new wxStaticBox(page, wxID_ANY, _("Path of the text editor")), wxVERTICAL);
	szrH = new wxBoxSizer(wxHORIZONTAL);
	txtTextEditor = new wxTextCtrl(page, IDC_TEXT_TEXT_EDITOR, ini->GetTextEditor(), wxDefaultPosition, wxSize(320, -1));
	szrH->Add(txtTextEditor, flags);
	btn = new wxButton(page, IDC_BUTTON_TEXT_EDITOR, _("File..."));
	szrH->Add(btn, flags);
	bszr->Add(szrH, flags);
	szrPage->Add(bszr, flags);

	page->SetSizerAndFit(szrPage);

	//
	//

	szrAll = new wxBoxSizer(wxVERTICAL);
	szrAll->Add(book, flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK | wxCANCEL);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}

int ConfigBox::ShowModal()
{
	return wxDialog::ShowModal();
}

void ConfigBox::OnOK(wxCommandEvent& event)
{
	if (IsModal()) {
		EndModal(wxID_OK);
	} else {
		SetReturnCode(wxID_OK);
		this->Show(false);
	}
}

void ConfigBox::OnCheckTempFolder(wxCommandEvent& event)
{
	bool chk = event.IsChecked();
	SetEditableTempFolder(!chk);
}

void ConfigBox::InitializeTempFolder()
{
	wxString def_tmp_dir = wxFileName::GetTempDir();
	wxString tmp_dir = ini->GetTemporaryFolder();
	bool def_tmp = tmp_dir.IsEmpty();
	if (def_tmp) {
		tmp_dir = def_tmp_dir;
	}
	txtTempFolder->SetValue(tmp_dir);
	chkTempFolder->SetValue(def_tmp);
	SetEditableTempFolder(!def_tmp);
}

void ConfigBox::SetEditableTempFolder(bool val)
{
	txtTempFolder->SetEditable(val);
	btnTempFolder->Enable(val);
}

void ConfigBox::OnClickTempFolder(wxCommandEvent& event)
{
	UiDiskDirDialog dlg(_("Select the folder for temporary."));
	int sts = dlg.ShowModal();
	if (sts == wxID_OK) {
		// パスを設定
		txtTempFolder->SetValue(dlg.GetPath());
	}
}

void ConfigBox::OnClickBinaryEditor(wxCommandEvent& event)
{
	UiDiskOpenFileDialog dlg(_("Select the application for editing binary data."));
	int sts = dlg.ShowModal();
	if (sts == wxID_OK) {
		// パスを設定
		txtBinaryEditor->SetValue(dlg.GetPath());
	}
}

void ConfigBox::OnClickTextEditor(wxCommandEvent& event)
{
	UiDiskOpenFileDialog dlg(_("Select the application for editing text data."));
	int sts = dlg.ShowModal();
	if (sts == wxID_OK) {
		// パスを設定
		txtTextEditor->SetValue(dlg.GetPath());
	}
}

/// 設定を反映させる
void ConfigBox::CommitData()
{
	ini->ShowDeletedFile(chkShowDelFile->GetValue());
	ini->AddExtensionExport(chkAddExtExport->GetValue());
	ini->SetCurrentDateExport(chkDateExport->GetValue());
	ini->SkipImportDialog(chkSuppImport->GetValue());
	ini->DecideAttrImport(chkDecAttrImport->GetValue());
	ini->SetCurrentDateImport(chkDateImport->GetValue());
	ini->IgnoreDateTime(chkIgnoreDate->GetValue());
	ini->SetDirDepth(spnDirDepth->GetValue());
	if (chkTempFolder->IsChecked()) {
		ini->ClearTemporaryFolder();
	} else {
		ini->SetTemporaryFolder(txtTempFolder->GetValue());
	}
	ini->SetBinaryEditor(txtBinaryEditor->GetValue());
	ini->SetTextEditor(txtTextEditor->GetValue());
	ini->ShowInterDirItem(chkInterDirItem->GetValue());
	int limit_size = spnCacheLimit->GetValue();
	int shrink_size = spnCacheShrink->GetValue();
	ini->CalcCacheSize(limit_size, shrink_size);
	ini->SetCacheLimitSize(limit_size);
	ini->SetCacheShrinkSize(shrink_size);
	int sel = comLanguage->GetSelection();
	wxString lang;
	switch(sel) {
	case 0:
		break;
	case 1:
		lang = wxT("unknown");
		break;
	default:
		lang = comLanguage->GetString(sel);
		break;
	}
	ini->SetLanguage(lang);
}
