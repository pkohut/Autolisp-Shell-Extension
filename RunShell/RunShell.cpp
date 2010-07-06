/**	\file RunShell.cpp
*	\brief 
*/

/****************************************************************************/
/*	RunShell.cpp															*/
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


#include "stdafx.h"
#include "tchar.h"
#include "DocShells.h"
#include "ConsoleWindow.h"

#if defined(ARX2004) || defined(ARX2005) || defined(ARX2006)
#pragma comment(linker, "/export:_acrxGetApiVersion,PRIVATE")
#pragma comment(linker, "/export:_acrxEntryPoint,PRIVATE")
#endif

#define ELEMENTS(array) (sizeof(array)/sizeof((array)[0]))

struct func_entry { ACHAR *func_name; int (*func) (struct resbuf *); };

// forward references
int OpenShell(resbuf * pRb);
int CloseShell(resbuf * pRb);
int ReadShellData(resbuf * pRb);
int GetLastShellError(resbuf * pRb);
int WriteShellData(resbuf * pRb);

int DoFunc(void);
int FuncLoad(void);

// ADS available commands and function pointers
static struct func_entry func_table[] = {
    {_T("OpenShell"), OpenShell},
    {_T("CloseShell"), CloseShell},
    {_T("ReadShellData"), ReadShellData},
    {_T("WriteShellData"), WriteShellData},
    {_T("GetLastShellError"), GetLastShellError},    
};

extern "C" AcRx::AppRetCode
acrxEntryPoint(AcRx::AppMsgCode msg, void * appId)
{
    switch(msg) {
case AcRx::kInitAppMsg:
    acrxDynamicLinker->unlockApplication(appId);
    acrxDynamicLinker->registerAppMDIAware(appId);
    break;
case AcRx::kUnloadAppMsg:
    // delete the single instance of CConsoleWindow
    // before exiting AutoCAD.
    if(g_pConsole) {
        delete g_pConsole;
        g_pConsole = NULL;
    }
    break;
case AcRx::kInvkSubrMsg:
    DoFunc();
    break;
case AcRx::kLoadDwgMsg:
    FuncLoad();
    }
    return AcRx::kRetOK;
}

static int FuncLoad(void)
{
    for(int i = 0; i < ELEMENTS(func_table); ++i) {
        if(!acedDefun(func_table[i].func_name, (short) i))
            return RTERROR;
    }
    return RTNORM;
}

static int DoFunc(void)
{
    int val = acedGetFunCode();
    if(val < 0 || val >= ELEMENTS(func_table)) {
        acdbFail(_T("Received nonexistent function code."));
        return RTERROR;
    }

    resbuf * pRb = acedGetArgs();
    val = (*func_table[val].func)(pRb);
    acutRelRb(pRb);
    return val;


}

// Helper function for RESBUF's that contain strings
int GetResBufValue(const resbuf * pRb, TString & sValue)
{
    if(!pRb)
        return RTERROR;
    if(pRb->restype != RTSTR)
        return RTERROR;
    sValue = pRb->resval.rstring;
    return RTNORM;
}

// Helper function for RESBUF's that contains RTSHORT or RTLONG
int GetResBufValue(const resbuf * pRb, int & nValue)
{
    if(!pRb)
        return RTERROR;
    if(pRb->restype == RTLONG) {
        nValue = pRb->resval.rlong;
        return RTNORM;
    }
    if(pRb->restype == RTSHORT) {
        nValue = pRb->resval.rint;
        return RTNORM;
    }
    return RTERROR;
}


/** \brief The gateway function between Autolisp and CShellPipe::OpenShell
*	\param pRb a resbuf that must have 2 link that are strings
*	\returns RTRSLT meaning a result is being returned.
*
*	When this function makes calls to any other function and the return
*	value from those functions are RTERROR, then this function will
*	return RTRSLT with acedRetNil(). The Autolisp function that called
*	this will receive a Nil return.
*
*	Otherwise if every is OK (all functions called return RTNORM) then
*	this function will return RTRSLT with acedRetInt() which is the
*	handle associated with CShellPipe instance.
*	
*	The handle returned to Autolisp is used by readshelldata and
*	closeshell.
*/
static int OpenShell(resbuf * pRb)
{
    TCHAR * pcszApplicationName = NULL, * pcszCommandLine = NULL;
    // Get the application name string
    if(!pRb || pRb->restype != RTSTR) {
        acedRetNil();
        return RSRSLT;
    }
    pcszApplicationName = pRb->resval.rstring;

    // get the command line string
    if(!pRb->rbnext || pRb->rbnext->restype != RTSTR) {
        acedRetNil();
        return RSRSLT;
    }
    pcszCommandLine = pRb->rbnext->resval.rstring;

    // Create a new instance of CShellPipe
    CShellPipe * pPipe = new CShellPipe;
    if(pPipe->OpenShell(pRb->resval.rstring, pRb->rbnext->resval.rstring) != RTNORM) {
        delete pPipe;
        acedRetNil();
        return RSRSLT;
    }

    int nHandle = docShells.docData().AddShell(pPipe);
    if(!nHandle)
        acedRetNil();
    else
        acedRetInt(nHandle);
    return RSRSLT;
}


/** \brief Closes a CShellPipe instance
*	\param pRb a resbuf containing the handle value
*	\returns RTRSLT meaning a result is being returned.
*
*	The pRb must be a RTSHORT or RTLONG value that is a handle
*	to an associated CShellPipe to be close.

*	When this function makes calls to any other function and the return
*	value from those functions are RTERROR, then this function will
*	return RTRSLT with acedRetNil(). The Autolisp function that called
*	this will receive a Nil return.
*
*	Otherwise if every is OK (all functions called return RTNORM) then
*	this function will return RTRSLT with acedRetT().
*	
*	The calling Autolisp function will receive T as a returned value.
*/
static int CloseShell(resbuf * pRb)
{
    int nHandle = 0;
    // get handle, bail if not a RTLONG or RTSHORT
    if(GetResBufValue(pRb, nHandle) != RTNORM) {
        acedRetNil();
        return RSRSLT;
    }

    // use the handle to delete the CShellPipe associated with it
    if(docShells.docData().DeleteShell(nHandle) == RTNORM)
        acedRetT();
    else
        acedRetNil();
    return RSRSLT;
}


/** \brief Reads data from a CShellPipe instance
*	\param pRb a resbuf containing the handle value
*	\returns RTRSLT meaning a result is being returned.
*
*	The pRb must be a RTSHORT or RTLONG value that is a handle
*	to an associated CShellPipe to be read data from.

*	When this function makes calls to any other function and the return
*	value from those functions are RTERROR, then this function will
*	return RTRSLT with acedRetNil(). The Autolisp function that called
*	this will receive a Nil return.
*
*	Otherwise if everything is OK (all functions called return RTNORM) then
*	this function will return RTRSLT with acedRetStr() that contains
*	the read data.
*	
*	The calling Autolisp function will receive a string as a returned value,
*	which is the read data.
*/
static int ReadShellData(resbuf * pRb)
{
    int nHandle = 0;
    // get the handle, bail if pRb is not RTLONG or RTSHORT
    if(GetResBufValue(pRb, nHandle) != RTNORM) {
        acedRetNil();
        return RSRSLT;
    }

    // use the handle to get the associated CShellPipe instance.
    CShellPipe * pShell = docShells.docData().GetShell(nHandle);
    if(!pShell) {
        acedRetNil();
        return RSRSLT;
    }

    // read the data from the CShellPipe instance
    TString sResults;
    if(pShell->ReadShellData(sResults) != RTNORM) {
        acedRetNil();
        return RSRSLT;
    }

    // make the result available to the calling Autolisp function.
    acedRetStr(sResults.c_str());
    return RSRSLT;
}

/** \brief Writes data to the CShellPipe instance
*	\param pRb a resbuf containing the handle value, and string to write
*	\returns RTRSLT meaning a result is being returned. The calling Autolisp
*	function will receive a T as a returned value if the function succeeds,
*	otherwise Nil is returned
*
*	The pRb must be a list containing a RTSHORT or RTLONG for the first value
*	that is the handle associated to a CShellPipe instance.
*	The second value must be a RTSTR for the second value which is the value
*	to be written to Stdin of CPipeShell.
*
*	When this function makes calls to any other function and the return
*	value from those functions are RTERROR, then this function will
*	return RTRSLT with acedRetNil(). The Autolisp function that called
*	this will receive a Nil return.
*
*	Otherwise if everything is OK (all functions called return RTNORM) then
*	this function will return RTRSLT with acedRetStr() that contains
*	the read data.
*	
*/
static int WriteShellData(resbuf * pRb)
{
    int nHandle = 0;
    // get the handle, bail if pRb is not RTLONG or RTSHORT
    if(GetResBufValue(pRb, nHandle) != RTNORM) {
        acedRetNil();
        return RSRSLT;
    }

    // Get the string to send to stdio. Bail if it is NULL
    // or not a string
    TCHAR * pcszSendString = NULL;
    if(!pRb->rbnext || pRb->rbnext->restype != RTSTR) {
        acedRetNil();
        return RSRSLT;
    }
    pcszSendString = pRb->rbnext->resval.rstring;


    // use the handle to get the associated CShellPipe instance.
    CShellPipe * pShell = docShells.docData().GetShell(nHandle);
    if(!pShell) {
        acedRetNil();
        return RSRSLT;
    }

    // read the data from the CShellPipe instance
    if(pShell->WriteShellData(pcszSendString) != RTNORM) {
        acedRetNil();
        return RSRSLT;
    }

    acedRetT();
    return RSRSLT;
}

static int GetLastShellError(resbuf * pRb)
{
    TString sResult;
    DWORD dwError = CShellPipe::GetLastShellError(sResult);

    resbuf * pErrorRb = acutBuildList(RTLONG, dwError, RTSTR, sResult.c_str(), 0);
    acedRetList(pErrorRb);
    return RSRSLT;
}