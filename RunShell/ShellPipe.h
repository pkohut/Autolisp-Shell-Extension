/**	\file ShellPipe.h
*	\brief 
*/

/****************************************************************************/
/*	ShellPipe.h													*/
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


#pragma once
#include "ShellHandle.h"

/**
*	\brief Defines the class link this application with a child shell
*
*/
class CShellPipe
{
	enum { ADS_BUFFER_SIZE = 504 };	/**< Buffer size ARX docs say acedRetStr supports
									*/
public:
	CShellPipe(void);
	~CShellPipe(void);

	/**
	*	\brief Opens a shell instance
	*	\param[in] pcszApplicationName the application the child process will run
	*	\param[in] pcszCommandLine the command line parameters for the child process
	*	\returns RTNORM if successful, otherwise RTERROR
	*
	*	Function initializes a child process.  To create a DOS command shell the string
	*	pcszApplicationName must contain the full path to cmd.exe. pcszCommandLine contains
	*	a string to supply to the command line.  It is important that the string start with
	*	/c followed by the commands to execute.
	*
	*	\sa http://msdn.microsoft.com/en-us/library/ms682425%28VS.85%29.aspx for more details
	*	about the string pcszApplicationName and pcszCommandLine and how they can be used 
	*	\sa http://ss64.com/nt/cmd.html for information about the parameters cmd.exe excepts.
	*	\sa http://ss64.com/nt/ for a list of built in cmd.exe command available for Windows XP
	*	\sa http://en.wikibooks.org/wiki/Bourne_Shell_Scripting/Files_and_streams for information
	*	about how the Bourne Shell available on most POSTIX systems. Some methods shown do not
	*	work in cmd.exe, there might be Windows equivalent methods to do those tasks.
	*	\code
	*	(setq handle (openshell "c:\\windows\\system32\\cmd.exe" "/c dir"))
	*	\endcode
	*	\todo make pcszApplication name work with the %COMSPEC% enviornment variable. Right
	*	 now it isn't being expanded.
	*/
	int OpenShell(const TCHAR * pcszApplicationName, const TCHAR * pcszCommandLine );

	/**
	*	\brief Reads the child process stdout
	*	\param[out] sResults the value read from the child process stdout
	*	\returns RTNORM if successful and can be called again to read more data,
	*	otherwise RTERROR for errors or if nothing left to read.
	*	
	*	Reads ADS_BUFFER_SIZE chars of data from the child process stdout and places the result
	*	into sResult.
	*
	*	\code
	*	(setq s (readshelldata handle)) ;; handle obtained from ADS OpenShell function
	*	\endcode
	*/
	int ReadShellData(TString & sResults);

	/**
	*	\brief Writes the string to child process stdin
	*	\param[in] pcszString the string to write to stdin
	*	\returns RTNORM if successful and can be called again to write more data,
	*	otherwise RTERROR for errors.
	*
	*	Writes the string in pcszString to the child process pipe for STDIN
	*	This function can be called repeatedly to write more data,
	*	up until ReadShellPipe has been called, at which point the
	*	write pipe as been closed and no further writing to the pipe
	*	is allowed.
	*/
	int WriteShellData(const TCHAR * pcszString);

	/**
	*	\brief Closes a previously opened shell
	*	\returns RTNORM, always.
	*
	*	Closes all the member CShellHandle variables.
	*/
	int CloseShell(void);

	static DWORD GetLastShellError(TString & sResult);

private:

	/**
	*	\brief Initializes and creates a child process
	*
	*	Called by OpenShell
	*
	*	\code
	*	(closeshell handle) ;; handle from openshell
	*	\endcode
	*/
	BOOL CreateChildProcess(const TCHAR * pcszApplicationName, const TCHAR * pcszCommandLine);

	/**
	*	\brief Called whenever an error occurs
	*
	*	When an error in one of the other functions is detected this function
	*	should be called.  It gets the last error from the OS and stores the value
	*	so the host application can later retrieve it.
	*	\todo Create function that host application can call to retrieve the last
	*	error number and text string.
	*/
	int SetErrorReturnCode(void);

	CShellHandle m_hChildError;	/**< Child handle */
	CShellHandle m_hChildWrite;	/**< Child handle */
	CShellHandle m_hChildRead;	/**< Child handle */
	CShellHandle m_hParentWrite;	/**< Child handle */
	CShellHandle m_hParentRead;	/**< Child handle */
	CShellHandle m_hParentError;	/**< Child handle */

    PROCESS_INFORMATION m_pi;

	static DWORD m_dwLastError;	/**< Records the last error one of the functions triggered. */
};
