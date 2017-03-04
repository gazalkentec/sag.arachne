#pragma once

#pragma comment (lib, "libnodave/libnodave.lib")

#include <fstream>

#include "sqlite/sqlite3.h"

enum DB_TYPES {
	DB_ORACLE = 0,
	DB_MSSQL = 1,
	DB_SQLITE = 2
};

enum PLC_TYPES {
	AUTODETECT = 0,
	S7300 = 1
};

enum SERVICE_TYPES {
	ARACHNE_NODE = 0,
	ARACHNE_PLC_CONTROL = 1,
	ARACHNE_WEIGHT_CONTROL = 2,
	ARACHNE_TERMAL_CONTROL = 3,
	ARACHNE_BUZZER_CONTROL = 4
};

inline LPTSTR ExtractFilePath(LPCTSTR FileName, LPTSTR buf)
{
	int i, len = lstrlen(FileName);
	for (i = len - 1; i >= 0; i--)
	{
		if (FileName[i] == _T('\\'))
			break;
	}
	lstrcpyn(buf, FileName, i + 2);
	return buf;
}