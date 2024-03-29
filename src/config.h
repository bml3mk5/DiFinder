﻿/// @file config.h
///
/// @brief 設定ファイル入出力
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include "common.h"
#include <wx/string.h>
#include <wx/arrstr.h>
#include "ui/uifilelist.h"


#define MAX_RECENT_FILES 20
// セクタキャッシュ限界サイズ(MB)
#define CACHE_LIMIT_SIZE 200
// セクタキャッシュ縮小サイズ(MB)
#define CACHE_SHRINK_SIZE 100

/// 設定ファイルパラメータ
class Params
{
protected:
	wxString	mFilePath;			///< ファイルパス
	wxString	mExportFilePath;	///< エクスポート先パス
	wxArrayString mRecentFiles;		///< 最近使用したファイル
	wxString	mCharCode;			///< キャラクターコード名
	wxString	mListFontName;		///< リストウィンドウのフォント名
	int			mListFontSize;		///< リストウィンドウのフォントサイズ
	wxString	mDumpFontName;		///< ダンプウィンドウのフォント名
	int			mDumpFontSize;		///< ダンプウィンドウのフォントサイズ
	bool		mShowDeletedFile;	///< 削除したファイルを表示するか
	bool		mAddExtExport;		///< エクスポート時に属性から拡張子を追加するか
	bool		mCurrentDateExport;	///< エクスポート時に現在日時を設定するか
	bool		mDecideAttrImport;	///< インポート時に拡張子で属性を決定したら拡張子を削除するか
	bool		mSkipImportDialog;	///< インポートダイアログを抑制するか
	bool		mIgnoreDateTime;	///< インポートやプロパティ変更時に日時を無視するか
	bool		mCurrentDateImport;	///< インポート時に現在日時を設定するか
	bool		mShowInterDirItem;	///< プロパティで内部データをリストで表示するか
	int			mDirDepth;			///< 一度に処理できるディレクトリの深さ
	int			mWindowWidth;		///< ウィンドウ幅
	int			mWindowHeight;		///< ウィンドウ高さ
	wxString	mTemporaryFolder;	///< テンポラリフォルダのパス
	wxString	mBinaryEditor;		///< バイナリエディタのパス
	wxString	mTextEditor;		///< テキストエディタのパス
	int			mCacheLimitSize;	///< セクタキャッシュの限界サイズ(MB)
	int			mCacheShrinkSize;	///< セクタキャッシュの縮小サイズ(MB)
	wxString	mLanguage;			///< 言語
	int			mListColumnWidth[LISTCOL_END];	///< ファイルリストの各カラムの幅
	int			mListColumnPos[LISTCOL_END];	///< ファイルリストの各カラムの位置

public:
	Params();
	virtual ~Params() {}

	/// @name properties
	//@{
	void			SetFilePath(const wxString &val);
	const wxString &GetFilePath() const { return mFilePath; }
	void			SetExportFilePath(const wxString &val, bool is_dir = false);
	const wxString &GetExportFilePath() const;
	void			AddRecentFile(const wxString &val);
	const wxString &GetRecentFile() const;
	const wxArrayString &GetRecentFiles() const;
	void			SetCharCode(const wxString &val) { mCharCode = val; }
	const wxString &GetCharCode() const { return mCharCode; }
	void			SetListFontName(const wxString &val) { mListFontName = val; }
	const wxString &GetListFontName() const { return mListFontName; }
	void			SetListFontSize(int val) { mListFontSize = val; }
	int				GetListFontSize() const { return mListFontSize; }
	void			SetDumpFontName(const wxString &val) { mDumpFontName = val; }
	const wxString &GetDumpFontName() const { return mDumpFontName; }
	void			SetDumpFontSize(int val) { mDumpFontSize = val; }
	int				GetDumpFontSize() const { return mDumpFontSize; }
	void			ShowDeletedFile(bool val) { mShowDeletedFile = val; }
	bool			IsShownDeletedFile() const { return mShowDeletedFile; }
	void			AddExtensionExport(bool val) { mAddExtExport = val; }
	bool			IsAddExtensionExport() const { return mAddExtExport; }
	void			SetCurrentDateExport(bool val) { mCurrentDateExport = val; }
	bool			IsSetCurrentDateExport() const { return mCurrentDateExport; }
	void			DecideAttrImport(bool val) { mDecideAttrImport = val; }
	bool			IsDecideAttrImport() const { return mDecideAttrImport; }
	void			SkipImportDialog(bool val) { mSkipImportDialog = val; }
	bool			IsSkipImportDialog() const { return mSkipImportDialog; }
	void			IgnoreDateTime(bool val) { mIgnoreDateTime = val; }
	bool			DoesIgnoreDateTime() const { return mIgnoreDateTime; }
	void			SetCurrentDateImport(bool val) { mCurrentDateImport = val; }
	bool			IsSetCurrentDateImport() const { return mCurrentDateImport; }
	void			ShowInterDirItem(bool val) { mShowInterDirItem = val; }
	bool			DoesShowInterDirItem() const { return mShowInterDirItem; }
	void			SetDirDepth(int val) { mDirDepth = val; }
	int				GetDirDepth() const { return mDirDepth; }
	void			SetWindowWidth(int val) { mWindowWidth = val; }
	int				GetWindowWidth() const { return mWindowWidth; }
	void			SetWindowHeight(int val) { mWindowHeight = val; }
	int				GetWindowHeight() const { return mWindowHeight; }
	void			SetTemporaryFolder(const wxString &val);
	const wxString &GetTemporaryFolder() const { return mTemporaryFolder; }
	void			ClearTemporaryFolder() { mTemporaryFolder.Empty(); }
	void			SetBinaryEditor(const wxString &val);
	const wxString &GetBinaryEditor() const { return mBinaryEditor; }
	void			SetTextEditor(const wxString &val);
	const wxString &GetTextEditor() const { return mTextEditor; }
	void 			SetCacheLimitSize(int val) { mCacheLimitSize = val; }
	int				GetCacheLimitSize() const { return mCacheLimitSize; }
	void			SetCacheShrinkSize(int val) { mCacheShrinkSize = val; }
	int				GetCacheShrinkSize() const { return mCacheShrinkSize; }
	void			SetLanguage(const wxString &val) { mLanguage = val; }
	const wxString &GetLanguage() const { return mLanguage; }
	void			SetListColumnWidth(int id, int val) { mListColumnWidth[id] = val; }
	int				GetListColumnWidth(int id) const { return mListColumnWidth[id]; }
	void			SetListColumnPos(int id, int val) { mListColumnPos[id] = val; }
	int				GetListColumnPos(int id) const { return mListColumnPos[id]; }
	//@}
};

/// 設定ファイル入出力
class Config : public Params
{
private:
	wxString ini_file;

public:
	Config();
	~Config();
	void SetFileName(const wxString &file);
	void Load(const wxString &file);
	void Load();
	void Save();
	static void CalcCacheSize(int &limit, int &shrink);
};

extern Config gConfig;

#endif /* _CONFIG_H_ */
