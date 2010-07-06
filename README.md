Autolisp Shell Extension
------------------------
This extension enables Autolisp to run a shell command with stdout and stdin streams. The shelled process' output to stdout can be read into an Autolisp application, and Autolisp strings can be written to the shelled process input stream. Communicating through stream pipes allows more control of the shelled process than the built in AutoCAD SHELL command or using WScript.

ARX binaries are available of this extension for AutoCAD 2004-2010, from the [downloads button](http://github.com/pkohut/Autolisp-Shell-Extension/downloads)

    ARX Binary           | Runs in AutoCAD version(s)
    ____________________ | __________________________
    RunShell_2004.arx    | 2004 - 2006
    RunShell_2007.arx    | 2007 - 2009 32 bit
    RunShell_2008_64.arx | 2008 - 2009 64 bit
    RunShell_2010.arx    | 2010 32 bit
    RunShell_2010_64.arx | 2010 64 bit


New Commands Available to Autolisp
----------------------------------
__OpenShell__  
Opens a shell child process, with input and output streams piped.  
Usage: (OpenShell string1 string2)

* _string1_ the full path name of the application to shell. Environment variables are supported in the form of %ENV_VAR%.
* _string2_ the command line string to send the shelled application. Environment variables are supported in the form of %ENV_VAR%.
* returns _integer_ handle on success, _nil_ otherwise.

Example
> (setq handle (openshell "%comspec%" "/c dir"))  

Opens the cmd.exe program located usually at c:\windows\system32\cmd.exe", passes /c dir to the command line. /c is part of the command line option for cmd.exe. It runs the command then terminates the shell when finished. The "dir" part of the command line runs the directory command in the cmd.exe shell. If successful then an integer handle is returned which is used with the other shell extension commands.  An opened shell should be closed as soon as possible. Any opened shells associated with the drawing will be closed when the drawing is closed.

See CreateProcess documentation on MSDN for more information about the application name and command line strings and how they can be used.

__CloseShell__  
Closes a previously opened shell.  
Usage: (CloseShell handle)
	
* _handle_ the integer handle returned from the OpenShell command.
* returns _T_ if success, _nil_ otherwise.

Shells should be closed when no longer need. Each open shell is associated with a drawing that it was opened in. When a drawing is close any associated shells will be closed as well.

__ReadShellData__  
Reads the stdout stream from the shelled application.  
Usage: (ReadShellData handle)

* _handle_ the integer handle returned from the OpenShell command.
* returns a _string_ if success, _nil_ otherwise or no data left to retrieve.

Reads the stdout stream from the shelled command. Internally the buffer is limited to 504 bytes (503 because of the last NULL) as documented by the _acedRetStr_ function in the _ObjectARX SDK_. An Autolisp application can continue to call ReadShellData from a loop to retrieve all more and more of the stdout data. When no more stdout data is available _nil_ is returned.

_Note_ version 0.01 of the _Autolisp Shell Extension_ closes the stdin pipe when _ReadShellData_ is called, making future _WriteShellData_ calls to the shell return _nil_.

__WriteShellData__  
Writes to the stdin of the shell application  
Usage: (WriteShellData handle string)

* _handle_ the integer handle returned from the OpenShell command.
* _string_ to write to the shelled command, via the stdin stream.
* returns _T_ if success, _nil_ otherwise.

Writes a string to the stdin of the shell command. The string is written as is to child process stdio. Formatted line text needing ending carriage returns will need to be provided that way, this function _do not_ add ending carriage returns. Control ASCII characters from 0 - 31 can be sent using the octal escaped string (i.e., Ctrl-z is the string "\026").

The stdin string is close if _ReadStringData_ is called.

__GetLastShellError__  
Gets the last shell error code integer and if possible a string version  
Usage: (GetLastShellError)

* return a list. First item is an integer error code (see GetLastError on MSDN), the second item in the list is a formatted string of the error code. This function is a single instance and does not use a handle.


Installing ARX Binaries
----------
Refer to the AutoCAD documentation on loading ARX applications.

Compiling Source Code
---------------------
The source files include projects for building AutoCAD 2004, 2007, 2008 64 bit, 2010 32 bit, and 2010 64 bit, versions. To build the projects a properly setup ObjectARX developement platfom must be install (and everything that entails), VC Build Hook should also be install, google it for more info.

Sample Usage
------------

    ;;; Test 1
    ;;; Get the directory listing and print it to AutoCAD’s command line
    (defun c:test1 (/ handle sResult)
      (setq handle (openshell "c:\\windows\\system32\\cmd.exe" "/c dir"))
      (if (/= nil handle)
        (progn
          (while (/= nil (setq sResult (readshelldata handle)))
    	(princ sResult)
          )
          (closeshell handle)))
      (princ))

    ;;; Test 2
    ;;; Change directory to \Windows\System32, the get the directory
    ;;; list and print to autocads command line.
    ;;; Example only of how to combine commands with &&
    (defun c:test2 (/ handle sResult)
      (setq handle (openshell "c:\\windows\\system32\\cmd.exe" "/c cd \\Windows\\System32\\ && dir"))
      (if (/= nil handle)
        (progn
          (while (/= nil (setq sResult (readshelldata handle)))
    	(princ sResult))
          (closeshell handle)))
      (princ))

	  
    ;;; test 3
    ;;; Change directory to \Windows\System32, the sort the directory
    ;;; contents and print the results to Autocad's command line.
    ;;; Example using pipes and combining commands with &&
    (defun c:test3 (/ handle sResult)
      (setq handle (openshell "c:\\windows\\system32\\cmd.exe" "/c cd \\Windows\\System32\\ && dir | sort"))
      (if (/= nil handle)
        (progn
          (while (/= nil (setq sResult (readshelldata handle)))
    	(princ sResult))
          (closeshell handle)))
      (princ))

    ;;; test 4
    ;;; From the current directory type the contents of a file (\windows\system.ini),
    ;;; capturing the results to a variable, then printing the variable.
    ;;; system.ini is probably a small file, find a bigger one to test with.
    (defun c:test4 (/ handle sResult sContents)
      (setq handle (openshell "c:\\windows\\system32\\cmd.exe" "/c type \\Windows\\system.ini"))
      (if (/= nil handle)
        (progn
          (setq sContents "")
          (while (/= nil (setq sResult (readshelldata handle)))
    	(setq sContents (strcat sContents sResult)))
          (closeshell handle)))
      (princ sContents)
      (princ))

    ;;; Opens cmd.exe shell using the envirnment variable %comspec% if set.
    ;;; The sort program is run and input is provided via the WriteShellData
    ;;; function. This test provides a single string with carriage seperating
    ;;; the words of the input. This is one way to provide line buffered input.
    (defun c:Test5 (/ handle sResult)
      (setq    handle (openshell "%comspec%" "/c sort"))
      (if (/= nil handle)
        (progn
          (writeshelldata handle "There\nis\na\nfly\n")
          (setq sResult (readshelldata handle))
          (if (/= nil sResult)
        (princ sResult)))
        (closeshell handle))
      (princ))	  
	  
    ;;; Opens cmd.exe shell using the envirnment variable %comspec% if set.
    ;;; The sort program is run and input is provided to it through WriteShellData
	;;; by calling it multiple times.
    (defun c:Test6 (/ aList handl sResult)
      (setq aList (list "Lost" "in" "the" "swamp" "with" "the" "gaters"))
      (setq    handle (openshell "%comspec%" "/c sort"))
      (if (/= nil handle)
        (progn
          (foreach s aList (writeshelldata handle (strcat s "\n")))
          (setq sResult (readshelldata handle))
          (if (/= nil sResult)
        (princ sResult)))
        (closeshell handle))
      (princ))


__Final thoughts__  
It should be possible to run most types of shell programs and pipe their streams if they use stdio and stout.  Programs that don't use stdout and stdin (uses lots of printfs, etc.) probably won't work well.

By downloading and/or using this software you agree to the following terms of use:

Legal Stuff
-----------
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this software except in compliance with the License.
    You may obtain a copy of the License at
    
      http://www.apache.org/licenses/LICENSE-2.0
    
    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

Which basically means: whatever you do, I can't be held accountable if something breaks.


