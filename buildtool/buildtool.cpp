// DXGL
// Copyright (C) 2012 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#include <cstdio>
#include <iostream>
#include <windows.h>
#include "../common/releasever.h"

using namespace std;

int GetSVNRev(char *path)
{
	char pathbase[FILENAME_MAX+1];
	char pathin[FILENAME_MAX+1];
	char pathout[FILENAME_MAX+1];
	char command[1024];
	strncpy(pathbase,path,FILENAME_MAX);
	strncpy(pathin,path,FILENAME_MAX);
	strncpy(pathout,path,FILENAME_MAX);
	pathbase[strlen(pathbase)-7] = 0;
	strncat(pathin,"\\rev.in",FILENAME_MAX-strlen(pathin));
	strncat(pathout,"\\rev",FILENAME_MAX-strlen(pathin));
	strcpy(command,"subwcrev ");
	strcat(command,pathbase);
	strcat(command," ");
	strcat(command,pathin);
	strcat(command," ");
	strcat(command,pathout);
	STARTUPINFOA startinfo;
	ZeroMemory(&startinfo,sizeof(STARTUPINFOA));
	startinfo.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION process;
	if(CreateProcessA(NULL,command,NULL,NULL,FALSE,0,NULL,NULL,&startinfo,&process))
	{
		WaitForSingleObject(process.hProcess,INFINITE);
		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
		FILE *revfile = fopen(pathout,"r");
		if(!revfile)
		{
			cout << "WARNING:  Failed to create revision file" << endl;
			return 0;
		}
		char revstring[32];
		fgets(revstring,32,revfile);
		fclose(revfile);
		return atoi(revstring);
	}
	else
	{
		int result = MessageBoxA(NULL,"Could not find subwcrev.exe, would you like to download TortoiseSVN?","TortoiseSVN not found",
			MB_YESNO|MB_ICONWARNING);
		if(result == IDYES)
		{
			cout << "ERROR:  Please try again after installing TortoiseSVN." << endl;
			ShellExecuteA(NULL,"open","http://tortoisesvn.net/",NULL,NULL,SW_SHOWNORMAL);
			exit(-1);
		}
		else return 0;
	}
}

int ProcessHeaders(char *path)
{
	char pathin[FILENAME_MAX+1];
	char pathout[FILENAME_MAX+1];
	char buffer[1024];
	char verbuffer[20];
	char numstring[16];
	char *findptr;
	int revision = GetSVNRev(path);
	strncpy(pathin,path,FILENAME_MAX);
	strncpy(pathout,path,FILENAME_MAX);
	strncat(pathin,"\\version.h.in",FILENAME_MAX-strlen(pathin));
	strncat(pathout,"\\version.h",FILENAME_MAX-strlen(pathin));
	FILE *filein = fopen(pathin,"r");
	if(!filein)
	{
		cout << "ERROR:  Cannot read file " << pathin << endl;
		return errno;
	}
	FILE *fileout = fopen(pathout,"w");
	if(!fileout)
	{
		cout << "ERROR:  Cannot create file " << pathin << endl;
		return errno;
	}
	while(fgets(buffer,1024,filein))
	{
		findptr = strstr(buffer,"$MAJOR");
		if(findptr) strncpy(findptr,STR(DXGLMAJORVER) "\n",6);
		findptr = strstr(buffer,"$MINOR");
		if(findptr) strncpy(findptr,STR(DXGLMINORVER) "\n",6);
		findptr = strstr(buffer,"$POINT");
		if(findptr) strncpy(findptr,STR(DXGLPOINTVER) "\n",6);
		findptr = strstr(buffer,"$REVISION");
		if(findptr)
		{
			_itoa(revision,verbuffer,10);
			strcat(verbuffer,"\n");
			strncpy(findptr,verbuffer,9);
		}
		findptr = strstr(buffer,"$VERSTRING");
		if(findptr)
		{
			if(revision)
			{
				strcpy(verbuffer, "\"");
				strcat(verbuffer,DXGLSTRVER);
				strcat(verbuffer," r");
				_itoa(revision,numstring,10);
				strcat(verbuffer,numstring);
				strcat(verbuffer,"\"");
				strncpy(findptr,verbuffer,22);
			}
			else strncpy(findptr,"\"" DXGLSTRVER "\"\n",15);
		}
		fputs(buffer,fileout);
	}
	fclose(filein);
	filein = NULL;
	fclose(fileout);
	fileout = NULL;
	strncpy(pathin,path,FILENAME_MAX);
	strncpy(pathout,path,FILENAME_MAX);
	strncat(pathin,"\\version.nsh.in",FILENAME_MAX-strlen(pathin));
	strncat(pathout,"\\version.nsh",FILENAME_MAX-strlen(pathin));
	filein = fopen(pathin,"r");
	if(!filein)
	{
		cout << "ERROR:  Cannot read file " << pathin << endl;
		return errno;
	}
	fileout = fopen(pathout,"w");
	if(!fileout)
	{
		cout << "ERROR:  Cannot create file " << pathin << endl;
		return errno;
	}
	while(fgets(buffer,1024,filein))
	{
		findptr = strstr(buffer,"$PRODUCTVERSTRING");
		if(findptr) strncpy(findptr,"\"" DXGLSTRVER "\"\n",15);
		findptr = strstr(buffer,"$PRODUCTREVISION");
		if(findptr)
		{
			if(revision)
			{
				strcpy(verbuffer,"\"");
				_itoa(revision,numstring,10);
				strcat(verbuffer,numstring);
				strcat(verbuffer,"\"\n");
				strncpy(findptr,verbuffer,17);
			}
			else strncpy(findptr,"\"\"\n",17);
		}
#ifdef _DEBUG
		if(strstr(buffer,";!define _DEBUG")) strcpy(buffer,"!define _DEBUG");
#endif
		fputs(buffer,fileout);
	}
	fclose(filein);
	filein = NULL;
	fclose(fileout);
	fileout = NULL;
	return 0;
}

int MakeHelp(char *path)
{
	HKEY hKey;
	bool foundhhc = false;
	char hhcpath[(MAX_PATH+1)*2];
	DWORD buffersize = MAX_PATH+1;
	if(RegOpenKeyExA(HKEY_CURRENT_USER,"Software\\Microsoft\\HTML Help Workshop",0,KEY_READ,&hKey) == ERROR_SUCCESS)
	{
		if(RegQueryValueExA(hKey,"InstallDir",NULL,NULL,(LPBYTE)hhcpath,&buffersize) == ERROR_SUCCESS)
		{
			strcat(hhcpath,"\\hhc.exe");
			PROCESS_INFORMATION process;
			STARTUPINFOA startinfo;
			ZeroMemory(&startinfo,sizeof(STARTUPINFOA));
			startinfo.cb = sizeof(STARTUPINFOA);
			strcat(hhcpath," ");
			strcat(hhcpath,path);
			if(CreateProcessA(NULL,hhcpath,NULL,NULL,FALSE,0,NULL,NULL,&startinfo,&process))
			{
				foundhhc = true;
				WaitForSingleObject(process.hProcess,INFINITE);
				CloseHandle(process.hProcess);
				CloseHandle(process.hThread);
			}
		}
		RegCloseKey(hKey);
	}
	if(!foundhhc)
	{
		int result = MessageBoxA(NULL,"Could not find HTML Help Workshop, would you like to download it?","HTML Help Workshop not found",
			MB_YESNO|MB_ICONERROR);
		if(result == IDYES) ShellExecuteA(NULL,"open","http://tortoisesvn.net/",NULL,NULL,SW_SHOWNORMAL);
		cout << "ERROR:  HTML Help Compiler not found." << endl;
		return -1;
	}
	return 0;
}

int MakeInstaller(char *path)
{
	// Registry path:  HKEY_LOCAL_MACHINE\SOFTWARE\NSIS\(Default)
	return 0;
}


int main(int argc, char *argv[])
{

    cout << "DXGL Build Tool, version " << DXGLSTRVER << endl;
#ifdef _DEBUG
	cout << "Debug version." << endl;
#endif
	if(argc > 1)
	{
		if(!strcmp(argv[1],"makeheader"))
		{
			if(argc < 3)
			{
				cout << "ERROR:  No working directory specified." << endl;
				return 1;
			}
			return ProcessHeaders(argv[2]);
		}
		if(!strcmp(argv[1],"makehelp"))
		{
			if(argc < 3)
			{
				cout << "ERROR:  No working directory specified." << endl;
				return 1;
			}
			return MakeHelp(argv[2]);
		}
		if(!strcmp(argv[1],"makeinstaller"))
		{
			if(argc < 3)
			{
				cout << "ERROR:  No working directory specified." << endl;
				return 1;
			}
			return MakeInstaller(argv[2]);
		}
	}
	else
	{
	}
    return 0;
}
