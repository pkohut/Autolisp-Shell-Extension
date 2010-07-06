/**	\file DocShells.h
*	\brief 
*/

/****************************************************************************/
/*	DocShells.h																*/
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
#include "ShellPipe.h"

class CConsoleWindow;

/** \brief Tracks opened shells and assigns handles
*
*	
*/
class CDocShells
{
public:
	CDocShells(void);
	~CDocShells(void);

	/** \brief Adds a shell to the collection
	*	\param pShell Pointer to CShellPipe to be added to
	*	the CDocShells collection.
	*	\returns a handle (key value) to the collection.
	*
	*	Uses m_nNextHandle as the key value associated with pShell, then
	*	increments m_nNextHandle.
	*/
	int AddShell(CShellPipe * pShell);

	/** \brief Delete a shell from the collection
	*	\param handle to the shell to delete
	*	\returns RTNORM if the shell was deleted, RTERROR otherwise
	*
	*	The handle is one that was acquired from AddShell. The CShellPipe
	*	associated with the handle is deleted and its destructor called.
	*/
	int DeleteShell(int nHandle);

	/** \brief Get a CShellPipe instance from a handle
	*	\param handle previously acquired from AddShell
	*	\returns CShellPipe instance associated with the handle,
	*	or NULL if the handle is invalid.
	*/
	CShellPipe * GetShell(int nHandle) const;

private:
	std::map<int, CShellPipe *> m_shells; /**< collection of CShellPipe s */
	static int m_nNextHandle; /**< The next handle value to be assigned. */
};

extern AcApDataManager<CDocShells> docShells; // makes CDocShells MDI aware in Autocad.

