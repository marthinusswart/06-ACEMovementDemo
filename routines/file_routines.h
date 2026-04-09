#ifndef FILE_ROUTINES_H
#define FILE_ROUTINES_H

#include <exec/types.h>

/**
 * @brief Load a file from disk into chip memory
 *
 * @param szFilePath Relative path to the file to load (e.g., "bpl/stage-0001.bpl")
 * @param pDataOut Pointer to pointer that will receive the allocated chip memory address
 * @return File size in bytes on success, 0 on failure
 */
ULONG loadFileToChipMem(const char *szFilePath, void **pDataOut);

/**
 * @brief Free chip memory allocated by loadFileToChipMem
 *
 * @param pData Pointer to the chip memory to free
 * @param ulSize Size of the memory block in bytes
 */
void freeChipMem(void *pData, ULONG ulSize);

#endif // FILE_ROUTINES_H
