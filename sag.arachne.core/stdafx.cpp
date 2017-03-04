// stdafx.cpp: исходный файл, содержащий только стандартные включаемые модули
// sag.arachne.core.pch будет предкомпилированным заголовком
// stdafx.obj будет содержать предварительно откомпилированные сведения о типе

#include "stdafx.h"

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