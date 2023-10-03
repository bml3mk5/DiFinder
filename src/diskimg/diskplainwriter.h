/// @file diskplainwriter.h
///
/// @brief べたディスクライター
///
/// @author Copyright (c) Sasaji. All rights reserved.
///

#ifndef _DISKPLAIN_WRITER_H_
#define _DISKPLAIN_WRITER_H_

#include "../common.h"
#include "diskwriter.h"


//class wxOutputStream;
class DiskWriter;
class DiskImage;
class DiskImageFile;
class DiskImageDisk;
class DiskResult;

/// べたディスクライター
class DiskPlainWriter : public DiskImageWriter
{
protected:
	/// 保存
	virtual int SaveFile(DiskImageFile *file);
	/// ディスク1つを保存
	virtual int SaveDisk(DiskImageDisk *disk);

public:
	DiskPlainWriter(DiskWriter *dw_, DiskResult *result_);

	/// ストリームの内容をファイルに保存
	virtual int SaveDisk(DiskImage *image, int disk_number);
};

#endif /* _DISKPLAIN_WRITER_H_ */
