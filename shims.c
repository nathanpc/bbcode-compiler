/**
 * shims.c
 * A collection of shims and Windows-specific functions.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "shims.h"

#ifdef _WIN32
	#include <windows.h>
	#include <tchar.h>

	#if defined(_DEBUG) && !defined(UNDER_CE)
		#include <crtdbg.h>
	#endif // defined(DEBUG) && !defined(UNDER_CE)
#endif /* _WIN32 */

#if defined(_WIN32) && defined(_DEBUG) && !defined(UNDER_CE)
/* Windows memory leak detection debugging variables. */
static _CrtMemState snapBegin;
static _CrtMemState snapEnd;
static _CrtMemState snapDiff;
#endif // _WIN32 && DEBUG && !UNDER_CE

/**
 * Takes a memory snapshot to later check for leaks.
 *
 * @warning This function should only be used in Windows.
 */
void mem_snapshot_start(void) {
#if defined(_WIN32) && defined(_DEBUG) && !defined(UNDER_CE)
	_CrtMemCheckpoint(&snapBegin);
#endif // _WIN32 && DEBUG && !UNDER_CE
}

/**
 * Produce a report if any memory leaks have been found.
 *
 * @warning This function should only be used in Windows.
 */
void mem_snapshot_report(void) {
#if defined(_WIN32) && defined(_DEBUG) && !defined(UNDER_CE)
	_CrtMemCheckpoint(&snapEnd);
	if (_CrtMemDifference(&snapDiff, &snapBegin, &snapEnd)) {
		OutputDebugString(_T("MEMORY LEAKS DETECTED\r\n"));
		OutputDebugString(_T("----------- _CrtMemDumpStatistics ---------\r\n"));
		_CrtMemDumpStatistics(&snapDiff);
		OutputDebugString(_T("----------- _CrtMemDumpAllObjectsSince ---------\r\n"));
		_CrtMemDumpAllObjectsSince(&snapBegin);
		OutputDebugString(_T("----------- _CrtDumpMemoryLeaks ---------\r\n"));
		_CrtDumpMemoryLeaks();
	} else {
		OutputDebugString(_T("No memory leaks detected. Congratulations!\r\n"));
	}
#endif // _WIN32 && DEBUG && !UNDER_CE
}
