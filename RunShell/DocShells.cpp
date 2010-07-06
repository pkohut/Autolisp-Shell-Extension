/**	\file DocShells.cpp
*	\brief 
*/

/****************************************************************************/
/*	DocShells.cpp													*/
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


#include "StdAfx.h"
#include "ConsoleWindow.h"
#include "DocShells.h"

AcApDataManager<CDocShells> docShells;
int CDocShells::m_nNextHandle = 1;

typedef std::map<int, CShellPipe *> Shells;

CDocShells::CDocShells(void)
{
	// Do a one time only initialization of the console.
	// It should be deleted during the kUnloadAppMsg message
	if(!g_pConsole)
		g_pConsole = new CConsoleWindow;
}

CDocShells::~CDocShells(void)
{
}

int CDocShells::AddShell( CShellPipe * pShell )
{
	m_shells[m_nNextHandle] = pShell;
	return m_nNextHandle++;
}

int CDocShells::DeleteShell( int nHandle )
{
	Shells::iterator it = m_shells.find(nHandle);
	if(it!= m_shells.end()) {
		delete it->second;
		m_shells.erase(it);
		return RTNORM;
	}
	return RTERROR;
}

CShellPipe * CDocShells::GetShell( int nHandle ) const
{
	Shells::const_iterator it = m_shells.find(nHandle);
	if(it!= m_shells.end())
		return it->second;
	return NULL;
}
