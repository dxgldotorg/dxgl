// DXGL
// Copyright (C) 2012-2018 William Feely

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
#include <tchar.h>
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
	char verbuffer[36];
	char numstring[16];
	char *findptr;
	FILE *filein;
	FILE *fileout;
	int revision = GetSVNRev(path);
	BOOL nosign = FALSE;
	if (SIGNMODE < 1) nosign = TRUE;
	if ((SIGNMODE == 1) && DXGLBETA) nosign = TRUE;
	#ifdef _DEBUG
	if (SIGNMODE <= 2) nosign = TRUE;
	#endif
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
		if(findptr) strncpy(findptr,STR(DXGLPOINTVER) "\n",9);
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
				if (DXGLBETA) strcat(verbuffer, " Prerelease");
				strcat(verbuffer,"\"");
				strncpy(findptr,verbuffer,38);
			}
			else strncpy(findptr,"\"" DXGLSTRVER "\"\n",15);
		}
		if (DXGLBETA)
		{
			if (strstr(buffer, "//#define DXGLBETA")) strcpy(buffer, "#define DXGLBETA");
		}
		findptr = strstr(buffer, "$SIGNTOOL");
		if (findptr)
		{
			if (nosign) strncpy(findptr, "0", 10);
			else strncpy(findptr, "1", 10);
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
		if (findptr) strncpy(findptr, "\"" DXGLSTRVER "\"\n", 15);
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
		findptr = strstr(buffer, "$COMPILERTYPE");
		if(findptr)
		{
			#ifdef _MSC_VER
			#if (_MSC_VER == 1600)
			strncpy(findptr, "\"VC2010\"\n", 13);
			#elif (_MSC_VER == 1800)
			strncpy(findptr, "\"VC2013\"\n", 13);
			#elif (_MSC_VER == 1920)
			strncpy(findptr, "\"VC2019_0\"\n", 13);
			#elif ((_MSC_VER > 1900) && (_MSC_VER < 1916))
			#error Please update your Visual Studio 2017 to version 2017.9 before continuing.
			#elif (_MSC_VER > 1920)
			#pragma message ("Detected a newer version of Visual Studio, compiling assuming 2019.0.")
			strncpy(findptr, "\"VC2019_0\"\n", 13);
			#else
			strncpy(findptr, "\"UNKNOWN\"\n", 13);
			#endif
			#endif
		}
		if (DXGLBETA)
		{
			if(strstr(buffer,";!define _BETA")) strcpy(buffer,"!define _BETA\n");
		}
#ifdef _DEBUG
		if(strstr(buffer,";!define _DEBUG")) strcpy(buffer,"!define _DEBUG\n");
#endif
		fputs(buffer,fileout);
	}
	fclose(filein);
	filein = NULL;
	fclose(fileout);
	fileout = NULL;
	return 0;
}

void ParseHTMLFile(const TCHAR *filein, const TCHAR *fileout, const TCHAR *filetemplate)
{
	FILE *in;
	FILE *out;
	FILE *template;
	FILE *title;
	TCHAR filetitle[MAX_PATH * 2];
	TCHAR *ptr;
	char buffer[32768];
	in = _tfopen(filein, _T("r"));
	if (!in) return;
	template = _tfopen(filetemplate, _T("r"));
	if (!template)
	{
		fclose(in);
		return;
	}
	out = _tfopen(fileout, _T("w"));
	if (!out)
	{
		fclose(in);
		fclose(template);
		return;
	}
	_tcscpy(filetitle, filein);
	ptr = _tcsrchr(filetitle, _T('b'));
	*ptr = 0;
	_tcscat(filetitle, _T("title"));
	title = _tfopen(filetitle, _T("r"));
	fputs("Procesing help file ", stdout);
	_putts(filein);
	while (fgets(buffer, 32768, template))
	{
		if (!strncmp(buffer, "$HTMLCONTENT", 12))
		{
			rewind(in);
			while (fgets(buffer, 32768, in))
			{
				fputs(buffer, out);
			}
		}
		else if (!strncmp(buffer, "$HTMLTITLE", 10))
		{
			rewind(title);
			if (title)
			{
				fgets(buffer, 32768, title);
				fputs(buffer, out);
				fputs("\r\n", out);
			}
			else fputs("Unknown title\r\n", out);
		}
		else
		{
			fputs(buffer, out);
		}
	}
	fclose(in);
	fclose(template);
	fclose(out);
	if (title) fclose(title);
}

int ProcessHTMLFiles(const TCHAR *path, const TCHAR *templatepath)
{
	WIN32_FIND_DATA finddata;
	HANDLE hFind = NULL;
	TCHAR findpath[MAX_PATH+1];
	TCHAR filename[MAX_PATH * 2];
	TCHAR fileout[MAX_PATH * 2];
	TCHAR *ptr;
	_tcsncpy(findpath, path, MAX_PATH+1);
	_tcscat(findpath, _T("*.*"));
	hFind = FindFirstFile(findpath, &finddata);
	if (hFind == INVALID_HANDLE_VALUE) return 1;
	do
	{
		if (!_tcscmp(finddata.cFileName, _T("."))) continue;
		if (!_tcscmp(finddata.cFileName, _T(".."))) continue;
		_tcscpy(filename, path);
		_tcscat(filename, finddata.cFileName);
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			_tcscat(filename, _T("\\"));
			if (ProcessHTMLFiles(filename, templatepath))
			{
				FindClose(hFind);
				return 1;
			}
			continue;
		}
		ptr = _tcsrchr(filename, _T('.'));
		if (ptr)
		{
			if (!_tcscmp(ptr, _T(".htmlbody")))
			{
				_tcscpy(fileout, filename);
				ptr = _tcsrchr(fileout, _T('b'));
				*ptr = 0;
				ParseHTMLFile(filename, fileout, templatepath);
			}
		}
	} while (FindNextFile(hFind, &finddata));
	FindClose(hFind);
	return 0;
}

int MakeHelp(char *path)
{
	HKEY hKey;
	BOOL foundhhc = FALSE;
	char hhcpath[(MAX_PATH + 1) * 2];
	DWORD buffersize = MAX_PATH + 1;
	PROCESS_INFORMATION process;
	STARTUPINFOA startinfo;
	char htmlpath[MAX_PATH + 1];
	TCHAR htmlpath2[MAX_PATH + 1];
	char templatepath[MAX_PATH + 1];
	TCHAR templatepath2[MAX_PATH + 1];
	strcpy(htmlpath, path);
	if (strrchr(htmlpath, '\\')) *(strrchr(htmlpath, '\\')) = 0;
	strcpy(templatepath, htmlpath);
	strcat(htmlpath, "\\html\\");
	strcat(templatepath, "\\template.html");
#ifdef _UNICODE
	MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED, htmlpath, -1, htmlpath2, MAX_PATH + 1);
	MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED, templatepath, -1, templatepath2, MAX_PATH + 1);
#else
	strncpy(htmlpath2, htmlpath, MAX_PATH + 1);
	strncpy(templatepath2, templatepath, MAX_PATH + 1);
#endif
	if (ProcessHTMLFiles(htmlpath2, templatepath2))
	{
		puts("Error processing HTML files.");
		return -1;
	}
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
		if(result == IDYES) ShellExecuteA(NULL,"open","https://nsis.sourceforge.io/Main_Page",NULL,NULL,SW_SHOWNORMAL);
		puts("ERROR:  NSIS not found.");
		return -1;
	}
	return 0;
}

int SignEXE(char *exefile)
{
	PROCESS_INFORMATION process;
	STARTUPINFOA startinfo;
	const char signtoolsha1path[] = "signtool sign /t http://timestamp.comodoca.com ";
	const char signtoolsha256path[] = "signtool sign /tr http://timestamp.comodoca.com /td sha256 /fd sha256 /as ";
	char signpath[MAX_PATH + 80];
	BOOL nosign = FALSE;
	if (SIGNMODE < 1) nosign = TRUE;
	if ((SIGNMODE == 1) && DXGLBETA) nosign = TRUE;
	#ifdef _DEBUG
		if (SIGNMODE <= 2) nosign = TRUE;
	#endif
	if (nosign)
	{
		puts("Skipping file signature.");
		return 0;
	}
	strcpy(&signpath, &signtoolsha1path);
	strcat(&signpath, "\"");
	strncat(&signpath, exefile, MAX_PATH);
	strcat(&signpath, "\"");
	ZeroMemory(&startinfo, sizeof(STARTUPINFOA));
	startinfo.cb = sizeof(STARTUPINFOA);
	if (CreateProcessA(NULL, &signpath, NULL, NULL, FALSE, 0, NULL, NULL, &startinfo, &process))
	{
		WaitForSingleObject(process.hProcess, INFINITE);
		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
		Sleep(15000);
	}
	strcpy(&signpath, &signtoolsha256path);
	strcat(&signpath, "\"");
	strncat(&signpath, exefile, MAX_PATH);
	strcat(&signpath, "\"");
	ZeroMemory(&startinfo, sizeof(STARTUPINFOA));
	startinfo.cb = sizeof(STARTUPINFOA);
	if (CreateProcessA(NULL, &signpath, NULL, NULL, FALSE, 0, NULL, NULL, &startinfo, &process))
	{
		WaitForSingleObject(process.hProcess, INFINITE);
		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
		Sleep(15000);
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
		if (!strcmp(argv[1], "sign"))
		{
			if (argc < 3)
			{
				puts("ERROR:  file specified.");
				return 1;
			}
			return SignEXE(argv[2]);
		}
	}
	else
	{
	}
    return 0;
}
