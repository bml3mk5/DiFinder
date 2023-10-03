/// @file diskplainwriter.cpp
///
/// @brief べたディスクライター
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskplainwriter.h"
#include <wx/stream.h>
#include "../diskimg/diskimage.h"
#include "diskresult.h"


//
// べた形式で保存
//
DiskPlainWriter::DiskPlainWriter(DiskWriter *dw_, DiskResult *result_)
	: DiskImageWriter(dw_, result_)
{
}

/// べたイメージでファイルに保存
/// @param [in,out] image ディスクイメージ
/// @param [in]     disk_number ディスク番号(0-) / -1のときは全体 
/// @retval  0 正常
/// @retval -1 エラー
int DiskPlainWriter::SaveDisk(DiskImage *image, int disk_number)
{
	p_result->Clear();

	DiskImageFile *file = image->GetFile();
	if (!file) {
		p_result->SetError(DiskResult::ERR_NO_DATA);
		return p_result->GetValid();
	}

	if (disk_number < 0) {
		// 全体を保存
		return SaveFile(file);

	} else {
		// 指定したディスクを保存
		DiskImageDisk *disk = file->GetDisk(disk_number);

		if (!disk) {
			p_result->SetError(DiskResult::ERR_NO_DISK);
			return p_result->GetValid();
		}

		return SaveDisk(disk); 
	}
}
/// べたイメージを保存
int DiskPlainWriter::SaveFile(DiskImageFile *file)
{
	file->Flush();

	return 0;
}
/// べたイメージでディスク1つを保存
/// @param [in,out] disk        ディスク1つのイメージ
/// @retval  0 正常
/// @retval -1 エラー
int DiskPlainWriter::SaveDisk(DiskImageDisk *disk)
{
	disk->Flush();

	return 0;
}
