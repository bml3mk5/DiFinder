/// @file diskd88writer.cpp
///
/// @brief D88ディスクライター
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#include "diskd88writer.h"
#include <wx/stream.h>
#include "../diskd88.h"
#include "diskd88creator.h"
#include "diskresult.h"


//
// D88形式で保存
//
DiskD88Writer::DiskD88Writer(DiskWriter *dw_, DiskResult *result_)
	: DiskImageWriter(dw_, result_)
{
}

/// ストリームの内容をファイルに保存できるか
/// @param [in,out] image ディスクイメージ
/// @param [in]     disk_number ディスク番号(0-) / -1のときは全体 
/// @param [in]     side_number サイド番号(0-) / -1のときは両面 
/// @retval  0 正常
/// @retval  1 警告あり
/// @retval -1 エラー
int DiskD88Writer::ValidateDisk(DiskImage *image, int disk_number, int side_number)
{
	m_result->Clear();

	DiskImageFile *file = image->GetFile();
	if (!file) {
		m_result->SetError(DiskResult::ERR_NO_DATA);
		return m_result->GetValid();
	}

	if (disk_number < 0) {
		// 全体を保存できるか
		DiskImageDisks *disks = file->GetDisks();
		if (!disks || disks->Count() <= 0) {
			m_result->SetError(DiskResult::ERR_NO_DISK);
			return m_result->GetValid();
		}
		for(size_t disk_num = 0; disk_num < disks->Count(); disk_num++) {
			DiskImageDisk *disk = disks->Item(disk_num);
#if 0
			DiskD88Tracks *tracks = disk->GetTracks();
			if (tracks && tracks->Count() > DISKD88_MAX_TRACKS) {
				m_result->SetWarn(DiskResult::ERRV_TOO_MANY_TRACKS, disk_num, DISKD88_MAX_TRACKS);
			}
#endif
		}
	} else {
		// 指定したディスクを保存できるか
		DiskImageDisk *disk = file->GetDisk(disk_number);
#if 0
		DiskImageTracks *tracks = disk->GetTracks();
		if (tracks && tracks->Count() > DISKD88_MAX_TRACKS) {
			m_result->SetWarn(DiskResult::ERRV_TOO_MANY_TRACKS, disk_number, DISKD88_MAX_TRACKS);
		}
#endif
	}

	return m_result->GetValid();
}

/// ストリームの内容をファイルに保存
/// @param [in,out] image ディスクイメージ
/// @param [in]     disk_number ディスク番号(0-) / -1のときは全体 
/// @param [in]     side_number サイド番号(0-) / -1のときは両面 
/// @param [out]    ostream     出力先
/// @retval  0 正常
/// @retval -1 エラー
int DiskD88Writer::SaveDisk(DiskImage *image, int disk_number, int side_number, wxOutputStream *ostream)
{
	m_result->Clear();

	DiskImageFile *file = image->GetFile();
	if (!file) {
		m_result->SetError(DiskResult::ERR_NO_DATA);
		return m_result->GetValid();
	}

	if (disk_number < 0) {
		// 全体を保存
		DiskImageDisks *disks = file->GetDisks();
		if (!disks || disks->Count() <= 0) {
			m_result->SetError(DiskResult::ERR_NO_DISK);
			return m_result->GetValid();
		}
		for(size_t disk_num = 0; disk_num < disks->Count(); disk_num++) {
			DiskImageDisk *disk = disks->Item(disk_num);
			SaveDisk(disk, -1, ostream); 
		}
	} else {
		// 指定したディスクを保存
		DiskImageDisk *disk = file->GetDisk(disk_number);

		SaveDisk(disk, side_number, ostream); 
	}

	return m_result->GetValid();
}

/// ディスク1つを保存
/// @param [in]  disk        ディスク
/// @param [in]  side_number サイド 両面なら -1
/// @param [out] ostream     出力先
int DiskD88Writer::SaveDisk(DiskImageDisk *disk, int side_number, wxOutputStream *ostream)
{
	if (!disk) {
		m_result->SetError(DiskResult::ERR_NO_DISK);
		return m_result->GetValid();
	}

	// オフセットを再計算する
	size_t new_size = 0;
	disk->SetOffsetStart(sizeof(d88_header_t));
	if (side_number < 0) {
		new_size = disk->ShrinkTracks(m_dw->IsTrimUnusedData());
		disk->SetSizeWithoutHeader((wxUint32)new_size);
	}

	// ディスクヘッダ
	DiskD88DiskHeader newheader;
//	memset(&newheader, 0, sizeof(d88_header_t));
//	memcpy(&newheader, disk->GetHeader(), (size_t)disk->GetOffsetStart());
	if (!disk->CopyHeaderTo(newheader)) {
		// TODO: D88以外のデータから作る場合
	}

	// write disk header
	ostream->Write(&newheader, sizeof(d88_header_t));	

	DiskImageTracks *tracks = disk->GetTracks();
	if (!tracks) {
		m_result->SetError(DiskResult::ERR_NO_DATA);
		return m_result->GetValid();
	}

	// オフセットクリア
	newheader.ClearOffsets();
//	memset(newheader.offsets, 0, sizeof(newheader.offsets));

	size_t track_start = (side_number < 0 ? 0 : side_number);
	size_t track_count = tracks->Count();
	size_t track_step  = (side_number < 0 ? 1 : 2);

	size_t track_offpos = 0;
	size_t track_offset = (size_t)disk->GetOffsetStart();
	for(size_t track_num = track_start; track_num < track_count && track_offpos < DISKD88_MAX_TRACKS; track_num += track_step) {
		DiskImageTrack *track = tracks->Item(track_num);
		if (!track) continue;
		size_t track_size = 0;
		DiskImageSectors *sectors = track->GetSectors();
		size_t count = sectors ? sectors->Count() : 0;
		for(size_t sector_num = 0; sector_num < count; sector_num++) {
			DiskImageSector *sector = sectors->Item(sector_num);
			if (!sector) continue;

			// セクタヘッダ
			DiskD88SectorHeader secheader;
//			memcpy(&secheader, sector->GetHeader(), sizeof(d88_sector_header_t));
			if (!sector->CopyHeaderTo(secheader)) {
				// TODO: ヘッダ情報がない場合
				// C,H,R,Nをセット
				secheader.IDC(sector->GetIDC());
				secheader.IDH(sector->GetIDH());
				secheader.IDR(sector->GetIDR());
				secheader.IDN(sector->GetIDN());
				secheader.SetNumberOfSectors(sector->GetSectorsPerTrack());
				secheader.SetBufferSize(sector->GetIDN());
			}

			if (side_number >= 0) {
				// 片面だけ保存のときはID Hを0にする
//				secheader.id.h = 0;
				secheader.IDH(0);
			}

			// write sector header
			ostream->Write(&secheader, sizeof(d88_sector_header_t));
			track_size += sizeof(d88_sector_header_t);

			// write sector body
			wxUint8 *buffer = sector->GetSectorBuffer();
			size_t buffer_size = sector->GetSectorBufferSize();
			if (buffer && buffer_size) {
				ostream->Write((void *)buffer, buffer_size);	
				track_size += buffer_size;
			}
//			sector->ClearModify();
		}
		//
		if (!m_dw->IsTrimUnusedData()) {
			// 余分なデータ
			wxUint8 *extra_data = track->GetExtraData();
			size_t   extra_size = track->GetExtraDataSize();
			if (extra_data && extra_size > 0) {
				ostream->Write(extra_data, extra_size);
				track_size += extra_size;
			}
		}
		if (track_size > 0) {
			// オフセットをセット
			newheader.SetOffset(track_offpos, (wxUint32)track_offset);
//			newheader.offsets[track_offpos] = wxUINT32_SWAP_ON_BE((wxUint32)track_offset);
			track_offpos++;
			track_offset += track_size;
			newheader.SetDiskSize((wxUint32)track_offset);
//			newheader.disk_size = wxUINT32_SWAP_ON_BE((wxUint32)track_offset);
		}
	}
	if (side_number >= 0) {
		// 片面だけ保存のときはディスクヘッダを更新
		ostream->SeekO(0);
		ostream->Write(&newheader, sizeof(d88_header_t));
	}

	if (m_result->GetValid() >= 0) {
		disk->ClearModify();
	}

	return m_result->GetValid();
}
