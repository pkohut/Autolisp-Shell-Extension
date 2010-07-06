// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#pragma pack (push, 8)
#pragma warning(disable: 4786 4996)

#define VC_EXTRALEAN
#define STRICT

//-----------------------------------------------------------------------------
#include <windows.h>
#include <map>
#include "arxHeaders.h"
#include <string>


#ifdef _UNICODE
    typedef std::wstring TString;
#else
    typedef std::string TString;
#endif

#pragma pack (pop)

