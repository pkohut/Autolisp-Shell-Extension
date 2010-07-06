/**	\file ShellPipe.cpp
*	\brief 
*/

/****************************************************************************/
/*	ShellPipe.cpp													*/
/****************************************************************************/
/*                                                                          */
/*  Copyright 2010 Paul Kohut                                               */
/*  Licensed under the Apache License, Version 2.0 (the "License"); you may */
/*  not use this file except in compliance with the License. You may obtain */
/*  a copy of the License at                                                */
/*                                                                          */
/*  http://www.apache.org/licenses/LICENSE-2.0                              */
/*                                                                          */
/*  Unless required by applicable law or agreed to in writing, software     */
/*  distributed under the License is distributed on an "AS IS" BASIS,       */
/*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or         */
/*  implied. See the License for the specific language governing            */
/*  permissions and limitations under the License.                          */
/*                                                                          */
/****************************************************************************/

/*  Code in this file is based on MSDN article "Creating a Child Process	*/
/*  with Redirected Input and Output										*/
/*  http://msdn.microsoft.com/en-us/library/ms682499%28VS.85%29.aspx		*/

#include "StdAfx.h"
#include "ShellPipe.h"
#include <tchar.h>
#include <algorithm>
#include <cctype>

#define _ENVIRONMENT_VARIABLE_LIMIT 32768

// trim from both ends
template<class T>
static inline T &trim(T &s) {
    return ltrim(rtrim(s));
}

// trim from start
template<class T>
static inline T &ltrim(T &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
template<class T>
static inline T &rtrim(T &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

int ExpandEnvironmentString(const TCHAR * pcszSource, TString & sNewString)
{
    //	pszNewString = NULL;
    DWORD dwSize = ExpandEnvironmentStrings(pcszSource, NULL, 0);
    if(dwSize == 0) {
        return RTERROR;
    }
    sNewString.resize(dwSize);
    dwSize = ExpandEnvironmentStrings(pcszSource, &sNewString[0], dwSize);
    if(dwSize == 0) {
        sNewString = _T("");
        return RTERROR;
    }
    return RTNORM;
}



DWORD CShellPipe::m_dwLastError = 0;

CShellPipe::CShellPipe(void)
{
    memset(&m_pi, 0, sizeof(PROCESS_INFORMATION));
	m_dwLastError = 0;
}

CShellPipe::~CShellPipe(void)
{
}

int CShellPipe::SetErrorReturnCode( void )
{
	m_dwLastError = GetLastError();
	return RTERROR;
}

int CShellPipe::OpenShell( const TCHAR * pcszApplicationName, const TCHAR * pcszCommandLine )
{
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	// Create a pipe for the child process's STDOUT. 
	if(!CreatePipe(&m_hParentRead.Handle(), &m_hChildWrite.Handle(), &sa, 0))
		return SetErrorReturnCode();

	// Ensure the read handle to the pipe for STDOUT is not inherited.
	SetHandleInformation(m_hParentRead.Handle(), HANDLE_FLAG_INHERIT, 0);

	// Create a pipe for the child process's STDIN. 
	if(!CreatePipe(&m_hChildRead.Handle(), &m_hParentWrite.Handle(), &sa, 0))
		return SetErrorReturnCode();

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	SetHandleInformation(m_hParentWrite.Handle(), HANDLE_FLAG_INHERIT, 0);

	// Create a pipe for the child process's STDERR. 
	if(!CreatePipe(&m_hParentError.Handle(), &m_hChildError.Handle(), &sa, 0))
		return SetErrorReturnCode();

	// Ensure the read handle to the pipe for STDERR is not inherited. 
	SetHandleInformation(m_hParentError.Handle(), HANDLE_FLAG_INHERIT, 0);

	// Create the child process. 
	if(CreateChildProcess(pcszApplicationName, pcszCommandLine) != RTNORM)
		return SetErrorReturnCode();

	m_dwLastError = 0;
	return RTNORM;
}

// Create a child process that uses the previously created pipes for STDIN and STDOUT.
BOOL CShellPipe::CreateChildProcess( const TCHAR * pcszApplicationName, const TCHAR * pcszCommandLine )
{
	// Set up members of the PROCESS_INFORMATION structure. 
//	PROCESS_INFORMATION pi;
	memset(&m_pi, 0, sizeof(PROCESS_INFORMATION));

	// Set up members of the STARTUPINFO structure. 
	// This structure specifies the STDIN and STDOUT handles for redirection.
	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.hStdError = m_hChildError.Handle();
	si.hStdOutput = m_hChildWrite.Handle();
	si.hStdInput = m_hChildRead.Handle();
	si.dwFlags = STARTF_USESTDHANDLES;

	// Expand environment variables in application name, so %ComSpec% would
	// be expanded to c:\Windows\System32\cmd.exe or what ever the value is.	
    TString sAppName;
	if(ExpandEnvironmentString(pcszApplicationName, sAppName) != RTNORM) {
		if(pcszApplicationName && !sAppName.size())
			return RTERROR;
	}

    TString sBuffer = pcszCommandLine;
    sBuffer.resize(_ENVIRONMENT_VARIABLE_LIMIT);    

	// Create the child process. 
	BOOL bVal = CreateProcess(sAppName.c_str(), &sBuffer[0], NULL, NULL, TRUE,
		0 /*CREATE_NEW_CONSOLE*/, NULL, NULL, &si, &m_pi);

	if(!bVal)
		return SetErrorReturnCode();

	// Close handles to the child process and its primary thread.
	// Some applications might keep these handles to monitor the status
	// of the child process, for example. 
	CloseHandle(m_pi.hProcess);
	CloseHandle(m_pi.hThread);

	return RTNORM;
}

// Writes pcszString to the child process's pipe for STDIN.
// This function can be called repeatedly to write more data,
// up until ReadShellPipe is called, at which point the
// write pipe as been closed, and no further writing to the
// pipe is allowed.
// 
int CShellPipe::WriteShellData( const TCHAR * pcszString )
{
#ifdef _UNICODE
	// convert unicode to ASCII
	DWORD dwWritten;
	DWORD nBufLength = WideCharToMultiByte(CP_UTF8, 0, pcszString, -1, 0, 0, NULL, NULL);
    std::string sBuf;
    sBuf.resize(nBufLength);
    // Use strlen so terminating NULL in not written to the buffer. Otherwise
    // string will include /r/n conversion and that is not wanted.
    // version of the string will include /r/n, and that is not wanted.
	WideCharToMultiByte(CP_UTF8, 0, pcszString, -1, &sBuf[0], nBufLength, NULL, NULL);
	BOOL bVal = WriteFile(m_hParentWrite.Handle(), sBuf.c_str(), strlen(sBuf.c_str()), &dwWritten, NULL);
	if(!bVal) {
		return SetErrorReturnCode();
	}
	return RTNORM;
#else
	DWORD dwWritten;
	if(!WriteFile(m_hParentWrite.Handle(), pcszString, strlen(pcszString), &dwWritten, NULL)) {
		return SetErrorReturnCode();
	}
	return RTNORM;
#endif
}



// Read output from the child process's pipe for STDOUT
// and write to sResults. This function can be called
// repeatedly to get more data. If no more data to read
// then it returns RTERROR
int CShellPipe::ReadShellData( TString & sResults )
{
	DWORD dwRead;
    std::string sBuf;
    sBuf.resize(ADS_BUFFER_SIZE);

	// Close the write end of the pipes before reading from the 
	// read end of the pipe, to control child process execution.
	// The pipe is assumed to have enough buffer space to hold the
	// data the child process has already written to it.
	if(!m_hChildWrite.CloseHandle())
		return SetErrorReturnCode();
	if(!m_hParentWrite.CloseHandle()) // needs to be closed if writing to pipe was done
		return SetErrorReturnCode();  // (thread will hang otherwise. Safe to just close it.

	if(!ReadFile(m_hParentRead.Handle(), &sBuf[0], ADS_BUFFER_SIZE - 1, &dwRead, NULL) || dwRead == 0)
		return SetErrorReturnCode();
	sBuf[dwRead] = _T('\0');
#ifdef _UNICODE
    DWORD dwSize = MultiByteToWideChar(CP_UTF8, 0, sBuf.c_str(), -1, 0, 0);
    sResults.resize(dwSize);
    MultiByteToWideChar(CP_UTF8, 0, sBuf.c_str(), -1, &sResults[0], dwSize);
#else
    sResults = sBuf.c_str();
#endif
	
	m_dwLastError = 0;
	return RTNORM;
}

int CShellPipe::CloseShell(void)
{
    WaitForSingleObject(m_pi.hProcess, INFINITE);

	m_hChildError.CloseHandle();
	m_hChildWrite.CloseHandle();
	m_hChildRead.CloseHandle();
	m_hParentWrite.CloseHandle();
	m_hParentRead.CloseHandle();
	m_hParentError.CloseHandle();

	m_dwLastError = 0;
	return RTNORM;
}

DWORD CShellPipe::GetLastShellError( TString & sResult )
{
	TCHAR * pszMsgBuffer = NULL;

	if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, m_dwLastError, 0,
		(LPTSTR) &pszMsgBuffer,
		0, NULL) == 0)
	{
		sResult = _T("Problem formating string. Last Shell Error value returned as RTLONG");
		return m_dwLastError;
	}

    sResult = pszMsgBuffer;
    LocalFree(pszMsgBuffer);
    trim<TString>(sResult);
    
	return m_dwLastError;
}
