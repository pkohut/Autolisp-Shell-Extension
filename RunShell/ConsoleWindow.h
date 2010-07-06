/**	\file ConsoleWindow.h
*	\brief 
*/

/****************************************************************************/
/*	ConsoleWindow.h															*/
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

/**	\brief Maintains a console window for this app
*	\note THERE SHOULD ONLY BE ONE INSTANCE OF THIS CLASS
*	
*	This class is responsible for adding attaching and freeing
*	a console window to this application process. If the
*	process does not have a console window attached then one
*	is attached, otherwise the existing one is used.
*/
class CConsoleWindow
{
public:
	/** \brief default constructor
	*
	*	Gets the HWND to the currently attached console and
	*	records that value to m_hPreviousConsoleWindow.
	*	If the returned HWND is NULL then a console window
	*	is attached.
	*/
	CConsoleWindow(void);

	/**	\brief default destructor
	*
	*	Frees the attached console if one was attached during
	*	the constructor.
	*/
	~CConsoleWindow(void);

private:	
	HWND m_hPreviousConsoleWindow;
};

extern const CConsoleWindow * g_pConsole;
