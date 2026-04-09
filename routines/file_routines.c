#include "file_routines.h"
#include <ace/utils/disk_file.h>
#include <ace/utils/file.h>
#include <proto/exec.h>
#include <exec/memory.h>

extern struct ExecBase *SysBase;

ULONG loadFileToChipMem(const char *szFilePath, void **pDataOut)
{
	if (!szFilePath || !pDataOut)
		return 0;

	*pDataOut = NULL;

	// Open the file
	tFile *pFile = diskFileOpen(szFilePath, DISK_FILE_MODE_READ, 0);
	if (!pFile)
		return 0;

	// Get file size
	LONG lFileSize = fileGetSize(pFile);
	if (lFileSize <= 0)
	{
		fileClose(pFile);
		return 0;
	}

	// Allocate chip memory
	void *pChipMem = AllocMem(lFileSize, MEMF_CHIP | MEMF_CLEAR);
	if (!pChipMem)
	{
		fileClose(pFile);
		return 0;
	}

	// Read file into chip memory
	ULONG ulBytesRead = fileRead(pFile, pChipMem, lFileSize);
	fileClose(pFile);

	if (ulBytesRead != (ULONG)lFileSize)
	{
		FreeMem(pChipMem, lFileSize);
		return 0;
	}

	*pDataOut = pChipMem;
	return (ULONG)lFileSize;
}

void freeChipMem(void *pData, ULONG ulSize)
{
	if (pData && ulSize > 0)
	{
		FreeMem(pData, ulSize);
	}
}
