/// @file diskplaincreator.cpp
///
/// @brief ベタディスクイメージ作成
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskplaincreator.h"
#include <wx/wfstream.h>
#include "diskparam.h"
#include "diskplain.h"
#include "diskresult.h"


//
//
//
DiskPlainCreator::DiskPlainCreator(const wxString &diskname, const DiskParam &param, bool write_protect, DiskImageFile *file, DiskResult &result)
{
	m_diskname = diskname;
	p_param = &param;
	m_write_protect = write_protect;
	p_file = file;
	p_result = &result;
}

DiskPlainCreator::~DiskPlainCreator()
{
}

/// ディスクデータの作成
/// @return 作成したディスクサイズ
wxUint32 DiskPlainCreator::CreateFile()
{
	int sector_nums = p_param->GetTracksPerSide() * p_param->GetSidesPerDisk() * p_param->GetSectorsPerTrack();
	int sector_size = p_param->GetSectorSize();
	if (sector_nums == 0) {
		p_result->SetError(DiskResult::ERR_CANNOT_SAVE);
		return 0;
	}

//	wxString file_path = p_file->GetFilePath();
	wxString file_path = m_diskname;

	// create new file
	wxFileOutputStream ostream(file_path);
	if (!ostream.IsOk()) {
		p_result->SetError(DiskResult::ERR_CANNOT_SAVE);
		return 0;
	}

	DiskPlainSectorHeader header;
	DiskPlainSector *sector = new DiskPlainSector(0, header, NULL, sector_size, 0);

	for(int n = 0; n < sector_nums; n++) {
		size_t len = ostream.Write(sector->GetSectorBuffer(), sector->GetSectorBufferSize()).LastWrite();
		if (len == 0) break;
	}

	delete sector;

	return (wxUint32)sector_nums * sector_size;
}
/// ディスクイメージの新規作成
int DiskPlainCreator::Create()
{
	CreateFile();
	return p_result->GetValid();
}
