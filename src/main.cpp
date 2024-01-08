/// @file main.cpp
///
/// @brief 本体
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "main.h"
#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/toolbar.h>
#include <wx/dir.h>
#include "charcodes.h"
#include "ui/mymenu.h"
#include "ui/diskparambox.h"
#include "basicfmt/basicfmt.h"
#include "basicfmt/basicdiritem.h"
#include "ui/uirpanel.h"
#include "ui/uidisklist.h"
#include "ui/uidiskattr.h"
#include "ui/uifilelist.h"
#include "ui/uirawdisk.h"
#include "ui/uibindump.h"
#include "ui/uifatarea.h"
#include "diskimg/diskparam.h"
#include "diskimg/bootparam.h"
#include "diskimg/fileparam.h"
#include "ui/fileselbox.h"
#include "diskimg/diskwriter.h"
#include "diskimg/diskresult.h"
#include "ui/fontminibox.h"
#include "ui/configbox.h"
#include "diskimg/diskimage.h"
#include "diskimg/diskplain.h"
#include "logging.h"
#include "version.h"
// icon
#include "res/difinder.xpm"
#include "res/fd_5inch_16_1.xpm"
#include "res/fd_5inch_16_2.xpm"
#include "res/hdd_16_new.xpm"
#include "res/hdd_16_open.xpm"
#include "res/fd_5inch_16_add.xpm"
#include "res/fd_5inch_16_delete.xpm"
#include "res/hd_part_16_export.xpm"
#include "res/hd_part_16_import.xpm"
#include "res/foldericon_open.xpm"
#include "res/foldericon_close.xpm"
#include "res/fileicon_normal.xpm"
#include "res/fileicon_delete.xpm"
#include "res/fileicon_hidden.xpm"
#include "res/labelicon_normal.xpm"
#include "res/hdd_16.xpm"
#include "res/hd_part_16.xpm"

#define MYDISK_TRANS \
	_("can't open file '%s'") \
	_("can't create file '%s'") \
	_("can't close file descriptor %d") \
	_("can't read from file descriptor %d") \
	_("can't write to file descriptor %d") \
	_("can't flush file descriptor %d") \
	_("can't seek on file descriptor %d") \
	_("can't get seek position on file descriptor %d")

#define MOD_COUNT_MAX 20

//////////////////////////////////////////////////////////////////////

wxIMPLEMENT_APP(UiDiskApp);

wxBEGIN_EVENT_TABLE(UiDiskApp, wxApp)
#ifdef CAPTURE_MOD_KEY_ON_APP
	EVT_IDLE(UiDiskApp::OnAppIdle)
#endif
wxEND_EVENT_TABLE()

UiDiskApp::UiDiskApp()
{
	frame = NULL;
#ifdef CAPTURE_MOD_KEY_ON_APP
	mod_keys = 0;
	mod_cnt = 0;
#endif
}

/// 初期処理
bool UiDiskApp::OnInit()
{
	SetAppPath();
	SetAppName(_T(APPLICATION_NAME));

	if (!wxApp::OnInit()) {
		return false;
	}

	// log file
	myLog.Open(ini_path, GetAppName(), _T(".log"));

	// load ini file
	gConfig.Load(ini_path + GetAppName() + _T(".ini"));

	// set locale search path and catalog name

	wxString locale_name = gConfig.GetLanguage();
	int lang_num = 0;
	if (locale_name.IsEmpty()) {
		lang_num = wxLocale::GetSystemLanguage();
	} else {
		const wxLanguageInfo * const lang = wxLocale::FindLanguageInfo(locale_name);
		if (lang) {
			lang_num = lang->Language;
		} else {
			lang_num = wxLANGUAGE_UNKNOWN;
		}
	}
	if (mLocale.Init(lang_num, wxLOCALE_LOAD_DEFAULT)) {
		mLocale.AddCatalogLookupPathPrefix(res_path + _T("lang"));
		mLocale.AddCatalogLookupPathPrefix(_T("lang"));
		mLocale.AddCatalog(_T(APPLICATION_NAME));
	}
	if (mLocale.IsLoaded(_T(APPLICATION_NAME))) {
		locale_name = mLocale.GetCanonicalName();
	} else {
		locale_name = wxT("");
	}

	// load xml
	wxString errmsgs;
	if (!gDiskTemplates.Load(res_path + wxT("data/"), locale_name, errmsgs)) {
		errmsgs += wxT("\n");
		errmsgs += _("Cannot load disk types data file.");
		wxMessageBox(errmsgs, _("Error"), wxOK);
		return false;
	}
	if (!gBootTemplates.Load(res_path + wxT("data/"), locale_name, errmsgs)) {
		errmsgs += wxT("\n");
		errmsgs += _("Cannot load boot types data file.");
		wxMessageBox(errmsgs, _("Error"), wxOK);
		return false;
	}
	if (!gDiskBasicTemplates.Load(res_path + wxT("data/"), locale_name, errmsgs)) {
		errmsgs += wxT("\n");
		errmsgs += _("Cannot load disk basic types data file.");
		wxMessageBox(errmsgs, _("Error"), wxOK);
		return false;
	}
	if (!CharCodes::Load(res_path + wxT("data/"), locale_name, errmsgs)) {
		wxMessageBox(_("Cannot load char codes data file."), _("Error"), wxOK);
		return false;
	}
	if (!gFileTypes.Load(res_path + wxT("data/"), locale_name, errmsgs)) {
		wxMessageBox(_("Cannot load file types data file."), _("Error"), wxOK);
		return false;
	}

	int w = gConfig.GetWindowWidth();
	int h = gConfig.GetWindowHeight();
	frame = new UiDiskFrame(GetAppName(), wxSize(w, h));
	frame->Show(true);
	SetTopWindow(frame);

	if (!frame->Init(in_file)) {
		return false;
	}

	return true;
}

#define OPTION_VERBOSE "verbose"

/// コマンドラインの解析
void UiDiskApp::OnInitCmdLine(wxCmdLineParser &parser)
{
	// the standard command line options
	static const wxCmdLineEntryDesc cmdLineDesc[] = {
		{
			wxCMD_LINE_SWITCH, "h", "help",
			"show this help message",
			wxCMD_LINE_VAL_NONE,
			wxCMD_LINE_OPTION_HELP
		},

#if wxUSE_LOG
		{
			wxCMD_LINE_SWITCH, NULL, OPTION_VERBOSE,
			"generate verbose log messages",
			wxCMD_LINE_VAL_NONE,
			0x0
		},
#endif // wxUSE_LOG
	    {
			wxCMD_LINE_PARAM, NULL, NULL,
			"input file",
			wxCMD_LINE_VAL_STRING,
			wxCMD_LINE_PARAM_OPTIONAL
		},

		// terminator
		wxCMD_LINE_DESC_END
	};

	parser.SetDesc(cmdLineDesc);
}

/// コマンドラインの解析完了
bool UiDiskApp::OnCmdLineParsed(wxCmdLineParser &parser)
{
#if wxUSE_LOG
	if ( parser.Found(OPTION_VERBOSE) ) {
		wxLog::SetVerbose(true);
	}
#endif // wxUSE_LOG
	if (parser.GetParamCount() > 0) {
		in_file = parser.GetParam(0);
	}
	return true;
}

/// 終了処理
int UiDiskApp::OnExit()
{
	// save ini file
	gConfig.Save();
	// remove temp directories
	RemoveTempDirs();

	return 0;
}

#ifdef CAPTURE_MOD_KEY_ON_APP
/// アイドル時の処理
void UiDiskApp::OnAppIdle(wxIdleEvent& event)
{
	if (mod_cnt > 0) {
		mod_cnt--;
	}
	// 一定時間、キー入力がなければ修飾キー押下状態をクリア
	if (mod_cnt == 0) {
		mod_keys = 0;
	}
}

/// イベント強制取得
int UiDiskApp::FilterEvent(wxEvent& event)
{
	const wxEventType t = event.GetEventType();
	if (t == wxEVT_KEY_DOWN || t == wxEVT_KEY_UP) {
		mod_keys = ((wxKeyEvent &)event).GetModifiers();
		mod_cnt = MOD_COUNT_MAX;
	}
	// Continue processing the event normally as well.
	return Event_Skip;
}
#endif

/// ファイルを開く(Mac用)
void UiDiskApp::MacOpenFile(const wxString &fileName)
{
	if (frame) {
		if (!frame->CloseDataFile()) return;
		frame->PreOpenDataFile(fileName);
	}
}

/// ファイルを開く(Mac用)
void UiDiskApp::MacOpenFiles(const wxArrayString &fileNames)
{
	if (frame) {
		wxString fileName = fileNames.Item(0);
		if (!frame->CloseDataFile()) return;
		frame->PreOpenDataFile(fileName);
	}
}

/// アプリケーションのパスを設定
void UiDiskApp::SetAppPath()
{
	app_path = wxFileName::FileName(argv[0]).GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
#ifdef __WXOSX__
	if (app_path.Find(_T("MacOS")) >= 0) {
		wxFileName file = wxFileName::FileName(app_path+"../../../");
		file.Normalize();
		ini_path = file.GetPath(wxPATH_GET_SEPARATOR);
		file = wxFileName::FileName(app_path+"../../Contents/Resources/");
		file.Normalize();
		res_path = file.GetPath(wxPATH_GET_SEPARATOR);
	} else
#endif
	{
		ini_path = app_path;
		res_path = app_path;
	}
}

/// アプリケーションのパスを返す
const wxString &UiDiskApp::GetAppPath()
{
	return app_path;
}

/// 設定ファイルのあるパスを返す
const wxString &UiDiskApp::GetIniPath()
{
	return ini_path;
}

/// リソースファイルのあるパスを返す
const wxString &UiDiskApp::GetResPath()
{
	return res_path;
}

/// テンポラリディレクトリを作成する
bool UiDiskApp::MakeTempDir(wxString &tmp_dir_path)
{
	wxFileName file_path = wxFileName(gConfig.GetTemporaryFolder(), wxGetApp().GetAppName());

	tmp_dir_path = wxFileName::CreateTempFileName(file_path.GetFullPath());
	// 上記でファイルができてしまうので削除
	if (wxFileExists(tmp_dir_path)) {
		wxRemoveFile(tmp_dir_path);
	}
	// ディレクトリを作成
	if (!wxDir::Make(tmp_dir_path)) {
		return false;
	}
	// ファイルパスは保存
	tmp_dirs.Add(tmp_dir_path);
	return true;
}

/// テンポラリディレクトリを削除する
void UiDiskApp::RemoveTempDir(const wxString &tmp_dir_path)
{
	RemoveTempDir(tmp_dir_path, 0);

	// リストからも削除
	tmp_dirs.Remove(tmp_dir_path);
}

/// テンポラリディレクトリを削除する
void UiDiskApp::RemoveTempDir(const wxString &tmp_dir_path, int depth)
{
	if (tmp_dir_path.IsEmpty()) return;
	if (depth > 20) return;

	// テンポラリ内のファイル削除
	wxDir dir(tmp_dir_path);
	wxString file_name;
	bool sts = dir.GetFirst(&file_name, wxEmptyString);
	while(sts) {
		wxFileName file_path(tmp_dir_path, file_name);
		wxString full_name = file_path.GetFullPath();
		if (wxFileName::DirExists(full_name)) {
			RemoveTempDir(full_name, depth + 1);
		} else {
			wxRemoveFile(full_name);
		}
		sts = dir.GetNext(&file_name);
	}
	dir.Close();

	// テンポラリディレクトリを削除
	wxDir::Remove(tmp_dir_path);
}

/// テンポラリディレクトリをすべて削除する
void UiDiskApp::RemoveTempDirs()
{
	wxArrayString dirs = tmp_dirs;
	for(size_t n = 0; n < dirs.Count(); n++) {
		RemoveTempDir(dirs.Item(n));
	}
}

//////////////////////////////////////////////////////////////////////
//
// Status Counter
//
StatusCounter::StatusCounter()
{
	m_current = 0;
	m_count = 0;
	m_using = 0;
}
StatusCounter::~StatusCounter()
{
}
void StatusCounter::Clear()
{
	m_current = 0;
	m_count = 0;
	m_using = 0;
	m_message.Empty();
}
void StatusCounter::Start(int count, const wxString &message)
{
	m_current = 0;
	m_count = count;
	m_using = 1;
	m_message = message;
}
void StatusCounter::Append(int count)
{
	m_count += count;
}
void StatusCounter::Increase()
{
	m_current++;
}
void StatusCounter::Finish(const wxString &message)
{
	m_message = message;
	m_using = 3;
}
wxString StatusCounter::GetCurrentMessage() const
{
	wxString str = wxString::Format(wxT("%d/%d "), m_current, m_count);
	str += m_message;
	return str;
}

StatusCounters::StatusCounters()
{
}
StatusCounters::~StatusCounters()
{
}
StatusCounter &StatusCounters::Item(int idx)
{
	if (idx < 0 || idx >= StatusCountersMax) idx = 0;
	return m_sc[idx];
}
void StatusCounters::Clear()
{
	for(int idx = 0; idx < StatusCountersMax; idx++) {
		StatusCounter *sc = &m_sc[idx];
		if (sc->IsFinished()) {
			sc->Clear();
		}
	}
}
int StatusCounters::Start(int count, const wxString &message)
{
	int decide = -1;
	for(int idx = 0; idx < StatusCountersMax; idx++) {
		StatusCounter *sc = &m_sc[idx];
		if (sc->IsIdle()) {
			sc->Start(count, message);
			m_delay.Stop();
			Clear();
			decide = idx;
			break;
		}
	}
	return decide;
}
void StatusCounters::Append(int idx, int count)
{
	if (idx < 0 || idx >= StatusCountersMax) idx = 0;
	m_sc[idx].Append(count);
}
void StatusCounters::Increase(int idx)
{
	if (idx < 0 || idx >= StatusCountersMax) idx = 0;
	m_sc[idx].Increase();
}
void StatusCounters::Finish(int idx, const wxString &message, wxEvtHandler *owner)
{
	if (idx < 0 || idx >= StatusCountersMax) idx = 0;
	m_sc[idx].Finish(message);
	m_delay.SetOwner(owner, IDT_STATUS_COUNTER);
	m_delay.StartOnce(5000);
}
int StatusCounters::Current(int idx) const
{
	if (idx < 0 || idx >= StatusCountersMax) idx = 0;
	return m_sc[idx].Current();
}
int StatusCounters::Count(int idx) const
{
	if (idx < 0 || idx >= StatusCountersMax) idx = 0;
	return m_sc[idx].Count();
}
wxString StatusCounters::GetCurrentMessage(int idx) const
{
	if (idx < 0 || idx >= StatusCountersMax) idx = 0;
	return m_sc[idx].GetCurrentMessage();
}

//////////////////////////////////////////////////////////////////////
//
// Frame
//
static const int IDT_TOOLBAR = 500;
static const long TOOLBAR_STYLE = wxTB_FLAT | wxTB_DOCKABLE | wxTB_TEXT;

// Attach Event
wxBEGIN_EVENT_TABLE(UiDiskFrame, wxFrame)
	// menu event
	EVT_CLOSE(UiDiskFrame::OnClose)

	EVT_MENU(wxID_EXIT,  UiDiskFrame::OnQuit)
	EVT_MENU(wxID_ABOUT, UiDiskFrame::OnAbout)

	EVT_MENU(IDM_OPEN_FILE, UiDiskFrame::OnOpenFile)

	EVT_MENU(IDM_CLOSE_FILE, UiDiskFrame::OnCloseFile)

	EVT_MENU(IDM_SAVE_FILE, UiDiskFrame::OnSaveFile)
//	EVT_MENU(IDM_SAVEAS_FILE, UiDiskFrame::OnSaveAsFile)

	EVT_MENU_RANGE(IDM_RECENT_FILE_0, IDM_RECENT_FILE_0 + MAX_RECENT_FILES - 1, UiDiskFrame::OnOpenRecentFile)

	EVT_MENU(IDM_EXPORT_DATA, UiDiskFrame::OnExportDataFromDisk)
	EVT_MENU(IDM_IMPORT_DATA, UiDiskFrame::OnImportDataToDisk)
	EVT_MENU(IDM_DELETE_DATA, UiDiskFrame::OnDeleteDataFromDisk)
	EVT_MENU(IDM_RENAME_DATA_ON_DISK, UiDiskFrame::OnRenameDataOnDisk)
	EVT_MENU(IDM_COPY_DATA, UiDiskFrame::OnCopyDataFromDisk)
	EVT_MENU(IDM_PASTE_DATA, UiDiskFrame::OnPasteDataToDisk)
	EVT_MENU(IDM_MAKE_DIRECTORY_ON_DISK, UiDiskFrame::OnMakeDirectoryOnDisk)
	EVT_MENU(IDM_PROPERTY_DATA, UiDiskFrame::OnPropertyOnDisk)

	EVT_MENU(IDM_BASIC_MODE, UiDiskFrame::OnBasicMode)
	EVT_MENU(IDM_RAWDISK_MODE, UiDiskFrame::OnRawDiskMode)
	EVT_MENU_RANGE(IDM_CHAR_0, IDM_CHAR_0 + 10, UiDiskFrame::OnChangeCharCode)
	EVT_MENU(IDM_SHOW_DELFILE, UiDiskFrame::OnShowDeletedFile)
	EVT_MENU(IDM_CONFIGURE, UiDiskFrame::OnConfigure)

	EVT_MENU(IDM_WINDOW_BINDUMP, UiDiskFrame::OnOpenBinDump)
	EVT_MENU(IDM_WINDOW_FATAREA, UiDiskFrame::OnOpenFatArea)
	EVT_MENU(IDM_FILELIST_COLUMN, UiDiskFrame::OnChangeColumnsOfFileList)
	EVT_MENU(IDM_CHANGE_FONT, UiDiskFrame::OnChangeFont)

	EVT_TIMER(StatusCounters::IDT_STATUS_COUNTER, UiDiskFrame::OnTimerStatusCounter)
	EVT_TIMER(IDT_STATUS_TIMER, UiDiskFrame::OnTimerStatusTimer)

#ifdef USE_MENU_OPEN
	EVT_MENU_OPEN(UiDiskFrame::OnMenuOpen)
#endif
wxEND_EVENT_TABLE()

// 翻訳用
#define DIALOG_BUTTON_STRING _("OK"),_("Cancel")
#define APPLE_MENU_STRING _("Hide difinder"),_("Hide Others"),_("Show All"),_("Quit difinder"),_("Services"),_("Preferences…"),_("Minimize"),_("Zoom"),_("Bring All to Front")

UiDiskFrame::UiDiskFrame(const wxString& title, const wxSize& size)
#if defined(__WXOSX__)
	: wxFrame(NULL, -1, title, wxDefaultPosition, wxDefaultSize)
#else
	: wxFrame(NULL, -1, title, wxDefaultPosition, size)
#endif
{
#if defined(__WXOSX__)
	SetClientSize(size);
#endif

//	p_image = new DiskD88;
	p_image = new DiskPlain;

	unique_number = 0;

	// icon
#ifdef __WXMSW__
	SetIcon(wxIcon(_T(APPLICATION_NAME)));
#elif defined(__WXGTK__) || defined(__WXMOTIF__)
	SetIcon(wxIcon(APPLICATION_XPMICON_NAME));
#endif

	// menu
	MakeMenu();

	// status bar
	RecreateStatusbar();

	// tool bar
    RecreateToolbar();

	// splitter window
	panel = new UiDiskPanel(this);

	bindump_frame = NULL;
	fatarea_frame = NULL;

	// status timer
	m_status_timer.SetOwner(this, IDT_STATUS_TIMER);
	m_status_timer.Start(1000);
}

UiDiskFrame::~UiDiskFrame()
{
	// フレーム
#if defined(__WXOSX__)
	wxSize sz = GetClientSize();
#else
	wxSize sz = GetSize();
#endif
	gConfig.SetWindowWidth(sz.GetWidth());
	gConfig.SetWindowHeight(sz.GetHeight());

	delete p_image;
}

/// フレーム部の初期処理
bool UiDiskFrame::Init(const wxString &in_file)
{
	bool valid = false;

	if (!in_file.IsEmpty()) {
		valid = PreOpenDataFile(in_file);
	}
	if (!valid) {
		// 起動時にファイルを開いたときはもう更新している
		UpdateMenuFile();
		UpdateMenuDisk();
		UpdateMenuMode();
		UpdateToolBar();
	}
	if (panel) {
		// キャラクターコードマップ番号設定
		SetDefaultCharCode();
	}

	return true;
}

/// ツールバーの再生成
void UiDiskFrame::RecreateToolbar()
{
	// delete and recreate the toolbar
	wxToolBar *toolBar = GetToolBar();
	long style = toolBar ? toolBar->GetWindowStyle() : TOOLBAR_STYLE;
	delete toolBar;

	SetToolBar(NULL);

	style &= ~(wxTB_HORIZONTAL | wxTB_VERTICAL | wxTB_BOTTOM | wxTB_RIGHT | wxTB_HORZ_LAYOUT);
	style |= wxTB_TOP;
	style &= ~wxTB_NO_TOOLTIPS;
	style |= wxTB_HORZ_LAYOUT;
	style |= wxTB_TEXT;

	toolBar = CreateToolBar(style, IDT_TOOLBAR);

	PopulateToolbar(toolBar);
}

#define INIT_TOOL_BMP(bmp) \
	toolBarBitmaps[bmp] = wxBitmap(bmp##_xpm)

/// ツールバーの構築
void UiDiskFrame::PopulateToolbar(wxToolBar* toolBar)
{
	// Set up toolbar
	enum {
		hdd_16_open,
		foldericon_close,
		hdd_16,
		hd_part_16_export,
		hd_part_16_import,
		Tool_Max
	};

	wxBitmap toolBarBitmaps[Tool_Max];

	INIT_TOOL_BMP(hdd_16_open);
	INIT_TOOL_BMP(foldericon_close);
	INIT_TOOL_BMP(hdd_16);
	INIT_TOOL_BMP(hd_part_16_export);
	INIT_TOOL_BMP(hd_part_16_import);

	int w = toolBarBitmaps[hdd_16_open].GetWidth(),
		h = toolBarBitmaps[hdd_16_open].GetHeight();

	toolBar->SetToolBitmapSize(wxSize(w, h));

	toolBar->AddTool(IDM_OPEN_FILE, _("Open"),
		toolBarBitmaps[hdd_16_open], wxNullBitmap, wxITEM_NORMAL,
		_("Open a disk image"));
	toolBar->AddTool(IDM_CLOSE_FILE, _("Close"),
		toolBarBitmaps[foldericon_close], wxNullBitmap, wxITEM_NORMAL,
		_("Close the disk image"));
	toolBar->AddTool(IDM_SAVE_FILE, _("Save"),
		toolBarBitmaps[hdd_16], wxNullBitmap, wxITEM_NORMAL,
		_("Save the disk image"));
	toolBar->AddSeparator();
	toolBar->AddTool(IDM_EXPORT_DATA, _("Export"),
		toolBarBitmaps[hd_part_16_export], wxNullBitmap, wxITEM_NORMAL,
		_("Export a file from the disk"));
	toolBar->AddTool(IDM_IMPORT_DATA, _("Import"),
		toolBarBitmaps[hd_part_16_import], wxNullBitmap, wxITEM_NORMAL,
		_("Import a file to the disk"));

	// after adding the buttons to the toolbar, must call Realize() to reflect
	// the changes
	toolBar->Realize();
	int m_rows = 1;
	toolBar->SetRows(toolBar->IsVertical() ? (int)toolBar->GetToolsCount() / m_rows : m_rows);
}

/// ステータスバーの再生成
void UiDiskFrame::RecreateStatusbar()
{
	// delete and recreate the statusbar
	wxStatusBar *statBar = GetStatusBar();
	delete statBar;
	SetStatusBar(NULL);

	long style = wxSTB_DEFAULT_STYLE;

	statBar = new wxStatusBar(this, wxID_ANY, style);
	statBar->SetFieldsCount(3);

    SetStatusBar(statBar);

	AdjustStatusBarPosition();
}

/// ステータスバーの位置
void UiDiskFrame::AdjustStatusBarPosition()
{
	wxStatusBar *statBar = GetStatusBar();
	if (!statBar) return;

	int cnt = statBar->GetFieldsCount();
	if (cnt < 2) return;

	wxSize sz = GetClientSize();
	int *w = new int[cnt];
	for(int i=0; i<cnt; i++) {
		if (i == 0) {
			w[i] = sz.x / 2;
		} else {
			w[i] = sz.x / 2 / (cnt - 1);
		}
	}

	SetStatusWidths(cnt, w);

	delete [] w;
}

#ifdef USE_MENU_OPEN
/// メニュー更新
void UiDiskFrame::OnMenuOpen(wxMenuEvent& event)
{
	wxMenu *menu = event.GetMenu();

	if (menu == NULL) return;

	if (menu == menuFile) {	// File...
		UpdateMenuFile();
	} else if (menu == menuDisk) { // Disk...
		UpdateMenuDisk();
	} else if (menu == menuMode) { // Mode...
		UpdateMenuMode();
	}
}
#endif

/// ドロップされたファイルを開く
void UiDiskFrame::OpenDroppedFile(const wxString &path)
{
 	if (!CloseDataFile()) return;
	PreOpenDataFile(path);
}

////////////////////////////////////////
//
// イベントプロシージャ
//

/// ウィンドウを閉じたとき
void UiDiskFrame::OnClose(wxCloseEvent& event)
{
	if (!CloseDataFile(!event.CanVeto())) {
		event.Veto();
		return;
	}
	event.Skip();
}

/// メニュー 終了選択
void UiDiskFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(false);
}

/// メニュー Aboutダイアログ表示選択
void UiDiskFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
	UiDiskAbout(this, wxID_ANY).ShowModal();
}

/// メニュー 開く選択
void UiDiskFrame::OnOpenFile(wxCommandEvent& WXUNUSED(event))
{
	ShowOpenFileDialog();
}

/// メニュー 最近使用したファイル開く選択
void UiDiskFrame::OnOpenRecentFile(wxCommandEvent& event)
{
	wxMenuItem *item = menuRecentFiles->FindItem(event.GetId());
	if (!item) return;
	wxFileName path = item->GetItemLabel();
	if (!CloseDataFile()) return;
	PreOpenDataFile(path.GetFullPath());
}

/// メニュー 閉じる選択
void UiDiskFrame::OnCloseFile(wxCommandEvent& WXUNUSED(event))
{
	CloseDataFile();
}

/// メニュー 保存選択
void UiDiskFrame::OnSaveFile(wxCommandEvent& WXUNUSED(event))
{
	SaveDataFile();
}
/// メニュー 名前を付けて保存選択
void UiDiskFrame::OnSaveAsFile(wxCommandEvent& WXUNUSED(event))
{
	ShowSaveAsFileDialog();
}

/// メニュー エクスポート選択
void UiDiskFrame::OnExportDataFromDisk(wxCommandEvent& WXUNUSED(event))
{
	ExportDataFromDisk();
}
/// メニュー インポート選択
void UiDiskFrame::OnImportDataToDisk(wxCommandEvent& WXUNUSED(event))
{
	ImportDataToDisk();
}
/// メニュー 削除選択
void UiDiskFrame::OnDeleteDataFromDisk(wxCommandEvent& WXUNUSED(event))
{
	DeleteDataFromDisk();
}
/// メニュー リネーム選択
void UiDiskFrame::OnRenameDataOnDisk(wxCommandEvent& WXUNUSED(event))
{
	RenameDataOnDisk();
}
/// メニュー コピー選択
void UiDiskFrame::OnCopyDataFromDisk(wxCommandEvent& WXUNUSED(event))
{
	CopyDataFromDisk();
}
/// メニュー ペースト選択
void UiDiskFrame::OnPasteDataToDisk(wxCommandEvent& WXUNUSED(event))
{
	PasteDataToDisk();
}
/// メニュー ディレクトリ作成選択
void UiDiskFrame::OnMakeDirectoryOnDisk(wxCommandEvent& WXUNUSED(event))
{
	MakeDirectoryOnDisk();
}
/// メニュー プロパティ選択
void UiDiskFrame::OnPropertyOnDisk(wxCommandEvent& WXUNUSED(event))
{
	PropertyOnDisk();
}

/// メニュー 設定ダイアログ選択
void UiDiskFrame::OnConfigure(wxCommandEvent& WXUNUSED(event))
{
	ShowConfigureDialog();
}

/// ファイルモード選択
void UiDiskFrame::OnBasicMode(wxCommandEvent& event)
{
	ChangeRPanel(0, NULL);
}

/// Rawディスクモード選択
void UiDiskFrame::OnRawDiskMode(wxCommandEvent& event)
{
	ChangeRPanel(1, NULL);
}

/// キャラクターコード選択
void UiDiskFrame::OnChangeCharCode(wxCommandEvent& event)
{
	int sel = event.GetId() - IDM_CHAR_0;
	wxString name = gCharCodeChoices.GetItemName(wxT("main"), (size_t)sel);
	ChangeCharCode(name);
}

/// 削除ファイルを表示するか
void UiDiskFrame::OnShowDeletedFile(wxCommandEvent& event)
{
	gConfig.ShowDeletedFile(event.IsChecked());
	// ファイルリストを更新
	SetFileListData();
}

/// ダンプウィンドウ選択
void UiDiskFrame::OnOpenBinDump(wxCommandEvent& event)
{
	if (!bindump_frame) {
		// ウィンドウを開く
		OpenBinDumpWindow();
	} else {
		// ウィンドウを閉じる
		CloseBinDumpWindow();
	}
}

/// 使用状況ウィンドウ選択
void UiDiskFrame::OnOpenFatArea(wxCommandEvent& event)
{
	if (!fatarea_frame) {
		// ウィンドウを開く
		OpenFatAreaWindow();
	} else {
		// ウィンドウを閉じる
		CloseFatAreaWindow();
	}
}

/// ファイルリストの列選択
void UiDiskFrame::OnChangeColumnsOfFileList(wxCommandEvent& event)
{
	ChangeColumnsOfFileList();
}

/// フォント変更選択
void UiDiskFrame::OnChangeFont(wxCommandEvent& event)
{
	ShowListFontDialog();
}

/// ステータスカウンター終了タイマー
void UiDiskFrame::OnTimerStatusCounter(wxTimerEvent& event)
{
	ClearStatusCounter();
}

/// ステータスメッセージ更新タイマー
void UiDiskFrame::OnTimerStatusTimer(wxTimerEvent& event)
{
	if (!p_image) return;

	wxString str;
	p_image->GetStatusMessage(str);
	SetStatusText(str, 1);
}

////////////////////////////////////////
//
// ウィンドウ操作
//

/// メニューの作成
void UiDiskFrame::MakeMenu()
{
	menuFile = new MyMenu;
	menuData = new MyMenu;
	menuMode = new MyMenu;
	menuView = new MyMenu;
	menuHelp = new MyMenu;
	MyMenu *sm;

	// file menu
	menuFile->Append( IDM_OPEN_FILE, _("&Open...\tCTRL+O") );
	menuFile->AppendSeparator();
	menuFile->Append( IDM_CLOSE_FILE, _("&Close") );
	menuFile->AppendSeparator();
	menuFile->Append( IDM_SAVE_FILE, _("&Save\tCTRL+S") );
//	menuFile->Append( IDM_SAVEAS_FILE, _("Save &As...") );
	menuFile->AppendSeparator();
		menuRecentFiles = new MyMenu();
		UpdateMenuRecentFiles();
	menuFile->AppendSubMenu(menuRecentFiles, _("&Reccent Files") );
	menuFile->AppendSeparator();
	menuFile->Append( wxID_EXIT, _("E&xit\tALT+F4") );
	// data menu
	menuData->Append( IDM_EXPORT_DATA, _("&Export...") );
	menuData->Append( IDM_IMPORT_DATA, _("&Import...") );
	menuData->AppendSeparator();
	menuData->Append( IDM_DELETE_DATA, _("&Delete...") );
	menuData->Append( IDM_RENAME_DATA_ON_DISK, _("Rena&me File") );
	menuData->AppendSeparator();
	menuData->Append(IDM_COPY_DATA, _("&Copy"));
	menuData->Append(IDM_PASTE_DATA, _("&Paste..."));
	menuData->AppendSeparator();
	menuData->Append( IDM_MAKE_DIRECTORY_ON_DISK, _("Make Directory(&F)...") );
	menuData->AppendSeparator();
	menuData->Append( IDM_PROPERTY_DATA, _("&Property") );
	// mode menu
	menuMode->AppendRadioItem( IDM_BASIC_MODE, _("BASIC Mode") );
	menuMode->AppendRadioItem( IDM_RAWDISK_MODE, _("Raw Disk Mode") );
	menuMode->AppendSeparator();
		sm = new MyMenu();
		const CharCodeChoice *choice = gCharCodeChoices.Find(wxT("main"));
		if (choice) {
			for(size_t i=0; i<choice->Count(); i++) {
				const CharCodeMap *map = choice->Item(i);
				sm->AppendRadioItem( IDM_CHAR_0 + (int)i, map->GetDescription() );
			}
		}
	menuMode->AppendSubMenu(sm, _("&Charactor Code") );
	menuMode->AppendSeparator();
		sm = new MyMenu();
		sm->AppendCheckItem( IDM_SHOW_DELFILE, _("Show deleted and hidden files on the file list.") );
	menuMode->AppendSubMenu(sm, _("Quick &Settings") );
	menuMode->AppendSeparator();
	menuMode->Append( IDM_CONFIGURE, _("C&onfigure...") );
	// view menu
	menuView->AppendCheckItem( IDM_WINDOW_BINDUMP, _("&Dump Window") );
	menuView->AppendCheckItem( IDM_WINDOW_FATAREA, _("&Availability Window") );
	menuView->AppendSeparator();
	menuView->Append( IDM_FILELIST_COLUMN, _("Columns of File &List...") );
	menuView->AppendSeparator();
	menuView->Append( IDM_CHANGE_FONT, _("&Font...") );
#if defined(__WXOSX__) && wxCHECK_VERSION(3,1,2)
	// view system menu on mac os x
	menuView->AppendSeparator();
#endif
	// help menu
	menuHelp->Append( wxID_ABOUT, _("&About...") );

	// menu bar
	MyMenuBar *menuBar = new MyMenuBar;
	menuBar->Append( menuFile, _("&File") );
	menuBar->Append( menuData, _("&Data") );
	menuBar->Append( menuMode, _("&Mode") );
	menuBar->Append( menuView, _("&View") );
#if defined(__WXOSX__) && wxCHECK_VERSION(3,1,2)
	// window system menu on mac os x
	menuBar->Append( new wxMenu(), _("&Window") );
#endif
	menuBar->Append( menuHelp, _("&Help") );

	SetMenuBar( menuBar );
}

/// ファイルメニューの更新
void UiDiskFrame::UpdateMenuFile()
{
	bool opened = p_image->IsOpened();
	menuFile->Enable(IDM_CLOSE_FILE, opened);
	menuFile->Enable(IDM_SAVE_FILE, opened);

	UiDiskList *list = GetDiskListPanel();
	if (list) {
		UpdateMenuDiskList(list);
	}
}

/// ディスクメニューの更新
void UiDiskFrame::UpdateMenuDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		UpdateMenuFileList(list);
		return;
	}

	UiDiskRawPanel *rawpl = GetDiskRawPanel();
	if (rawpl) {
		UpdateMenuRawDisk(rawpl);
		return;
	}
}

/// モードメニューの更新
void UiDiskFrame::UpdateMenuMode()
{
	int sel = gCharCodeChoices.IndexOf(wxT("main"), gConfig.GetCharCode());
	wxMenuItem *mitem = menuMode->FindItem(IDM_CHAR_0 + sel);
	if (mitem) mitem->Check(true);
	menuMode->Check(IDM_SHOW_DELFILE, gConfig.IsShownDeletedFile());
}

/// 最近使用したファイル一覧を更新
void UiDiskFrame::UpdateMenuRecentFiles()
{
	// メニューを更新
	wxArrayString names = gConfig.GetRecentFiles();
	for(int i=0; i<MAX_RECENT_FILES && i<(int)names.Count(); i++) {
		if (menuRecentFiles->FindItem(IDM_RECENT_FILE_0 + i)) menuRecentFiles->Delete(IDM_RECENT_FILE_0 + i);
		menuRecentFiles->Append(IDM_RECENT_FILE_0 + i, names[i]);
	}
}

/// ツールバーを更新
void UiDiskFrame::UpdateToolBar()
{
	wxToolBar *toolBar = GetToolBar();
	if (!toolBar) return;

	bool opened = p_image->IsOpened();

	opened = (opened && p_image->CountDisks() > 0);

	UiDiskList *dlist = GetDiskListPanel();
	if (dlist) {
		UpdateToolBarDiskList(dlist);
	}

	UiDiskFileList *flist = GetFileListPanel();
	if (flist) {
		UpdateToolBarFileList(flist);
		return;
	}

	UiDiskRawPanel *rawpl = GetDiskRawPanel();
	if (rawpl) {
		UpdateToolBarRawDisk(rawpl);
		return;
	}
}

/// メニューのディスク項目を更新
void UiDiskFrame::UpdateMenuDiskList(UiDiskList *list)
{
}

/// ツールバーのディスク項目を更新
void UiDiskFrame::UpdateToolBarDiskList(UiDiskList *list)
{
}

/// メニューとツールバーのディスク項目を更新
void UiDiskFrame::UpdateMenuAndToolBarDiskList(UiDiskList *list)
{
	UpdateMenuDiskList(list);
	UpdateToolBarDiskList(list);
}

/// メニューのファイルリスト項目を更新
void UiDiskFrame::UpdateMenuFileList(UiDiskFileList *list)
{
	UiDiskList *lpanel = GetLPanel();
	menuData->Enable(IDM_PROPERTY_DATA, (lpanel->IsSelectedDiskImage()));

	bool opened = (list != NULL && list->CanUseBasicDisk());

	menuData->Enable(IDM_MAKE_DIRECTORY_ON_DISK, opened && list->CanMakeDirectory());

	opened = (opened && list->IsAssignedBasicDisk());
	menuData->Enable(IDM_IMPORT_DATA, opened);
	menuData->Enable(IDM_PASTE_DATA, opened);

	int	cnt = list->GetListSelectedItemCount();
	opened = (opened && cnt > 0);
	menuData->Enable(IDM_EXPORT_DATA, opened);
	menuData->Enable(IDM_DELETE_DATA, opened);
	menuData->Enable(IDM_COPY_DATA, opened);
	opened = (opened && cnt == 1);
	menuData->Enable(IDM_RENAME_DATA_ON_DISK, opened);
}

/// ツールバーのファイルリスト項目を更新
void UiDiskFrame::UpdateToolBarFileList(UiDiskFileList *list)
{
	wxToolBar *toolBar = GetToolBar();
	if (!toolBar) return;

	bool opened = p_image->IsOpened();
	toolBar->EnableTool(IDM_CLOSE_FILE, opened);
	toolBar->EnableTool(IDM_SAVE_FILE, opened);

	opened = (list && list->IsFormattedBasicDisk());
	toolBar->EnableTool(IDM_IMPORT_DATA, opened);

	int	cnt = list->GetListSelectedItemCount();
	opened = (opened && cnt > 0);
	toolBar->EnableTool(IDM_EXPORT_DATA, opened);
	toolBar->EnableTool(IDM_DELETE_DATA, opened);
}

/// メニューとツールバーのファイルリスト項目を更新
void UiDiskFrame::UpdateMenuAndToolBarFileList(UiDiskFileList *list)
{
	UpdateMenuFileList(list);
	UpdateToolBarFileList(list);
}

/// メニューの生ディスク項目を更新
void UiDiskFrame::UpdateMenuRawDisk(UiDiskRawPanel *rawpanel)
{
	UiDiskList *lpanel = GetLPanel();
	menuData->Enable(IDM_PROPERTY_DATA, (lpanel->IsSelectedDiskImage()));

	bool opened = (rawpanel != NULL);
	opened = (opened && rawpanel->TrackListExists());
	menuData->Enable(IDM_EXPORT_DATA, opened);
	menuData->Enable(IDM_IMPORT_DATA, opened);
	menuData->Enable(IDM_COPY_DATA, opened);
	menuData->Enable(IDM_PASTE_DATA, opened);
	menuData->Enable(IDM_DELETE_DATA, opened);

	menuData->Enable(IDM_RENAME_DATA_ON_DISK, false);
	menuData->Enable(IDM_MAKE_DIRECTORY_ON_DISK, false);
}

/// ツールバーの生ディスク項目を更新
void UiDiskFrame::UpdateToolBarRawDisk(UiDiskRawPanel *rawpanel)
{
	wxToolBar *toolBar = GetToolBar();
	if (!toolBar) return;

	bool opened = p_image->IsOpened();
	toolBar->EnableTool(IDM_CLOSE_FILE, opened);
	toolBar->EnableTool(IDM_SAVE_FILE, opened);

	opened = (rawpanel != NULL);
	opened = (opened && rawpanel->TrackListExists());
	toolBar->EnableTool(IDM_EXPORT_DATA, opened);
	toolBar->EnableTool(IDM_IMPORT_DATA, opened);
}

/// メニューとツールバーの生ディスク項目を更新
void UiDiskFrame::UpdateMenuAndToolBarRawDisk(UiDiskRawPanel *rawpanel)
{
	UpdateMenuRawDisk(rawpanel);
	UpdateToolBarRawDisk(rawpanel);
}

/// ウィンドウ上のデータを更新 タイトルバーにファイルパスを表示
void UiDiskFrame::UpdateDataOnWindow(const wxString &path, bool keep)
{
	// update window
	if (!path.IsEmpty()) {
		wxString title = wxGetApp().GetAppName() + wxT(" - ") + path;
		SetTitle(title);
	}
	UpdateDataOnWindow(keep);
}

/// ウィンドウ上のデータを更新
void UiDiskFrame::UpdateDataOnWindow(bool keep)
{
	int dl_num = -1;
	int dl_sid = -1;
	if (keep) {
		// get current position
		GetDiskListSelectedPos(dl_num, dl_sid);
	}

	// update panel
	ClearRPanelData();
	SetDiskListData(GetFileName());

	if (keep) {
		// set position
		SetDiskListPos(dl_num, dl_sid);
	}

#ifndef USE_MENU_OPEN
	// update menu
	UpdateMenuFile();
	UpdateMenuMode();
#endif
}

/// 保存後のウィンドウ上のデータを更新
void UiDiskFrame::UpdateSavedDataOnWindow()
{
	// Rawパネルを更新
	RefreshRawPanelData();
}

/// 保存後のウィンドウ上のデータを更新
void UiDiskFrame::UpdateSavedDataOnWindow(const wxString &path)
{
	// 左パネルのパスを更新
	UpdateFilePathOnWindow(path);
	// Rawパネルを更新
	RefreshRawPanelData();
}

/// ウィンドウ上のファイルパスを更新
void UiDiskFrame::UpdateFilePathOnWindow(const wxString &path)
{
	if (!path.IsEmpty()) {
		wxString title = wxGetApp().GetAppName();
		title += wxT(" - ");
		title += path;
		SetTitle(title);
	}

	SetDiskListFilePath(GetFileName());
}

/// キャラクターコード選択
void UiDiskFrame::ChangeCharCode(const wxString &name)
{
	if (GetCharCode() == name) return;

	p_image->SetCharCode(name);

	UiDiskFileList *listpanel = GetFileListPanel(true);
	if (listpanel) listpanel->ChangeCharCode(name);
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) lpanel->ChangeCharCode(name);

	gConfig.SetCharCode(name);

	UpdateMenuMode();
}

/// キャラクターコード番号を返す
const wxString &UiDiskFrame::GetCharCode() const
{
	return gConfig.GetCharCode();
}

/// キャラクターコード番号設定
void UiDiskFrame::SetDefaultCharCode()
{
	wxString name = gConfig.GetCharCode();

	UiDiskFileList *listpanel = GetFileListPanel(true);
	if (listpanel) listpanel->ChangeCharCode(name);
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) lpanel->ChangeCharCode(name);
}

/// フォント変更ダイアログ
void UiDiskFrame::ShowListFontDialog()
{
	UiDiskList *rlist = GetDiskListPanel();
	if (!rlist) return;

	wxFont font = rlist->GetFont();

	FontMiniBox dlg(this, wxID_ANY, GetFont());
	wxString name = gConfig.GetListFontName();
	if (name.IsEmpty()) name = font.GetFaceName();
	int size = gConfig.GetListFontSize();
	if (size == 0) size = font.GetPointSize();
	dlg.SetFontName(name);
	dlg.SetFontSize(size);
	int sts = dlg.ShowModal();
	if (sts == wxID_OK) {
		wxFont new_font(dlg.GetFontSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, dlg.GetFontName());

		SetListFont(new_font);

		gConfig.SetListFontName(dlg.GetFontName());
		gConfig.SetListFontSize(dlg.GetFontSize());
	}
}

/// リストウィンドウのフォント変更
void UiDiskFrame::SetListFont(const wxFont &font)
{
	UiDiskList *rlist = GetDiskListPanel();
	if (rlist) rlist->SetListFont(font);

	UiDiskRPanel *llist = GetRPanel();
	if (llist) llist->SetListFont(font);
}

/// リストウィンドウのフォント設定
void UiDiskFrame::GetDefaultListFont(wxFont &font)
{
	wxFont def_font = GetFont();

	wxString name = gConfig.GetListFontName();
	if (name.IsEmpty()) name = def_font.GetFaceName();
	int size = gConfig.GetListFontSize();
	if (size == 0) size = def_font.GetPointSize();

	font = wxFont(size, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, name);
}

/// ファイルリストの列を変更
void UiDiskFrame::ChangeColumnsOfFileList()
{
	UiDiskFileList *llist = GetFileListPanel(true);
	if (llist) llist->ShowListColumnDialog();
}

/// 選択しているModeメニュー BASICかRAW DISKか
/// @retval 0 BASIC
/// @retval 1 RAW DISK
int UiDiskFrame::GetSelectedMode()
{
	if (!menuMode) return 0;

	if (menuMode->FindItem(IDM_BASIC_MODE)->IsChecked()) {
		return 0;
	} else if (menuMode->FindItem(IDM_RAWDISK_MODE)->IsChecked()) {
		return 1;
	}
	return 0;
}

/// 全パネルにデータをセットする（イメージ選択時）
void UiDiskFrame::SetDataOnFile(DiskImageFile *file)
{
	SetDiskAttrData(file);
	SetRawPanelData(file);
	ClearBinDumpData();
	UpdateMenuAndToolBarDiskList(GetLPanel());
}

/// 全パネルにデータをセットする（ディスク選択時）
///
/// ディスク選択orツリー展開で、ルートディレクトリをアサインする。
/// refresh_listをtrueにすればファイルリストを更新する
/// @param [in] disk         ディスク
/// @param [in] side_number  AB面ありの時、サイド番号
/// @param [in] refresh_list 右パネルのディスクを選択した時、左パネルのファイルリストを更新
void UiDiskFrame::SetDataOnDisk(DiskImageDisk *disk, int side_number, bool refresh_list)
{
	AttachDiskBasicOnFileList(disk, side_number);
	if (refresh_list) {
		ClearFatAreaData();
		SetFileListData();
	}
	SetRawPanelData(disk);
	ClearBinDumpData();
	UpdateMenuAndToolBarDiskList(GetLPanel());
}

/// 全パネルのデータをクリアする
void UiDiskFrame::ClearAllData()
{
	DetachDiskBasicOnFileList();
	ClearDiskAttrData();
	ClearFileListData();
	ClearRawPanelData();
	ClearBinDumpData();
	ClearFatAreaData();
	UpdateMenuAndToolBarDiskList(GetLPanel());
}

/// 全パネルのデータをクリアしてRAW DISKパネルだけデータをセット
void UiDiskFrame::ClearAllAndSetRawData(DiskImageDisk *disk, int side_number)
{
	ClearFileListData();
	ClearBinDumpData();
	ClearFatAreaData();
	UpdateMenuAndToolBarDiskList(GetLPanel());
}

/// タイトル名を設定
wxString UiDiskFrame::MakeTitleName(const wxString &path)
{
	wxString title;
	if (path.IsEmpty()) {
		title = _("(new file)");
	} else {
		title = path;
	}
	return title;
}

/// タイトル名を返す
wxString UiDiskFrame::GetFileName()
{
	return MakeTitleName(p_image->GetName());
}

////////////////////////////////////////
//
// ディスク操作
//

/// オープンダイアログ
void UiDiskFrame::ShowOpenFileDialog()
{
	if (!CloseDataFile()) return;

	UiDiskFileDialog dlg(
		_("Open File"),
		GetIniRecentPath(),
		wxEmptyString,
		gFileTypes.GetWildcardForLoad(),
		wxFD_OPEN);

	int rc = dlg.ShowModal();
	wxString path = dlg.GetPath();

	if (rc == wxID_OK) {
		PreOpenDataFile(path);
	}
}
/// 拡張子でファイル種別を判別する オープン時
///
/// 拡張子からファイル種別を判別し、必要なら選択ダイアログを表示する。
/// 判別できた場合やダイアログでOKを選択したらファイルを実際に開く。
/// 
/// @param[in] path ファイルパス
bool UiDiskFrame::PreOpenDataFile(const wxString &path)
{
	wxFileName file_path(path);
	wxString   file_format;
	DiskParam  param_hint;

	int rc = CheckOpeningDataFile(path, file_path.GetExt(), file_format, param_hint);
	if (rc < 0) {
		// エラー終了
		return false;
	}
	if (rc == 0) {
		// 開く
		return OpenDataFile(path, file_format, param_hint);
	} else {
		return true;
	}
}
/// 指定したディスクイメージをチェック
///
/// 拡張子からファイル種別を判別し、必要なら選択ダイアログを表示する。
///
/// @param [in]     path        ファイルパス
/// @param [in]     ext         拡張子
/// @param [in,out] file_format ファイルの形式名("d88","plain"など)
/// @param [out]    param_hint  ディスクパラメータヒント(plain時のみ)
/// @retval  0     候補あり正常
/// @retval -1     エラー終了
/// @retval -32767 キャンセルで終了
int UiDiskFrame::CheckOpeningDataFile(const wxString &path, const wxString &ext, wxString &file_format, DiskParam &param_hint)
{
	DiskParamPtrs	n_disk_params;	// パラメータ候補
	DiskParam		n_manual_param;	// 手動設定時のパラメータ
	int rc = 1;

	FileParam *fitem = gFileTypes.FindExt(ext);
	if (!fitem) {
		// 不明の拡張子なのでファイル種類を選択してもらう
		rc = ShowFileSelectDialog(path, file_format) ? 1 : 0;
	}
	if (rc == 1) {
		// 既知の拡張子ならファイルをチェックする
		rc = p_image->Check(path, file_format, n_disk_params, n_manual_param);
		int count = (int)n_disk_params.Count(); 
		if (count > 1 || rc == 1) {
			// 選択ダイアログを表示
			if (ShowParamSelectDialog(path, n_disk_params, count == 0 ? &n_manual_param : NULL, param_hint)) {
				// 選択した
				rc = 0;
			} else {
				// キャンセル
				rc = -32767;
			}
		} else if (count == 1) {
			// 候補1つ
			param_hint = *n_disk_params.Item(0);
			rc = 0;
		}
	}
	// エラーメッセージ表示
	if (rc < 0 && rc != -32767) {
		p_image->ShowErrorMessage();
	}
	return rc;
}
/// 指定したディスクイメージを開く
/// @param [in] path        ファイルパス
/// @param [in] file_format ファイルの形式名("d88","plain"など)
/// @param [in] param_hint  ディスクパラメータヒント(plain時のみ)
bool UiDiskFrame::OpenDataFile(const wxString &path, const wxString &file_format, const DiskParam &param_hint)
{
	bool valid = false;

	// set recent file path
	SetIniRecentPath(path);

	// open disk
	int rc = p_image->Open(path, file_format, param_hint);
	if (rc >= 0) {
		//
		wxFileName fn(path);
		myLog.SetInfo("Opened the disk image: %s", fn.GetFullName().t_str());
		// update window
		UpdateDataOnWindow(path, false);
		valid = true;
	}

	if (rc != 0) {
		// message
		p_image->ShowErrorMessage();
	}
	unique_number++;

	UpdateToolBar();

	return valid;
}

/// ファイル種類選択ダイアログ
/// @param [in]  path        ファイルパス
/// @param [out] file_format 選択したファイルの形式名("d88","plain"など)
/// @return true OKボタンを押した
bool UiDiskFrame::ShowFileSelectDialog(const wxString &path, wxString &file_format)
{
	FileSelBox dlg(this, wxID_ANY);
	int sts = dlg.ShowModal();
	if (sts == wxID_OK) {
		wxString name = dlg.GetFormatType();
		file_format = dlg.GetFormatType();
	}
	return (sts == wxID_OK);
}

/// ディスク種類選択ダイアログ
/// @param [in] path         ファイルパス
/// @param [in] disk_params  ディスクパラメータの候補
/// @param [in] manual_param 手動設定の初期パラメータ
/// @param [out] param_hint  選択したディスクパラメータ
/// @return true OKボタンを押した
bool UiDiskFrame::ShowParamSelectDialog(const wxString &path, const DiskParamPtrs &disk_params, const DiskParam *manual_param, DiskParam &param_hint)
{
	// パラメータを選択
	DiskParamBox dlg(this, wxID_ANY, DiskParamBox::SELECT_DISK_TYPE, 0, NULL, &disk_params, manual_param, DiskParamBox::SHOW_TEMPLATE);
	int sts = dlg.ShowModal();
	if (sts != wxID_OK) {
		return false;
	}
	dlg.GetParam(param_hint);
	return true;
}

/// ファイルを閉じる
/// @param [in] force 強制 (確認をしない)
/// @return false:キャンセルした
bool UiDiskFrame::CloseDataFile(bool force)
{
	if (!p_image->IsOpened()) return true;

	// 変更されているか
	if (!force && p_image->IsModified()) {
		int ans = wxMessageBox(_("This file is modified. Do you want to save it?"), _("Close File"), wxYES_NO | wxCANCEL | wxICON_INFORMATION);  
		if (ans == wxCANCEL) {
			return false;
		}
		if (ans == wxYES) {
			SaveDataFile();
		}
	}

	unique_number++;
	
	// プロパティウィンドウを閉じる
	CloseAllFileAttr();

	// update window
	wxString title = wxGetApp().GetAppName();
	SetTitle(title);

	// update panel
	ClearRPanelData();
	ClearDiskListData();

	//
	p_image->Close();

#ifndef USE_MENU_OPEN
	// update menu
	UpdateMenuFile();
	UpdateMenuMode();
#endif
	UpdateToolBar();

	return true;
}

/// 名前を付けて保存ダイアログ
void UiDiskFrame::ShowSaveAsFileDialog()
{
	if (!p_image->IsOpened()) return;

#if 0
	UiDiskFileDialog dlg(
		_("Save File"),
		p_image->GetPath(),
		p_image->GetFileName(),
		gFileTypes.GetWildcardForSave(),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	int rc = dlg.ShowModal();
	wxString path = dlg.GetPath();
	FileFormat *fmt = gFileTypes.GetFilterForSave(dlg.GetFilterIndex());

	if (rc == wxID_OK) {
		SaveDataFile(path, fmt ? fmt->GetName() : wxT(""));
	}
#endif
}

/// 上書き保存
void UiDiskFrame::SaveDataFile()
{
	if (!p_image->IsOpened()) return;

	int rc;

	// 保存できるか
	rc = p_image->CanSave(); 
	if (rc != 0) {
		// 書き込み禁止などのエラー
		rc = p_image->ShowErrWarnMessage();
		if (rc < 0) return;
	}

	// 上書き保存
	rc = p_image->Save(
		DiskWriteOptions(
			false
		)
	);
	if (rc >= 0) {
		// update window
		UpdateSavedDataOnWindow();
	}
	if (rc != 0) {
		// message
		p_image->ShowErrorMessage();
	}
}

/// 指定したファイルに保存
/// @param [in] path        ファイルパス
/// @param [in] file_format 保存フォーマット形式
void UiDiskFrame::SaveDataFile(const wxString &path, const wxString &file_format)
{
	// set recent file path
	SetIniRecentPath(path);

	int rc;

	// validate disk
	rc = p_image->CanSave(path, file_format);
	if (rc != 0) {
		rc = p_image->ShowErrWarnMessage();
		if (rc < 0) return;
	}

	// save disk
	rc = p_image->Save(path, file_format,
		DiskWriteOptions(
			false
		)
	);
	if (rc >= 0) {
		// update window
		UpdateSavedDataOnWindow(path);
	}
	if (rc != 0) {
		// message
		p_image->ShowErrorMessage();
	}
}

/// ディスクパラメータを表示/変更
void UiDiskFrame::ShowFileAttr()
{
	DiskImageFile *file = p_image->GetFile();
	DiskParamBox dlg(this, wxID_ANY, DiskParamBox::SHOW_DISK_PARAM, -1, file, NULL, NULL, DiskParamBox::SHOW_DISKLABEL_ALL);
	int sts = dlg.ShowModal();
	if (sts == wxID_OK) {
		DiskParam param;
		dlg.GetParam(param);
	}
}
/// ディスクパラメータを表示/変更
void UiDiskFrame::ShowDiskAttr()
{
	UiDiskList *list = GetLPanel();
	if (list) {
		list->ShowDiskAttr();
		return;
	}
}
/// ディスクからファイルをエクスポート
void UiDiskFrame::ExportDataFromDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->ShowExportDataFileDialog();
		return;
	}
	UiDiskRawPanel *panel = GetDiskRawPanel();
	if (panel) {
		panel->ShowExportDataDialog();
		return;
	}
}
/// ディスクにファイルをインポート
void UiDiskFrame::ImportDataToDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->ShowImportDataFileDialog();
		return;
	}
	UiDiskRawPanel *panel = GetDiskRawPanel();
	if (panel) {
		panel->ShowImportDataDialog();
		return;
	}
}
/// ディスクからファイルを削除
void UiDiskFrame::DeleteDataFromDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->DeleteDataFile();
		return;
	}
	UiDiskRawPanel *panel = GetDiskRawPanel();
	if (panel) {
		panel->ShowDeleteDataDialog();
		return;
	}
}
/// ディスクのファイル名を変更
void UiDiskFrame::RenameDataOnDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->StartEditingFileName();
		return;
	}
}
/// ディスクのデータをコピー
void UiDiskFrame::CopyDataFromDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->CopyToClipboard();
		return;
	}
	UiDiskRawPanel *panel = GetDiskRawPanel();
	if (panel) {
		panel->CopyToClipboard();
		return;
	}
}
/// ディスクにデータをペースト
void UiDiskFrame::PasteDataToDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->PasteFromClipboard();
		return;
	}
	UiDiskRawPanel *panel = GetDiskRawPanel();
	if (panel) {
		panel->PasteFromClipboard();
		return;
	}
}
/// ディスクにディレクトリを作成
void UiDiskFrame::MakeDirectoryOnDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->ShowMakeDirectoryDialog();
		return;
	}
}
/// ファイルのプロパティ
void UiDiskFrame::PropertyOnDisk()
{
	wxWindow *fwin = wxWindow::FindFocus();
	UiDiskList *lpanel = GetLPanel();
	if (fwin == lpanel) {
		lpanel->ShowDiskAttr();
		return;
	}
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->ShowFileAttr();
		return;
	}
	UiDiskRawPanel *panel = GetDiskRawPanel();
	if (panel) {
		panel->ShowRawDiskAttr();
		return;
	}
}

/// DISK BASIC用に論理フォーマットできるか
bool UiDiskFrame::IsFormattableDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		return list->IsFormattableBasicDisk();
	}
	return false;
}

/// BASIC情報ダイアログ
void UiDiskFrame::ShowBasicAttr()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		list->ShowBasicAttr();
		return;
	}
}

/// DISK BASICが使用できるか
bool UiDiskFrame::CanUseBasicDisk()
{
	UiDiskFileList *list = GetFileListPanel();
	if (list) {
		return list->CanUseBasicDisk();
	}
	return false;
}

/// ディレクトリをアサイン
bool UiDiskFrame::AssignDirectory(DiskImageDisk *disk, int side_num, DiskBasicDirItem *dir_item)
{
	UiDiskFileList *listpanel = GetFileListPanel();
	if (!listpanel) return false;
	return listpanel->AssignDirectory(disk, side_num, dir_item);
}

/// ディレクトリを移動
bool UiDiskFrame::ChangeDirectory(DiskImageDisk *disk, int side_num, DiskBasicDirItem *dir_item, bool refresh_list)
{
	UiDiskFileList *listpanel = GetFileListPanel();
	if (!listpanel) return false;
	return listpanel->ChangeDirectory(disk, side_num, dir_item, refresh_list);
}

/// ディレクトリを削除
bool UiDiskFrame::DeleteDirectory(DiskImageDisk *disk, int side_num, DiskBasicDirItem *dir_item)
{
	UiDiskFileList *listpanel = GetFileListPanel();
	if (!listpanel) return false;
	return listpanel->DeleteDirectory(disk, side_num, dir_item);
}

/// ファイル名属性プロパティダイアログをすべて閉じる
void UiDiskFrame::CloseAllFileAttr()
{
	UiDiskFileList *listpanel = GetFileListPanel(true);
	if (listpanel) listpanel->CloseAllFileAttr();
}

////////////////////////////////////////
//
// 左パネル
//

/// 左パネルを返す
UiDiskList *UiDiskFrame::GetLPanel()
{
	return panel->GetLPanel();
}
/// 左パネルのディスクツリーを返す
UiDiskList *UiDiskFrame::GetDiskListPanel()
{
	return panel->GetLPanel();
}
/// 左パネルのディスクツリーにデータを設定する
void UiDiskFrame::SetDiskListData(const wxString &filename)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) lpanel->SetFileName(filename);
}
/// 左パネルのディスクツリーをクリア
void UiDiskFrame::ClearDiskListData()
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) lpanel->ClearFileName();
}
/// 左パネルのディスクツリーのディスクを選択しているか
bool UiDiskFrame::IsDiskListSelectedDisk()
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) return lpanel->IsSelectedDisk();
	else return false;
}

/// 左パネルのディスクツリーの選択している位置
/// @param [out] disk_number ディスク番号
/// @param [out] side_number サイド番号
void UiDiskFrame::GetDiskListSelectedPos(int &disk_number, int &side_number)
{
	UiDiskList *lpanel = GetDiskListPanel();
	int num = -1;
	int sid = -1;
	if (lpanel) {
		lpanel->GetSelectedDisk(num, sid);
	}
	disk_number = num;
	side_number = sid;
}

/// 左パネルのディスクツリーを選択
/// @param [in] disk_number ディスク番号
/// @param [in] side_number サイド番号
void UiDiskFrame::SetDiskListPos(int disk_number, int side_number)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->ChangeSelection(disk_number, side_number);
	}
}

/// 左パネルのディスクツリーにルートディレクトリを設定
void UiDiskFrame::RefreshRootDirectoryNodeOnDiskList(DiskImageDisk *disk, int side_number)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->RefreshRootDirectoryNode(disk, side_number);
	}
}

/// 左パネルのディスクツリーにディレクトリを設定
void UiDiskFrame::RefreshDirectoryNodeOnDiskList(DiskImageDisk *disk, DiskBasicDirItem *dir_item)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->RefreshDirectoryNode(disk, dir_item);
	}
}

/// 左パネルの全てのディレクトリツリーを更新
void UiDiskFrame::RefreshAllDirectoryNodesOnDiskList(DiskImageDisk *disk, int side_number)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->RefreshAllDirectoryNodes(disk, side_number);
	}
}

/// 左パネルのディスクツリーのディレクトリを選択
void UiDiskFrame::SelectDirectoryNodeOnDiskList(DiskImageDisk *disk, DiskBasicDirItem *dir_item)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->SelectDirectoryNode(disk, dir_item);
	}
}

/// 左パネルのディスクツリーのディレクトリを削除
void UiDiskFrame::DeleteDirectoryNodeOnDiskList(DiskImageDisk *disk, DiskBasicDirItem *dir_item)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->DeleteDirectoryNode(disk, dir_item);
	}
}

/// 左パネルのディスクツリーのディレクトリを一括削除
void UiDiskFrame::DeleteDirectoryNodesOnDiskList(DiskImageDisk *disk, DiskBasicDirItems &dir_items)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->DeleteDirectoryNodes(disk, dir_items);
	}
}

/// 左パネルのディスクツリーのディレクトリ名を再設定
void UiDiskFrame::RefreshDiskListDirectoryName(DiskImageDisk *disk)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->RefreshDirectoryName(disk);
	}
}

/// 左パネルのディスクツリーにファイルパスを設定
void UiDiskFrame::SetDiskListFilePath(const wxString &path)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->SetFilePath(path);
	}
}

/// 左パネルのディスクツリーにディスク名を設定
void UiDiskFrame::SetDiskListName(const wxString &name)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->SetName(name);
	}
}

/// 左パネルの選択しているディスクの子供を削除
/// @param [in] newparam BASICパラメータ 通常NULL BASICを変更した際に設定する 
void UiDiskFrame::RefreshDiskListOnSelectedDisk(const DiskBasicParam *newparam)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->RefreshSelectedDisk(newparam);
	}
}

/// 選択しているディスクのサイドを再選択
void UiDiskFrame::RefreshDiskListOnSelectedSide(const DiskBasicParam *newparam)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->RefreshSelectedSide(newparam);
	}
}

/// 左パネルのディスクツリーを再選択
/// @param [in] newparam BASICパラメータ 通常NULL BASICを変更した際に設定する 
void UiDiskFrame::ReSelectDiskList(const DiskBasicParam *newparam)
{
	UiDiskList *lpanel = GetDiskListPanel();
	if (lpanel) {
		lpanel->ReSelect(newparam);
	}
}

////////////////////////////////////////
//
// 右パネル
//

/// 右パネルを返す
UiDiskRPanel *UiDiskFrame::GetRPanel()
{
	return panel->GetRPanel();
}

/// 右パネルのデータウィンドウを変更 ファイルリスト/RAWディスク
void UiDiskFrame::ChangeRPanel(int num, const DiskBasicParam *param)
{
	UiDiskRPanel *rpanel = panel->GetRPanel();
	if (rpanel) rpanel->ChangePanel(num);
	UiDiskList *lpanel = panel->GetLPanel();
	if (lpanel) lpanel->ReSelect(param);

	UpdateMenuDisk();
	UpdateMenuMode();
}

/// 右パネルのすべてのコントロール内のデータをクリア
void UiDiskFrame::ClearRPanelData()
{
	DetachDiskBasicOnFileList();
	ClearDiskAttrData();
	ClearFileListData();
	ClearRawPanelData();
	ClearBinDumpData();
	ClearFatAreaData();
}

////////////////////////////////////////
//
// 右上パネルのディスク属性
//

/// 右上パネルのディスク属性パネルを返す
UiDiskDiskAttr *UiDiskFrame::GetDiskAttrPanel()
{
	UiDiskRPanel *rpanel = panel->GetRPanel();
	if (rpanel) return rpanel->GetDiskAttrPanel();
	else return NULL;
}
/// 右上パネルのディスク属性にデータを設定する
void UiDiskFrame::SetDiskAttrData(DiskImageFile *file)
{
	UiDiskDiskAttr *dapanel = GetDiskAttrPanel();
	if (dapanel) dapanel->SetAttr(file);
}
/// 右上パネルのディスク属性にデータを設定する
void UiDiskFrame::SetDiskAttrData(DiskImageDisk *disk)
{
	UiDiskDiskAttr *dapanel = GetDiskAttrPanel();
	if (dapanel) dapanel->SetAttr(disk);
}
/// 右上パネルのディスク属性をクリア
void UiDiskFrame::ClearDiskAttrData()
{
	UiDiskDiskAttr *dapanel = GetDiskAttrPanel();
	if (dapanel) dapanel->ClearData();
}

////////////////////////////////////////
//
// 右下パネルのファイルリスト
//

/// 右下パネルのファイルリストパネルを返す
/// @param [in] inst  true:常にポインタを返す / false:リスト非表示ならNULLを返す 
UiDiskFileList *UiDiskFrame::GetFileListPanel(bool inst)
{
	UiDiskRPanel *rpanel = panel->GetRPanel();
	if (rpanel) return rpanel->GetFileListPanel(inst);
	else return NULL;
}
/// 右下パネルのファイルリストにDISK BASICをアタッチ
void UiDiskFrame::AttachDiskBasicOnFileList(DiskImageDisk *disk, int side_num)
{
	UiDiskFileList *listpanel = GetFileListPanel();
	if (listpanel) listpanel->AttachDiskBasic(disk, side_num);
}
/// 右下パネルのファイルリストからDISK BASICをデタッチ
void UiDiskFrame::DetachDiskBasicOnFileList()
{
	UiDiskFileList *listpanel = GetFileListPanel(true);
	if (listpanel) listpanel->DetachDiskBasic();
}
/// 右下パネルのファイルリストにデータを設定する
void UiDiskFrame::SetFileListData()
{
	UiDiskFileList *listpanel = GetFileListPanel();
	if (listpanel) listpanel->SetFiles();
}
/// 右下パネルのファイルリストをクリア
void UiDiskFrame::ClearFileListData()
{
	UiDiskFileList *listpanel = GetFileListPanel();
	if (listpanel) listpanel->ClearFiles();
}

////////////////////////////////////////
//
// 右下パネルのRAWディスクパネル
//

/// 右下パネルのRAWディスクパネルを返す
/// @param [in] inst  true:常にポインタを返す / false:リスト非表示ならNULLを返す 
UiDiskRawPanel *UiDiskFrame::GetDiskRawPanel(bool inst)
{
	UiDiskRPanel *rpanel = panel->GetRPanel();
	if (rpanel) return rpanel->GetRawPanel(inst);
	else return NULL;
}
/// 右下パネルのRAWディスクパネルにデータを設定する
void UiDiskFrame::SetRawPanelData(DiskImageFile *file)
{
	UiDiskRawPanel *rawpanel = GetDiskRawPanel();
	if (rawpanel) rawpanel->SetTrackListData(file);
}
/// 右下パネルのRAWディスクパネルにデータを設定する
void UiDiskFrame::SetRawPanelData(DiskImageDisk *disk)
{
	UiDiskRawPanel *rawpanel = GetDiskRawPanel();
	if (rawpanel) rawpanel->SetTrackListData(disk);
}
/// 右下パネルのRAWディスクパネルをクリア
void UiDiskFrame::ClearRawPanelData()
{
	UiDiskRawPanel *rawpanel = GetDiskRawPanel();
	if (rawpanel) rawpanel->ClearTrackListData();
}
/// 右下パネルのRAWディスクパネルにデータを再設定する
void UiDiskFrame::RefreshRawPanelData()
{
	UiDiskRawPanel *rawpanel = GetDiskRawPanel();
	if (rawpanel) rawpanel->RefreshTrackListData();
}

////////////////////////////////////////
//
// ダンプウィンドウ
//

/// ダンプウィンドウにデータを設定する
void UiDiskFrame::SetBinDumpData(int sec_pos, const wxUint8 *buf, size_t len, const wxString &char_code, bool invert)
{
	if (bindump_frame) {
		bindump_frame->SetDataInvert(invert);
		bindump_frame->SetDatas(sec_pos, buf, len);
	}
}
/// ダンプウィンドウにデータを設定する
void UiDiskFrame::SetBinDumpData(int sec_pos, const wxUint8 *buf, size_t len)
{
	if (bindump_frame) {
		bindump_frame->SetDatas(sec_pos, buf, len);
	}
}
/// ダンプウィンドウにデータを追記する
void UiDiskFrame::AppendBinDumpData(int sec_pos, const wxUint8 *buf, size_t len, const wxString &char_code, bool invert)
{
	if (bindump_frame) {
		bindump_frame->SetDataInvert(invert);
		bindump_frame->AppendDatas(sec_pos, buf, len);
	}
}
/// ダンプウィンドウにデータを追記する
void UiDiskFrame::AppendBinDumpData(int sec_pos, const wxUint8 *buf, size_t len)
{
	if (bindump_frame) {
		bindump_frame->AppendDatas(sec_pos, buf, len);
	}
}
/// ダンプウィンドウをクリア
void UiDiskFrame::ClearBinDumpData()
{
	if (bindump_frame) {
		bindump_frame->ClearDatas();
	}
}

/// ダンプウィンドウを開く
void UiDiskFrame::OpenBinDumpWindow()
{
	if (bindump_frame) return;

	// ウィンドウを開く
	bindump_frame = new UiDiskBinDumpFrame(this, _("Dump View"), wxSize(640, 480));
	// 位置はメインウィンドウの右側
	wxSize sz = GetSize();
	wxPoint pt = GetPosition();
	pt.x = pt.x + sz.x - 32;
	bindump_frame->SetPosition(pt);
	bindump_frame->Show();
	bindump_frame->SetFocus();
}
/// ダンプウィンドウを閉じる
void UiDiskFrame::CloseBinDumpWindow()
{
	if (!bindump_frame) return;

	bindump_frame->Close();
	bindump_frame = NULL;
}
/// ダンプウィンドウを閉じる時にウィンドウ側から呼ばれるコールバック
void UiDiskFrame::BinDumpWindowClosed()
{
	bindump_frame = NULL;

	if (!IsBeingDeleted()) {
		wxMenuItem *mitem = menuView->FindItem(IDM_WINDOW_BINDUMP);
		if (mitem) {
			mitem->Check(false);
		}
	}
}

////////////////////////////////////////
//
// 使用状況ウィンドウ
//

/// 使用状況ウィンドウにデータを設定する
void UiDiskFrame::SetFatAreaData()
{
	if (fatarea_frame) {
		wxUint32 offset = 0;
		const wxArrayInt *arr = NULL;
		UiDiskFileList *list = GetFileListPanel();
		if (list) {
			list->GetFatAvailability(&offset, &arr);
			SetFatAreaData(offset, arr);
		}
	}
}

/// 使用状況ウィンドウにデータを設定する
void UiDiskFrame::SetFatAreaData(wxUint32 offset, const wxArrayInt *arr)
{
	if (fatarea_frame && arr) {
		fatarea_frame->SetData(offset, arr);
	}
}
/// 使用状況ウィンドウをクリア
void UiDiskFrame::ClearFatAreaData()
{
	if (fatarea_frame) {
		fatarea_frame->ClearData();
	}
}
/// 使用状況ウィンドウにフォーカスさせるグループ番号を設定する
void UiDiskFrame::SetFatAreaGroup(wxUint32 group_num)
{
	if (fatarea_frame) {
		fatarea_frame->SetGroup(group_num);
	}
}
/// 使用状況ウィンドウにフォーカスさせるグループ番号を設定する
void UiDiskFrame::SetFatAreaGroup(const DiskBasicGroups &group_items, const wxArrayInt &extra_group_nums)
{
	if (fatarea_frame) {
		fatarea_frame->SetGroup(group_items, extra_group_nums);
	}
}
/// 使用状況ウィンドウにフォーカスをはずすグループ番号を設定する
void UiDiskFrame::UnsetFatAreaGroup(const DiskBasicGroups &group_items, const wxArrayInt &extra_group_nums)
{
	if (fatarea_frame) {
		fatarea_frame->UnsetGroup(group_items, extra_group_nums);
	}
}
/// 使用状況ウィンドウでフォーカスしているグループ番号をクリア
void UiDiskFrame::ClearFatAreaGroup()
{
	if (fatarea_frame) {
		fatarea_frame->ClearGroup();
	}
}
/// 使用状況ウィンドウを開く
void UiDiskFrame::OpenFatAreaWindow()
{
	if (fatarea_frame) return;

	// ウィンドウを開く
	fatarea_frame = new UiDiskFatAreaFrame(this, _("Availability"), wxDefaultSize);
	// 位置はメインウィンドウの右側
	wxSize sz = GetSize();
	wxPoint pt = GetPosition();
	wxSize csz = fatarea_frame->GetSize();
	pt.x = pt.x + sz.x - 32;
	pt.y = pt.y + sz.y - csz.y;
	fatarea_frame->SetPosition(pt);
	fatarea_frame->Show();
	fatarea_frame->SetFocus();

	SetFatAreaData();
}
/// 使用状況ウィンドウを閉じる
void UiDiskFrame::CloseFatAreaWindow()
{
	if (!fatarea_frame) return;

	fatarea_frame->Close();
	fatarea_frame = NULL;
}
/// 使用状況ウィンドウを閉じる時にウィンドウ側から呼ばれるコールバック
void UiDiskFrame::FatAreaWindowClosed()
{
	fatarea_frame = NULL;

	if (!IsBeingDeleted()) {
		wxMenuItem *mitem = menuView->FindItem(IDM_WINDOW_FATAREA);
		if (mitem) {
			mitem->Check(false);
		}
	}
}

////////////////////////////////////////
//
// 設定ファイル
//

/// 最近使用したパスを取得
const wxString &UiDiskFrame::GetIniRecentPath() const
{
	return gConfig.GetFilePath();
}

/// 最近使用したパスを取得(エクスポート用)
const wxString &UiDiskFrame::GetIniExportFilePath() const
{
	return gConfig.GetExportFilePath();
}

/// 最近使用したファイルを更新（一覧も更新）
void UiDiskFrame::SetIniRecentPath(const wxString &path)
{
	// set recent file path
	gConfig.AddRecentFile(path);
	UpdateMenuRecentFiles();
}

/// 最近使用したパスを更新
void UiDiskFrame::SetIniFilePath(const wxString &path)
{
	gConfig.SetFilePath(path);
}

/// 最近使用したパスを更新(エクスポート用)
void UiDiskFrame::SetIniExportFilePath(const wxString &path, bool is_dir)
{
	gConfig.SetExportFilePath(path, is_dir);
}

/// ダンプフォントを更新
void UiDiskFrame::SetIniDumpFont(const wxFont &font)
{
	gConfig.SetDumpFontName(font.GetFaceName());
	gConfig.SetDumpFontSize(font.GetPointSize());
}

/// ダンプフォント名を返す
const wxString &UiDiskFrame::GetIniDumpFontName() const
{
	return gConfig.GetDumpFontName();
}

/// ダンプフォントサイズを返す
int UiDiskFrame::GetIniDumpFontSize() const
{
	return gConfig.GetDumpFontSize();
}

/// 設定ダイアログ表示
void UiDiskFrame::ShowConfigureDialog()
{
	ConfigBox dlg(this, wxID_ANY, &gConfig);
	int sts = dlg.ShowModal();
	if (sts == wxID_OK) {
		dlg.CommitData();
		// メニューを更新
		UpdateMenuMode();
		// ファイルリストを更新
		SetFileListData();
	}
}

/// ステータスカウンタを開始
int UiDiskFrame::StartStatusCounter(int count, const wxString &message)
{
	return stat_counters.Start(count, message);
}
/// ステータスカウンタに母数を追加
void UiDiskFrame::AppendStatusCounter(int idx, int count)
{
	stat_counters.Append(idx, count);
}
/// ステータスカウンタの数を＋１
void UiDiskFrame::IncreaseStatusCounter(int idx)
{
	stat_counters.Increase(idx);
	SetStatusText(stat_counters.GetCurrentMessage(idx), 0);
}
/// ステータスカウンタの計数を終了
void UiDiskFrame::FinishStatusCounter(int idx, const wxString &message)
{
	stat_counters.Finish(idx, message, this);
	SetStatusText(stat_counters.GetCurrentMessage(idx), 0);
}
/// ステータスカウンタをクリア
void UiDiskFrame::ClearStatusCounter()
{
	stat_counters.Clear();
	SetStatusText(wxT(""), 0);
}

/// 指定ファイルを引数にして外部エディタを起動する
/// @param[in] editor_type 0:binary editor 1:text editor
/// @param[in] file
bool UiDiskFrame::OpenFileWithEditor(enEditorTypes editor_type, const wxFileName &file)
{
	// エディタのあるパスを得る
	wxString editor = editor_type == EDITOR_TYPE_BINARY ? gConfig.GetBinaryEditor() : gConfig.GetTextEditor();
	if (editor.IsEmpty()) {
		wxMessageBox(_("No path of an editor specified."), _("Edit"), wxICON_ERROR | wxOK);
		return false;
	}
	// エディタを起動
	editor += wxT(" \"");
	editor += file.GetFullPath();
	editor += wxT("\"");

	wxProcess *process = NULL;
	long psts = wxExecute(editor, wxEXEC_SYNC, process);
	// エディタ終了
	if (psts < 0) {
		// コマンド起動失敗
		return false;
	}

	return true;
}


//////////////////////////////////////////////////////////////////////

// ドラッグアンドドロップ時のフォーマットID
wxDataFormat *UiDiskPanelDataFormat = NULL;

//
// メインパネルは分割ウィンドウ
//
// Attach Event
wxBEGIN_EVENT_TABLE(UiDiskPanel, wxSplitterWindow)
wxEND_EVENT_TABLE()

UiDiskPanel::UiDiskPanel(UiDiskFrame *parent)
                : wxSplitterWindow(parent, wxID_ANY,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_BORDER | wxSP_LIVE_UPDATE |
                                   wxCLIP_CHILDREN /* | wxSP_NO_XP_THEME */ )
{
    frame = parent;

	// resize right window when resize parent window.
	SetSashGravity(0.0);

	// control panel
	lpanel = new UiDiskList(frame, this);
	rpanel = new UiDiskRPanel(frame, this, frame->GetSelectedMode());
	
	SplitVertically(lpanel, rpanel, 200);

	SetMinimumPaneSize(10);

	SetDropTarget(new UiDiskPanelDropTarget(parent, this));
}

UiDiskPanel::~UiDiskPanel()
{
}

/// 外部からのDnD
bool UiDiskPanel::ProcessDroppedFiles(wxCoord x, wxCoord y, const wxArrayString &filenames)
{
	if (filenames.Count() == 0) return false;

	bool is_disk_file = false;

	// 分割位置
	int pos_x = GetSashPosition();
	bool disk_is_empty = (frame->GetDiskImage().CountDisks() == 0);

	if (disk_is_empty || x < pos_x) {
		is_disk_file = true;
	}
	bool sts = false;
	if (is_disk_file) {
		// ディスクイメージ１つだけ開く
		frame->OpenDroppedFile(filenames.Item(0));
	} else {
		// ファイルをインポート
		UiDiskFileList *file_list = rpanel->GetFileListPanel();
		if (file_list) {
			sts = file_list->DropDataFiles(filenames);
		}
		UiDiskRawPanel *raw_panel = rpanel->GetRawPanel();
		if (raw_panel) {
			for(int n = 0; n < (int)filenames.Count(); n++) {
				wxString filename = filenames.Item(n);
				sts = raw_panel->ShowImportTrackRangeDialog(filename);
			}
		}
	}
	return sts;
}

//////////////////////////////////////////////////////////////////////
//
// File Drag and Drop
//
UiDiskPanelDropTarget::UiDiskPanelDropTarget(UiDiskFrame *parentframe, UiDiskPanel *parentwindow)
	: wxDropTarget()
{
	parent = parentwindow;
	frame = parentframe;

	wxDataObjectComposite* dataobj = new wxDataObjectComposite();

	dataobj->Add(new wxFileDataObject());
	SetDataObject(dataobj);
}

wxDragResult UiDiskPanelDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult def)
{
	if ( !GetData() ) return wxDragNone;
	bool sts = false;
	wxDataObjectComposite *comobj = (wxDataObjectComposite *)GetDataObject();
	if (comobj) {
		wxDataFormat fmt = comobj->GetReceivedFormat();
		if (fmt.GetType() == wxDF_FILENAME) {
			// エクスプローラからのDnD
			wxFileDataObject *dobj = (wxFileDataObject *)comobj->GetObject(fmt);
			sts = parent->ProcessDroppedFiles(x, y, dobj->GetFilenames());
		}
	}
	return (sts ? def : wxDragError);
}

//////////////////////////////////////////////////////////////////////
//
// File Dialog
//
UiDiskFileDialog::UiDiskFileDialog(const wxString& message, const wxString& defaultDir, const wxString& defaultFile, const wxString& wildcard, long style)
            : wxFileDialog(NULL, message, defaultDir, defaultFile, wildcard, style)
{
}

//////////////////////////////////////////////////////////////////////
//
// Dir Dialog
//
UiDiskDirDialog::UiDiskDirDialog(const wxString& message, const wxString& defaultDir, long style)
            : wxDirDialog(NULL, message, defaultDir, style)
{
}

//////////////////////////////////////////////////////////////////////
//
// About dialog
//
UiDiskAbout::UiDiskAbout(wxWindow* parent, wxWindowID id)
	: wxDialog(parent, id, _("About..."), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
	wxSizerFlags flags = wxSizerFlags().Expand().Border(wxALL, 4);

	wxBoxSizer *szrLeft   = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrRight  = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *szrMain   = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer *szrAll    = new wxBoxSizer(wxVERTICAL);

	szrLeft->Add(new wxStaticBitmap(this, wxID_ANY,
		wxBitmap(APPLICATION_XPMICON_NAME), wxDefaultPosition, wxSize(64, 64))
		, flags);

	wxString str = _T("");
	str += _T("DiskImageFinder, Version ");
	str += _T(APPLICATION_VERSION);
	str += _T(" \"");
	str += _T(PLATFORM);
	str += _T("\"\n\n");
#ifdef _DEBUG
	str += _T("(Debug Version)\n\n");
#endif
	str	+= _T("using ");
	str += wxVERSION_STRING;
	str += _T("\n\n");
	str	+= _T(APP_COPYRIGHT);

	szrRight->Add(new wxStaticText(this, wxID_ANY, str), flags);

	wxSizer *szrButtons = CreateButtonSizer(wxOK);
	szrMain->Add(szrLeft, flags);
	szrMain->Add(szrRight, flags);
	szrAll->Add(szrMain, flags);
	szrAll->Add(szrButtons, flags);

	SetSizerAndFit(szrAll);
}
