// DXGL
// Copyright (C) 2012-2013 William Feely

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

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <windows.h>
#include "../common/releasever.h"


int GetSVNRev(char *path)
{
	char pathbase[FILENAME_MAX+1];
	char pathin[FILENAME_MAX+1];
	char pathout[FILENAME_MAX+1];
	HKEY hKey;
	BOOL foundsvn = FALSE;
	char svnpath[(MAX_PATH + 1) * 4];
	DWORD buffersize = MAX_PATH + 1;
	STARTUPINFOA startinfo;
	PROCESS_INFORMATION process;
	FILE *revfile;
	char revstring[32];
	strncpy(pathbase, path, FILENAME_MAX);
	strncpy(pathin,path,FILENAME_MAX);
	pathin[FILENAME_MAX] = 0;
	strncpy(pathout,path,FILENAME_MAX);
	pathout[FILENAME_MAX] = 0;
	pathbase[strlen(pathbase)-7] = 0;
	strncat(pathin,"\\rev.in",FILENAME_MAX-strlen(pathin));
	pathin[FILENAME_MAX] = 0;
	strncat(pathout,"\\rev",FILENAME_MAX-strlen(pathin));
	pathout[FILENAME_MAX] = 0;
	if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,"Software\\TortoiseSVN",0,KEY_READ,&hKey) == ERROR_SUCCESS)
	{
		if(RegQueryValueExA(hKey,"Directory",NULL,NULL,(LPBYTE)svnpath,&buffersize) == ERROR_SUCCESS)
		{
			foundsvn = TRUE;
			strcat(svnpath,"bin\\subwcrev.exe ");
			strcat(svnpath,pathbase);
			strcat(svnpath," ");
			strcat(svnpath,pathin);
			strcat(svnpath," ");
			strcat(svnpath,pathout);
			ZeroMemory(&startinfo,sizeof(STARTUPINFOA));
			startinfo.cb = sizeof(STARTUPINFO);
			puts(svnpath);
			if(CreateProcessA(NULL,svnpath,NULL,NULL,FALSE,0,NULL,NULL,&startinfo,&process))
			{
				WaitForSingleObject(process.hProcess,INFINITE);
				CloseHandle(process.hProcess);
				CloseHandle(process.hThread);
				revfile = fopen(pathout,"r");
				if(!revfile)
				{
					puts("WARNING:  Failed to create revision file");
					RegCloseKey(hKey);
					return 0;
				}
				fgets(revstring,32,revfile);
				fclose(revfile);
				RegCloseKey(hKey);
				return atoi(revstring);
			}
			else foundsvn = FALSE;
		}
		RegCloseKey(hKey);
	}
	if(!foundsvn)
	{
		int result = MessageBoxA(NULL,"Could not find subwcrev.exe, would you like to download TortoiseSVN?","TortoiseSVN not found",
			MB_YESNO|MB_ICONWARNING);
		if(result == IDYES)
		{
			puts("ERROR:  Please try again after installing TortoiseSVN.");
			ShellExecuteA(NULL,"open","http://tortoisesvn.net/",NULL,NULL,SW_SHOWNORMAL);
			exit(-1);
		}
		else return 0;
	}
	return 0;
}

int ProcessHeaders(char *path)
{
	char pathin[FILENAME_MAX+1];
	char pathout[FILENAME_MAX+1];
	char buffer[1024];
	char verbuffer[20];
	char numstring[16];
	char *findptr;
	FILE *filein;
	FILE *fileout;
	int revision = GetSVNRev(path);
	strncpy(pathin,path,FILENAME_MAX);
	pathin[FILENAME_MAX] = 0;
	strncpy(pathout,path,FILENAME_MAX);
	pathout[FILENAME_MAX] = 0;
	strncat(pathin,"\\version.h.in",FILENAME_MAX-strlen(pathin));
	pathin[FILENAME_MAX] = 0;
	strncat(pathout,"\\version.h",FILENAME_MAX-strlen(pathin));
	pathout[FILENAME_MAX] = 0;
	filein = fopen(pathin,"r");
	if(!filein)
	{
		fputs("ERROR:  Cannot read file ", stdout);
		puts(pathin);
		return errno;
	}
	fileout = fopen(pathout,"w");
	if(!fileout)
	{
		fputs("ERROR:  Cannot create file ", stdout);
		puts(pathin);
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
	pathin[FILENAME_MAX] = 0;
	strncpy(pathout,path,FILENAME_MAX);
	pathout[FILENAME_MAX] = 0;
	strncat(pathin,"\\version.nsh.in",FILENAME_MAX-strlen(pathin));
	pathin[FILENAME_MAX] = 0;
	strncat(pathout,"\\version.nsh",FILENAME_MAX-strlen(pathin));
	pathout[FILENAME_MAX] = 0;
	filein = fopen(pathin,"r");
	if(!filein)
	{
		fputs("ERROR:  Cannot read file ", stdout);
		puts(pathin);
		return errno;
	}
	fileout = fopen(pathout,"w");
	if(!fileout)
	{
		fputs("ERROR:  Cannot create file ", stdout);
		puts(pathin);
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
	BOOL foundhhc = FALSE;
	char hhcpath[(MAX_PATH+1)*2];
	DWORD buffersize = MAX_PATH+1;
	PROCESS_INFORMATION process;
	STARTUPINFOA startinfo;
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\HTML Help Workshop", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if(RegQueryValueExA(hKey,"InstallDir",NULL,NULL,(LPBYTE)hhcpath,&buffersize) == ERROR_SUCCESS)
		{
			strcat(hhcpath,"\\hhc.exe");
			ZeroMemory(&startinfo,sizeof(STARTUPINFOA));
			startinfo.cb = sizeof(STARTUPINFOA);
			strcat(hhcpath," ");
			strncat(hhcpath,path,MAX_PATH);
			if(CreateProcessA(NULL,hhcpath,NULL,NULL,FALSE,0,NULL,NULL,&startinfo,&process))
			{
				foundhhc = TRUE;
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
		if(result == IDYES) ShellExecuteA(NULL,"open", "http://www.microsoft.com/en-us/download/details.aspx?id=21138"
			,NULL,NULL,SW_SHOWNORMAL);
		puts("ERROR:  HTML Help Compiler not found.");
		return -1;
	}
	return 0;
}

int MakeInstaller(char *path)
{
	HKEY hKey;
	BOOL foundnsis = FALSE;
	char nsispath[(MAX_PATH+1)*2];
	DWORD buffersize = MAX_PATH+1;
	PROCESS_INFORMATION process;
	STARTUPINFOA startinfo;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\NSIS", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		if(RegQueryValueExA(hKey,"",NULL,NULL,(LPBYTE)nsispath,&buffersize) == ERROR_SUCCESS)
		{
			strcat(nsispath,"\\makensis.exe");
			ZeroMemory(&startinfo,sizeof(STARTUPINFOA));
			startinfo.cb = sizeof(STARTUPINFOA);
			strcat(nsispath," ");
			strncat(nsispath,path,MAX_PATH);
			if(CreateProcessA(NULL,nsispath,NULL,NULL,FALSE,0,NULL,NULL,&startinfo,&process))
			{
				foundnsis = TRUE;
				WaitForSingleObject(process.hProcess,INFINITE);
				CloseHandle(process.hProcess);
				CloseHandle(process.hThread);
			}
		}
		RegCloseKey(hKey);
	}
	if(!foundnsis)
	{
		int result = MessageBoxA(NULL,"Could not find NSIS, would you like to download it?","NSIS not found",
			MB_YESNO|MB_ICONERROR);
		if(result == IDYES) ShellExecuteA(NULL,"open","http://nsis.sourceforge.net/Main_Page",NULL,NULL,SW_SHOWNORMAL);
		puts("ERROR:  NSIS not found.");
		return -1;
	}
	return 0;
}


int main(int argc, char *argv[])
{

	fputs("DXGL Build Tool, version ", stdout);
	puts(DXGLSTRVER);
#ifdef _DEBUG
	puts("Debug version.");
#endif
	if(argc > 1)
	{
		if(!strcmp(argv[1],"makeheader"))
		{
			if(argc < 3)
			{
				puts("ERROR:  No working directory specified.");
				return 1;
			}
			return ProcessHeaders(argv[2]);
		}
		if(!strcmp(argv[1],"makehelp"))
		{
			if(argc < 3)
			{
				puts("ERROR:  No working directory specified.");
				return 1;
			}
			return MakeHelp(argv[2]);
		}
		if(!strcmp(argv[1],"makeinstaller"))
		{
			if(argc < 3)
			{
				puts("ERROR:  No working directory specified.");
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
